#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
GPIO功能测试脚本
"""
from PIL import Image
import os
import ctypes
import time
from ctypes import c_int, c_char, c_char_p, c_ubyte, c_ushort, c_uint, byref, Structure, POINTER, create_string_buffer
from ctypes import *

# 定义设备信息结构体
class DeviceInfo(Structure):
    _fields_ = [
        ("serial", c_char * 64),       # 设备序列号
        ("vendor_id", c_ushort),       # 厂商ID
        ("product_id", c_ushort),      # 产品ID
        ("description", c_char * 256), # 设备描述
        ("manufacturer", c_char * 256),# 制造商
        ("interface_str", c_char * 256)# 接口
    ]

# 定义GPIO端口
GPIO_PORT0 = 0x00    # GPIO端口0 (对应STM32的PH7引脚)
GPIO_PORT1 = 0x01    # GPIO端口1
GPIO_PORT2 = 0x02    # GPIO端口2

# 定义GPIO方向
GPIO_DIR_OUTPUT = 0x01    # 输出模式
GPIO_DIR_INPUT = 0x00     # 输入模式

# 定义错误码
GPIO_SUCCESS = 0             # 成功
GPIO_ERR_NOT_SUPPORT = -1    # 不支持该功能
GPIO_ERR_USB_WRITE_FAIL = -2 # USB写入失败
GPIO_ERR_USB_READ_FAIL = -3  # USB读取失败
GPIO_ERR_PARAM_INVALID = -4  # 参数无效

def main():
    # 当前目录
    current_dir = os.path.dirname(os.path.abspath(__file__))
    dll_path = os.path.join(current_dir, "USB_G2X.dll")
    print(f"正在加载DLL: {dll_path}")
    
    # 检查DLL是否存在
    if not os.path.exists(dll_path):
        print(f"错误: DLL文件不存在: {dll_path}")
        return
    
    # 加载DLL
    usb_application = ctypes.CDLL(dll_path)
    print("成功加载DLL")
    
    # 启用日志功能
    print("启用USB调试日志...")
    usb_application.USB_SetLogging(1)
    
    # 测试设备扫描
    max_devices = 10
    devices = (DeviceInfo * max_devices)()
    
    print("调用 USB_ScanDevice 函数...")
    
    # ===================================================
    # 函数: USB_ScanDevice
    # 描述: 扫描并获取符合条件的USB设备信息
    # 参数:
    #   devices: 设备信息结构体数组，用于存储扫描到的设备信息
    #   max_devices: 数组的最大容量，指定最多能存储多少个设备信息
    # 返回值:
    #   >=0: 实际扫描到的设备数量
    #   <0: 发生错误，返回错误代码
    # ===================================================
    result = usb_application.USB_ScanDevice(ctypes.byref(devices), max_devices)
    
    if result < 0:
        print(f"扫描设备失败，错误代码: {result}")
        return
    
    device_count = result
    print(f"找到 {device_count} 个设备")
    
    if device_count == 0:
        print("没有找到设备，请检查USB连接")
        return
    
    # 显示设备信息
    for i in range(device_count):
        device = devices[i]
        print(f"设备 {i}:")
        print(f"  序列号: {device.serial.decode('utf-8')}")
        print(f"  厂商ID: 0x{device.vendor_id:04X}")
        print(f"  产品ID: 0x{device.product_id:04X}")
        print(f"  描述: {device.description.decode('utf-8')}")
        print(f"  制造商: {device.manufacturer.decode('utf-8')}")
        print(f"  接口: {device.interface_str.decode('utf-8')}")
    
    # 选择第一个设备
    selected_device = 0
    if device_count > 1:
        while True:
            try:
                selected_device = int(input(f"请选择设备 (0-{device_count-1}): "))
                if 0 <= selected_device < device_count:
                    break
                else:
                    print(f"请输入0到{device_count-1}之间的数字")
            except ValueError:
                print("请输入有效的数字")
    
    serial_param = devices[selected_device].serial
    print(f"已选择设备: {serial_param.decode('utf-8')}")
    
    # ===================================================
    # 函数: GPIO_SetOutput
    # 描述: 设置GPIO为输出模式
    # 参数:
    #   serial_param: 设备序列号
    #   GPIOIndex: GPIO索引
    #   OutputMask: 输出引脚掩码，每个位对应一个引脚，1表示设置为输出
    # 返回值:
    #   =0: 成功设置GPIO
    #   <0: 设置失败，返回错误代码
    # ===================================================
    usb_application.GPIO_SetOutput.argtypes = [c_char_p, c_int, c_ubyte]
    usb_application.GPIO_SetOutput.restype = c_int
    
    # ===================================================
    # 函数: GPIO_Write
    # 描述: 写入GPIO输出值
    # 参数:
    #   serial_param: 设备序列号
    #   GPIOIndex: GPIO索引
    #   WriteValue: 写入的值，每个位对应一个引脚
    # 返回值:
    #   =0: 成功写入GPIO
    #   <0: 写入失败，返回错误代码
    # ===================================================
    usb_application.GPIO_Write.argtypes = [c_char_p, c_int, c_ubyte]
    usb_application.GPIO_Write.restype = c_int
    
    # 选择GPIO端口
    gpio_index = GPIO_PORT0
    
    # 测试USB通信是否正常工作
    print("\n测试USB调试日志是否工作...")
    usb_application.USB_SetLogging(1)  # 显式开启日志
    print("USB调试日志已开启\n")
    
    # 检查Device句柄是否有效
    print(f"\n设备序列号: {serial_param}")
    
    # ==================================================
    # 函数: USB_OpenDevice
    # 描述: 打开USB设备连接
    # 参数:
    #   serial_param: 设备序列号
    # 返回值:
    #   =0: 设备成功打开
    #   <0: 打开失败，返回错误代码
    # ==================================================
    usb_application.USB_OpenDevice.argtypes = [c_char_p]
    usb_application.USB_OpenDevice.restype = c_int
    
    print("\n打开设备...")
    open_result = usb_application.USB_OpenDevice(serial_param)
    if open_result == 0:
        print("设备打开成功")
    else:
        print(f"设备打开失败，错误代码: {open_result}")
        return
    
    # 设置GPIO函数参数类型

    
    # 设置第一个GPIO引脚为输出模式
    print("\n尝试调用GPIO_SetOutput...")
    output_mask = 0x01  # 只设置第一个引脚为输出
    set_output_result = usb_application.GPIO_SetOutput(serial_param, gpio_index, output_mask)
    print(f"GPIO_SetOutput返回值: {set_output_result}\n")
    
    if set_output_result == GPIO_SUCCESS:
        print(f"成功设置GPIO端口{gpio_index}的第一个引脚为输出模式")
        
        # 简单测试 - 只拉高一个引脚
        led_value = 0x01  # 只点亮第一个LED
        write_result = usb_application.GPIO_Write(serial_param, gpio_index, 0)
        if write_result == GPIO_SUCCESS:
            print(f"GPIO写入成功: 0x{led_value:02X} (只点亮第一个LED)")
        else:
            print(f"GPIO写入失败，错误代码: {write_result}")
        time.sleep(5)
        write_result = usb_application.GPIO_Write(serial_param, gpio_index, 1)



    else:
        print(f"设置GPIO为输出模式失败，错误代码: {set_output_result}")
    
    # 关闭所有LED
    usb_application.GPIO_Write(serial_param, gpio_index, 1)
    
    print("\n关闭设备...")
    
    # ===================================================
    # 函数: USB_CloseDevice
    # 描述: 关闭USB设备连接
    # 参数:
    #   serial_param: 设备序列号
    # 返回值:
    #   =0: 设备成功关闭
    #   <0: 关闭失败，返回错误代码
    # ===================================================
    close_result = usb_application.USB_CloseDevice(serial_param)
    if close_result == 0:
        print("设备关闭成功")
    else:
        print(f"设备关闭失败，错误代码: {close_result}")


if __name__ == "__main__":
    main()
