#ifndef __USB_IIC_H__
#define __USB_IIC_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// 错误码定义
#define IIC_SUCCESS             0   // 操作成功
#define IIC_ERROR_INVALID_PARAM -1  // 参数无效
#define IIC_ERROR_IO            -2  // IO错误
#define IIC_ERROR_TIMEOUT       -3  // 操作超时
#define IIC_ERROR_OTHER         -4  // 其他错误

// IIC配置结构体
typedef struct _IIC_CONFIG {
    unsigned int    ClockSpeedHz;   // IIC时钟频率:单位为Hz
    unsigned short  OwnAddr;        // USB2XXX为从机时自己的地址
    unsigned char   Master;         // 主从选择控制:0-从机，1-主机
    unsigned char   AddrBits;       // 从机地址模式，7-7bit模式，10-10bit模式
    unsigned char   EnablePu;       // 使能引脚芯片内部上拉电阻，若不使能，则I2C总线上必须接上拉电阻
} IIC_CONFIG, *PIIC_CONFIG;

// IIC索引定义
#define IIC1_INDEX   0   // IIC1
#define IIC2_INDEX   1   // IIC2

// IIC通信API
#ifdef _WIN32
#define WINAPI __stdcall
#else
#define WINAPI
#endif

// IIC初始化
WINAPI int IIC_Init(const char* target_serial, int IICIndex, PIIC_CONFIG pConfig);

// IIC从机写数据
WINAPI int IIC_SlaveWriteBytes(const char* target_serial, int IICIndex, unsigned char *pWriteData, int WriteLen, int TimeOutMs);


#ifdef __cplusplus
}
#endif

#endif // __USB_IIC_H__
