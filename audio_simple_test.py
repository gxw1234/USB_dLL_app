#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
简化的音频播放测试程序
只需要一个AudioStart函数就能播放WAV文件
"""


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






import os
import ctypes
import time
from ctypes import c_int, c_char, c_char_p, c_void_p, c_uint, byref, Structure, POINTER, CFUNCTYPE


class I2S_CONFIG(Structure):
    _fields_ = [
        ("Mode", c_char),           # I2S模式:0-主机发送，1-主机接收，2-从机发送，3-从机接收
        ("Standard", c_char),       # I2S标准:0-飞利浦标准，1-MSB对齐，2-LSB对齐，3-PCM短帧，4-PCM长帧
        ("DataFormat", c_char),     # 数据格式:0-16位，1-24位，2-32位
        ("MCLKOutput", c_char),     # MCLK输出:0-禁用，1-启用
        ("AudioFreq", c_uint)       # 音频频率:8000,16000,22050,44100,48000,96000,192000
    ]

# 定义音频进度回调函数类型
AudioProgressCallback = CFUNCTYPE(None, c_uint, c_uint, c_void_p)

# 进度回调函数示例
def audio_progress_callback(current_chunk, total_chunks, user_data):
    """音频进度回调函数"""
    percent = (current_chunk / total_chunks) * 100
    bar_length = 40
    filled = int(bar_length * current_chunk / total_chunks)
    bar = '=' * filled + '-' * (bar_length - filled)
    print(f"\r播放进度: [{bar}] {percent:.1f}% ({current_chunk}/{total_chunks})", end='', flush=True)
    if current_chunk == total_chunks:
        print()  # 完成时换行

# 定义设备信息结构体
class DeviceInfo(Structure):
    _fields_ = [
        ("serial", c_char * 64),
        ("description", c_char * 128),
        ("manufacturer", c_char * 128),
        ("vendor_id", ctypes.c_ushort),
        ("product_id", ctypes.c_ushort),
        ("device_id", c_int)
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

    # 扫描USB设备
    max_devices = 10
    devices = (DeviceInfo * max_devices)()
    print("调用 USB_ScanDevice 函数...")
    result = usb_application.USB_ScanDevices(ctypes.byref(devices), max_devices)
    print(f"扫描结果: {result}")
    
    if result <= 0:
        print("未扫描到设备")
        return

    print(f"找到 {result} 个USB设备:")
    for i in range(result):
        serial = devices[i].serial.decode('utf-8', errors='ignore').strip('\x00')
        desc = devices[i].description.decode('utf-8', errors='ignore').strip('\x00')
        manufacturer = devices[i].manufacturer.decode('utf-8', errors='ignore').strip('\x00')
        print(f"设备 {i + 1}: {serial} - {desc} ({manufacturer})")

    # 打开第一个设备
    print(f"打开设备: {devices[0].serial.decode('utf-8', errors='ignore').strip('\x00')}")
    serial_param = devices[0].serial
    handle = usb_application.USB_OpenDevice(serial_param)
    time.sleep(1)
    if handle < 0:
        print(f"设备打开失败，错误代码: {handle}")
        return
    print(f"设备打开成功，句柄: {handle}")

    usb_application.USB_SetLogging(1)
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
        print("err")
    try:
        wav_file_path = r"D:\STM32OBJ\usb_test_obj\test_1\stm32h750ibt6\dox\combined_stereo.wav"
        print(f"使用WAV文件: {wav_file_path}")
        if not os.path.exists(wav_file_path):
            print(f"错误: WAV文件不存在: {wav_file_path}")
            return
        print("\n开始播放音频...")
        
        # # 可选：注册进度回调（不注册也可以正常播放）
        # callback_func = AudioProgressCallback(audio_progress_callback)  # 必须保持引用
        # usb_application.AudioSetProgressCallback(callback_func, None)


        audio_result = usb_application.AudioStart(serial_param, wav_file_path.encode('utf-8'), 20)
        if audio_result == 0:
            print("音频播放成功完成！")
        else:
            print(f"音频播放失败，错误代码: {audio_result}")
            if audio_result == -1:
                print("错误: 文件未找到")
            elif audio_result == -2:
                print("错误: 无效的音频格式")
            elif audio_result == -3:
                print("错误: 设备错误")
            else:
                print("错误: 其他错误")

    except Exception as e:
        print(f"音频播放过程中发生错误: {e}")
    finally:
        # 关闭设备
        print("\n关闭设备...")
        close_result = usb_application.USB_CloseDevice(serial_param)
        if close_result == 0:
            print("设备关闭成功")

if __name__ == "__main__":
    main()
