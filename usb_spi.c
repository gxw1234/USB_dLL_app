#include "usb_spi.h"
#include "usb_device.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 协议类型和命令ID定义（应与STM32端保持一致）
#define PROTOCOL_SPI        0x01    // SPI协议
#define PROTOCOL_IIC        0x02    // IIC协议
#define PROTOCOL_UART       0x03    // UART协议
#define PROTOCOL_GPIO       0x04    // GPIO协议

// 通用命令ID定义
#define CMD_INIT            0x01    // 初始化命令
#define CMD_WRITE           0x02    // 写数据命令
#define CMD_READ            0x03    // 读数据命令
#define CMD_TRANSFER        0x04    // 读写数据命令
#define CMD_END_MARKER      0xA5A5A5A5 // 命令包结束符

// 通用命令包头结构 (已移除end_marker字段)
typedef struct _GENERIC_CMD_HEADER {
  uint8_t protocol_type;  // 协议类型：SPI/IIC/UART等
  uint8_t cmd_id;         // 命令ID：初始化/读/写等
  uint8_t device_index;   // 设备索引
  uint8_t param_count;    // 参数数量
  uint16_t data_len;      // 数据部分长度
  uint16_t total_packets; // 整包总数
} GENERIC_CMD_HEADER, *PGENERIC_CMD_HEADER;

// 简化的参数头结构
typedef struct _PARAM_HEADER {
  uint16_t param_len;     // 参数长度
} PARAM_HEADER, *PPARAM_HEADER;

// 内部辅助函数声明
extern void debug_printf(const char *format, ...);

/**
 * @brief 添加参数到参数缓冲区
 * 
 * @param buffer 目标缓冲区
 * @param pos 当前位置指针
 * @param data 参数数据
 * @param len 参数长度
 * @return int 添加后的位置
 */
static int Add_Parameter(unsigned char* buffer, int pos, void* data, uint16_t len) {
    // 添加参数头
    PARAM_HEADER header;
    header.param_len = len;
    
    // 复制参数头
    memcpy(buffer + pos, &header, sizeof(PARAM_HEADER));
    pos += sizeof(PARAM_HEADER);
    
    // 复制参数数据
    memcpy(buffer + pos, data, len);
    pos += len;
    
    return pos;
}

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
    

    GENERIC_CMD_HEADER cmd_header;
    cmd_header.protocol_type = PROTOCOL_SPI;     // SPI协议
    cmd_header.cmd_id = CMD_INIT;               // 初始化命令
    cmd_header.device_index = (uint8_t)SPIIndex; // 设备索引
    cmd_header.param_count = 1;                 // 参数数量：1个
    cmd_header.data_len = 0;                    // 数据部分长度为0

    // 计算总长度，包括命令头、参数头、SPI配置和结束符
    cmd_header.total_packets = sizeof(GENERIC_CMD_HEADER) + sizeof(PARAM_HEADER) + sizeof(SPI_CONFIG) + sizeof(uint32_t);
    int total_len = sizeof(GENERIC_CMD_HEADER) + sizeof(PARAM_HEADER) + sizeof(SPI_CONFIG) + sizeof(uint32_t);
    
    unsigned char* send_buffer = (unsigned char*)malloc(total_len);
    if (!send_buffer) {
        debug_printf("内存分配失败");
        return SPI_ERROR_OTHER;
    }
    
    // 复制命令头
    memcpy(send_buffer, &cmd_header, sizeof(GENERIC_CMD_HEADER));
    int pos = sizeof(GENERIC_CMD_HEADER);
    
    // 添加参数
    pos = Add_Parameter(send_buffer, pos, pConfig, sizeof(SPI_CONFIG));
    
    // 在数据包末尾添加结束符
    uint32_t end_marker = CMD_END_MARKER;
    memcpy(send_buffer + pos, &end_marker, sizeof(uint32_t));

    int ret = USB_WriteData(target_serial, send_buffer, total_len);
    free(send_buffer);
    
    if (ret < 0) {
        debug_printf("发送SPI初始化命令失败: %d", ret);
        return SPI_ERROR_IO;
    }
    
    debug_printf("成功发送SPI初始化命令，SPI索引: %d", SPIIndex);
    return SPI_SUCCESS;
}

/**
 * @brief 通过SPI发送数据
 * 
 * @param target_serial 目标设备的序列号
 * @param SPIIndex SPI索引
 * @param pWriteBuffer 要发送的数据缓冲区
 * @param WriteLen 要发送的数据长度
 * @return int 成功返回0，失败返回错误代码
 */
WINAPI int SPI_WriteBytes(const char* target_serial, int SPIIndex, unsigned char* pWriteBuffer, int WriteLen) {
    // 验证参数
    if (!target_serial || !pWriteBuffer || WriteLen <= 0) {
        debug_printf("参数无效: target_serial=%p, pWriteBuffer=%p, WriteLen=%d", target_serial, pWriteBuffer, WriteLen);
        return SPI_ERROR_INVALID_PARAM;
    }
    
    if (SPIIndex < SPI1_CS0 || SPIIndex > SPI2_CS2) {
        debug_printf("SPI索引无效: %d", SPIIndex);
        return SPI_ERROR_INVALID_PARAM;
    }
    
    GENERIC_CMD_HEADER cmd_header;
    cmd_header.protocol_type = PROTOCOL_SPI;    // SPI协议
    cmd_header.cmd_id = CMD_WRITE;             // 写数据命令
    cmd_header.device_index = (uint8_t)SPIIndex; // 设备索引
    cmd_header.param_count = 0;                // 参数数量，写操作不需要额外参数
    cmd_header.data_len = WriteLen;            // 数据部分长度
    cmd_header.total_packets = sizeof(GENERIC_CMD_HEADER) + WriteLen + sizeof(uint32_t);  // 总大小，包含结束符
    

    // 计算总长度，包含命令头 + 数据 + 结束符
    int total_len = sizeof(GENERIC_CMD_HEADER) + WriteLen + sizeof(uint32_t);
    unsigned char* send_buffer = (unsigned char*)malloc(total_len);
    if (!send_buffer) {
        debug_printf("内存分配失败");
        return SPI_ERROR_OTHER;
    }
    
    memcpy(send_buffer, &cmd_header, sizeof(GENERIC_CMD_HEADER));
    memcpy(send_buffer + sizeof(GENERIC_CMD_HEADER), pWriteBuffer, WriteLen);
    uint32_t end_marker = CMD_END_MARKER;
    memcpy(send_buffer + sizeof(GENERIC_CMD_HEADER) + WriteLen, &end_marker, sizeof(uint32_t));
    int ret = USB_WriteData(target_serial, send_buffer, total_len);
    
    // 释放缓冲区
    free(send_buffer);
    
    if (ret < 0) {
        debug_printf("发送SPI写数据命令失败: %d", ret);
        return SPI_ERROR_IO;
    }
    
    debug_printf("成功发送SPI写数据命令，SPI索引: %d, 数据长度: %d字节", SPIIndex, WriteLen);
    return SPI_SUCCESS;
}
