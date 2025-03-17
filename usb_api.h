#ifndef USB_API_H
#define USB_API_H

#ifdef __cplusplus
extern "C" {
#endif

// 动态库导出宏定义
#ifdef _WIN32
    #ifdef USB_API_EXPORTS
        #define WINAPI __declspec(dllexport) __stdcall
    #else
        #define WINAPI __declspec(dllimport) __stdcall
    #endif
#else
    #define WINAPI
#endif

// 设备信息结构体
typedef struct {
    char serial[64];           // 设备序列号
    unsigned short vendor_id;  // 厂商ID
    unsigned short product_id; // 产品ID
    char description[256];     // 设备描述
    char manufacturer[256];    // 制造商
    char interface_str[256];   // 接口（改名为interface_str避免关键字冲突）
} device_info_t;

/**
 * @brief 扫描连接的USB设备
 * 
 * @param devices 用于存储发现的设备信息的数组
 * @param max_devices 数组的最大容量
 * @return int 成功扫描到的设备数量，小于0表示错误
 */
WINAPI int USB_ScanDevice(device_info_t* devices, int max_devices);

/**
 * @brief 打开指定序列号的USB设备
 * 
 * @param DevHandle 目标设备的序列号，如果为NULL则打开第一个可用设备
 * @return int 成功返回设备句柄(≥0)，失败返回错误代码(<0)
 */
WINAPI int USB_OpenDevice(const char* DevHandle);

/**
 * @brief 关闭USB设备
 * 
 * @param DevHandle 目标设备的序列号
 * @return int 成功返回0，失败返回错误代码
 */
WINAPI int USB_CloseDevice(const char* DevHandle);

/**
 * @brief 从USB设备读取数据
 * 
 * @param target_serial 目标设备的序列号
 * @param data 数据缓冲区
 * @param length 要读取的数据长度
 * @return int 实际读取的数据长度，小于0表示错误
 */
WINAPI int USB_ReadData(const char* target_serial, unsigned char* data, int length);

// 错误代码定义
#define USB_SUCCESS             0    // 成功
#define USB_ERROR_NOT_FOUND    -1    // 设备未找到
#define USB_ERROR_ACCESS       -2    // 访问被拒绝
#define USB_ERROR_IO           -3    // I/O错误
#define USB_ERROR_INVALID_PARAM -4   // 参数无效
#define USB_ERROR_OTHER        -99   // 其他错误

#ifdef __cplusplus
}
#endif

#endif // USB_API_H
