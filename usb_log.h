/**
 * @file usb_log.h
 * @brief USB日志系统接口
 */

#ifndef USB_LOG_H
#define USB_LOG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <windows.h>

// 调试开关，0=开启日志，1=关闭日志
#define USB_DEBUG_ENABLE 0

/**
 * @brief 设置日志状态
 * 
 * @param enable 1=开启日志，0=关闭日志
 */
void USB_SetLog(int enable);

/**
 * @brief 调试输出函数
 * 
 * @param format 格式化字符串
 * @param ... 可变参数
 */
void debug_printf(const char* format, ...);

#ifdef __cplusplus
}
#endif

#endif // USB_LOG_H
