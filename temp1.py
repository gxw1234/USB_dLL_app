import re
import struct

input_file = r"C:\Users\A\Desktop\新建文件夹\1.txt"    # 输入文件名
output_file =  r"C:\Users\A\Desktop\新建文件夹\1\output1.bin"  # 输出二进制文件名

with open(input_file, "r", encoding="utf-8") as fin, open(output_file, "wb") as fout:
    for line in fin:
        match = re.search(r'(?:1,|2,)(0x[0-9a-fA-F]+)', line)
        if match:
            value = int(match.group(1), 16)
            fout.write(struct.pack('<H', value))  # 小端2字节写入