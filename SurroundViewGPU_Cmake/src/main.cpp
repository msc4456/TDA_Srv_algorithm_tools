#include <glad/glad.h>
#include <GLFW/glfw3.h>
#define STB_IMAGE_IMPLEMENTATION
//#include "learnopengl/stb_image.h"
#include <unistd.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "common/stb_image.h"
#include "common/luma_calc.h"
#include "common/Drawpara.h"
#include "s1_car_draw/car.h"
#include "s2_srv_draw/srv.h"
#include <iostream>
#include <cstdio>

//#define LUMA_AVG
#define STAND_ALONG

#define MAX_VIEWPORTS 2

using namespace std;

bool seq_color = false;
//定义绘图窗口尺寸
const GLuint src_wind_h =1080;
const GLuint src_wind_w =1080;

//定义亮度统计因子
float factor1,factor2,factor3,factor4;

surface_data_t *srv_renderObj;

glm::mat4 mProjection[MAX_VIEWPORTS];
// Camera matrix
glm::mat4 mView[MAX_VIEWPORTS];
// Model matrix : an identity matrix (model will be at the origin)
glm::mat4 mModel_bowl[MAX_VIEWPORTS];  // Changes for each model !
// Our ModelViewProjection : multiplication of our 3 matrices
glm::mat4 mMVP_bowl[MAX_VIEWPORTS];
glm::mat4 mMVP_car[MAX_VIEWPORTS];

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window);
unsigned char *pixeldata = (unsigned char*)malloc(20*20*3);
int main()
{
    // glfw初始化配置
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // 使用glfw创建窗口
    GLFWwindow* window = glfwCreateWindow(src_wind_h, src_wind_w, "Surroundview_sim", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;     //窗口创建不成功弹出错误
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);                                   //创建上下文
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);//注册画布调整回调函数

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }
    //--------------------------------绑定静态纹理————---------------------------------------//
    GLuint texture[4];
    #ifdef STAND_ALONG
    standalone_init_texture("../../ext/resource/image/","bmp",texture);
    #elif
    /* code  stream_copy = 1*/ //从framebuffer中拷贝图像数据进行纹理绑定
    #endif
    
    surface_data_t renderobj;
    srv_renderObj = &renderobj;
    //初始化渲染对象
    if(!srv_setup(srv_renderObj));
    {
        printf("srv_setup failed\n");
    }
    if(car_init()){ printf("load_3d model files failed\n");return -1;}  //返回零则初始化成功

    //--------------------------------------渲染循环---------------------------------------//
    while (!glfwWindowShouldClose(window))
    {
        processInput(window);

        // render
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        glDepthFunc(GL_LEQUAL);
        glEnable(GL_DEPTH_TEST);//通过关闭深度测试构建透明效果
        //glEnable(GL_CULL_FACE);
        //glCullFace(GL_FRONT);

        srv_draw(srv_renderObj,texture,1);
        srv_update_view(srv_renderObj);     //必须在启用着色器后使用

        //绘制车模型
        //car_draw(1,srv_renderObj->upon,srv_renderObj->angle,srv_renderObj->dist);

        glFlush();
        usleep(22000);

        //交换缓存
        glfwSwapBuffers(window);
        glfwPollEvents();        //接收事件信息并返回
    }
    
    // -------------------------------------------释放内存-----------------------------------//
    car_deinit();//释放模型

    glfwTerminate();//清理所有未完成的GLFW资源的内存
    return 0;
}
//----------------------------键盘服务函数-------------------------------------//

void processInput(GLFWwindow *window)
{
    usleep(100);
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) 
    {
      srv_renderObj->upon+= 2.0f;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) 
    {
       srv_renderObj->upon-= 2.0f;
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) 
    {  
       srv_renderObj->angle+= 2.0f;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) 
    {
       srv_renderObj->angle-= 2.0f;
    }
    if (glfwGetKey(window, GLFW_KEY_8) == GLFW_PRESS) 
    {
       seq_color = false;
       srv_renderObj->dist-= 0.1f;
    }
    if (glfwGetKey(window, GLFW_KEY_5) == GLFW_PRESS) 
    {
       seq_color = true;
       srv_renderObj->dist+= 0.1f;
    }
    if(glfwGetKey(window, GLFW_KEY_7) == GLFW_PRESS)
    {
    //glCopyPixels(0,0,20,20,GL_COLOR);
    glReadPixels(0,0,20,20,GL_RGB,GL_BYTE,pixeldata);//从frame buffer中读取像素数据
    for(int i =0;i<20;i++)
    {
        printf("pixeldata is %d\n",pixeldata[i]);
    }
    }
    // if(srv_renderObj->upon>90.0)   srv_renderObj->upon  =  90.0;
    // if(srv_renderObj->upon<21.0)   srv_renderObj->upon  =  21.0;
    // if(srv_renderObj->angle>90.0)  srv_renderObj->angle =  90.0;
    // if(srv_renderObj->angle<-90.0) srv_renderObj->angle = -90.0;
    //printf("%f,%f\n",-view_dist*cos(c*upon),view_dist*sin(c*upon));
    //printf("%f,%f\n",upon,angle);
    
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    /*     if (height == 0)	             // 防止除数为零
	{
		height = 1;                      // 
	}
    SRV_SingleWind_H =height;
    SRV_SingleWind_W =picture_ratio1*SRV_SingleWind_H;

    //拼接图区域
    SRV_MultWind_H   =height;
    SRV_MultiWind_W  =SRV_MultWind_H*picture_ratio2;

    //窗口总尺寸
    SRV_MainWind_H   =SRV_SingleWind_H;
    SRV_MainWind_W   =SRV_SingleWind_W+SRV_MultiWind_W;*/
    glViewport(0,0,height*src_wind_w/src_wind_h,height);
}