#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
IIC通信测试脚本
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

# 定义IIC配置结构体
class IIC_CONFIG(Structure):
    _fields_ = [
        ("ClockSpeedHz", c_uint),      # IIC时钟频率:单位为Hz
        ("OwnAddr", c_ushort),         # USB2XXX为从机时自己的地址
        ("Master", c_ubyte),           # 主从选择控制:0-从机，1-主机
        ("AddrBits", c_ubyte),         # 从机地址模式，7-7bit模式，10-10bit模式
        ("EnablePu", c_ubyte)          # 使能引脚芯片内部上拉电阻
    ]

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
    
    # 启用日志功能（可选）
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
    serial_param = devices[0].serial    # 打开第一个设备
    
    # serial_param ="xxxxxxx".encode('utf-8')   # 如果已知设备的序列号
    # ===================================================
    # 函数: USB_OpenDevice
    # 描述: 打开指定序列号的USB设备
    # 参数:
    #   serial_param: 目标设备的序列号，如果为NULL则打开第一个可用设备
    # 返回值:
    #   >=0: 成功打开设备，返回设备句柄
    #   <0: 打开失败，返回错误代码
    # ===================================================
    open_result = usb_application.USB_OpenDevice(serial_param)
    if open_result >= 0:
        print("成功打开设备")
        
        print("\n测试IIC初始化功能...")
        # 定义IIC相关常量
        IIC_INDEX_0 = 0
        IIC_SUCCESS = 0
        
        # 创建IIC配置结构体
        iic_config = IIC_CONFIG()
        iic_config.ClockSpeedHz = 400000    # 400KHz
        iic_config.OwnAddr = 0x6e           # 从机地址
        iic_config.Master = 0               # 从机模式
        iic_config.AddrBits = 7             # 7bit地址模式
        iic_config.EnablePu = 0             # 禁用内部上拉电阻
        
        # ===================================================
        # 函数: IIC_Init
        # 描述: 初始化IIC设备
        # 参数:
        #   serial_param: 设备序列号
        #   IICIndex: IIC索引，指定使用哪个IIC接口
        #   iic_config: IIC配置结构体，包含IIC时钟频率、地址模式等参数
        # 返回值:
        #   =0: 成功初始化IIC
        #   <0: 初始化失败，返回错误代码
        # ===================================================
        # 定义 IIC_Init 函数参数类型
        usb_application.IIC_Init.argtypes = [c_char_p, c_int, POINTER(IIC_CONFIG)]
        usb_application.IIC_Init.restype = c_int
        
        iic_init_result = usb_application.IIC_Init(serial_param, IIC_INDEX_0, byref(iic_config))
        if iic_init_result == IIC_SUCCESS:
            print("成功发送IIC初始化命令，现在可以查看STM32串口输出")
            
            # 等待一下，给I2C初始化时间
            time.sleep(0.5)
            
            # 测试IIC从机写数据
            print("\n测试IIC从机写数据...")
            
            # 创建要发送的数据
            ls001_ = (c_ubyte * 2)(0x39, 0x01)  # 创建包含0x39和0x01的数据缓冲区
            
            # 定义 IIC_SlaveWriteBytes 函数参数类型
            # 第三个参数使用POINTER((c_ubyte * 2))来匹配数组类型
            usb_application.IIC_SlaveWriteBytes.argtypes = [c_char_p, c_int, POINTER((c_ubyte * 2)), c_int, c_int]
            usb_application.IIC_SlaveWriteBytes.restype = c_int
            
            # 调用IIC从机写数据函数
            slave_write_result = usb_application.IIC_SlaveWriteBytes(serial_param, IIC_INDEX_0, byref(ls001_), 2, 3000)
            
            if slave_write_result == IIC_SUCCESS:
                print(f"成功发送IIC从机写数据命令，数据: [0x39, 0x01]")
            else:
                print(f"发送IIC从机写数据失败，错误代码: {slave_write_result}")
            
            # 其他测试代码可以在这里添加
            # 例如：
            # 1. 可以使用IIC_WriteBytes发送数据到某个从设备
            # 2. 可以使用IIC_ReadBytes从某个从设备读取数据
            # 3. 可以使用IIC_WriteReadBytes进行读写组合操作
            
            # 测试示例：向设备地址0x50发送一些测试数据
            """
            slave_addr = 0x50
            test_data = (c_ubyte * 5)(0x01, 0x02, 0x03, 0x04, 0x05)
            usb_application.IIC_WriteBytes.argtypes = [c_char_p, c_int, c_ubyte, POINTER(c_ubyte), c_int, c_ubyte]
            usb_application.IIC_WriteBytes.restype = c_int
            
            write_result = usb_application.IIC_WriteBytes(serial_param, IIC_INDEX_0, slave_addr, test_data, 5, 1)
            if write_result == IIC_SUCCESS:
                print(f"成功发送IIC写数据命令，发送了5字节数据到地址0x{slave_addr:02X}")
            else:
                print(f"发送IIC写数据失败，错误代码: {write_result}")
            """
            
        else:
            print(f"IIC初始化失败，错误代码: {iic_init_result}")
        
        # 等待一下，给设备处理时间
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
    else:
        print(f"打开设备失败，错误代码: {open_result}")

if __name__ == "__main__":
    main()
