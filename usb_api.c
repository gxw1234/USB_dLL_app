#include "usb_api.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

// 目标设备的VID和PID
#define VENDOR_ID   0x1733
#define PRODUCT_ID  0xAABB

// USB描述符字符串定义
#define USBD_PRODUCT_STRING_HS     "GXW"
#define USBD_INTERFACE_STRING_HS   "WINUSB Interface"
#define USBD_MANUFACTURER_STRING   "STMicroelectronics"

// libusb相关定义
#define LIBUSB_SUCCESS          0
#define LIBUSB_ERROR_TIMEOUT   -7
#define LIBUSB_ERROR_NOT_FOUND -5

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
typedef int (*libusb_get_string_descriptor_ascii_t)(void*, unsigned char, unsigned char*, int);

// 设备描述符结构体简化定义
typedef struct {
    unsigned char  bLength;
    unsigned char  bDescriptorType;
    unsigned short bcdUSB;
    unsigned char  bDeviceClass;
    unsigned char  bDeviceSubClass;
    unsigned char  bDeviceProtocol;
    unsigned char  bMaxPacketSize0;
    unsigned short idVendor;
    unsigned short idProduct;
    unsigned short bcdDevice;
    unsigned char  iManufacturer;
    unsigned char  iProduct;
    unsigned char  iSerialNumber;
    unsigned char  bNumConfigurations;
} usb_device_descriptor;

// 全局变量
static HMODULE hLibusbDll = NULL;
static void* libusbContext = NULL;
static int isInitialized = 0;

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

// 打开的设备信息
#define MAX_OPEN_DEVICES 16
typedef struct {
    char serial[64];
    void* handle;
    int is_open;
} open_device_t;

static open_device_t open_devices[MAX_OPEN_DEVICES] = {0};

// 初始化libusb
static int initialize_libusb() {
    if (isInitialized) {
        return USB_SUCCESS;
    }

    // 加载libusb动态库
    hLibusbDll = LoadLibrary("libusb-1.0.dll");
    if (!hLibusbDll) {
        return USB_ERROR_OTHER;
    }

    // 获取函数指针
    p_libusb_init = (libusb_init_t)GetProcAddress(hLibusbDll, "libusb_init");
    p_libusb_exit = (libusb_exit_t)GetProcAddress(hLibusbDll, "libusb_exit");
    p_libusb_open_device_with_vid_pid = (libusb_open_device_with_vid_pid_t)GetProcAddress(hLibusbDll, "libusb_open_device_with_vid_pid");
    p_libusb_claim_interface = (libusb_claim_interface_t)GetProcAddress(hLibusbDll, "libusb_claim_interface");
    p_libusb_bulk_transfer = (libusb_bulk_transfer_t)GetProcAddress(hLibusbDll, "libusb_bulk_transfer");
    p_libusb_close = (libusb_close_t)GetProcAddress(hLibusbDll, "libusb_close");
    p_libusb_release_interface = (libusb_release_interface_t)GetProcAddress(hLibusbDll, "libusb_release_interface");
    p_libusb_get_device_list = (libusb_get_device_list_t)GetProcAddress(hLibusbDll, "libusb_get_device_list");
    p_libusb_free_device_list = (libusb_free_device_list_t)GetProcAddress(hLibusbDll, "libusb_free_device_list");
    p_libusb_get_device_descriptor = (libusb_get_device_descriptor_t)GetProcAddress(hLibusbDll, "libusb_get_device_descriptor");
    p_libusb_get_string_descriptor_ascii = (libusb_get_string_descriptor_ascii_t)GetProcAddress(hLibusbDll, "libusb_get_string_descriptor_ascii");
    p_libusb_get_device = (libusb_get_device_t)GetProcAddress(hLibusbDll, "libusb_get_device");
    p_libusb_open = (libusb_open_t)GetProcAddress(hLibusbDll, "libusb_open");

    // 验证所有函数指针
    if (!p_libusb_init || !p_libusb_exit || !p_libusb_open_device_with_vid_pid || 
        !p_libusb_claim_interface || !p_libusb_bulk_transfer || !p_libusb_close ||
        !p_libusb_release_interface || !p_libusb_get_device_list || !p_libusb_free_device_list ||
        !p_libusb_get_device_descriptor || !p_libusb_get_string_descriptor_ascii ||
        !p_libusb_get_device || !p_libusb_open) {
        FreeLibrary(hLibusbDll);
        hLibusbDll = NULL;
        return USB_ERROR_OTHER;
    }

    // 初始化libusb
    int ret = p_libusb_init(&libusbContext);
    if (ret != LIBUSB_SUCCESS) {
        FreeLibrary(hLibusbDll);
        hLibusbDll = NULL;
        return USB_ERROR_OTHER;
    }

    isInitialized = 1;
    return USB_SUCCESS;
}

// 清理libusb
static void cleanup_libusb() {
    if (!isInitialized) {
        return;
    }

    // 关闭所有打开的设备
    for (int i = 0; i < MAX_OPEN_DEVICES; i++) {
        if (open_devices[i].is_open && open_devices[i].handle) {
            p_libusb_release_interface(open_devices[i].handle, 0);
            p_libusb_close(open_devices[i].handle);
            open_devices[i].is_open = 0;
            open_devices[i].handle = NULL;
            open_devices[i].serial[0] = '\0';
        }
    }

    // 退出libusb
    if (libusbContext) {
        p_libusb_exit(libusbContext);
        libusbContext = NULL;
    }

    // 卸载DLL
    if (hLibusbDll) {
        FreeLibrary(hLibusbDll);
        hLibusbDll = NULL;
    }

    isInitialized = 0;
}

// 查找空闲设备槽
static int find_free_device_slot() {
    for (int i = 0; i < MAX_OPEN_DEVICES; i++) {
        if (!open_devices[i].is_open) {
            return i;
        }
    }
    return -1;
}

// 根据序列号查找已打开的设备
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

// 扫描USB设备实现
USB_API int USB_ScanDevice(device_info_t* devices, int max_devices) {
    if (!devices || max_devices <= 0) {
        return USB_ERROR_INVALID_PARAM;
    }

    // 初始化libusb
    int ret = initialize_libusb();
    if (ret != USB_SUCCESS) {
        return ret;
    }

    void** device_list = NULL;
    int device_count = 0;
    
    // 获取设备列表
    int cnt = p_libusb_get_device_list(libusbContext, &device_list);
    if (cnt < 0) {
        return USB_ERROR_OTHER;
    }

    // 遍历设备
    for (int i = 0; i < cnt && device_count < max_devices; i++) {
        void* dev = device_list[i];
        usb_device_descriptor desc;
        
        // 获取设备描述符
        if (p_libusb_get_device_descriptor(dev, &desc) != 0) {
            continue;
        }
        
        // 只扫描特定VID和PID的设备
        if (desc.idVendor == VENDOR_ID && desc.idProduct == PRODUCT_ID) {
            // 打开设备以获取详细信息
            void* handle = NULL;
            if (p_libusb_open(dev, &handle) != 0) {
                continue;
            }
            
            // 填充设备信息
            devices[device_count].vendor_id = desc.idVendor;
            devices[device_count].product_id = desc.idProduct;
            
            // 获取序列号
            if (desc.iSerialNumber > 0) {
                unsigned char serial_buffer[64] = {0};
                if (p_libusb_get_string_descriptor_ascii(handle, desc.iSerialNumber, serial_buffer, sizeof(serial_buffer)) > 0) {
                    strncpy(devices[device_count].serial, (char*)serial_buffer, sizeof(devices[device_count].serial) - 1);
                }
            }
            
            // 获取产品描述
            if (desc.iProduct > 0) {
                unsigned char desc_buffer[256] = {0};
                if (p_libusb_get_string_descriptor_ascii(handle, desc.iProduct, desc_buffer, sizeof(desc_buffer)) > 0) {
                    strncpy(devices[device_count].description, (char*)desc_buffer, sizeof(devices[device_count].description) - 1);
                } else {
                    // 如果无法从设备获取描述，则使用预定义的字符串
                    strncpy(devices[device_count].description, USBD_PRODUCT_STRING_HS, sizeof(devices[device_count].description) - 1);
                }
            } else {
                // 如果设备没有提供产品描述符，则使用预定义的字符串
                strncpy(devices[device_count].description, USBD_PRODUCT_STRING_HS, sizeof(devices[device_count].description) - 1);
            }
            
            // 获取制造商信息
            if (desc.iManufacturer > 0) {
                unsigned char manufacturer_buffer[256] = {0};
                if (p_libusb_get_string_descriptor_ascii(handle, desc.iManufacturer, manufacturer_buffer, sizeof(manufacturer_buffer)) > 0) {
                    strncpy(devices[device_count].manufacturer, (char*)manufacturer_buffer, sizeof(devices[device_count].manufacturer) - 1);
                } else {
                    // 如果无法从设备获取制造商信息，则使用预定义的字符串
                    strncpy(devices[device_count].manufacturer, USBD_MANUFACTURER_STRING, sizeof(devices[device_count].manufacturer) - 1);
                }
            } else {
                // 如果设备没有提供制造商描述符，则使用预定义的字符串
                strncpy(devices[device_count].manufacturer, USBD_MANUFACTURER_STRING, sizeof(devices[device_count].manufacturer) - 1);
            }
            
            // 设置接口信息（从libusb无法直接获取接口字符串描述符，因此使用预定义的字符串）
            strncpy(devices[device_count].interface_str, USBD_INTERFACE_STRING_HS, sizeof(devices[device_count].interface_str) - 1);
            
            // 关闭设备句柄
            p_libusb_close(handle);
            
            device_count++;
        }
    }
    
    // 释放设备列表
    p_libusb_free_device_list(device_list, 1);
    
    return device_count;
}

// 打开USB设备实现
USB_API int USB_OpenDevice(const char* target_serial) {
    // 初始化libusb
    int ret = initialize_libusb();
    if (ret != USB_SUCCESS) {
        return ret;
    }
    
    // 如果已经打开，直接返回设备槽索引
    if (target_serial) {
        int idx = find_device_by_serial(target_serial);
        if (idx >= 0) {
            return idx;
        }
    }
    
    // 查找空闲设备槽
    int slot = find_free_device_slot();
    if (slot < 0) {
        return USB_ERROR_OTHER;
    }
    
    // 直接尝试打开特定VID/PID的设备
    void* handle = p_libusb_open_device_with_vid_pid(libusbContext, VENDOR_ID, PRODUCT_ID);
    if (!handle) {
        return USB_ERROR_NOT_FOUND;
    }
    
    // 申请接口
    if (p_libusb_claim_interface(handle, 0) != 0) {
        p_libusb_close(handle);
        return USB_ERROR_ACCESS;
    }
    
    // 如果没有指定序列号，尝试获取序列号
    char serial_buffer[64] = {0};
    if (!target_serial) {
        // 获取设备描述符
        usb_device_descriptor desc;
        void* dev = p_libusb_get_device(handle);
        if (p_libusb_get_device_descriptor(dev, &desc) == 0 && desc.iSerialNumber > 0) {
            unsigned char temp_buffer[64] = {0};
            if (p_libusb_get_string_descriptor_ascii(handle, desc.iSerialNumber, temp_buffer, sizeof(temp_buffer)) > 0) {
                strncpy(serial_buffer, (char*)temp_buffer, sizeof(serial_buffer) - 1);
                target_serial = serial_buffer;
            }
        }
    }
    
    // 保存设备信息
    open_devices[slot].handle = handle;
    open_devices[slot].is_open = 1;
    if (target_serial) {
        strncpy(open_devices[slot].serial, target_serial, sizeof(open_devices[slot].serial) - 1);
    } else {
        // 如果没有序列号，使用VID:PID字符串作为标识
        snprintf(open_devices[slot].serial, sizeof(open_devices[slot].serial) - 1, "%04X:%04X", VENDOR_ID, PRODUCT_ID);
    }
    
    return slot;
}

// 关闭USB设备实现
USB_API int USB_CloseDevice(const char* target_serial) {
    if (!isInitialized || !target_serial) {
        return USB_ERROR_INVALID_PARAM;
    }
    
    int idx = find_device_by_serial(target_serial);
    if (idx < 0) {
        return USB_ERROR_NOT_FOUND;
    }
    
    if (open_devices[idx].handle) {
        p_libusb_release_interface(open_devices[idx].handle, 0);
        p_libusb_close(open_devices[idx].handle);
        open_devices[idx].handle = NULL;
        open_devices[idx].is_open = 0;
        open_devices[idx].serial[0] = '\0';
    }
    
    return USB_SUCCESS;
}

// 从USB设备读取数据实现
USB_API int USB_ReadData(const char* target_serial, unsigned char* data, int length) {
    if (!isInitialized || !target_serial || !data || length <= 0) {
        return USB_ERROR_INVALID_PARAM;
    }
    
    int idx = find_device_by_serial(target_serial);
    if (idx < 0) {
        return USB_ERROR_NOT_FOUND;
    }
    
    if (!open_devices[idx].is_open || !open_devices[idx].handle) {
        return USB_ERROR_NOT_FOUND;
    }
    
    int actual_length = 0;
    int ret = p_libusb_bulk_transfer(open_devices[idx].handle, 0x81, data, length, &actual_length, 1000);
    
    if (ret == LIBUSB_SUCCESS) {
        return actual_length;
    } else if (ret == LIBUSB_ERROR_TIMEOUT) {
        return 0;
    } else {
        return USB_ERROR_IO;
    }
}

// DLL入口点
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
    switch (fdwReason) {
        case DLL_PROCESS_ATTACH:
            // 初始化代码
            break;
        case DLL_PROCESS_DETACH:
            // 清理代码
            cleanup_libusb();
            break;
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
            break;
    }
    return TRUE;
}
