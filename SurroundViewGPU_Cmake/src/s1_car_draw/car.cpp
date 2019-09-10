/*
备注：SRV_CAR_MODEL_PATH 处设置模型的相对路径
 */

#include "car.h"
#include "../shader/renderutils.h"
#include "../Pod_tools/PVRTModelPOD.h"
//#include <GL/glext.h>  //包含GL_BGR等扩展纹理类型

#define GLM_FORCE_RADIANS
#include <glm/vec3.hpp> // glm::vec3
#include <glm/vec4.hpp> // glm::vec4, glm::ivec4
#include <glm/mat4x4.hpp> // glm::mat4
#include <glm/gtc/matrix_transform.hpp> // glm::translate, glm::rotate, glm::scale, glm::perspective
#include <glm/gtc/type_ptr.hpp> // glm::value_ptr
#include <glm/gtx/string_cast.hpp>// 用于打印 mat4 查看当前状态

#include <limits.h>

#if !defined(SRV_CAR_MODEL_PATH)
#define SRV_CAR_MODEL_PATH "../../ext/resource/models"  //关键路径,以最后生成Surroundview的可执行文件来索引路径
#endif


//#define GL_BGR        0x80E0

extern glm::mat4 mProjection[];
extern glm::mat4 mView[];
extern glm::mat4 mMVP_car[];

int num_viewports = 0;
/******************************************************************************
 Defines
******************************************************************************/
// Index to bind the attributes to vertex shaders
GLuint aVertex;
GLuint aNormal;
GLuint aTexCoord;
#define VERTEX_ARRAY    aVertex
#define NORMAL_ARRAY    aNormal
#define TEXCOORD_ARRAY  aTexCoord

/******************************************************************************
 Consts
******************************************************************************/
// Camera constants. Used for making the projection matrix
const float g_fCameraNear = 4.0f;
const float g_fCameraFar  = 5000.0f;

const float g_fDemoFrameRate = 1.0f / 30.0f;

// The camera to use from the pod file
const int g_ui32Camera = 0;

typedef struct
{
	GLfloat x;
	GLfloat y;
	GLfloat z;
} vertex3df;

typedef struct _srv_viewport_t
{
	unsigned int x;
	unsigned int y;
	unsigned int width;
	unsigned int height;
	bool animate;
} srv_viewport_t;

srv_viewport_t srv_viewports[] = {
	{
		0,//x : 0,
		0,//y : 0,
		960,//width : 960,
		1080,//height: 1080,
		true,//animate: true,
	},
	{
		960,//x : 960,
		0,//y : 0,
		960,//width : 960,
		1080,//height: 1080,
		false,//animate: false,
	}
};
/******************************************************************************
 Content file names
******************************************************************************/

//模型绘制顶点着色器
static const char vshader_car[] =
"   #version 150 core\n"   //指定GLSL版本，TDA2X不支持3.0及以上的GLSL
"	attribute vec3 inVertex;\n"
"	attribute vec3 inNormal;\n"
"	attribute vec2 inTexCoord;\n"
"	uniform mat4  MVPMatrix;\n"     //模型，观察，透视复合矩阵
//"	uniform mat4  car_model;\n"     //模型矩阵
//"	uniform mat4  car_view;\n"      //观察矩阵
//"	uniform mat4  car_projection;\n"//透视矩阵  
"	uniform vec3  LightDirection;\n"
"	varying float  LightIntensity;\n"
"	varying vec2   TexCoord;\n"
"	void main()\n"
"	{\n"
//"	    gl_Position = car_projection * car_view * car_model * vec4(inVertex, 1.0);\n"
"       gl_Position = MVPMatrix*vec4(inVertex, 1.0);\n"
"	    TexCoord = inTexCoord;\n"
"	    LightIntensity = dot(inNormal, -LightDirection);\n"
"    } \n";

//模型绘制片段着色器
static const char fshader_car[] =
"   #version 150 core\n" //指定GLSL版本，TDA2X不支持3.0及以上的GLSL
"	precision mediump float;\n"
"	uniform sampler2D  sTexture;\n"
"	varying float  LightIntensity;\n"
"	varying vec2   TexCoord;\n"
"	void main()\n"
"	{\n"
"		gl_FragColor.rgb = texture2D(sTexture, TexCoord).bbb;\n"
"		gl_FragColor.a = 1.0;\n"
"   } \n";

// OpenGL handles for shaders, textures and VBOs
GLuint m_uiVertShader;
GLuint m_uiFragShader;

// Group shader programs and their uniform locations together
//定义car 运行程序对象
struct _car_program_t
{
        GLuint uiId;
        GLuint uiMVPMatrixLoc;
        GLuint uiLightDirLoc;
}
car_program;

//定义car数据结构
typedef struct _car_data
{
	const char *foldername;
	const char *filename;
	CPVRTModelPOD   *scene;
	GLuint* vbo;
	GLuint* indices;
	GLuint* texture_ids;
	GLint xrot_degrees;
	GLint yrot_degrees;
	GLint zrot_degrees;
	GLfloat scale;
} car_data_t;

//初始化car对象，结构体数组，一次性申请三个car对象，并初始化car模型数据
car_data_t car_data[] = {
	{
		"jeep",//foldername   : "jeep",
		"jeep.pod",//filename     : "jeep.pod",
		NULL,//scene        : NULL,
		NULL,//vbo          : NULL,
		NULL,//indices      : NULL,
		NULL,//texture_ids  : NULL,
		90,//xrot_degrees : 90,
		180,//yrot_degrees : 180,
		0,//zrot_degrees : 0,
		25.0f,//scale        : 25.0f,
	},
	{
		"suv",//foldername   : "suv",
		"suv.pod",//filename     : "suv.pod",
		NULL,//scene        : NULL,
		NULL,//vbo          : NULL,
		NULL,//indices      : NULL,
		NULL,//texture_ids  : NULL,
		90,//xrot_degrees : 90,
		180,//yrot_degrees : 180,
		0,//zrot_degrees : 0,
		15.0f,//scale        : 15.0f,
	},
	{
		"sedan_generic",//foldername   : "sedan_generic",
		"sedan_generic.pod",//filename     : "sedan_generic.pod",
		NULL,//scene        : NULL,
		NULL,//vbo          : NULL,
		NULL,//indices      : NULL,
		NULL,//texture_ids  : NULL,
		90,//xrot_degrees : 90,
		0,//yrot_degrees : 0,
		0,//zrot_degrees : 0,
		25.0f,//scale        : 25.0f,
	}
};

car_data_t *active_car;
int active_car_index = 1;
int num_car_models;


int load_texture(GLuint tex, int width, int height, int textureType, void* data)
{
    GLenum target = GL_TEXTURE_2D;
    GLint param = GL_NEAREST;

    if ((textureType == GL_RGB) || (textureType == GL_RGBA)) {
        target = GL_TEXTURE_2D;
        //param = GL_NEAREST;
		param = GL_LINEAR;

        glPixelStorei(GL_UNPACK_ALIGNMENT,1);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            textureType,
            width,
            height,
            0,
            textureType,
            GL_UNSIGNED_BYTE,//textureFormat,
            data
            );
        GL_CHECK(glTexImage2D);
		//glGenerateMipmap(GL_TEXTURE_2D);
    } else {
        printf("Incorrect texture type %x\n", textureType);
        return -1;
    }

    glTexParameteri(target, GL_TEXTURE_MIN_FILTER, param); //GL_LINEAR);
    glTexParameteri(target, GL_TEXTURE_MAG_FILTER, param); //GL_LINEAR);
    glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_REPEAT);
    GL_CHECK(glTexParameteri);
    return 0;
}

int load_texture_from_raw_file(GLuint tex, int width, int height, int textureType, const char* filename, int offset)
{
    int ret;
    void* data;
    FILE* fp;
    int dataread;
    int numbytes = 0;
    //open and load raw file
    switch (textureType) {
        case GL_RGBA:
            numbytes = 4 * width * height;
            break;

        case GL_RGB:
            numbytes = 3 * width * height;
            break;

        default:
            printf("Invalid texture type %d\n", textureType);
            return -1;
    }

    fp = fopen(filename, "rb");
    if(!fp)
    {
        printf("Error opening file %s\n", filename);
        return -1;
    }

    data = malloc(numbytes);
    fseek(fp, offset, SEEK_CUR);
    dataread = fread(data, 1, numbytes, fp);
    fclose(fp);
    if(dataread != numbytes) {
        printf("Error in file size != width*height\n");
        return -1; //TODO: reinstate this
    }

    ret = load_texture(tex, width, height, textureType, data);

    free(data);

    return ret;
}

int load_texture_bmp(GLuint tex, const char *filename)
{
	FILE *fp;
	uint8_t header[54];
	uint32_t width, height, offset;
	uint16_t bpp;
	int type;

	fp = fopen(filename, "rb");
	if(!fp)
	{
		fprintf(stderr, "cannot open bmp file: %s\n", filename);
		return -1;
	}

	fread(header, sizeof(uint8_t), 54, fp);


	offset = *(int *)(&header[10]);
	width  = *(int *)(&header[18]);
	height = *(int *)(&header[22]);
	bpp    = *(int *)(&header[28]);

	fclose(fp);

	switch(bpp)
	{
		case 24:
			type = GL_RGB;//此处未兼容BGR，TDA GPU不支持BGR，需要将YUV带入前进行色序调整
			///usr/include/GLES2/gl2.h
			break;
		case 32:
			type = GL_RGBA;
			break;
		default:
			printf("Invalid number of bits per pixel: %d in %s\n", bpp, filename);
			return -1;
	}
	return(load_texture_from_raw_file(tex, width, height, type, filename, offset));
}

void get_bmp_info(const char *filename, uint32_t *width, uint32_t *height, uint32_t *bpp)
{
	FILE *fp;
	uint8_t header[54];

	fp = fopen(filename, "rb");
	fread(header, sizeof(uint8_t), 54, fp);

	*width  = *(uint32_t *)(&header[18]);
	*height = *(uint32_t *)(&header[22]);
	*bpp    = *(uint32_t *)(&header[28]);

	fclose(fp);
}
//int load_texture_bmp(GLuint tex, const char *filename);
//int load_texture_from_raw_file(GLuint tex, int width, int height, int textureType, const char* filename, int offset);

bool LoadTextures(car_data_t *cd)
{
	    int err;
        cd->texture_ids = new GLuint[cd->scene->nNumMaterial];

        if(!cd->texture_ids)
        {
                printf("ERROR: Insufficient memory.\n");
                return false;
        }

        for(int i = 0; i < (int) cd->scene->nNumMaterial; ++i)
        {
                cd->texture_ids[i] = 0;
                SPODMaterial* pMaterial = &cd->scene->pMaterial[i];

                if(pMaterial->nIdxTexDiffuse != -1)
                {
                        char filename[1024];
                        char * sTextureName = cd->scene->pTexture[pMaterial->nIdxTexDiffuse].pszName;
                        sprintf(filename, "%s/%s/%s", SRV_CAR_MODEL_PATH,
                                cd->foldername,
								sTextureName);
                        glGenTextures(1, &cd->texture_ids[i]);
                        glActiveTexture(GL_TEXTURE5);
                        glBindTexture(GL_TEXTURE_2D, cd->texture_ids[i]);
                        err = load_texture_bmp( cd->texture_ids[i], filename);
                        if(err != 0)
                        {
                            fprintf(stderr, "Failed to load texture: %s\n", filename);
                            return false;
                        }
                }
        }
        return true;
}

bool LoadShaders()
{
		car_program.uiId = renderutils_createProgram(vshader_car, fshader_car);
		if(car_program.uiId == 0)
		{
			printf("Car program could not be created\n");
			return false;
		}
		printf("Car program has be created\n");
        glUseProgram(car_program.uiId);

		aVertex = glGetAttribLocation(car_program.uiId, "inVertex");
		aNormal = glGetAttribLocation(car_program.uiId, "inNormal");
		aTexCoord = glGetAttribLocation(car_program.uiId, "inTexCoord");

        // Set the sampler2D variable to the first texture unit
        glUniform1i(glGetUniformLocation(car_program.uiId, "sTexture"), 5);

        // Store the location of uniforms for later use
        car_program.uiMVPMatrixLoc  = glGetUniformLocation(car_program.uiId, "MVPMatrix");
        car_program.uiLightDirLoc   = glGetUniformLocation(car_program.uiId, "LightDirection");
        return true;
}

bool LoadVbos(car_data_t *cd)
{
        if(!cd->scene->pMesh[0].pInterleaved)
        {
                printf("ERROR: Only interleaved vertex data is supported.\n");
                return false;
        }

        if (!cd->vbo)      cd->vbo = new GLuint[cd->scene->nNumMesh];
        if (!cd->indices)  cd->indices = new GLuint[cd->scene->nNumMesh];

        /*
                Load vertex data of all meshes in the scene into VBOs

                The meshes have been exported with the "Interleave Vectors" option,
                so all data is interleaved in the buffer at pMesh->pInterleaved.
                Interleaving data improves the memory access pattern and cache efficiency,
                thus it can be read faster by the hardware.
        */
        glGenBuffers(cd->scene->nNumMesh, cd->vbo);
        for (unsigned int i = 0; i < cd->scene->nNumMesh; ++i)
        {
                // Load vertex data into buffer object
                SPODMesh& Mesh = cd->scene->pMesh[i];
                PVRTuint32 uiSize = Mesh.nNumVertex * Mesh.sVertex.nStride;
                glBindBuffer(GL_ARRAY_BUFFER, cd->vbo[i]);
                glBufferData(GL_ARRAY_BUFFER, uiSize, Mesh.pInterleaved, GL_STATIC_DRAW);

                // Load index data into buffer object if available
                cd->indices[i] = 0;
                if (Mesh.sFaces.pData)
                {
                        glGenBuffers(1, &cd->indices[i]);
                        uiSize = PVRTModelPODCountIndices(Mesh) * Mesh.sFaces.nStride;
                        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cd->indices[i]);
                        glBufferData(GL_ELEMENT_ARRAY_BUFFER, uiSize, Mesh.sFaces.pData, GL_STATIC_DRAW);
                }
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	return true;
}

bool InitView()
{
    //设置汽车观察视角
	EPVRTError err;
	int at_least_one_model = 0;
	num_car_models = sizeof(car_data)/sizeof(car_data_t);//计算有多少个车模型
	printf("the num_car_models is %d\n",num_car_models); 
	for(int i = 0; i < num_car_models; i++)
	{
		CPVRTModelPOD scene;
		car_data[i].scene = new CPVRTModelPOD;
		char filename[1024];
		sprintf(filename, "%s/%s/%s", SRV_CAR_MODEL_PATH,
				car_data[i].foldername,
				car_data[i].filename);
		printf("the path of pod files is ||: %s\n",filename);
		// Load the scene
		if(scene.ReadFromFile(filename) != PVR_SUCCESS)
		{
			printf("Couldn't load .pod file: %s\n", car_data[i].filename);
			delete car_data[i].scene;
			car_data[i].scene = NULL;
			printf("001 some error happend\n");
			continue;
		}

		// Make sure meshes are non-interleaved for flattening to world space
		for (unsigned int m = 0; m < scene.nNumMesh; ++m)
		{
			// Load vertex data into buffer object
			SPODMesh& Mesh = scene.pMesh[m];
			if(Mesh.pInterleaved)
				PVRTModelPODToggleInterleaved(Mesh, 1);
		}

		err = PVRTModelPODFlattenToWorldSpace(scene, *(car_data[i].scene));
		if(err)
		{
			printf("Could not flatten POD model: %d\n", err);
			delete car_data[i].scene;
			car_data[i].scene = NULL;
			printf("002 some error happend\n");
			continue;
		}

		// Make sure meshes are interleaved for use in our code
		for (unsigned int m = 0; m < car_data[i].scene->nNumMesh; ++m)
		{
			// Load vertex data into buffer object
			SPODMesh& Mesh = car_data[i].scene->pMesh[m];
			if(!Mesh.pInterleaved)
				PVRTModelPODToggleInterleaved(Mesh, 1);
		}

		/*
		   Initialize VBO data
		 */
		if(!LoadVbos(&car_data[i]))
		{
			printf("Couldn't load vbos for .pod file: %s\n", car_data[i].filename);
			delete car_data[i].scene;
			delete car_data[i].vbo;
			delete car_data[i].indices;
			car_data[i].scene = NULL;
			car_data[i].vbo = NULL;
			car_data[i].indices = NULL;
			printf("003 some error happend\n");
			continue;	
		}

		/*
		   Load textures
		 */
		if(!LoadTextures(&car_data[i]))
		{
			printf("Couldn't load textures for .pod file: %s\n", car_data[i].filename);
			delete car_data[i].scene;
			delete car_data[i].vbo;
			delete car_data[i].indices;
			delete car_data[i].texture_ids;
			car_data[i].scene = NULL;
			car_data[i].vbo = NULL;
			car_data[i].indices = NULL;
			car_data[i].texture_ids = NULL;
			printf("004 some error happend\n");
			continue;
		}
		at_least_one_model = 1;
	}

	if(!at_least_one_model)
	{
		printf("ERROR: Couldn't load any car model\n");
		return false;
	}

	while(car_data[active_car_index].scene == NULL)
		active_car_index = (active_car_index + 1) % num_car_models;

        /*
                Load and compile the shaders & link programs
        */
	    bool flag_loadshader =LoadShaders();
        if(!flag_loadshader)
        {
        		printf("Failed to load shaders\n");
                //PVRShellSet(prefExitMessage, ErrorStr.c_str());
                return false;
        }
        /*
                Initialize Print3D
        */
        // Enable backface culling and depth test

        //glCullFace(GL_BACK);    //背面剔除,若不省略会导致碗形曲面立面无法显示
        //glEnable(GL_CULL_FACE); //背面剔除

        return true;
}

bool ReleaseView()
{
	for(int i = 0; i < num_car_models; i++)
	{
		// Deletes the textures
		if(car_data[i].scene)
		{
			glDeleteTextures(car_data[i].scene->nNumMaterial, &car_data[i].texture_ids[0]);

			// Delete buffer objects
			glDeleteBuffers(car_data[i].scene->nNumMesh, car_data[i].vbo);
			glDeleteBuffers(car_data[i].scene->nNumMesh, car_data[i].indices);

			// Free resources
			delete[] car_data[i].vbo;
			delete[] car_data[i].indices;
			delete[] car_data[i].texture_ids;
			delete car_data[i].scene;

			car_data[i].vbo = NULL;
			car_data[i].indices = NULL;
			car_data[i].texture_ids = NULL;
			car_data[i].scene = NULL;
		}
	}

	// Delete program and shader objects
	glDeleteProgram(car_program.uiId);

	glDeleteShader(m_uiVertShader);
	glDeleteShader(m_uiFragShader);
    return true;
}

void DrawMesh(car_data_t *cd, int i32NodeIndex)
{
        int i32MeshIndex = cd->scene->pNode[i32NodeIndex].nIdx;
        SPODMesh* pMesh = &cd->scene->pMesh[i32MeshIndex];

		//FIXME: Hack for jeep model to remove the back wheel
		//if(i32NodeIndex == 5)
		//	return;

        // bind the VBO for the mesh
        glBindBuffer(GL_ARRAY_BUFFER, cd->vbo[i32MeshIndex]);
        // bind the index buffer, won't hurt if the handle is 0
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cd->indices[i32MeshIndex]);

        // Enable the vertex attribute arrays
        glEnableVertexAttribArray(VERTEX_ARRAY);
        glEnableVertexAttribArray(NORMAL_ARRAY);
        glEnableVertexAttribArray(TEXCOORD_ARRAY);

        // Set the vertex attribute offsets
        glVertexAttribPointer(VERTEX_ARRAY, 3, GL_FLOAT, GL_FALSE, pMesh->sVertex.nStride, pMesh->sVertex.pData);
        glVertexAttribPointer(NORMAL_ARRAY, 3, GL_FLOAT, GL_FALSE, pMesh->sNormals.nStride, pMesh->sNormals.pData);
        glVertexAttribPointer(TEXCOORD_ARRAY, 2, GL_FLOAT, GL_FALSE, pMesh->psUVW[0].nStride, pMesh->psUVW[0].pData);

        if(pMesh->nNumStrips == 0)
        {
                if(cd->indices[i32MeshIndex])
                {
                        // Indexed Triangle list

                        // Are our face indices unsigned shorts? If they aren't, then they are unsigned ints
                        GLenum type = (pMesh->sFaces.eType == EPODDataUnsignedShort) ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT;
                        glDrawElements(GL_TRIANGLES, pMesh->nNumFaces*3, type, 0);
                }
                else
                {
                        // Non-Indexed Triangle list
                        glDrawArrays(GL_TRIANGLES, 0, pMesh->nNumFaces*3);
                }
        }
        else
        {
                PVRTuint32 offset = 0;

                // Are our face indices unsigned shorts? If they aren't, then they are unsigned ints
                GLenum type = (pMesh->sFaces.eType == EPODDataUnsignedShort) ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT;

                for(int i = 0; i < (int)pMesh->nNumStrips; ++i)
                {
                        if(cd->indices[i32MeshIndex])
                        {
                                // Indexed Triangle strips
                                glDrawElements(GL_TRIANGLE_STRIP, pMesh->pnStripLength[i]+2, type, (void*)(long)(offset * pMesh->sFaces.nStride));
                        }
                        else
                        {
                                // Non-Indexed Triangle strips
                                glDrawArrays(GL_TRIANGLE_STRIP, offset, pMesh->pnStripLength[i]+2);
                        }
                        offset += pMesh->pnStripLength[i]+2;
                }
        }

        // Safely disable the vertex attribute arrays
        glDisableVertexAttribArray(VERTEX_ARRAY);
        glDisableVertexAttribArray(NORMAL_ARRAY);
		glDisableVertexAttribArray(TEXCOORD_ARRAY);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

bool RenderScene(car_data_t *cd, int viewport_id, float upon, float angle,float view_dist)
{
        //Clear the color and depth buffer
        //glClear(GL_DEPTH_BUFFER_BIT);
        //glDepthFunc(GL_LEQUAL);
        //glEnable(GL_DEPTH_TEST);
        // Use shader program
        glUseProgram(car_program.uiId);//规定所使用的shader

        /*
                Get the direction of the first light from the scene.
        */
        
        //计算进入shader的模型矩阵
        glm::mat4 car_model = glm::mat4(1.0f);                                                     //重置单位矩阵
	    car_model = glm::translate(car_model, glm::vec3(0.0f,0.0f,0.0f));                          //对模型进行平移
		car_model = glm::scale(car_model,glm::vec3(0.1f, 0.1f, 0.1f));                             //缩放矩阵
		car_model = glm::rotate(car_model, degreesToRadians(cd->xrot_degrees), glm::vec3(1.0f, 0.0f, 0.0f));  //旋转矩阵,模型绕X轴旋转
		car_model = glm::rotate(car_model, degreesToRadians(cd->yrot_degrees), glm::vec3(0.0f, 1.0f, 0.0f));  //旋转矩阵,模型绕Y轴旋转
		car_model = glm::rotate(car_model, degreesToRadians(cd->zrot_degrees), glm::vec3(0.0f, 0.0f, 1.0f));  //旋转矩阵,模型绕Z轴旋转
        //计算进入shader的观察矩阵
		glm::mat4 car_view = glm::lookAt(glm::vec3(view_dist*cos(degreesToRadians(upon))*sin(degreesToRadians(angle)),
		                                           -view_dist*cos(degreesToRadians(upon))*cos(degreesToRadians(angle)),
												   view_dist*sin(degreesToRadians(upon))),
						                           glm::vec3(0.0f, 0.0f, 0.0f), 
												   glm::vec3(0.0f, 0.0f, 1.0f));
        
		//计算进入shader的透视矩阵
		glm::mat4 car_projection = glm::perspective(45.0f, 1.0f, 0.1f, 1000.0f);

		//计算进入shader的MVP矩阵
        glm::mat4 mvp = car_projection * car_view * car_model ; 

        for (unsigned int i = 0; i < cd->scene->nNumMeshNode; ++i)
        {
			SPODNode& Node = cd->scene->pNode[i];
			
			//传递变换矩阵到shader中
			glUniformMatrix4fv(car_program.uiMVPMatrixLoc, 1, GL_FALSE, &mvp[0][0]);
			
			//printf("Log:%s\n",glm::to_string(mMVP_car[1]).c_str());//打印输出当前的模型观测矩阵的值
			
			GLuint uiTex = 0;
			
			if(Node.nIdxMaterial != -1)
			uiTex = cd->texture_ids[Node.nIdxMaterial];
			
			glActiveTexture(GL_TEXTURE5);
			glBindTexture(GL_TEXTURE_2D, uiTex);

		    //绘制网格面
		    DrawMesh(cd, i);
        }
        //glDisable(GL_DEPTH_TEST);
        return true;
}

int car_init()
{
	bool flag =InitView();
    printf("Init view status is %d\n",flag);
	return 0;
}

int car_deinit()
{
	ReleaseView();
	return 0;
}

void car_draw(int viewport_id,float upon,float angle,float view_dist)
{
    RenderScene(&car_data[active_car_index], viewport_id,upon,angle,view_dist);//新增加带参数，upon和angle
}

void car_updateView(int i)
{
	glm::mat4 mView_car;
	mView_car = glm::scale(mView[i], glm::vec3(car_data[active_car_index].scale));
	mView_car = glm::rotate(mView_car, degreesToRadians(car_data[active_car_index].xrot_degrees), glm::vec3(1.0, 0.0, 0.0));
	mView_car = glm::rotate(mView_car, degreesToRadians(car_data[active_car_index].yrot_degrees), glm::vec3(0.0, 1.0, 0.0));
	mView_car = glm::rotate(mView_car, degreesToRadians(car_data[active_car_index].zrot_degrees), glm::vec3(0.0, 0.0, 1.0));
	mMVP_car[i] = mProjection[i] * mView_car;
}

void car_change()
{
	do
	{
		active_car_index = (active_car_index + 1) % num_car_models;
	}
	while (car_data[active_car_index].scene == NULL);
	for (int i = 0; i < num_viewports; i++)
		car_updateView(i);
}

void car_worldToNdc(float *xndc, float *yndc, GLfloat x, GLfloat y, GLfloat z)
{
    glm::vec4 vTransformedVector = mMVP_car[0] * glm::vec4(x, y, z, 1);
    *xndc = vTransformedVector.x/vTransformedVector.z;
    *yndc = vTransformedVector.y/vTransformedVector.z;
}


void render_ndcToScreen(int *xscr, int *yscr, float xndc, float yndc)
{
	*xscr = (int)round(srv_viewports[0].x + (srv_viewports[0].width * (xndc + 1.0)/2));
	*yscr = (int)round(srv_viewports[0].y + (srv_viewports[0].height * (1.0 - yndc)/2));
}

void car_worldToScreen(int *xscr, int *yscr, GLfloat x, GLfloat y, GLfloat z)
{
        float xndc, yndc;
        car_worldToNdc(&xndc, &yndc, x, y, z);
        render_ndcToScreen(xscr, yscr, xndc, yndc);
}

void car_getScreenLimits(int *xmin, int *xmax, int *ymin, int *ymax)
{
	int x, y;
	car_data_t *cd = &car_data[active_car_index];
	*xmin = INT_MAX;
	*xmax = -INT_MAX;
	*ymin = INT_MAX;
	*ymax = -INT_MAX;

        for (unsigned int i = 0; i < cd->scene->nNumMesh; ++i)
        {
                // Load vertex data into buffer object
		SPODMesh& Mesh = cd->scene->pMesh[i];
		for (unsigned int j = 0; j < Mesh.nNumVertex; j++)
		{
			vertex3df *v = (vertex3df *)((uint8_t *)(Mesh.pInterleaved) + (Mesh.sVertex.nStride * j));
			car_worldToScreen(&x, &y, v->x, v->y, v->z);
			if(x < *xmin) *xmin = x;
			if(x > *xmax) *xmax = x;
			if(y < *ymin) *ymin = y;
			if(y > *ymax) *ymax = y;
		}
	}
}