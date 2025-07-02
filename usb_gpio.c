#include "usb_gpio.h"
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
#define CMD_SET_DIR         0x04    // 设置方向命令
#define CMD_TRANSFER        0x05    // 读写数据命令
#define CMD_END_MARKER      0xA5A5A5A5 // 命令包结束符

// 通用命令包头结构
typedef struct _GENERIC_CMD_HEADER {
  uint8_t protocol_type;  // 协议类型：SPI/IIC/UART/GPIO等
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
 * @brief 设置GPIO为输出模式
 * 
 * @param target_serial 目标设备的序列号
 * @param GPIOIndex GPIO索引
 * @param OutputMask 输出引脚掩码，每个位对应一个引脚，1表示设置为输出
 * @return int 成功返回0，失败返回错误代码
 */
WINAPI int GPIO_SetOutput(const char* target_serial, int GPIOIndex, uint8_t OutputMask) {
    // 验证参数
    debug_printf("GPIO_SetOutput开始执行");
    if (!target_serial) {
        debug_printf("参数无效: target_serial=%p", target_serial);
        return GPIO_ERR_PARAM_INVALID;
    }
    
    if (GPIOIndex < GPIO_PORT0 || GPIOIndex > GPIO_PORT2) {
        debug_printf("GPIO索引无效: %d", GPIOIndex);
        return GPIO_ERR_PARAM_INVALID;
    }
    
    // 创建通用命令包头
    GENERIC_CMD_HEADER cmd_header;
    cmd_header.protocol_type = PROTOCOL_GPIO;     // GPIO协议
    cmd_header.cmd_id = CMD_SET_DIR;            // 设置方向命令
    cmd_header.device_index = (uint8_t)GPIOIndex; // 设备索引
    cmd_header.param_count = 0;                 // 参数数量：0个
    cmd_header.data_len = 1;                    // 数据部分长度: 1字节掩码
    
    // 计算总长度：命令头 + 数据 + 结束符
    int total_len = sizeof(GENERIC_CMD_HEADER) + 1 + sizeof(uint32_t);
    cmd_header.total_packets = total_len;
    
    // 分配发送缓冲区
    unsigned char* send_buffer = (unsigned char*)malloc(total_len);
    if (!send_buffer) {
        debug_printf("内存分配失败");
        return GPIO_ERR_OTHER;
    }
    
    // 复制命令头到缓冲区
    memcpy(send_buffer, &cmd_header, sizeof(GENERIC_CMD_HEADER));
    
    // 复制输出掩码
    send_buffer[sizeof(GENERIC_CMD_HEADER)] = OutputMask;
    
    // 在数据包末尾添加结束符
    uint32_t end_marker = CMD_END_MARKER;
    memcpy(send_buffer + sizeof(GENERIC_CMD_HEADER) + 1, &end_marker, sizeof(uint32_t));
    
    debug_printf("发送GPIO设置输出命令: protocol=%d, cmd=%d, index=%d, data_len=%d, mask=0x%02X",
                 cmd_header.protocol_type, cmd_header.cmd_id, cmd_header.device_index, 
                 cmd_header.data_len, OutputMask);
    
    // 使用USB_WriteData发送数据
    int ret = USB_WriteData(target_serial, send_buffer, total_len);
    
    // 释放缓冲区
    free(send_buffer);
    
    if (ret < 0) {
        debug_printf("发送GPIO设置输出命令失败: 错误代码=%d", ret);
        return GPIO_ERR_USB_WRITE_FAIL;
    }
    
    debug_printf("成功发送GPIO设置输出命令，GPIO索引: %d, 掩码: 0x%02X", GPIOIndex, OutputMask);
    return GPIO_SUCCESS;
}

/**
 * @brief 写入GPIO输出值
 * 
 * @param target_serial 目标设备的序列号
 * @param GPIOIndex GPIO索引
 * @param WriteValue 写入的值，每个位对应一个引脚
 * @return int 成功返回0，失败返回错误代码
 */
WINAPI int GPIO_Write(const char* target_serial, int GPIOIndex, uint8_t WriteValue) {
    // 验证参数
    debug_printf("GPIO_Write开始执行");
    if (!target_serial) {
        debug_printf("参数无效: target_serial=%p", target_serial);
        return GPIO_ERR_PARAM_INVALID;
    }
    

    
    // 创建通用命令包头
    GENERIC_CMD_HEADER cmd_header;
    cmd_header.protocol_type = PROTOCOL_GPIO;     // GPIO协议
    cmd_header.cmd_id = CMD_WRITE;              // 写数据命令
    cmd_header.device_index = (uint8_t)GPIOIndex; // 设备索引
    cmd_header.param_count = 0;                 // 参数数量：0个
    cmd_header.data_len = 1;                    // 数据部分长度: 1字节数据
    
    // 计算总长度：命令头 + 数据 + 结束符
    int total_len = sizeof(GENERIC_CMD_HEADER) + 1 + sizeof(uint32_t);
    cmd_header.total_packets = total_len;
    
    // 分配发送缓冲区
    unsigned char* send_buffer = (unsigned char*)malloc(total_len);
    if (!send_buffer) {
        debug_printf("内存分配失败");
        return GPIO_ERR_OTHER;
    }
    
    // 复制命令头到缓冲区
    memcpy(send_buffer, &cmd_header, sizeof(GENERIC_CMD_HEADER));
    
    // 复制GPIO数据
    send_buffer[sizeof(GENERIC_CMD_HEADER)] = WriteValue;
    
    // 在数据包末尾添加结束符
    uint32_t end_marker = CMD_END_MARKER;
    memcpy(send_buffer + sizeof(GENERIC_CMD_HEADER) + 1, &end_marker, sizeof(uint32_t));
    
    debug_printf("发送GPIO写命令: protocol=%d, cmd=%d, index=%d, data_len=%d, value=0x%02X",
                 cmd_header.protocol_type, cmd_header.cmd_id, cmd_header.device_index, 
                 cmd_header.data_len, WriteValue);
    
    // 使用USB_WriteData发送数据
    int ret = USB_WriteData(target_serial, send_buffer, total_len);
    
    // 释放缓冲区
    free(send_buffer);
    
    if (ret < 0) {
        debug_printf("发送GPIO写数据命令失败: 错误代码=%d", ret);
        return GPIO_ERR_USB_WRITE_FAIL;
    }
    
    debug_printf("成功发送GPIO写数据命令，GPIO索引: %d, 数据: 0x%02X", GPIOIndex, WriteValue);
    return GPIO_SUCCESS;
}
