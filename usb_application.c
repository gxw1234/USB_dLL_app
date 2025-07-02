/**
 * @file usb_application.c
 * @brief USB应用层公共接口实现
 */

#include "usb_application.h"
#include "usb_device.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <stdint.h>
#include <process.h>  // 用于线程函数


#define VENDOR_ID   0xCCDD          // 设备VID
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
WINAPI int USB_ScanDevice(device_info_t* devices, int max_devices) {
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
WINAPI int USB_OpenDevice(const char* target_serial) {
    // 初始化设备层
    int ret = usb_device_init();
    if (ret != USB_SUCCESS) {
        debug_printf("初始化USB设备层失败, 错误码: %d", ret);
        return ret;
    }
    
    // 如果已经打开，直接返回设备槽索引
    if (target_serial) {
        int idx = find_device_by_serial(target_serial);
        if (idx >= 0) {
            debug_printf("设备已打开, 序列号: %s, 槽位: %d", target_serial, idx);
            return idx;
        }
    }
    
    // 查找空闲设备槽
    int slot = find_free_device_slot();
    if (slot < 0) {
        debug_printf("没有可用的设备槽");
        return USB_ERROR_OTHER;
    }
    
    // 需要先扫描设备列表查找目标设备
    void** device_list = NULL;
    void* handle = NULL;
    char serial_buffer[64] = {0};
    
    // 获取设备列表
    int cnt = usb_device_get_device_list(g_libusb_context, &device_list);
    if (cnt < 0) {
        debug_printf("获取设备列表失败, 错误码: %d", cnt);
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
        debug_printf("未找到匹配的设备");
        return USB_ERROR_NOT_FOUND;
    }
    
    // 释放设备列表
    usb_device_free_device_list(device_list, 1);
    
    // 申请接口
    if (usb_device_claim_interface(handle, 0) != 0) {
        usb_device_close(handle);
        debug_printf("申请接口失败");
        return USB_ERROR_ACCESS;
    }
    
    // 初始化环形缓冲区
    open_devices[slot].ring_buffer.buffer = (unsigned char*)malloc(RING_BUFFER_SIZE);
    if (!open_devices[slot].ring_buffer.buffer) {
        usb_device_release_interface(handle, 0);
        usb_device_close(handle);
        debug_printf("内存分配失败");
        return USB_ERROR_OTHER;
    }
    
    // 初始化临界区
    if (!InitializeCriticalSectionAndSpinCount(&open_devices[slot].ring_buffer.cs, 0x1000)) {
        free(open_devices[slot].ring_buffer.buffer);
        usb_device_release_interface(handle, 0);
        usb_device_close(handle);
        debug_printf("初始化临界区失败");
        return USB_ERROR_OTHER;
    }
    
    // 初始化环形缓冲区参数
    open_devices[slot].ring_buffer.size = RING_BUFFER_SIZE;
    open_devices[slot].ring_buffer.write_pos = 0;
    open_devices[slot].ring_buffer.read_pos = 0;
    open_devices[slot].ring_buffer.data_size = 0;
    
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
    
    debug_printf("成功打开设备, 序列号: %s", open_devices[slot].serial);
    
    // 启动读取线程
    open_devices[slot].thread_running = TRUE;
    open_devices[slot].stop_thread = FALSE;
    open_devices[slot].read_thread = CreateThread(NULL, 0, read_thread_func, &open_devices[slot], 0, NULL);
    if (!open_devices[slot].read_thread) {
        DeleteCriticalSection(&open_devices[slot].ring_buffer.cs);
        free(open_devices[slot].ring_buffer.buffer);
        usb_device_release_interface(handle, 0);
        usb_device_close(handle);
        open_devices[slot].is_open = 0;
        open_devices[slot].handle = NULL;
        debug_printf("创建读取线程失败");
        return USB_ERROR_OTHER;
    }
    
    debug_printf("成功启动读取线程");
    return slot;
}

// 关闭USB设备实现
WINAPI int USB_CloseDevice(const char* target_serial) {
    if (!target_serial) {
        return USB_ERROR_INVALID_PARAM;
    }
    
    int idx = find_device_by_serial(target_serial);
    if (idx < 0) {
        return USB_ERROR_NOT_FOUND;
    }
    
    // 停止读取线程
    if (open_devices[idx].read_thread) {
        open_devices[idx].stop_thread = TRUE;
        // 使用超时等待，避免永久阻塞
        if (WaitForSingleObject(open_devices[idx].read_thread, 3000) == WAIT_TIMEOUT) {
            // 如果3秒后线程仍未退出，则强制终止
            TerminateThread(open_devices[idx].read_thread, 0);
        }
        CloseHandle(open_devices[idx].read_thread);
        open_devices[idx].read_thread = NULL;
    }
    
    // 释放环形缓冲区
    EnterCriticalSection(&open_devices[idx].ring_buffer.cs);
    if (open_devices[idx].ring_buffer.buffer) {
        free(open_devices[idx].ring_buffer.buffer);
        open_devices[idx].ring_buffer.buffer = NULL;
    }
    LeaveCriticalSection(&open_devices[idx].ring_buffer.cs);
    DeleteCriticalSection(&open_devices[idx].ring_buffer.cs);
    
    // 关闭设备句柄
    if (open_devices[idx].handle) {
        usb_device_release_interface(open_devices[idx].handle, 0);
        usb_device_close(open_devices[idx].handle);
        open_devices[idx].handle = NULL;
        open_devices[idx].is_open = 0;
        open_devices[idx].serial[0] = '\0';
    }
    
    return USB_SUCCESS;
}

// 从USB设备读取数据实现
WINAPI int USB_ReadData(const char* target_serial, unsigned char* data, int length) {
    if (!target_serial || !data || length <= 0) {
        debug_printf("无效的参数: target_serial=%p, data=%p, length=%d", target_serial, data, length);
        return USB_ERROR_INVALID_PARAM;
    }
    
    int idx = find_device_by_serial(target_serial);
    if (idx < 0) {
        debug_printf("未找到设备: %s", target_serial);
        return USB_ERROR_NOT_FOUND;
    }
    
    if (!open_devices[idx].is_open || !open_devices[idx].handle) {
        debug_printf("设备未打开或句柄无效: %s", target_serial);
        return USB_ERROR_NOT_FOUND;
    }
    
    // 检查线程状态
    if (!open_devices[idx].thread_running || !open_devices[idx].read_thread) {
        debug_printf("读取线程未运行: %s", target_serial);
        return USB_ERROR_IO;
    }
    
    // 从环形缓冲区读取最新的数据
    EnterCriticalSection(&open_devices[idx].ring_buffer.cs);
    
    int available = open_devices[idx].ring_buffer.data_size;
    int to_read = (available < length) ? available : length;
    
    debug_printf("请求读取 %d 字节，可用 %d 字节，将读取 %d 字节", length, available, to_read);
    
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
            debug_printf("读取了所有可用数据，重置缓冲区");
        }
    } else {
        debug_printf("没有可用数据");
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

/**
 * @brief 向USB设备发送数据
 * 
 * @param target_serial 目标设备的序列号
 * @param data 要发送的数据缓冲区
 * @param length 要发送的数据长度
 * @return int 实际发送的数据长度，小于0表示错误
 */
WINAPI int USB_WriteData(const char* target_serial, unsigned char* data, int length) {
    // 验证参数
    if (!target_serial || !data || length <= 0) {
        debug_printf("参数无效: target_serial=%p, data=%p, length=%d", target_serial, data, length);
        return USB_ERROR_INVALID_PARAM;
    }
    
    // 查找已打开的设备
    int idx = find_device_by_serial(target_serial);
    if (idx < 0) {
        debug_printf("设备未找到或未打开: %s", target_serial);
        return USB_ERROR_NOT_FOUND;
    }
    
    // 获取设备句柄
    void* handle = open_devices[idx].handle;
    if (!handle) {
        debug_printf("设备句柄无效: %s", target_serial);
        return USB_ERROR_NOT_FOUND;
    }
    
    // 准备发送数据 (端点1是OUT端点)
    int transferred = 0;
    int ret = usb_device_bulk_transfer(handle, 0x01, data, length, &transferred, 1000);
    
    if (ret != 0) {
        debug_printf("发送数据失败: %d", ret);
        return USB_ERROR_IO;
    }
    
    debug_printf("成功发送 %d 字节数据", transferred);
    return transferred;
}

/**
 * @brief 设置USB调试日志状态
 * 
 * @param enable 1=开启日志，0=关闭日志
 */
WINAPI void USB_SetLogging(int enable) {
    USB_SetLog(enable);
}

// DLL入口点
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
    switch (fdwReason) {
        case DLL_PROCESS_ATTACH:
            // 初始化代码 - 不需要额外操作
            break;
        case DLL_PROCESS_DETACH:
            // 清理代码
            usb_device_cleanup();
            // 确保所有设备关闭
            for (int i = 0; i < MAX_OPEN_DEVICES; i++) {
                if (open_devices[i].is_open && open_devices[i].handle) {
                    // 停止线程
                    if (open_devices[i].read_thread) {
                        open_devices[i].stop_thread = TRUE;
                        // 使用超时等待，避免永久阻塞
                        if (WaitForSingleObject(open_devices[i].read_thread, 1000) == WAIT_TIMEOUT) {
                            // 如果1秒后线程仍未退出，则强制终止
                            TerminateThread(open_devices[i].read_thread, 0);
                        }
                        CloseHandle(open_devices[i].read_thread);
                        open_devices[i].read_thread = NULL;
                    }
                    
                    // 释放缓冲区
                    if (open_devices[i].ring_buffer.buffer) {
                        DeleteCriticalSection(&open_devices[i].ring_buffer.cs);
                        free(open_devices[i].ring_buffer.buffer);
                    }
                    
                    // 释放设备
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
