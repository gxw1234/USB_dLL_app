#include "usb_power.h"
#include "usb_device.h"
#include "usb_application.h"
#include "usb_middleware.h"
#include "usb_protocol.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "usb_log.h"





WINAPI int POWER_SetVoltage(const char* target_serial, uint8_t channel, uint16_t voltage_mv) {

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
    cmd_header.cmd_id = POWER_CMD_SET_VOLTAGE;
    cmd_header.device_index = channel;
    cmd_header.param_count = 1;
    cmd_header.data_len = 0;
    
    unsigned char* send_buffer;
    int total_len = build_protocol_frame(&send_buffer, &cmd_header, &voltage_mv, sizeof(uint16_t), NULL, 0);
    if (total_len < 0) {
        debug_printf("构建协议帧失败");
        return POWER_ERROR_OTHER;
    }
    
    int ret = usb_middleware_write_data(device_id, send_buffer, total_len);
    free(send_buffer);
    if (ret < 0) {
        debug_printf("发送设置电压命令失败: %d", ret);
        return POWER_ERROR_IO;
    }
    debug_printf("成功发送设置电压命令，通道: %d, 电压值: %d mV", channel, voltage_mv);
    return POWER_SUCCESS;
}

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
    cmd_header.cmd_id = POWER_CMD_START_READING;
    cmd_header.device_index = channel;
    cmd_header.param_count = 0;
    cmd_header.data_len = 0;
    
    unsigned char* send_buffer;
    int total_len = build_protocol_frame(&send_buffer, &cmd_header, NULL, 0, NULL, 0);
    if (total_len < 0) {
        debug_printf("构建协议帧失败");
        return POWER_ERROR_OTHER;
    }
    
    int ret = usb_middleware_write_data(device_id, send_buffer, total_len);
    free(send_buffer);
    if (ret < 0) {
        debug_printf("发送开始读取电流命令失败: %d", ret);
        return POWER_ERROR_IO;
    }
    debug_printf("成功发送开始读取电流命令，通道: %d", channel);
    return POWER_SUCCESS;
}


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
    cmd_header.cmd_id = POWER_CMD_STOP_READING;
    cmd_header.device_index = channel;
    cmd_header.param_count = 0;
    cmd_header.data_len = 0;
    
    unsigned char* send_buffer;
    int total_len = build_protocol_frame(&send_buffer, &cmd_header, NULL, 0, NULL, 0);
    if (total_len < 0) {
        debug_printf("构建协议帧失败");
        return POWER_ERROR_OTHER;
    }
    
    int ret = usb_middleware_write_data(device_id, send_buffer, total_len);
    free(send_buffer);
    if (ret < 0) {
        debug_printf("发送停止读取电流命令失败: %d", ret);
        return POWER_ERROR_IO;
    }
    debug_printf("成功发送停止读取电流命令，通道: %d", channel);
    return POWER_SUCCESS;
}


WINAPI int POWER_ReadCurrentData(const char* target_serial, uint8_t channel, float* current_value) {
    if (!target_serial || !current_value) {
        debug_printf("参数无效: target_serial=%p, current_value=%p", target_serial, current_value);
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
    cmd_header.cmd_id = POWER_CMD_READ_CURRENT_DATA;
    cmd_header.device_index = channel;
    cmd_header.param_count = 0;
    cmd_header.data_len = 0;
    
    unsigned char* send_buffer;
    int total_len = build_protocol_frame(&send_buffer, &cmd_header, NULL, 0, NULL, 0);
    if (total_len < 0) {
        debug_printf("构建协议帧失败");
        return POWER_ERROR_OTHER;
    }
    
    int ret = usb_middleware_write_data(device_id, send_buffer, total_len);
    free(send_buffer);
    if (ret < 0) {
        debug_printf("发送读取电流数据命令失败: %d", ret);
        return POWER_ERROR_IO;
    }
    
    // TODO: 实现从设备读取电流数据的逻辑
    // 这里需要调用usb_middleware_read_data来接收响应数据
    // 并解析出float类型的电流值
    *current_value = 0.0f;  // 临时返回值
    
    debug_printf("成功发送读取电流数据命令，通道: %d", channel);
    return POWER_SUCCESS;
}
