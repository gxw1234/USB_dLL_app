#include "usb_uart.h"
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

WINAPI int UART_Init(const char* target_serial, int uart_index, UART_CONFIG* config) {
    debug_printf("UART_Init开始执行");
    if (!target_serial || !config) {
        debug_printf("参数无效: target_serial=%p, config=%p", target_serial, config);
        return USB_ERROR_INVALID_PARAM;
    }
    
    if (uart_index < 1 || uart_index > 4) {
        debug_printf("UART索引无效: %d (有效范围: 1-4)", uart_index);
        return USB_ERROR_INVALID_PARAM;
    }
    
    int device_id = usb_middleware_find_device_by_serial(target_serial);
    if (device_id < 0) {
        debug_printf("设备未打开: %s", target_serial);
        return USB_ERROR_OTHER;
    }
    
    debug_printf("UART配置: BaudRate=%d, WordLength=%d, StopBits=%d, Parity=%d, TEPolarity=0x%02X", 
                config->BaudRate, config->WordLength, config->StopBits, 
                config->Parity, config->TEPolarity);
    
    GENERIC_CMD_HEADER cmd_header;
    cmd_header.protocol_type = PROTOCOL_UART;
    cmd_header.cmd_id = CMD_INIT;
    cmd_header.device_index = (uint8_t)uart_index;
    cmd_header.param_count = 1;
    cmd_header.data_len = 0;
    
    unsigned char* send_buffer;
    int total_len = build_protocol_frame(&send_buffer, &cmd_header, (unsigned char*)config, sizeof(UART_CONFIG), NULL, 0);
    if (total_len < 0) {
        return USB_ERROR_OTHER;
    }
    
    int ret = usb_middleware_write_data(device_id, send_buffer, total_len);
    free(send_buffer);
    
    debug_printf("UART初始化结果: %d", ret);
    return (ret >= 0) ? USB_SUCCESS : USB_ERROR_OTHER;
}

WINAPI int UART_ReadBytes(const char* target_serial, int uart_index, unsigned char* pReadBuffer, int ReadLen) {
    if (!target_serial || !pReadBuffer || ReadLen <= 0) {
        debug_printf("参数无效: target_serial=%p, pReadBuffer=%p, ReadLen=%d", target_serial, pReadBuffer, ReadLen);
        return USB_ERROR_INVALID_PARAM;
    }

    int device_id = usb_middleware_find_device_by_serial(target_serial);
    if (device_id < 0) {
        debug_printf("设备未打开: %s", target_serial);
        return USB_ERROR_OTHER;
    }
    
    int actual_read = usb_middleware_read_uart_data(device_id, pReadBuffer, ReadLen);
    if (actual_read < 0) {
        debug_printf("从UART缓冲区读取数据失败: %d", actual_read);
        return USB_ERROR_OTHER;
    }
    
    if (actual_read > 0) {
        debug_printf("成功读取UART数据，UART索引: %d, 数据长度: %d字节", uart_index, actual_read);
    }
    return actual_read;
}

WINAPI int UART_WriteBytes(const char* target_serial, int uart_index, unsigned char* pWriteBuffer, int WriteLen) {
    if (!target_serial || !pWriteBuffer || WriteLen <= 0) {
        debug_printf("参数无效: target_serial=%p, pWriteBuffer=%p, WriteLen=%d", target_serial, pWriteBuffer, WriteLen);
        return USB_ERROR_INVALID_PARAM;
    }

    int device_id = usb_middleware_find_device_by_serial(target_serial);
    if (device_id < 0) {
        debug_printf("设备未打开: %s", target_serial);
        return USB_ERROR_OTHER;
    }

    GENERIC_CMD_HEADER cmd_header;
    cmd_header.protocol_type = PROTOCOL_UART;    // UART协议
    cmd_header.cmd_id = CMD_WRITE;              // 写数据命令
    cmd_header.device_index = (uint8_t)uart_index; // 设备索引
    cmd_header.param_count = 0;                 // 参数数量，写操作不需要额外参数
    cmd_header.data_len = WriteLen;             // 数据部分长度

    unsigned char* send_buffer;
    int total_len = build_protocol_frame(&send_buffer, &cmd_header, 
                                   NULL, 0, 
                                   pWriteBuffer, WriteLen);
    if (total_len < 0) {
        return USB_ERROR_OTHER;
    }
    int ret = usb_middleware_write_data(device_id, send_buffer, total_len);
    free(send_buffer);
    
    if (ret >= 0) {
        debug_printf("成功发送UART数据，UART索引: %d, 数据长度: %d字节", uart_index, WriteLen);
        return USB_SUCCESS;
    } else {
        debug_printf("发送UART数据失败: %d", ret);
        return USB_ERROR_OTHER;
    }
}