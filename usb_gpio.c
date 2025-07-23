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
    cmd_header.data_len = 0; // 数据部分长度为0，参数通过参数区传递
    cmd_header.total_packets = sizeof(GENERIC_CMD_HEADER) + sizeof(PARAM_HEADER) + sizeof(uint8_t) + sizeof(uint32_t);
    int total_len = sizeof(GENERIC_CMD_HEADER) + sizeof(PARAM_HEADER) + sizeof(uint8_t) + sizeof(uint32_t);
    unsigned char* send_buffer = (unsigned char*)malloc(total_len);
    if (!send_buffer) {
        debug_printf("内存分配失败");
        return USB_ERROR_OTHER;
    }
    memcpy(send_buffer, &cmd_header, sizeof(GENERIC_CMD_HEADER));
    int pos = sizeof(GENERIC_CMD_HEADER);
    // 添加参数头和参数体
    PARAM_HEADER param_header;
    param_header.param_len = sizeof(uint8_t);
    memcpy(send_buffer + pos, &param_header, sizeof(PARAM_HEADER));
    pos += sizeof(PARAM_HEADER);
    memcpy(send_buffer + pos, &pull_mode, sizeof(uint8_t));
    pos += sizeof(uint8_t);
    uint32_t end_marker = CMD_END_MARKER;
    memcpy(send_buffer + pos, &end_marker, sizeof(uint32_t));
    int ret = usb_middleware_write_data(device_id, send_buffer, total_len);
    free(send_buffer);
    
    debug_printf("GPIO设置输出结果: %d", ret);
    return (ret >= 0) ? USB_SUCCESS : USB_ERROR_OTHER;
}





WINAPI int GPIO_SetOpenDrain(const char* target_serial, int GPIOIndex, uint8_t pull_mode) {
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
    cmd_header.cmd_id = GPIO_DIR_OUTPUT_OD;
    cmd_header.device_index = (uint8_t)GPIOIndex;
    cmd_header.param_count = 1;
    cmd_header.data_len = 0; // 数据部分长度为0，参数通过参数区传递
    cmd_header.total_packets = sizeof(GENERIC_CMD_HEADER) + sizeof(PARAM_HEADER) + sizeof(uint8_t) + sizeof(uint32_t);
    int total_len = sizeof(GENERIC_CMD_HEADER) + sizeof(PARAM_HEADER) + sizeof(uint8_t) + sizeof(uint32_t);
    unsigned char* send_buffer = (unsigned char*)malloc(total_len);
    if (!send_buffer) {
        debug_printf("内存分配失败");
        return USB_ERROR_OTHER;
    }
    memcpy(send_buffer, &cmd_header, sizeof(GENERIC_CMD_HEADER));
    int pos = sizeof(GENERIC_CMD_HEADER);
    // 添加参数头和参数体
    PARAM_HEADER param_header;
    param_header.param_len = sizeof(uint8_t);
    memcpy(send_buffer + pos, &param_header, sizeof(PARAM_HEADER));
    pos += sizeof(PARAM_HEADER);
    memcpy(send_buffer + pos, &pull_mode, sizeof(uint8_t));
    pos += sizeof(uint8_t);
    uint32_t end_marker = CMD_END_MARKER;
    memcpy(send_buffer + pos, &end_marker, sizeof(uint32_t));
    int ret = usb_middleware_write_data(device_id, send_buffer, total_len);
    free(send_buffer);
    
    debug_printf("GPIO设置输出结果: %d", ret);
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
    
    // 组包协议头和数据（使用通用参数格式）
    GENERIC_CMD_HEADER cmd_header;
    cmd_header.protocol_type = PROTOCOL_GPIO;
    cmd_header.cmd_id = GPIO_DIR_INPUT;  
    cmd_header.device_index = (uint8_t)GPIOIndex;
    cmd_header.param_count = 1;
    cmd_header.data_len = 0; // 数据部分长度为0，参数通过参数区传递
    cmd_header.total_packets = sizeof(GENERIC_CMD_HEADER) + sizeof(PARAM_HEADER) + sizeof(uint8_t) + sizeof(uint32_t);
    int total_len = sizeof(GENERIC_CMD_HEADER) + sizeof(PARAM_HEADER) + sizeof(uint8_t) + sizeof(uint32_t);
    unsigned char* send_buffer = (unsigned char*)malloc(total_len);
    if (!send_buffer) {
        debug_printf("内存分配失败");
        return USB_ERROR_OTHER;
    }
    memcpy(send_buffer, &cmd_header, sizeof(GENERIC_CMD_HEADER));
    int pos = sizeof(GENERIC_CMD_HEADER);
    // 添加参数头和参数体
    PARAM_HEADER param_header;
    param_header.param_len = sizeof(uint8_t);
    memcpy(send_buffer + pos, &param_header, sizeof(PARAM_HEADER));
    pos += sizeof(PARAM_HEADER);
    memcpy(send_buffer + pos, &pull_mode, sizeof(uint8_t));
    pos += sizeof(uint8_t);
    uint32_t end_marker = CMD_END_MARKER;
    memcpy(send_buffer + pos, &end_marker, sizeof(uint32_t));
    int ret = usb_middleware_write_data(device_id, send_buffer, total_len);
    free(send_buffer);
    debug_printf("GPIO设置输入结果: %d", ret);
    return (ret >= 0) ? USB_SUCCESS : USB_ERROR_OTHER;
}


WINAPI int GPIO_Write(const char* target_serial, int GPIOIndex, uint8_t WriteValue) {
    debug_printf("GPIO_Write开始执行");
    if (!target_serial) {
        debug_printf("参数无效: target_serial=%p", target_serial);
        return USB_ERROR_INVALID_PARAM;
    }

    
    int device_id = usb_middleware_find_device_by_serial(target_serial);
    if (device_id < 0) {
        debug_printf("设备未打开: %s", target_serial);
        return USB_ERROR_OTHER;
    }
    
    // 组包协议头和数据（使用通用参数格式）
    GENERIC_CMD_HEADER cmd_header;
    cmd_header.protocol_type = PROTOCOL_GPIO;
    cmd_header.cmd_id = GPIO_DIR_WRITE;
    cmd_header.device_index = (uint8_t)GPIOIndex;
    cmd_header.param_count = 1;
    cmd_header.data_len = 0; // 数据部分长度为0，参数通过参数区传递
    cmd_header.total_packets = sizeof(GENERIC_CMD_HEADER) + sizeof(PARAM_HEADER) + sizeof(uint8_t) + sizeof(uint32_t);
    int total_len = sizeof(GENERIC_CMD_HEADER) + sizeof(PARAM_HEADER) + sizeof(uint8_t) + sizeof(uint32_t);
    unsigned char* send_buffer = (unsigned char*)malloc(total_len);
    if (!send_buffer) {
        debug_printf("内存分配失败");
        return USB_ERROR_OTHER;
    }
    memcpy(send_buffer, &cmd_header, sizeof(GENERIC_CMD_HEADER));
    int pos = sizeof(GENERIC_CMD_HEADER);
    // 添加参数头和参数体
    PARAM_HEADER param_header;
    param_header.param_len = sizeof(uint8_t);
    memcpy(send_buffer + pos, &param_header, sizeof(PARAM_HEADER));
    pos += sizeof(PARAM_HEADER);
    memcpy(send_buffer + pos, &WriteValue, sizeof(uint8_t));
    pos += sizeof(uint8_t);
    uint32_t end_marker = CMD_END_MARKER;
    memcpy(send_buffer + pos, &end_marker, sizeof(uint32_t));
    int ret = usb_middleware_write_data(device_id, send_buffer, total_len);
    free(send_buffer);
    // debug_printf("GPIO_Write开始执行, index=%d, value=0x%02X", GPIOIndex, WriteValue);
    debug_printf("--GPIO写入结果: %d", ret);
    return (ret >= 0) ? USB_SUCCESS : USB_ERROR_OTHER;
}
WINAPI int STM32_reset(const char* target_serial) {
    debug_printf("STM32_reset");
    if (!target_serial) {
        debug_printf("参数无效: target_serial=%p", target_serial);
        return USB_ERROR_INVALID_PARAM;
    }


    int GPIOIndex =1;
    uint8_t WriteValue = 1;

    
    int device_id = usb_middleware_find_device_by_serial(target_serial);
    if (device_id < 0) {
        debug_printf("设备未打开: %s", target_serial);
        return USB_ERROR_OTHER;
    }
    
    // 组包协议头和数据（使用通用参数格式）
    GENERIC_CMD_HEADER cmd_header;
    cmd_header.protocol_type = PROTOCOL_RESETSTM32;
    cmd_header.cmd_id = CMD_INIT;
    cmd_header.device_index = (uint8_t)1;
    cmd_header.param_count = 1;
    cmd_header.data_len = 0; // 数据部分长度为0，参数通过参数区传递
    cmd_header.total_packets = sizeof(GENERIC_CMD_HEADER) + sizeof(PARAM_HEADER) + sizeof(uint8_t) + sizeof(uint32_t);
    int total_len = sizeof(GENERIC_CMD_HEADER) + sizeof(PARAM_HEADER) + sizeof(uint8_t) + sizeof(uint32_t);
    unsigned char* send_buffer = (unsigned char*)malloc(total_len);
    if (!send_buffer) {
        debug_printf("内存分配失败");
        return USB_ERROR_OTHER;
    }
    memcpy(send_buffer, &cmd_header, sizeof(GENERIC_CMD_HEADER));
    int pos = sizeof(GENERIC_CMD_HEADER);
    // 添加参数头和参数体
    PARAM_HEADER param_header;
    param_header.param_len = sizeof(uint8_t);
    memcpy(send_buffer + pos, &param_header, sizeof(PARAM_HEADER));
    pos += sizeof(PARAM_HEADER);
    memcpy(send_buffer + pos, &WriteValue, sizeof(uint8_t));
    pos += sizeof(uint8_t);
    uint32_t end_marker = CMD_END_MARKER;
    memcpy(send_buffer + pos, &end_marker, sizeof(uint32_t));
    int ret = usb_middleware_write_data(device_id, send_buffer, total_len);
    free(send_buffer);
    debug_printf("--复位: %d", ret);
    return (ret >= 0) ? USB_SUCCESS : USB_ERROR_OTHER;
}
