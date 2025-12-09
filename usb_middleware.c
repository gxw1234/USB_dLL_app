#include "usb_middleware.h"
#include "usb_device.h"
#include "usb_log.h"
#include "usb_protocol.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#else
#include "platform_compat.h"
#endif
#include <time.h>
#include "usb_log.h"

#define MAX_DEVICES 10
#define SPI_BUFFER_SIZE (10 * 1024 * 1024)    // SPI专用缓冲区
#define POWER_BUFFER_SIZE (512 * 1024)       // 电源数据缓冲区 (增大以适应电流数据)
#define PWM_BUFFER_SIZE (4 * 1024)           // PWM专用缓冲区
#define UART_BUFFER_SIZE (64 * 1024)         // UART专用缓冲区
#define RAW_BUFFER_SIZE (512 * 1024)         //  原始数据临时缓冲区
#define STATUS_BUFFER_SIZE (16 * 1024)       // 状态响应缓冲区

static device_handle_t g_devices[MAX_DEVICES];
static int g_device_count = 0;
static int g_next_device_id = 0;
static int g_initialized = 0;

static DWORD WINAPI usb_device_read_thread_func(LPVOID lpParameter) {
    device_handle_t* device = (device_handle_t*)lpParameter;
    unsigned char temp_buffer[8192];  // 增加缓冲区大小以适应大数据包
    while (!device->stop_thread) {
        int actual_length = 0;
        int ret = usb_device_bulk_transfer(device->libusb_handle, 0x81, temp_buffer, sizeof(temp_buffer), &actual_length, 1000);
        if (ret == 0 && actual_length > 0) {
            parse_and_dispatch_protocol_data(device, temp_buffer, actual_length);
        } else if (ret == -7) {
            // debug_printf("读取超时");
            // Sleep(1);
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
        //检查是否有足够的字节来读取完整的协议头
        if (pos + sizeof(GENERIC_CMD_HEADER) > length) {

            EnterCriticalSection(&device->raw_buffer.cs);
            write_to_ring_buffer(&device->raw_buffer, raw_data + pos, length - pos);
            LeaveCriticalSection(&device->raw_buffer.cs);
            break;
        }
        
        GENERIC_CMD_HEADER* header = (GENERIC_CMD_HEADER*)(raw_data + pos);
        int packet_size = sizeof(GENERIC_CMD_HEADER) + header->data_len;

        // 检查是否有足够的字节来读取完整的数据包
        if (pos + packet_size > length) {

            EnterCriticalSection(&device->raw_buffer.cs);
            write_to_ring_buffer(&device->raw_buffer, raw_data + pos, length - pos);
            LeaveCriticalSection(&device->raw_buffer.cs);
            break;
        }
        //检查是否有足够的字节来读取完整的数据包
        
        if (header->protocol_type == PROTOCOL_SPI) {

            unsigned char* spi_data = raw_data + pos + sizeof(GENERIC_CMD_HEADER);
            int spi_data_len = header->data_len;
            //获取互斥锁，确保只有一个线程能访问共享资源
            EnterCriticalSection(&device->protocol_buffers[PROTOCOL_SPI].cs);
            //将SPI数据写入到专用的环形缓冲区中
            write_to_ring_buffer(&device->protocol_buffers[PROTOCOL_SPI], spi_data, spi_data_len);
            //释放互斥锁，允许其他等待的线程访问共享资源
            LeaveCriticalSection(&device->protocol_buffers[PROTOCOL_SPI].cs);
            
            // debug_printf("分发SPI数据: %d字节, cmd_id=%d, device_index=%d", spi_data_len, header->cmd_id, header->device_index);
        } else if (header->protocol_type == PROTOCOL_STATUS) {
            // 处理状态响应数据
            unsigned char* status_data = raw_data + pos;  // 包含完整的协议头
            int status_data_len = packet_size;
            
            debug_printf("收到状态响应: protocol_type=%d, cmd_id=%d, device_index=%d, data_len=%d", 
                        header->protocol_type, header->cmd_id, header->device_index, status_data_len);
            
            EnterCriticalSection(&device->protocol_buffers[PROTOCOL_STATUS].cs);
            write_to_ring_buffer(&device->protocol_buffers[PROTOCOL_STATUS], status_data, status_data_len);
            LeaveCriticalSection(&device->protocol_buffers[PROTOCOL_STATUS].cs);
            
            // debug_printf("分发状态数据: %d字节, cmd_id=%d, device_index=%d", status_data_len, header->cmd_id, header->device_index);
        } else if (header->protocol_type == PROTOCOL_PWM) {
            // 处理PWM响应数据
            unsigned char* pwm_data = raw_data + pos;  // 包含完整的协议头
            int pwm_data_len = packet_size;
            
            debug_printf("收到PWM响应: protocol_type=%d, cmd_id=%d, device_index=%d, data_len=%d", 
                        header->protocol_type, header->cmd_id, header->device_index, pwm_data_len);
            
            EnterCriticalSection(&device->protocol_buffers[PROTOCOL_PWM].cs);
            write_to_ring_buffer(&device->protocol_buffers[PROTOCOL_PWM], pwm_data, pwm_data_len);
            LeaveCriticalSection(&device->protocol_buffers[PROTOCOL_PWM].cs);
            
            // debug_printf("分发PWM数据: %d字节, cmd_id=%d, device_index=%d", pwm_data_len, header->cmd_id, header->device_index);
        } else if (header->protocol_type == PROTOCOL_UART) {
            // 处理UART响应数据
            unsigned char* uart_data = raw_data + pos + sizeof(GENERIC_CMD_HEADER);
            int uart_data_len = header->data_len;
            
            debug_printf("收到UART数据: protocol_type=%d, cmd_id=%d, device_index=%d, data_len=%d", 
                        header->protocol_type, header->cmd_id, header->device_index, uart_data_len);
            
            EnterCriticalSection(&device->protocol_buffers[PROTOCOL_UART].cs);
            write_to_ring_buffer(&device->protocol_buffers[PROTOCOL_UART], uart_data, uart_data_len);
            LeaveCriticalSection(&device->protocol_buffers[PROTOCOL_UART].cs);
            
            debug_printf("分发UART数据: %d字节, cmd_id=%d, device_index=%d", uart_data_len, header->cmd_id, header->device_index);
        } else if (header->protocol_type == PROTOCOL_GPIO) {
            if (header->cmd_id == GPIO_DIR_READ && header->data_len >= 1) {
                unsigned char level = *(raw_data + pos + sizeof(GENERIC_CMD_HEADER));
                unsigned int idx = header->device_index;
                if (idx < 256) {
                    device->gpio_level[idx] = level;
                    device->gpio_level_valid[idx] = 1;
                }
            }
        } else if (header->protocol_type == PROTOCOL_GET_FIRMWARE_INFO) {
            // 处理固件信息响应数据
            unsigned char* firmware_data = raw_data + pos;  // 包含完整的协议头和数据
            int firmware_data_len = packet_size;
            
            debug_printf("收到固件信息响应: protocol_type=%d, cmd_id=%d, device_index=%d, data_len=%d", 
                        header->protocol_type, header->cmd_id, header->device_index, firmware_data_len);
            
            // 将固件信息数据写入原始缓冲区，供应用层读取
            EnterCriticalSection(&device->raw_buffer.cs);
            write_to_ring_buffer(&device->raw_buffer, firmware_data, firmware_data_len);
            LeaveCriticalSection(&device->raw_buffer.cs);
            
            debug_printf("分发固件信息数据: %d字节, cmd_id=%d, device_index=%d", firmware_data_len, header->cmd_id, header->device_index);
        } else if (header->protocol_type == PROTOCOL_CURRENT) {
            // 处理电流数据
            unsigned char* current_data = raw_data + pos + sizeof(GENERIC_CMD_HEADER);
            int current_data_len = header->data_len;
            
            debug_printf("收到电流数据: protocol_type=%d, cmd_id=%d, device_index=%d, data_len=%d", 
                        header->protocol_type, header->cmd_id, header->device_index, current_data_len);
            
            // 将电流数据写入POWER缓冲区（复用POWER协议缓冲区）
            EnterCriticalSection(&device->protocol_buffers[PROTOCOL_POWER].cs);
            int before_size = device->protocol_buffers[PROTOCOL_POWER].data_size;
            write_to_ring_buffer(&device->protocol_buffers[PROTOCOL_POWER], current_data, current_data_len);
            int after_size = device->protocol_buffers[PROTOCOL_POWER].data_size;
            LeaveCriticalSection(&device->protocol_buffers[PROTOCOL_POWER].cs);
            
            debug_printf("分发电流数据: %d字节, cmd_id=%d, device_index=%d, 缓冲区: %d->%d", 
                        current_data_len, header->cmd_id, header->device_index, before_size, after_size);
        } else {
            debug_printf("收到非SPI协议数据: protocol_type=%d, cmd_id=%d", header->protocol_type, header->cmd_id);
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
            memset(&devices[device_count], 0, sizeof(device_info_t));
            int found_in_opened = 0;
            for (int j = 0; j < MAX_DEVICES; j++) {
                if (g_devices[j].state == DEVICE_STATE_OPEN && g_devices[j].libusb_handle) {
                    void* opened_dev = usb_device_get_device(g_devices[j].libusb_handle);
                    if (opened_dev == dev) {
                        strncpy(devices[device_count].serial, g_devices[j].serial, sizeof(devices[device_count].serial) - 1);
                        strncpy(devices[device_count].description, "USB Device (Open)", sizeof(devices[device_count].description) - 1);
                        strncpy(devices[device_count].manufacturer, "USB Device (Open)", sizeof(devices[device_count].manufacturer) - 1);
                        found_in_opened = 1;
                        debug_printf("找到已打开设备，序列号: %s", g_devices[j].serial);
                        break;
                    }
                }
            }
            
            if (!found_in_opened) {
                void* handle = NULL;
                int open_result = usb_device_open(dev, &handle);
                if (open_result == 0 && handle != NULL) {
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
                    
                    usb_device_close(handle);
                } else {
                    snprintf(devices[device_count].serial, sizeof(devices[device_count].serial), "DEVICE_%04X_%04X", desc.idVendor, desc.idProduct);
                    strncpy(devices[device_count].description, "USB Device (Access Denied)", sizeof(devices[device_count].description) - 1);
                    strncpy(devices[device_count].manufacturer, "Unknown", sizeof(devices[device_count].manufacturer) - 1);
                }
            }
            devices[device_count].vendor_id = desc.idVendor;
            devices[device_count].product_id = desc.idProduct;
            devices[device_count].device_id = device_count;
            device_count++;
        }
    }
    usb_device_free_device_list(device_list, 1);
    debug_printf("扫描到 %d 个USB设备", device_count);
    return device_count;
}

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
    
    ring_buffer_t* pwm_rb = &g_devices[slot].protocol_buffers[PROTOCOL_PWM];
    pwm_rb->size = PWM_BUFFER_SIZE;
    pwm_rb->buffer = (unsigned char*)malloc(PWM_BUFFER_SIZE);
    pwm_rb->write_pos = 0;
    pwm_rb->read_pos = 0;
    pwm_rb->data_size = 0;
    InitializeCriticalSection(&pwm_rb->cs);
    
    ring_buffer_t* uart_rb = &g_devices[slot].protocol_buffers[PROTOCOL_UART];
    uart_rb->size = UART_BUFFER_SIZE;
    uart_rb->buffer = (unsigned char*)malloc(UART_BUFFER_SIZE);
    uart_rb->write_pos = 0;
    uart_rb->read_pos = 0;
    uart_rb->data_size = 0;
    InitializeCriticalSection(&uart_rb->cs);
    
    ring_buffer_t* status_rb = &g_devices[slot].protocol_buffers[PROTOCOL_STATUS];
    status_rb->size = STATUS_BUFFER_SIZE;
    status_rb->buffer = (unsigned char*)malloc(STATUS_BUFFER_SIZE);
    status_rb->write_pos = 0;
    status_rb->read_pos = 0;
    status_rb->data_size = 0;
    InitializeCriticalSection(&status_rb->cs);
    
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
    
    ring_buffer_t* pwm_rb = &g_devices[slot].protocol_buffers[PROTOCOL_PWM];
    EnterCriticalSection(&pwm_rb->cs);
    if (pwm_rb->buffer) {
        free(pwm_rb->buffer);
        pwm_rb->buffer = NULL;
    }
    LeaveCriticalSection(&pwm_rb->cs);
    DeleteCriticalSection(&pwm_rb->cs);
    
    ring_buffer_t* uart_rb = &g_devices[slot].protocol_buffers[PROTOCOL_UART];
    EnterCriticalSection(&uart_rb->cs);
    if (uart_rb->buffer) {
        free(uart_rb->buffer);
        uart_rb->buffer = NULL;
    }
    LeaveCriticalSection(&uart_rb->cs);
    DeleteCriticalSection(&uart_rb->cs);
    
    ring_buffer_t* status_rb = &g_devices[slot].protocol_buffers[PROTOCOL_STATUS];
    EnterCriticalSection(&status_rb->cs);
    if (status_rb->buffer) {
        free(status_rb->buffer);
        status_rb->buffer = NULL;
    }
    LeaveCriticalSection(&status_rb->cs);
    DeleteCriticalSection(&status_rb->cs);
    
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
    
//     if (to_read > 0) {
//         debug_printf("从SPI缓冲区读取了 %d 字节数据", to_read);
//     }
    
    return to_read;
}

int usb_middleware_read_status_data(int device_id, unsigned char* data, int length) {
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
    ring_buffer_t* status_rb = &g_devices[slot].protocol_buffers[PROTOCOL_STATUS];
    
    EnterCriticalSection(&status_rb->cs);
    int available = status_rb->data_size;
    int to_read = (available < length) ? available : length;
    
    if (to_read > 0) {
        if (status_rb->read_pos + to_read <= status_rb->size) {
            memcpy(data, status_rb->buffer + status_rb->read_pos, to_read);
        } else {
            int first_part = status_rb->size - status_rb->read_pos;
            memcpy(data, status_rb->buffer + status_rb->read_pos, first_part);
            memcpy(data + first_part, status_rb->buffer, to_read - first_part);
        }
        status_rb->read_pos = (status_rb->read_pos + to_read) % status_rb->size;
        status_rb->data_size -= to_read;
    }
    
    LeaveCriticalSection(&status_rb->cs);
    return to_read;
}

int usb_middleware_read_uart_data(int device_id, unsigned char* data, int length) {
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
    ring_buffer_t* uart_rb = &g_devices[slot].protocol_buffers[PROTOCOL_UART];
    EnterCriticalSection(&uart_rb->cs);
    int available = uart_rb->data_size;
    int to_read = (available < length) ? available : length;
    if (to_read > 0) {
        if (uart_rb->read_pos + to_read <= uart_rb->size) {
            memcpy(data, uart_rb->buffer + uart_rb->read_pos, to_read);
        } else {
            int first_part = uart_rb->size - uart_rb->read_pos;
            memcpy(data, uart_rb->buffer + uart_rb->read_pos, first_part);
            memcpy(data + first_part, uart_rb->buffer, to_read - first_part);
        }
        uart_rb->read_pos = (uart_rb->read_pos + to_read) % uart_rb->size;
        uart_rb->data_size -= to_read;
    }
    LeaveCriticalSection(&uart_rb->cs);
    
    return to_read;
}

int usb_middleware_wait_gpio_level(int device_id, int gpio_index, unsigned char* level, int timeout_ms) {
    if (!g_initialized || !level || gpio_index < 0 || gpio_index >= 256) {
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
    int waited = 0;
    while (waited <= timeout_ms) {
        if (g_devices[slot].gpio_level_valid[gpio_index]) {
            *level = g_devices[slot].gpio_level[gpio_index];
            g_devices[slot].gpio_level_valid[gpio_index] = 0;
            return USB_SUCCESS;
        }
        Sleep(1);
        waited += 1;
    }
    return USB_ERROR_TIMEOUT;
}

int usb_middleware_read_power_data(int device_id, unsigned char* data, int length) {
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
    ring_buffer_t* power_rb = &g_devices[slot].protocol_buffers[PROTOCOL_POWER];
    
    EnterCriticalSection(&power_rb->cs);
    int available = power_rb->data_size;
    int to_read = (available < length) ? available : length;
    debug_printf("读取电流数据: 可用=%d字节, 请求=%d字节, 实际读取=%d字节", available, length, to_read);
    
    if (to_read > 0) {
        if (power_rb->read_pos + to_read <= power_rb->size) {
            memcpy(data, power_rb->buffer + power_rb->read_pos, to_read);
        } else {
            int first_part = power_rb->size - power_rb->read_pos;
            memcpy(data, power_rb->buffer + power_rb->read_pos, first_part);
            memcpy(data + first_part, power_rb->buffer, to_read - first_part);
        }
        // 更新读取位置，实现环形缓冲区
        power_rb->read_pos = (power_rb->read_pos + to_read) % power_rb->size;
        power_rb->data_size -= to_read;
    }
    
    LeaveCriticalSection(&power_rb->cs);
    return to_read;
}

int usb_middleware_read_pwm_data(int device_id, unsigned char* data, int length) {
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
    ring_buffer_t* pwm_rb = &g_devices[slot].protocol_buffers[PROTOCOL_PWM];
    
    EnterCriticalSection(&pwm_rb->cs);
    int available = pwm_rb->data_size;
    int to_read = (available < length) ? available : length;
    
    if (to_read > 0) {
        if (pwm_rb->read_pos + to_read <= pwm_rb->size) {
            memcpy(data, pwm_rb->buffer + pwm_rb->read_pos, to_read);
        } else {
            int first_part = pwm_rb->size - pwm_rb->read_pos;
            memcpy(data, pwm_rb->buffer + pwm_rb->read_pos, first_part);
            memcpy(data + first_part, pwm_rb->buffer, to_read - first_part);
        }
        pwm_rb->read_pos = (pwm_rb->read_pos + to_read) % pwm_rb->size;
        pwm_rb->data_size -= to_read;
    }
    
    LeaveCriticalSection(&pwm_rb->cs);
    return to_read;
}
