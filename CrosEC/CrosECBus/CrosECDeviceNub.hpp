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

class CrosECDeviceNub : public IOService
{
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
    
    IOReturn transferCommand(CrosECCommand *cmd) {
        if (ecDev != nullptr) return ecDev->transferCommand(cmd);
        return kIOReturnNoDevice;
    }
    
private:
    IOACPIPlatformDevice *acpiDev {nullptr};
    CrosECBus *ecDev {nullptr};
};
