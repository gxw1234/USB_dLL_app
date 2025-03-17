/**
 * @file usb_device.h
 * @brief USB设备底层操作
 */

#ifndef USB_DEVICE_H
#define USB_DEVICE_H

#include <windows.h>
#include <stdint.h>

// libusb相关定义
#define LIBUSB_SUCCESS         0
#define LIBUSB_ERROR_IO       -1
#define LIBUSB_ERROR_TIMEOUT  -7
#define LIBUSB_ERROR_NOT_FOUND -5

// 错误代码定义
#define USB_SUCCESS            0    // 成功
#define USB_ERROR_NOT_FOUND   -1    // 设备未找到
#define USB_ERROR_ACCESS      -2    // 访问被拒绝
#define USB_ERROR_IO          -3    // I/O错误
#define USB_ERROR_INVALID_PARAM -4  // 参数无效
#define USB_ERROR_OTHER       -99   // 其他错误

// 设备描述符结构体
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

// 设备相关函数声明
int usb_device_init(void);
void usb_device_cleanup(void);
int usb_device_get_device_list(void* ctx, void*** device_list);
void usb_device_free_device_list(void** list, int unref_devices);
int usb_device_get_device_descriptor(void* dev, usb_device_descriptor* desc);
int usb_device_open(void* dev, void** handle);
void usb_device_close(void* handle);
int usb_device_get_string_descriptor_ascii(void* handle, uint8_t desc_index, unsigned char* data, int length);
int usb_device_claim_interface(void* handle, int interface_number);
int usb_device_release_interface(void* handle, int interface_number);
int usb_device_bulk_transfer(void* handle, unsigned char endpoint, unsigned char* data, int length, int* transferred, unsigned int timeout);
void* usb_device_get_device(void* handle);
void* usb_device_open_device_with_vid_pid(void* ctx, unsigned short vid, unsigned short pid);

// libusb上下文管理
extern void* g_libusb_context;

#endif // USB_DEVICE_H
