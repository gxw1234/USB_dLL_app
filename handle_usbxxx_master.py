
import datetime
import threading
import time
from handle_read_picture_data import *
import ctypes
import time
from ctypes import c_int, c_char, c_char_p, c_ubyte, c_ushort, c_uint, byref, Structure, POINTER, create_string_buffer
from ctypes import *
import os
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

from PIL import Image
from numpy import array

class MYThread_usbxxx_master(threading.Thread):
    def __init__(self, usb, event=threading.Event(), frame=96):
        super(MYThread_usbxxx_master, self).__init__()
        self.usbxxxxdevID = usb
        self.serial_param = usb.encode('utf-8')
        # 获取相对于当前工作目录的路径
        dll_path = os.path.join(os.getcwd(),"libs\\USB_G2X.dll")
        print(f"正在加载DLL: {dll_path}")

        # 检查DLL是否存在
        if not os.path.exists(dll_path):
            print(f"错误: DLL文件不存在: {dll_path}")
            return
        # 加载DLL
        self.usb_application = ctypes.CDLL(dll_path)
        print("成功加载DLL")
        self.gpio_scan_index = 0x05
        self.gpio_power_index = 0x09
        #自造结尾的图片
        self.end_picture = hex_images_end()

    def usb_open(self):
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
        result = self.usb_application.USB_ScanDevices(ctypes.byref(devices), max_devices)
        print(f"扫描结果: {result}")
        if result > 0:
            print(f"找到 {result} 个USB设备:")
            serial_list = list()
            for i in range(result):
                serial = devices[i].serial.decode('utf-8', errors='ignore').strip('\x00')
                desc = devices[i].description.decode('utf-8', errors='ignore').strip('\x00')
                manufacturer = devices[i].manufacturer.decode('utf-8', errors='ignore').strip('\x00')
                serial_list.append(serial)
                # 显示设备信息
                print(f"设备 {i + 1}:")
                print(f"  序列号: {serial}")
                print(f"  产品名称: {desc}")
                print(f"  制造商: {manufacturer}")

            if self.usbxxxxdevID not in serial_list:
                print(f"找不到USB2XXX设备！")
                return -1
        else:
            print("未扫描到设备")
            return -1
        print(f"尝试打开设备: {devices[0].serial.decode('utf-8', errors='ignore').strip('x00')}")
        self.serial_param = devices[0].serial  # 打开第一个设备
        # ===================================================
        # 函数: USB_OpenDevice
        # 描述: 打开指定序列号的USB设备
        # 参数:
        #   serial_param: 目标设备的序列号，如果为NULL则打开第一个可用设备
        # 返回值:
        #   >=0: 成功打开设备，返回设备句柄
        #   <0: 打开失败，返回错误代码
        # ===================================================
        handle = self.usb_application.USB_OpenDevice(self.serial_param)
        time.sleep(1)
        if handle >= 0:
            print(f"设备打开成功，句柄: {handle}")
            print("\n测试SPI初始化功能...")
            # 定义SPI相关常量
            SPI1_CS0 = 0
            #SPI_SUCCESS = 0
            # 创建SPI配置结构体
            spi_config = SPI_CONFIG()
            spi_config.Mode = 0  # 硬件控制（全双工模式）
            spi_config.Master = 1  # 主机模式
            spi_config.CPOL = 0
            spi_config.CPHA = 1
            spi_config.LSBFirst = 0  # MSB在前
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
            spi_init_result = self.usb_application.SPI_Init(self.serial_param, SPI1_CS0, byref(spi_config))
            if spi_init_result == 0:
                print(f"SPI初始化成功！")
                self.usb_application.GPIO_SetOpenDrain(self.serial_param, self.gpio_scan_index,
                                                  1)  # 设置为GPIO_MODE_OUTPUT_OD模式  //@param pull_mode 上拉下拉模式：0=无上拉下拉，1=上拉，2=下拉
                self.usb_application.GPIO_Write(self.serial_param, self.gpio_scan_index, 1)  # 默认状态 GPIO_PIN_RESET


            else:
                print(f"SPI初始化失败！")
                return -1
            time.sleep(1)
        return handle


    def read_image(self, File_path: str):
        if os.path.exists(File_path) == False:
            print("没有找到文件")
        else:
            self.images_data = hex_images(File_path)

    # 下压
    # PinMask  指定需要输出数据的引脚，该值每个bit位对应一个引脚，最低位对应P0。对应的bit位为1，则该引脚要根据PinValue对应位的值输出对应的电平状态。对应的bit位为0，则不影响该引脚的电平状态。
    # PinValue  根据PinMask的值控制引脚输出对应的电平状态，该数据每个bit位对应一个引脚，最低bit位对应P0，若对应的bit位为1且PinMask的对应bit位为1则该引脚输出高电平，若对应bit位为0且PinMask的对应位为1则该引脚输出低电平，否则不影响引脚输出的电平状态。
    def scan_down(self, PinMask: int = 0x05, PinValue: int = 0x00):
        print("===============下压=================")
        self.usb_application.GPIO_Write(self.serial_param, self.gpio_scan_index, 0)

    # 抬起
    # PinMask  指定需要输出数据的引脚，该值每个bit位对应一个引脚，最低位对应P0。对应的bit位为1，则该引脚要根据PinValue对应位的值输出对应的电平状态。对应的bit位为0，则不影响该引脚的电平状态。
    # PinValue  根据PinMask的值控制引脚输出对应的电平状态，该数据每个bit位对应一个引脚，最低bit位对应P0，若对应的bit位为1且PinMask的对应bit位为1则该引脚输出高电平，若对应bit位为0且PinMask的对应位为1则该引脚输出低电平，否则不影响引脚输出的电平状态。
    def scan_upward(self, PinMask: int = 0x05, PinValue: int = 0x02):
        print("===============抬起=================")
        self.usb_application.GPIO_Write(self.serial_param, self.gpio_scan_index, 1)


    # 上电
    # PinMask  指定需要输出数据的引脚，该值每个bit位对应一个引脚，最低位对应P0。对应的bit位为1，则该引脚要根据PinValue对应位的值输出对应的电平状态。对应的bit位为0，则不影响该引脚的电平状态。
    # PinValue  根据PinMask的值控制引脚输出对应的电平状态，该数据每个bit位对应一个引脚，最低bit位对应P0，若对应的bit位为1且PinMask的对应bit位为1则该引脚输出高电平，若对应bit位为0且PinMask的对应位为1则该引脚输出低电平，否则不影响引脚输出的电平状态。
    def Powe_on(self, PinMask: int = 0x02, PinValue: int = 0x00):
        print("===============上电=================")
        self.usb_application.GPIO_Write(self.serial_param, self.gpio_power_index, 1)

    # 断电
    # PinMask  指定需要输出数据的引脚，该值每个bit位对应一个引脚，最低位对应P0。对应的bit位为1，则该引脚要根据PinValue对应位的值输出对应的电平状态。对应的bit位为0，则不影响该引脚的电平状态。
    # PinValue  根据PinMask的值控制引脚输出对应的电平状态，该数据每个bit位对应一个引脚，最低bit位对应P0，若对应的bit位为1且PinMask的对应bit位为1则该引脚输出高电平，若对应bit位为0且PinMask的对应位为1则该引脚输出低电平，否则不影响引脚输出的电平状态。
    def Powe_off(self, PinMask: int = 0x02, PinValue: int = 0x02):
        print("===============断电=================")
        self.usb_application.GPIO_Write(self.serial_param, self.gpio_power_index, 0)

    def Single_frame_sen(self, data):
        self.usb_application.SPI_WriteBytes(self.serial_param, 0, data, len(data))
        time.sleep(0.007)

    def PoweroffPoweron(self):
        self.Powe_off()  # 断电
        time.sleep(4)
        self.Powe_on()  # 上电
        time.sleep(5)
    """
    下压，
    灌图，
    抬起  
    """
    def scan_start(self):
        #self.PoweroffPoweron()
        print(f'{datetime.datetime.now()}，准备灌图进入扫描(96帧)...')
        T1 = time.time()
        self.scan_down()  # 下压
        time.sleep(0.1)
        for i in self.images_data:
            self.Single_frame_sen(i)
        self.scan_upward()  # 抬起
        time.sleep(0.1)
        for i in self.end_picture:
            self.Single_frame_sen(i)
        T2 = time.time()
        print('已完成灌图。程序运行时间:%s毫秒' % ((T2 - T1) * 1000))

    def USB_CloseDevice(self):
        self.usb_application.USB_CloseDevice(self.serial_param)

if __name__ == '__main__':
    master_usb = MYThread_usbxxx_master("357A39533033")
    ret = master_usb.usb_open()
    if ret >= 0:
        print(f"USB2XXX打开成功")

    else:
        print(f"'USB2XXX打开失败")


    print("关闭")
    master_usb.USB_CloseDevice()
