#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
UART发送功能测试脚本
测试UART_WriteBytes函数
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

def test_uart_write_string(usb_application, target_serial, message):
    """测试发送字符串数据"""
    print(f"\n=== UART发送字符串测试 ===")
    print(f"发送消息: '{message}'")
    
    uart_index = 1
    data = message.encode('utf-8')
    data_len = len(data)
    
    # 创建发送缓冲区
    send_buffer = (ctypes.c_ubyte * data_len)()
    for i, byte in enumerate(data):
        send_buffer[i] = byte
    
    print(f"数据长度: {data_len} 字节")
    hex_str = ' '.join([f'{b:02X}' for b in data])
    print(f"HEX数据: {hex_str}")
    
    # ===================================================
    # 函数: UART_WriteBytes
    # 描述: 向UART发送数据
    # 参数:
    #   target_serial: 目标设备序列号
    #   uart_index: UART索引 (1对应USART3/PD8/PD9)
    #   pWriteBuffer: 要发送的数据缓冲区
    #   WriteLen: 数据长度
    # 返回值:
    #   USB_SUCCESS: 发送成功
    #   <0: 发送失败，返回错误代码
    # ===================================================
    result = usb_application.UART_WriteBytes(
        target_serial.encode('utf-8'),
        uart_index,
        send_buffer,
        data_len
    )
    
    if result == USB_SUCCESS:
        print(f"✓ 字符串发送成功!")
        return True
    else:
        print(f"✗ 字符串发送失败，错误代码: {result}")
        return False

def test_uart_write_hex_data(usb_application, target_serial):
    """测试发送十六进制数据"""
    print(f"\n=== UART发送十六进制数据测试 ===")
    
    # 测试数据：0x01, 0x02, 0x03, 0x04, 0x05
    hex_data = [0x01, 0x02, 0x03, 0x04, 0x05, 0xFF, 0xAA, 0x55]
    uart_index = 1
    data_len = len(hex_data)
    
    # 创建发送缓冲区
    send_buffer = (ctypes.c_ubyte * data_len)()
    for i, byte in enumerate(hex_data):
        send_buffer[i] = byte
    
    print(f"发送十六进制数据: {' '.join([f'0x{b:02X}' for b in hex_data])}")
    print(f"数据长度: {data_len} 字节")
    
    result = usb_application.UART_WriteBytes(
        target_serial.encode('utf-8'),
        uart_index,
        send_buffer,
        data_len
    )
    
    if result == USB_SUCCESS:
        print(f"✓ 十六进制数据发送成功!")
        return True
    else:
        print(f"✗ 十六进制数据发送失败，错误代码: {result}")
        return False

def test_uart_write_large_data(usb_application, target_serial):
    """测试发送大数据包"""
    print(f"\n=== UART发送大数据包测试 ===")
    
    # 创建200字节的测试数据
    data_size = 200
    uart_index = 1
    
    # 创建测试数据：递增的字节序列
    test_data = [(i % 256) for i in range(data_size)]
    
    # 创建发送缓冲区
    send_buffer = (ctypes.c_ubyte * data_size)()
    for i, byte in enumerate(test_data):
        send_buffer[i] = byte
    
    print(f"发送大数据包，长度: {data_size} 字节")
    print(f"数据模式: 递增字节序列 (0x00, 0x01, 0x02, ... 0xFF, 0x00, ...)")
    
    result = usb_application.UART_WriteBytes(
        target_serial.encode('utf-8'),
        uart_index,
        send_buffer,
        data_size
    )
    
    if result == USB_SUCCESS:
        print(f"✓ 大数据包发送成功!")
        return True
    else:
        print(f"✗ 大数据包发送失败，错误代码: {result}")
        return False

def test_uart_write_interactive(usb_application, target_serial):
    """交互式发送测试"""
    print(f"\n=== UART交互式发送测试 ===")
    print("请输入要发送的文本 (输入 'quit' 退出):")
    
    uart_index = 1
    
    try:
        while True:
            user_input = input("发送> ")
            
            if user_input.lower() == 'quit':
                print("退出交互式发送测试")
                break
            
            if not user_input:
                continue
            
            # 添加换行符
            message = user_input + "\r\n"
            data = message.encode('utf-8')
            data_len = len(data)
            
            # 创建发送缓冲区
            send_buffer = (ctypes.c_ubyte * data_len)()
            for i, byte in enumerate(data):
                send_buffer[i] = byte
            
            print(f"发送: '{user_input}' (+ \\r\\n)")
            
            result = usb_application.UART_WriteBytes(
                target_serial.encode('utf-8'),
                uart_index,
                send_buffer,
                data_len
            )
            
            if result == USB_SUCCESS:
                print(f"✓ 发送成功 ({data_len} 字节)")
            else:
                print(f"✗ 发送失败，错误代码: {result}")
                
    except KeyboardInterrupt:
        print("\n用户中断，退出交互式测试")
    except Exception as e:
        print(f"\n交互式测试中发生错误: {e}")

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
    
    # 设置DLL函数返回类型和参数类型
    usb_application.UART_WriteBytes.restype = ctypes.c_int
    usb_application.UART_WriteBytes.argtypes = [ctypes.c_char_p, ctypes.c_int, ctypes.POINTER(ctypes.c_ubyte), ctypes.c_int]
    
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
    
    try:
        # 执行UART初始化
        if not test_uart_init(usb_application, target_serial):
            print("UART初始化失败，跳过发送测试")
            return
        
        # 等待一下让UART稳定
        time.sleep(0.5)
        
        # 简单发送测试
        print("\n" + "="*50)
        print("UART发送测试 - Hello World")
        print("="*50)
        
        # 发送 Hello World
        test_uart_write_string(usb_application, target_serial, "Hello World")
        
    finally:
        # 关闭设备
        print("\n关闭设备...")
        usb_application.USB_CloseDevice(target_serial.encode('utf-8'))
        print("设备已关闭")

if __name__ == "__main__":
    main()