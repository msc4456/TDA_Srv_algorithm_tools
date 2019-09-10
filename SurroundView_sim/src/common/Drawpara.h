#ifndef DRAWPARA_H
#define DRAWPARA_H

#include <glad/glad.h>
#define picture_H 720
//#define picture_ratio1 4.0/3.0
#define picture_ratio1 1.0/1.0
#define picture_ratio2 8.0/11.0  //考虑车身长度与左右上下视场区域  11.0 =car_H+2*3  8.0 =car_w+2*3

//视角控制参数
GLfloat   angle  = 0.0f;
GLfloat   upon   = 45.0f;     //默认在45度视角进行观察
bool      view_mode = 0;
GLfloat   view_dist =1.0f;    //变更顶点坐标范围后，需要重新调整
//GLfloat   PI =3.14;
//static float c = PI/180.0;    //弧度和角度转换参数

//视窗宽高比
GLfloat singleRatio      = picture_ratio1;
GLfloat multRatio        = picture_ratio2;

//单视图区域
GLfloat SRV_SingleWind_H = picture_H;
GLfloat SRV_SingleWind_W = picture_ratio1*SRV_SingleWind_H;

//拼接图区域
GLfloat SRV_MultWind_H   = picture_H;
GLfloat SRV_MultiWind_W  = SRV_MultWind_H*picture_ratio2;

//窗口总尺寸
GLfloat SRV_MainWind_H   = picture_H;
GLfloat SRV_MainWind_W   = SRV_SingleWind_W+SRV_MultiWind_W;

//拼接取景域
GLfloat multView_startH  =-40.0;

#endif 