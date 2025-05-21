#ifndef USB_GPIO_H
#define USB_GPIO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <windows.h>

// GPIO索引定义
#define GPIO_PORT0       0x00    // GPIO端口0
#define GPIO_PORT1       0x01    // GPIO端口1
#define GPIO_PORT2       0x02    // GPIO端口2

// GPIO方向定义
#define GPIO_DIR_OUTPUT  0x01    // 输出模式
#define GPIO_DIR_INPUT   0x00    // 输入模式

// 命令定义
#define CMD_GPIO_SET_DIR 0x70    // 设置GPIO方向命令
#define CMD_GPIO_WRITE   0x71    // 写GPIO命令

// 错误码定义
#define GPIO_SUCCESS             0       // 成功
#define GPIO_ERR_NOT_SUPPORT     -1      // 不支持该功能
#define GPIO_ERR_USB_WRITE_FAIL  -2      // USB写入失败
#define GPIO_ERR_USB_READ_FAIL   -3      // USB读取失败
#define GPIO_ERR_PARAM_INVALID   -4      // 参数无效
#define GPIO_ERR_OTHER           -5      // 其他错误

/**
 * @brief 设置GPIO为输出模式
 * 
 * @param target_serial 目标设备的序列号
 * @param GPIOIndex GPIO索引
 * @param OutputMask 输出引脚掩码，每个位对应一个引脚，1表示设置为输出
 * @return int 成功返回0，失败返回错误代码
 */
WINAPI int GPIO_SetOutput(const char* target_serial, int GPIOIndex, uint8_t OutputMask);

/**
 * @brief 写入GPIO输出值
 * 
 * @param target_serial 目标设备的序列号
 * @param GPIOIndex GPIO索引
 * @param WriteValue 写入的值，每个位对应一个引脚
 * @return int 成功返回0，失败返回错误代码
 */
WINAPI int GPIO_Write(const char* target_serial, int GPIOIndex, uint8_t WriteValue);

#ifdef __cplusplus
}
#endif

#endif // USB_GPIO_H
