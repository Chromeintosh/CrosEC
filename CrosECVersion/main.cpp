//
//  main.cpp
//  CrosECVersion
//
//  Created by Gwydien on 9/26/23.
//

#include <iostream>
#include <IOKit/IOKitLib.h>
#include <mach/mach_error.h>
#include "CrosECShared.h"

int main(int argc, const char * argv[]) {
    io_service_t service;
    io_connect_t connect;
    kern_return_t kernResult;
    
    service = IOServiceGetMatchingService(kIOMasterPortDefault, IOServiceMatching("CrosECBus"));
    if (service == 0) {
        std::cerr << "No Chrome EC!" << std::endl;
        return -1;
    }
    
    kernResult = IOServiceOpen(service, mach_task_self(), 0, &connect);
    if (kernResult != KERN_SUCCESS) {
        std::cerr << "Failed to open EC!" << std::endl;
        return -2;
    }
    
    struct ECVersion {
        char versionString[32];
        char versionStringRW[32];
        char reserved[32];
        uint32_t currentImage;
    };
    
    CrosECUserCommandRequest request {0};
    CrosECUserCommandResponse response;
    request.command = kCrosCMD_Version;
    request.version = 0;
    request.maxReceiveSize = sizeof(ECVersion);
    request.sendSize = 0;
    
    size_t requestSize = sizeof(request);
    size_t responseSize = sizeof(response);
    
    kernResult = IOConnectCallStructMethod(connect, kCrosUser_Command, &request, requestSize, &response, &responseSize);
    
    if (kernResult != KERN_SUCCESS) {
        std::cerr << "Failed to get EC version! " << mach_error_string(kernResult) << std::endl;
    } else {
        ECVersion *buf = (ECVersion *) response.receiveBuffer;
        std::cout << "EC RO Version: " << buf->versionString << std::endl;
        std::cout << "EC RW Version: " << buf->versionString << std::endl;
    }
    
    kernResult = IOServiceClose(connect);
    if (kernResult != KERN_SUCCESS) {
        std::cerr << "Failed to close service" << std::endl;
    }
    
    IOObjectRelease(service);
    
    return 0;
}
