#ifndef USB_POWER_H
#define USB_POWER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <windows.h>
#include <stdint.h>


#define POWER_SUCCESS           0   // 成功
#define POWER_ERROR_INVALID_PARAM -1  // 无效参数
#define POWER_ERROR_IO          -2  // IO错误
#define POWER_ERROR_OTHER       -3  // 其他错误


#define POWER_CMD_SET_VOLTAGE   0x01  // 设置电压命令
#define POWER_CMD_START_CURRENT_READING 0x02  // 开始读取电流命令
#define POWER_CMD_STOP_CURRENT_READING  0x03  // 停止读取电流命令
#define POWER_CMD_READ_CURRENT_DATA     0x04  // 读取电流数据命令


#define POWER_CHANNEL_1         0x01  // 电源通道1
#define POWER_CHANNEL_UA        0x02  // 微安电流通道
#define POWER_CHANNEL_MA        0x03  // 毫安电流通道


typedef struct _VOLTAGE_CONFIG {
    uint8_t channel;     // 电源通道
    uint16_t voltage;   // 电压值（单位：mV）
} VOLTAGE_CONFIG, *PVOLTAGE_CONFIG;


WINAPI int POWER_SetVoltage(const char* target_serial, uint8_t channel, uint16_t voltage_mv);


WINAPI int POWER_StartCurrentReading(const char* target_serial, uint8_t channel);


WINAPI int POWER_StopCurrentReading(const char* target_serial, uint8_t channel);


WINAPI int POWER_ReadCurrentData(const char* target_serial, uint8_t channel, float* current_value);

#ifdef __cplusplus
}
#endif

#endif // USB_POWER_H
