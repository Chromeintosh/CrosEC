//
//  CrosECUserClient.hpp
//  CrosEC
//
//  Created by Gwydien on 9/24/23.
//

#pragma once

#include <IOKit/IOUserClient.h>

class CrosECBus;

class CrosECUserClient : public IOUserClient {
    OSDeclareDefaultStructors(CrosECUserClient);
public:
    bool start(IOService *provider) override;
    IOReturn clientClose() override;
    
    IOExternalMethod *getTargetAndMethodForIndex(IOService **target, uint32_t index) override;
private:
    CrosECBus *ec{nullptr};
};
