#ifndef USB_PROTOCOL_H
#define USB_PROTOCOL_H

// 协议类型定义（和与STM32端保持一致）
#define PROTOCOL_SPI        0x01    // SPI协议
#define PROTOCOL_IIC        0x02    // IIC协议
#define PROTOCOL_UART       0x03    // UART协议
#define PROTOCOL_GPIO       0x04    // GPIO协议
#define PROTOCOL_RESETSTM32      0x06    // 复位STM32
#define PROTOCOL_BOOTLOADER_WRITE_BYTES    0x07    // 写数据命令

// 通用命令ID定义
#define CMD_INIT            0x01    // 初始化命令
#define CMD_WRITE           0x02    // 写数据命令
#define CMD_READ            0x03    // 读数据命令
#define CMD_TRANSFER        0x04    // 读写数据命令



//----SPI------------
#define CMD_QUEUE_STATUS    0x05    // 队列状态查询命令
#define CMD_QUEUE_START    0x06    // 队列状态查询命令    
#define CMD_QUEUE_STOP    0x07    // 队列状态查询命令
#define CMD_QUEUE_WRITE           0x08    // 写数据命令

//----结束符------------
#define CMD_END_MARKER      0xA5A5A5A5 // 命令包结束符


//----GPIO------------
#define GPIO_DIR_OUTPUT  0x01    // 输出模式
#define GPIO_DIR_OUTPUT_OD  0x02    // 输出开漏
#define GPIO_DIR_INPUT   0x00    // 输入模式
#define GPIO_DIR_WRITE   0x03    // 写入
#define GPIO_SCAN_DIR_WRITE   0x04    // 扫描写入

//----Bootloader------------
#define BOOTLOADER_WRITE_BYTES    0x05    // 写数据命令
#define BOOTLOADER_SWITCH_RUN   0x06    // 切换到RUN模式
#define BOOTLOADER_SWITCH_BOOT   0x07    // 切换到BOOT模式


#endif // USB_PROTOCOL_H 