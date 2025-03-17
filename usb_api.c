/**
 * @file usb_api.c
 * @brief USB API公共接口实现
 */

#include "usb_api.h"
#include "usb_device.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <stdint.h>
#include <process.h>  // 用于线程函数

// 目标设备的VID和PID
#define VENDOR_ID   0x1733          // 设备VID
#define PRODUCT_ID  0xAABB          // 设备PID

// 环形缓冲区大小 (10MB)
#define RING_BUFFER_SIZE (10 * 1024 * 1024)

// 环形缓冲区结构
typedef struct {
    unsigned char* buffer;       // 缓冲区指针
    unsigned int size;           // 缓冲区大小
    unsigned int write_pos;      // 写入位置
    unsigned int read_pos;       // 读取位置
    unsigned int data_size;      // 当前数据大小
    CRITICAL_SECTION cs;         // 临界区，用于线程同步
} ring_buffer_t;

// 打开的设备信息
#define MAX_OPEN_DEVICES 16
typedef struct {
    char serial[64];
    void* handle;
    int is_open;
    
    // 添加环形缓冲区和线程相关字段
    ring_buffer_t ring_buffer;   // 环形缓冲区
    HANDLE read_thread;          // 读取线程句柄
    BOOL thread_running;         // 线程运行标志
    BOOL stop_thread;            // 线程停止请求标志
} open_device_t;

static open_device_t open_devices[MAX_OPEN_DEVICES] = {0};

// 线程函数前向声明
DWORD WINAPI read_thread_func(LPVOID lpParameter);

// 查找空闲设备槽
static int find_free_device_slot(void) {
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

    // 初始化设备层
    int ret = usb_device_init();
    if (ret != USB_SUCCESS) {
        return ret;
    }

    void** device_list = NULL;
    int device_count = 0;
    
    // 获取设备列表
    int cnt = usb_device_get_device_list(g_libusb_context, &device_list);
    if (cnt < 0) {
        return USB_ERROR_OTHER;
    }

    // 遍历设备
    for (int i = 0; i < cnt && device_count < max_devices; i++) {
        void* dev = device_list[i];
        usb_device_descriptor desc;
        
        // 获取设备描述符
        if (usb_device_get_device_descriptor(dev, &desc) != 0) {
            continue;
        }
        
        // 只扫描特定VID和PID的设备
        if (desc.idVendor == VENDOR_ID && desc.idProduct == PRODUCT_ID) {
            // 打开设备以获取详细信息
            void* handle = NULL;
            if (usb_device_open(dev, &handle) != 0) {
                continue;
            }
            
            // 初始化设备信息结构体
            memset(&devices[device_count], 0, sizeof(device_info_t));
            
            // 填充设备信息
            devices[device_count].vendor_id = desc.idVendor;
            devices[device_count].product_id = desc.idProduct;
            
            // 获取序列号
            if (desc.iSerialNumber > 0) {
                unsigned char serial_buffer[64] = {0};
                if (usb_device_get_string_descriptor_ascii(handle, desc.iSerialNumber, serial_buffer, sizeof(serial_buffer)) > 0) {
                    strncpy(devices[device_count].serial, (char*)serial_buffer, sizeof(devices[device_count].serial) - 1);
                }
            }
            
            // 获取产品描述
            if (desc.iProduct > 0) {
                unsigned char desc_buffer[256] = {0};
                if (usb_device_get_string_descriptor_ascii(handle, desc.iProduct, desc_buffer, sizeof(desc_buffer)) > 0) {
                    strncpy(devices[device_count].description, (char*)desc_buffer, sizeof(devices[device_count].description) - 1);
                }
            }
            
            // 获取制造商信息
            if (desc.iManufacturer > 0) {
                unsigned char manufacturer_buffer[256] = {0};
                if (usb_device_get_string_descriptor_ascii(handle, desc.iManufacturer, manufacturer_buffer, sizeof(manufacturer_buffer)) > 0) {
                    strncpy(devices[device_count].manufacturer, (char*)manufacturer_buffer, sizeof(devices[device_count].manufacturer) - 1);
                }
            }
            
            // 关闭设备句柄
            usb_device_close(handle);
            
            device_count++;
        }
    }
    
    // 释放设备列表
    usb_device_free_device_list(device_list, 1);
    
    return device_count;
}

// 打开USB设备实现
USB_API int USB_OpenDevice(const char* target_serial) {
    // 初始化设备层
    int ret = usb_device_init();
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
    
    // 需要先扫描设备列表查找目标设备
    void** device_list = NULL;
    void* handle = NULL;
    char serial_buffer[64] = {0};
    
    // 获取设备列表
    int cnt = usb_device_get_device_list(g_libusb_context, &device_list);
    if (cnt < 0) {
        return USB_ERROR_OTHER;
    }
    
    // 遍历设备
    for (int i = 0; i < cnt; i++) {
        void* dev = device_list[i];
        usb_device_descriptor desc;
        
        // 获取设备描述符
        if (usb_device_get_device_descriptor(dev, &desc) != 0) {
            continue;
        }
        
        // 检查VID/PID是否匹配
        if (desc.idVendor == VENDOR_ID && desc.idProduct == PRODUCT_ID) {
            // 临时打开设备获取序列号
            void* temp_handle = NULL;
            if (usb_device_open(dev, &temp_handle) != 0) {
                continue;
            }
            
            // 读取序列号
            if (desc.iSerialNumber > 0) {
                unsigned char temp_serial[64] = {0};
                if (usb_device_get_string_descriptor_ascii(temp_handle, desc.iSerialNumber, temp_serial, sizeof(temp_serial)) > 0) {
                    // 如果提供了目标序列号，检查是否匹配
                    if (target_serial) {
                        if (strcmp(target_serial, (char*)temp_serial) == 0) {
                            // 匹配目标序列号，保存句柄
                            handle = temp_handle;
                            strncpy(serial_buffer, (char*)temp_serial, sizeof(serial_buffer) - 1);
                            break;
                        }
                    } else {
                        // 未提供目标序列号，使用第一个找到的设备
                        handle = temp_handle;
                        strncpy(serial_buffer, (char*)temp_serial, sizeof(serial_buffer) - 1);
                        break;
                    }
                }
            } else if (!target_serial) {
                // 没有序列号但也没指定目标序列号，使用此设备
                handle = temp_handle;
                snprintf(serial_buffer, sizeof(serial_buffer) - 1, "%04X:%04X", desc.idVendor, desc.idProduct);
                break;
            }
            
            // 如果不匹配或没有序列号，关闭临时句柄
            usb_device_close(temp_handle);
        }
    }
    
    // 如果未找到设备
    if (!handle) {
        if (device_list) {
            usb_device_free_device_list(device_list, 1);
        }
        return USB_ERROR_NOT_FOUND;
    }
    
    // 释放设备列表
    usb_device_free_device_list(device_list, 1);
    
    // 申请接口
    if (usb_device_claim_interface(handle, 0) != 0) {
        usb_device_close(handle);
        return USB_ERROR_ACCESS;
    }
    
    // 保存设备信息
    open_devices[slot].handle = handle;
    open_devices[slot].is_open = 1;
    
    // 如果有序列号，保存序列号
    if (serial_buffer[0] != '\0') {
        strncpy(open_devices[slot].serial, serial_buffer, sizeof(open_devices[slot].serial) - 1);
    } else if (target_serial) {
        strncpy(open_devices[slot].serial, target_serial, sizeof(open_devices[slot].serial) - 1);
    } else {
        // 如果没有序列号，使用VID:PID字符串作为标识
        snprintf(open_devices[slot].serial, sizeof(open_devices[slot].serial) - 1, "%04X:%04X", VENDOR_ID, PRODUCT_ID);
    }
    
    // 初始化环形缓冲区
    open_devices[slot].ring_buffer.buffer = (unsigned char*)malloc(RING_BUFFER_SIZE);
    open_devices[slot].ring_buffer.size = RING_BUFFER_SIZE;
    open_devices[slot].ring_buffer.write_pos = 0;
    open_devices[slot].ring_buffer.read_pos = 0;
    open_devices[slot].ring_buffer.data_size = 0;
    InitializeCriticalSection(&open_devices[slot].ring_buffer.cs);
    
    // 启动读取线程
    open_devices[slot].read_thread = CreateThread(NULL, 0, read_thread_func, &open_devices[slot], 0, NULL);
    if (!open_devices[slot].read_thread) {
        free(open_devices[slot].ring_buffer.buffer);
        open_devices[slot].ring_buffer.buffer = NULL;
        return USB_ERROR_OTHER;
    }
    
    open_devices[slot].thread_running = TRUE;
    open_devices[slot].stop_thread = FALSE;
    
    return slot;
}

// 关闭USB设备实现
USB_API int USB_CloseDevice(const char* target_serial) {
    if (!target_serial) {
        return USB_ERROR_INVALID_PARAM;
    }
    
    int idx = find_device_by_serial(target_serial);
    if (idx < 0) {
        return USB_ERROR_NOT_FOUND;
    }
    
    if (open_devices[idx].handle) {
        usb_device_release_interface(open_devices[idx].handle, 0);
        usb_device_close(open_devices[idx].handle);
        open_devices[idx].handle = NULL;
        open_devices[idx].is_open = 0;
        open_devices[idx].serial[0] = '\0';
    }
    
    // 停止读取线程
    open_devices[idx].stop_thread = TRUE;
    WaitForSingleObject(open_devices[idx].read_thread, INFINITE);
    CloseHandle(open_devices[idx].read_thread);
    open_devices[idx].read_thread = NULL;
    
    // 释放环形缓冲区
    if (open_devices[idx].ring_buffer.buffer) {
        free(open_devices[idx].ring_buffer.buffer);
        open_devices[idx].ring_buffer.buffer = NULL;
    }
    
    return USB_SUCCESS;
}

// 从USB设备读取数据实现
USB_API int USB_ReadData(const char* target_serial, unsigned char* data, int length) {
    if (!target_serial || !data || length <= 0) {
        return USB_ERROR_INVALID_PARAM;
    }
    
    int idx = find_device_by_serial(target_serial);
    if (idx < 0) {
        return USB_ERROR_NOT_FOUND;
    }
    
    if (!open_devices[idx].is_open || !open_devices[idx].handle) {
        return USB_ERROR_NOT_FOUND;
    }
    
    // 从环形缓冲区读取最新的数据
    EnterCriticalSection(&open_devices[idx].ring_buffer.cs);
    
    int available = open_devices[idx].ring_buffer.data_size;
    int to_read = (available < length) ? available : length;
    
    if (to_read > 0) {
        // 从环形缓冲区的末尾读取最新数据
        int start_pos;
        if (available <= length) {
            // 可用数据少于请求的长度，返回所有可用数据
            start_pos = open_devices[idx].ring_buffer.read_pos;
        } else {
            // 可用数据多于请求的长度，返回最新的length字节
            start_pos = (open_devices[idx].ring_buffer.write_pos - length + RING_BUFFER_SIZE) % RING_BUFFER_SIZE;
        }
        
        // 复制数据
        if (start_pos + to_read <= RING_BUFFER_SIZE) {
            // 连续数据块
            memcpy(data, open_devices[idx].ring_buffer.buffer + start_pos, to_read);
        } else {
            // 环形缓冲区环绕，需要两次复制
            int first_part = RING_BUFFER_SIZE - start_pos;
            memcpy(data, open_devices[idx].ring_buffer.buffer + start_pos, first_part);
            memcpy(data + first_part, open_devices[idx].ring_buffer.buffer, to_read - first_part);
        }
        
        // 更新读取位置，但只有当我们读取的是完整的可用数据时才更新
        if (available <= length) {
            open_devices[idx].ring_buffer.read_pos = open_devices[idx].ring_buffer.write_pos;
            open_devices[idx].ring_buffer.data_size = 0;
        }
    }
    
    LeaveCriticalSection(&open_devices[idx].ring_buffer.cs);
    
    return to_read;
}

// 从USB设备读取线程函数
DWORD WINAPI read_thread_func(LPVOID lpParameter) {
    open_device_t* device = (open_device_t*)lpParameter;
    unsigned char temp_buffer[4096]; // 临时缓冲区，用于读取数据
    
    while (!device->stop_thread) {
        int actual_length = 0;
        
        // 从设备读取数据到临时缓冲区
        int ret = usb_device_bulk_transfer(device->handle, 0x81, temp_buffer, sizeof(temp_buffer), &actual_length, 1000);
        
        if (ret == LIBUSB_SUCCESS && actual_length > 0) {
            // 将数据写入环形缓冲区
            EnterCriticalSection(&device->ring_buffer.cs);
            
            // 检查环形缓冲区空间是否足够
            if (device->ring_buffer.data_size + actual_length > device->ring_buffer.size) {
                // 空间不足，丢弃旧数据
                // 需要丢弃的数据量
                int discard = (device->ring_buffer.data_size + actual_length) - device->ring_buffer.size;
                device->ring_buffer.read_pos = (device->ring_buffer.read_pos + discard) % device->ring_buffer.size;
                device->ring_buffer.data_size -= discard;
            }
            
            // 写入新数据
            // 检查是否需要环绕写入
            if (device->ring_buffer.write_pos + actual_length <= device->ring_buffer.size) {
                // 直接写入
                memcpy(device->ring_buffer.buffer + device->ring_buffer.write_pos, temp_buffer, actual_length);
            } else {
                // 需要环绕写入
                int first_part = device->ring_buffer.size - device->ring_buffer.write_pos;
                memcpy(device->ring_buffer.buffer + device->ring_buffer.write_pos, temp_buffer, first_part);
                memcpy(device->ring_buffer.buffer, temp_buffer + first_part, actual_length - first_part);
            }
            
            // 更新写入位置和数据大小
            device->ring_buffer.write_pos = (device->ring_buffer.write_pos + actual_length) % device->ring_buffer.size;
            device->ring_buffer.data_size += actual_length;
            
            LeaveCriticalSection(&device->ring_buffer.cs);
        } else if (ret == LIBUSB_ERROR_TIMEOUT) {
            // 超时，继续尝试
            Sleep(10);
        } else {
            // 读取错误，暂停一下再试
            Sleep(100);
        }
    }
    
    return 0;
}

// DLL入口点
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
    switch (fdwReason) {
        case DLL_PROCESS_ATTACH:
            // 初始化代码
            break;
        case DLL_PROCESS_DETACH:
            // 清理代码
            usb_device_cleanup();
            // 确保所有设备关闭
            for (int i = 0; i < MAX_OPEN_DEVICES; i++) {
                if (open_devices[i].is_open && open_devices[i].handle) {
                    usb_device_release_interface(open_devices[i].handle, 0);
                    usb_device_close(open_devices[i].handle);
                    open_devices[i].is_open = 0;
                    open_devices[i].handle = NULL;
                }
            }
            break;
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
            break;
    }
    return TRUE;
}
