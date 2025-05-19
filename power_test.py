#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
简单的USB应用层测试脚本
"""

import os
import ctypes
import time
from ctypes import c_int, c_char, c_char_p, c_ubyte, c_ushort, c_uint, byref, Structure, POINTER, create_string_buffer
import struct
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
            print(f"成功发送设置电压命令，")
        else:
            print(f"设置电压失败，错误代码: {power_result}")
        time.sleep(1)

        power_result = usb_application.POWER_StartCurrentReading(serial_param, POWER_CHANNEL_1)
        if power_result == POWER_SUCCESS:
            print(f"开始读取")
        else:
            print(f"开始，错误代码: {power_result}")

        time.sleep(5)
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

        read_result = usb_application.USB_ReadData(serial_param, buffer, buffer_size)
        if read_result > 0:
            print(f"成功读取 {read_result} 字节数据")
            import struct
            received_bytes = bytes(buffer[:read_result])
            data_point_count = read_result // 5
            print(f"解析到的电流数据:")
            current_values = []
            current_types = []
            for i in range(min(data_point_count, 10)):  
                try:
                    base = i * 5 
                    data_type = received_bytes[base]  
                    current_value = struct.unpack('f', received_bytes[base+1:base+5])[0]  # 后4个字节是浮点数
                    current_values.append(current_value)
                    current_types.append(data_type)
                    unit = "uA" if data_type == 1 else "mA"  # 1表示微安，0表示毫安
                    print(f"  电流{i+1}: {current_value:.6f} {unit}")
                except Exception as e:
                    print(f"  无法解析第{i+1}个电流值: {str(e)}")
            print(f"共有{data_point_count}个数据点")
        elif read_result == 0:
            print("超时，未读取到数据")
        else:
            print(f"读取失败，错误代码: {read_result}")



        power_result = usb_application.POWER_StopCurrentReading(serial_param, POWER_CHANNEL_1)
        if power_result == POWER_SUCCESS:

            print(f"停止读取")
        else:
            print(f"停止读取，错误代码: {power_result}")



        # ===================================================
        # 函数: USB_CloseDevice
        # 描述: 关闭USB设备连接
        # 参数:
        #   serial_param: 设备序列号
        # 返回值:
        #   =0: 设备成功关闭
        #   <0: 关闭失败，返回错误代码
        # ===================================================
        print("\n关闭设备...")
        close_result = usb_application.USB_CloseDevice(serial_param)
        if close_result == 0:
            print("设备关闭成功")


if __name__ == "__main__":
    main()
