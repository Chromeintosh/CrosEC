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

IOExternalMethod CrosECUserClient::externalMethods[kCrosUser_MaxMethod] = {
    {nullptr, (IOMethod) &CrosECUserClient::userCommand, kIOUCStructIStructO,
        sizeof(CrosECUserCommandRequest), sizeof(CrosECUserCommandResponse)},
    {nullptr, (IOMethod) &CrosECUserClient::userReadMem, kIOUCStructIStructO,
        sizeof(CrosECUserReadMemoryRequest), sizeof(CrosECUserReadMemoryResponse)},
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
    if (index >= kCrosUser_MaxMethod) {
        return nullptr;
    }
    
    *target = this;
    return &externalMethods[index];
}

IOReturn CrosECUserClient::userCommand(CrosECUserCommandRequest *request, CrosECUserCommandResponse *response,
                                       size_t requestSize, size_t *responseSize) {
    CrosECCommand cmd;
    
    if (request == nullptr || requestSize != sizeof(CrosECUserCommandRequest)) {
        return kIOReturnBadArgument;
    }
    
    if (response == nullptr || *responseSize != sizeof(CrosECUserCommandResponse)) {
        return kIOReturnBadArgument;
    }
    
    if (request->sendSize >= kCrosECMaxPacketDataLen ||
        request->receiveSize >= kCrosECMaxPacketDataLen) {
        return kIOReturnOverrun;
    }
    
    // Request
    cmd.command = request->command;
    cmd.version = request->version;
    cmd.sendSize = request->sendSize;
    cmd.recvSize = request->receiveSize;
    cmd.sendBuffer = request->sendBuffer;
    
    // Response
    cmd.ecResponse = 0;
    cmd.recvBuffer = response->receiveBuffer;
    
    IOReturn transferRet = ec->transferCommand(&cmd);
    response->ecResponse = cmd.ecResponse;
    
    return transferRet;
}

IOReturn CrosECUserClient::userReadMem(CrosECUserReadMemoryRequest *request, CrosECUserReadMemoryResponse *response,
                                       size_t requestSize, size_t *responseSize) {
    CrosECReadMemory mem;
    IOReturn ret;
    
    if (request == nullptr || requestSize != sizeof(*request)) {
        return kIOReturnBadArgument;
    }
    
    if (response == nullptr || *responseSize != sizeof(*response)) {
        return kIOReturnBadArgument;
    }
    
    mem.data = response->data;
    mem.offset = request->offset;
    
    if (request->readSize == kCrosECUserReadStringSize) {
        ret = ec->readMemoryString(&mem);
        response->stringSize = mem.size;
    } else {
        if (request->readSize >= kCrosECMaxMemoryReadLen) {
            return kIOReturnBadArgument;
        }
        
        mem.size = (uint32_t) request->readSize;
        ret = ec->readMemoryBytes(&mem);
    }
    
    return ret;
}
