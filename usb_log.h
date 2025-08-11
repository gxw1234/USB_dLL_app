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
#define USB_DEBUG_ENABLE 1

void USB_SetLog(int enable);
void debug_printf(const char* format, ...);

#ifdef __cplusplus
}
#endif

#endif // USB_LOG_H
