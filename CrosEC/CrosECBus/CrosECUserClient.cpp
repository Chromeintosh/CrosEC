//
//  CrosECUserClient.cpp
//  CrosEC
//
//  Created by Gwydien on 9/24/23.
//

#include "CrosECUserClient.hpp"
#include "CrosECBus.hpp"

#define super IOUserClient
OSDefineMetaClassAndStructors(CrosECUserClient, IOUserClient);

constexpr static size_t MaxExternalMethod = 0;
static IOExternalMethod externalMethods[MaxExternalMethod] = {
};

bool CrosECUserClient::start(IOService *provider) {
    if (!super::start(provider)) {
        return false;
    }
    
    ec = OSDynamicCast(CrosECBus, provider);
    return ec != nullptr;
}

IOReturn CrosECUserClient::clientClose() {
    return kIOReturnSuccess;
}

IOExternalMethod *CrosECUserClient::getTargetAndMethodForIndex(IOService **target, uint32_t index) {
    if (index >= MaxExternalMethod) {
        return nullptr;
    }
    
    *target = this;
    return &externalMethods[index];
}
