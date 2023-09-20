//
//  CrosECNub.cpp
//  CrosEC
//
//  Created by Gwydien on 9/19/23.
//

#include "CrosECDeviceNub.hpp"

#define super IOService
OSDefineMetaClassAndStructors(CrosECDeviceNub, IOService);

bool CrosECDeviceNub::start(IOService *provider) {
    if (!super::start(provider)) {
        return false;
    }
    
    ecDev = OSDynamicCast(CrosECBus, provider);
    if (ecDev == nullptr) {
        return false;
    }
    
    acpiDev = OSDynamicCast(IOACPIPlatformDevice, getProperty("acpi-device"));
    if (acpiDev == nullptr) {
        return false;
    }
    
    setName(acpiDev->getName());
    registerService();
    return true;
}
