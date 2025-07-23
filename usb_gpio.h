#ifndef USB_GPIO_H
#define USB_GPIO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define GPIO_DIR_OUTPUT  0x01    // 输出模式
#define GPIO_DIR_OUTPUT_OD  0x02    // 输出开漏
#define GPIO_DIR_INPUT   0x00    // 输入模式
#define GPIO_DIR_WRITE   0x03    // 写入

//设置为输出模式
 //pull_mode 上拉下拉模式：0=无上拉下拉，1=上拉，2=下拉
int GPIO_SetOutput(const char* target_serial, int GPIOIndex, uint8_t pull_mode);

//设置为开漏模式
 //pull_mode 上拉下拉模式：0=无上拉下拉，1=上拉，2=下拉
int GPIO_SetOpenDrain(const char* target_serial, int GPIOIndex, uint8_t pull_mode);

//设置为输入模式
//pull_mode 上拉下拉模式：0=无上拉下拉，1=上拉，2=下拉
int GPIO_SetInput(const char* target_serial, int GPIOIndex, uint8_t pull_mode);


int GPIO_Write(const char* target_serial, int GPIOIndex, uint8_t WriteValue);

//复位STM32
int STM32_reset(const char* target_serial);


#ifdef __cplusplus
}
#endif

#endif // USB_GPIO_H
