#ifndef USB_APPLICATION_H
#define USB_APPLICATION_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "usb_middleware.h"

// 错误代码定义
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

/**
 * @brief 初始化USB应用层
 * @return int 成功返回0，失败返回错误代码
 */
WINAPI int USB_Init(void);

/**
 * @brief 清理USB应用层
 */
WINAPI void USB_Exit(void);

/**
 * @brief 扫描USB设备
 * @param devices 设备信息数组
 * @param max_devices 最大设备数
 * @return int 扫描到的设备数量
 */
WINAPI int USB_ScanDevices(device_info_t* devices, int max_devices);

/**
 * @brief 打开USB设备
 * @param serial 设备序列号，NULL表示打开第一个可用设备
 * @return int 成功返回设备ID(≥0)，失败返回错误代码
 */
WINAPI int USB_OpenDevice(const char* serial);

/**
 * @brief 关闭USB设备
 * @param serial 设备序列号
 * @return int 成功返回0，失败返回错误代码
 */
WINAPI int USB_CloseDevice(const char* serial);

/**
 * @brief 根据序列号查找设备ID
 * @param serial 设备序列号
 * @return int 设备ID，未找到返回-1
 */
WINAPI int USB_FindDeviceBySerial(const char* serial);

/**
 * @brief 检查设备是否已打开
 * @param device_id 设备ID
 * @return int 1=已打开，0=未打开，<0=错误
 */
WINAPI int USB_IsDeviceOpen(int device_id);

/**
 * @brief 获取设备数量
 * @return int 当前管理的设备数量
 */
WINAPI int USB_GetDeviceCount(void);

/**
 * @brief 设置USB调试日志状态
 * @param enable 1=启用日志，0=禁用日志
 */
WINAPI void USB_SetLogging(int enable);

#ifdef __cplusplus
}
#endif

#endif // USB_APPLICATION_H
