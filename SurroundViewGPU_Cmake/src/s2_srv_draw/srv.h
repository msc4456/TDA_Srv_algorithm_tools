/*
 *******************************************************************************
 *
 * Copyright (C) 2016 Texas Instruments Incorporated - http://www.ti.com/
 * ALL RIGHTS RESERVED
 *
 *******************************************************************************
 */

#ifndef _SRV_H_
#define _SRV_H_

#include <glad/glad.h>

//曲面对象数据结构
typedef struct{
    // 3d-lut
    float*vertex_arry;
    unsigned int*indices_arry;
    unsigned int vertex_num;
    unsigned int indices_num;
    
    //render context setting
    GLfloat screen_width;
    GLfloat screen_height;
    GLuint shader_uid;
    GLuint view_portID;

    //view pos parameter
    GLfloat dist;
    GLfloat upon;
    GLfloat angle;

}surface_data_t;


typedef struct _srv_lut_t
{
    float x;
    float y;
    float z;
    float u1;
    float v1;
    float u2;
    float v2;
    float blend1;
    float blend2;
}srv_lut_t;

//srv functions
int srv_setup(surface_data_t*pObj);
int standalone_init_texture(const char*base_path,const char*filetype,GLuint (&texYuv)[4]);
void srv_update_view(surface_data_t *pObj);
void srv_draw(surface_data_t*pObj,GLuint *texYuv,GLuint Viewport_id);
GLuint Shader_create(const char* vShaderCode, const char* fShaderCode);
      
#endif /*   _SRV_H_    */
