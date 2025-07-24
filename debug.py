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
        # 将二维数组展平为一维数组
        flat_img_array = img_array.flatten()
        # 直接创建ctypes数组并添加到列表（不再使用外层列表）
        data_list.append((c_ubyte * len(flat_img_array))(*flat_img_array))

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
    usb_application.USB_SetLogging(1)
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



        time.sleep(1)
        ret = usb_application.SPI_GetQueueStatus(serial_param,1)
        print(f'ret:{ret}')



        ret = usb_application.SPI_StartQueue(serial_param,1)



        ret = usb_application.SPI_StopQueue(serial_param,1)
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
