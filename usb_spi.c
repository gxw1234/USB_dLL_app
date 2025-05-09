#include "usb_spi.h"
#include "usb_device.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief 初始化SPI
 * 
 * @param target_serial 目标设备的序列号
 * @param SPIIndex SPI索引
 * @param pConfig SPI配置结构体
 * @return int 成功返回0，失败返回错误代码
 */
WINAPI int SPI_Init(const char* target_serial, int SPIIndex, PSPI_CONFIG pConfig) {
    // 验证参数
    if (!target_serial || !pConfig) {
        debug_printf("参数无效: target_serial=%p, pConfig=%p", target_serial, pConfig);
        return SPI_ERROR_INVALID_PARAM;
    }
    
    if (SPIIndex < SPI1_CS0 || SPIIndex > SPI2_CS2) {
        debug_printf("SPI索引无效: %d", SPIIndex);
        return SPI_ERROR_INVALID_PARAM;
    }
    
    // 创建命令包
    CMD_HEADER cmd_header;
    cmd_header.cmd_id = CMD_SPI_INIT;
    cmd_header.spi_index = (unsigned char)SPIIndex;
    cmd_header.data_len = sizeof(SPI_CONFIG);
    
    // 创建发送缓冲区
    int total_len = sizeof(CMD_HEADER) + sizeof(SPI_CONFIG);
    unsigned char* send_buffer = (unsigned char*)malloc(total_len);
    if (!send_buffer) {
        debug_printf("内存分配失败");
        return SPI_ERROR_OTHER;
    }
    
    // 复制命令头和SPI配置到缓冲区
    memcpy(send_buffer, &cmd_header, sizeof(CMD_HEADER));
    memcpy(send_buffer + sizeof(CMD_HEADER), pConfig, sizeof(SPI_CONFIG));
    
    // 使用USB_WriteData发送数据
    int ret = USB_WriteData(target_serial, send_buffer, total_len);
    
    // 释放缓冲区
    free(send_buffer);
    
    if (ret < 0) {
        debug_printf("发送SPI初始化命令失败: %d", ret);
        return SPI_ERROR_IO;
    }
    
    debug_printf("成功发送SPI初始化命令，SPI索引: %d", SPIIndex);
    return SPI_SUCCESS;
}
