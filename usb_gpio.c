#include "usb_gpio.h"
#include "usb_device.h"
#include "usb_middleware.h"
#include "usb_protocol.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct _GENERIC_CMD_HEADER {
  uint8_t protocol_type;  // 协议类型：SPI/IIC/UART/GPIO等
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

// 通用的帧构建函数
static int build_gpio_frame(unsigned char** buffer, GENERIC_CMD_HEADER* cmd_header, void* param_data, size_t param_size) {
    // total_packets只包含协议数据部分，不包含帧头和帧尾
    cmd_header->total_packets = sizeof(GENERIC_CMD_HEADER) + sizeof(PARAM_HEADER) + param_size;
    int total_len = sizeof(uint32_t) + sizeof(GENERIC_CMD_HEADER) + sizeof(PARAM_HEADER) + param_size + sizeof(uint32_t);
    
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
    
    // 添加参数头和参数体
    PARAM_HEADER param_header;
    param_header.param_len = param_size;
    memcpy(*buffer + pos, &param_header, sizeof(PARAM_HEADER));
    pos += sizeof(PARAM_HEADER);
    memcpy(*buffer + pos, param_data, param_size);
    pos += param_size;
    
    // 添加帧尾标识符
    uint32_t end_marker = CMD_END_MARKER;
    memcpy(*buffer + pos, &end_marker, sizeof(uint32_t));
    
    debug_printf("[PC] GPIO sending frame with header 0x%08X, protocol_type=%d, cmd_id=%d", 
                 FRAME_START_MARKER, cmd_header->protocol_type, cmd_header->cmd_id);
    
    return total_len;
}

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
    int total_len = build_gpio_frame(&send_buffer, &cmd_header, &pull_mode, sizeof(uint8_t));
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
    int total_len = build_gpio_frame(&send_buffer, &cmd_header, &pull_mode, sizeof(uint8_t));
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
    int total_len = build_gpio_frame(&send_buffer, &cmd_header, &pull_mode, sizeof(uint8_t));
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
    int total_len = build_gpio_frame(&send_buffer, &cmd_header, &WriteValue, sizeof(uint8_t));
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
    int total_len = build_gpio_frame(&send_buffer, &cmd_header, &WriteValue, sizeof(uint8_t));
    if (total_len < 0) {
        return USB_ERROR_OTHER;
    }
    
    int ret = usb_middleware_write_data(device_id, send_buffer, total_len);
    free(send_buffer);
    
    debug_printf("--GPIO_scan_Write: %d", ret);
    
    // 给STM32一点时间处理命令和准备响应
    Sleep(1);
    
    unsigned char response_buffer[1];
    int max_loops = 10000000;
    for (int i = 0; i < max_loops; i++) {
        int actual_read = usb_middleware_read_spi_data(device_id, response_buffer, 1);
        if (actual_read > 0) {
            Sleep(20);
            return response_buffer[0];
        }
    }
    debug_printf("GPIO没有收到IIC响应");
    return (ret >= 0) ? USB_SUCCESS : USB_ERROR_OTHER;
}

WINAPI int STM32_reset(const char* target_serial) {
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
    int total_len = build_gpio_frame(&send_buffer, &cmd_header, &reset_value, sizeof(uint8_t));
    if (total_len < 0) {
        return USB_ERROR_OTHER;
    }
    
    int ret = usb_middleware_write_data(device_id, send_buffer, total_len);
    free(send_buffer);
    
    debug_printf("--复位: %d", ret);
    return (ret >= 0) ? USB_SUCCESS : USB_ERROR_OTHER;
}
