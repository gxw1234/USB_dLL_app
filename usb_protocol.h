#ifndef USB_PROTOCOL_H
#define USB_PROTOCOL_H

#include <stddef.h>
#include <stdint.h>

// 协议类型定义  protocol_type
#define PROTOCOL_SPI        0x01    
#define PROTOCOL_IIC        0x02    
#define PROTOCOL_UART       0x03    
#define PROTOCOL_GPIO       0x04    
#define PROTOCOL_POWER      0x05    
#define PROTOCOL_RESETSTM32      0x06    
#define PROTOCOL_BOOTLOADER_WRITE_BYTES    0x07    
#define PROTOCOL_GET_FIRMWARE_INFO    0x08    
#define PROTOCOL_STATUS    0x09    // 专用状态响应协议
#define PROTOCOL_AUDIO      0x0A    // 音频协议
#define PROTOCOL_CURRENT    0x0B    // 电流数据协议
#define PROTOCOL_PWM        0x0C    // PWM协议


//------------cmd_id以下都是命令ID-------------------------

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
#define FRAME_START_MARKER  0x5A5A5A5A

//----GPIO------------
#define GPIO_DIR_OUTPUT  0x01    // 输出模式
#define GPIO_DIR_OUTPUT_OD  0x02    // 输出开漏
#define GPIO_DIR_INPUT   0x00    // 输入模式
#define GPIO_DIR_WRITE   0x03    // 写入
#define GPIO_SCAN_DIR_WRITE   0x04    // 扫描写入
#define GPIO_DIR_READ   0x06    // 读取

//----Bootloader------------
#define BOOTLOADER_START_WRITE    0x04    // 开始写数据命令
#define BOOTLOADER_WRITE_BYTES    0x05    // 写数据命令
#define BOOTLOADER_SWITCH_RUN   0x06    // 切换到RUN模式
#define BOOTLOADER_SWITCH_BOOT   0x07    // 切换到BOOT模式
#define BOOTLOADER_RESET   0x08    // 复位


//----IIC------------
#define GPIO_SCAN_MODE_WRITE   0x04    // IIC


//----Power------------
#define POWER_CMD_SET_VOLTAGE   0x01  // 设置电压命令
#define POWER_CMD_START_READING 0x02  // 开始读取电流命令
#define POWER_CMD_STOP_READING  0x03  // 停止读取电流命令
#define POWER_CMD_READ_CURRENT_DATA     0x04  // 读取电流数据命令
#define POWER_CMD_POWER_ON      0x04  // 电源上电命令
#define POWER_CMD_POWER_OFF     0x05  // 电源断电命令
#define POWER_CMD_START_TEST_MODE     0x06  // 电源状态命令
#define POWER_CMD_STOP_TEST_MODE     0x07  // 电源状态命令


//----Audio I2S------------
#define AUDIO_CMD_INIT      0x01  // 初始化命令
#define AUDIO_CMD_PLAY      0x02  // 播放命令
#define AUDIO_CMD_STOP      0x03  // 停止命令
#define AUDIO_CMD_START     0x04  // 启动队列命令
#define AUDIO_CMD_STATUS    0x05  // 状态查询命令
#define AUDIO_CMD_VOLUME    0x06  // 音量控制命令

//----Current Data------------
#define CURRENT_CMD_DATA   0x01  // 电流数据命令

//----PWM------------
#define PWM_CMD_INIT            0x01  // PWM输入捕获初始化命令
#define PWM_CMD_START_MEASURE   0x02  // 开始PWM测量命令
#define PWM_CMD_STOP_MEASURE    0x03  // 停止PWM测量命令
#define PWM_CMD_GET_RESULT      0x04  // 获取PWM测量结果命令

//ST发送PC接收
// 协议类型定义  protocol_type



//CMD_ID
#define GET_STATUS 01





typedef struct _GENERIC_CMD_HEADER {
    uint8_t protocol_type;  
    uint8_t cmd_id;         
    uint8_t device_index;   // 设备索引
    uint8_t param_count;    // 参数数量
    uint16_t data_len;      // 数据部分长度
    uint16_t total_packets; // 整包总数
  } GENERIC_CMD_HEADER, *PGENERIC_CMD_HEADER;
  

  typedef struct _PARAM_HEADER {
    uint16_t param_len;     
  } PARAM_HEADER, *PPARAM_HEADER;
  
// 组包
int build_protocol_frame(unsigned char** buffer, GENERIC_CMD_HEADER* cmd_header, 
                        void* param_data, size_t param_len, 
                        void* data_payload, size_t data_len);




#endif // USB_PROTOCOL_H