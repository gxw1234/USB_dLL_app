#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
UART功能测试脚本
测试UART初始化功能
"""
from datetime import datetime
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

# 定义UART配置结构体
class UART_CONFIG(Structure):
    _fields_ = [
        ("BaudRate", c_uint),        # 波特率
        ("WordLength", c_ubyte),     # 数据位宽，0-8bit,1-9bit
        ("StopBits", c_ubyte),       # 停止位宽，0-1bit,1-0.5bit,2-2bit,3-1.5bit
        ("Parity", c_ubyte),         # 奇偶校验，0-No,4-Even,6-Odd
        ("TEPolarity", c_ubyte)      # TE输出控制，0x80-输出TE信号且低电平有效，0x81-输出TE信号且高电平有效，0x00不输出TE信号
    ]

# 定义错误码
USB_SUCCESS = 0
USB_ERROR_INVALID_PARAM = -1
USB_ERROR_OTHER = -2
USB_ERROR_TIMEOUT = -3

def test_uart_init(usb_application, target_serial):
    """测试UART初始化功能"""
    print("\n=== UART初始化测试 ===")
    
    # UART配置: 115200波特率，8位数据，1个停止位，无奇偶校验
    config = UART_CONFIG()
    config.BaudRate = 115200    # 波特率
    config.WordLength = 0       # 数据位宽，0-8bit
    config.StopBits = 0         # 停止位宽，0-1bit
    config.Parity = 0           # 奇偶校验，0-No
    config.TEPolarity = 0x00    # TE输出控制，0x00不输出TE信号
    uart_index = 1  # UART索引1对应USART3 (PD8/PD9)
    
    print(f"UART配置参数:")
    print(f"  波特率: {config.BaudRate}")
    print(f"  数据位: 8bit")
    print(f"  停止位: 1bit")
    print(f"  奇偶校验: None")
    print(f"  TE控制: 0x{config.TEPolarity:02X}")
    
    # 调用UART_Init函数
    print(f"调用 UART_Init('{target_serial}', {uart_index}, config)...")
    
    # ===================================================
    # 函数: UART_Init
    # 描述: 初始化UART接口
    # 参数:
    #   target_serial: 目标设备序列号
    #   uart_index: UART索引 (1对应USART3/PD8/PD9)
    #   config: UART配置参数结构体指针
    # 返回值:
    #   USB_SUCCESS: 初始化成功
    #   <0: 初始化失败，返回错误代码
    # ===================================================
    result = usb_application.UART_Init(
        target_serial.encode('utf-8'),
        uart_index,
        ctypes.byref(config)
    )
    
    if result == USB_SUCCESS:
        print(f"✓ UART初始化成功!")
        return True
    else:
        print(f"✗ UART初始化失败，错误代码: {result}")
        return False

def test_uart_read_continuous(usb_application, target_serial):
    """持续读取UART数据"""
    print("\n=== UART数据持续读取测试 ===")
    print("开始持续读取UART数据...")
    print("请向UART发送数据 (按Ctrl+C停止)")
    
    uart_index = 1
    buffer_size = 256
    read_buffer = (ctypes.c_ubyte * buffer_size)()
    try:
        while True:
            # ===================================================
            # 函数: UART_ReadBytes
            # 描述: 从UART缓冲区读取数据
            # 参数:
            #   target_serial: 目标设备序列号
            #   uart_index: UART索引
            #   pReadBuffer: 读取缓冲区
            #   ReadLen: 读取长度
            # 返回值:
            #   >=0: 实际读取的字节数
            #   <0: 读取失败，返回错误代码
            # ===================================================
            result = usb_application.UART_ReadBytes(
                target_serial.encode('utf-8'),
                uart_index,
                read_buffer,
                buffer_size
            )
            
            # 添加调试信息
            if result > 0:
                current_timestamp = datetime.now().timestamp()
                print(f'result: {result}, timestamp: {current_timestamp}')
                #
                # # 将读取到的数据转换为字节数组
                # data = bytes(read_buffer[:result])
                #
                # # 显示接收到的数据
                # print(f"[{time.strftime('%H:%M:%S')}] 接收到 {result} 字节:")
                #
                # # 显示十六进制格式
                # hex_str = ' '.join([f'{b:02X}' for b in data])
                # print(f"  HEX: {hex_str}")
                #
                # # 显示ASCII格式（可打印字符）
                # ascii_str = ''.join([chr(b) if 32 <= b < 127 else '.' for b in data])
                # print(f"  ASCII: '{ascii_str}'")
                #
            elif result < 0:
                print(f"读取数据失败，错误代码: {result}")
                break
            
            # 短暂延时，避免CPU占用过高
            time.sleep(0.1)
            
    except KeyboardInterrupt:
        print("\n用户中断，停止读取数据")
    except Exception as e:
        print(f"\n读取过程中发生错误: {e}")

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
    
    # 设置DLL函数返回类型
    usb_application.UART_ReadBytes.restype = ctypes.c_int
    usb_application.UART_ReadBytes.argtypes = [ctypes.c_char_p, ctypes.c_int, ctypes.POINTER(ctypes.c_ubyte), ctypes.c_int]
    
    # 测试设备扫描
    max_devices = 10
    devices = (DeviceInfo * max_devices)()
    
    print("调用 USB_ScanDevice 函数...")
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
        print(f"设备 {i+1}:")
        print(f"  序列号: {device.serial.decode('utf-8')}")
        print(f"  描述: {device.description.decode('utf-8')}")
        print(f"  制造商: {device.manufacturer.decode('utf-8')}")
        print(f"  VID: 0x{device.vendor_id:04X}")
        print(f"  PID: 0x{device.product_id:04X}")
    
    # 选择第一个设备进行测试
    target_serial = devices[0].serial.decode('utf-8')
    print(f"\n使用设备进行测试: {target_serial}")
    
    # 打开设备
    print("打开设备...")
    result = usb_application.USB_OpenDevice(target_serial.encode('utf-8'))
    if result < 0:
        print(f"打开设备失败，错误代码: {result}")
        return
    
    print("设备打开成功!")
    usb_application.USB_SetLogging(1)
    try:
        # 执行UART测试
        if test_uart_init(usb_application, target_serial):
            # UART初始化成功，开始持续读取数据
            # test_uart_read_continuous(usb_application, target_serial)



            test_data = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
            send_buffer = (ctypes.c_ubyte * 10)(*test_data)
            
            result = usb_application.UART_WriteBytes(
                target_serial.encode('utf-8'),
                1,
                send_buffer,
                10
            )


            if result == USB_SUCCESS:
                print(f"发送10字节数据成功: ")
            else:
                print(f"发送失败，错误码: {result}")
        else:
            print("UART初始化失败，跳过数据读取测试")
        
    finally:
        # 关闭设备
        print("\n关闭设备...")
        usb_application.USB_CloseDevice(target_serial.encode('utf-8'))
        print("设备已关闭")

if __name__ == "__main__":
    main()