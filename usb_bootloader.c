#include "usb_bootloader.h"
#include "usb_device.h"
#include "usb_middleware.h"
#include "usb_protocol.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 通用命令包头结构 (已移除end_marker字段)
typedef struct _GENERIC_CMD_HEADER {
  uint8_t protocol_type;  // 协议类型：SPI/IIC/UART等
  uint8_t cmd_id;         // 命令ID：初始化/读/写等
  uint8_t device_index;   // 设备索引
  uint8_t param_count;    // 参数数量
  uint16_t data_len;      // 数据部分长度
  uint16_t total_packets; // 整包总数
} GENERIC_CMD_HEADER, *PGENERIC_CMD_HEADER;


typedef struct _PARAM_HEADER {
  uint16_t param_len;     // 参数长度
} PARAM_HEADER, *PPARAM_HEADER;


extern void debug_printf(const char *format, ...);
// 定义帧头标识符
#define FRAME_START_MARKER  0x5A5A5A5A
#define CMD_END_MARKER      0xA5A5A5A5

static int Add_Parameter(unsigned char* buffer, int pos, void* data, uint16_t len) {
    PARAM_HEADER header;
    header.param_len = len;
    memcpy(buffer + pos, &header, sizeof(PARAM_HEADER));
    pos += sizeof(PARAM_HEADER);
    memcpy(buffer + pos, data, len);
    pos += len;
    return pos;
}

static int build_spi_frame(unsigned char** buffer, GENERIC_CMD_HEADER* cmd_header, 
                          unsigned char* param_data, int param_len, 
                          unsigned char* data_payload, int data_len) {
    cmd_header->total_packets = sizeof(GENERIC_CMD_HEADER);
    if (param_len > 0) {
        cmd_header->total_packets += sizeof(PARAM_HEADER) + param_len;
    }
    if (data_len > 0) {
        cmd_header->total_packets += data_len;
    }
    int total_len = sizeof(uint32_t) + cmd_header->total_packets + sizeof(uint32_t);
    *buffer = (unsigned char*)malloc(total_len);
    if (!*buffer) {
        debug_printf("内存分配失败");
        return -1;
    }
    int pos = 0;
    // 添加帧头标识符
    uint32_t frame_header = FRAME_START_MARKER;
    memcpy(*buffer + pos, &frame_header, sizeof(uint32_t));
    pos += sizeof(uint32_t);
    
    // 添加协议头
    memcpy(*buffer + pos, cmd_header, sizeof(GENERIC_CMD_HEADER));
    pos += sizeof(GENERIC_CMD_HEADER);
    
    // 添加参数（如果有）
    if (param_len > 0 && param_data) {
        pos = Add_Parameter(*buffer, pos, param_data, param_len);
    }

    // 添加数据负载（如果有）
    if (data_len > 0 && data_payload) {
        memcpy(*buffer + pos, data_payload, data_len);
        pos += data_len;
    }
    
    
    // 添加帧尾标识符
    uint32_t end_marker = CMD_END_MARKER;
    memcpy(*buffer + pos, &end_marker, sizeof(uint32_t));
    
    debug_printf("[PC] SPI sending frame with header 0x%08X, protocol_type=%d, cmd_id=%d", 
                 FRAME_START_MARKER, cmd_header->protocol_type, cmd_header->cmd_id);
    
    return total_len;
}


int Bootloader_WriteBytes(const char* target_serial, int SPIIndex, unsigned char* pWriteBuffer, int WriteLen) {
    if (!target_serial || !pWriteBuffer || WriteLen <= 0) {
        debug_printf("参数无效: target_serial=%p, pWriteBuffer=%p, WriteLen=%d", target_serial, pWriteBuffer, WriteLen);
        return SPI_ERROR_INVALID_PARAM;
    }

    int device_id = usb_middleware_find_device_by_serial(target_serial);
    if (device_id < 0) {
        debug_printf("设备未打开: %s", target_serial);
        return SPI_ERROR_OTHER;
    }

    GENERIC_CMD_HEADER cmd_header;
    cmd_header.protocol_type = PROTOCOL_BOOTLOADER_WRITE_BYTES;    // SPI协议
    cmd_header.cmd_id = BOOTLOADER_WRITE_BYTES;             // 写数据命令
    cmd_header.device_index = (uint8_t)SPIIndex; // 设备索引
    cmd_header.param_count = 0;                // 参数数量，写操作不需要额外参数
    cmd_header.data_len = WriteLen;            // 数据部分长度

    unsigned char* send_buffer;
    int total_len = build_spi_frame(&send_buffer, &cmd_header, 
                                   NULL, 0, 
                                   pWriteBuffer, WriteLen);
    if (total_len < 0) {
        return SPI_ERROR_OTHER;
    }
    int ret = usb_middleware_write_data(device_id, send_buffer, total_len);
    free(send_buffer);
    return SPI_SUCCESS;
}




int Bootloader_SwitchRun(const char* target_serial, int SPIIndex, unsigned char* pWriteBuffer, int WriteLen) {
    if (!target_serial || !pWriteBuffer || WriteLen <= 0) {
        debug_printf("参数无效: target_serial=%p, pWriteBuffer=%p, WriteLen=%d", target_serial, pWriteBuffer, WriteLen);
        return SPI_ERROR_INVALID_PARAM;
    }

    int device_id = usb_middleware_find_device_by_serial(target_serial);
    if (device_id < 0) {
        debug_printf("设备未打开: %s", target_serial);
        return SPI_ERROR_OTHER;
    }

    GENERIC_CMD_HEADER cmd_header;
    cmd_header.protocol_type = PROTOCOL_BOOTLOADER_WRITE_BYTES;    // SPI协议
    cmd_header.cmd_id = BOOTLOADER_SWITCH_RUN;             // 写数据命令
    cmd_header.device_index = (uint8_t)SPIIndex; // 设备索引
    cmd_header.param_count = 0;                // 参数数量，写操作不需要额外参数
    cmd_header.data_len = WriteLen;            // 数据部分长度

    unsigned char* send_buffer;
    int total_len = build_spi_frame(&send_buffer, &cmd_header, 
                                   NULL, 0, 
                                   pWriteBuffer, WriteLen);
    if (total_len < 0) {
        return SPI_ERROR_OTHER;
    }
    int ret = usb_middleware_write_data(device_id, send_buffer, total_len);
    free(send_buffer);
    return SPI_SUCCESS;
}

int Bootloader_SwitchBoot(const char* target_serial, int SPIIndex, unsigned char* pWriteBuffer, int WriteLen) {
    if (!target_serial || !pWriteBuffer || WriteLen <= 0) {
        debug_printf("参数无效: target_serial=%p, pWriteBuffer=%p, WriteLen=%d", target_serial, pWriteBuffer, WriteLen);
        return SPI_ERROR_INVALID_PARAM;
    }

    int device_id = usb_middleware_find_device_by_serial(target_serial);
    if (device_id < 0) {
        debug_printf("设备未打开: %s", target_serial);
        return SPI_ERROR_OTHER;
    }

    GENERIC_CMD_HEADER cmd_header;
    cmd_header.protocol_type = PROTOCOL_BOOTLOADER_WRITE_BYTES;    // SPI协议
    cmd_header.cmd_id = BOOTLOADER_SWITCH_BOOT;             // 写数据命令
    cmd_header.device_index = (uint8_t)SPIIndex; // 设备索引
    cmd_header.param_count = 0;                // 参数数量，写操作不需要额外参数
    cmd_header.data_len = WriteLen;            // 数据部分长度

    unsigned char* send_buffer;
    int total_len = build_spi_frame(&send_buffer, &cmd_header, 
                                   NULL, 0, 
                                   pWriteBuffer, WriteLen);
    if (total_len < 0) {
        return SPI_ERROR_OTHER;
    }
    int ret = usb_middleware_write_data(device_id, send_buffer, total_len);
    free(send_buffer);
    return SPI_SUCCESS;
}



