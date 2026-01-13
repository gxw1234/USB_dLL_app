/**
 * @file usb_middleware.h
 * @brief USB中间层 - 统一设备管理和所有协议实现
 * 为应用层提供统一的设备管理和协议接口，所有协议实现都在这里
 */

#ifndef USB_MIDDLEWARE_H
#define USB_MIDDLEWARE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#ifdef _WIN32
#include <windows.h>
#else
#include "platform_compat.h"
#endif


#define PROTOCOL_SPI        0x01    // SPI协议
#define PROTOCOL_GPIO       0x04    // GPIO协议
#define PROTOCOL_POWER      0x05    // 电源协议
#define PROTOCOL_STATUS     0x09    // 状态响应协议
#define MAX_PROTOCOL_TYPES  10      // 最大协议类型数量(0=未使用, 1=SPI, 4=GPIO, 5=POWER, 9=STATUS)
typedef struct {
    unsigned char* buffer;     // 缓冲区指针
    unsigned int size;         // 缓冲区大小
    unsigned int write_pos;    // 写入位置
    unsigned int read_pos;     // 读取位置
    unsigned int data_size;    // 当前数据大小
    CRITICAL_SECTION cs;       // 临界区，用于线程同步
} ring_buffer_t;


typedef struct {
    char serial[64];           // 设备序列号
    char description[128];     // 设备描述
    char manufacturer[128];    // 制造商信息
    uint16_t vendor_id;        // 厂商ID
    uint16_t product_id;       // 产品ID
    int device_id;             // 设备ID
} device_info_t;

// 设备状态枚举
typedef enum {
    DEVICE_STATE_CLOSED = 0,   // 设备已关闭
    DEVICE_STATE_OPENING,      // 设备正在打开
    DEVICE_STATE_OPEN,         // 设备已打开
    DEVICE_STATE_ERROR         // 设备错误状态
} device_state_t;

typedef struct {
    char serial[64];           
    void* libusb_handle;       
    device_state_t state;     
    int interface_claimed;    
    unsigned int ref_count;    
    unsigned int last_access;  
    int device_id;             
    void* read_thread;        
    int thread_running;       
    int stop_thread;           
    unsigned char* rx_cache;
    unsigned int rx_cache_size;
    unsigned int rx_cache_capacity;
    ring_buffer_t protocol_buffers[MAX_PROTOCOL_TYPES]; 
    ring_buffer_t raw_buffer;  
    // GPIO电平缓存：按device_index存储最近一次读取的电平
    unsigned char gpio_level[256];
    unsigned char gpio_level_valid[256];
} device_handle_t;

// 错误代码定义
#define USB_SUCCESS             0    // 成功
#define USB_ERROR_NOT_FOUND    -1    // 设备未找到
#define USB_ERROR_ACCESS       -2    // 访问被拒绝
#define USB_ERROR_IO           -3    // I/O错误
#define USB_ERROR_INVALID_PARAM -4   // 参数无效
#define USB_ERROR_ALREADY_OPEN -5    // 设备已打开
#define USB_ERROR_NOT_OPEN     -6    // 设备未打开
#define USB_ERROR_TIMEOUT      -7    // 超时错误
#define USB_ERROR_BUSY         -8    // 设备忙碌
#define USB_ERROR_OTHER        -99   // 其他错误

// ==================== 设备管理接口 ====================

int usb_middleware_init(void);
void usb_middleware_cleanup(void);
int usb_middleware_scan_devices(device_info_t* devices, int max_devices);
int usb_middleware_open_device(const char* serial);
int usb_middleware_close_device(int device_id);
int usb_middleware_find_device_by_serial(const char* serial);
int usb_middleware_is_device_open(int device_id);

// ==================== 统一数据读写接口 ====================


int usb_middleware_read_data(int device_id, unsigned char* data, int length);


int usb_middleware_read_spi_data(int device_id, unsigned char* data, int length);

// UART数据读取函数
int usb_middleware_read_uart_data(int device_id, unsigned char* data, int length);

// UART缓冲区清除函数
int usb_middleware_clear_uart_buffer(int device_id);

// GPIO电平等待获取（从数组中取），timeout_ms毫秒
int usb_middleware_wait_gpio_level(int device_id, int gpio_index, unsigned char* level, int timeout_ms);

// 专用状态数据读取函数
int usb_middleware_read_status_data(int device_id, unsigned char* data, int length);

// 专用电源/电流数据读取函数
int usb_middleware_read_power_data(int device_id, unsigned char* data, int length);

// 专用PWM数据读取函数
int usb_middleware_read_pwm_data(int device_id, unsigned char* data, int length);

int usb_middleware_write_data(int device_id, unsigned char* data, int length);

// ==================== 内部工具函数 ====================


void parse_and_dispatch_protocol_data(device_handle_t* device, unsigned char* raw_data, int length);


void write_to_ring_buffer(ring_buffer_t* rb, unsigned char* data, int length);

void usb_middleware_update_device_access(int device_id);


int usb_middleware_get_device_count(void);

#ifdef __cplusplus
}
#endif

#endif // USB_MIDDLEWARE_H 