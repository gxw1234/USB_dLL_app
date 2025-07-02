#ifndef USB_GPIO_H
#define USB_GPIO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
// #include <windows.h> // 移除windows.h，避免跨平台报错



// GPIO方向定义
#define GPIO_DIR_OUTPUT  0x01    // 输出模式
#define GPIO_DIR_INPUT   0x00    // 输入模式

/**
 * @brief 设置GPIO为输出模式
 * 
 * @param target_serial 目标设备的序列号
 * @param GPIOIndex GPIO索引
 * @param OutputMask 输出引脚掩码，每个位对应一个引脚，1表示设置为输出
 * @return int 成功返回0，失败返回错误代码
 */
int GPIO_SetOutput(const char* target_serial, int GPIOIndex, uint8_t OutputMask);

/**
 * @brief 写入GPIO输出值
 * 
 * @param target_serial 目标设备的序列号
 * @param GPIOIndex GPIO索引
 * @param WriteValue 写入的值，每个位对应一个引脚
 * @return int 成功返回0，失败返回错误代码
 */
int GPIO_Write(const char* target_serial, int GPIOIndex, uint8_t WriteValue);

#ifdef __cplusplus
}
#endif

#endif // USB_GPIO_H
