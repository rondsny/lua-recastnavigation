#!/bin/bash

if [ ! -d "./test/images" ]
then
    mkdir ./test/images
fi

# 遍历 nav_data 目录所有文件
for file in `ls ./test/nav_data`
do
    # 获取文件名
    file_name=${file%.*}
    # 获取文件后缀
    file_suffix=${file##*.}
    # 判断文件后缀是否为 png
    if [ $file_suffix = "navmesh" ]
    then
        # 保存文件
        lua test.lua ./test/nav_data/$file
        python3 tools/show_navmesh.py ./test/images/$file_name.png
    fi
done