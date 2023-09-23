//
//  CrosECSensorHub.hpp
//  CrosEC
//
//  Created by Gwydien on 9/20/23.
//

#pragma once

#include <IOKit/IOService.h>
#include <IOKit/graphics/IOFramebuffer.h>

#include "CrosECDeviceNub.hpp"

#define packed __attribute__ ((aligned (1)))

enum {
    CrosSensor_Cmd_MotionDump = 0,
    CrosSensor_Cmd_SensorInfo = 1,
    CrosSensor_Cmd_SensorData = 6,
};

enum TabletOrientation_t {
    CrosSensor_Ori_Lanscape,
    CrosSensor_Ori_PortraitLeft,
    CrosSensor_Ori_LandscapeFlipped,
    CrosSensor_Ori_PortraitRight
};

/* List of motion sensor types. */
enum {
    CrosSensor_Type_Accel = 0,
    CrosSensor_Type_Gyro = 1,
    CrosSensor_Type_Mag = 2,
    CrosSensor_Type_Prox = 3,
    CrosSensor_Type_Light = 4,
    CrosSensor_Type_Activity = 5,
    CrosSensor_Type_Baro = 6,
    CrosSensor_Type_Sync = 7,
    CrosSensor_Type_Light_RGB = 8,
    CrosSensor_Type_Max
};

/* List of motion sensor locations. */
enum {
    CrosSensor_Loc_Base = 0,
    CrosSensor_Loc_Lid = 1,
    CrosSensor_Loc_Camera = 2,
    CrosSensor_Loc_Max
};

enum TabletMode_t {
    CrosSensor_Mode_LaptopMode = 0,
    CrosSensor_Mode_TabletMode = 1,
    CrosSensor_Mode_Unknown = 2,
};

struct CrosSensorRequest {
    uint8_t cmd;
    uint8_t sensorIdx;
} packed;

struct CrosSensorDumpRequest {
    uint8_t cmd;
    uint8_t maxSensorCount;
} packed;

struct CrosSensorAccelDataResponse {
    uint8_t flags;
    uint8_t sensorIdx;
    int16_t x;
    int16_t y;
    int16_t z;
} packed;

struct CrosSensorDumpResponse {
    uint8_t moduleFlags;
    uint8_t sensorCount;
} packed;

struct CrosSensorInfoResponse {
    uint8_t type;
    uint8_t location;
    uint8_t chip;
} packed;

/*
  * Cut down version of the Cros EC Sensor Hub which only checks for
  * 1) Tablet Mode
  * 2) Orientation of the sensor contained in the lid
 */
class CrosECSensorHub : public IOService {
    OSDeclareDefaultStructors(CrosECSensorHub);
public:
    CrosECSensorHub *probe(IOService *provider, int32_t *score) override;
    bool start(IOService *provider) override;
    void stop(IOService *provider) override;
    
    IOReturn setPowerState(unsigned long ordinal, IOService *dev) override;
    IOReturn message(uint32_t message, IOService *device, void *arg) override;
private:
    CrosECDeviceNub *nub {nullptr};
    
    IOWorkLoop *workloop {nullptr};
    IOTimerEventSource *timer {nullptr};
    IOCommandGate *commandGate {nullptr};
    
    IOFramebuffer *framebuffer {nullptr};
    
    size_t lidIdx {0xFFFFFFFF};
    
    TabletMode_t tabletMode {CrosSensor_Mode_Unknown};
    TabletOrientation_t tabletOrientation {CrosSensor_Ori_Lanscape};
    
    void readLidSensorGated(IOTimerEventSource *sender);
    IOReturn setTabletModeGated();
    
    IOFramebuffer* getFramebuffer();
    void rotateDevice(TabletOrientation_t newOrientation);
};
