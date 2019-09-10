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

#define QUADRANT_WIDTH  (1080/4/2 + 1)
#define QUADRANT_HEIGHT (1080/4/2 + 1)
#define QUADRANTS 4

extern int seq_color;

int shader_output_select = 0;
bool tex_load_state = false;//初始状态：未导入纹理

static void * prevLUT=(void *)0xdead;

GLenum render_mode = GL_TRIANGLE_STRIP;//渲染模式选择三角形渲染

extern bool srv_render_to_file;
//Mesh splitting logic
#define MAX_VBO_MESH_SPLIT 8

static GLuint vboId[QUADRANTS*2];// one for vertex, one for element indices
static GLuint vaoId[QUADRANTS];// just one for VAO storage state

#define MAX_INDEX_BUFFERS 2

typedef struct {
	unsigned int *buffer;
	unsigned int length;
} t_index_buffer;

t_index_buffer index_buffers[MAX_INDEX_BUFFERS];
unsigned int active_index_buffer = 1;
bool index_buffer_changed = true;

//着色器参数信息
struct _srv_program{
    GLuint srv_Uid;
    GLuint sample_tex1;
    GLuint sample_tex2;
}srv_program;

//Shaders for surface view
static const char srv_vert_shader[] =
"#version 150 core\n"             //使用GLSL1.5
"attribute vec3 aPos;\n"          //顶点坐标
"attribute vec2 aTextureCoord1;\n"//纹理坐标输入
"attribute vec2 aTextureCoord2;\n"//纹理坐标输入
"attribute vec2 iblendValus;\n"
"varying vec2 oTexCoord1;\n"      //纹理坐标
"varying vec2 oTexCoord2;\n"      //纹理坐标
"varying vec2 oblendValus;\n"     //混合权重系数
"uniform mat4 model;\n"       //模型矩阵
"uniform mat4 view;\n"        //观察矩阵
"uniform mat4 projection;\n"  //投影矩阵
"void main()\n"
"{\n"
"   gl_Position = projection*view*model*vec4(aPos,1.0f);\n"//MVP获取顶点绘制位置
"   oTexCoord1 = vec2(aTextureCoord1.y,1-aTextureCoord1.x);\n"     
"   oTexCoord2 = vec2(aTextureCoord2.y,1-aTextureCoord2.x);\n"
"   oblendValus = iblendValus;\n"

"}\n";

static const char srv_frag_shader[] =
"#version 150 core\n"
"out vec4 FragColor_RGB;\n"
"varying vec2 oTexCoord1;\n"//前通道纹理坐标输入
"varying vec2 oTexCoord2;\n"//左通道纹理坐标输入
"varying vec2 oblendValus;\n" //权重系数下行
"uniform sampler2D texture1;\n"//前2D纹理采样
"uniform sampler2D texture2;\n"//左2D纹理采样
"uniform int color_seq;\n"
"void main()\n"
"{\n"
"	vec4 BGR_data =  texture2D(texture1,oTexCoord1)*oblendValus.x\n" // 右侧相机纹理
"				    +texture2D(texture2,oTexCoord2)*oblendValus.y;\n"//左侧相机纹理
//在shader中完成RGB到BGR的转换，shader中也可以完成YUV的转换,后续通过输入进行扩展
//"       FragColor_RGB = BGR_data;\n"
"       FragColor_RGB = vec4(BGR_data.b,BGR_data.g,BGR_data.r,1.0);\n"
"}\n";


GLuint vertexPositionAttribLoc;  
GLuint vertexTexCoord1AttribLoc;
GLuint vertexTexCoord2AttribLoc;
GLuint blendAttribLoc;
GLuint samplerLocation0;
GLuint samplerLocation1;

int view_init(surface_data_t *pObj)
{
  pObj->angle =0.0f;
  pObj->upon  =45.0f;
  pObj->dist  =1.0;
  pObj->screen_width = 880;
  pObj->screen_height= 960;
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
    filename[1]="right0";
    filename[2]="rear0";
    filename[3]="left0";

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

int srv_setup(surface_data_t*pObj)//读入lut,创建着色器
{
    unsigned int flag=0;
    FILE *fp = NULL;
    //获取表格数据
    pObj->vertex_num  = 73984; //顶点个数
    pObj->indices_num = 36450;//三角形个数
    pObj->vertex_arry = (float*)malloc(pObj->vertex_num*sizeof(srv_lut_t));

    pObj->indices_arry =(unsigned int*)malloc(pObj->indices_num*3*sizeof(unsigned int)); 

    fp =fopen("../../src/shader/srv_lut.bin","rb");                                //导入顶点
    flag =fread(pObj->vertex_arry,pObj->vertex_num/4*sizeof(srv_lut_t),4,fp);         //读入顶点数据表,每次读sizeof(vertices)个，读一次
    if(flag==0) printf("import vertex failed\n");

    fclose(fp);                                           
    fp =fopen("../../src/shader/srv_indices.bin","rb");                            //导入曲面
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
    samplerLocation0 = glGetUniformLocation(pObj->shader_uid, "texture1");
	glUniform1i(samplerLocation0, 0);
	samplerLocation1 = glGetUniformLocation(pObj->shader_uid, "texture2");
	glUniform1i(samplerLocation1, 1);

    //配置纹理色序
	vertexPositionAttribLoc  = glGetAttribLocation(pObj->shader_uid, "aPos");
    printf("vertexPositionAttribLoc is %d\n",vertexPositionAttribLoc);

    vertexTexCoord1AttribLoc = glGetAttribLocation(pObj->shader_uid, "aTextureCoord1");
    printf("vertexPositionAttribLoc is %d\n",vertexTexCoord1AttribLoc);

	vertexTexCoord2AttribLoc = glGetAttribLocation(pObj->shader_uid, "aTextureCoord2");
    printf("vertexTexCoord2AttribLoc is %d\n",vertexTexCoord2AttribLoc);

	blendAttribLoc           = glGetAttribLocation(pObj->shader_uid, "iblendValus");
    printf("blendAttribLoc is %d\n",vertexTexCoord2AttribLoc);

    glGenVertexArrays(4, vaoId);   //生成顶点数组对象
    return 0;  
}

void srv_update_view(surface_data_t *pObj)
{
    glm::mat4 srf_model = glm::mat4(1.0f);//重置模型矩阵,不做任何旋转操作
    //srf_model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f,0.0f,0.0f));//
    //srf_model = glm::rotate(srf_model, angle, glm::vec3(0.0f, 0.0f, 1.0f));//

    //设置观察矩阵
    #ifdef perspview
    glm::mat4 srf_view = glm::lookAt(
                         glm::vec3(
                                  pObj->dist*cos(degreesToRadians(pObj->upon))*sin(degreesToRadians(pObj->angle)), 
                                 -pObj->dist*cos(degreesToRadians(pObj->upon))*cos(degreesToRadians(pObj->angle)), 
                                  pObj->dist*sin(degreesToRadians(pObj->upon))),
                         glm::vec3(0.0f, 0.0f, 0.0f), 
                         glm::vec3(0.0f, 0.0f, 1.0f)
                         );
    //设置透视矩阵
    glm::mat4 srf_projection = glm::perspective(degreesToRadians(45), 960.0f/1080.0f, 0.1f, 1000.0f);
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
    glUniformMatrix4fv(glGetUniformLocation(pObj->shader_uid, "model"), 1, GL_FALSE, &srf_model[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(pObj->shader_uid, "view"), 1, GL_FALSE, &srf_view[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(pObj->shader_uid, "projection"), 1, GL_FALSE, &srf_projection[0][0]);
    
}

//缓冲区分配，向着色器传递数组表格
/*static int srv_init_vertices_vbo(
                         GLuint ArrayBuffer_ID,     //曲面数组缓冲区编号
                         GLuint VertexBuffer_ID,    //曲面顶点缓冲区编号
                         GLuint IndicesBuffer_ID,   //片面索引缓冲区编号
                         surface_data_t *pObj,      //曲面信息
                         GLuint vertex_offset       //偏移信息
)*/
static int srv_init_vertices_vbo(
                                surface_data_t *pObj, 
                                GLuint arryId,
                                GLuint vertexId, 
                                GLuint indexId,
		                        void* vertexBuff, 
                                void* indexBuff,
		                        int vertexBuffSize, 
                                int indexBuffSize
		                        )
{
    //glUseProgram(pObj->shader_uid);
    //glGenVertexArrays(1, &ArrayBuffer_ID);        //生成顶点数组对象
    //glBindVertexArray(ArrayBuffer_ID);              //绑定顶点数组对象

    //--------------------------------------------顶点buffer---------------------------------------------------
    //glGenBuffers(1, &VertexBufferID);                //生成顶点缓冲区对象
    glBindBuffer(GL_ARRAY_BUFFER, vertexId);     //绑定顶点缓冲区对象
    glBufferData(GL_ARRAY_BUFFER, vertexBuffSize,vertexBuff, GL_STATIC_DRAW);//创建并初始化顶点数据

    // position attribute--------------------CPU向GPU传入顶点数组的数据并分配属性----------------------------------//
    //参数1：属性索引  参数2：参数的个数  参数3：参数的类型  参数5：相邻的同属性参数在表格中的跨度  参数6：第一个参数在表格中的偏移量
    // vertex
    glVertexAttribPointer(vertexPositionAttribLoc, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)0);
    glVertexAttribPointer(vertexTexCoord1AttribLoc, 2, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(3 * sizeof(float)));
    glVertexAttribPointer(vertexTexCoord2AttribLoc, 2, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(5 * sizeof(float)));
    glVertexAttribPointer(blendAttribLoc, 2, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(7 * sizeof(float)));  //混合权重

    //target指定目标缓冲区对象。 符号常量必须为GL_ARRAY_BUFFER或GL_ELEMENT_ARRAY_BUFFER。
    //size指定缓冲区对象的新数据存储的大小（以字节为单位）。
    //data指定将复制到数据存储区以进行初始化的数据的指针，如果不复制数据，则指定NULL。
    //usage指定数据存储的预期使用模式。 符号常量必须为GL_STREAM_DRAW，GL_STATIC_DRAW或GL_DYNAMIC_DRAW。

    //glGenBuffers(1, &IndicesBuffer_ID);       //生成索引缓冲区对象
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexId);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexBuffSize, indexBuff,GL_STATIC_DRAW);
    
    //Enable for the rendering
	glEnableVertexAttribArray(vertexPositionAttribLoc);
	glEnableVertexAttribArray(vertexTexCoord1AttribLoc);
	glEnableVertexAttribArray(vertexTexCoord2AttribLoc);
	glEnableVertexAttribArray(blendAttribLoc);

    return 0;
}

void surroundview_init_vertices_vbo_wrap(surface_data_t *pObj)
{
	int i;
	GLuint vertex_offset = 0;
    float* data_Get;
	for(i = 0;i < QUADRANTS;i ++)
	{
		vertex_offset = i * (sizeof(srv_lut_t)*QUADRANT_WIDTH*QUADRANT_HEIGHT);

        //Blending is included in vertex_array
        srv_init_vertices_vbo(
                pObj, 
                vaoId[i], vboId[i*2], vboId[i*2+1],
		        (char*)pObj->vertex_arry+vertex_offset, 
                (char*)pObj->indices_arry,
		        sizeof(srv_lut_t)*QUADRANT_WIDTH*QUADRANT_HEIGHT, 
                sizeof(unsigned int)*(QUADRANT_WIDTH-1)*(QUADRANT_HEIGHT-1)*2*3
                );

	}
	index_buffer_changed = false;
}


//纹理绑定
//可修改为仅绑定两个纹理
void onscreen_mesh_state_restore_program_textures_attribs(GLuint *texYuv, int tex1, int tex2,surface_data_t *pObj)
{

    glUseProgram(pObj->shader_uid);
	//set the program we need
    glUniform1i(samplerLocation0, 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texYuv[tex1]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glUniform1i(samplerLocation1, 1);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, texYuv[tex2]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glEnableVertexAttribArray(vertexPositionAttribLoc);
	glEnableVertexAttribArray(vertexTexCoord1AttribLoc);
	glEnableVertexAttribArray(vertexTexCoord2AttribLoc);
	glEnableVertexAttribArray(blendAttribLoc);

}


void onscreen_mesh_state_restore_vbo(GLuint arrayId,GLuint vertexId, GLuint indexId)
{
	//restore the vertices and indices
    glBindVertexArray(arrayId);

	glBindBuffer(GL_ARRAY_BUFFER, vertexId);
	glVertexAttribPointer(vertexPositionAttribLoc, 3, GL_FLOAT, GL_FALSE, sizeof(srv_lut_t), (void*)0);
	glVertexAttribPointer(vertexTexCoord1AttribLoc, 2, GL_FLOAT, GL_FALSE, sizeof(srv_lut_t), (void*)(3 * sizeof(float)));
	glVertexAttribPointer(vertexTexCoord2AttribLoc, 2, GL_FLOAT, GL_FALSE, sizeof(srv_lut_t), (void*)(5 * sizeof(float)));
    glVertexAttribPointer(blendAttribLoc, 2, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(7 * sizeof(float)));  //混合权重
	//Index buffer
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexId);

    //Enable for the rendering
	//glEnableVertexAttribArray(vertexPositionAttribLoc);
	//glEnableVertexAttribArray(vertexTexCoord1AttribLoc);
	//glEnableVertexAttribArray(vertexTexCoord2AttribLoc);
	//glEnableVertexAttribArray(blendAttribLoc);
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
    
    if(index_buffer_changed == true)
	{
        glGenBuffers(QUADRANTS*2, vboId);
		surroundview_init_vertices_vbo_wrap(pObj);
    }
    //更新MVP到着色器
    srv_update_view(pObj);
    for(int i=0;i<4;i++)
    {
        //绑定纹理
        //int i = 2;
        onscreen_mesh_state_restore_program_textures_attribs(texYuv, i%4, (3+i)%4,pObj);   
	    //向顶点缓冲区写新数据，顶点坐标，纹理坐标，blending值
        onscreen_mesh_state_restore_vbo(vaoId[i],vboId[i*2], vboId[i*2+1]); 
        glDrawElements(GL_TRIANGLES, pObj->indices_num*3, GL_UNSIGNED_INT, 0);  
    }
    /*for(int j =0;j<8;j++)
    {
     glDeleteBuffers(1,&vboId[j]);  //
    }*/
    glBindBuffer(GL_ARRAY_BUFFER, 0);                       //绑定指向0
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);               //绑定指向0
	glFlush();
}