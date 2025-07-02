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
#include <windows.h>

// 环形缓冲区结构
typedef struct {
    unsigned char* buffer;     // 缓冲区指针
    unsigned int size;         // 缓冲区大小
    unsigned int write_pos;    // 写入位置
    unsigned int read_pos;     // 读取位置
    unsigned int data_size;    // 当前数据大小
    CRITICAL_SECTION cs;       // 临界区，用于线程同步
} ring_buffer_t;

// 设备信息结构（从应用层移到这里，避免循环依赖）
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

// 设备句柄结构
typedef struct {
    char serial[64];           // 设备序列号
    void* libusb_handle;       // libusb设备句柄
    device_state_t state;      // 设备状态
    int interface_claimed;     // 接口是否已申请
    unsigned int ref_count;    // 引用计数
    unsigned int last_access;  // 最后访问时间
    int device_id;             // 设备ID
    // 新增：后台读取线程和环形缓冲区
    void* read_thread;         // 读取线程句柄（使用void*避免Windows类型依赖）
    int thread_running;        // 线程运行标志
    int stop_thread;           // 线程停止请求标志
    ring_buffer_t ring_buffer; // 环形缓冲区
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
#define USB_ERROR_OTHER        -99   // 其他错误

// ==================== 设备管理接口 ====================

/**
 * @brief 初始化USB中间层
 * @return int 成功返回0，失败返回错误代码
 */
int usb_middleware_init(void);

/**
 * @brief 清理USB中间层
 */
void usb_middleware_cleanup(void);

/**
 * @brief 扫描USB设备
 * @param devices 设备信息数组
 * @param max_devices 最大设备数
 * @return int 扫描到的设备数量
 */
int usb_middleware_scan_devices(device_info_t* devices, int max_devices);

/**
 * @brief 打开USB设备
 * @param serial 设备序列号，NULL表示打开第一个可用设备
 * @return int 成功返回设备ID(≥0)，失败返回错误代码
 */
int usb_middleware_open_device(const char* serial);

/**
 * @brief 关闭USB设备
 * @param device_id 设备ID
 * @return int 成功返回0，失败返回错误代码
 */
int usb_middleware_close_device(int device_id);

/**
 * @brief 根据序列号查找设备ID
 * @param serial 设备序列号
 * @return int 设备ID，未找到返回-1
 */
int usb_middleware_find_device_by_serial(const char* serial);

/**
 * @brief 检查设备是否已打开
 * @param device_id 设备ID
 * @return int 1=已打开，0=未打开，<0=错误
 */
int usb_middleware_is_device_open(int device_id);

// ==================== 统一数据读写接口 ====================

/**
 * @brief 从USB设备读取数据
 * @param device_id 设备ID
 * @param data 数据缓冲区
 * @param length 要读取的数据长度
 * @return int 实际读取的数据长度，小于0表示错误
 */
int usb_middleware_read_data(int device_id, unsigned char* data, int length);

/**
 * @brief 向USB设备发送数据
 * @param device_id 设备ID
 * @param data 要发送的数据缓冲区
 * @param length 要发送的数据长度
 * @return int 实际发送的数据长度，小于0表示错误
 */
int usb_middleware_write_data(int device_id, unsigned char* data, int length);

// ==================== 内部工具函数 ====================

/**
 * @brief 更新设备访问时间
 * @param device_id 设备ID
 */
void usb_middleware_update_device_access(int device_id);

/**
 * @brief 获取设备数量
 * @return int 当前管理的设备数量
 */
int usb_middleware_get_device_count(void);

#ifdef __cplusplus
}
#endif

#endif // USB_MIDDLEWARE_H 