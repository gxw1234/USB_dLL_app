import os
from PIL import Image

# 设置图片的尺寸
width, height = 96, 240

# 定义输出文件夹
output_folder = 'output_images'

# 如果文件夹不存在，创建它
if not os.path.exists(output_folder):
    os.makedirs(output_folder)

# 生成90张灰度.bmp图片
for i in range(1, 91):
    # 创建一个新的灰度图像，默认为黑色
    img = Image.new('L', (width, height), 100)  # 'L'模式用于灰度图像，0表示黑色
    # 生成文件名，包含文件夹路径
    filename = os.path.join(output_folder, f"{i}.bmp")
    # 保存为.bmp格式
    img.save(filename)

    print(f"生成了 {filename}")

print("所有灰度图像生成完毕！")
