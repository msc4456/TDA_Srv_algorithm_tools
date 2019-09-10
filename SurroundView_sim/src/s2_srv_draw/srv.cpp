/*
 *******************************************************************************
 *
 * Copyright (C) 2013 Texas Instruments Incorporated - http://www.ti.com/
 * ALL RIGHTS RESERVED
 *
 *******************************************************************************
 */

//#include "render.h"
#include <math.h>
#include "../shader/renderutils.h"
#include "srv.h"
#include "../s1_car_draw/car.h"

#include <glm/vec3.hpp> // glm::vec3
#include <glm/vec4.hpp> // glm::vec4, glm::ivec4
#include <glm/mat4x4.hpp> // glm::mat4
#include <glm/gtc/matrix_transform.hpp> // glm::translate, glm::rotate, glm::scale, glm::perspective
#include <glm/gtc/type_ptr.hpp> // glm::value_ptr
#include <glm/gtx/string_cast.hpp>// 用于打印 mat4 查看当前状态

//#define orthoview
#define perspview

extern int seq_color;

int shader_output_select = 0;
bool tex_load_state = false;//初始状态：未导入纹理

GLenum render_mode = GL_TRIANGLE_STRIP;//渲染模式选择三角形渲染

extern bool srv_render_to_file;

//Mesh splitting logic
#define MAX_VBO_MESH_SPLIT 8

static GLuint vboId[MAX_VBO_MESH_SPLIT*3];

static void * prevLUT=(void *)0xdead;

#define MAX_INDEX_BUFFERS 2

typedef struct {
	unsigned int *buffer;
	unsigned int length;
} t_index_buffer;

t_index_buffer index_buffers[MAX_INDEX_BUFFERS];
unsigned int active_index_buffer = 1;
bool index_buffer_changed = 0;

//着色器参数信息
struct _srv_program{
    GLuint srv_Uid;
    GLuint sample_tex1;
    GLuint sample_tex2;
    GLuint sample_tex3;
    GLuint sample_tex4;
}srv_program;

//Shaders for surface view
static const char srv_vert_shader[] =
"#version 150 core\n"         //使用GLSL1.5
"attribute vec3 aPos;\n"      //顶点坐标
"attribute vec2 aTexCoord1;\n"//纹理坐标
"attribute vec2 aTexCoord2;\n"
"attribute vec2 aTexCoord3;\n"
"attribute vec2 aTexCoord4;\n"
"attribute float aTexWeight1;\n"
"attribute float aTexWeight2;\n"
"attribute float aTexWeight3;\n"
"attribute float aTexWeight4;\n"
"varying vec2 oTexCoord1;\n"//前纹理坐标矢量
"varying vec2 oTexCoord2;\n"//左纹理坐标矢量
"varying vec2 oTexCoord3;\n"//后纹理坐标矢量
"varying vec2 oTexCoord4;\n"//右纹理坐标矢量
"varying float oTexWeight1;\n"//前纹理权重
"varying float oTexWeight2;\n"//左纹理权重
"varying float oTexWeight3;\n"//后纹理权重
"varying float oTexWeight4;\n"//右纹理权重
"uniform mat4 model;\n"       //模型矩阵
"uniform mat4 view;\n"        //观察矩阵
"uniform mat4 projection;\n"  //投影矩阵
"uniform float gain1;\n"      //前通道增益
"uniform float gain2;\n"      //左通道增益
"uniform float gain3;\n"      //后通道增益
"uniform float gain4;\n"      //右通道增益
"void main()\n"
"{\n"
"   gl_Position = projection*view*model*vec4(aPos,1.0f);\n"//MVP获取顶点绘制位置
"   oTexCoord1 = vec2(aTexCoord1.x,1-aTexCoord1.y);\n"     
"   oTexCoord2 = vec2(aTexCoord2.x,1-aTexCoord2.y);\n"
"   oTexCoord3 = vec2(aTexCoord3.x,1-aTexCoord3.y);\n"
"   oTexCoord4 = vec2(aTexCoord4.x,1-aTexCoord4.y);\n"
"   oTexWeight1 = aTexWeight1*gain1;\n"
"   oTexWeight2 = aTexWeight2*gain2;\n"
"   oTexWeight3 = aTexWeight3*gain3;\n"
"   oTexWeight4 = aTexWeight4*gain4;\n"
"}\n";

static const char srv_frag_shader[] =
"#version 150 core\n"
"out vec4 FragColor_RGB;\n"
"varying vec2 oTexCoord1;\n"   //前通道纹理坐标输入
"varying vec2 oTexCoord2;\n"   //左通道纹理坐标输入
"varying vec2 oTexCoord3;\n"   //后通道纹理坐标输入
"varying vec2 oTexCoord4;\n"   //右通道纹理坐标输入
"varying float oTexWeight1;\n" //前通道纹理坐标权重输入
"varying float oTexWeight2;\n" //左通道纹理坐标权重输入
"varying float oTexWeight3;\n" //后通道纹理坐标权重输入
"varying float oTexWeight4;\n" //右通道纹理坐标权重输入
// 纹理采样，光栅化之后
"uniform sampler2D texture1;\n"//前2D纹理采样
"uniform sampler2D texture2;\n"//左2D纹理采样
"uniform sampler2D texture3;\n"//后2D纹理采样
"uniform sampler2D texture4;\n"//右2D纹理采样
"uniform int color_seq;\n"
"void main()\n"
"{\n"
// linearly interpolate between both textures (80% container, 20% awesomeface)
// FragColor = mix(texture(texture1, TexCoord), texture(texture2, TexCoord), 0.2);
//"   int BGR_type = 1;\n"//定义色序
"	vec4 BGR_data =  texture2D(texture1,oTexCoord1)*oTexWeight1\n"
"				    +texture2D(texture2,oTexCoord2)*oTexWeight2\n"
"				    +texture2D(texture3,oTexCoord3)*oTexWeight3\n"
"				    +texture2D(texture4,oTexCoord4)*oTexWeight4;\n"
//在shader中完成RGB到BGR的转换，shader中也可以完成YUV的转换,后续通过输入进行扩展
"   if(color_seq == 0)\n"
"   FragColor_RGB = BGR_data;\n"
"   if(color_seq == 1)\n"
"   FragColor_RGB = vec4(BGR_data.b,BGR_data.g,BGR_data.r,1.0);\n"
"   if(color_seq == 2) {\n"
"   "
"   }\n"
"}\n";

int view_init(surface_data_t*pObj)
{
  pObj->angle =0.0f;
  pObj->upon  =45.0f;
  pObj->dist  =1.0;
  pObj->screen_width = 600;
  pObj->screen_height= 800;
}

void checkCompileErrors(GLuint shader, const char* type)
{
    GLint success;
    GLchar infoLog[1024];
    if (type != "PROGRAM")
    {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(shader, 1024, NULL, infoLog);
            printf("ERROR::SHADER_COMPILATION_ERROR of type: %s %s\n",type,infoLog);
        }
    }
    else
    {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success)
        {
            glGetProgramInfoLog(shader, 1024, NULL, infoLog);
            printf("ERROR::PROGRAM_LINKING_ERROR of type: %s %s\n",type,infoLog);
        }
    }
}

GLuint Shader_create(const char* vShaderCode, const char* fShaderCode)
{
        // 2. compile shaders
        unsigned int vertex, fragment;
        // vertex shader
        vertex = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex, 1, &vShaderCode, NULL);
        glCompileShader(vertex);
        checkCompileErrors(vertex, "VERTEX");
        // fragment Shader
        fragment = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment, 1, &fShaderCode, NULL);
        glCompileShader(fragment);
        checkCompileErrors(fragment, "FRAGMENT");
        // shader Program
        GLuint ID = glCreateProgram();
        glAttachShader(ID, vertex);
        glAttachShader(ID, fragment);
        glLinkProgram(ID);
        checkCompileErrors(ID, "PROGRAM");
        // delete the shaders as they're linked into our program now and no longer necessery
        glDeleteShader(vertex);
        glDeleteShader(fragment);
        return ID;
}

//初始化纹理信息，当程序运行在单击模式下，纹理静态导入一次
int standalone_init_texture(const char*base_path,const char*filetype,GLuint (&texture)[4])
{
    char filepath[50];
    const char*filename[4];

    int flag;
    //static GLuint texYuv[4] ={0};//注意静态变量类型

    filename[0]="front0";
    filename[1]="left0";
    filename[2]="rear0";
    filename[3]="right0";

    for(char i=0;i<4;i++)
    {
    sprintf(filepath,"%s/%s.%s",base_path,filename[i],filetype);//必须是bmp文件
   	glGenTextures(1, &texture[i]);
	glBindTexture(GL_TEXTURE_2D, texture[i]);
	flag =load_texture_bmp(texture[i], filepath);
    if(flag!=0){
        printf("texture bind failed\n");
        return -1;
        }
    }
    return true;
}

void get_vedio_stream()
{
    /*code ...*/
}

int srv_setup(surface_data_t *pObj)//读入lut,创建着色器
{
    unsigned int flag=0;
    FILE *fp = NULL;
    //获取表格数据
    pObj->vertex_num  = 6702;
    pObj->indices_num = 12810;
    pObj->vertex_arry = (float*)malloc(pObj->vertex_num*15*sizeof(float));
    pObj->indices_arry =(unsigned int*)malloc(pObj->indices_num*3*sizeof(unsigned int));                                      
    fp =fopen("../../src/shader/vertex.bin","rb");                                //导入顶点
    flag =fread(pObj->vertex_arry,pObj->vertex_num*15*sizeof(float),1,fp);          //读入顶点数据表,每次读sizeof(vertices)个，读一次
    if(flag==0) printf("import vertex failed\n");
    //for(int i=0;i<9;i++)
    //{
    //    printf("data is %f \n",pObj->vertex_arry[i]);
    //}
    fclose(fp);                                           
    fp =fopen("../../src/shader/surface.bin","rb");                               //导入曲面
    flag =fread(pObj->indices_arry,pObj->indices_num*3*sizeof(unsigned int),1,fp);
    fclose(fp);

    //初始化全景视角
    view_init(pObj);//该函数必须在shader已经创建之后再进行

    //编译创建着色器
    //pObj.shader_uid = renderutils_createProgram(srv_vert_shader,srv_frag_shader);//不打印shader调试信息
    pObj->shader_uid = Shader_create(srv_vert_shader,srv_frag_shader);//带shader调试信息

	if (pObj->shader_uid==0)//如果渲染创建不成功，返回-1
	{
        printf("srv_program shader created failed\n");
        return -1;
	}
    else
    {
        printf("srv_program shader created finished, ID is %d\n",pObj->shader_uid);
    }
    
    
    //启用着色器
    glUseProgram(pObj->shader_uid);
    
    //初次向shader传递静态图像
    glUniform1i(glGetUniformLocation(pObj->shader_uid, "texture1"), 0);
    glUniform1i(glGetUniformLocation(pObj->shader_uid, "texture2"), 1);
    glUniform1i(glGetUniformLocation(pObj->shader_uid, "texture3"), 2);
    glUniform1i(glGetUniformLocation(pObj->shader_uid, "texture4"), 3);

    //配置纹理色序
    glUniform1i(glGetUniformLocation(pObj->shader_uid, "color_seq"), 0);//2:YUV  1 :=> BGR  0: =>RGB

    #ifdef LUM_AVG
    /* */
    #else
    glUniform1f(glGetUniformLocation(pObj->shader_uid, "gain1"), 1);
    glUniform1f(glGetUniformLocation(pObj->shader_uid, "gain2"), 1);
    glUniform1f(glGetUniformLocation(pObj->shader_uid, "gain3"), 1);
    glUniform1f(glGetUniformLocation(pObj->shader_uid, "gain4"), 1); 
    #endif

    return 0;  
}

void srv_update_view(surface_data_t *pObj)
{
    glm::mat4 srf_model = glm::mat4(1.0f);//重置模型矩阵,不做任何旋转操作
    //srf_model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f,0.0f,0.0f));//
    //srf_model = glm::rotate(srf_model, angle, glm::vec3(0.0f, 0.0f, 1.0f));//

    //设置观察矩阵
    #ifdef perspview
    glm::mat4 srf_view = glm::lookAt(glm::vec3(
                                             pObj->dist*cos(degreesToRadians(pObj->upon))*sin(degreesToRadians(pObj->angle)), 
                                            -pObj->dist*cos(degreesToRadians(pObj->upon))*cos(degreesToRadians(pObj->angle)), 
                                             pObj->dist*sin(degreesToRadians(pObj->upon))),
                                     glm::vec3(0.0f, 0.0f, 0.0f), 
                                     glm::vec3(0.0f, 0.0f, 1.0f)
                                     );
    //设置透视矩阵
    glm::mat4 srf_projection = glm::perspective(45.0f, 1.0f/1.0f, 0.1f, 1000.0f);
    #endif
    #ifdef orthoview
    glm::mat4 srf_view = glm::lookAt(glm::vec3(0.0f, 0.0f, 0.1f),
                                     glm::vec3(0.0f, 0.0f, 0.0f), 
                                     glm::vec3(0.0f, 1.0f, 0.0f)
                                     );   
    glm::mat4 srf_projection = glm::ortho(-0.5f,0.5f,-0.5f,0.5f,0.0f,100.0f);
    #endif
    //glOrtho(multView_startH, -multView_startH, multView_startH/multRatio, -multView_startH/multRatio, 100.0f, -100.0f);
    //传参到着色器,传入shader后在进行乘法
    glUniformMatrix4fv(glGetUniformLocation(pObj->shader_uid, "view"), 1, GL_FALSE, &srf_view[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(pObj->shader_uid, "projection"), 1, GL_FALSE, &srf_projection[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(pObj->shader_uid, "model"), 1, GL_FALSE, &srf_model[0][0]);

    if(seq_color)
    glUniform1i(glGetUniformLocation(pObj->shader_uid, "color_seq"), 0);//2:YUV  1 :=> BGR  0: =>RGB
    else
    {
    glUniform1i(glGetUniformLocation(pObj->shader_uid, "color_seq"), 1);//2:YUV  1 :=> BGR  0: =>RGB
    }
    
}

//缓冲区分配，向着色器传递数组表格
int srv_init_vertices_vbo(
                         GLuint &ArrayBuffer_iD,  //曲面数组缓冲区编号
                         GLuint &VertexBufferID,  //曲面顶点缓冲区编号
                         GLuint &IndicesBuffer_ID,//片面索引缓冲区编号
                         surface_data_t *pObj    //曲面信息
                        )
{
    glGenVertexArrays(1, &ArrayBuffer_iD);           //生成顶点数组对象
    glBindVertexArray(ArrayBuffer_iD);               //绑定顶点数组对象

    glGenBuffers(1, &VertexBufferID);                //生成顶点缓冲区对象
    glBindBuffer(GL_ARRAY_BUFFER, VertexBufferID);   //绑定顶点缓冲区对象
    glBufferData(GL_ARRAY_BUFFER, pObj->vertex_num*15*sizeof(float), pObj->vertex_arry, GL_STATIC_DRAW);//创建并初始化顶点数据
    //target指定目标缓冲区对象。 符号常量必须为GL_ARRAY_BUFFER或GL_ELEMENT_ARRAY_BUFFER。
    //size指定缓冲区对象的新数据存储的大小（以字节为单位）。
    //data指定将复制到数据存储区以进行初始化的数据的指针，如果不复制数据，则指定NULL。
    //usage指定数据存储的预期使用模式。 符号常量必须为GL_STREAM_DRAW，GL_STATIC_DRAW或GL_DYNAMIC_DRAW。

    glGenBuffers(1, &IndicesBuffer_ID);              //生成索引缓冲区对象
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IndicesBuffer_ID);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, pObj->indices_num*3*sizeof(unsigned int), pObj->indices_arry,GL_STATIC_DRAW);
    // glBufferData(GL_ELEMENT_ARRAY_BUFFER, 3, indices, GL_STATIC_DRAW);

    // position attribute--------------------CPU向GPU传入顶点数组的数据并分配属性----------------------------------//
    //参数1：属性索引  参数2：参数的个数  参数3：参数的类型  参数5：相邻的同属性参数在表格中的跨度  参数6：第一个参数在表格中的偏移量
    // vertex
    glVertexAttribPointer(glGetAttribLocation(pObj->shader_uid,"aPos"), 3, GL_FLOAT, GL_FALSE, 15 * sizeof(float), (void*)0);//从VBO中
    glVertexAttribPointer(glGetAttribLocation(pObj->shader_uid,"aTexCoord1"), 2, GL_FLOAT, GL_FALSE, 15 * sizeof(float), (void*)(3 * sizeof(float)));
    glVertexAttribPointer(glGetAttribLocation(pObj->shader_uid,"aTexCoord2"), 2, GL_FLOAT, GL_FALSE, 15 * sizeof(float), (void*)(5 * sizeof(float)));
    glVertexAttribPointer(glGetAttribLocation(pObj->shader_uid,"aTexCoord3"), 2, GL_FLOAT, GL_FALSE, 15 * sizeof(float), (void*)(7 * sizeof(float)));
    glVertexAttribPointer(glGetAttribLocation(pObj->shader_uid,"aTexCoord4"), 2, GL_FLOAT, GL_FALSE, 15 * sizeof(float), (void*)(9 * sizeof(float)));
    glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, 15 * sizeof(float), (void*)(11 * sizeof(float)));
    glVertexAttribPointer(6, 1, GL_FLOAT, GL_FALSE, 15 * sizeof(float), (void*)(12 * sizeof(float)));
    glVertexAttribPointer(7, 1, GL_FLOAT, GL_FALSE, 15 * sizeof(float), (void*)(13 * sizeof(float)));
    glVertexAttribPointer(8, 1, GL_FLOAT, GL_FALSE, 15 * sizeof(float), (void*)(14 * sizeof(float)));

    return 0;
}

//纹理绑定
//可修改为仅绑定两个纹理
void onscreen_mesh_state_restore_program_textures_attribs(GLuint *texYuv, int tex1, int tex2, int tex3,int tex4,surface_data_t *pObj)
{

    glUseProgram(pObj->shader_uid);
	//set the program we need
    glUniform1i(glGetUniformLocation(pObj->shader_uid, "texture1"), 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texYuv[tex1]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	GL_CHECK(glBindTexture);

	glUniform1i(glGetUniformLocation(pObj->shader_uid, "texture2"), 1);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, texYuv[tex2]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	GL_CHECK(glBindTexture);

    glUniform1i(glGetUniformLocation(pObj->shader_uid, "texture3"), 2);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, texYuv[tex3]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	GL_CHECK(glBindTexture);

    glUniform1i(glGetUniformLocation(pObj->shader_uid, "texture4"), 3);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, texYuv[tex4]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	GL_CHECK(glBindTexture);

}

void srv_draw(surface_data_t*pObj,GLuint *texYuv,GLuint Viewport_id)
{
    if(!tex_load_state)//  tex_load_state = 0 从未导入过纹理,则进行纹理导入
    { //texYuv = standalone_init_texture("../resource/image/","bmp");
     tex_load_state = true;
    }
    else //
    {
      /* code :从framebuffer中动态导入图像，更新到纹理中*/
    }
    //更新MVP到着色器
    srv_update_view(pObj);
    //传递纹理到着色器
    onscreen_mesh_state_restore_program_textures_attribs(texYuv, 0, 1, 2, 3,pObj);
    
    //定义缓冲区，分配编号
    GLuint VAO,VBO,EBO;
	//向顶点缓冲区写新数据，顶点坐标，纹理坐标，blending值
    srv_init_vertices_vbo(VAO,VBO,EBO,pObj);//曲面数组缓冲区编号,曲面顶点缓冲区编号,片面索引缓冲区编号,渲染对象，纹理数组
    
    for(int k=0;k<=8;k++) glEnableVertexAttribArray(k);
    glDrawElements(GL_TRIANGLES, pObj->indices_num*3*sizeof(float)/sizeof(unsigned int), GL_UNSIGNED_INT, 0);
    for(int k=0;k<=8;k++) glDisableVertexAttribArray(k);

    glDeleteBuffers(1,&VBO);//地址指向零之前，需要删除buffer，否则会导致内存和显存的持续泄漏
    glDeleteBuffers(1,&EBO);//

    glBindBuffer(GL_ARRAY_BUFFER, 0);                       //绑定指向0
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);               //绑定指向0

	glFlush();
}
