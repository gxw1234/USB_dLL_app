#include "usb_bootloader.h"
#include "usb_device.h"
#include "usb_middleware.h"
#include "usb_protocol.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>



extern void debug_printf(const char *format, ...);


int Bootloader_StartWrite(const char* target_serial, int SPIIndex, unsigned char* pWriteBuffer, int WriteLen) {
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
    cmd_header.protocol_type = PROTOCOL_BOOTLOADER_WRITE_BYTES;    // Bootloader协议
    cmd_header.cmd_id = BOOTLOADER_START_WRITE;             // 开始写数据命令
    cmd_header.device_index = (uint8_t)SPIIndex; // 设备索引
    cmd_header.param_count = 0;                // 参数数量，开始写操作不需要额外参数
    cmd_header.data_len = WriteLen;            // 数据部分长度

    unsigned char* send_buffer;
    int total_len = build_protocol_frame(&send_buffer, &cmd_header, 
                                   NULL, 0, 
                                   pWriteBuffer, WriteLen);
    if (total_len < 0) {
        return SPI_ERROR_OTHER;
    }
    int ret = usb_middleware_write_data(device_id, send_buffer, total_len);
    free(send_buffer);
    return SPI_SUCCESS;
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
    int total_len = build_protocol_frame(&send_buffer, &cmd_header, 
                                   NULL, 0, 
                                   pWriteBuffer, WriteLen);
    if (total_len < 0) {
        return SPI_ERROR_OTHER;
    }
    int ret = usb_middleware_write_data(device_id, send_buffer, total_len);
    free(send_buffer);




    unsigned char response_buffer[1];
    int max_loops = 100000;
    for (int i = 0; i < max_loops; i++) {
        int actual_read = usb_middleware_read_spi_data(device_id, response_buffer, 1);
        if (actual_read > 0) {
            return response_buffer[0]; // 有数据立即返回，无数据则返回-1  读取10000次
        }
       Sleep(1);
    }

    return SPI_ERROR_IO; // 读取失败

}




int Bootloader_SwitchRun(const char* target_serial, int SPIIndex, unsigned char* pWriteBuffer, int WriteLen) {
    if (!target_serial) {
        debug_printf("参数无效: target_serial=%p", target_serial);
        return SPI_ERROR_INVALID_PARAM;
    }

    int device_id = usb_middleware_find_device_by_serial(target_serial);
    if (device_id < 0) {
        debug_printf("设备未打开: %s", target_serial);
        return SPI_ERROR_OTHER;
    }

    GENERIC_CMD_HEADER cmd_header;
    cmd_header.protocol_type = PROTOCOL_BOOTLOADER_WRITE_BYTES;    // Bootloader协议
    cmd_header.cmd_id = BOOTLOADER_SWITCH_RUN;             // 切换到应用程序运行模式
    cmd_header.device_index = (uint8_t)SPIIndex; // 设备索引
    cmd_header.param_count = 0;                // 参数数量
    cmd_header.data_len = 0;                   // 无需数据，只发送命令
    
    // 发送启动模式标识 (0xA5A5A5A5 = BOOT_MODE_APPLICATION)
    uint32_t boot_mode = 0xA5A5A5A5;
    
    unsigned char* send_buffer;
    int total_len = build_protocol_frame(&send_buffer, &cmd_header, 
                                   NULL, 0, 
                                   (unsigned char*)&boot_mode, sizeof(boot_mode));
    if (total_len < 0) {
        return SPI_ERROR_OTHER;
    }
    
    debug_printf("发送切换到应用程序运行模式命令");
    int ret = usb_middleware_write_data(device_id, send_buffer, total_len);
    free(send_buffer);
    return (ret >= 0) ? SPI_SUCCESS : SPI_ERROR_IO;
}

int Bootloader_SwitchBoot(const char* target_serial, int SPIIndex, unsigned char* pWriteBuffer, int WriteLen) {
    if (!target_serial) {
        debug_printf("参数无效: target_serial=%p", target_serial);
        return SPI_ERROR_INVALID_PARAM;
    }

    int device_id = usb_middleware_find_device_by_serial(target_serial);
    if (device_id < 0) {
        debug_printf("设备未打开: %s", target_serial);
        return SPI_ERROR_OTHER;
    }

    GENERIC_CMD_HEADER cmd_header;
    cmd_header.protocol_type = PROTOCOL_BOOTLOADER_WRITE_BYTES;    // Bootloader协议
    cmd_header.cmd_id = BOOTLOADER_SWITCH_BOOT;            // 切换到Bootloader模式
    cmd_header.device_index = (uint8_t)SPIIndex; // 设备索引
    cmd_header.param_count = 0;                // 参数数量
    cmd_header.data_len = 0;                   // 无需数据，只发送命令
    
    // 发送启动模式标识 (0x5A5A5A5A = BOOT_MODE_BOOTLOADER)
    uint32_t boot_mode = 0x5A5A5A5A;
    
    unsigned char* send_buffer;
    int total_len = build_protocol_frame(&send_buffer, &cmd_header, 
                                   NULL, 0, 
                                   (unsigned char*)&boot_mode, sizeof(boot_mode));
    if (total_len < 0) {
        return SPI_ERROR_OTHER;
    }
    
    debug_printf("发送切换到Bootloader模式命令");
    int ret = usb_middleware_write_data(device_id, send_buffer, total_len);
    free(send_buffer);
    return (ret >= 0) ? SPI_SUCCESS : SPI_ERROR_IO;
}

int Bootloader_Reset(const char* target_serial, int SPIIndex, unsigned char* pWriteBuffer, int WriteLen) {
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
    cmd_header.protocol_type = PROTOCOL_BOOTLOADER_WRITE_BYTES;    // Bootloader协议
    cmd_header.cmd_id = BOOTLOADER_RESET;             // 复位命令
    cmd_header.device_index = (uint8_t)SPIIndex; // 设备索引
    cmd_header.param_count = 0;                // 参数数量，复位操作不需要额外参数
    cmd_header.data_len = WriteLen;            // 数据部分长度

    unsigned char* send_buffer;
    int total_len = build_protocol_frame(&send_buffer, &cmd_header, 
                                   NULL, 0, 
                                   pWriteBuffer, WriteLen);
    if (total_len < 0) {
        return SPI_ERROR_OTHER;
    }
    int ret = usb_middleware_write_data(device_id, send_buffer, total_len);
    free(send_buffer);
    return SPI_SUCCESS;
}



