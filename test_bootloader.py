#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
Bootloader固件烧录工具
支持烧录bin文件到STM32设备
"""
import os
import ctypes
import time
from ctypes import c_int, c_char, c_char_p, c_ubyte, c_ushort, c_uint, byref, Structure, POINTER, create_string_buffer
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

def parse_bin_file(bin_file_path):
    """
    直接读取BIN文件，返回二进制数据
    """
    if not os.path.exists(bin_file_path):
        print(f"错误: BIN文件不存在: {bin_file_path}")
        return None
    
    try:
        with open(bin_file_path, 'rb') as f:
            binary_data = f.read()
        
        print(f"成功读取BIN文件，总大小: {len(binary_data)} 字节")
        return binary_data
        
    except Exception as e:
        print(f"读取BIN文件出错: {e}")
        return None

def split_data_to_packets(data, packet_size=450):
    """
    将数据分割成指定大小的数据包
    """
    packets = []
    for i in range(0, len(data), packet_size):
        packet = data[i:i+packet_size]
        packets.append(packet)
    
    print(f"数据分割完成，总包数: {len(packets)}，每包大小: {packet_size} 字节")
    return packets


def main():
    # 当前目录
    current_dir = os.path.dirname(os.path.abspath(__file__))
    dll_path = os.path.join(current_dir, "USB_G2X.dll")
    
    # 指定要烧录的BIN文件路径
    bin_file_path = r'D:\STM32OBJ\usb_test_obj\test_1\stm32h750ibt6\build\usb_app.bin'
    
    print(f"正在加载DLL: {dll_path}")
    print(f"准备烧录BIN文件: {bin_file_path}")

    # 检查DLL是否存在
    if not os.path.exists(dll_path):
        print(f"错误: DLL文件不存在: {dll_path}")
        return

    # 读取BIN文件
    firmware_data = parse_bin_file(bin_file_path)
    if firmware_data is None:
        return
    
    # 分割数据为450字节的包
    data_packets = split_data_to_packets(firmware_data, 450)

    # 加载DLL
    usb_application = ctypes.CDLL(dll_path)
    print("成功加载DLL")
    
    # 扫描设备
    max_devices = 10
    devices = (DeviceInfo * max_devices)()
    usb_application.USB_SetLogging(0)
    result = usb_application.USB_ScanDevices(ctypes.byref(devices), max_devices)

    print(f"扫描结果: {result}")
    if result > 0:
        print(f"找到 {result} 个USB设备:")
        for i in range(result):
            serial = devices[i].serial.decode('utf-8', errors='ignore').strip('\x00')
            desc = devices[i].description.decode('utf-8', errors='ignore').strip('\x00')
            manufacturer = devices[i].manufacturer.decode('utf-8', errors='ignore').strip('\x00')
            print(f"设备 {i + 1}:")
            print(f"  序列号: {serial}")
            print(f"  产品名称: {desc}")
            print(f"  制造商: {manufacturer}")
            print()
    else:
        print("未扫描到设备")
        return

    # 打开设备
    print(f"尝试打开设备: {devices[0].serial.decode('utf-8', errors='ignore').strip('\x00')}")
    serial_param = devices[0].serial
    handle = usb_application.USB_OpenDevice(serial_param)
    time.sleep(1)
    if handle >= 0:
        print(f"设备打开成功，句柄: {handle}")


        if 1:
            try:
                # 发送开始写入命令
                print("发送开始写入命令...")
                dummy_data = (c_ubyte * 1)(0)  # 开始写入命令只需要一个字节的数据
                start_ret = usb_application.Bootloader_StartWrite(serial_param, 1, dummy_data, 1)
                print(f"开始写入命令返回: {start_ret}")
                if start_ret != 0:
                    print(f"开始写入命令失败，返回码: {start_ret}")
                    return
                print(f"开始分包发送固件数据，总包数: {len(data_packets)}")
                start_time = time.time()
                for i, packet in enumerate(data_packets):
                    # 创建ctypes数组
                    packet_buffer = (c_ubyte * len(packet))(*packet)
                    ret = usb_application.Bootloader_WriteBytes(serial_param, 1, packet_buffer, len(packet))
                    if ret != 0:
                        print(f"第 {i+1} 包发送失败，返回码: {ret}")
                        break
                    else:
                        # 显示进度
                        progress = (i + 1) / len(data_packets) * 100
                        print(f"进度: {i+1}/{len(data_packets)} ({progress:.1f}%) - 包大小: {len(packet)} 字节")

                end_time = time.time()
                total_time = end_time - start_time
                total_bytes = len(firmware_data)
                speed = total_bytes / total_time / 1024  # KB/s

                print(f"\n烧录完成!")
                print(f"总时间: {total_time:.2f} 秒")
                print(f"总大小: {total_bytes} 字节")
                print(f"传输速度: {speed:.2f} KB/s")

                # 发送切换到应用程序模式命令
                print("发送切换到应用程序模式命令...")
                dummy_data = (c_ubyte * 1)(0)
                usb_application.Bootloader_SwitchRun(serial_param, 1, dummy_data, 1)
                
                # 发送复位命令
                print("发送复位命令...")
                usb_application.Bootloader_Reset(serial_param, 1, dummy_data, 1)
                print("设备将重启并进入应用程序")

            except Exception as e:
                print(f"烧录过程中出错: {e}")


        if 1:  # 禁用原来的代码
            dummy_data = (c_ubyte * 1)(0)  # 开始写入命令只需要一个字节的数据

            start_ret = usb_application.Bootloader_SwitchRun(serial_param, 1, dummy_data, 1)
            # start_ret = usb_application.Bootloader_SwitchBoot(serial_param, 1, dummy_data, 1)



            start_ret = usb_application.Bootloader_Reset(serial_param, 1, dummy_data, 1)


            print("11")
        # 关闭设备
        close_result = usb_application.USB_CloseDevice(serial_param)
        if close_result == 0:
            print("设备关闭成功")
        else:
            print(f"设备关闭失败，返回码: {close_result}")
    else:
        print(f"设备打开失败，返回码: {handle}")


if __name__ == "__main__":
    main()
