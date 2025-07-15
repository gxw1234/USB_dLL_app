#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
简单的USB应用层测试脚本
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
        ("serial", c_char * 64),
        ("description", c_char * 128),
        ("manufacturer", c_char * 128),
        ("vendor_id", c_ushort),
        ("product_id", c_ushort),
        ("device_id", c_int)
    ]


# 定义SPI配置结构体
class SPI_CONFIG(Structure):
    _fields_ = [
        ("Mode", c_char),  # SPI控制方式
        ("Master", c_char),  # 主从选择控制
        ("CPOL", c_char),  # 时钟极性控制
        ("CPHA", c_char),  # 时钟相位控制
        ("LSBFirst", c_char),  # 数据移位方式
        ("SelPolarity", c_char),  # 片选信号极性
        ("ClockSpeedHz", c_uint)  # SPI时钟频率
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
    # 定义函数类型
    # 调用日志开关函数，参数1表示开启
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

    print(f"扫描结果: {result}")
    if result > 0:
        print(f"找到 {result} 个USB设备:")
        for i in range(result):
            serial = devices[i].serial.decode('utf-8', errors='ignore').strip('\x00')
            desc = devices[i].description.decode('utf-8', errors='ignore').strip('\x00')
            manufacturer = devices[i].manufacturer.decode('utf-8', errors='ignore').strip('\x00')
            # 显示设备信息
            print(f"设备 {i + 1}:")
            print(f"  序列号: {serial}")
            print(f"  产品名称: {desc}")
            print(f"  制造商: {manufacturer}")
            print()
    else:
        print("未扫描到设备")
        return

    print(f"尝试打开设备: {devices[0].serial.decode('utf-8', errors='ignore').strip('\x00')}")
    serial_param = devices[0].serial  # 打开第一个设备

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
        print("\n测试SPI初始化功能...")
        # 定义SPI相关常量
        SPI1_CS0 = 1
        SPI_SUCCESS = 0

        # 创建SPI配置结构体
        spi_config = SPI_CONFIG()
        spi_config.Mode = 0  # 硬件控制（全双工模式）
        spi_config.Master = 0  # 从机模式
        spi_config.CPOL = 0
        spi_config.CPHA = 0
        spi_config.LSBFirst = 0  # MSB在前
        spi_config.SelPolarity = 0
        spi_config.ClockSpeedHz = 10000000  # 25MHz
        # ===================================================
        # 函数: SPI_Init
        # 描述: 初始化SPI设备
        # 参数:
        #   serial_param: 设备序列号
        #   SPIIndex: SPI索引，指定使用哪个SPI接口和片选
        #   spi_config: SPI配置结构体，包含SPI模式、主从设置、时钟极性等参数
        # 返回值:
        #   =0: 成功初始化SPI
        #   <0: 初始化失败，返回错误代码
        # ===================================================
        # 定义 SPI_Init 函数参数类型

        spi_init_result = usb_application.SPI_Init(serial_param, SPI1_CS0, byref(spi_config))
        # time.sleep(1)

        gpio_index = 0x00
        output_mask = 0x00  # 只设置第一个引脚为输出
        # set_output_result = usb_application.GPIO_SetOutput(serial_param, gpio_index, output_mask)

        if spi_init_result == SPI_SUCCESS:
            print("成功发送SPI初始化命令")

            # ===================================================
            # 函数: SPI_SlaveReadBytes
            # 描述: 从SPI设备读取数据（从机模式）
            # 参数:
            #   serial_param: 设备序列号
            #   SPIIndex: SPI索引，指定使用哪个SPI接口和片选
            #   pReadBuffer: 读取数据的缓冲区
            #   ReadLen: 要读取的数据长度
            # 返回值:
            #   >=0: 实际读取的数据长度
            #   <0: 读取失败，返回错误代码
            # ===================================================
            print("\n开始读取SPI数据...")
            read_buffer_size = 20  # 读取缓冲区大小
            read_buffer = (c_ubyte * read_buffer_size)()


            for i in range(5):
                read_result = usb_application.SPI_SlaveReadBytes(serial_param, SPI1_CS0, read_buffer, read_buffer_size)
                if read_result > 0:
                    print(f"第{i + 1}次读取成功，读取了{read_result}字节数据:")
                    print_len = min(read_result, 20)
                    print("数据内容: ", end="")
                    for j in range(print_len):
                        print(f"{read_buffer[j]:02X} ", end="")
                    if read_result > 20:
                        print("...")
                    else:
                        print()
                elif read_result == 0:
                    print(f"第{i + 1}次读取：没有可用数据")
                else:
                    print(f"第{i + 1}次读取失败，错误代码: {read_result}")

                time.sleep(0.5)  # 等待500毫秒









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
