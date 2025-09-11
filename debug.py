#!/usr/bin/env python
# -*- coding: utf-8 -*-

"""
接线
KEY接： P5
CS接：  P6
MO接：  P3
SCLK接： P4
SCL :   P25
SDA  :P19
"""
from PIL import Image
import os
import ctypes
import time
from ctypes import c_int, c_char, c_char_p, c_ubyte, c_ushort, c_uint, byref, Structure, POINTER, create_string_buffer
from ctypes import *

SPI_FIRSTBIT_LSB = 1
SPI_FIRSTBIT_MSB = 0
SPI_MODE_MASTER = 1
SPI_MODE_SLAVE = 0
SPI_POLARITY_HIGH = 1
SPI_POLARITY_LOW = 0
SPI_PHASE_2EDGE = 1
SPI_PHASE_1EDGE = 0



SMOKE_DATA =[0 for i in range(96*240)]
data_list = []
data_list.append((c_ubyte * len(SMOKE_DATA))(*SMOKE_DATA))
data_list.append((c_ubyte * len(SMOKE_DATA))(*SMOKE_DATA))
data_list.append((c_ubyte * len(SMOKE_DATA))(*SMOKE_DATA))
data_list.append((c_ubyte * len(SMOKE_DATA))(*SMOKE_DATA))
data_list.append((c_ubyte * len(SMOKE_DATA))(*SMOKE_DATA))
data_list.append((c_ubyte * len(SMOKE_DATA))(*SMOKE_DATA))








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
        ("Mode", c_char),  # SPI控制方式
        ("Master", c_char),  # 主从选择控制
        ("CPOL", c_char),  # 时钟极性控制
        ("CPHA", c_char),  # 时钟相位控制
        ("LSBFirst", c_char),  # 数据移位方式
        ("SelPolarity", c_char),  # 片选信号极性
        ("ClockSpeedHz", c_uint)  # SPI时钟频率
    ]


# 定义电压配置结构体
class VOLTAGE_CONFIG(Structure):
    _fields_ = [
        ("channel", c_ubyte),  # 电源通道
        ("voltage", c_ushort)  # 电压值（单位：mV）
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
        flat_img_array = img_array.flatten()
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
    result = usb_application.USB_ScanDevices(ctypes.byref(devices), max_devices)
    print(f"扫描结果: {result}")
    if result > 0:
        print(f"找到 {result} 个USB设备:")
        for i in range(result):
            serial = devices[i].serial.decode('utf-8', errors='ignore').strip('\x00')
            desc = devices[i].description.decode('utf-8', errors='ignore').strip('\x00')
            manufacturer = devices[i].manufacturer.decode('utf-8', errors='ignore').strip('\x00')
            # 显示设备信息
            print(f"设备 {i + 1}:")
            print(f"  序列号: {serial}")
            print(f"  产品名称: {desc}")
            print(f"  制造商: {manufacturer}")
            print()
    else:
        print("未扫描到设备")
        return

    print(f"尝试打开设备: {devices[0].serial.decode('utf-8', errors='ignore').strip('\x00')}")
    serial_param = devices[0].serial  # 打开第一个设备






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
        SPI1_CS0 = 1
        SPI_SUCCESS = 0

        # 创建SPI配置结构体
        spi_config = SPI_CONFIG()
        spi_config.Mode = 0  # 硬件控制（全双工模式）
        spi_config.Master = SPI_MODE_MASTER  # 主机模式
        spi_config.CPOL = SPI_POLARITY_LOW
        spi_config.CPHA = SPI_PHASE_2EDGE
        spi_config.LSBFirst = SPI_FIRSTBIT_LSB
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
        time.sleep(1)
        key_gpio_index = 0x05
        usb_application.GPIO_SetOpenDrain(serial_param, key_gpio_index,
                                          1)  # 设置为GPIO_MODE_OUTPUT_OD模式  //@param pull_mode 上拉下拉模式：0=无上拉下拉，1=上拉，2=下拉
        usb_application.GPIO_Write(serial_param, key_gpio_index, 1)  # 默认状态 GPIO_PIN_RESET
        print(f'初始化')
        if spi_init_result == SPI_SUCCESS:
            print("成功发送SPI初始化命令")
            SPIIndex = SPI1_CS0
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
            path = r'D:\py\autoScan\2\0019_img_0019_out'

            # path = r'D:\py\autoScan\2\0019_img_0019_out - 副本'
            # path = r'D:\py\autoScan\2\1'
            images = hex_images(path)
            # 启动队列

            # ===================================================
            # SPI图像传输队列流程说明:
            # 1. 启动SPI队列，队列可缓存10张图像
            # 2. GPIO下压操作，等待IIC响应确认后再开始图像传输
            # 3. 先发送8张图像到队列，让队列有数据持续传输
            # 4. 查询队列状态，如果剩余图像少于7张，继续补充图像
            # 5. 发送完成后再次查询队列状态确认传输完成
            # 
            # 队列工作原理:
            # - 队列容量: 10张图像 (MAX_QUEUE_SIZE = 10)
            # - GPIO同步: 启动队列后执行GPIO_scan_Write下压操作，等待IIC响应确认
            # - 预填充: 先发送8张图像确保队列有足够数据
            # - 动态补充: 当队列剩余<7张时补充新图像
            # - 状态监控: 通过SPI_GetQueueStatus查询队列状态
            # - 固定延时: 每张图像传输完成后有硬件定时器做精准的延时
            #   这个延时确保图像数据完全处理完成，避免数据冲突
            # ===================================================
            # 启动队列
            usb_application.SPI_StartQueue(serial_param, SPIIndex)
            # ret = usb_application.GPIO_scan_Write(serial_param, key_gpio_index, 0)  # 用同步接口，下压之接等IIC回复才退出
            # print(f'下压状态：{ret}')
            if 1:

                Count =0
                for  i in range(1000):
                    Count+1
                    print(f'下压')
                    # ret = usb_application.GPIO_Write(serial_param, key_gpio_index, 0)  #用同步接口，下压之接等IIC回复才退出
                    # time.sleep(0.2)
                    ret = usb_application.GPIO_scan_Write(serial_param, key_gpio_index, 0)  #用同步接口，下压之接等IIC回复才退出
                    print(f'下压状态：{ret}')
                    # "先发送前8张图像...  因为有队列可以存十张图，先让队列有数据一直传")
                    for i in range(min(8, len(images))):
                        usb_application.SPI_Queue_WriteBytes(serial_param, SPIIndex, images[i], len(images[i]))
                    # 发送剩余的图像
                    remaining_images = len(images) - 8
                    if remaining_images > 0:
                        # print(f"开始发送剩余的 {remaining_images} 张图像...")
                        for i in range(8, len(images)):
                            # 检查队列状态，如果大于8就等待
                            while True:
                                queue_status = usb_application.SPI_GetQueueStatus(serial_param, SPIIndex)
                                # print(f"当前队列状态: {queue_status}")
                                if 0 <= queue_status <= 7:
                                    break  # 如果队列状态在 0 到 7 之间，退出循环
                                elif queue_status > 7:
                                    time.sleep(0.003)  # 如果队列状态大于 7，等待 3ms 再次检查
                            # 发送图像
                            ret = usb_application.SPI_Queue_WriteBytes(serial_param, SPIIndex, images[i], len(images[i]))
                            # if ret == 0:
                            #     print(f"成功发送第 {i+1} 张图像")
                            # else:
                            #     print(f"发送第 {i+1} 张图像失败，错误代码: {ret}")
                    # 等待队列完全清空后再抬起按键
                    print("等待队列完全清空...")
                    previous_status = -1  # 初始化上一个状态为-1，确保第一次循环不会直接退出
                    while True:
                        queue_status = usb_application.SPI_GetQueueStatus(serial_param, SPIIndex)
                        if queue_status == 0 and previous_status == 0:
                            break
                        previous_status = queue_status

                    usb_application.GPIO_Write(serial_param, key_gpio_index, 1)  # 抬起按键
                    print("队列已清空，所有图像发送完成")
                    # time.sleep(0.01)
                    for i in images[-5:]:
                        usb_application.SPI_Queue_WriteBytes(serial_param, SPIIndex, i, len(i))
                    if Count % 2 == 0:
                        time.sleep(10)
                        print("11111")
                    else:
                        time.sleep(2)
                        print("22222222")


            if 0:
                write_buffer_size = 240*96
                write_buffer = (c_ubyte * write_buffer_size)()
                for i in range(write_buffer_size):
                    write_buffer[i] = 1
                a =time.time()




                for i in range(8):
                    write_result = usb_application.SPI_Queue_WriteBytes(serial_param, SPIIndex, write_buffer, len(write_buffer))

                ret = usb_application.SPI_GetQueueStatus(serial_param, 1)
                print(f'查询状态：{ret}')

                # write_result = usb_application.SPI_Queue_WriteBytes(serial_param, SPIIndex, write_buffer, len(write_buffer))
                # write_result = usb_application.SPI_Queue_WriteBytes(serial_param, SPIIndex, write_buffer, len(write_buffer))
                # write_result = usb_application.SPI_Queue_WriteBytes(serial_param, SPIIndex, write_buffer, len(write_buffer))
                # write_result = usb_application.SPI_Queue_WriteBytes(serial_param, SPIIndex, write_buffer, len(write_buffer))
                # write_result = usb_application.SPI_Queue_WriteBytes(serial_param, SPIIndex, write_buffer, len(write_buffer))
                # ret = usb_application.SPI_Queue_WriteBytes(serial_param, SPIIndex, images[1],len(images[1]))
                # print(f'ret:{ret}')
                # ret = usb_application.SPI_Queue_WriteBytes(serial_param, SPIIndex, images[1], len(images[1]))
                # print(f'ret:{ret}')
                # ret = usb_application.SPI_Queue_WriteBytes(serial_param, SPIIndex, images[1], len(images[1]))
                # print(f'ret:{ret}')
                # ret = usb_application.SPI_Queue_WriteBytes(serial_param, SPIIndex, images[1], len(images[1]))
                # print(f'ret:{ret}')
                # ret = usb_application.SPI_Queue_WriteBytes(serial_param, SPIIndex, images[1], len(images[1]))
                #
                # usb_application.GPIO_Write(serial_param, key_gpio_index, 1)  # 默认状态 GPIO_PIN_RESET
            if 0:
                for i in range(1):
                    queue_start_result = usb_application.SPI_StartQueue(serial_param, SPIIndex)
                    if queue_start_result == 0:
                        print("成功启动SPI队列")
                    else:
                        print(f"启动SPI队列失败，错误代码: {queue_start_result}")
                        return
                    T1 = time.time()
                    usb_application.GPIO_scan_Write(serial_param, key_gpio_index, 0)  # 下压按键
                    # time.sleep(0.1)
                    # 使用队列控制的图像发送
                    image_index = 0
                    total_images = len(images)
                    sent_count = 0
                    max_send_count = 92
                    print(f"开始发送 {max_send_count} 张图像...")
                    for i in range(max_send_count):
                        max_retries = 3  # 重试次数
                        retry_count = 0
                        while retry_count <= max_retries:
                            ret = usb_application.SPI_Queue_WriteBytes(serial_param, SPIIndex, images[image_index],
                                                                       len(images[image_index]))
                            if ret == 0:
                                sent_count += 1
                                break
                            else:
                                # 丢帧，校验失败，重试
                                retry_count += 1
                                if retry_count <= max_retries:
                                    print(f"图像 {i + 1} 发送失败 (ret: {ret})，第 {retry_count} 次重试...")
                                    time.sleep(0.01)  # 重试前等待10ms
                                else:
                                    print(f"图像 {i + 1} 发送失败，已达到最大重试次数 {max_retries}")
                        image_index += 1
                        if image_index >= len(images):
                            image_index = 0  # 循环使用图像
                    usb_application.GPIO_Write(serial_param, key_gpio_index, 1)  # 抬起按键
                    print(f'发送完成，总用时: {time.time() - T1:.3f}秒，成功发送: {sent_count}/{total_images} 张图像')
                    for i in images[-5:]:
                        usb_application.SPI_WriteBytes(serial_param, SPIIndex, i, len(i))
                    time.sleep(5)
                    print("SPI队列已停止")

        else:
            print(f"SPI初始化失败，错误代码: {1}")

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
