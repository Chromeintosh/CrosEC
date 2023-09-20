//
//  Common.h
//  CrosEC
//
//  Created by Gwydien on 9/19/23.
//

#pragma once

enum {
    /* Thermal 0x00-0x1F */
    kCrosCMD_Temp_Sensor        = 0x00,
    kCrosCMD_Fan                = 0x10,
    kCrosCMD_Temp_Sensor_B      = 0x18,
    
    /* EC Info 0x20-0x27 */
    kCrosCMD_Id                 = 0x20,
    kCrosCMD_Id_Version         = 0x22,
    kCrosCMD_Thermal_Version    = 0x23,
    kCrosCMD_Battery_Version    = 0x24,
    kCrosCMD_Switches_Version   = 0x25,
    kCrosCMD_Events_Version     = 0x26,
    kCrosCMD_Host_Command_Flags = 0x27,
    
    kCrosCMD_Switches           = 0x30,
    kCrosCMD_Host_Events        = 0x34,
    
    /* Battery Data 0x40-0x7F */
    
    /* Sensor Hub Data */
    
    kCrosCMD_ALS_Data           = 0x80,
    kCrosCMD_Accel_Status       = 0x90,
    kCrosCMD_Accel_Data         = 0x92,
    kCrosCMD_Gyro_Data          = 0xA0
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
    uint32_t sendSize;
    uint32_t recvSize;
    uint32_t ecResponse;
    uint8_t *data;
};

struct CrosECReadMemory {
    uint32_t offset;
    uint32_t size;
    uint8_t *data;
};
