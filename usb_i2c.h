#ifndef USB_I2C_H
#define USB_I2C_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

// 跨平台宏定义
#ifndef WINAPI
#ifdef _WIN32
#define WINAPI __stdcall
#else
#define WINAPI
#endif
#endif

// I2C配置结构体
typedef struct {
    uint32_t ClockSpeedHz;      // IIC时钟频率:单位为Hz
    uint16_t OwnAddr;           // USB2XXX为从机时自己的地址
    uint8_t Master;             // 主从选择控制:0-从机，1-主机
    uint8_t AddrBits;           // 从机地址模式，7-7bit模式，10-10bit模式
    uint8_t EnablePu;           // 使能引脚芯片内部上拉电阻，若不使能，则I2C总线上必须接上拉电阻
} IIC_CONFIG, *PIIC_CONFIG;

// I2C写入数据结构体
typedef struct {
    uint16_t DeviceAddress;     // 设备地址
    uint16_t MemAddress;        // 内存地址
    uint8_t MemAddSize;         // 内存地址大小: 1=8位, 2=16位
    uint16_t DataLength;        // 数据长度
    uint32_t TimeoutMs;         // 超时时间(毫秒)
    uint8_t Data[];             // 数据内容
} I2C_WRITE_DATA;

// I2C初始化
// @param target_serial 设备序列号
// @param i2c_index I2C索引 (1对应I2C4)
// @param config I2C配置参数
// @return 成功返回USB_SUCCESS，失败返回错误码
WINAPI int IIC_Init(const char* target_serial, int i2c_index, PIIC_CONFIG config);

// I2C写入数据
// @param target_serial 设备序列号
// @param i2c_index I2C索引
// @param device_addr 设备地址
// @param write_buffer 要写入的数据
// @param write_len 数据长度
// @param timeout_ms 超时时间(毫秒)
// @return 成功返回USB_SUCCESS，失败返回错误码
WINAPI int IIC_WriteBytes(const char* target_serial, int i2c_index, 
                         uint16_t device_addr, const uint8_t* write_buffer, 
                         uint16_t write_len, uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif

#endif // USB_I2C_H