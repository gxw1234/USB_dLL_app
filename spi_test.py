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
        ("serial", c_char * 64),       # 设备序列号
        ("vendor_id", c_ushort),       # 厂商ID
        ("product_id", c_ushort),      # 产品ID
        ("description", c_char * 256), # 设备描述
        ("manufacturer", c_char * 256),# 制造商
        ("interface_str", c_char * 256)# 接口
    ]

# 定义SPI配置结构体
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
    
# 定义电压配置结构体
class VOLTAGE_CONFIG(Structure):
    _fields_ = [
        ("channel", c_ubyte),        # 电源通道
        ("voltage", c_ushort)        # 电压值（单位：mV）
    ]



from numpy import array
def image_sort(data: list):
    image_format = "." + data[0].split(".")[1]
    image_length_length = len(data[0].split(".")[0])
    Formatting = "%0" + str(image_length_length) + "d"
    im_split = [int(i.split(".")[0]) for i in data]
    for i in range(len(im_split)):
        for j in range(i + 1, (len(im_split))):
            if im_split[i] > im_split[j]:
                im_split[i], im_split[j] = im_split[j], im_split[i]
    return [str(Formatting % i) + image_format for i in im_split]


def Process_99_frames(images_data_list=list):
    temp = []
    temp1 = []
    temp2 = []
    for k, i in enumerate(images_data_list):
        if k < 80:
            temp += list(i)
        elif k < 160:
            temp1 += list(i)
        else:
            temp2 += list(i)

    return temp, temp1, temp2


def hex_images(save_path: str) -> list:
    file_dir = save_path
    dir_list = os.listdir(file_dir)
    data_list = []

    data_list_sort = image_sort(dir_list)

    for cur_file in data_list_sort:
        # 获取文件的绝对路径
        path = os.path.join(file_dir, cur_file)
        im = Image.open(path)
        img = im.convert('L')
        img_array = array(img)
        temp, temp1, temp2 = Process_99_frames(img_array)
        data_list.append([(c_ubyte * len(temp))(*temp), (c_ubyte * len(temp1))(*temp1), (c_ubyte * len(temp2))(*temp2)])
    return data_list




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



        print("\n测试SPI初始化功能...")
        # 定义SPI相关常量
        SPI1_CS0 = 0
        SPI_SUCCESS = 0
        
        # 创建SPI配置结构体
        spi_config = SPI_CONFIG()
        spi_config.Mode = 0              # 硬件控制（全双工模式）
        spi_config.Master = 1          # 主机模式
        spi_config.CPOL = 0           
        spi_config.CPHA = 1
        spi_config.LSBFirst = 0        # MSB在前
        spi_config.SelPolarity = 0    
        spi_config.ClockSpeedHz = 25000000  # 25MHz
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
        if spi_init_result == SPI_SUCCESS:
            print("成功发送SPI初始化命令，现在可以查看STM32串口输出")
            SPIIndex = SPI1_CS0




            write_buffer_size = 1024  # 20KB
            write_buffer = (c_ubyte * write_buffer_size)()
            # 生成测试数据
            for i in range(write_buffer_size):
                write_buffer[i] = i % 256

            # test_size = 10
            # print(f"发送的前{test_size}个字节数据: ", end="")
            # for i in range(test_size):
            #     print(f"{write_buffer[i]:02X} ", end="")
            # print()
            
            # ===================================================
            # 函数: SPI_WriteBytes
            # 描述: 通过SPI发送数据
            # 参数:
            #   serial_param: 设备序列号
            #   SPIIndex: SPI索引，指定使用哪个SPI接口和片选
            #   write_buffer: 要发送的数据缓冲区
            #   test_size: 要发送的数据长度(字节)
            # 返回值:
            #   =0: 成功发送SPI数据
            #   <0: 发送失败，返回错误代码
            # ===================================================
            # 定义SPI_WriteBytes函数参数类型
            usb_application.SPI_WriteBytes.argtypes = [c_char_p, c_int, POINTER(c_ubyte), c_int]
            usb_application.SPI_WriteBytes.restype = c_int

            current_line = r"D:\py\autoScan\temp1\1"

            images_data = hex_images(current_line)

            usb_application.GPIO_SetOutput(serial_param, 0, 0)



            for i in images_data:
                write_result = usb_application.GPIO_Write(serial_param, 0, 0)
                for ii in i:
                    # 直接传递数组，ctypes会自动转换为指针
                    usb_application.SPI_WriteBytes(serial_param, SPIIndex, ii, len(ii))
                write_result = usb_application.GPIO_Write(serial_param, 0, 1)
            #
            # def Single_frame_sen(self, data):
            #     USB2XXXLib.GPIO_Write(self.DevHandle, self.pb_cs, 0x00)
            #     for ii in data:
            #         USB2XXXLib.SPI_WriteBytes(self.DevHandle, SPI1_CS0, byref(ii), len(ii))
            #     USB2XXXLib.GPIO_Write(self.DevHandle, self.pb_cs, self.pb_cs)


            # 调用SPI_WriteBytes函数发送数据
            # write_result = usb_application.SPI_WriteBytes(serial_param, SPIIndex, write_buffer, len(write_buffer))


            #
            # if write_result == SPI_SUCCESS:
            #     print(f"成功发送SPI写数据命令，发送了{len(write_buffer)}字节数据，")
            # else:
            #     print(f"发送SPI写数据失败，错误代码: {write_result}")
        else:
            print(f"SPI初始化失败，错误代码: {spi_init_result}")
        
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


if __name__ == "__main__":
    main()
