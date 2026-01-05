#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
I2C功能测试脚本
"""
import os
import ctypes
import time

from ctypes import *

# 定义设备信息结构体
class DeviceInfo(Structure):
    _fields_ = [
        ("serial", c_char * 64),
        ("description", c_char * 128),
        ("manufacturer", c_char * 128),
        ("vendor_id", c_ushort),
        ("product_id", c_ushort),
        ("device_id", c_int)
    ]

# 定义IIC配置结构体
class IIC_CONFIG(Structure):
    _fields_ = [
        ("ClockSpeedHz", c_uint32),      # IIC时钟频率:单位为Hz
        ("OwnAddr", c_ushort),           # USB2XXX为从机时自己的地址
        ("Master", c_ubyte),             # 主从选择控制:0-从机，1-主机
        ("AddrBits", c_ubyte),           # 从机地址模式，7-7bit模式，10-10bit模式
        ("EnablePu", c_ubyte),           # 使能引脚芯片内部上拉电阻
    ]

# 定义错误码
I2C_SUCCESS = 0             # 成功
I2C_ERR_NOT_SUPPORT = -1    # 不支持该功能
I2C_ERR_USB_WRITE_FAIL = -2 # USB写入失败
I2C_ERR_USB_READ_FAIL = -3  # USB读取失败
I2C_ERR_PARAM_INVALID = -4  # 参数无效

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
    result = usb_application.USB_ScanDevices(ctypes.byref(devices), max_devices)
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
    # 函数: USB_OpenDevice
    # 描述: 打开USB设备连接
    # 参数:
    #   serial_param: 设备序列号
    # 返回值:
    #   =0: 设备成功打开
    #   <0: 打开失败，返回错误代码
    # ===================================================
    open_result = usb_application.USB_OpenDevice(serial_param)
    if open_result != 0:
        print(f"打开设备失败，错误代码: {open_result}")
        return
    
    print("设备打开成功")
    
    try:
        # ===================================================
        # I2C功能测试
        # ===================================================
        print("\n开始I2C功能测试...")
        
        # 配置IIC函数参数类型
        usb_application.IIC_Init.argtypes = [c_char_p, c_int, POINTER(IIC_CONFIG)]
        usb_application.IIC_Init.restype = c_int
        
        usb_application.IIC_WriteBytes.argtypes = [c_char_p, c_int, c_ushort, POINTER(c_ubyte), c_ushort, c_uint32]
        usb_application.IIC_WriteBytes.restype = c_int
        
        # 1. IIC初始化测试
        print("\n1. IIC初始化测试")
        iic_config = IIC_CONFIG()
        iic_config.ClockSpeedHz = 400000    # 400kHz
        iic_config.OwnAddr = 0x08           # 自身地址
        iic_config.Master = 1               # 主机模式
        iic_config.AddrBits = 7             # 7位地址模式
        iic_config.EnablePu = 1             # 启用内部上拉电阻
        
        i2c_index = 1  # I2C索引1（对应I2C4）
        
        print(f"初始化IIC{i2c_index}，时钟速度: {iic_config.ClockSpeedHz}Hz")
        init_result = usb_application.IIC_Init(serial_param, i2c_index, ctypes.byref(iic_config))
        if init_result == I2C_SUCCESS:
            print("IIC初始化成功")
        else:
            print(f"IIC初始化失败，错误代码: {init_result}")
            return
        
        # 2. 简单IIC测试 - 只发送一个字节
        print("\n2. 简单IIC测试（发送一个字节）")
        device_addr = 0x50      # 示例设备地址
        test_value = 0xAA       # 测试数据
        write_buffer = (c_ubyte * 1)(test_value)
        timeout_ms = 10         # 10ms超时
        
        print(f"向设备0x{device_addr:02X}发送一个字节: 0x{test_value:02X}")
        result = usb_application.IIC_WriteBytes(
            serial_param, i2c_index, device_addr, write_buffer, 1, timeout_ms
        )
        
        if result == I2C_SUCCESS:
            print("IIC发送成功")
        else:
            print(f"IIC发送失败，错误代码: {result}")
            print("注意：如果没有连接IIC从设备，会出现Acknowledge Failure错误")
            print("注意：如果没有连接I2C从设备，会出现Acknowledge Failure错误")
        
        print("\nI2C功能测试完成")
        
    except Exception as e:
        print(f"测试过程中发生异常: {e}")
    
    finally:
        # 关闭设备
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