#include "usb_gpio.h"
#include "usb_device.h"
#include "usb_middleware.h"
#include "usb_protocol.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "usb_log.h"


WINAPI int GPIO_SetOutput(const char* target_serial, int GPIOIndex, uint8_t pull_mode) {
    debug_printf("GPIO_SetOutput开始执行");
    if (!target_serial) {
        debug_printf("参数无效: target_serial=%p", target_serial);
        return USB_ERROR_INVALID_PARAM;
    }
    int device_id = usb_middleware_find_device_by_serial(target_serial);
    if (device_id < 0) {
        debug_printf("设备未打开: %s", target_serial);
        return USB_ERROR_OTHER;
    }
    GENERIC_CMD_HEADER cmd_header;
    cmd_header.protocol_type = PROTOCOL_GPIO;
    cmd_header.cmd_id = GPIO_DIR_OUTPUT;
    cmd_header.device_index = (uint8_t)GPIOIndex;
    cmd_header.param_count = 1;
    cmd_header.data_len = 0;
    unsigned char* send_buffer;
    int total_len = build_protocol_frame(&send_buffer, &cmd_header, &pull_mode, sizeof(uint8_t), NULL, 0);
    if (total_len < 0) {
        return USB_ERROR_OTHER;
    }
    int ret = usb_middleware_write_data(device_id, send_buffer, total_len);
    free(send_buffer);
    debug_printf("GPIO设置输出结果: %d", ret);
    return (ret >= 0) ? USB_SUCCESS : USB_ERROR_OTHER;
}

WINAPI int GPIO_SetOpenDrain(const char* target_serial, int GPIOIndex, uint8_t pull_mode) {
    debug_printf("GPIO_SetOpenDrain开始执行");
    if (!target_serial) {
        debug_printf("参数无效: target_serial=%p", target_serial);
        return USB_ERROR_INVALID_PARAM;
    }
    int device_id = usb_middleware_find_device_by_serial(target_serial);
    if (device_id < 0) {
        debug_printf("设备未打开: %s", target_serial);
        return USB_ERROR_OTHER;
    }
    
    GENERIC_CMD_HEADER cmd_header;
    cmd_header.protocol_type = PROTOCOL_GPIO;
    cmd_header.cmd_id = GPIO_DIR_OUTPUT_OD;
    cmd_header.device_index = (uint8_t)GPIOIndex;
    cmd_header.param_count = 1;
    cmd_header.data_len = 0;
    
    unsigned char* send_buffer;
    int total_len = build_protocol_frame(&send_buffer, &cmd_header, &pull_mode, sizeof(uint8_t), NULL, 0);
    if (total_len < 0) {
        return USB_ERROR_OTHER;
    }
    
    int ret = usb_middleware_write_data(device_id, send_buffer, total_len);
    free(send_buffer);
    
    debug_printf("GPIO设置开漏输出结果: %d", ret);
    return (ret >= 0) ? USB_SUCCESS : USB_ERROR_OTHER;
}

WINAPI int GPIO_SetInput(const char* target_serial, int GPIOIndex, uint8_t pull_mode) {
    debug_printf("GPIO_SetInput开始执行");
    if (!target_serial) {
        debug_printf("参数无效: target_serial=%p", target_serial);
        return USB_ERROR_INVALID_PARAM;
    }
    int device_id = usb_middleware_find_device_by_serial(target_serial);
    if (device_id < 0) {
        debug_printf("设备未打开: %s", target_serial);
        return USB_ERROR_OTHER;
    }
    
    GENERIC_CMD_HEADER cmd_header;
    cmd_header.protocol_type = PROTOCOL_GPIO;
    cmd_header.cmd_id = GPIO_DIR_INPUT;
    cmd_header.device_index = (uint8_t)GPIOIndex;
    cmd_header.param_count = 1;
    cmd_header.data_len = 0;
    
    unsigned char* send_buffer;
    int total_len = build_protocol_frame(&send_buffer, &cmd_header, &pull_mode, sizeof(uint8_t), NULL, 0);
    if (total_len < 0) {
        return USB_ERROR_OTHER;
    }
    
    int ret = usb_middleware_write_data(device_id, send_buffer, total_len);
    free(send_buffer);
    
    debug_printf("GPIO设置输入结果: %d", ret);
    return (ret >= 0) ? USB_SUCCESS : USB_ERROR_OTHER;
}

WINAPI int GPIO_Write(const char* target_serial, int GPIOIndex, uint8_t WriteValue) {
    if (!target_serial) {
        debug_printf("参数无效: target_serial=%p", target_serial);
        return USB_ERROR_INVALID_PARAM;
    }
    int device_id = usb_middleware_find_device_by_serial(target_serial);
    if (device_id < 0) {
        debug_printf("设备未打开: %s", target_serial);
        return USB_ERROR_OTHER;
    }
    
    GENERIC_CMD_HEADER cmd_header;
    cmd_header.protocol_type = PROTOCOL_GPIO;
    cmd_header.cmd_id = GPIO_DIR_WRITE;
    cmd_header.device_index = (uint8_t)GPIOIndex;
    cmd_header.param_count = 1;
    cmd_header.data_len = 0;
    
    unsigned char* send_buffer;
    int total_len = build_protocol_frame(&send_buffer, &cmd_header, &WriteValue, sizeof(uint8_t), NULL, 0);
    if (total_len < 0) {
        return USB_ERROR_OTHER;
    }
    int ret = usb_middleware_write_data(device_id, send_buffer, total_len);
    free(send_buffer);
    debug_printf("GPIO写入结果: %d", ret);
    return (ret >= 0) ? USB_SUCCESS : USB_ERROR_OTHER;
}

WINAPI int GPIO_scan_Write(const char* target_serial, int GPIOIndex, uint8_t WriteValue) {
    if (!target_serial) {
        debug_printf("参数无效: target_serial=%p", target_serial);
        return USB_ERROR_INVALID_PARAM;
    }
    int device_id = usb_middleware_find_device_by_serial(target_serial);
    if (device_id < 0) {
        debug_printf("设备未打开: %s", target_serial);
        return USB_ERROR_OTHER;
    }
    
    GENERIC_CMD_HEADER cmd_header;
    cmd_header.protocol_type = PROTOCOL_GPIO;
    cmd_header.cmd_id = GPIO_SCAN_DIR_WRITE;
    cmd_header.device_index = (uint8_t)GPIOIndex;
    cmd_header.param_count = 1;
    cmd_header.data_len = 0;
    unsigned char* send_buffer;
    int total_len = build_protocol_frame(&send_buffer, &cmd_header, &WriteValue, sizeof(uint8_t), NULL, 0);
    if (total_len < 0) {
        return USB_ERROR_OTHER;
    }
    int ret = usb_middleware_write_data(device_id, send_buffer, total_len);  //下压
    free(send_buffer);
    debug_printf("--GPIO_scan_Write: %d", ret);
    unsigned char response_buffer[16];  
    int max_loops =2000;

    //下压之后等IIC回复，通过USB读取状态
    for (int i = 0; i < max_loops; i++) {
        int actual_read = usb_middleware_read_status_data(device_id, response_buffer, sizeof(response_buffer));
        if (actual_read >= sizeof(GENERIC_CMD_HEADER) + 1) {  // 包含协议头+状态数据
            GENERIC_CMD_HEADER* header = (GENERIC_CMD_HEADER*)response_buffer;
            if (header->protocol_type == PROTOCOL_STATUS && 
                header->cmd_id == GPIO_SCAN_MODE_WRITE ) {
                uint8_t queue_status = response_buffer[sizeof(GENERIC_CMD_HEADER)];
                Sleep(50);
                return queue_status;
            }
        }
        Sleep(1);
    }

    debug_printf("GPIO没有收到IIC响应");
    return USB_ERROR_OTHER;
}

WINAPI int GPIO_Read(const char* target_serial, int GPIOIndex, uint8_t* level) {
    if (!target_serial || !level) {
        debug_printf("参数无效: target_serial=%p, level=%p", target_serial, level);
        return USB_ERROR_INVALID_PARAM;
    }
    int device_id = usb_middleware_find_device_by_serial(target_serial);
    if (device_id < 0) {
        debug_printf("设备未打开: %s", target_serial);
        return USB_ERROR_OTHER;
    }

    // 发送读取命令
    GENERIC_CMD_HEADER cmd_header;
    cmd_header.protocol_type = PROTOCOL_GPIO;
    cmd_header.cmd_id = GPIO_DIR_READ;
    cmd_header.device_index = (uint8_t)GPIOIndex;
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
        return USB_ERROR_OTHER;
    }

    // 从数组等待电平值（中间层在解析线程写入）
    int wait_ret = usb_middleware_wait_gpio_level(device_id, GPIOIndex, level, 2000);
    return wait_ret;
}

WINAPI int USB_device_reset(const char* target_serial) {
    if (!target_serial) {
        debug_printf("参数无效: target_serial=%p", target_serial);
        return USB_ERROR_INVALID_PARAM;
    }
    
    int device_id = usb_middleware_find_device_by_serial(target_serial);
    if (device_id < 0) {
        debug_printf("设备未打开: %s", target_serial);
        return USB_ERROR_OTHER;
    }
    
    GENERIC_CMD_HEADER cmd_header;
    cmd_header.protocol_type = PROTOCOL_RESETSTM32;
    cmd_header.cmd_id = CMD_INIT;
    cmd_header.device_index = 1;
    cmd_header.param_count = 1;
    cmd_header.data_len = 0;
    uint8_t reset_value = 1;
    unsigned char* send_buffer;
    int total_len = build_protocol_frame(&send_buffer, &cmd_header, &reset_value, sizeof(uint8_t), NULL, 0);
    if (total_len < 0) {
        return USB_ERROR_OTHER;
    }
    int ret = usb_middleware_write_data(device_id, send_buffer, total_len);
    free(send_buffer);
    return (ret >= 0) ? USB_SUCCESS : USB_ERROR_OTHER;
}
