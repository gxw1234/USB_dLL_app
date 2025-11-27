#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import ctypes
from ctypes import Structure, c_char, c_uint, c_ubyte, byref, POINTER
import time
import os





# I2S 接口索引
I2S_INDEX_1 = 1    # I2S2 (与C语言中的I2S_INDEX_1保持一致)

# I2S 模式定义
I2S_MODE_MASTER_TX = 0    # 主机发送模式
I2S_MODE_MASTER_RX = 1    # 主机接收模式
I2S_MODE_SLAVE_TX = 2     # 从机发送模式
I2S_MODE_SLAVE_RX = 3     # 从机接收模式

# I2S 标准定义
I2S_STANDARD_PHILIPS = 0   # 飞利浦标准
I2S_STANDARD_MSB = 1      # MSB对齐
I2S_STANDARD_LSB = 2      # LSB对齐
I2S_STANDARD_PCM_SHORT = 3 # PCM短帧
I2S_STANDARD_PCM_LONG = 4  # PCM长帧

# I2S 数据格式定义
I2S_DATAFORMAT_16B = 0    # 16位数据格式
I2S_DATAFORMAT_24B = 1    # 24位数据格式
I2S_DATAFORMAT_32B = 2    # 32位数据格式

# I2S MCLK输出控制
I2S_MCLKOUTPUT_DISABLE = 0 # 禁用MCLK输出
I2S_MCLKOUTPUT_ENABLE = 1  # 启用MCLK输出

# I2S 音频频率定义
I2S_AUDIOFREQ_8K = 8000      # 8kHz采样率
I2S_AUDIOFREQ_16K = 16000    # 16kHz采样率
I2S_AUDIOFREQ_22K = 22050    # 22.05kHz采样率
I2S_AUDIOFREQ_44K = 44100    # 44.1kHz采样率
I2S_AUDIOFREQ_48K = 48000    # 48kHz采样率
I2S_AUDIOFREQ_96K = 96000    # 96kHz采样率
I2S_AUDIOFREQ_192K = 192000  # 192kHz采样率

# I2S 错误码定义
I2S_SUCCESS = 0              # 成功
I2S_ERROR_NOT_FOUND = -1     # 设备未找到
I2S_ERROR_ACCESS = -2        # 访问被拒绝
I2S_ERROR_IO = -3            # I/O错误
I2S_ERROR_INVALID_PARAM = -4 # 参数无效
I2S_ERROR_OTHER = -99        # 其他错误

# USB 错误码定义
USB_SUCCESS = 0               # 成功
USB_ERROR_NOT_FOUND = -1     # 设备未找到
USB_ERROR_ACCESS = -2        # 访问被拒绝
USB_ERROR_IO = -3            # I/O错误
USB_ERROR_INVALID_PARAM = -4 # 参数无效
USB_ERROR_ALREADY_OPEN = -5  # 设备已打开
USB_ERROR_NOT_OPEN = -6      # 设备未打开
USB_ERROR_TIMEOUT = -7       # 超时错误
USB_ERROR_OTHER = -99        # 其他错误





# 定义设备信息结构体 (与C代码中的device_info_t保持一致)
class DeviceInfo(Structure):
    _fields_ = [
        ("serial", c_char * 64),           # 设备序列号
        ("description", c_char * 128),     # 设备描述
        ("manufacturer", c_char * 128),    # 制造商信息
        ("vendor_id", ctypes.c_uint16),    # 厂商ID
        ("product_id", ctypes.c_uint16),   # 产品ID
        ("device_id", ctypes.c_int)        # 设备ID
    ]

# 定义双路音频配置结构体
class DUAL_AUDIO_CONFIG(Structure):
    _fields_ = [
        ("left_audio_path", ctypes.c_char_p),    # 左声道音频文件路径
        ("right_audio_path", ctypes.c_char_p),   # 右声道音频文件路径
        ("gap_duration", ctypes.c_float),        # 右声道延迟时间（秒）
        ("generate_file", ctypes.c_int),         # 是否生成合成音频文件
        ("output_path", ctypes.c_char_p),        # 输出合成音频文件路径
        ("left_volume", ctypes.c_int),           # 左声道音量 (0-100)
        ("right_volume", ctypes.c_int),          # 右声道音量 (0-100)
    ]
class I2S_CONFIG(Structure):
    _fields_ = [
        ("Mode", c_char),           # I2S模式:0-主机发送，1-主机接收，2-从机发送，3-从机接收
        ("Standard", c_char),       # I2S标准:0-飞利浦标准，1-MSB对齐，2-LSB对齐，3-PCM短帧，4-PCM长帧
        ("DataFormat", c_char),     # 数据格式:0-16位，1-24位，2-32位
        ("MCLKOutput", c_char),     # MCLK输出:0-禁用，1-启用
        ("AudioFreq", c_uint)       # 音频频率:8000,16000,22050,44100,48000,96000,192000
    ]


def main():
    # 加载DLL
    dll_path = r"D:\STM32OBJ\usb_test_obj\test_1\USB_dLL_app\USB_G2X.dll"
    print(f"正在加载DLL: {dll_path}")
    
    try:
        usb_application = ctypes.CDLL(dll_path)
        print("成功加载DLL")
    except Exception as e:
        print(f"加载DLL失败: {e}")
        return
    # 扫描设备
    print("调用 USB_ScanDevices 函数...")
    devices = (DeviceInfo * 10)()
    device_count = usb_application.USB_ScanDevices(devices, 10)
    print(f"扫描结果: {device_count}")

    if device_count == 0:
        print("未找到设备")
        return
    print(f"找到 {device_count} 个USB设备:")
    for i in range(device_count):
        serial = devices[i].serial.decode('utf-8', errors='ignore').strip('\x00')
        description = devices[i].description.decode('utf-8', errors='ignore').strip('\x00')
        manufacturer = devices[i].manufacturer.decode('utf-8', errors='ignore').strip('\x00')
        print(f"设备 {i+1}: {serial} - {description} ({manufacturer})")

    # 打开第一个设备
    serial_str = devices[0].serial.decode('utf-8', errors='ignore').strip('\x00')
    print(f"打开设备: {serial_str}")
    serial_param = serial_str.encode('utf-8')
    handle = usb_application.USB_OpenDevice(serial_param)
    time.sleep(1)
    if handle < 0:
        print(f"设备打开失败，错误代码: {handle}")
        return
    print(f"设备打开成功，句柄: {handle}")

    try:
        # I2S配置 - 使用宏定义
        i2s_config = I2S_CONFIG()
        i2s_config.Mode = I2S_MODE_MASTER_TX  # 主机发送模式
        i2s_config.Standard = I2S_STANDARD_PHILIPS  # 飞利浦标准
        i2s_config.DataFormat = I2S_DATAFORMAT_16B  # 16位数据格式
        i2s_config.MCLKOutput = I2S_MCLKOUTPUT_ENABLE  # 启用MCLK输出
        # i2s_config.AudioFreq = I2S_AUDIOFREQ_48K  # 48kHz采样率
        i2s_config.AudioFreq = I2S_AUDIOFREQ_16K    # 16kHz采样率
        i2s_init_result = usb_application.I2S_Init(serial_param, I2S_INDEX_1, byref(i2s_config))
        time.sleep(0.5)
        if i2s_init_result != I2S_SUCCESS:
            print(f"I2S初始化失败，错误代码: {i2s_init_result}")
            return
        else:
            print(f'设备打开成功')
    except:
        pass

    try:
        # 双路音频文件路径
        left_audio_file = r"D:\STM32OBJ\usb_test_obj\test_1\stm32h750ibt6\dox\16K_L.wav"
        right_audio_file = r"D:\STM32OBJ\usb_test_obj\test_1\stm32h750ibt6\dox\16K_R.wav"
        output_file = r"D:\STM32OBJ\usb_test_obj\test_1\stm32h750ibt6\dox\dual_combined.wav"
        if not os.path.exists(left_audio_file):
            print(f"错误: 左声道文件不存在: {left_audio_file}")
            return
        if not os.path.exists(right_audio_file):
            print(f"错误: 右声道文件不存在: {right_audio_file}")
            return
        # 配置双路音频参数
        dual_config = DUAL_AUDIO_CONFIG()
        dual_config.left_audio_path = left_audio_file.encode('utf-8')
        dual_config.right_audio_path = right_audio_file.encode('utf-8')
        dual_config.gap_duration = 1.0  # 右声道延迟1秒
        dual_config.generate_file = 1   # 生成合成音频文件
        dual_config.output_path = output_file.encode('utf-8')
        dual_config.left_volume = 20    # 左声道音量%  100%是原声，100以下是减少 ，100以上是放大
        dual_config.right_volume = 20   # 右声道音量%
        print("\n开始双路音频播放...")
        print(f"延迟时间: {dual_config.gap_duration}秒")
        print(f"左声道音量: {dual_config.left_volume}%")
        print(f"右声道音量: {dual_config.right_volume}%")
        print(f"生成合成文件: {'是' if dual_config.generate_file else '否'}")
        if dual_config.generate_file:
            print(f"输出文件: {output_file}")
        audio_result = usb_application.AudioStartDual(serial_param, byref(dual_config))
        if audio_result == 0:
            print("双路音频播放成功完成！")
            if dual_config.generate_file and os.path.exists(output_file):
                print(f"合成音频文件已生成: {output_file}")
        else:
            print(f"双路音频播放失败，错误代码: {audio_result}")
    except Exception as e:
        print(f"双路音频播放过程中发生错误: {e}")
    finally:
        print("\n关闭设备...")
        close_result = usb_application.USB_CloseDevice(serial_param)
        if close_result == 0:
            print("设备关闭成功")

if __name__ == "__main__":
    main()
