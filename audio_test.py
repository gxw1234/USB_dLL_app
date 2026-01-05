#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
I2S音频传输测试程序
读取WAV音频文件，实现音频数据的队列传输和I2S播放
"""

import os
import ctypes
import time
import struct
from ctypes import c_int, c_char, c_char_p, c_ubyte, c_ushort, c_uint, byref, Structure, POINTER, create_string_buffer

# =============================================================================
# I2S 配置宏定义 (对应C语言中的宏定义)
# =============================================================================

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

# 定义I2S配置结构体
class I2S_CONFIG(Structure):
    _fields_ = [
        ("Mode", c_char),           # I2S模式:0-主机发送，1-主机接收，2-从机发送，3-从机接收
        ("Standard", c_char),       # I2S标准:0-飞利浦标准，1-MSB对齐，2-LSB对齐，3-PCM短帧，4-PCM长帧
        ("DataFormat", c_char),     # 数据格式:0-16位，1-24位，2-32位
        ("MCLKOutput", c_char),     # MCLK输出:0-禁用，1-启用
        ("AudioFreq", c_uint)       # 音频频率:8000,16000,22050,44100,48000,96000,192000
    ]


BUFFSIZE = 1280


def read_wav_file(file_path, chunk_size=BUFFSIZE):
    """
    读取WAV音频文件并分块，跳过WAV文件头
    
    Args:
        file_path: WAV文件路径
        chunk_size: 每个音频块的大小 (字节)
    
    Returns:
        list: 音频数据块列表，每个块为ctypes数组
    """
    if not os.path.exists(file_path):
        print(f"错误: WAV文件不存在: {file_path}")
        return []
    
    file_size = os.path.getsize(file_path)
    print(f"WAV文件大小: {file_size} 字节")
    
    audio_chunks = []
    
    try:
        with open(file_path, 'rb') as f:
            # 读取WAV文件头
            riff_header = f.read(12)  # "RIFF" + file_size + "WAVE"
            if riff_header[:4] != b'RIFF' or riff_header[8:12] != b'WAVE':
                print("错误: 不是有效的WAV文件")
                return []
            
            # 查找"fmt "块
            while True:
                chunk_header = f.read(8)
                if len(chunk_header) < 8:
                    print("错误: WAV文件格式不正确")
                    return []
                
                chunk_id = chunk_header[:4]
                chunk_size_bytes = struct.unpack('<I', chunk_header[4:8])[0]
                
                if chunk_id == b'fmt ':
                    # 读取格式信息
                    fmt_data = f.read(chunk_size_bytes)
                    if len(fmt_data) >= 16:
                        audio_format, channels, sample_rate, byte_rate, block_align, bits_per_sample = struct.unpack('<HHIIHH', fmt_data[:16])
                        print(f"WAV格式信息:")
                        print(f"  音频格式: {audio_format} (1=PCM)")
                        print(f"  声道数: {channels}")
                        print(f"  采样率: {sample_rate} Hz")
                        print(f"  位深度: {bits_per_sample} 位")
                        
                        if audio_format != 1:
                            print("错误: 只支持PCM格式的WAV文件")
                            return []
                elif chunk_id == b'data':
                    # 找到数据块，开始读取PCM数据
                    print(f"找到数据块，大小: {chunk_size_bytes} 字节")
                    
                    chunk_count = 0
                    while True:
                        chunk_data = f.read(chunk_size)
                        if not chunk_data:
                            break
                        
                        # 如果最后一块不足5120字节，用零填充
                        if len(chunk_data) < chunk_size:
                            chunk_data += b'\x00' * (chunk_size - len(chunk_data))
                            print(f"最后一块数据不足{chunk_size}字节，已用零填充")
                        
                        # 转换为ctypes数组
                        audio_array = (c_ubyte * len(chunk_data))(*chunk_data)
                        audio_chunks.append(audio_array)
                        chunk_count += 1
                        
                        if chunk_count % 100 == 0:
                            print(f"已读取 {chunk_count} 个音频块...")
                    
                    print(f"总共读取了 {chunk_count} 个音频块，每块 {chunk_size} 字节")
                    break
                else:
                    # 跳过其他块
                    f.seek(chunk_size_bytes, 1)
            
    except Exception as e:
        print(f"读取WAV文件时发生错误: {e}")
        return []
    
    return audio_chunks

def get_wav_info(file_path):
    """
    获取WAV文件信息
    
    Args:
        file_path: WAV文件路径
    
    Returns:
        dict: WAV文件信息
    """
    if not os.path.exists(file_path):
        return None
    
    try:
        with open(file_path, 'rb') as f:
            # 读取WAV文件头
            riff_header = f.read(12)
            if riff_header[:4] != b'RIFF' or riff_header[8:12] != b'WAVE':
                return None
            
            # 查找"fmt "块
            while True:
                chunk_header = f.read(8)
                if len(chunk_header) < 8:
                    return None
                
                chunk_id = chunk_header[:4]
                chunk_size_bytes = struct.unpack('<I', chunk_header[4:8])[0]
                
                if chunk_id == b'fmt ':
                    fmt_data = f.read(chunk_size_bytes)
                    if len(fmt_data) >= 16:
                        audio_format, channels, sample_rate, byte_rate, block_align, bits_per_sample = struct.unpack('<HHIIHH', fmt_data[:16])
                        
                        # 查找数据块大小
                        while True:
                            data_chunk_header = f.read(8)
                            if len(data_chunk_header) < 8:
                                return None
                            
                            data_chunk_id = data_chunk_header[:4]
                            data_chunk_size = struct.unpack('<I', data_chunk_header[4:8])[0]
                            
                            if data_chunk_id == b'data':
                                # 计算时长和块数
                                bytes_per_sample = (bits_per_sample // 8) * channels
                                total_samples = data_chunk_size // bytes_per_sample
                                duration_seconds = total_samples / sample_rate
                                
                                chunk_size = 5120
                                chunk_duration_ms = (chunk_size // bytes_per_sample) / sample_rate * 1000
                                total_chunks = (data_chunk_size + chunk_size - 1) // chunk_size
                                

                                info = {
                                    'file_size': os.path.getsize(file_path),
                                    'data_size': data_chunk_size,
                                    'sample_rate': sample_rate,
                                    'channels': channels,
                                    'bit_depth': bits_per_sample,
                                    'audio_format': audio_format,
                                    'total_samples': total_samples,
                                    'duration_seconds': duration_seconds,
                                    'chunk_size': chunk_size,
                                    'chunk_duration_ms': chunk_duration_ms,
                                    'total_chunks': total_chunks
                                }
                                return info
                            else:
                                f.seek(data_chunk_size, 1)
                else:
                    f.seek(chunk_size_bytes, 1)
                    
    except Exception as e:
        print(f"解析WAV文件时发生错误: {e}")
        return None




class DeviceInfoDetail(Structure):
    _fields_ = [
        ("FirmwareName", c_char * 32),  # 固件名称字符串
        ("BuildDate", c_char * 32),  # 固件编译时间字符串
        ("HardwareVersion", c_int),  # 硬件版本号
        ("FirmwareVersion", c_int),  # 固件版本号
        ("SerialNumber", c_int * 3),  # 适配器序列号
        ("Functions", c_int)  # 适配器当前具备的功能
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


    usb_application = ctypes.CDLL(dll_path)
    print("成功加载DLL")
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
    print("\n测试I2S音频传输功能...")
    try:
        # I2S配置 - 使用宏定义
        i2s_config = I2S_CONFIG()
        i2s_config.Mode = I2S_MODE_MASTER_TX        # 主机发送模式
        i2s_config.Standard = I2S_STANDARD_PHILIPS   # 飞利浦标准
        i2s_config.DataFormat = I2S_DATAFORMAT_16B  # 16位数据格式
        i2s_config.MCLKOutput = I2S_MCLKOUTPUT_ENABLE  # 启用MCLK输出
        i2s_config.AudioFreq = I2S_AUDIOFREQ_16K    # 16kHz采样率
        # i2s_config.AudioFreq = I2S_AUDIOFREQ_48K    # 48kHz采样率
        i2s_init_result = usb_application.I2S_Init(serial_param, I2S_INDEX_1, byref(i2s_config))


        print("\n获取设备详细信息...")
        dev_info = DeviceInfoDetail()
        func_str = create_string_buffer(256)  # 创建功能字符串缓冲区

        time.sleep(1)
        if 1:
            if i2s_init_result != I2S_SUCCESS:
                print(f"I2S初始化失败，错误代码: {i2s_init_result}")
                return
            wav_file_path = r"D:\STM32OBJ\usb_test_obj\test_1\stm32h750ibt6\dox\R.wav"
            # wav_file_path = r"D:\STM32OBJ\usb_test_obj\test_1\stm32h750ibt6\dox\48_xl.wav"
            print(f"使用固定WAV文件: {wav_file_path}")
            if not os.path.exists(wav_file_path):
                print(f"错误: WAV文件不存在: {wav_file_path}")
                return
            wav_info = get_wav_info(wav_file_path)
            if wav_info:
                print(f"\nWAV文件信息:")
                print(f"  文件大小: {wav_info['file_size']} 字节")
                print(f"  数据大小: {wav_info['data_size']} 字节")
                print(f"  采样率: {wav_info['sample_rate']} Hz")
                print(f"  声道数: {wav_info['channels']}")
                print(f"  位深度: {wav_info['bit_depth']} 位")
                print(f"  音频格式: {wav_info['audio_format']}")
                print(f"  总时长: {wav_info['duration_seconds']:.2f} 秒")
                print(f"  音频块数: {wav_info['total_chunks']}")
                print(f"  每块时长: {wav_info['chunk_duration_ms']:.1f} 毫秒")

            print(f"\n读取WAV文件: {os.path.basename(wav_file_path)}")
            audio_chunks = read_wav_file(wav_file_path)

            if not audio_chunks:
                print("读取WAV文件失败")
                return

            print(f"成功读取 {len(audio_chunks)} 个音频块")

            print("启动I2S音频队列...")
            usb_application.I2S_SetVolume(serial_param, I2S_INDEX_1, 10)
            queue_start_result = usb_application.I2S_StartQueue(serial_param, I2S_INDEX_1)
            if queue_start_result != I2S_SUCCESS:
                print(f"启动I2S队列失败，错误代码: {queue_start_result}")
                return

            write_buffer_size = 10
            write_buffer = (c_ubyte * write_buffer_size)()
            for i in range(write_buffer_size):
                write_buffer[i] = i
            print("开始WAV音频传输...")
            # 先发送前8个音频块，让队列有数据持续播放
            initial_chunks = min(8, len(audio_chunks))
            print(f"预填充音频队列 (前{initial_chunks}个音频块)...")

            for i in range(initial_chunks):
                ret = usb_application.I2S_Queue_WriteBytes(serial_param, I2S_INDEX_1, audio_chunks[i], len(audio_chunks[i]) )
            #发送剩余的音频块
            queue_status = usb_application.I2S_GetQueueStatus(serial_param, I2S_INDEX_1)
            # time.sleep(1)
            print(f'--------------queue_status:{queue_status}')
            remaining_chunks = len(audio_chunks) - initial_chunks

            if 1:
                if remaining_chunks > 0:
                    print(f"发送剩余的 {remaining_chunks} 个音频块...")
                    for i in range(initial_chunks, len(audio_chunks)):
                        # 检查队列状态，如果大于7就等待
                        while True:
                            queue_status = usb_application.I2S_GetQueueStatus(serial_param, I2S_INDEX_1)
                            if 0 <= queue_status <= 7:
                                break  # 队列有空间，可以发送
                            elif queue_status > 7:
                                time.sleep(0.01)  # 队列满，等待10ms
                            else:
                                print(f"队列状态查询失败: {queue_status}")
                                break
                        # 发送音频块
                        ret = usb_application.I2S_Queue_WriteBytes(serial_param, I2S_INDEX_1, audio_chunks[i], len(audio_chunks[i]) )
                        if ret == 0:
                            if (i + 1) % 50 == 0 or i == len(audio_chunks) - 1:
                                print(f"成功发送第 {i+1} 个音频块")
                        else:
                            print(f"发送第 {i+1} 个音频块失败，状态码: {ret}")
                # 等待音频队列完全播放完成
                print("等待WAV音频播放完成...")

            print("WAV音频播放完成")
    except Exception as e:
        print(f"音频传输过程中发生错误: {e}")
    finally:
        # 关闭设备
        print("\n关闭设备...")
        close_result = usb_application.USB_CloseDevice(serial_param)
        if close_result == 0:
            print("设备关闭成功")

if __name__ == "__main__":
    main()
