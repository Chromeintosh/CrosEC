//
//  CrosECKeyboardBacklight.hpp
//  CrosEC
//
//  Created by Gwydien on 9/19/23.
//

#pragma once

#include <IOKit/IOService.h>
#include <libkern/libkern.h>
#include <VirtualSMCSDK/kern_vsmcapi.hpp>

#include "CrosECDeviceNub.hpp"

struct KeyboardBacklightRequest {
    UINT8 percent;
};

class SMCKeyboardBacklightValue : public VirtualSMCValue {
    SMC_RESULT update(const SMC_DATA *src) override;
public:
    SMCKeyboardBacklightValue(CrosECDeviceNub *nub) : nub(nub) {};
private:
    CrosECDeviceNub *nub;
};

class CrosECKeyboardBacklight : public IOService {
    OSDeclareDefaultStructors(CrosECKeyboardBacklight);
public:
    bool start(IOService *provider) override;
private:
    
    CrosECDeviceNub *nub {nullptr};
    
    /**
     *  VirtualSMC service registration notifier
     */
    IONotifier *vsmcNotifier {nullptr};
    
    /**
     *  Registered plugin instance
     */
    VirtualSMCAPI::Plugin vsmcPlugin {
        xStringify(PRODUCT_NAME),
        parseModuleVersion(xStringify(MODULE_VERSION)),
        VirtualSMCAPI::Version,
    };
    
    /**
     * SMC Key names
     */
    static constexpr SMC_KEY KeyLKSB = SMC_MAKE_IDENTIFIER('L','K','S','B');
    static constexpr SMC_KEY KeyLKSS = SMC_MAKE_IDENTIFIER('L','K','S','S');
    
    /**
     *  Keyboard backlight brightness
     */
    struct lkb {
        uint8_t unknown0 {0};
        uint8_t unknown1 {1};
    };

    struct lks {
        uint8_t unknown0 {0};
        uint8_t unknown1 {1};
    };
    
    /**
     *  Handler from VirtualSMC signaling it's ready for plugins
     *
     *  @param sensors   SMCLightSensor service
     *  @param refCon    reference
     *  @param vsmc      VirtualSMC service
     *  @param notifier  created notifier
     */
    static bool vsmcNotificationHandler(void *sensors, void *refCon, IOService *vsmc, IONotifier *notifier);
};
