#ifndef USB_PROTOCOL_H
#define USB_PROTOCOL_H

// 协议类型定义（和与STM32端保持一致）
#define PROTOCOL_SPI        0x01    // SPI协议
#define PROTOCOL_IIC        0x02    // IIC协议
#define PROTOCOL_UART       0x03    // UART协议
#define PROTOCOL_GPIO       0x04    // GPIO协议
#define PROTOCOL_RESETSTM32      0x06    // 复位STM32


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

//----帧头和结束符------------
#define FRAME_START_MARKER  0x5AA55AA5 // 帧开始标记
#define CMD_END_MARKER      0xA5A5A5A5 // 命令包结束符

// 通用命令包头结构 (包含帧头标记)
typedef struct _GENERIC_CMD_HEADER {
    uint32_t start_marker;  // 帧开始标记 0x5AA55AA5
    uint8_t protocol_type;  // 协议类型：SPI/IIC/UART等
    uint8_t cmd_id;         // 命令ID：初始化/读/写等
    uint8_t device_index;   // 设备索引
    uint8_t param_count;    // 参数数量
    uint16_t data_len;      // 数据部分长度
    uint16_t total_packets; // 整包总数(包括帧头和结束符)
} __attribute__((packed)) GENERIC_CMD_HEADER, *PGENERIC_CMD_HEADER;

// 参数头结构
typedef struct _PARAM_HEADER {
    uint16_t param_len;     // 参数长度
} PARAM_HEADER, *PPARAM_HEADER;

// 帧头结构
typedef struct _FRAME_HEADER {
    uint32_t start_marker;    // 帧开始标记 0x5AA55AA5
    uint16_t frame_length;    // 整个帧的长度(包括帧头和结束符)
    uint16_t reserved;        // 保留字段
} FRAME_HEADER, *PFRAME_HEADER;


//----GPIO------------
#define GPIO_DIR_OUTPUT  0x01    // 输出模式
#define GPIO_DIR_OUTPUT_OD  0x02    // 输出开漏
#define GPIO_DIR_INPUT   0x00    // 输入模式
#define GPIO_DIR_WRITE   0x03    // 写入
#define GPIO_SCAN_DIR_WRITE   0x04    // 扫描写入

#endif // USB_PROTOCOL_H 