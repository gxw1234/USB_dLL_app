#include "usb_pwm.h"
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

WINAPI int PWM_Init(const char* target_serial, int pwm_index) {
    debug_printf("PWM_Init开始执行");
    if (!target_serial) {
        debug_printf("参数无效: target_serial=%p", target_serial);
        return USB_ERROR_INVALID_PARAM;
    }
    
    if (pwm_index < 1 || pwm_index > 4) {
        debug_printf("PWM通道索引无效: %d (有效范围: 1-4)", pwm_index);
        return USB_ERROR_INVALID_PARAM;
    }
    
    int device_id = usb_middleware_find_device_by_serial(target_serial);
    if (device_id < 0) {
        debug_printf("设备未打开: %s", target_serial);
        return USB_ERROR_OTHER;
    }
    
    GENERIC_CMD_HEADER cmd_header;
    cmd_header.protocol_type = PROTOCOL_PWM;
    cmd_header.cmd_id = PWM_CMD_INIT;
    cmd_header.device_index = (uint8_t)pwm_index;
    cmd_header.param_count = 0;
    cmd_header.data_len = 0;
    
    unsigned char* send_buffer;
    int total_len = build_protocol_frame(&send_buffer, &cmd_header, NULL, 0, NULL, 0);
    if (total_len < 0) {
        return USB_ERROR_OTHER;
    }
    
    int ret = usb_middleware_write_data(device_id, send_buffer, total_len);
    free(send_buffer);
    
    debug_printf("PWM初始化结果: %d", ret);
    return (ret >= 0) ? USB_SUCCESS : USB_ERROR_OTHER;
}

WINAPI int PWM_StartMeasure(const char* target_serial, int pwm_index) {
    debug_printf("PWM_StartMeasure开始执行");
    if (!target_serial) {
        debug_printf("参数无效: target_serial=%p", target_serial);
        return USB_ERROR_INVALID_PARAM;
    }
    
    if (pwm_index < 1 || pwm_index > 4) {
        debug_printf("PWM通道索引无效: %d (有效范围: 1-4)", pwm_index);
        return USB_ERROR_INVALID_PARAM;
    }
    
    int device_id = usb_middleware_find_device_by_serial(target_serial);
    if (device_id < 0) {
        debug_printf("设备未打开: %s", target_serial);
        return USB_ERROR_OTHER;
    }
    
    GENERIC_CMD_HEADER cmd_header;
    cmd_header.protocol_type = PROTOCOL_PWM;
    cmd_header.cmd_id = PWM_CMD_START_MEASURE;
    cmd_header.device_index = (uint8_t)pwm_index;
    cmd_header.param_count = 0;
    cmd_header.data_len = 0;
    
    unsigned char* send_buffer;
    int total_len = build_protocol_frame(&send_buffer, &cmd_header, NULL, 0, NULL, 0);
    if (total_len < 0) {
        return USB_ERROR_OTHER;
    }
    
    int ret = usb_middleware_write_data(device_id, send_buffer, total_len);
    free(send_buffer);
    
    debug_printf("PWM开始测量结果: %d", ret);
    return (ret >= 0) ? USB_SUCCESS : USB_ERROR_OTHER;
}

WINAPI int PWM_StopMeasure(const char* target_serial, int pwm_index) {
    debug_printf("PWM_StopMeasure开始执行");
    if (!target_serial) {
        debug_printf("参数无效: target_serial=%p", target_serial);
        return USB_ERROR_INVALID_PARAM;
    }
    
    if (pwm_index < 1 || pwm_index > 4) {
        debug_printf("PWM通道索引无效: %d (有效范围: 1-4)", pwm_index);
        return USB_ERROR_INVALID_PARAM;
    }
    
    int device_id = usb_middleware_find_device_by_serial(target_serial);
    if (device_id < 0) {
        debug_printf("设备未打开: %s", target_serial);
        return USB_ERROR_OTHER;
    }
    
    GENERIC_CMD_HEADER cmd_header;
    cmd_header.protocol_type = PROTOCOL_PWM;
    cmd_header.cmd_id = PWM_CMD_STOP_MEASURE;
    cmd_header.device_index = (uint8_t)pwm_index;
    cmd_header.param_count = 0;
    cmd_header.data_len = 0;
    
    unsigned char* send_buffer;
    int total_len = build_protocol_frame(&send_buffer, &cmd_header, NULL, 0, NULL, 0);
    if (total_len < 0) {
        return USB_ERROR_OTHER;
    }
    
    int ret = usb_middleware_write_data(device_id, send_buffer, total_len);
    free(send_buffer);
    
    debug_printf("PWM停止测量结果: %d", ret);
    return (ret >= 0) ? USB_SUCCESS : USB_ERROR_OTHER;
}

WINAPI int PWM_GetResult(const char* target_serial, int pwm_index, PWM_MeasureResult_t* result) {
    debug_printf("PWM_GetResult开始执行");
    if (!target_serial || !result) {
        debug_printf("参数无效: target_serial=%p, result=%p", target_serial, result);
        return USB_ERROR_INVALID_PARAM;
    }
    
    if (pwm_index < 1 || pwm_index > 4) {
        debug_printf("PWM通道索引无效: %d (有效范围: 1-4)", pwm_index);
        return USB_ERROR_INVALID_PARAM;
    }
    
    int device_id = usb_middleware_find_device_by_serial(target_serial);
    if (device_id < 0) {
        debug_printf("设备未打开: %s", target_serial);
        return USB_ERROR_OTHER;
    }
    
    // 发送获取结果命令
    GENERIC_CMD_HEADER cmd_header;
    cmd_header.protocol_type = PROTOCOL_PWM;
    cmd_header.cmd_id = PWM_CMD_GET_RESULT;
    cmd_header.device_index = (uint8_t)pwm_index;
    cmd_header.param_count = 0;
    cmd_header.data_len = 0;
    
    unsigned char* send_buffer;
    int total_len = build_protocol_frame(&send_buffer, &cmd_header, NULL, 0, NULL, 0);
    if (total_len < 0) {
        return USB_ERROR_OTHER;
    }
    
    int ret = usb_middleware_write_data(device_id, send_buffer, total_len);
    free(send_buffer);
    
    if (ret < 0) {
        debug_printf("发送PWM获取结果命令失败: %d", ret);
        return USB_ERROR_OTHER;
    }
    
    // 等待并读取PWM测量结果响应
    unsigned char response_buffer[64];  // 足够大的缓冲区
    int max_loops = 2000;  // 最大等待2秒
    
    for (int i = 0; i < max_loops; i++) {
        int actual_read = usb_middleware_read_pwm_data(device_id, response_buffer, sizeof(response_buffer));
        if (actual_read >= sizeof(GENERIC_CMD_HEADER) + sizeof(PWM_MeasureResult_t)) {
            GENERIC_CMD_HEADER* header = (GENERIC_CMD_HEADER*)response_buffer;
            if (header->protocol_type == PROTOCOL_PWM && 
                header->cmd_id == PWM_CMD_GET_RESULT &&
                header->device_index == pwm_index) {
                
                // 复制PWM测量结果
                memcpy(result, response_buffer + sizeof(GENERIC_CMD_HEADER), sizeof(PWM_MeasureResult_t));
                
                debug_printf("PWM CH%d 测量结果: Freq=%luHz, Duty=%lu.%02lu%%, Period=%luus, PulseWidth=%luus",
                           pwm_index, result->frequency, 
                           result->duty_cycle / 100, result->duty_cycle % 100,
                           result->period_us, result->pulse_width_us);
                
                // 直接返回成功，不做业务判断
                return USB_SUCCESS;
            }
        }
        Sleep(1);
    }
    
    debug_printf("PWM获取结果超时");
    return USB_ERROR_TIMEOUT;
}