//
//  Common.h
//  CrosEC
//
//  Created by Gwydien on 9/19/23.
//

#pragma once

// Memmap (Read mem)
enum {
    /* Thermal 0x00-0x1F */
    kCrosMEM_Temp_Sensor        = 0x00,
    kCrosMEM_Fan                = 0x10,
    kCrosMEM_Temp_Sensor_B      = 0x18,
    
    /* EC Info 0x20-0x27 */
    kCrosMEM_Id                 = 0x20,
    kCrosMEM_Id_Version         = 0x22,
    kCrosMEM_Thermal_Version    = 0x23,
    kCrosMEM_Battery_Version    = 0x24,
    kCrosMEM_Switches_Version   = 0x25,
    kCrosMEM_Events_Version     = 0x26,
    kCrosMEM_Host_Command_Flags = 0x27,
    
    kCrosMEM_Switches           = 0x30,
    kCrosMEM_Host_Events        = 0x34,
    
    /* Battery Data 0x40-0x7F */
    
    /* Sensor Hub Data */
    
    kCrosMEM_ALS_Data           = 0x80,
    kCrosMEM_Accel_Status       = 0x90,
    kCrosMEM_Accel_Data         = 0x92,
    kCrosMEM_Gyro_Data          = 0xA0
};

// Commands
enum {
    kCrosCMD_Motion_Sense       = 0x2B
};

/*
 * Host command response codes (16-bit).  Note that response codes should be
 * stored in a uint16_t rather than directly in a value of this type.
 */
enum {
    kCrosRes_Success                = 0,
    kCrosRes_Invalid_Command        = 1,
    kCrosRes_Error                  = 2,
    kCrosRes_Invalid_Parameter      = 3,
    kCrosRes_Access_Denied          = 4,
    kCrosRes_Invalid_Response       = 5,
    kCrosRes_Invalid_Version        = 6,
    kCrosRes_Invalid_Checksum       = 7,
    kCrosRes_In_Progress            = 8, /* Command in Progress still */
    kCrosRes_Unavailable            = 9,
    kCrosRes_Timeout                = 10,
    kCrosRes_Overflow               = 11,
    kCrosRes_Invalid_Header         = 12,
    kCrosRes_Request_Truncated      = 13,
    kCrosRes_Response_Too_Big       = 14,
    kCrosRes_Bus_Error              = 15,
    kCrosRes_Busy                   = 16,
    kCrosRes_Invalid_Header_Version = 17,
    kCrosRes_Invalid_Header_Crc     = 18,
    kCrosRes_Invalid_Data_Crc       = 19,
    kCrosRes_Dup_Unavailable        = 20,
};

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
