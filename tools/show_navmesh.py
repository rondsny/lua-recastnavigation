# -*- coding: utf-8 -*-

import sys
import matplotlib.pyplot as plt
from matplotlib.patches import Polygon
import map_data as md

# 获取第一个参数
# 如果没有参数，或者参数不是数字，就使用默认值

png_name = "map.png"
if len(sys.argv) > 1:
    png_name = sys.argv[1]

colors = [
'#423629',
'#4f5d2f',
'#7d7e75',
'#b0b2b8',
'#cfd6ea',
'#423629',
'#4f5d2f',
'#7d7e75',
'#b0b2b8',
'#cfd6ea',
]

width = md.BORDER_X_MAX - md.BORDER_X_MIN
height = md.BORDER_Z_MAX - md.BORDER_Z_MIN

# 创建 Matplotlib 的坐标系，并添加多边形
fig, ax = plt.subplots(figsize=(width/10, height/10))

list = md.DATA
for i in range(len(list)):
    item = list[i]

    # 创建多边形，并设置其属性
    poly = Polygon(item[0], facecolor=colors[item[1]], edgecolor='#222227')

    ax.add_patch(poly)

# 设置图形的范围
# 如果不设置范围，坐标系的范围会根据数据自动调整
ax.set_xlim(md.BORDER_X_MIN, md.BORDER_X_MAX)
ax.set_ylim(md.BORDER_Z_MIN, md.BORDER_Z_MAX)

plt.savefig(png_name, bbox_inches='tight')
# plt.show()