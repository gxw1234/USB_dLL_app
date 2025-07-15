

#include "usb_middleware.h"
#include "usb_device.h"
#include "usb_log.h"
#include "usb_protocol.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <time.h>

typedef struct _GENERIC_CMD_HEADER {
    uint8_t protocol_type;  // 协议类型：SPI/IIC/UART等
    uint8_t cmd_id;         // 命令ID：初始化/读/写等
    uint8_t device_index;   // 设备索引
    uint8_t param_count;    // 参数数量
    uint16_t data_len;      // 数据部分长度
    uint16_t total_packets; // 整包总数
} GENERIC_CMD_HEADER;


#define MAX_DEVICES 10
#define SPI_BUFFER_SIZE (10 * 1024 * 1024)    // SPI专用缓冲区
#define POWER_BUFFER_SIZE (128 * 1024)       // 电源数据缓冲区
#define RAW_BUFFER_SIZE (512 * 1024)         //  原始数据临时缓冲区


static device_handle_t g_devices[MAX_DEVICES];
static int g_device_count = 0;
static int g_next_device_id = 0;
static int g_initialized = 0;

extern void debug_printf(const char *format, ...);

static DWORD WINAPI usb_device_read_thread_func(LPVOID lpParameter) {
    device_handle_t* device = (device_handle_t*)lpParameter;
    unsigned char temp_buffer[4096];
    while (!device->stop_thread) {
        int actual_length = 0;
        int ret = usb_device_bulk_transfer(device->libusb_handle, 0x81, temp_buffer, sizeof(temp_buffer), &actual_length, 1000);
        if (ret == 0 && actual_length > 0) {
            parse_and_dispatch_protocol_data(device, temp_buffer, actual_length);
        } else if (ret == -7) {
            debug_printf("读取超时");
            Sleep(10);
        } else {
            debug_printf("读取错误: %d", ret);
            Sleep(100);
        }
    }

    return 0;
}


void parse_and_dispatch_protocol_data(device_handle_t* device, unsigned char* raw_data, int length) {
    int pos = 0;
    while (pos < length) {
        if (pos + sizeof(GENERIC_CMD_HEADER) > length) {

            EnterCriticalSection(&device->raw_buffer.cs);
            write_to_ring_buffer(&device->raw_buffer, raw_data + pos, length - pos);
            LeaveCriticalSection(&device->raw_buffer.cs);
            break;
        }
        
        GENERIC_CMD_HEADER* header = (GENERIC_CMD_HEADER*)(raw_data + pos);
        int packet_size = sizeof(GENERIC_CMD_HEADER) + header->data_len;

        if (pos + packet_size > length) {

            EnterCriticalSection(&device->raw_buffer.cs);
            write_to_ring_buffer(&device->raw_buffer, raw_data + pos, length - pos);
            LeaveCriticalSection(&device->raw_buffer.cs);
            break;
        }
        
        if (header->protocol_type == PROTOCOL_SPI) {

            unsigned char* spi_data = raw_data + pos + sizeof(GENERIC_CMD_HEADER);
            int spi_data_len = header->data_len;
            
            EnterCriticalSection(&device->protocol_buffers[PROTOCOL_SPI].cs);
            write_to_ring_buffer(&device->protocol_buffers[PROTOCOL_SPI], spi_data, spi_data_len);
            LeaveCriticalSection(&device->protocol_buffers[PROTOCOL_SPI].cs);
            
            debug_printf("分发SPI数据: %d字节", spi_data_len);
        }
        
        pos += packet_size;
    }
}

void write_to_ring_buffer(ring_buffer_t* rb, unsigned char* data, int length) {
    if (rb->data_size + length > rb->size) {
        int discard = (rb->data_size + length) - rb->size;
        rb->read_pos = (rb->read_pos + discard) % rb->size;
        rb->data_size -= discard;
    }
    if (rb->write_pos + length <= rb->size) {
        memcpy(rb->buffer + rb->write_pos, data, length);
    } else {
        int first_part = rb->size - rb->write_pos;
        memcpy(rb->buffer + rb->write_pos, data, first_part);
        memcpy(rb->buffer, data + first_part, length - first_part);
    }
    rb->write_pos = (rb->write_pos + length) % rb->size;
    rb->data_size += length;
}


int usb_middleware_init(void) {
    if (g_initialized) {
        debug_printf("USB中间层已经初始化");
        return USB_SUCCESS;
    }
    
    memset(g_devices, 0, sizeof(g_devices));
    g_device_count = 0;
    g_next_device_id = 0;
    
    int ret = usb_device_init();
    if (ret < 0) {
        debug_printf("USB设备层初始化失败: %d", ret);
        return USB_ERROR_OTHER;
    }
    
    g_initialized = 1;
    debug_printf("USB中间层初始化成功");
    return USB_SUCCESS;
}


void usb_middleware_cleanup(void) {
    if (!g_initialized) {
        return;
    }
    for (int i = 0; i < MAX_DEVICES; i++) {
        if (g_devices[i].state == DEVICE_STATE_OPEN) {
            usb_middleware_close_device(g_devices[i].device_id);
        }
    }
    
    usb_device_cleanup();
    
    memset(g_devices, 0, sizeof(g_devices));
    g_device_count = 0;
    g_next_device_id = 0;
    g_initialized = 0;
    
    debug_printf("USB中间层清理完成");
}

/**
 * @brief 扫描USB设备
 */
int usb_middleware_scan_devices(device_info_t* devices, int max_devices) {
    if (!g_initialized || !devices || max_devices <= 0) {
        return 0;
    }
    
    void** device_list = NULL;
    int cnt = usb_device_get_device_list(g_libusb_context, &device_list);
    if (cnt < 0) {
        debug_printf("获取设备列表失败: %d", cnt);
        return 0;
    }
    
    int device_count = 0;
    
    for (int i = 0; i < cnt && device_count < max_devices; i++) {
        void* dev = device_list[i];
        usb_device_descriptor desc;
        
        if (usb_device_get_device_descriptor(dev, &desc) != 0) {
            continue;
        }
        
        if (desc.idVendor == 0xCCDD && desc.idProduct == 0xAABB) {
            void* handle = NULL;
            if (usb_device_open(dev, &handle) != 0) {
                continue;
            }
            
            memset(&devices[device_count], 0, sizeof(device_info_t));
            
            if (desc.iSerialNumber > 0) {
                unsigned char serial_buffer[64] = {0};
                if (usb_device_get_string_descriptor_ascii(handle, desc.iSerialNumber, serial_buffer, sizeof(serial_buffer)) > 0) {
                    strncpy(devices[device_count].serial, (char*)serial_buffer, sizeof(devices[device_count].serial) - 1);
                }
            }
            
            if (desc.iProduct > 0) {
                unsigned char desc_buffer[256] = {0};
                if (usb_device_get_string_descriptor_ascii(handle, desc.iProduct, desc_buffer, sizeof(desc_buffer)) > 0) {
                    strncpy(devices[device_count].description, (char*)desc_buffer, sizeof(devices[device_count].description) - 1);
                }
            }
            
            if (desc.iManufacturer > 0) {
                unsigned char manufacturer_buffer[256] = {0};
                if (usb_device_get_string_descriptor_ascii(handle, desc.iManufacturer, manufacturer_buffer, sizeof(manufacturer_buffer)) > 0) {
                    strncpy(devices[device_count].manufacturer, (char*)manufacturer_buffer, sizeof(devices[device_count].manufacturer) - 1);
                }
            }
            
            devices[device_count].device_id = device_count;
            
            usb_device_close(handle);
            
            device_count++;
        }
    }
    
    // 释放设备列表
    usb_device_free_device_list(device_list, 1);
    
    debug_printf("扫描到 %d 个USB设备", device_count);
    return device_count;
}

/**
 * @brief 打开USB设备
 */
int usb_middleware_open_device(const char* serial) {
    if (!g_initialized) {
        debug_printf("USB中间层未初始化");
        return USB_ERROR_OTHER;
    }
    
    int slot = -1;
    for (int i = 0; i < MAX_DEVICES; i++) {
        if (g_devices[i].state == DEVICE_STATE_CLOSED) {
            slot = i;
            break;
        }
    }
    
    if (slot == -1) {
        debug_printf("设备槽已满");
        return USB_ERROR_OTHER;
    }
    
    void** device_list = NULL;
    int cnt = usb_device_get_device_list(g_libusb_context, &device_list);
    if (cnt < 0) {
        debug_printf("获取设备列表失败: %d", cnt);
        return USB_ERROR_OTHER;
    }
    
    void* device_handle = NULL;
    
    for (int i = 0; i < cnt; i++) {
        void* dev = device_list[i];
        usb_device_descriptor desc;
        
        if (usb_device_get_device_descriptor(dev, &desc) != 0) {
            continue;
        }
        
        if (desc.idVendor == 0xCCDD && desc.idProduct == 0xAABB) {
            if (serial) {
                void* temp_handle = NULL;
                if (usb_device_open(dev, &temp_handle) == 0) {
                    unsigned char serial_buffer[64] = {0};
                    if (usb_device_get_string_descriptor_ascii(temp_handle, desc.iSerialNumber, serial_buffer, sizeof(serial_buffer)) > 0) {
                        if (strcmp((char*)serial_buffer, serial) == 0) {
                            // 找到目标设备
                            device_handle = temp_handle;
                            break;
                        }
                    }
                    usb_device_close(temp_handle);
                }
            } else {
                // 打开第一个可用设备
                if (usb_device_open(dev, &device_handle) == 0) {
                    break;
                }
            }
        }
    }
    
    usb_device_free_device_list(device_list, 1);
    
    if (!device_handle) {
        debug_printf("打开设备失败: %s", serial ? serial : "NULL");
        return USB_ERROR_ACCESS;
    }
    
    if (usb_device_claim_interface(device_handle, 0) != 0) {
        usb_device_close(device_handle);
        debug_printf("申请接口失败");
        return USB_ERROR_ACCESS;
    }
    
    g_devices[slot].libusb_handle = device_handle;
    g_devices[slot].state = DEVICE_STATE_OPEN;
    g_devices[slot].interface_claimed = 1;
    g_devices[slot].ref_count = 1;
    g_devices[slot].last_access = (unsigned int)time(NULL);
    g_devices[slot].device_id = g_next_device_id++;
    
    if (serial) {
        strncpy(g_devices[slot].serial, serial, sizeof(g_devices[slot].serial) - 1);
    } else {
        strcpy(g_devices[slot].serial, "UNKNOWN");
    }
    
    g_device_count++;
    
    ring_buffer_t* spi_rb = &g_devices[slot].protocol_buffers[PROTOCOL_SPI];
    spi_rb->size = SPI_BUFFER_SIZE;
    spi_rb->buffer = (unsigned char*)malloc(SPI_BUFFER_SIZE);
    spi_rb->write_pos = 0;
    spi_rb->read_pos = 0;
    spi_rb->data_size = 0;
    InitializeCriticalSection(&spi_rb->cs);
    
    ring_buffer_t* power_rb = &g_devices[slot].protocol_buffers[PROTOCOL_POWER];
    power_rb->size = POWER_BUFFER_SIZE;
    power_rb->buffer = (unsigned char*)malloc(POWER_BUFFER_SIZE);
    power_rb->write_pos = 0;
    power_rb->read_pos = 0;
    power_rb->data_size = 0;
    InitializeCriticalSection(&power_rb->cs);
    
    ring_buffer_t* raw_rb = &g_devices[slot].raw_buffer;
    raw_rb->size = RAW_BUFFER_SIZE;
    raw_rb->buffer = (unsigned char*)malloc(RAW_BUFFER_SIZE);
    raw_rb->write_pos = 0;
    raw_rb->read_pos = 0;
    raw_rb->data_size = 0;
    InitializeCriticalSection(&raw_rb->cs);
    
    g_devices[slot].stop_thread = FALSE;
    g_devices[slot].thread_running = TRUE;
    
    g_devices[slot].read_thread = CreateThread(NULL, 0, usb_device_read_thread_func, &g_devices[slot], 0, NULL);
    if (!g_devices[slot].read_thread) {
        DeleteCriticalSection(&spi_rb->cs);
        DeleteCriticalSection(&raw_rb->cs);
        free(spi_rb->buffer);
        free(raw_rb->buffer);
        usb_device_release_interface(device_handle, 0);
        usb_device_close(device_handle);
        g_devices[slot].state = DEVICE_STATE_CLOSED;
        g_devices[slot].libusb_handle = NULL;
        g_devices[slot].interface_claimed = 0;
        g_devices[slot].ref_count = 0;
        g_devices[slot].last_access = 0;
        g_devices[slot].device_id = -1;
        return USB_ERROR_OTHER;
    }
    
    debug_printf("成功打开设备: %s, 设备ID: %d", g_devices[slot].serial, g_devices[slot].device_id);
    return g_devices[slot].device_id;
}

/**
 * @brief 关闭USB设备
 */
int usb_middleware_close_device(int device_id) {
    debug_printf("开始关闭设备: %d", device_id);
    
    if (!g_initialized) {
        debug_printf("中间层未初始化，无法关闭设备: %d", device_id);
        return USB_ERROR_OTHER;
    }
    
    int slot = -1;
    for (int i = 0; i < MAX_DEVICES; i++) {
        if (g_devices[i].device_id == device_id && g_devices[i].state == DEVICE_STATE_OPEN) {
            slot = i;
            break;
        }
    }
    
    if (slot == -1) {
        debug_printf("设备未找到或未打开: %d", device_id);
        return USB_ERROR_NOT_FOUND;
    }
    
    debug_printf("找到设备槽位: %d, 序列号: %s", slot, g_devices[slot].serial);
    
    if (g_devices[slot].read_thread) {
        debug_printf("停止读取线程: 设备ID %d", device_id);
        g_devices[slot].stop_thread = TRUE;
        if (WaitForSingleObject(g_devices[slot].read_thread, 3000) == WAIT_TIMEOUT) {
            debug_printf("线程等待超时，强制终止: 设备ID %d", device_id);
            TerminateThread(g_devices[slot].read_thread, 0);
        }
        CloseHandle(g_devices[slot].read_thread);
        g_devices[slot].read_thread = NULL;
        debug_printf("线程已停止: 设备ID %d", device_id);
    } else {
        debug_printf("没有找到读取线程: 设备ID %d", device_id);
    }
    
    debug_printf("释放环形缓冲区: 设备ID %d", device_id);

    ring_buffer_t* spi_rb = &g_devices[slot].protocol_buffers[PROTOCOL_SPI];
    EnterCriticalSection(&spi_rb->cs);
    if (spi_rb->buffer) {
        free(spi_rb->buffer);
        spi_rb->buffer = NULL;
    }
    LeaveCriticalSection(&spi_rb->cs);
    DeleteCriticalSection(&spi_rb->cs);
    
    ring_buffer_t* power_rb = &g_devices[slot].protocol_buffers[PROTOCOL_POWER];
    EnterCriticalSection(&power_rb->cs);
    if (power_rb->buffer) {
        free(power_rb->buffer);
        power_rb->buffer = NULL;
    }
    LeaveCriticalSection(&power_rb->cs);
    DeleteCriticalSection(&power_rb->cs);
    
    ring_buffer_t* raw_rb = &g_devices[slot].raw_buffer;
    EnterCriticalSection(&raw_rb->cs);
    if (raw_rb->buffer) {
        free(raw_rb->buffer);
        raw_rb->buffer = NULL;
    }
    LeaveCriticalSection(&raw_rb->cs);
    DeleteCriticalSection(&raw_rb->cs);
    
    debug_printf("关闭设备句柄: 设备ID %d", device_id);
    usb_device_close(g_devices[slot].libusb_handle);
    
    memset(&g_devices[slot], 0, sizeof(device_handle_t));
    g_device_count--;
    
    debug_printf("成功关闭设备: %d", device_id);
    return USB_SUCCESS;
}

/**
 * @brief 根据序列号查找设备ID
 */
int usb_middleware_find_device_by_serial(const char* serial) {
    if (!g_initialized || !serial) {
        return -1;
    }
    
    for (int i = 0; i < MAX_DEVICES; i++) {
        if (g_devices[i].state == DEVICE_STATE_OPEN && 
            strcmp(g_devices[i].serial, serial) == 0) {
            return g_devices[i].device_id;
        }
    }
    
    return -1;
}

/**
 * @brief 检查设备是否已打开
 */
int usb_middleware_is_device_open(int device_id) {
    if (!g_initialized) {
        return 0;
    }
    
    for (int i = 0; i < MAX_DEVICES; i++) {
        if (g_devices[i].device_id == device_id) {
            return g_devices[i].state == DEVICE_STATE_OPEN ? 1 : 0;
        }
    }
    
    return 0;
}

/**
 * @brief 从USB设备读取数据
 */
int usb_middleware_read_data(int device_id, unsigned char* data, int length) {
    if (!g_initialized || !data || length <= 0) {
        return USB_ERROR_INVALID_PARAM;
    }
    
    int slot = -1;
    for (int i = 0; i < MAX_DEVICES; i++) {
        if (g_devices[i].device_id == device_id && g_devices[i].state == DEVICE_STATE_OPEN) {
            slot = i;
            break;
        }
    }
    
    if (slot == -1) {
        debug_printf("设备未找到或未打开: %d", device_id);
        return USB_ERROR_NOT_FOUND;
    }
    
    usb_middleware_update_device_access(device_id);
    ring_buffer_t* rb = &g_devices[slot].raw_buffer;
    EnterCriticalSection(&rb->cs);
    int available = rb->data_size;
    int to_read = (available < length) ? available : length;
    if (to_read > 0) {
        int start_pos;
        if (available <= length) {
            start_pos = rb->read_pos;
        } else {
            start_pos = (rb->write_pos - length + rb->size) % rb->size;
        }
        if (start_pos + to_read <= rb->size) {
            memcpy(data, rb->buffer + start_pos, to_read);
        } else {
            int first_part = rb->size - start_pos;
            memcpy(data, rb->buffer + start_pos, first_part);
            memcpy(data + first_part, rb->buffer, to_read - first_part);
        }
        if (available <= length) {
            rb->read_pos = rb->write_pos;
            rb->data_size = 0;
        }
    }
    LeaveCriticalSection(&rb->cs);
    
    return to_read;
}


int usb_middleware_write_data(int device_id, unsigned char* data, int length) {
    if (!g_initialized || !data || length <= 0) {
        return USB_ERROR_INVALID_PARAM;
    }
    
    int slot = -1;
    for (int i = 0; i < MAX_DEVICES; i++) {
        if (g_devices[i].device_id == device_id && g_devices[i].state == DEVICE_STATE_OPEN) {
            slot = i;
            break;
        }
    }
    
    if (slot == -1) {
        debug_printf("设备未找到或未打开: %d", device_id);
        return USB_ERROR_NOT_FOUND;
    }
    
    usb_middleware_update_device_access(device_id);
    
    int transferred = 0;
    int ret = usb_device_bulk_transfer(g_devices[slot].libusb_handle, 0x01, data, length, &transferred, 1000);
    if (ret < 0) {
        debug_printf("写入数据失败: %d", ret);
        return USB_ERROR_IO;
    }
    
    return transferred;
}

void usb_middleware_update_device_access(int device_id) {
    for (int i = 0; i < MAX_DEVICES; i++) {
        if (g_devices[i].device_id == device_id) {
            g_devices[i].last_access = (unsigned int)time(NULL);
            break;
        }
    }
}

/**
 * @brief 获取设备数量
 */
int usb_middleware_get_device_count(void) {
    return g_device_count;
}


int usb_middleware_read_spi_data(int device_id, unsigned char* data, int length) {
    if (!g_initialized || !data || length <= 0) {
        return USB_ERROR_INVALID_PARAM;
    }
    int slot = -1;
    for (int i = 0; i < MAX_DEVICES; i++) {
        if (g_devices[i].device_id == device_id && g_devices[i].state == DEVICE_STATE_OPEN) {
            slot = i;
            break;
        }
    }
    if (slot == -1) {
        debug_printf("设备未找到或未打开: %d", device_id);
        return USB_ERROR_NOT_FOUND;
    }
    usb_middleware_update_device_access(device_id);
    ring_buffer_t* spi_rb = &g_devices[slot].protocol_buffers[PROTOCOL_SPI];
    EnterCriticalSection(&spi_rb->cs);
    int available = spi_rb->data_size;
    int to_read = (available < length) ? available : length;
    if (to_read > 0) {
        if (spi_rb->read_pos + to_read <= spi_rb->size) {
            memcpy(data, spi_rb->buffer + spi_rb->read_pos, to_read);
        } else {
            int first_part = spi_rb->size - spi_rb->read_pos;
            memcpy(data, spi_rb->buffer + spi_rb->read_pos, first_part);
            memcpy(data + first_part, spi_rb->buffer, to_read - first_part);
        }
        spi_rb->read_pos = (spi_rb->read_pos + to_read) % spi_rb->size;
        spi_rb->data_size -= to_read;
    }
    LeaveCriticalSection(&spi_rb->cs);
    
    if (to_read > 0) {
        debug_printf("从SPI缓冲区读取了 %d 字节数据", to_read);
    }
    
    return to_read;
} 