/**
 * @file usb_device.c
 * @brief USB设备底层操作实现
 */

#include "usb_device.h"
#include "usb_log.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>

// 全局变量
static HMODULE hLibusbDll = NULL;
void* g_libusb_context = NULL;
int g_is_initialized = 0;

// libusb函数指针类型定义
typedef int (*libusb_init_t)(void**);
typedef void (*libusb_exit_t)(void*);
typedef void* (*libusb_open_device_with_vid_pid_t)(void*, unsigned short, unsigned short);
typedef int (*libusb_claim_interface_t)(void*, int);
typedef int (*libusb_bulk_transfer_t)(void*, unsigned char, unsigned char*, int, int*, unsigned int);
typedef void (*libusb_close_t)(void*);
typedef int (*libusb_release_interface_t)(void*, int);
typedef int (*libusb_get_device_list_t)(void*, void***);
typedef void (*libusb_free_device_list_t)(void**, int);
typedef int (*libusb_get_device_descriptor_t)(void*, void*);
typedef int (*libusb_get_string_descriptor_ascii_t)(void*, unsigned char, unsigned char*, int);
typedef void* (*libusb_get_device_t)(void*);
typedef int (*libusb_open_t)(void*, void**);

// 函数指针
static libusb_init_t p_libusb_init;
static libusb_exit_t p_libusb_exit;
static libusb_open_device_with_vid_pid_t p_libusb_open_device_with_vid_pid;
static libusb_claim_interface_t p_libusb_claim_interface;
static libusb_bulk_transfer_t p_libusb_bulk_transfer;
static libusb_close_t p_libusb_close;
static libusb_release_interface_t p_libusb_release_interface;
static libusb_get_device_list_t p_libusb_get_device_list;
static libusb_free_device_list_t p_libusb_free_device_list;
static libusb_get_device_descriptor_t p_libusb_get_device_descriptor;
static libusb_get_string_descriptor_ascii_t p_libusb_get_string_descriptor_ascii;
static libusb_get_device_t p_libusb_get_device;
static libusb_open_t p_libusb_open;

// 初始化USB设备层
int usb_device_init(void) {
    debug_printf("正在初始化USB设备层...");
    
    if (g_is_initialized) {
        debug_printf("USB设备层已初始化");
        return USB_SUCCESS;
    }

    // 加载libusb DLL
    debug_printf("尝试加载libusb-1.0.dll");
    hLibusbDll = LoadLibrary("libusb-1.0.dll");
    if (!hLibusbDll) {
        debug_printf("加载libusb-1.0.dll失败, 错误码: %d", GetLastError());
        return USB_ERROR_OTHER;
    }
    debug_printf("成功加载libusb-1.0.dll");

    // 获取函数指针
    p_libusb_init = (libusb_init_t)GetProcAddress(hLibusbDll, "libusb_init");
    p_libusb_exit = (libusb_exit_t)GetProcAddress(hLibusbDll, "libusb_exit");
    p_libusb_open_device_with_vid_pid = (libusb_open_device_with_vid_pid_t)GetProcAddress(hLibusbDll, "libusb_open_device_with_vid_pid");
    p_libusb_close = (libusb_close_t)GetProcAddress(hLibusbDll, "libusb_close");
    p_libusb_claim_interface = (libusb_claim_interface_t)GetProcAddress(hLibusbDll, "libusb_claim_interface");
    p_libusb_release_interface = (libusb_release_interface_t)GetProcAddress(hLibusbDll, "libusb_release_interface");
    p_libusb_bulk_transfer = (libusb_bulk_transfer_t)GetProcAddress(hLibusbDll, "libusb_bulk_transfer");
    p_libusb_get_device_list = (libusb_get_device_list_t)GetProcAddress(hLibusbDll, "libusb_get_device_list");
    p_libusb_free_device_list = (libusb_free_device_list_t)GetProcAddress(hLibusbDll, "libusb_free_device_list");
    p_libusb_get_device_descriptor = (libusb_get_device_descriptor_t)GetProcAddress(hLibusbDll, "libusb_get_device_descriptor");
    p_libusb_get_string_descriptor_ascii = (libusb_get_string_descriptor_ascii_t)GetProcAddress(hLibusbDll, "libusb_get_string_descriptor_ascii");
    p_libusb_get_device = (libusb_get_device_t)GetProcAddress(hLibusbDll, "libusb_get_device");
    p_libusb_open = (libusb_open_t)GetProcAddress(hLibusbDll, "libusb_open");
    
    if (!p_libusb_init || !p_libusb_exit || !p_libusb_open_device_with_vid_pid || !p_libusb_close ||
        !p_libusb_claim_interface || !p_libusb_bulk_transfer || !p_libusb_release_interface ||
        !p_libusb_get_device_list || !p_libusb_free_device_list || !p_libusb_get_device_descriptor ||
        !p_libusb_get_string_descriptor_ascii || !p_libusb_get_device || !p_libusb_open) {
        debug_printf("获取函数指针失败");
        FreeLibrary(hLibusbDll);
        hLibusbDll = NULL;
        return USB_ERROR_OTHER;
    }
    debug_printf("成功获取所有函数指针");

    // 初始化libusb
    debug_printf("正在初始化libusb...");
    int ret = p_libusb_init(&g_libusb_context);
    if (ret != LIBUSB_SUCCESS) {
        debug_printf("初始化libusb失败, 错误码: %d", ret);
        FreeLibrary(hLibusbDll);
        hLibusbDll = NULL;
        return USB_ERROR_OTHER;
    }
    debug_printf("成功初始化libusb");

    g_is_initialized = 1;
    return USB_SUCCESS;
}

// 清理USB设备层
void usb_device_cleanup(void) {
    if (!g_is_initialized) {
        return;
    }

    // 退出libusb
    if (g_libusb_context) {
        p_libusb_exit(g_libusb_context);
        g_libusb_context = NULL;
    }

    // 卸载DLL
    if (hLibusbDll) {
        FreeLibrary(hLibusbDll);
        hLibusbDll = NULL;
    }

    g_is_initialized = 0;
}

// 获取设备列表
int usb_device_get_device_list(void* ctx, void*** device_list) {
    if (!g_is_initialized || !device_list) {
        debug_printf("获取设备列表失败: 未初始化或参数无效");
        return -1;
    }
    int count = p_libusb_get_device_list(ctx ? ctx : g_libusb_context, device_list);
    debug_printf("获取设备列表: 找到 %d 个设备", count);
    return count;
}

// 释放设备列表
void usb_device_free_device_list(void** list, int unref_devices) {
    if (g_is_initialized && list) {
        p_libusb_free_device_list(list, unref_devices);
    }
}

// 获取设备描述符
int usb_device_get_device_descriptor(void* dev, usb_device_descriptor* desc) {
    if (!g_is_initialized || !dev || !desc) {
        debug_printf("获取设备描述符失败: 未初始化或参数无效");
        return -1;
    }
    int ret = p_libusb_get_device_descriptor(dev, desc);
    if (ret == 0 && desc) {
        debug_printf("获取设备描述符: VID=0x%04X, PID=0x%04X", desc->idVendor, desc->idProduct);
    } else {
        debug_printf("获取设备描述符失败, 错误码: %d", ret);
    }
    return ret;
}

// 打开设备
int usb_device_open(void* dev, void** handle) {
    if (!g_is_initialized || !dev || !handle) {
        debug_printf("打开设备失败: 未初始化或参数无效");
        return -1;
    }
    int ret = p_libusb_open(dev, handle);
    if (ret != 0) {
        debug_printf("打开设备失败, 错误码: %d", ret);
    }
    return ret;
}

// 关闭设备
void usb_device_close(void* handle) {
    if (g_is_initialized && handle) {
        p_libusb_close(handle);
    }
}

// 获取字符串描述符
int usb_device_get_string_descriptor_ascii(void* handle, uint8_t desc_index, unsigned char* data, int length) {
    if (!g_is_initialized || !handle || !data || length <= 0) {
        debug_printf("获取字符串描述符失败: 未初始化或参数无效");
        return -1;
    }
    int ret = p_libusb_get_string_descriptor_ascii(handle, desc_index, data, length);
    if (ret < 0) {
        debug_printf("获取字符串描述符失败, 错误码: %d", ret);
    }
    return ret;
}

// 申请接口
int usb_device_claim_interface(void* handle, int interface_number) {
    if (!g_is_initialized || !handle) {
        return -1;
    }
    int ret = p_libusb_claim_interface(handle, interface_number);
    return ret;
}

// 释放接口
int usb_device_release_interface(void* handle, int interface_number) {
    if (!g_is_initialized || !handle) {
        return -1;
    }
    int ret = p_libusb_release_interface(handle, interface_number);
    return ret;
}

// 批量传输
int usb_device_bulk_transfer(void* handle, unsigned char endpoint, unsigned char* data, int length, int* transferred, unsigned int timeout) {
    if (!g_is_initialized || !handle || !data || length <= 0 || !transferred) {
        return -1;
    }
    int ret = p_libusb_bulk_transfer(handle, endpoint, data, length, transferred, timeout);
    return ret;
}

// 获取设备
void* usb_device_get_device(void* handle) {
    if (!g_is_initialized || !handle) {
        return NULL;
    }
    return p_libusb_get_device(handle);
}

// 打开指定VID和PID的设备
void* usb_device_open_device_with_vid_pid(void* ctx, unsigned short vid, unsigned short pid) {
    if (!g_is_initialized) {
        return NULL;
    }
    return p_libusb_open_device_with_vid_pid(ctx ? ctx : g_libusb_context, vid, pid);
}
