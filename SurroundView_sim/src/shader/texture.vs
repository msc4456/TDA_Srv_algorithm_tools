#version 330 core
//从CPU传入GPU的，分配过的属性表
layout (location = 0) in vec3 aPos;        //属性0：顶点XYZ坐标
//备注：曲面上的任意一个顶点在四个通道的纹理图像中均有坐标，当该顶点不被某通道的摄像头所观测到时，在该通道中的纹理坐标为0，0
layout (location = 1) in vec2 aTexCoord1;  //属性1：顶点在前通道中的纹理坐标
layout (location = 2) in vec2 aTexCoord2;  //属性2：顶点在左通道中的纹理坐标
layout (location = 3) in vec2 aTexCoord3;  //属性3：顶点在后通道中的纹理坐标
layout (location = 4) in vec2 aTexCoord4;  //属性4：顶点在右通道中的纹理坐标

layout (location = 5) in float aTexWeight1;//属性5：顶点在前通道中的纹理权重
layout (location = 6) in float aTexWeight2;//属性6：顶点在左通道中的纹理权重
layout (location = 7) in float aTexWeight3;//属性7：顶点在后通道中的纹理权重
layout (location = 8) in float aTexWeight4;//属性8：顶点在右通道中的纹理权重


//----------------------------------------------------------------------------------
//可直接通过layout语句传递给片段着色器，不必要一定通过顶点着色器中转
out vec2 oTexCoord1;    //前通道纹理坐标输出
out vec2 oTexCoord2;    //左通道纹理坐标输出
out vec2 oTexCoord3;    //后通道纹理坐标输出
out vec2 oTexCoord4;    //右通道纹理坐标输出

out float oTexWeight1;  //前通道纹理权重输出
out float oTexWeight2;  //左通道纹理权重输出
out float oTexWeight3;  //后通道纹理权重输出
out float oTexWeight4;  //右通道纹理权重输出

uniform mat4 model;     //模型矩阵
uniform mat4 view;      //观察矩阵
uniform mat4 projection;//透视矩阵

uniform float gain1;    //前通道增益
uniform float gain2;    //左通道增益
uniform float gain3;    //后通道增益
uniform float gain4;    //右通道增益

//-----------------------------------------------------------------------------------
void main()
{
	//MVP复合矩阵确定顶点在屏幕中的绘制位置,全部为齐次运算
	gl_Position = projection * view * model * vec4(aPos, 1.0f);//MVP matrix multied in GPU slice
	//gl_Position = vec4(aPos, 1.0f);

    //通过顶点着色器透传纹理坐标
	oTexCoord1 = vec2(aTexCoord1.x,1-aTexCoord1.y);
	oTexCoord2 = vec2(aTexCoord2.x,1-aTexCoord2.y);
	oTexCoord3 = vec2(aTexCoord3.x,1-aTexCoord3.y);
	oTexCoord4 = vec2(aTexCoord4.x,1-aTexCoord4.y);
    
	//通过顶点着色器，及内置乘法器传递
	oTexWeight1 = aTexWeight1*gain1;
	oTexWeight2 = aTexWeight2*gain2;
	oTexWeight3 = aTexWeight3*gain3;
	oTexWeight4 = aTexWeight4*gain4;

}