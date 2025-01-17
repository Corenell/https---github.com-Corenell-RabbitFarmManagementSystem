import os
import random
import shutil

def rename_files(image_folder, label_folder):
    image_files = {os.path.splitext(f)[0]: f for f in os.listdir(image_folder) if f.endswith('.jpeg')}
    label_files = {os.path.splitext(f)[0]: f for f in os.listdir(label_folder) if f.endswith('.txt')}
    
    for name in image_files:
        if name in label_files:
            new_name = str(random.randint(1000, 9999))
            while os.path.exists(os.path.join(image_folder, new_name + '.jpeg')) or os.path.exists(os.path.join(label_folder, new_name + '.txt')):
                new_name = str(random.randint(1000, 9999))
            os.rename(os.path.join(image_folder, image_files[name]), os.path.join(image_folder, new_name + '.jpeg'))
            os.rename(os.path.join(label_folder, label_files[name]), os.path.join(label_folder, new_name + '.txt'))

# 示例用法
image_folder = "D:\\流产图片\\流产数据集"
label_folder = "D:\\流产图片\\label\\biao_qian"
rename_files(image_folder, label_folder)
