#include "usb_i2c.h"
#include "usb_device.h"
#include "usb_middleware.h"
#include "usb_protocol.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "usb_log.h"
#ifdef _WIN32
#include <windows.h>
#else
#include "platform_compat.h"
#endif

WINAPI int IIC_Init(const char* target_serial, int i2c_index, PIIC_CONFIG config) {
    debug_printf("IIC_Init开始执行");
    if (!target_serial || !config) {
        debug_printf("参数无效: target_serial=%p, config=%p", target_serial, config);
        return USB_ERROR_INVALID_PARAM;
    }
    
    if (i2c_index < 1 || i2c_index > 4) {
        debug_printf("I2C索引无效: %d (有效范围: 1-4)", i2c_index);
        return USB_ERROR_INVALID_PARAM;
    }
    
    int device_id = usb_middleware_find_device_by_serial(target_serial);
    if (device_id < 0) {
        debug_printf("设备未打开: %s", target_serial);
        return USB_ERROR_OTHER;
    }
    
    debug_printf("IIC配置: ClockSpeedHz=%lu, Master=%d, AddrBits=%d, EnablePu=%d", 
                config->ClockSpeedHz, config->Master, config->AddrBits, config->EnablePu);
    
    GENERIC_CMD_HEADER cmd_header;
    cmd_header.protocol_type = PROTOCOL_IIC;
    cmd_header.cmd_id = CMD_INIT;
    cmd_header.device_index = (uint8_t)i2c_index;
    cmd_header.param_count = 1;
    cmd_header.data_len = 0;
    
    unsigned char* send_buffer;
    int total_len = build_protocol_frame(&send_buffer, &cmd_header, (unsigned char*)config, sizeof(IIC_CONFIG), NULL, 0);
    if (total_len < 0) {
        return USB_ERROR_OTHER;
    }
    
    int ret = usb_middleware_write_data(device_id, send_buffer, total_len);
    free(send_buffer);
    
    debug_printf("IIC初始化结果: %d", ret);
    return (ret >= 0) ? USB_SUCCESS : USB_ERROR_OTHER;
}

WINAPI int IIC_WriteBytes(const char* target_serial, int i2c_index, 
                         uint16_t device_addr, const uint8_t* write_buffer, 
                         uint16_t write_len, uint32_t timeout_ms) {
    debug_printf("IIC_WriteBytes开始执行");
    if (!target_serial || !write_buffer) {
        debug_printf("参数无效: target_serial=%p, write_buffer=%p", target_serial, write_buffer);
        return USB_ERROR_INVALID_PARAM;
    }
    
    if (i2c_index < 1 || i2c_index > 4) {
        debug_printf("I2C索引无效: %d (有效范围: 1-4)", i2c_index);
        return USB_ERROR_INVALID_PARAM;
    }
    
    if (write_len == 0 || write_len > 1024) {
        debug_printf("数据长度无效: %d (有效范围: 1-1024)", write_len);
        return USB_ERROR_INVALID_PARAM;
    }
    
    int device_id = usb_middleware_find_device_by_serial(target_serial);
    if (device_id < 0) {
        debug_printf("设备未打开: %s", target_serial);
        return USB_ERROR_OTHER;
    }
    
    debug_printf("IIC写入: DevAddr=0x%04X, DataLen=%d, Timeout=%dms", 
                device_addr, write_len, timeout_ms);
    
    // 构建I2C写入数据结构 - 简化版本，无内存地址
    size_t write_data_size = sizeof(I2C_WRITE_DATA) + write_len;
    I2C_WRITE_DATA* write_data = (I2C_WRITE_DATA*)malloc(write_data_size);
    if (!write_data) {
        debug_printf("内存分配失败");
        return USB_ERROR_OTHER;
    }
    
    write_data->DeviceAddress = device_addr;
    write_data->MemAddress = 0;         // 无内存地址
    write_data->MemAddSize = 0;         // 无内存地址
    write_data->DataLength = write_len;
    write_data->TimeoutMs = timeout_ms; // 超时参数
    memcpy(write_data->Data, write_buffer, write_len);
    
    GENERIC_CMD_HEADER cmd_header;
    cmd_header.protocol_type = PROTOCOL_IIC;
    cmd_header.cmd_id = CMD_WRITE;
    cmd_header.device_index = (uint8_t)i2c_index;
    cmd_header.param_count = 0;
    cmd_header.data_len = (uint16_t)write_data_size;
    
    unsigned char* send_buffer;
    int total_len = build_protocol_frame(&send_buffer, &cmd_header, NULL, 0, (unsigned char*)write_data, write_data_size);
    
    free(write_data);
    
    if (total_len < 0) {
        return USB_ERROR_OTHER;
    }
    
    int ret = usb_middleware_write_data(device_id, send_buffer, total_len);
    free(send_buffer);
    
    debug_printf("IIC写入结果: %d", ret);
    return (ret >= 0) ? USB_SUCCESS : USB_ERROR_OTHER;
}