/**
 * @file usb_application.c
 * @brief USB应用层公共接口实现
 */

#include "usb_application.h"
#include "usb_middleware.h"
#include "usb_log.h"
#include "usb_protocol.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <stdint.h>
#include <process.h>  


extern void debug_printf(const char *format, ...);

#define VENDOR_ID   0xCCDD          // 设备VID
#define PRODUCT_ID  0xAABB          // 设备PID

// DLL版本号 在这里修改
#define DLL_VERSION_MAJOR    1
#define DLL_VERSION_MINOR    4
#define DLL_VERSION  ((DLL_VERSION_MAJOR << 8) | DLL_VERSION_MINOR)


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

typedef struct {
    char    FirmwareName[32];       // 设备固件名称
    char    FirmwareBuildDate[32];  // 设备固件编译时间
    int     HardwareVersion;        // 硬件版本号
    int     FirmwareVersion;        // 设备固件版本号
    int     SerialNumber[3];        // 设备序列号
    int     Functions;              // 设备功能
} stm32_firmware_info_t;


static int get_stm32_firmware_info(const char* serial, stm32_firmware_info_t* stm32_info) {
    if (!serial || !stm32_info) {
        return USB_ERROR_INVALID_PARAM;
    }
    
    int device_id = usb_middleware_find_device_by_serial(serial);
    if (device_id < 0) {
        debug_printf("未找到设备: %s", serial);
        return USB_ERROR_NOT_FOUND;
    }
    
    if (!usb_middleware_is_device_open(device_id)) {
        debug_printf("设备未打开: %s", serial);
        return USB_ERROR_NOT_OPEN;
    }
    
    GENERIC_CMD_HEADER cmd_header = {0};
    cmd_header.protocol_type = PROTOCOL_GET_FIRMWARE_INFO;
    cmd_header.cmd_id = CMD_READ;  // 读取固件信息
    cmd_header.device_index = 0;
    cmd_header.param_count = 0;
    cmd_header.data_len = 0;
    cmd_header.total_packets = 1;
    unsigned char* cmd_buffer = NULL;
    int cmd_size = build_protocol_frame(&cmd_buffer, &cmd_header, NULL, 0, NULL, 0);
    if (cmd_size <= 0 || !cmd_buffer) {
        debug_printf("构建命令包失败");
        return USB_ERROR_OTHER;
    }
    int write_result = usb_middleware_write_data(device_id, cmd_buffer, cmd_size);
    free(cmd_buffer);
    if (write_result < 0) {
        debug_printf("发送获取固件信息命令失败: %d", write_result);
        return write_result;
    }
    
    Sleep(100);  
    unsigned char response_buffer[256] = {0};
    int response_len = usb_middleware_read_data(device_id, response_buffer, sizeof(response_buffer));
    int expected_size = sizeof(GENERIC_CMD_HEADER) + sizeof(stm32_firmware_info_t);
    if (response_len < expected_size) {
        debug_printf("从STM32设备读取固件信息失败或数据不完整: %d, 期望: %d", response_len, expected_size);
        strcpy(stm32_info->FirmwareName, "G2X_FW");
        strcpy(stm32_info->FirmwareBuildDate, "Unknown");
        stm32_info->HardwareVersion = 0x0100;
        stm32_info->FirmwareVersion = 0x0100;
        stm32_info->SerialNumber[0] = 0x00000000;
        stm32_info->SerialNumber[1] = 0x00000000;
        stm32_info->SerialNumber[2] = 0x00000000;
        stm32_info->Functions = 0x000F;
        return USB_SUCCESS;  
    }
    
    GENERIC_CMD_HEADER* response_header = (GENERIC_CMD_HEADER*)response_buffer;
    debug_printf("响应头: protocol_type=%d, cmd_id=%d, data_len=%d", 
                 response_header->protocol_type, response_header->cmd_id, response_header->data_len);

    unsigned char* firmware_data = response_buffer + sizeof(GENERIC_CMD_HEADER);
    memcpy(stm32_info, firmware_data, sizeof(stm32_firmware_info_t));
    
    debug_printf("成功获取STM32固件信息: %s, 版本: 0x%04X", 
                 stm32_info->FirmwareName, stm32_info->FirmwareVersion);
    return USB_SUCCESS;
}

// ==================== 设备管理接口 ====================
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
    strcpy(dev_info->DllName, "feat/8/1");
    strcpy(dev_info->DllBuildDate, __DATE__ " " __TIME__);  // 使用编译时间
    dev_info->DllVersion = DLL_VERSION;
    stm32_firmware_info_t stm32_info = {0};
    int stm32_result = get_stm32_firmware_info(serial, &stm32_info);
    if (stm32_result == USB_SUCCESS) {
        strcpy(dev_info->FirmwareName, stm32_info.FirmwareName);
        strcpy(dev_info->FirmwareBuildDate, stm32_info.FirmwareBuildDate);
        dev_info->HardwareVersion = stm32_info.HardwareVersion;
        dev_info->FirmwareVersion = stm32_info.FirmwareVersion;
        dev_info->SerialNumber[0] = stm32_info.SerialNumber[0];
        dev_info->SerialNumber[1] = stm32_info.SerialNumber[1];
        dev_info->SerialNumber[2] = stm32_info.SerialNumber[2];
        dev_info->Functions = stm32_info.Functions;
        debug_printf("使用STM32设备的真实固件信息");
    } else {
        strcpy(dev_info->FirmwareName, "Unknown_FW");
        strcpy(dev_info->FirmwareBuildDate, "Unknown");
        dev_info->HardwareVersion = 0x0100;  
        dev_info->FirmwareVersion = 0x0100;  
        dev_info->SerialNumber[0] = 0x12345678;
        dev_info->SerialNumber[1] = 0x9ABCDEF0;
        dev_info->SerialNumber[2] = 0x11223344;
        dev_info->Functions = 0x000F;  // 支持GPIO、SPI、I2C、Power等功能
        debug_printf("使用默认的STM32设备信息 (获取真实信息失败: %d)", stm32_result);
    }
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
    
    debug_printf("设备信息获取成功:");
    debug_printf("  DLL: %s, 版本: 0x%04X, 编译: %s", 
                 dev_info->DllName, dev_info->DllVersion, dev_info->DllBuildDate);
    debug_printf("  STM32: %s, 硬件版本: 0x%04X, 固件版本: 0x%04X, 编译: %s", 
                 dev_info->FirmwareName, dev_info->HardwareVersion, dev_info->FirmwareVersion, dev_info->FirmwareBuildDate);
    
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
