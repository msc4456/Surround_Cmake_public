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


#define SRV_BLEND_LUT_BIN_FILE "./srv_blend_lut.bin"
#define SRV_LUT_BIN_FILE "./srv_lut.bin"

#define SRV_INDEX_BIN_FILE "./srv_indices.bin"

GLuint vertexPositionAttribLoc; //3 panes+1 car
GLuint vertexTexCoord1AttribLoc; //4 panes+1 car
GLuint vertexTexCoord2AttribLoc; //4 panes+1 car
GLuint vertexIndexImage1AttribLoc; //4 panes+1 car
GLuint vertexIndexImage2AttribLoc; //4 panes+1 car
GLuint blendAttribLoc; //blend of LUT
GLuint uiProgramObject;

GLint samplerLocation0;
GLint samplerLocation1;
GLint samplerLocation2;
GLint samplerLocation3;
GLint projMatrixLocation;
GLint mvMatrixLocation;
GLint uniform_select;
#ifndef STATIC_COORDINATES
GLint uniform_rangex;
GLint uniform_rangey;
GLint uniform_texturex;
GLint uniform_texturey;
#endif

int shader_output_select = 0;
GLenum render_mode = GL_TRIANGLES;

bool srv_render_to_file =false;

extern glm::mat4 mMVP_bowl;


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

//Shaders for surround view
static const char srv_vert_shader[] =
" #version 150 core\n"//指定GLSL版本，TDA2X不支持3.0及以上的GLSL
" attribute vec3 aVertexPosition;\n "
" attribute vec2 aTextureCoord1;\n "
" attribute vec2 aTextureCoord2;\n "
" attribute vec2 blendVals;\n "
" uniform mat4 uMVMatrix;\n "
" varying vec2 outNormTexture;\n "
" varying vec2 outNormTexture1;\n "
" varying vec2 outBlendVals;\n "
" uniform float uRangeX;\n "
" uniform float uRangeY;\n "
" uniform float uTextureX;\n "
" uniform float uTextureY;\n "

" void main(void) {\n "
"     gl_Position = uMVMatrix * vec4(aVertexPosition.x, aVertexPosition.y, aVertexPosition.z, 1.0);\n "
"     outNormTexture.x = aTextureCoord1.t/uTextureX;\n"
"     outNormTexture.y = aTextureCoord1.s/uTextureY;\n"
"     outNormTexture1.x = aTextureCoord2.t/uTextureX;\n"
"     outNormTexture1.y = aTextureCoord2.s/uTextureY;\n"
"     outBlendVals = blendVals;\n"
" }\n"
;

static const char srv_frag_shader[] =
" #version 150 core\n"   //指定GLSL版本，TDA2X不支持3.0及以上的GLSL
" precision mediump float;\n"
" uniform sampler2D uSampler[2];\n"
" uniform int select;\n"
" varying vec2 outNormTexture;\n"
" varying vec2 outNormTexture1;\n"
" varying vec2 outBlendVals;\n"
" vec4 iFragColor1;\n"
" vec4 iFragColor2;\n"
" void main(){\n"
"       iFragColor1  = texture2D(uSampler[0], outNormTexture);\n "
"       iFragColor2  = texture2D(uSampler[1], outNormTexture1);\n "
"       gl_FragColor = (outBlendVals.x)*iFragColor1 + (outBlendVals.y)*iFragColor2;\n "
" }\n"
;


GLuint render_loadShader(GLenum shaderType, const char* pSource) {
   GLuint shader = glCreateShader(shaderType);
   if (shader) {
       glShaderSource(shader, 1, &pSource, NULL);
       glCompileShader(shader);
       GLint compiled = 0;
       glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
       if (!compiled) {
           GLint infoLen = 0;
           glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
           if (infoLen) {
               char* buf = (char*) malloc(infoLen);
               if (buf) {
                   glGetShaderInfoLog(shader, infoLen, NULL, buf);
                   printf(" GL: Could not compile shader %d:\n%s\n",
                       shaderType, buf);
                   free(buf);
               }
           } else {
               printf(" GL: Guessing at GL_INFO_LOG_LENGTH size\n");
               char* buf = (char*) malloc(0x1000);
               if (buf) {
                   glGetShaderInfoLog(shader, 0x1000, NULL, buf);
                   printf(" GL: Could not compile shader %d:\n%s\n",
                   shaderType, buf);
                   free(buf);
               }
           }
           glDeleteShader(shader);
           shader = 0;
       }
   }
   return shader;
}

GLuint render_createProgram(const char* pVertexSource, const char* pFragmentSource) {
   GLuint vertexShader = render_loadShader(GL_VERTEX_SHADER, pVertexSource);
   if (!vertexShader) {
       return 0;
   }

   GLuint pixelShader = render_loadShader(GL_FRAGMENT_SHADER, pFragmentSource);
   if (!pixelShader) {
       return 0;
   }

   GLuint program = glCreateProgram();
   if (program) {
       glAttachShader(program, vertexShader);
       //System_eglCheckGlError("glAttachShader");
       glAttachShader(program, pixelShader);
       //System_eglCheckGlError("glAttachShader");
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
                   printf(" GL: Could not link program:\n%s\n", buf);
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

int srv_import_blend_lut(uint32_t blend_lut_size,
		srv_blend_lut_t *blend_lut)
{

    FILE *f_static_blend_lut;
    size_t bytes_read;

    //read static load_file
    f_static_blend_lut = fopen(SRV_BLEND_LUT_BIN_FILE,"rb");
    if(!f_static_blend_lut)
    {
        printf("open blend lut file failed!!!\n");
        return -1;
    }
    else
    {
        printf("open blend lut file successful !!!\n");
    }

    bytes_read = fread(blend_lut, sizeof(srv_blend_lut_t), blend_lut_size, f_static_blend_lut);
    if(bytes_read<=0)
    {
        printf("import blend lut failed !!!\n");
        return -1;
    }
    else
    {
        for(int i =0;i<=blend_lut_size/100;i++)
        {
        printf("import blend lut data %d is %d %d\n",i,blend_lut[i].blend1,blend_lut[i].blend2); 
        }
        fclose(f_static_blend_lut);
        return 1;     
    }

}

int srv_import_lut(uint32_t lut_size, srv_lut_t *Vertex_lut)
{
    FILE *f_static_lut_file;
    size_t bytes_read;

    //read static load_file
    f_static_lut_file = fopen(SRV_LUT_BIN_FILE,"rb");

    if(!f_static_lut_file)
    {
        printf("opening lut file read failed!!!\n");
        return -1;
    }
    else
    {
        printf("opening lut file read successfull!!!\n");
    }

    bytes_read = fread(Vertex_lut,sizeof(_srv_lut_t),lut_size,f_static_lut_file);

    if(bytes_read<=0)
    {
        printf("read static lut failed !!!\n");
        return -1;
    }

    else
    {
        for(int i =0;i<lut_size/100;i++)
        {
        printf("static data %d is %d %d %d %d %d %d %d",i,Vertex_lut[i].x,Vertex_lut[i].y,Vertex_lut[i].z,Vertex_lut[i].u1,Vertex_lut[i].v1,Vertex_lut[i].u2,Vertex_lut[i].v2);
        printf("\n");
        }
        fclose(f_static_lut_file);
        return 1;   
    }
}

int srv_indices_import(size_t indice_length,t_index_buffer* index_buffer)
{
    FILE *f_static_indices;
    size_t bytes_read;

	index_buffer->length =indice_length;


    //read static load_file
    f_static_indices = fopen(SRV_INDEX_BIN_FILE,"rb");
    if(!f_static_indices)
    {
        printf("opening index file failed!!!\n");
        return -1;
    }
    else
    {
        printf("opening index file successful !!!\n");
    }
    bytes_read = fread(index_buffer->buffer, sizeof(uint32_t), indice_length, f_static_indices);

    if(bytes_read<=0)
    {
        printf("read static lut failed !!!\n");
        return -1;
    }
    else
    {
        for(int i =0;i<100;i=i+3)
        {
        printf("Triganle data %d is %d %d %d \n",i,index_buffer->buffer[i],index_buffer->buffer[i+1],index_buffer->buffer[i+2]); 
        }
        return 1;
        fclose(f_static_indices);
    }
}


void generate_indices(t_index_buffer *index_buffer, unsigned int xlength, unsigned int ylength, unsigned int gap)
{
	unsigned int *buffer = index_buffer->buffer;
	unsigned int x, y, k=0;
	for (y=0; y<ylength-gap; y+=gap)
	{
		if(y>0)
			buffer[k++]=(unsigned int) (y*xlength);
		for (x=0; x<xlength; x+=gap)
		{
			buffer[k++]=(unsigned int) (y*xlength + x);
			buffer[k++]=(unsigned int) ((y+gap)*xlength + x);
		}
		if(y < ylength - 1 - gap)
			buffer[k++]=(unsigned int) ((y+gap)*xlength + (xlength -1));
	}
	index_buffer->length = k;
}

int srv_setup(render_state_t *pObj)
{

    printf("srv_setp runing\n");
	if(pObj->LUT3D == NULL)
	{
		// Generate the LUT if it wasn't passed
		/*pObj->LUT3D = malloc(sizeof(int16_t) * (POINTS_WIDTH/POINTS_SUBX) * (POINTS_HEIGHT/POINTS_SUBY) * 9 * 2);
		srv_generate_lut(POINTS_WIDTH, POINTS_HEIGHT,
				pObj->cam_width,
				pObj->cam_height,
				POINTS_SUBX,
				POINTS_SUBY,
				(srv_lut_t *)pObj->LUT3D); */

		pObj->LUT3D = malloc(sizeof(srv_lut_t) * (POINTS_WIDTH/POINTS_SUBX/2+1) * (POINTS_HEIGHT/POINTS_SUBY/2+1)*4);//4 个quad 总顶 点数
        srv_import_lut(4*(POINTS_WIDTH/POINTS_SUBX/2+1) * (POINTS_HEIGHT/POINTS_SUBY/2+1), (srv_lut_t *)pObj->LUT3D);
	}

    srv_lut_t*data;
    data = (srv_lut_t*)pObj->LUT3D;

    for(int i =0;i<100;i++)
    {
        printf("3D_lut vertex & texture %d is: %d,%d,%d,%d,%d,%d,%d\n",i,data[i].x,data[i].y,data[i].z,data[i].u1,data[i].v1,data[i].u2,data[i].v2);
    }

#ifndef SRV_INTERLEAVED_BLEND
	if(pObj->blendLUT3D == NULL)
	{
		// Generate the BlendLUT if it wasn't passed
		/*pObj->blendLUT3D = malloc(sizeof(int16_t) * (POINTS_WIDTH/POINTS_SUBX) * (POINTS_HEIGHT/POINTS_SUBY) * 9 * 2);
		srv_generate_blend_lut(POINTS_WIDTH, POINTS_HEIGHT,
				POINTS_SUBX,
				POINTS_SUBY,
				(srv_blend_lut_t *)pObj->blendLUT3D); */

        pObj->blendLUT3D = malloc(sizeof(srv_blend_lut_t) * (POINTS_WIDTH/POINTS_SUBX/2+1) * (POINTS_HEIGHT/POINTS_SUBY/2+1)*4);
        srv_import_blend_lut(4*(POINTS_WIDTH/POINTS_SUBX/2+1) * (POINTS_HEIGHT/POINTS_SUBY/2+1),(srv_blend_lut_t *)pObj->blendLUT3D);
	}


    srv_blend_lut_t*blend_data;
    blend_data = (srv_blend_lut_t*)pObj->blendLUT3D;
   
    for(int i =0;i<100;i++)
    {
    	printf("blend_lut %d is: %d,%d \n",i,blend_data[i].blend1,blend_data[i].blend2);
    }

#endif
	if(srv_render_to_file == true)
	{
     /*   */
	}

	else
	{
#if 1
		uiProgramObject = renderutils_createProgram(
				srv_vert_shader,
				srv_frag_shader
				);
#else
		uiProgramObject = renderutils_loadAndCreateProgram("srv_vert.vsh", "srv_frag.fsh");
#endif
	}
	if (uiProgramObject==0)
	{
		return -1;
	}

	glUseProgram(uiProgramObject);
	//System_eglCheckGlError("glUseProgram");

	//locate sampler uniforms
	uniform_select = glGetUniformLocation(uiProgramObject, "select");
	GL_CHECK(glGetAttribLocation);
	samplerLocation0 = glGetUniformLocation(uiProgramObject, "uSampler[0]");
	glUniform1i(samplerLocation0, 0);
	GL_CHECK(glUniform1i);
	samplerLocation1 = glGetUniformLocation(uiProgramObject, "uSampler[1]");
	glUniform1i(samplerLocation1, 1);
	GL_CHECK(glUniform1i);
	mvMatrixLocation = glGetUniformLocation(uiProgramObject, "uMVMatrix");
	GL_CHECK(glGetAttribLocation);

#ifndef STATIC_COORDINATES
	uniform_rangex = glGetUniformLocation(uiProgramObject, "uRangeX");
	GL_CHECK(glGetUniformLocation);
	uniform_rangey = glGetUniformLocation(uiProgramObject, "uRangeY");
	GL_CHECK(glGetUniformLocation);
	uniform_texturex = glGetUniformLocation(uiProgramObject, "uTextureX");
	GL_CHECK(glGetUniformLocation);
	uniform_texturey= glGetUniformLocation(uiProgramObject, "uTextureY");
	GL_CHECK(glGetUniformLocation);
	glUniform1f(uniform_rangex, (GLfloat)float(POINTS_WIDTH/2));
	GL_CHECK(glUniform1f);
	glUniform1f(uniform_rangey, (GLfloat)float(POINTS_HEIGHT/2));
	GL_CHECK(glUniform1f);
	glUniform1f(uniform_texturex, (GLfloat)float(pObj->cam_width * 16));
	GL_CHECK(glUniform1f);
	glUniform1f(uniform_texturey, (GLfloat)float(pObj->cam_height * 16));
	GL_CHECK(glUniform1f);
#endif

	vertexPositionAttribLoc = glGetAttribLocation(uiProgramObject, "aVertexPosition");
	GL_CHECK(glGetAttribLocation);
	blendAttribLoc = glGetAttribLocation(uiProgramObject, "blendVals");
	GL_CHECK(glGetAttribLocation);
	vertexTexCoord1AttribLoc = glGetAttribLocation(uiProgramObject, "aTextureCoord1");
	GL_CHECK(glGetAttribLocation);
	vertexTexCoord2AttribLoc = glGetAttribLocation(uiProgramObject, "aTextureCoord2");
	GL_CHECK(glGetAttribLocation);

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	//System_eglCheckGlError("glClearColor");
	glClear( GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	//System_eglCheckGlError("glClear");
	glDisable(GL_DEPTH_TEST);
#ifdef ENABLE_GLOBAL_BLENDING
	glEnable(GL_BLEND);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#endif

	//Generate indices
	for (int i = 0; i < MAX_INDEX_BUFFERS; i++)
	{
		/*index_buffers[i].buffer = (unsigned int *)malloc(QUADRANT_WIDTH * QUADRANT_HEIGHT * 3 * sizeof(unsigned int));
		generate_indices((t_index_buffer *)&index_buffers[i], QUADRANT_WIDTH, QUADRANT_HEIGHT, pow(2,i));*/
        index_buffers[i].buffer = (unsigned int *)malloc((QUADRANT_WIDTH-1) * (QUADRANT_HEIGHT-1) * 2* 3 * sizeof(unsigned int));   
        srv_indices_import((QUADRANT_WIDTH-1) * (QUADRANT_HEIGHT-1)*2*3,&index_buffers[i]);

	}
    t_index_buffer * index_data;
	index_data = &index_buffers[1];
	for(int i =0;i<100;i=i+3)
	{
      printf("get indices %d is %d %d %d\n",i,index_data->buffer[i],index_data->buffer[i+1],index_data->buffer[i+2]);
	}
	// Generate named buffers for indices and vertices
	glGenBuffers(QUADRANTS*3, vboId);

	return 0;
}

//Vertices init for surround view (VBO approach)
static int surroundview_init_vertices_vbo(render_state_t *pObj, GLuint vertexId, GLuint indexId, GLuint blendId,
		void* vertexBuff, void* indexBuff, void * blendBuff,
		int vertexBuffSize, int indexBuffSize, int blendBuffSize
		)
{
	//upload the vertex and texture and image index interleaved array
	//Bowl LUT - Interleaved data (5 data)
	glBindBuffer(GL_ARRAY_BUFFER, vertexId);

	glBufferData(GL_ARRAY_BUFFER, vertexBuffSize, vertexBuff, GL_STATIC_DRAW);
	glVertexAttribPointer(vertexPositionAttribLoc, 3, GL_VERTEX_ENUM, GL_FALSE, sizeof(srv_lut_t), 0);
	glVertexAttribPointer(vertexTexCoord1AttribLoc, 2, GL_TEXCOORD_ENUM, GL_FALSE, sizeof(srv_lut_t), (GLvoid*)(offsetof(srv_lut_t, u1)));
	glVertexAttribPointer(vertexTexCoord2AttribLoc, 2, GL_TEXCOORD_ENUM, GL_FALSE, sizeof(srv_lut_t), (GLvoid*)(offsetof(srv_lut_t, u2)));
#ifdef SRV_INTERLEAVED_BLEND
	glVertexAttribPointer(blendAttribLoc, 2, GL_BLEND_ENUM, GL_TRUE, sizeof(srv_lut_t), (GLvoid*)(offsetof(srv_lut_t, blend1)));
#endif
	GL_CHECK(glVertexAttribPointer);

#ifndef SRV_INTERLEAVED_BLEND
	glBindBuffer(GL_ARRAY_BUFFER, blendId);
	glBufferData(GL_ARRAY_BUFFER, blendBuffSize, blendBuff, GL_STATIC_DRAW);
	glVertexAttribPointer(blendAttribLoc, 2, GL_BLEND_ENUM, GL_TRUE, sizeof(_srv_blend_lut_t), 0);
	GL_CHECK(glVertexAttribPointer);
#endif

	//Index buffer
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexId);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexBuffSize, indexBuff, GL_STATIC_DRAW);
	GL_CHECK(glBufferData);

	//Enable for the rendering
	glEnableVertexAttribArray(vertexPositionAttribLoc);
	glEnableVertexAttribArray(vertexTexCoord1AttribLoc);
	glEnableVertexAttribArray(vertexTexCoord2AttribLoc);
	glEnableVertexAttribArray(blendAttribLoc);

	return 0;
}

void surroundview_init_vertices_vbo_wrap(render_state_t *pObj)
{
	int i;
	int vertexoffset = 0;
	int blendoffset = 0;

	for(i = 0;i < QUADRANTS;i ++)
	{
		vertexoffset = i * (sizeof(srv_lut_t)*QUADRANT_WIDTH*QUADRANT_HEIGHT);
		blendoffset = i * (sizeof(srv_blend_lut_t)*QUADRANT_WIDTH*QUADRANT_HEIGHT);

		surroundview_init_vertices_vbo(
				pObj,
				vboId[i*3], vboId[i*3+1], vboId[i*3+2],
				(char*)pObj->LUT3D + vertexoffset,
				(char*)(index_buffers[active_index_buffer].buffer),
				(char*)pObj->blendLUT3D + blendoffset,
				sizeof(srv_lut_t)*QUADRANT_WIDTH*QUADRANT_HEIGHT,
				sizeof(int)*(index_buffers[active_index_buffer].length),
				sizeof(srv_blend_lut_t)*QUADRANT_WIDTH*QUADRANT_HEIGHT
				);
	}
	index_buffer_changed = false;
}

void onscreen_mesh_state_restore_program_textures_attribs(render_state_t *pObj, GLuint *texYuv, int tex1, int tex2, int viewport_id)
{
	//set the program we need
	glUseProgram(uiProgramObject);

	glUniform1i(samplerLocation0, 0);
	glActiveTexture(GL_TEXTURE0);

#ifndef STANDALONE
	glBindTexture(GL_TEXTURE_2D, texYuv[tex1]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
#else
	glBindTexture(GL_TEXTURE_2D, texYuv[tex1]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
#endif
	GL_CHECK(glBindTexture);

	glUniform1i(samplerLocation1, 1);
	glActiveTexture(GL_TEXTURE1);

#ifndef STANDALONE
	glBindTexture(GL_TEXTURE_2D, texYuv[tex2]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
#else
	glBindTexture(GL_TEXTURE_2D, texYuv[tex2]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
#endif
	GL_CHECK(glBindTexture);

	//Enable the attributes
	glEnableVertexAttribArray(vertexPositionAttribLoc);
	glEnableVertexAttribArray(vertexTexCoord1AttribLoc);
	glEnableVertexAttribArray(vertexTexCoord2AttribLoc);
	glEnableVertexAttribArray(blendAttribLoc);

	glUniformMatrix4fv(mvMatrixLocation, 1, GL_FALSE, &mMVP_bowl[0][0]);
	GL_CHECK(glUniformMatrix4fv);

	glUniform1i(uniform_select, shader_output_select);

}

void onscreen_mesh_state_restore_vbo(render_state_t *pObj,
		GLuint vertexId, GLuint indexId, GLuint blendId)
{

	//restore the vertices and indices
	glBindBuffer(GL_ARRAY_BUFFER, vertexId);
	glVertexAttribPointer(vertexPositionAttribLoc, 3, GL_VERTEX_ENUM, GL_FALSE, sizeof(srv_lut_t), 0);
	glVertexAttribPointer(vertexTexCoord1AttribLoc, 2, GL_TEXCOORD_ENUM, GL_FALSE, sizeof(srv_lut_t), (GLvoid*)(offsetof(srv_lut_t, u1)));
	glVertexAttribPointer(vertexTexCoord2AttribLoc, 2, GL_TEXCOORD_ENUM, GL_FALSE, sizeof(srv_lut_t), (GLvoid*)(offsetof(srv_lut_t, u2)));
#ifdef SRV_INTERLEAVED_BLEND
	glVertexAttribPointer(blendAttribLoc, 2, GL_BLEND_ENUM, GL_TRUE, sizeof(srv_lut_t), (GLvoid*)(offsetof(srv_lut_t, blend1)));
#else
	glBindBuffer(GL_ARRAY_BUFFER, blendId);
	glVertexAttribPointer(blendAttribLoc, 2, GL_BLEND_ENUM, GL_TRUE, sizeof(_srv_blend_lut_t), 0);
#endif
	//Index buffer
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexId);
}

void srv_draw(render_state_t *pObj, GLuint *texYuv, int viewport_id)
{
	int i;
	if(prevLUT != pObj->LUT3D || index_buffer_changed == true)
	{
		prevLUT = pObj->LUT3D;
		surroundview_init_vertices_vbo_wrap(pObj);
	}

	//First setup the program once
	glUseProgram(uiProgramObject);
	//then change the meshes and draw
	for(i = 0;i < QUADRANTS;i ++)
	{
		onscreen_mesh_state_restore_program_textures_attribs(
				pObj, texYuv, (0+i)%4, (3+i)%4, viewport_id);
		onscreen_mesh_state_restore_vbo(
				pObj, vboId[i*3], vboId[i*3+1], vboId[i*3+2]);
		GL_CHECK(onscreen_mesh_state_restore_vbo);
		glDrawElements(render_mode, index_buffers[active_index_buffer].length, GL_UNSIGNED_INT,  0);
		GL_CHECK(glDrawElements);
	}
	glFlush();
}