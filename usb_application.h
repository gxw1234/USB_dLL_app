#ifndef USB_APPLICATION_H
#define USB_APPLICATION_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "usb_middleware.h"

#define USB_SUCCESS             0    // 成功
#define USB_ERROR_NOT_FOUND    -1    // 设备未找到
#define USB_ERROR_ACCESS       -2    // 访问被拒绝
#define USB_ERROR_IO           -3    // I/O错误
#define USB_ERROR_INVALID_PARAM -4   // 参数无效
#define USB_ERROR_ALREADY_OPEN -5    // 设备已打开
#define USB_ERROR_NOT_OPEN     -6    // 设备未打开
#define USB_ERROR_TIMEOUT      -7    // 超时错误
#define USB_ERROR_OTHER        -99   // 其他错误

// Windows API导出宏
#ifdef _WIN32
    #ifdef BUILDING_DLL
        #define WINAPI __declspec(dllexport)
    #else
        #define WINAPI __declspec(dllimport)
    #endif
#else
    #define WINAPI
#endif

// ==================== 设备管理接口 ====================


WINAPI int USB_Init(void);
WINAPI void USB_Exit(void);
WINAPI int USB_ScanDevices(device_info_t* devices, int max_devices);
WINAPI int USB_OpenDevice(const char* serial);
WINAPI int USB_CloseDevice(const char* serial);

WINAPI int USB_FindDeviceBySerial(const char* serial);

WINAPI int USB_IsDeviceOpen(int device_id);

WINAPI int USB_GetDeviceCount(void);


WINAPI void USB_SetLogging(int enable);

#ifdef __cplusplus
}
#endif

#endif // USB_APPLICATION_H
