//
//  CrosECKeyboardBacklight.hpp
//  CrosEC
//
//  Created by Gwydien on 9/19/23.
//

#pragma once

#include <IOKit/IOService.h>

#include "CrosECKeyboardBacklight.hpp"
#include "CrosECDeviceNub.hpp"

class CrosECKeyboardBacklight : public IOService {
    OSDeclareDefaultStructors(CrosECKeyboardBacklight);
public:
    
private:
    CrosECDeviceNub *nub {nullptr};
};
