1.本工程生成二进制文件 vertex_lut.bin 和index_lut.bin
2.vertex_lut.bin 文件中包含了 vertex(x,y,z),tex1(u,v),tex2(u,v),blend(b1,b2)

vertex_lut.bin数据结构：
vertex(x,y,z) 为顶点数据的三轴坐标，单位为cm
tex1(u,v)为顶点世界坐标在被成像的相邻的两个相机中顺时针侧相机成像的纹理坐标，单位为像素
tex2(u,v)为顶点世界坐标在被成像的相邻的两个相机中逆时针侧相机成像的纹理坐标，单位为像素
blend(b1,b2)为顶点对应的两个纹理坐标的像素权重

index_lux.bin数据结构
安装曲面surface的三角形片元排列顺序
起点 -> 中点 -> 终点 三角形顶点排列以顺时针方向为正，以保证后续openGL背面剔除时，曲面各单元背面方向统一






