
#include <IOKit/IOService.h>
#include <IOKit/acpi/IOACPIPlatformDevice.h>
#include <IOKit/IOCommandGate.h>
#include <IOKit/IOWorkLoop.h>

#include "CrosECShared.h"

#pragma once

struct CrosECCommand {
    uint32_t version;
    uint32_t command;
    size_t sendSize;
    uint8_t *sendBuffer;
    size_t recvSize;
    uint8_t *recvBuffer;
    uint32_t ecResponse;
};

struct CrosECReadMemory {
    uint32_t offset;
    uint32_t size;
    uint8_t *data;
};

/* ACPI Commands */
constexpr uint32_t kCrosLPC_ACPI_Data_Addr  = 0x62;
constexpr uint32_t kCrosLPC_ACPI_Cmd_Addr   = 0x66;

/* Host Commands */
constexpr uint32_t kCrosLPC_Host_Data_Addr  = 0x200;
constexpr uint32_t kCrosLPC_Host_Cmd_Addr   = 0x204;

/* Version 3 Host Command Arguments */
constexpr uint32_t kCrosLPC_Host_Pkt_Addr   = 0x800;
constexpr uint32_t kCrosLPC_Host_Pkt_Size   = 0x100;

/* Memory Mapped Values */
constexpr uint32_t kCrosLPC_MemMap_Base     = 0x900;
constexpr uint32_t kCrosLPC_MemMap_Size     = 0x100;

// LPC Command Status byte masks
enum {
    // Host has not read data register yet
    kCrosLPC_Status_ToHost      = 1 << 0,
    // EC has not read command/date byte
    kCrosLPC_Status_FromHost    = 1 << 1,
    kCrosLPC_Status_Processing  = 1 << 2,
    kCrosLPC_Status_LastWasCmd  = 1 << 3,
    kCrosLPC_Status_BurstMode   = 1 << 4,
    kCrosLPC_Status_SCIPending  = 1 << 5,
    kCrosLPC_Status_SMIPending  = 1 << 6,
    kCrosLPC_Status_Reserved    = 1 << 7
};

#define packed __attribute__ ((aligned (1)))

#define EC_HOST_REQUEST_VERSION 3

struct CrosECHostRequest {
    uint8_t version;
    uint8_t checksum;
    uint16_t command;
    uint8_t commandVersion;
    uint8_t reserved;
    uint16_t dataLength;
} packed;

struct CrosECHostResponse {
    uint8_t version; // == 3
    uint8_t checksum; // Response + data should total 0
    uint16_t result;
    uint16_t dataLength;
    uint16_t reserved; // == 0
} packed;

class CrosECBus : public IOService
{
    OSDeclareDefaultStructors(CrosECBus);
    
private:
    uint8_t readBytesWithSum(uint32_t offset, uint32_t length, uint8_t *dest);
    uint8_t writeBytesWithSum(uint32_t offset, uint32_t length, uint8_t *dest);
    
    IOBufferMemoryDescriptor *prepareHostCommand(CrosECCommand &cmd);
    bool publishNubs();
    
    IOACPIPlatformDevice *ecAcpiDev {nullptr};
    OSArray *deviceNubs {nullptr};
    
    IOWorkLoop *workLoop {nullptr};
    IOCommandGate *commandGate {nullptr};
    
    IOReturn transferCommandGated(CrosECCommand *cmd);
    IOReturn readMemoryBytesGated(CrosECReadMemory *cmd);
    IOReturn readMemoryStringGated(CrosECReadMemory *cmd);
public:
    /* Public interfaces */
    
    virtual IOService *probe(IOService *provider, int32_t *score) override;
    virtual bool start(IOService *provider) override;
    virtual void free() override;
    
    IOWorkLoop *getWorkLoop() const override;
    
    bool open(IOService *client);
    IOReturn transferCommand(CrosECCommand *cmd);
    IOReturn readMemoryBytes(CrosECReadMemory *cmd);
    IOReturn readMemoryString(CrosECReadMemory *cmd);
};
