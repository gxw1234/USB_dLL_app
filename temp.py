# Python示例
import ctypes
from ctypes import Structure, c_char, c_ushort, c_int

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

# 扫描设备
usb_dll = ctypes.CDLL(r"D:\STM32OBJ\usb_test_obj\test_1\USB_dLL_app\USB_G2X.dll")
max_devices = 10
devices = (DeviceInfo * max_devices)()
result = usb_dll.USB_ScanDevices(ctypes.byref(devices), max_devices)

if result >= 0:
    print(f"找到 {result} 个设备")
    for i in range(result):
        device = devices[i]
        print(f"设备{i}: {device.serial.decode('utf-8')}")
else:
    print(f"扫描失败，错误码: {result}")


# 定义设备详细信息结构体
class DeviceInfoDetail(Structure):
    _fields_ = [
        # DLL信息
        ("DllName", c_char * 32),
        ("DllBuildDate", c_char * 32),
        ("DllVersion", c_int),
        # STM32设备固件信息
        ("FirmwareName", c_char * 32),
        ("FirmwareBuildDate", c_char * 32),
        ("HardwareVersion", c_int),
        ("FirmwareVersion", c_int),
        # 设备信息
        ("SerialNumber", c_int * 3),
        ("Functions", c_int)
    ]


# 设置函数参数类型
usb_dll.USB_GetDeviceInfo.argtypes = [ctypes.c_char_p, POINTER(DeviceInfoDetail), ctypes.c_char_p]
usb_dll.USB_GetDeviceInfo.restype = ctypes.c_int

# 获取设备信息
serial_param = b"357C39553033"
dev_info = DeviceInfoDetail()
func_str = create_string_buffer(256)
result = usb_dll.USB_GetDeviceInfo(serial_param, byref(dev_info), func_str)

if result == 0:
    print("=== DLL信息 ===")
    print(f"DLL名称: {dev_info.DllName.decode('utf-8')}")
    dll_major = (dev_info.DllVersion >> 8) & 0xFF
    dll_minor = dev_info.DllVersion & 0xFF
    print(f"DLL版本: {dll_major}.{dll_minor}")

    print("=== STM32设备固件信息 ===")
    print(f"固件名称: {dev_info.FirmwareName.decode('utf-8')}")
    hw_major = (dev_info.HardwareVersion >> 8) & 0xFF
    hw_minor = dev_info.HardwareVersion & 0xFF
    print(f"硬件版本: {hw_major}.{hw_minor}")
else:
    print(f"获取设备信息失败，错误码: {result}")