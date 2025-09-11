#ifndef USB_I2S_H
#define USB_I2S_H

#ifdef __cplusplus
extern "C" {
#endif

#include "usb_application.h"


#define I2S_SUCCESS             0    // 成功
#define I2S_ERROR_NOT_FOUND    -1    // 设备未找到
#define I2S_ERROR_ACCESS       -2    // 访问被拒绝
#define I2S_ERROR_IO           -3    // I/O错误
#define I2S_ERROR_INVALID_PARAM -4   // 参数无效
#define I2S_ERROR_OTHER        -99   // 其他错误


typedef struct _I2S_CONFIG {
    char   Mode;            // I2S模式:0-主机发送，1-主机接收，2-从机发送，3-从机接收
    char   Standard;        // I2S标准:0-飞利浦标准，1-MSB对齐，2-LSB对齐，3-PCM短帧，4-PCM长帧
    char   DataFormat;      // 数据格式:0-16位，1-24位，2-32位
    char   MCLKOutput;      // MCLK输出:0-禁用，1-启用
    unsigned int AudioFreq; // 音频频率:8000,16000,22050,44100,48000,96000,192000
} I2S_CONFIG, *PI2S_CONFIG;


WINAPI int I2S_Init(const char* target_serial, int I2SIndex, PI2S_CONFIG pConfig);

WINAPI int I2S_Queue_WriteBytes(const char* target_serial, int I2SIndex, unsigned char* pWriteBuffer, int WriteLen);

WINAPI int I2S_GetQueueStatus(const char* target_serial, int I2SIndex);

WINAPI int I2S_StartQueue(const char* target_serial, int I2SIndex);

WINAPI int I2S_StopQueue(const char* target_serial, int I2SIndex);

WINAPI int I2S_SetVolume(const char* target_serial, int I2SIndex, unsigned char volume);

#ifdef __cplusplus
}
#endif

#endif // USB_I2S_H