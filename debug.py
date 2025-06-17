import serial
import time

# 串口配置
port = 'COM16'  # 请根据你的设备调整这个端口
baud_rate = 115200
width = 96
height = 240

# 计算总字节数
total_bytes = width * height

# 创建串口对象
ser = serial.Serial(port, baud_rate)

# 创建要发送的数据
data_to_send = bytes([i % 256 for i in range(total_bytes)])  # 示例数据

# 开始计时
start_time = time.time()

# 发送数据
bytes_written = ser.write(data_to_send)

# 结束计时
end_time = time.time()
elapsed_time = (end_time - start_time) * 1000  # 转换为毫秒

# 输出发送的时间和实际发送的字节数
print(f"发送 {bytes_written} 字节所需时间: {elapsed_time:.6f} 毫秒")

# 关闭串口
ser.close()
