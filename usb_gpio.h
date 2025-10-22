#ifndef USB_GPIO_H
#define USB_GPIO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>



//设置为输出模式
 //pull_mode 上拉下拉模式：0=无上拉下拉，1=上拉，2=下拉
int GPIO_SetOutput(const char* target_serial, int GPIOIndex, uint8_t pull_mode);

//设置为开漏模式
 //pull_mode 上拉下拉模式：0=无上拉下拉，1=上拉，2=下拉
int GPIO_SetOpenDrain(const char* target_serial, int GPIOIndex, uint8_t pull_mode);

//设置为输入模式
//pull_mode 上拉下拉模式：0=无上拉下拉，1=上拉，2=下拉
int GPIO_SetInput(const char* target_serial, int GPIOIndex, uint8_t pull_mode);

//普通写入GPIO
int GPIO_Write(const char* target_serial, int GPIOIndex, uint8_t WriteValue);

//读取GPIO电平（0/1）
int GPIO_Read(const char* target_serial, int GPIOIndex, uint8_t* level);

//扫描写入GPIO，需要等待IIC响应，才返回
int GPIO_scan_Write(const char* target_serial, int GPIOIndex, uint8_t WriteValue);

//复位STM32
int USB_device_reset(const char* target_serial);


#ifdef __cplusplus
}
#endif

#endif // USB_GPIO_H
