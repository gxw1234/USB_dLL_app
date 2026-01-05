#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
简化的UART Python封装类
让不懂C的用户更容易使用UART功能
"""

import os
import ctypes
import time
from ctypes import *

class DeviceInfo(Structure):
    _fields_ = [
        ("serial", c_char * 64),
        ("description", c_char * 128),
        ("manufacturer", c_char * 128),
        ("vendor_id", c_ushort),
        ("product_id", c_ushort),
        ("device_id", c_int)
    ]

class UART_CONFIG(Structure):
    _fields_ = [
        ("BaudRate", c_uint),
        ("WordLength", c_ubyte),
        ("StopBits", c_ubyte),
        ("Parity", c_ubyte),
        ("TEPolarity", c_ubyte)
    ]

USB_SUCCESS = 0

class SimpleUART:
    """简化的UART操作类"""
    
    def __init__(self, device_serial=None, uart_index=1, baudrate=115200):
        """
        初始化UART
        :param device_serial: 设备序列号，None则自动选择第一个设备
        :param uart_index: UART索引，默认1（对应USART3）
        :param baudrate: 波特率，默认115200
        """
        self.uart_index = uart_index
        self.device_serial = device_serial
        self.usb_app = None
        self.is_connected = False
        
        # 加载DLL
        self._load_dll()
        
        # 扫描并连接设备
        if device_serial is None:
            self.device_serial = self._scan_and_select_device()
        
        # 打开设备
        self._open_device()
        
        # 初始化UART
        self._init_uart(baudrate)
        
        print(f"✓ UART初始化成功: {self.device_serial}, 索引{uart_index}, 波特率{baudrate}")
    
    def _load_dll(self):
        """加载USB DLL"""
        try:
            self.usb_app = ctypes.CDLL('./USB_G2X.dll')
            
            # 设置函数类型
            self.usb_app.USB_ScanDevice.argtypes = [POINTER(DeviceInfo), c_int]
            self.usb_app.USB_ScanDevice.restype = c_int
            self.usb_app.USB_OpenDevice.argtypes = [c_char_p]
            self.usb_app.USB_OpenDevice.restype = c_int
            self.usb_app.UART_Init.argtypes = [c_char_p, c_int, POINTER(UART_CONFIG)]
            self.usb_app.UART_Init.restype = c_int
            self.usb_app.UART_SendString.argtypes = [c_char_p, c_int, c_char_p]
            self.usb_app.UART_SendString.restype = c_int
            self.usb_app.UART_ReadString.argtypes = [c_char_p, c_int, c_char_p, c_int]
            self.usb_app.UART_ReadString.restype = c_int
            self.usb_app.USB_CloseDevice.argtypes = [c_char_p]
            self.usb_app.USB_CloseDevice.restype = c_int
            
        except Exception as e:
            raise Exception(f"加载USB DLL失败: {e}")
    
    def _scan_and_select_device(self):
        """扫描并选择第一个设备"""
        devices = (DeviceInfo * 10)()
        device_count = self.usb_app.USB_ScanDevice(devices, 10)
        
        if device_count <= 0:
            raise Exception("未找到USB设备")
        
        return devices[0].serial.decode('utf-8')
    
    def _open_device(self):
        """打开设备"""
        result = self.usb_app.USB_OpenDevice(self.device_serial.encode('utf-8'))
        if result != USB_SUCCESS:
            raise Exception(f"打开设备失败，错误码: {result}")
        self.is_connected = True
    
    def _init_uart(self, baudrate):
        """初始化UART"""
        config = UART_CONFIG()
        config.BaudRate = baudrate
        config.WordLength = 0  # 8位
        config.StopBits = 0    # 1停止位
        config.Parity = 0      # 无校验
        config.TEPolarity = 0x00
        
        result = self.usb_app.UART_Init(
            self.device_serial.encode('utf-8'),
            self.uart_index,
            byref(config)
        )
        
        if result != USB_SUCCESS:
            raise Exception(f"UART初始化失败，错误码: {result}")
    
    def send(self, text):
        """
        发送字符串
        :param text: 要发送的字符串
        :return: True成功，False失败
        """
        if not self.is_connected:
            raise Exception("设备未连接")
        
        if isinstance(text, str):
            text_bytes = text.encode('utf-8')
        else:
            text_bytes = text
        
        result = self.usb_app.UART_SendString(
            self.device_serial.encode('utf-8'),
            self.uart_index,
            text_bytes
        )
        
        success = (result == USB_SUCCESS)
        if success:
            print(f"发送成功: '{text}' ({len(text_bytes)}字节)")
        else:
            print(f"发送失败，错误码: {result}")
        
        return success
    
    def read(self, max_length=256, timeout=1.0):
        """
        读取字符串
        :param max_length: 最大读取长度
        :param timeout: 超时时间（秒）
        :return: 读取到的字符串，无数据返回空字符串
        """
        if not self.is_connected:
            raise Exception("设备未连接")
        
        buffer = ctypes.create_string_buffer(max_length)
        
        # 尝试读取，带超时
        start_time = time.time()
        while time.time() - start_time < timeout:
            result = self.usb_app.UART_ReadString(
                self.device_serial.encode('utf-8'),
                self.uart_index,
                buffer,
                max_length
            )
            
            if result > 0:
                received = buffer.value.decode('utf-8', errors='ignore')
                print(f"接收到: '{received}' ({result}字节)")
                return received
            elif result < 0:
                print(f"读取失败，错误码: {result}")
                return ""
            
            time.sleep(0.01)  # 短暂等待
        
        return ""  # 超时返回空字符串
    
    def close(self):
        """关闭连接"""
        if self.is_connected:
            self.usb_app.USB_CloseDevice(self.device_serial.encode('utf-8'))
            self.is_connected = False
            print("设备连接已关闭")
    
    def __enter__(self):
        """支持with语句"""
        return self
    
    def __exit__(self, exc_type, exc_val, exc_tb):
        """支持with语句"""
        self.close()

# 使用示例
if __name__ == "__main__":
    try:
        # 方式1：简单使用
        uart = SimpleUART()
        uart.send("hello")
        response = uart.read()
        uart.close()
        
        # 方式2：with语句（推荐）
        with SimpleUART() as uart:
            uart.send("hello world")
            response = uart.read()
            print(f"收到回复: {response}")
            
    except Exception as e:
        print(f"错误: {e}")
