#ifndef USB_PROTOCOL_H
#define USB_PROTOCOL_H

// 协议类型定义（和与STM32端保持一致）
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


//----GPIO------------
#define GPIO_DIR_OUTPUT  0x01    // 输出模式
#define GPIO_DIR_OUTPUT_OD  0x02    // 输出开漏
#define GPIO_DIR_INPUT   0x00    // 输入模式
#define GPIO_DIR_WRITE   0x03    // 写入

#endif // USB_PROTOCOL_H 