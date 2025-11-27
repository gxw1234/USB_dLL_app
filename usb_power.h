#ifndef USB_POWER_H
#define USB_POWER_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
#include <windows.h>
#else
#include "platform_compat.h"
#endif
#include <stdint.h>


#define POWER_SUCCESS           0   // 成功
#define POWER_ERROR_INVALID_PARAM -1  // 无效参数
#define POWER_ERROR_IO          -2  // IO错误
#define POWER_ERROR_OTHER       -3  // 其他错误

#define POWER_CHANNEL_1         0x01  // 电源通道1


typedef struct _VOLTAGE_CONFIG {
    uint8_t channel;     // 电源通道
    uint16_t voltage;   // 电压值（单位：mV）
} VOLTAGE_CONFIG, *PVOLTAGE_CONFIG;


WINAPI int POWER_SetVoltage(const char* target_serial, uint8_t channel, uint16_t voltage_mv);


WINAPI int POWER_StartCurrentReading(const char* target_serial, uint8_t channel);


WINAPI int POWER_StopCurrentReading(const char* target_serial, uint8_t channel);

WINAPI int POWER_ReadCurrentData(const char* target_serial, uint8_t channel, unsigned char* buffer, int buffer_size);


//电源控制 开
WINAPI int POWER_PowerOn(const char* target_serial, uint8_t channel);

//电源控制 关
WINAPI int POWER_PowerOff(const char* target_serial, uint8_t channel);

//启动电源测试模式
WINAPI int POWER_StartTestMode(const char* target_serial, uint8_t channel);

//停止电源测试模式
WINAPI int POWER_StopTestMode(const char* target_serial, uint8_t channel);

#ifdef __cplusplus
}
#endif

#endif // USB_POWER_H
