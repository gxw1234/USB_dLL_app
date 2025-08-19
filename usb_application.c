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


WINAPI int USB_GetDeviceInfo(const char* serial, PDEVICE_INFO dev_info, char* func_str) {
    if (!dev_info) {
        debug_printf("获取设备信息失败: 参数无效");
        return USB_ERROR_INVALID_PARAM;
    }
    debug_printf("获取设备信息: %s", serial ? serial : "默认设备");
    strcpy(dev_info->FirmwareName, "USB_G2X_Firmware");
    strcpy(dev_info->BuildDate, "2025-01-23 10:30:00");
    dev_info->HardwareVersion = 0x0102;  // 硬件版本 1.2
    dev_info->FirmwareVersion = 0x0305;  // 固件版本 3.5
    dev_info->SerialNumber[0] = 0x12345678;
    dev_info->SerialNumber[1] = 0x9ABCDEF0;
    dev_info->SerialNumber[2] = 0x11223344;
    dev_info->Functions = 0x000F;  // 支持GPIO、SPI、I2C、Power等功能
    
    // 如果提供了func_str参数，根据Functions位标志动态生成功能字符串
    if (func_str) {
        char temp_str[256] = {0};
        int first = 1;
        
        if (dev_info->Functions & 0x0001) {  // GPIO功能
            if (!first) strcat(temp_str, ",");
            strcat(temp_str, "GPIO");
            first = 0;
        }
        if (dev_info->Functions & 0x0002) {  // SPI功能
            if (!first) strcat(temp_str, ",");
            strcat(temp_str, "SPI");
            first = 0;
        }
        if (dev_info->Functions & 0x0004) {  // I2C功能
            if (!first) strcat(temp_str, ",");
            strcat(temp_str, "I2C");
            first = 0;
        }
        if (dev_info->Functions & 0x0008) {  // POWER功能
            if (!first) strcat(temp_str, ",");
            strcat(temp_str, "POWER");
            first = 0;
        }
        strcpy(func_str, temp_str);
    }
    
    debug_printf("设备信息获取成功: %s, 硬件版本: 0x%04X, 固件版本: 0x%04X", 
                 dev_info->FirmwareName, dev_info->HardwareVersion, dev_info->FirmwareVersion);
    
    return USB_SUCCESS;
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
