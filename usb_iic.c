#include "usb_iic.h"
#include "usb_device.h"
#include "usb_application.h"
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

typedef struct _GENERIC_CMD_HEADER {
  uint8_t protocol_type;  // 协议类型：SPI/IIC/UART等
  uint8_t cmd_id;         // 命令ID：初始化/读/写等
  uint8_t device_index;   // 设备索引
  uint8_t param_count;    // 参数数量
  uint16_t data_len;      // 数据部分长度
} GENERIC_CMD_HEADER, *PGENERIC_CMD_HEADER;

typedef struct _PARAM_HEADER {
  uint16_t param_len;     // 参数长度
} PARAM_HEADER, *PPARAM_HEADER;

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
    PARAM_HEADER header;
    header.param_len = len;
    memcpy(buffer + pos, &header, sizeof(PARAM_HEADER));
    pos += sizeof(PARAM_HEADER);
    
    // 复制参数数据
    memcpy(buffer + pos, data, len);
    pos += len;
    
    return pos;
}

/**
 * @brief 初始化IIC
 * 
 * @param target_serial 目标设备的序列号
 * @param IICIndex IIC索引
 * @param pConfig IIC配置结构体
 * @return int 成功返回0，失败返回错误代码
 */
WINAPI int IIC_Init(const char* target_serial, int IICIndex, PIIC_CONFIG pConfig) {
    // 验证参数
    if (!target_serial || !pConfig) {
        debug_printf("参数无效: target_serial=%p, pConfig=%p", target_serial, pConfig);
        return IIC_ERROR_INVALID_PARAM;
    }
    
    if (IICIndex < IIC1_INDEX || IICIndex > IIC2_INDEX) {
        debug_printf("IIC索引无效: %d", IICIndex);
        return IIC_ERROR_INVALID_PARAM;
    }
    GENERIC_CMD_HEADER cmd_header;
    cmd_header.protocol_type = PROTOCOL_IIC;     // IIC协议
    cmd_header.cmd_id = CMD_INIT;               // 初始化命令
    cmd_header.device_index = (uint8_t)IICIndex; // 设备索引
    cmd_header.param_count = 1;                 // 参数数量：1个
    cmd_header.data_len = 0;                    // 数据部分长度为0
    
    // 计算总长度：命令头 + 参数头 + 参数值
    int total_len = sizeof(GENERIC_CMD_HEADER) + sizeof(PARAM_HEADER) + sizeof(IIC_CONFIG);
    unsigned char* send_buffer = (unsigned char*)malloc(total_len);
    if (!send_buffer) {
        debug_printf("内存分配失败");
        return IIC_ERROR_OTHER;
    }
    memcpy(send_buffer, &cmd_header, sizeof(GENERIC_CMD_HEADER));
    int pos = sizeof(GENERIC_CMD_HEADER);
    pos = Add_Parameter(send_buffer, pos, pConfig, sizeof(IIC_CONFIG));
    
    int ret = USB_WriteData(target_serial, send_buffer, total_len);
    free(send_buffer);
    
    if (ret < 0) {
        debug_printf("发送IIC初始化命令失败: %d", ret);
        return IIC_ERROR_IO;
    }
    
    debug_printf("成功发送IIC初始化命令，IIC索引: %d", IICIndex);
    return IIC_SUCCESS;
}

/**
 * @brief IIC从机写数据
 * 
 * @param DevHandle 设备句柄
 * @param IICIndex IIC索引
 * @param pWriteData 要发送的数据缓冲区
 * @param WriteLen 要发送的数据长度
 * @param TimeOutMs 超时时间(毫秒)
 * @return int 成功返回0，失败返回错误代码
 */
WINAPI int IIC_SlaveWriteBytes(const char* target_serial, int IICIndex, unsigned char *pWriteData, int WriteLen, int TimeOutMs) {
    // 验证参数
    if (!target_serial || !pWriteData || WriteLen <= 0 || TimeOutMs < 0) {
        debug_printf("参数无效: target_serial=%p, pWriteData=%p, WriteLen=%d, TimeOutMs=%d", 
                    target_serial, pWriteData, WriteLen, TimeOutMs);
        return IIC_ERROR_INVALID_PARAM;
    }
    
    if (IICIndex < IIC1_INDEX || IICIndex > IIC2_INDEX) {
        debug_printf("IIC索引无效: %d", IICIndex);
        return IIC_ERROR_INVALID_PARAM;
    }
    
    GENERIC_CMD_HEADER cmd_header;
    cmd_header.protocol_type = PROTOCOL_IIC;     // IIC协议
    cmd_header.cmd_id = CMD_WRITE;              // 写数据命令(这里使用写命令)
    cmd_header.device_index = (uint8_t)IICIndex; // 设备索引
    cmd_header.param_count = 0;                 // 参数数量：0个
    cmd_header.data_len = WriteLen;             // 数据部分长度
    int total_len = sizeof(GENERIC_CMD_HEADER) + WriteLen;

    unsigned char* send_buffer = (unsigned char*)malloc(total_len);
    if (!send_buffer) {
        debug_printf("内存分配失败");
        return IIC_ERROR_OTHER;
    }
    
    memcpy(send_buffer, &cmd_header, sizeof(GENERIC_CMD_HEADER));

    memcpy(send_buffer + sizeof(GENERIC_CMD_HEADER), pWriteData, WriteLen);
    debug_printf("IIC_SlaveWriteBytes: target_serial=%s, IICIndex=%d, WriteLen=%d, TimeOutMs=%d", 
               target_serial, IICIndex, WriteLen, TimeOutMs);
    int ret = USB_WriteData(target_serial, send_buffer, total_len);
    free(send_buffer);
    if (ret < 0) {
        debug_printf("发送IIC从机写数据命令失败: %d", ret);
        return IIC_ERROR_IO;
    }
    debug_printf("成功发送IIC从机写数据命令，IIC索引: %d, 数据长度: %d字节, 超时: %dms", 
                IICIndex, WriteLen, TimeOutMs);
    return IIC_SUCCESS;
}

