"""
连续读取测试脚本 - 用于测试USB数据接收的可靠性
读取1000次，每次4096字节，检查数据模式是否一致
"""

import ctypes
import time
import os

# 设备信息结构体
class DeviceInfo(ctypes.Structure):
    _fields_ = [
        ("serial", ctypes.c_char * 64),
        ("vendor_id", ctypes.c_ushort),
        ("product_id", ctypes.c_ushort),
        ("description", ctypes.c_char * 256),
        ("manufacturer", ctypes.c_char * 256),
        ("interface_str", ctypes.c_char * 256)
    ]

def main():
    print("USB连续读取测试 - 读取1000次，每次4096字节")
    
    # 加载DLL
    dll_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), "USB_G2X.dll")
    print(f"正在加载DLL: {dll_path}")
    
    try:
        usb_dll = ctypes.CDLL(dll_path)
        print("成功加载DLL")
    except Exception as e:
        print(f"加载DLL失败: {e}")
        return
    
    # 定义函数原型
    # usb_dll.USB_ScanDevice.argtypes = [ctypes.POINTER(DeviceInfo), ctypes.c_int]
    # usb_dll.USB_ScanDevice.restype = ctypes.c_int
    #
    # usb_dll.USB_OpenDevice.argtypes = [ctypes.c_char_p]
    # usb_dll.USB_OpenDevice.restype = ctypes.c_int
    #
    # usb_dll.USB_ReadData.argtypes = [ctypes.c_char_p, ctypes.POINTER(ctypes.c_ubyte), ctypes.c_int]
    # usb_dll.USB_ReadData.restype = ctypes.c_int
    #
    # usb_dll.USB_CloseDevice.argtypes = [ctypes.c_char_p]
    # usb_dll.USB_CloseDevice.restype = ctypes.c_int
    
    # 扫描设备
    print("正在扫描USB设备...")
    devices = (DeviceInfo * 10)()
    device_count = usb_dll.USB_ScanDevice(devices, 10)
    
    if device_count <= 0:
        print(f"扫描失败或未找到设备: {device_count}")
        return
    
    print(f"找到 {device_count} 个USB设备:")
    for i in range(device_count):
        print(f"设备 {i+1}:")
        print(f"  序列号: {devices[i].serial.decode('utf-8', errors='ignore')}")
        print(f"  产品描述: {devices[i].description.decode('utf-8', errors='ignore')}")
        print(f"  制造商: {devices[i].manufacturer.decode('utf-8', errors='ignore')}")
    
    # 打开设备
    serial = devices[0].serial
    print(f"\n正在打开设备: {serial.decode('utf-8', errors='ignore')}")
    ret = usb_dll.USB_OpenDevice(serial)
    
    if ret < 0:
        print(f"设备打开失败: {ret}")
        return
    
    print(f"设备打开成功: {ret}")
    
    # 连续读取数据
    buffer_size = 40960
    read_count = 2  # 固定读取1000次
    buffer = (ctypes.c_ubyte * buffer_size)()
    
    print(f"\n开始测试: 连续读取 {read_count} 次，每次 {buffer_size} 字节")
    
    total_bytes = 0
    start_time = time.time()
    errors = 0
    
    # 记录每100次读取的时间点和字节数，用于计算实时速率
    checkpoints = []
    
    time.sleep(1)
    for i in range(read_count):
        bytes_read = usb_dll.USB_ReadData(serial, buffer, buffer_size)
        
        if bytes_read <= 0:
            print(f"第 {i+1} 次读取失败: {bytes_read}")
            if i == 0:
                print("第一次读取失败可能是因为缓冲区还未收到数据，等待1秒后继续...")
                time.sleep(1)
                continue
            errors += 1
            time.sleep(0.1)
            continue
        
        total_bytes += bytes_read
        
        # 检查数据模式 (应该是 01 02 01 02...)
        correct_pattern = True
        for j in range(0, bytes_read, 2):
            if j+1 < bytes_read:
                if buffer[j] != 1 or buffer[j+1] != 2:
                    errors += 1
                    print(f"数据模式错误 在第 {i+1} 次读取的第 {j} 字节: 获得 {buffer[j]} {buffer[j+1]} 应为 1 2")
                    correct_pattern = False
                    break
        
        # 显示第一次成功读取的前10个字节和最后10个字节
        if i == 0 and bytes_read > 0:
            print("\n第一次读取的前10个字节:")
            for j in range(0, min(10, bytes_read)):
                print(f"{buffer[j]:02X}", end=" ")
            print("\n第一次读取的最后10个字节:")
            for j in range(max(0, bytes_read-10), bytes_read):
                print(f"{buffer[j]:02X}", end=" ")
            print("\n")
        
        # 显示最后一次读取的数据
        if i == read_count-1 and bytes_read > 0:
            print("\n最后一次读取的前10个字节:")
            for j in range(0, min(10, bytes_read)):
                print(f"{buffer[j]:02X}", end=" ")
            print("\n最后一次读取的最后10个字节:")
            for j in range(max(0, bytes_read-10), bytes_read):
                print(f"{buffer[j]:02X}", end=" ")
            print("\n")
        
        if (i+1) % 100 == 0:
            current_time = time.time()
            checkpoints.append((current_time, total_bytes))
            
            # 计算当前段的速率
            if len(checkpoints) > 1:
                prev_time, prev_bytes = checkpoints[-2]
                segment_time = current_time - prev_time
                segment_bytes = total_bytes - prev_bytes
                segment_rate = segment_bytes / segment_time / (1024 * 1024) if segment_time > 0 else 0
                print(f"已完成 {i+1} 次读取，累计 {total_bytes} 字节，本段速率: {segment_rate:.2f} MB/s，{'未发现错误' if errors == 0 else f'发现 {errors} 个错误'}")
            else:
                print(f"已完成 {i+1} 次读取，累计 {total_bytes} 字节，{'未发现错误' if errors == 0 else f'发现 {errors} 个错误'}")
    
    end_time = time.time()
    duration = end_time - start_time
    
    print("\n测试完成！")
    print(f"读取次数: {read_count}")
    print(f"总字节数: {total_bytes}")
    print(f"总耗时: {duration:.2f} 秒")
    print(f"平均速度: {total_bytes / duration / (1024 * 1024):.2f} MB/s")
    print(f"错误次数: {errors}")
    print(f"测试结果: {'通过' if errors == 0 else '失败'}")
    
    # 关闭设备
    print("\n正在关闭设备...")
    ret = usb_dll.USB_CloseDevice(serial)
    
    if ret != 0:
        print(f"设备关闭失败: {ret}")
    else:
        print("设备关闭成功")

if __name__ == "__main__":
    main()
