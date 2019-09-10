#include "renderutils.h"

unsigned long getFileLength(FILE *file)
{
	int ret;

	if(!file) return 0;

	fseek(file, 0L, SEEK_END);
	ret = ftell(file);
	fseek(file, 0L, SEEK_SET);

	return ret;
}

int renderutils_loadShader(char* filename, GLchar** shader_source)
{
	FILE *file;
	unsigned long len;
	size_t readlen;
	int err;
	file = fopen(filename, "r");
	if(!file)
	{
		ru_perror("Unable to open file: %s\n", filename);
		err = -ENOENT;
		goto exit;
	}

	len = getFileLength(file);

	if (len <= 0)
	{
		ru_perror("Shader file empty: %s\n", filename);
		err = -ENOENT;
		goto close_file;
	} 

	*shader_source = (GLchar *) new char[len+1];
	if (*shader_source == 0)
	{
		ru_perror("Unable to allocate memory for shader: %s\n", filename);
		err = -ENOMEM;   // can't reserve memory
		goto close_file;
	}

	// len isn't always strlen cause some characters are stripped in ascii read...
	// it is important to 0-terminate the real length later, len is just max possible value... 
	(*shader_source)[len] = (char)0;

	readlen = fread(*shader_source, 1, len, file);

	if(readlen <= 0)
	{
		ru_perror("Unable to read shader source from file: %s. fread returns: %d\n",
				filename,
				(int)readlen);
		err = -EBADF;
		delete[] *shader_source;
		goto close_file;
	}

	(*shader_source)[readlen] = (char)0;  // 0-terminate it at the correct position
	err = 0;

close_file:
	fclose(file);
exit:
	return err; // No Error
}


int renderutils_unloadShader(GLchar** shader_source)
{
	if (*shader_source != 0)
		delete[] *shader_source;
	*shader_source = 0;
	return 0;
}

GLuint renderutils_compileShader(GLenum shaderType, const GLchar* pSource) {
   GLuint shader = glCreateShader(shaderType);
   if (shader) {
       glShaderSource(shader, 1, &pSource, NULL);
       glCompileShader(shader);
       GLint compiled = 0;
       glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
       if (!compiled) {
           GLint infoLen = 0;
           glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
           printf("%d\n",infoLen);
           if (infoLen) {
               char* buf = (char*) malloc(infoLen);
               if (buf) {
                   glGetShaderInfoLog(shader, infoLen, NULL, buf);
                   ru_perror(" GL: Could not compile shader %d:\n%s\n",
                       shaderType, buf);
                   free(buf);
               }
           } else {
               char* buf = (char*) malloc(0x1000);
               if (buf) {
                   glGetShaderInfoLog(shader, 0x1000, NULL, buf);
                   ru_perror(" GL: Could not compile shader %d:\n%s\n",
                   shaderType, buf);
                   printf("22222222222222222222222222222222\n");
                   free(buf);
               }
           }
           glDeleteShader(shader);
           shader = 0;
       }
   }
   return shader;
}

GLuint renderutils_createProgram(const GLchar* pVertexSource, const GLchar* pFragmentSource)
{
   GLuint vertexShader = renderutils_compileShader(GL_VERTEX_SHADER, pVertexSource);
   if (!vertexShader) {
       return 0;
   }

   GLuint pixelShader = renderutils_compileShader(GL_FRAGMENT_SHADER, pFragmentSource);
   if (!pixelShader) {
       return 0;
   }

   GLuint program = glCreateProgram();
   if (program) {
       glAttachShader(program, vertexShader);
//       ru_checkGlError("glAttachShader");
       glAttachShader(program, pixelShader);
//       ru_checkGlError("glAttachShader");
       glLinkProgram(program);
       GLint linkStatus = GL_FALSE;
       glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
       if (linkStatus != GL_TRUE) {
           GLint bufLength = 0;
           glGetProgramiv(program, GL_INFO_LOG_LENGTH, &bufLength);
           if (bufLength) {
               char* buf = (char*) malloc(bufLength);
               if (buf) {
                   glGetProgramInfoLog(program, bufLength, NULL, buf);
                   ru_perror(" GL: Could not link program:\n%s\n", buf);
                   free(buf);
               }
           }
           glDeleteProgram(program);
           program = 0;
       }
   }
   if(vertexShader && pixelShader && program)
   {
     glDeleteShader(vertexShader);
     glDeleteShader(pixelShader);
    }
   return program;
}


GLuint renderutils_loadAndCreateProgram(char* vshfile, char* fshfile)
{
    GLchar *vshader, *fshader;
    if (renderutils_loadShader(vshfile, &vshader) < 0)
    {
        ru_perror("Cannot load vertex shader: %s\n", vshfile);
		return 0;
    }
    if (renderutils_loadShader(fshfile, &fshader) < 0)
    {
        ru_perror("Cannot load fragment shader: %s\n", fshfile);
		return 0;
    }
	return renderutils_createProgram(vshader, fshader);
}






