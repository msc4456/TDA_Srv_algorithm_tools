/*
 *******************************************************************************
 *
 * Copyright (C) 2013 Texas Instruments Incorporated - http://www.ti.com/
 * ALL RIGHTS RESERVED
 *
 *******************************************************************************
 */

#ifndef _RENDERUTILS_H_
#define _RENDERUTILS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef _WIN32
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#else
#include <GLES2/gl2.h>
#include <EGL/egl.h>
#endif

#define ru_perror(...) printf(__VA_ARGS__)

/* Function prototypes */
int renderutils_loadShader(char* filename, GLchar** shader_source);
int renderutils_unloadShader(GLchar** shader_source);
GLuint renderutils_compileShader(GLenum shaderType, const GLchar* pSource);
GLuint renderutils_createProgram(const GLchar* pVertexSource, const GLchar* pFragmentSource);
GLuint renderutils_loadAndCreateProgram(char* vshfile, char* fshfile);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _RENDERUTILS_H_ */

