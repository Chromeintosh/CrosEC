//
//  CrosECSensorHub.cpp
//  CrosEC
//
//  Created by Gwydien on 9/20/23.
//

#include <IOKit/IOWorkLoop.h>
#include <IOKit/IOTimerEventSource.h>
#include <IOKit/IORegistryEntry.h>
#include <IOKit/acpi/IOACPITypes.h>
#include <IOKit/graphics/IOGraphicsTypes.h>

#include "CrosECSensorHub.hpp"

#define super IOService
OSDefineMetaClassAndStructors(CrosECSensorHub, IOService);

#define ABS(a) ((a) < 0 ? -(a) : (a))

constexpr uint32_t TIMER_MS = 250;
constexpr int32_t RAW_TO_1G = 16383;
constexpr uint32_t kIOFBSetTransform = 0x00000400;

CrosECSensorHub *CrosECSensorHub::probe(IOService *provider, int32_t *score) {
    CrosSensorDumpRequest request {
        CrosSensor_Cmd_MotionDump, 0
    };
    CrosSensorDumpResponse response;
    CrosSensorRequest sensorInfoRequest;
    CrosSensorInfoResponse sensorInfoResponse;
    
    if (super::probe(provider, score) == nullptr) {
        return nullptr;
    }
    
    nub = OSDynamicCast(CrosECDeviceNub, provider);
    if (nub == nullptr) {
        return nullptr;
    }
    
    if (nub->getACPIDevice() == nullptr) {
        return nullptr;
    }
    
    //
    // Grab index for Lid sensor
    //
    
    IOReturn result = nub->transferCommand(kCrosCMD_Motion_Sense, 1, &request, sizeof(request), &response, sizeof(response));

    if (result != kIOReturnSuccess) {
        IOLog("CROS - ERROR!\n");
        return nullptr;
    }
    
    IOLog("CROS - Returned %d status with %d sensors\n", result, response.sensorCount);
    
    for (int i = 0; i < response.sensorCount; i++) {
        sensorInfoRequest.cmd = CrosSensor_Cmd_SensorInfo;
        sensorInfoRequest.sensorIdx = i;
        
        result = nub->transferCommand(kCrosCMD_Motion_Sense, 1, &sensorInfoRequest, sizeof(sensorInfoRequest), &sensorInfoResponse, sizeof(sensorInfoResponse));
        if (result != kIOReturnSuccess) {
            IOLog("CROS - Sensor Info Error\n");
            continue;
        }
        
        // We only use the lid accelerometer for rotating the screen
        if (sensorInfoResponse.location == CrosSensor_Loc_Lid && sensorInfoResponse.type == CrosSensor_Type_Accel) {
            lidIdx = i;
            IOLog("CROS - Found Lid Accelerometer at idx %d\n", i);
            break;
        }
    }
    
    return lidIdx < 0xFFFFFFFF ? this : nullptr;
}

bool CrosECSensorHub::start(IOService *provider) {
    if (!super::start(provider)) {
        return false;
    }
    
    IOACPIPlatformDevice *acpiDev = nub->getACPIDevice();
    if (!attach(acpiDev)) {
        return false;
    }
    
    framebuffer = getFramebuffer();
    if (framebuffer == nullptr) {
        return false;
    }
    
    // Grab Workloop from CrosECBus
    workloop = getWorkLoop();
    if (workloop == nullptr) {
        return false;
    }
    
    timer = IOTimerEventSource::timerEventSource(this, OSMemberFunctionCast(IOTimerEventSource::Action, this, &CrosECSensorHub::readLidSensorGated));
    if (timer == nullptr || workloop->addEventSource(timer) != kIOReturnSuccess) {
        return false;
    }
    
    commandGate = IOCommandGate::commandGate(this, OSMemberFunctionCast(Action, this, &CrosECSensorHub::setTabletModeGated));
    if (commandGate == nullptr || workloop->addEventSource(commandGate) != kIOReturnSuccess) {
        return false;
    }
    
    // Set initial state and kick off sensor timer if needed
    setTabletModeGated();
    
    registerService();
    return true;
}

void CrosECSensorHub::stop(IOService *provider) {
    if (workloop != nullptr) {
        if (timer != nullptr) {
            timer->disable();
            workloop->removeEventSource(timer);
            timer->release();
            timer = 0;
        }
    
        if (commandGate != nullptr) {
            commandGate->disable();
            workloop->removeEventSource(commandGate);
            commandGate->release();
            commandGate = 0;
        }
    }
    
    super::stop(provider);
}

IOReturn CrosECSensorHub::setPowerState(unsigned long ordinal, IOService *dev) {
    return kIOReturnSuccess;
}

IOFramebuffer* CrosECSensorHub::getFramebuffer() {
    IORegistryEntry* display = NULL;
    IOFramebuffer* framebuffer = NULL;
    
    OSDictionary *match = serviceMatching("IOBacklightDisplay");
    OSIterator *iterator = getMatchingServices(match);
    
    OSSafeReleaseNULL(match);
    if (iterator == nullptr) {
        return nullptr;
    }
    
    display = OSDynamicCast(IORegistryEntry, iterator->getNextObject());
    OSSafeReleaseNULL(iterator);
    
    // Get parent framebuffer
    if (display == nullptr) {
        return nullptr;
    }
    
    IORegistryEntry *entry = display->getParentEntry(gIOServicePlane)->getParentEntry(gIOServicePlane);
    if (entry == nullptr) {
        return nullptr;
    }
    
    framebuffer = reinterpret_cast<IOFramebuffer*>(entry->metaCast("IOFramebuffer"));

    return framebuffer;
}

void CrosECSensorHub::rotateDevice(TabletOrientation_t newOrientation) {
    if (newOrientation == tabletOrientation) {
        return;
    }
    
    IOOptionBits probe = kIOFBSetTransform;
    
    switch(newOrientation) {
        case CrosSensor_Ori_Lanscape: probe |= (kIOScaleRotate0 << 16); break;
        case CrosSensor_Ori_PortraitLeft: probe |= (kIOScaleRotate90 << 16); break;
        case CrosSensor_Ori_LandscapeFlipped: probe |= (kIOScaleRotate180 << 16); break;
        case CrosSensor_Ori_PortraitRight: probe |= (kIOScaleRotate270 << 16); break;
        default: return;
    }
    
    framebuffer->requestProbe(probe);
    tabletOrientation = newOrientation;
}

void CrosECSensorHub::readLidSensorGated(IOTimerEventSource *sender) {
    CrosSensorRequest request;
    CrosSensorAccelDataResponse response;
    
    request.cmd = CrosSensor_Cmd_SensorData;
    request.sensorIdx = lidIdx;
    
    IOReturn result = nub->transferCommand(kCrosCMD_Motion_Sense, 1, &request, sizeof(request), &response, sizeof(response));
    if (result != kIOReturnSuccess) {
        IOLog("CROS - Failed to read sensor data\n");
        
    /*
     From Chapter 6 in https://www.nxp.com/docs/en/application-note/AN3461.pdf
     (|Gpz| < 0.5g) AND (Gpx > 0.5g) AND (|Gpy| < 0.4g): Change orientation to Top
     (|Gpz| < 0.5g) AND (Gpx < -0.5g) AND (|Gpy| < 0.4g): Change orientation to Bottom
     (|Gpz| < 0.5g) AND (Gpy > 0.5g) AND (|Gpx| < 0.4g): Change orientation to Right
     (|Gpz| < 0.5g) AND (Gpy < -0.5g) AND (|Gpx| < 0.4g): Change orientation to Left.
     */
   // Make sure tablet is not laying down
    } else if (ABS(response.z) < RAW_TO_1G / 2) {
        if (response.x > RAW_TO_1G / 2 && ABS(response.y) < RAW_TO_1G * 4 / 10) {
           rotateDevice(CrosSensor_Ori_PortraitRight);
        } else if (response.x < -RAW_TO_1G / 2 && ABS(response.y) < RAW_TO_1G * 4 / 10) {
           rotateDevice(CrosSensor_Ori_PortraitLeft);
        } else if (response.y > RAW_TO_1G / 2 && ABS(response.x) < RAW_TO_1G * 4 / 10) {
           rotateDevice(CrosSensor_Ori_Lanscape);
        } else if (response.y < -RAW_TO_1G / 2 && ABS(response.x) < RAW_TO_1G * 4 / 10) {
           rotateDevice(CrosSensor_Ori_LandscapeFlipped);
        }
        
        IOLog("CROS - X: %d Y: %d Z: %d\n", response.x, response.y, response.z);
    }
    
    timer->setTimeoutMS(TIMER_MS);
}

IOReturn CrosECSensorHub::setTabletModeGated() {
    uint32_t tbmcVal;
    IOACPIPlatformDevice *acpiDev = nub->getACPIDevice();
    IOReturn ret;
    
    if (acpiDev == nullptr) {
        return kIOReturnNoDevice;
    }
    
    ret = acpiDev->evaluateInteger("TBMC", &tbmcVal);
    if (ret != kIOReturnSuccess) {
        return ret;
    }
    
    TabletMode_t newMode = (TabletMode_t) tbmcVal;
    if (newMode == tabletMode) {
        return kIOReturnSuccess;
    }
    
    tabletMode = newMode;
    
    setProperty("Tablet Mode", tabletMode == CrosSensor_Mode_TabletMode ? "Tablet" : "Laptop");
    if (tabletMode == CrosSensor_Mode_TabletMode) {
        timer->enable();
        timer->setTimeoutMS(TIMER_MS);
    } else {
        timer->cancelTimeout();
        timer->disable();
        rotateDevice(CrosSensor_Ori_Lanscape);
    }
    
    return kIOReturnSuccess;
}

IOReturn CrosECSensorHub::message(uint32_t message, IOService *device, void *arg) {
    
    switch (message) {
        case kIOACPIMessageDeviceNotification:
            IOLog("Cros - Message from ACPI %llx\n", (uint64_t) arg);
            return commandGate->runCommand();
    }
    
    return super::message(message, device, arg);
}
