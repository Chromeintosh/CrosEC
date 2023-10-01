//
//  CrosECKeyboardBacklight.cpp
//  CrosEC
//
//  Created by Gwydien on 9/19/23.
//

#include "CrosECKeyboardBacklight.hpp"

#define super IOService
OSDefineMetaClassAndStructors(CrosECKeyboardBacklight, IOService);

bool CrosECKeyboardBacklight::start(IOService *provider) {
    lkb lkb;
    lks lks;
    
    if (!super::start(provider)) {
        return false;
    }
    
    nub = OSDynamicCast(CrosECDeviceNub, provider);
    if (nub == nullptr) {
        return false;
    }
    
    VirtualSMCAPI::addKey(KeyLKSB, vsmcPlugin.data, VirtualSMCAPI::valueWithData(reinterpret_cast<const SMC_DATA *>(&lkb), sizeof(lkb),
        SmcKeyTypeLkb, new SMCKeyboardBacklightValue(nub), SMC_KEY_ATTRIBUTE_READ | SMC_KEY_ATTRIBUTE_WRITE));
    VirtualSMCAPI::addKey(KeyLKSS, vsmcPlugin.data, VirtualSMCAPI::valueWithData(reinterpret_cast<const SMC_DATA *>(&lks), sizeof(lks),
        SmcKeyTypeLks, nullptr, SMC_KEY_ATTRIBUTE_READ | SMC_KEY_ATTRIBUTE_WRITE));
    
    
    publishResource("KeyboardBacklight", kOSBooleanTrue);
    vsmcNotifier = VirtualSMCAPI::registerHandler(vsmcNotificationHandler, this);
    
    registerService();
    return vsmcNotifier != nullptr;
}

bool CrosECKeyboardBacklight::vsmcNotificationHandler(void *sensors, void *refCon, IOService *vsmc, IONotifier *notifier) {
    if (sensors == nullptr || vsmc == nullptr) {
        return false;
    }
    
    auto self = reinterpret_cast<CrosECKeyboardBacklight *>(sensors);
    IOReturn ret = vsmc->callPlatformFunction(VirtualSMCAPI::SubmitPlugin, true, sensors, &self->vsmcPlugin, nullptr, nullptr);
    
    return ret == kIOReturnSuccess;
}

SMC_RESULT SMCKeyboardBacklightValue::update(const SMC_DATA *src) {
    KeyboardBacklightRequest request;
    
    VirtualSMCValue::update(src);
    const uint16_t smcVal = *reinterpret_cast<const uint16_t *>(src);
    const uint16_t newLevel = ((smcVal & 0xFF00) >> 12) | ((smcVal & 0x00FF) << 4);
    request.percent = newLevel * 100 / 4092;

    nub->transferCommand(kCrosCMD_Keyboard_Backlight, 0, &request, sizeof(request), nullptr, 0);
    return SmcSuccess;
}
