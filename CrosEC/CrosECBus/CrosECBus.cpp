
#include <architecture/i386/pio.h>
#include <IOKit/IOBufferMemoryDescriptor.h>

#include "CrosECBus.hpp"
#include "CrosECDeviceNub.hpp"

#define super IOService
OSDefineMetaClassAndStructors(CrosECBus, IOService);

IOService *CrosECBus::probe(IOService *provider, int32_t *score) {
    if (super::probe(provider, score) == nullptr) {
        IOLog("CROS - Invalid super probe\n");
        return nullptr;
    }
    
    /* TODO: Check for MEC transport */
    
    if (inb(kCrosLPC_MemMap_Base + kCrosCMD_Id) != 'E' ||
        inb(kCrosLPC_MemMap_Base + kCrosCMD_Id + 1) != 'C') {
        IOLog("CROS - Invalid ID\n");
        return nullptr;
    }
    
    return this;
}

bool CrosECBus::start(IOService *provider) {
    if (!super::start(provider)) {
        return false;
    }
    
    ecAcpiDev = OSDynamicCast(IOACPIPlatformDevice, provider);
    if (ecAcpiDev == nullptr) {
        return false;
    }
    
    deviceNubs = OSArray::withCapacity(1);
    if (deviceNubs == nullptr) {
        return false;
    }
    
    workLoop = IOWorkLoop::workLoop();
    if (workLoop == nullptr) {
        return false;
    }
    
    commandGate = IOCommandGate::commandGate(this, OSMemberFunctionCast(IOCommandGate::Action, this, &CrosECBus::transferCommandGated));
    if (commandGate == nullptr) {
        return false;
    }
    
    if (workLoop->addEventSource(commandGate) != kIOReturnSuccess) {
        return false;
    }
    
    publishNubs();
    registerService();
    
    return true;
}

void CrosECBus::free() {
    if (commandGate != nullptr && workLoop != nullptr) {
        workLoop->removeEventSource(commandGate);
    }
    
    OSSafeReleaseNULL(commandGate);
    OSSafeReleaseNULL(workLoop);
    OSSafeReleaseNULL(deviceNubs);
    super::free();
}

IOWorkLoop *CrosECBus::getWorkLoop() const {
    return workLoop;
}

uint8_t CrosECBus::readBytesWithSum(uint32_t offset, uint32_t length, uint8_t *dest) {
    uint8_t sum = 0;
    
    for (uint32_t i = offset; i < offset + length; i++) {
        dest[i] = inb(i);
        sum += dest[i];
    }
    
    return sum;
}

uint8_t CrosECBus::writeBytesWithSum(uint32_t offset, uint32_t length, uint8_t *dest) {
    uint8_t sum = 0;
    
    for (uint32_t i = offset; i < offset + length; i++) {
        outb(i, dest[i]);
        sum += dest[i];
    }
    
    return sum;
}

/**
 * PrepareHostCommand - Creates a buffer with both the command header and data, which can be
 *  written directly to the EC.
 */
IOBufferMemoryDescriptor *CrosECBus::prepareHostCommand(CrosECCommand &cmd) {
    union {
        CrosECHostRequest request;
        uint8_t requestData[];
    };
    
    uint8_t checksum = 0;
    IOBufferMemoryDescriptor *buffer;
    
    request.version = EC_HOST_REQUEST_VERSION;
    request.command = cmd.command;
    request.dataLength = cmd.sendSize;
    request.commandVersion = cmd.version;
    request.reserved = 0;
    request.checksum = 0;
    
    // Calculate checksum of packet
    
    for (int i = 0; i < sizeof(CrosECHostRequest); i++) {
        checksum += requestData[i];
    }
    
    for (int i = 0; i < cmd.sendSize; i++) {
        checksum += cmd.data[i];
    }
    
    // Checksum of packet should add up to zero.
    request.checksum = -checksum;
    
    buffer = IOBufferMemoryDescriptor::withCapacity(sizeof(request) + cmd.sendSize, kIODirectionNone);
    
    if (buffer == nullptr) {
        return nullptr;
    }
    
    buffer->writeBytes(0, &request, sizeof(request));
    buffer->writeBytes(sizeof(request), cmd.data, cmd.sendSize);
    return buffer;
}

IOReturn CrosECBus::transferCommandGated(CrosECCommand *cmd) {
    CrosECHostResponse response;
    uint8_t byte = 0xDA;
    uint8_t checksum = 0;
    
    IOBufferMemoryDescriptor *request = prepareHostCommand(*cmd);
    if (request == nullptr) {
        return kIOReturnNoMemory;
    }
    
    // Write Command
    (void) writeBytesWithSum(kCrosLPC_Host_Pkt_Addr, (uint32_t) request->getLength(), (uint8_t *)request->getBytesNoCopy());
    OSSafeReleaseNULL(request);
    
    // Start Command
    writeBytesWithSum(kCrosLPC_Host_Cmd_Addr, sizeof(byte), &byte);
    
    // TODO: Wait for response
    
    // Check EC Response
    readBytesWithSum(kCrosLPC_Host_Data_Addr, sizeof(byte), &byte);
    
    if (byte == kCrosRes_In_Progress) {
        IOLog("CROS - Command in progress\n");
        return kIOReturnBusy;
    }
    
    // Read Response Header
    checksum = readBytesWithSum(kCrosLPC_Host_Pkt_Addr, sizeof(response), (uint8_t *) &response);
    cmd->ecResponse = response.result;
    
    if (response.dataLength != cmd->recvSize) {
        IOLog("CROS - Response length does not match buffer size\n");
        return kIOReturnDeviceError;
    }
    
    // Read Data
    checksum += readBytesWithSum(kCrosLPC_Host_Pkt_Addr + sizeof(response), response.dataLength, cmd->data);
    
    if (checksum != 0) {
        IOLog("CROS - Invalid Checksum\n");
        return kIOReturnIOError;
    }
    
    return kIOReturnSuccess;
}

IOReturn CrosECBus::transferCommand(CrosECCommand *cmd) {
    return commandGate->attemptCommand(cmd);
}

IOReturn CrosECBus::readMemoryBytesGated(CrosECReadMemory *cmd) {
    if (cmd->offset >= kCrosLPC_MemMap_Size - cmd->size) {
        return kIOReturnOverrun;
    }
    
    (void) readBytesWithSum(kCrosLPC_MemMap_Base + cmd->offset, cmd->size, cmd->data);
    
    return kIOReturnSuccess;
}

IOReturn CrosECBus::readMemoryBytes(CrosECReadMemory *cmd) {
    return commandGate->attemptAction(OSMemberFunctionCast(Action, this, &CrosECBus::readMemoryBytesGated));
}


IOReturn CrosECBus::readMemoryStringGated(CrosECReadMemory *cmd) {
    char c;
    uint32_t len = 0;
    
    if (cmd->offset >= kCrosLPC_MemMap_Size - cmd->size) {
        return kIOReturnOverrun;
    }
    
    for (int i = cmd->offset; i < kCrosLPC_MemMap_Size; i++) {
        readBytesWithSum(kCrosLPC_MemMap_Base + i, sizeof(c), (uint8_t *) &c);
        len++;
        if (c == '\0') {
            break;
        }
    }
    
    cmd->size = len;
    return kIOReturnSuccess;
}

IOReturn CrosECBus::readMemoryString(CrosECReadMemory *cmd) {
    return commandGate->attemptAction(OSMemberFunctionCast(Action, this, &CrosECBus::readMemoryStringGated));
}

bool CrosECBus::publishNubs() {
    IOACPIPlatformDevice *child;
    OSIterator* children = ecAcpiDev->getChildIterator(gIOACPIPlane);

    if (children == nullptr) {
        return false;
    }

    while ((child = OSDynamicCast(IOACPIPlatformDevice, children->getNextObject()))) {
        CrosECDeviceNub* deviceNub = OSTypeAlloc(CrosECDeviceNub);
        OSDictionary* nubProperties = child->dictionaryWithProperties();
        
        if (deviceNub == nullptr || nubProperties == nullptr) {
            IOLog("CROS - No device properties for nub\n");
            OSSafeReleaseNULL(deviceNub);
            OSSafeReleaseNULL(nubProperties);
            continue;
        }
        
        nubProperties->setObject("acpi-device", child);
        
        if (!deviceNub->init(nubProperties) ||
            !deviceNub->attach(this)) {
            IOLog("CROS - Unable to create and attach nub\n");
        } else if (!deviceNub->start(this)) {
            deviceNub->detach(this);
            IOLog("CROS - Unable to start nub\n");
        } else {
            deviceNubs->setObject(deviceNub);
        }

        OSSafeReleaseNULL(nubProperties);
        OSSafeReleaseNULL(deviceNub);
    }
    
    children->release();
    return true;
}
