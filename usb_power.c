#include "usb_power.h"
#include "usb_device.h"
#include "usb_application.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PROTOCOL_POWER      0x05    // 电源协议
#define CMD_SET            0x01    // 设置命令
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
    GENERIC_CMD_HEADER cmd_header;
    cmd_header.protocol_type = PROTOCOL_POWER;   // 电源协议
    cmd_header.cmd_id = POWER_CMD_SET_VOLTAGE;  // 设置电压命令
    cmd_header.device_index = channel;          // 设备索引（通道）
    cmd_header.data_len = sizeof(uint16_t);     // 数据长度：2字节电压值
    int total_len = sizeof(GENERIC_CMD_HEADER) + sizeof(uint16_t);
    unsigned char* send_buffer = (unsigned char*)malloc(total_len);
    if (!send_buffer) {
        debug_printf("内存分配失败");
        return POWER_ERROR_OTHER;
    }
    memcpy(send_buffer, &cmd_header, sizeof(GENERIC_CMD_HEADER));
    memcpy(send_buffer + sizeof(GENERIC_CMD_HEADER), &voltage_mv, sizeof(uint16_t));
    int ret = USB_WriteData(target_serial, send_buffer, total_len);
    free(send_buffer);
    if (ret < 0) {
        debug_printf("发送设置电压命令失败: %d", ret);
        return POWER_ERROR_IO;
    }
    debug_printf("成功发送设置电压命令，通道: %d, 电压值: %d mV", channel, voltage_mv);
    return POWER_SUCCESS;
}
