#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
简单的USB应用层测试脚本
"""

import os
import ctypes
import time
from ctypes import c_int, c_char, c_char_p, c_ubyte, c_ushort, c_uint, byref, Structure, POINTER, create_string_buffer

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
class SPI_CONFIG(Structure):
    _fields_ = [
        ("Mode", c_char),              # SPI控制方式
        ("Master", c_char),            # 主从选择控制
        ("CPOL", c_char),              # 时钟极性控制
        ("CPHA", c_char),              # 时钟相位控制
        ("LSBFirst", c_char),          # 数据移位方式
        ("SelPolarity", c_char),       # 片选信号极性
        ("ClockSpeedHz", c_uint)       # SPI时钟频率
    ]
    
class VOLTAGE_CONFIG(Structure):
    _fields_ = [
        ("channel", c_ubyte),        # 电源通道
        ("voltage", c_ushort)        # 电压值（单位：mV）
    ]

def main():
    current_dir = os.path.dirname(os.path.abspath(__file__))
    dll_path = os.path.join(current_dir, "USB_G2X.dll")
    print(f"正在加载DLL: {dll_path}")
    if not os.path.exists(dll_path):
        print(f"错误: DLL文件不存在: {dll_path}")
        return
    usb_application = ctypes.CDLL(dll_path)
    print("成功加载DLL")
    print("启用USB调试日志...")
    usb_application.USB_SetLogging(1)
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
    else:
        print("未扫描到设备")
        return

    print(f"尝试打开设备: {devices[0].serial.decode('utf-8', errors='ignore').strip('\x00')}")
    serial_param = devices[0].serial    #打开第一个设备
    
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
    handle = usb_application.USB_OpenDevice(serial_param)
    time.sleep(1)
    if handle >= 0:
        print(f"设备打开成功，句柄: {handle}")
        # 读取数据
        buffer_size = 102400
        buffer = (c_ubyte * buffer_size)()
        print("尝试读取数据...")
        
        # 等待一下，给设备处理时间
        time.sleep(1)
        print("\n测试电压设置功能...")
        POWER_CHANNEL_1 = 0x01
        POWER_SUCCESS = 0
        voltage_mv = 3300
        
        # ===================================================
        # 函数: POWER_SetVoltage
        # 描述: 设置电源电压
        # 参数:
        #   serial_param: 设备序列号
        #   channel: 电源通道
        #   voltage_mv: 要设置的电压值(mV)
        # 返回值:
        #   =0: 成功设置电压
        #   <0: 设置失败，返回错误代码
        # ===================================================
        # 定义 POWER_SetVoltage 函数参数类型
        usb_application.POWER_SetVoltage.argtypes = [c_char_p, c_ubyte, c_ushort]
        usb_application.POWER_SetVoltage.restype = c_int
        print(f"设置电压: 通道={POWER_CHANNEL_1}, 电压={voltage_mv}mV")
        power_result = usb_application.POWER_SetVoltage(serial_param, POWER_CHANNEL_1, 186)
        if power_result == POWER_SUCCESS:
            print(f"成功发送设置电压命令，现在可以查看STM32串口输出")
        else:
            print(f"设置电压失败，错误代码: {power_result}")
        time.sleep(1)
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


if __name__ == "__main__":
    main()
