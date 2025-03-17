/**
 * @file usb_application.c
 * @brief USB应用层公共接口实现
 */

#include "usb_application.h"
#include "usb_middleware.h"
#include "usb_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <stdint.h>
#include <process.h>  // 用于线程函数

// 调试输出函数
extern void debug_printf(const char *format, ...);

#define VENDOR_ID   0xCCDD          // 设备VID
#define PRODUCT_ID  0xAABB          // 设备PID

// 打开的设备信息
#define MAX_OPEN_DEVICES 16
typedef struct {
    char serial[64];
    int device_id;
    int is_open;
} open_device_t;

static open_device_t open_devices[MAX_OPEN_DEVICES] = {0};

// 查找空闲设备槽
static int find_free_device_slot(void) {
    for (int i = 0; i < MAX_OPEN_DEVICES; i++) {
        if (!open_devices[i].is_open) {
            return i;
        }
    }
    return -1;
}

static int find_device_by_serial(const char* serial) {
    if (!serial) {
        return -1;
    }
    
    for (int i = 0; i < MAX_OPEN_DEVICES; i++) {
        if (open_devices[i].is_open && strcmp(open_devices[i].serial, serial) == 0) {
            return i;
        }
    }
    return -1;
}

// ==================== 设备管理接口 ====================


/**
 * @brief 初始化USB应用层
 * @return int 成功返回0，失败返回错误代码
 */
WINAPI int USB_Init(void) {
    debug_printf("初始化USB应用层...");
    int ret = usb_middleware_init();
    if (ret == 0) {
        debug_printf("USB应用层初始化成功");
    } else {
        debug_printf("USB应用层初始化失败: %d", ret);
    }
    return ret;
}


WINAPI void USB_Exit(void) {
    debug_printf("清理USB应用层...");
    usb_middleware_cleanup();
    debug_printf("USB应用层清理完成");
}


WINAPI int USB_ScanDevices(device_info_t* devices, int max_devices) {
    if (!devices || max_devices <= 0) {
        debug_printf("参数无效: devices=%p, max_devices=%d", devices, max_devices);
        return 0;
    }
    
    int count = usb_middleware_scan_devices(devices, max_devices);
    debug_printf("扫描到 %d 个USB设备", count);
    return count;
}


WINAPI int USB_OpenDevice(const char* serial) {
    if (!serial) {
        debug_printf("打开第一个可用设备");
    } else {
        debug_printf("打开设备: %s", serial);
    }
    
    int device_id = usb_middleware_open_device(serial);
    if (device_id >= 0) {
        debug_printf("成功打开设备，ID: %d", device_id);
    } else {
        debug_printf("打开设备失败: %d", device_id);
    }
    return device_id;
}


WINAPI int USB_CloseDevice(const char* serial) {
    if (!serial) {
        debug_printf("关闭设备失败: 序列号参数为空");
        return USB_ERROR_INVALID_PARAM;
    }
    debug_printf("关闭设备: %s", serial);
    int device_id = usb_middleware_find_device_by_serial(serial);
    if (device_id < 0) {
        debug_printf("关闭设备失败: 未找到设备 %s", serial);
        return USB_ERROR_NOT_FOUND;
    }
    int ret = usb_middleware_close_device(device_id);
    if (ret == 0) {
        debug_printf("成功关闭设备: %s", serial);
    } else {
        debug_printf("关闭设备失败: %s, 错误: %d", serial, ret);
    }
    return ret;
}


WINAPI int USB_FindDeviceBySerial(const char* serial) {
    if (!serial) {
        debug_printf("序列号参数为空");
        return -1;
    }
    
    int device_id = usb_middleware_find_device_by_serial(serial);
    if (device_id >= 0) {
        debug_printf("找到设备: %s, ID: %d", serial, device_id);
    } else {
        debug_printf("未找到设备: %s", serial);
    }
    return device_id;
}


WINAPI int USB_IsDeviceOpen(int device_id) {
    int is_open = usb_middleware_is_device_open(device_id);
    debug_printf("设备 %d 状态: %s", device_id, is_open ? "已打开" : "未打开");
    return is_open;
}


WINAPI int USB_GetDeviceCount(void) {
    int count = usb_middleware_get_device_count();
    debug_printf("当前设备数量: %d", count);
    return count;
}


WINAPI void USB_SetLogging(int enable) {
    debug_printf("设置USB调试日志: %s", enable ? "启用" : "禁用");
    USB_SetLog(enable);
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
    switch (fdwReason) {
        case DLL_PROCESS_ATTACH:
            // 初始化中间层
            usb_middleware_init();
            break;
        case DLL_PROCESS_DETACH:
            // 清理中间层
            usb_middleware_cleanup();
            break;
    }
    return TRUE;
}
