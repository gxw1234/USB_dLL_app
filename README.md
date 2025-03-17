# USB 设备通信 API 文档

## 概述

这是一个 USB 设备通信 API，封装了 libusb 操作，提供简单易用的接口用于 USB 设备扫描、打开、关闭和数据读取。

## 特性

- 设备管理：扫描、打开和关闭 USB 设备
- 多设备支持：通过序列号识别和操作多个相同 VID/PID 的设备
- 数据读取：读取 USB 设备数据
- 异步处理：每个设备有独立的读取线程
- 缓冲区管理：使用环形缓冲区存储最新数据
- 调试功能：可配置的日志记录系统

## 调试开关

在 `usb_device.h` 中设置：

```c
// 调试开关，
#define USB_DEBUG_ENABLE 0  // 开启日志
// 或
#define USB_DEBUG_ENABLE 1  // 关闭日志
```

## API 函数

### 扫描设备


int USB_ScanDevice(device_info_t* devices, int max_devices);


### 打开设备


int USB_OpenDevice(const char* target_serial);


### 关闭设备


int USB_CloseDevice(const char* target_serial);


### 读取数据


int USB_ReadData(const char* target_serial, unsigned char* data, int length);


## 错误码

- `USB_SUCCESS (0)`: 操作成功
- `USB_ERROR_NOT_FOUND (-1)`: 设备未找到
- `USB_ERROR_ACCESS (-2)`: 访问被拒绝
- `USB_ERROR_IO (-3)`: I/O 错误
- `USB_ERROR_INVALID_PARAM (-4)`: 参数无效
- `USB_ERROR_OTHER (-99)`: 其他错误

## 编译说明

使用 `build_dll.bat` 脚本编译生成 DLL 文件：

```
.\build_dll.bat
```

## 依赖

- libusb-1.0.dll: 需要放在程序目录下或系统路径中

