#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
简单的USB API测试脚本
"""

import os
import ctypes
import time
from ctypes import c_int, c_char, c_char_p, c_ubyte, c_ushort, byref, Structure, POINTER, create_string_buffer

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

def main():
    # 当前目录
    current_dir = os.path.dirname(os.path.abspath(__file__))
    dll_path = os.path.join(current_dir, "usb_api.dll")
    print(f"正在加载DLL: {dll_path}")
    
    # 检查DLL是否存在
    if not os.path.exists(dll_path):
        print(f"错误: DLL文件不存在: {dll_path}")
        return
    
   
    # 加载DLL
    usb_api = ctypes.CDLL(dll_path)
    print("成功加载DLL")
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
    result = usb_api.USB_ScanDevice(ctypes.byref(devices), max_devices)

    print(f"扫描结果: {result}")
    if result > 0:
        print(f"找到 {result} 个USB设备:")
        for i in range(result):
            serial = devices[i].serial.decode('utf-8', errors='ignore').strip('\x00')
            desc = devices[i].description.decode('utf-8', errors='ignore').strip('\x00')
            manufacturer = devices[i].manufacturer.decode('utf-8', errors='ignore').strip('\x00')
            # 显示设备信息
            print(f"设备 {i+1}:")
            print(f"  序列号: {serial}")
            print(f"  产品名称: {desc}")
            print(f"  制造商: {manufacturer}")
            print()
            # 尝试打开设备
            print(f"尝试打开设备: {serial}")

    serial_param = devices[i].serial
    # serial_param ="xxxxxxx".encode('utf-8')   #如果已知设备的序列号
    
    # ===================================================
    # 函数: USB_OpenDevice
    # 描述: 打开指定序列号的USB设备
    # 参数:
    #   serial_param: 目标设备的序列号，如果为NULL则打开第一个可用设备
    # 返回值:
    #   >=0: 成功打开设备，返回设备句柄
    #   <0: 打开失败，返回错误代码
    # ===================================================
    handle = usb_api.USB_OpenDevice(serial_param)

    time.sleep(1)
    if handle >= 0:
        print(f"设备打开成功，句柄: {handle}")
        
        # 读取数据
        buffer_size = 102400
        buffer = (c_ubyte * buffer_size)()
        
        print("尝试读取数据...")
        
        # ===================================================
        # 函数: USB_ReadData
        # 描述: 从USB设备读取数据
        # 参数:
        #   serial_param: 设备序列号
        #   buffer: 数据缓冲区，用于存储读取到的数据
        #   buffer_size: 要读取的数据长度(字节)
        # 返回值:
        #   >0: 实际读取到的数据长度
        #   =0: 超时，未读取到数据
        #   <0: 读取失败，返回错误代码
        # ===================================================
        read_result = usb_api.USB_ReadData(serial_param, buffer, buffer_size)
        
        if read_result > 0:
            print(f"成功读取 {read_result} 字节数据")
            print("数据样本: ", end="")
            for i in range(min(read_result, 16)):
                print(f"{buffer[i]:02X} ", end="")
            print()
        elif read_result == 0:
            print("超时，未读取到数据")
        else:
            print(f"读取失败，错误代码: {read_result}")
        
        # 关闭设备
        print("关闭设备...")
        
        # ===================================================
        # 函数: USB_CloseDevice
        # 描述: 关闭USB设备连接
        # 参数:
        #   serial_param: 设备序列号
        # 返回值:
        #   =0: 设备成功关闭
        #   <0: 关闭失败，返回错误代码
        # ===================================================
        close_result = usb_api.USB_CloseDevice(serial_param)
        if close_result == 0:
            print("设备关闭成功")


if __name__ == "__main__":
    main()
