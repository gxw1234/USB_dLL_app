#include "usb_power.h"
#include "usb_device.h"
#include "usb_application.h"
#include "usb_middleware.h"
#include "usb_protocol.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PROTOCOL_POWER      0x05    // 电源协议
#define CMD_SET            0x01    // 设置命令
#define CMD_START_READING  0x02    // 开始读取命令
#define CMD_STOP_READING   0x03    // 停止读取命令
#define CMD_READ_DATA      0x04    // 读取数据命令
typedef struct _GENERIC_CMD_HEADER {
  uint8_t protocol_type;  // 协议类型
  uint8_t cmd_id;         // 命令ID
  uint8_t device_index;   // 设备索引
  uint16_t data_len;      // 数据部分长度
} GENERIC_CMD_HEADER, *PGENERIC_CMD_HEADER;


extern void debug_printf(const char *format, ...);

/**
 * @brief 设置电压
 * 
 * @param target_serial 目标设备的序列号
 * @param channel 电源通道
 * @param voltage_mv 电压值（单位：mV）
 * @return int 成功返回0，失败返回错误代码
 */
WINAPI int POWER_SetVoltage(const char* target_serial, uint8_t channel, uint16_t voltage_mv) {
    if (!target_serial) {
        debug_printf("参数无效: target_serial=%p", target_serial);
        return POWER_ERROR_INVALID_PARAM;
    }
    if (channel != POWER_CHANNEL_1) {
        debug_printf("电源通道无效: %d", channel);
        return POWER_ERROR_INVALID_PARAM;
    }
    int device_id = usb_middleware_find_device_by_serial(target_serial);
    if (device_id < 0) {
        device_id = usb_middleware_open_device(target_serial);
        if (device_id < 0) {
            debug_printf("打开设备失败: %d", device_id);
            return POWER_ERROR_OTHER;
        }
    }
    // 组包协议头和数据
    GENERIC_CMD_HEADER cmd_header;
    cmd_header.protocol_type = PROTOCOL_POWER;
    cmd_header.cmd_id = CMD_SET;
    cmd_header.device_index = channel;
    cmd_header.data_len = sizeof(uint16_t);
    int total_len = sizeof(GENERIC_CMD_HEADER) + sizeof(uint16_t);
    unsigned char* send_buffer = (unsigned char*)malloc(total_len);
    if (!send_buffer) {
        debug_printf("内存分配失败");
        return POWER_ERROR_OTHER;
    }
    memcpy(send_buffer, &cmd_header, sizeof(GENERIC_CMD_HEADER));
    memcpy(send_buffer + sizeof(GENERIC_CMD_HEADER), &voltage_mv, sizeof(uint16_t));
    int ret = usb_middleware_write_data(device_id, send_buffer, total_len);
    free(send_buffer);
    if (ret < 0) {
        debug_printf("发送设置电压命令失败: %d", ret);
        return POWER_ERROR_IO;
    }
    debug_printf("成功发送设置电压命令，通道: %d, 电压值: %d mV", channel, voltage_mv);
    return POWER_SUCCESS;
}

/**
 * @brief 开始读取电流
 * 
 * @param target_serial 目标设备的序列号
 * @param channel 电流通道 (POWER_CHANNEL_UA 或 POWER_CHANNEL_MA)
 * @return int 成功返回0，失败返回错误代码
 */
WINAPI int POWER_StartCurrentReading(const char* target_serial, uint8_t channel) {
    if (!target_serial) {
        debug_printf("参数无效: target_serial=%p", target_serial);
        return POWER_ERROR_INVALID_PARAM;
    }
    int device_id = usb_middleware_find_device_by_serial(target_serial);
    if (device_id < 0) {
        device_id = usb_middleware_open_device(target_serial);
        if (device_id < 0) {
            debug_printf("打开设备失败: %d", device_id);
            return POWER_ERROR_OTHER;
        }
    }
    GENERIC_CMD_HEADER cmd_header;
    cmd_header.protocol_type = PROTOCOL_POWER;
    cmd_header.cmd_id = CMD_START_READING;
    cmd_header.device_index = channel;
    cmd_header.data_len = 0;
    int total_len = sizeof(GENERIC_CMD_HEADER);
    unsigned char* send_buffer = (unsigned char*)malloc(total_len);
    if (!send_buffer) {
        debug_printf("内存分配失败");
        return POWER_ERROR_OTHER;
    }
    memcpy(send_buffer, &cmd_header, sizeof(GENERIC_CMD_HEADER));
    int ret = usb_middleware_write_data(device_id, send_buffer, total_len);
    free(send_buffer);
    if (ret < 0) {
        debug_printf("发送开始读取电流命令失败: %d", ret);
        return POWER_ERROR_IO;
    }
    debug_printf("成功发送开始读取电流命令，通道: %d", channel);
    return POWER_SUCCESS;
}

/**
 * @brief 停止读取电流
 * 
 * @param target_serial 目标设备的序列号
 * @param channel 电流通道 (POWER_CHANNEL_UA 或 POWER_CHANNEL_MA)
 * @return int 成功返回0，失败返回错误代码
 */
WINAPI int POWER_StopCurrentReading(const char* target_serial, uint8_t channel) {
    if (!target_serial) {
        debug_printf("参数无效: target_serial=%p", target_serial);
        return POWER_ERROR_INVALID_PARAM;
    }
    int device_id = usb_middleware_find_device_by_serial(target_serial);
    if (device_id < 0) {
        device_id = usb_middleware_open_device(target_serial);
        if (device_id < 0) {
            debug_printf("打开设备失败: %d", device_id);
            return POWER_ERROR_OTHER;
        }
    }
    GENERIC_CMD_HEADER cmd_header;
    cmd_header.protocol_type = PROTOCOL_POWER;
    cmd_header.cmd_id = CMD_STOP_READING;
    cmd_header.device_index = channel;
    cmd_header.data_len = 0;
    int total_len = sizeof(GENERIC_CMD_HEADER);
    unsigned char* send_buffer = (unsigned char*)malloc(total_len);
    if (!send_buffer) {
        debug_printf("内存分配失败");
        return POWER_ERROR_OTHER;
    }
    memcpy(send_buffer, &cmd_header, sizeof(GENERIC_CMD_HEADER));
    int ret = usb_middleware_write_data(device_id, send_buffer, total_len);
    free(send_buffer);
    if (ret < 0) {
        debug_printf("发送停止读取电流命令失败: %d", ret);
        return POWER_ERROR_IO;
    }
    debug_printf("成功发送停止读取电流命令，通道: %d", channel);
    return POWER_SUCCESS;
}

/**
 * @brief 读取电流数据
 * 
 * @param target_serial 目标设备的序列号
 * @param channel 电流通道 (POWER_CHANNEL_UA 或 POWER_CHANNEL_MA)
 * @param current_value 用于接收电流值的指针（单位：微安或毫安）
 * @return int 成功返回0，失败返回错误代码
 */
WINAPI int POWER_ReadCurrentData(const char* target_serial, uint8_t channel, float* current_value) {
    if (!target_serial || !current_value) {
        debug_printf("参数无效: target_serial=%p, current_value=%p", target_serial, current_value);
        return POWER_ERROR_INVALID_PARAM;
    }
    

  
    return POWER_SUCCESS;
}
