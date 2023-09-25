//
//  CrosECNub.hpp
//  CrosEC
//
//  Created by Gwydien on 9/19/23.
//

#pragma once

#include <IOKit/IOService.h>
#include <IOKit/acpi/IOACPIPlatformDevice.h>

#include "CrosECBus.hpp"

class CrosECDeviceNub : public IOService {
    OSDeclareDefaultStructors(CrosECDeviceNub);
public:
    bool start(IOService *provider) override;
    
    IOReturn readMemoryBytes(CrosECReadMemory *cmd) {
        if (ecDev != nullptr) return ecDev->readMemoryBytes(cmd);
        return kIOReturnNoDevice;
    }
    
    IOReturn readMemoryStrings(CrosECReadMemory *cmd) {
        if (ecDev != nullptr) return ecDev->readMemoryString(cmd);
        return kIOReturnNoDevice;
    }
    
    IOReturn transferCommand(uint32_t command,
                             uint32_t version,
                             void * sendBuffer, size_t sendSize,
                             void * recvBuffer, size_t recvSize) {
        CrosECCommand cmd = {
            version,
            command,
            sendSize,
            (uint8_t *) sendBuffer,
            recvSize,
            (uint8_t *) recvBuffer,
            0
        };
        
        IOReturn ret = transferCommand(&cmd);
        return ret != kIOReturnSuccess ? ret : cmd.ecResponse;
    }
    
    IOReturn transferCommand(CrosECCommand *cmd) {
        if (ecDev != nullptr) return ecDev->transferCommand(cmd);
        return kIOReturnNoDevice;
    }
    
    IOACPIPlatformDevice *getACPIDevice() {
        return acpiDev;
    }
    
private:
    IOACPIPlatformDevice *acpiDev {nullptr};
    CrosECBus *ecDev {nullptr};
};
