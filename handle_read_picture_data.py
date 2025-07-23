
import os
import re
from PIL import Image
from numpy import array
from ctypes import *


SMOKE_DATA =[0 for i in range(96*240)]
def tryint(s):
    try:
        return int(s)
    except ValueError:
        return s
def str2int(v_str):  # 将元素中的字符串和数字分割开
    return [tryint(sub_str) for sub_str in re.split('([0-9]+)', v_str)]
def sort_humanly(v_list):  # 以分割后的list为单位进行排序
    return sorted(v_list, key=str2int)

def image_sort(data:list):
    image_format = "."+data[0].split(".")[1]
    image_length_length =len(data[0].split(".")[0])
    Formatting = "%0"+str(image_length_length)+"d"
    im_split= [ int(i.split(".")[0]) for i in data ]
    for i in range(len(im_split)):
        for j in range(i+1,(len(im_split))):
            if im_split[i]>im_split[j]:
                im_split[i],im_split[j]=im_split[j],im_split[i]
    return [ str(Formatting % i)+image_format for i in  im_split]


def hex_images(save_path:str)-> list:

    file_dir = save_path
    dir_list = os.listdir(file_dir)
    bmp_list = [filename for filename in dir_list if filename.lower().endswith('.bmp')]
    print(f"图片张数：{len(bmp_list)}")
    data_list_sort = sort_humanly(bmp_list)
    print(f"data_list_sort:{data_list_sort}")

    data_list = []
    for cur_file in data_list_sort:
        # 获取文件的绝对路径
        path = os.path.join(file_dir, cur_file)
        im = Image.open(path)
        img = im.convert('L')
        img_array = array(img)
        flat_img_array = img_array.flatten()
        data_list.append((c_ubyte * len(flat_img_array))(*flat_img_array))
    return data_list
def hex_images_end()-> list:
    data_list = []
    data_list.append((c_ubyte * len(SMOKE_DATA))(*SMOKE_DATA))
    data_list.append((c_ubyte * len(SMOKE_DATA))(*SMOKE_DATA))
    data_list.append((c_ubyte * len(SMOKE_DATA))(*SMOKE_DATA))
    data_list.append((c_ubyte * len(SMOKE_DATA))(*SMOKE_DATA))
    data_list.append((c_ubyte * len(SMOKE_DATA))(*SMOKE_DATA))
    data_list.append((c_ubyte * len(SMOKE_DATA))(*SMOKE_DATA))
    return data_list

def hex_images1(save_path:str):
    # file_dir = r"D:\testimage"
    file_dir = save_path
    dir_list = os.listdir(file_dir)
    data_list = []
    for cur_file in dir_list:
        # 获取文件的绝对路径
        path = os.path.join(file_dir, cur_file)
        im = Image.open(path)
        img = im.convert('L')
        # img.show()
        img_array = array(img).tolist()
        print(img_array)
        print(type(img_array))



if __name__ == "__main__":
    ...
