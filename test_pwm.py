#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
PWM输入捕获测试程序
测试STM32H750的PWM输入捕获功能

硬件连接：
- PWM通道1: PA8 (TIM1_CH1) - 连接PWM信号源
- PWM通道2: PA9 (TIM1_CH2) - 连接PWM信号源

功能：
- 初始化PWM输入捕获
- 开始PWM测量
- 读取PWM参数（频率、占空比、周期、脉宽）
- 停止PWM测量
"""

import ctypes
import time
import sys
import os

# 加载DLL
try:
    # 获取当前脚本目录
    current_dir = os.path.dirname(os.path.abspath(__file__))
    dll_path = os.path.join(current_dir, "USB_G2X.dll")
    
    if not os.path.exists(dll_path):
        print(f"错误: 找不到DLL文件: {dll_path}")
        sys.exit(1)
    
    usb_dll = ctypes.CDLL(dll_path)
    print(f"✅ 成功加载DLL: {dll_path}")
except Exception as e:
    print(f"❌ 加载DLL失败: {e}")
    sys.exit(1)

# 定义设备信息结构体（与GPIO测试程序保持一致）
class DeviceInfo(ctypes.Structure):
    _fields_ = [
        ("serial", ctypes.c_char * 64),
        ("description", ctypes.c_char * 128),
        ("manufacturer", ctypes.c_char * 128),
        ("vendor_id", ctypes.c_ushort),
        ("product_id", ctypes.c_ushort),
        ("device_id", ctypes.c_int)
    ]

# 定义PWM定时器配置结构体
class PWMTimerConfig(ctypes.Structure):
    _fields_ = [
        ("prescaler", ctypes.c_uint32),           # 预分频器 (0-65535)
        ("period", ctypes.c_uint32),              # 计数周期 (0-65535 for 16-bit timer)
        ("counter_mode", ctypes.c_uint32),        # 计数模式 (0=UP, 1=DOWN, 2=CENTER_ALIGNED1, etc.)
        ("clock_division", ctypes.c_uint32),      # 时钟分频 (0=DIV1, 1=DIV2, 2=DIV4)
        ("auto_reload_preload", ctypes.c_uint32)  # 自动重载预装载 (0=DISABLE, 1=ENABLE)
    ]

# 定义PWM测量结果结构体
class PWMMeasureResult(ctypes.Structure):
    _fields_ = [
        ("frequency", ctypes.c_uint32),      # 频率 (Hz)
        ("duty_cycle", ctypes.c_uint32),     # 占空比 (0-10000, 表示0.00%-100.00%)
        ("period_us", ctypes.c_uint32),      # 周期 (微秒)
        ("pulse_width_us", ctypes.c_uint32)  # 脉冲宽度 (微秒)
    ]



def scan_devices():
    """扫描USB设备"""
    print("\n🔍 扫描USB设备...")
    devices = (DeviceInfo * 10)()  # 最多10个设备
    count = usb_dll.USB_ScanDevices(devices, 10)
    
    if count <= 0:
        print("❌ 未找到设备")
        return None
    
    print(f"✅ 找到 {count} 个设备:")
    for i in range(count):
        serial = devices[i].serial.decode('utf-8')
        description = devices[i].description.decode('utf-8')
        print(f"  设备{i+1}: {description} (序列号: {serial})")
    
    return devices[0].serial.decode('utf-8')  # 返回第一个设备的序列号

def test_pwm_simple(serial):
    """简单PWM测试 - 只测试通道1"""
    print("\n🔧 PWM通道1测试...")
    
    # 创建PWM定时器配置结构体
    pwm_config = PWMTimerConfig()
    pwm_config.prescaler = 240 - 1      # 240MHz / 240 = 1MHz (1us分辨率)
    pwm_config.period = 0xFFFF          # 16位计数器
    pwm_config.counter_mode = 0         # TIM_COUNTERMODE_UP
    pwm_config.clock_division = 0       # TIM_CLOCKDIVISION_DIV1
    pwm_config.auto_reload_preload = 0  # TIM_AUTORELOAD_PRELOAD_DISABLE
    
    print(f"📋 定时器配置:")
    print(f"   预分频器: {pwm_config.prescaler + 1} (时钟: {240000000 // (pwm_config.prescaler + 1) / 1000000:.1f}MHz)")
    print(f"   计数周期: {pwm_config.period}")
    print(f"   分辨率: {1000000 / (240000000 // (pwm_config.prescaler + 1)):.3f}us")
    
    # 1. 初始化PWM通道1 (带配置参数)
    ret = usb_dll.PWM_Init(serial.encode('utf-8'), 1, ctypes.byref(pwm_config))
    if ret != 0:
        print(f"❌ PWM初始化失败: {ret}")
        return
    print("✅ PWM初始化成功")
    
    # 2. 开始测量
    ret = usb_dll.PWM_StartMeasure(serial.encode('utf-8'), 1)
    if ret != 0:
        print(f"❌ 开始PWM测量失败: {ret}")
        return
    print("✅ PWM测量已开始")
    
    # 3. 等待3秒后读取结果
    print("等待3秒...")
    time.sleep(3)
    
    # 4. 读取一次结果
    result = PWMMeasureResult()
    ret = usb_dll.PWM_GetResult(serial.encode('utf-8'), 1, ctypes.byref(result))
    
    print(f"📊 PWM测量结果:")
    print(f"   频率: {result.frequency} Hz")
    print(f"   占空比: {result.duty_cycle / 100.0:.2f}%")
    print(f"   周期: {result.period_us} μs")
    print(f"   脉宽: {result.pulse_width_us} μs")
    
    # 5. 停止测量
    usb_dll.PWM_StopMeasure(serial.encode('utf-8'), 1)
    print("✅ PWM测量已停止")

def main():
    """主函数"""
    print("🚀 PWM输入捕获测试程序")
    print("=" * 50)
    
    # 1. 扫描设备
    serial = scan_devices()
    if not serial:
        return
    
    # 2. 打开设备
    print(f"\n📱 打开设备: {serial}")
    ret = usb_dll.USB_OpenDevice(serial.encode('utf-8'))
    if ret != 0:
        print(f"❌ 打开设备失败: {ret}")
        return
    print("✅ 设备打开成功")
    
    try:
        print("\n📋 PWM测试说明:")
        print("  - PWM通道1: PA8 (TIM1_CH1)")
        print("  - 请确保PA8引脚已连接PWM信号源")
        
        # 3. 直接开始PWM测试
        test_pwm_simple(serial)
    
    finally:
        # 5. 关闭设备
        print(f"\n📱 关闭设备...")
        usb_dll.USB_CloseDevice(serial.encode('utf-8'))
        print("✅ 设备已关闭")

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\n\n⚠️  用户中断程序")
    except Exception as e:
        print(f"\n❌ 程序异常: {e}")
    
    print("\n程序结束")