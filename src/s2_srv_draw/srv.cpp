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

int shader_output_select = 0;

GLenum render_mode = GL_TRIANGLE_STRIP;//渲染模式选择三角形渲染

extern bool srv_render_to_file;

#define STAND_ALONG

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
"#version 330 core\n"
//从CPU传入GPU的，分配过的属性表
"layout (location = 0) in vec3 aPos;\n"        //属性0：顶点XYZ坐标
//备注：曲面上的任意一个顶点在四个通道的纹理图像中均有坐标，当该顶点不被某通道的摄像头所观测到时，在该通道中的纹理坐标为0，0
"layout (location = 1) in vec2 aTexCoord1;\n"  //属性1：顶点在前通道中的纹理坐标
"layout (location = 2) in vec2 aTexCoord2;\n"  //属性2：顶点在左通道中的纹理坐标
"layout (location = 3) in vec2 aTexCoord3;\n"  //属性3：顶点在后通道中的纹理坐标
"layout (location = 4) in vec2 aTexCoord4;\n"  //属性4：顶点在右通道中的纹理坐标

"layout (location = 5) in float aTexWeight1;\n"//属性5：顶点在前通道中的纹理权重
"layout (location = 6) in float aTexWeight2;\n"//属性6：顶点在左通道中的纹理权重
"layout (location = 7) in float aTexWeight3;\n"//属性7：顶点在后通道中的纹理权重
"layout (location = 8) in float aTexWeight4;\n"//属性8：顶点在右通道中的纹理权重
//----------------------------------------------------------------------------------
//可直接通过layout语句传递给片段着色器，不必要一定通过顶点着色器中转
"out vec2 oTexCoord1;\n"    //前通道纹理坐标输出
"out vec2 oTexCoord2;\n"    //左通道纹理坐标输出
"out vec2 oTexCoord3;\n"    //后通道纹理坐标输出
"out vec2 oTexCoord4;\n"    //右通道纹理坐标输出

"out float oTexWeight1;\n"  //前通道纹理权重输出
"out float oTexWeight2;\n"  //左通道纹理权重输出
"out float oTexWeight3;\n"  //后通道纹理权重输出
"out float oTexWeight4;\n"  //右通道纹理权重输出

"uniform mat4 srv_model;\n"    //模型矩阵
"uniform mat4 srv_view;\n"      //观察矩阵
"uniform mat4 srv_projection;\n"//透视矩阵

"uniform float gain1;\n"    //前通道增益
"uniform float gain2;\n"    //左通道增益
"uniform float gain3;\n"   //后通道增益
"uniform float gain4;\n"    //右通道增益

//-----------------------------------------------------------------------------------
"void main()\n"
"{\n"
//MVP复合矩阵确定顶点在屏幕中的绘制位置,全部为齐次运算
"gl_Position = srv_projection * srv_view * srv_model * vec4(aPos, 1.0f);\n"//MVP matrix multied in GPU slice
//gl_Position = vec4(aPos, 1.0f);
//通过顶点着色器透传纹理坐标
"oTexCoord1 = aTexCoord1;\n"
"oTexCoord2 = aTexCoord2;\n"
"oTexCoord3 = aTexCoord3;\n"
"oTexCoord4 = aTexCoord4;\n"
    
//通过顶点着色器，及内置乘法器传递
"oTexWeight1 = aTexWeight1*gain1;\n"
"oTexWeight2 = aTexWeight2*gain2;\n"
"oTexWeight3 = aTexWeight3*gain3;\n"
"oTexWeight4 = aTexWeight4*gain4;\n"
"}\n";

static const char srv_frag_shader[] =
"#version 330 core\n"
"#version 330 core\n"
"out vec4 FragColor;\n"

"in vec2 oTexCoord1;\n"   //前通道纹理坐标输入
"in vec2 oTexCoord2;\n"   //左通道纹理坐标输入
"in vec2 oTexCoord3;\n"   //后通道纹理坐标输入
"in vec2 oTexCoord4;\n"   //右通道纹理坐标输入
"in float oTexWeight1;\n" //前通道纹理坐标权重输入
"in float oTexWeight2;\n" //左通道纹理坐标权重输入
"in float oTexWeight3;\n" //后通道纹理坐标权重输入
"in float oTexWeight4;\n" //右通道纹理坐标权重输入

// 纹理采样，光栅化之后
"uniform sampler2D texture1;\n"//前2D纹理采样
"uniform sampler2D texture2;\n"//左2D纹理采样
"uniform sampler2D texture3;\n"//后2D纹理采样
"uniform sampler2D texture4;\n"//右2D纹理采样

"void main()\n"
"{\n"
	// linearly interpolate between both textures (80% container, 20% awesomeface)
	// FragColor = mix(texture(texture1, TexCoord), texture(texture2, TexCoord), 0.2);

"	FragColor =  texture2D(texture1,oTexCoord1)*oTexWeight1\n"
"				+texture2D(texture2,oTexCoord2)*oTexWeight2\n"
"				+texture2D(texture3,oTexCoord3)*oTexWeight3\n"
"				+texture2D(texture4,oTexCoord4)*oTexWeight4;\n"
	// FragColor = texture(texture1,oTexCoord1)*0.25+texture(texture2,oTexCoord2)*0.25+texture(texture3,oTexCoord3)*0.25+texture(texture4,oTexCoord4)*0.25;
	// FragColor = texture(texture1,oTexCoord1);
	// FragColor = texture(texture2,oTexCoord2);
	// FragColor = texture(texture3,oTexCoord3);
	// FragColor = texture(texture4,oTexCoord1);

	// FragColor = texture(texture1,oTexCoord1)*0.5+ texture(texture2,oTexCoord2)*0.5;
	// FragColor = vec4(1.0,0.0,0.0,0.0);
"}\n";


//初始化纹理信息，当程序运行在单击模式下，纹理静态导入一次
GLuint* standalone_init_texture(const char*base_path,const char*filetype)
{
    char filepath[50];
    const char*filename[4];
    static GLuint texYuv[4] ={0};//注意静态变量类型

    filename[0]="front0";
    filename[1]="left0";
    filename[2]="rear0";
    filename[3]="right0";

    for(char i=0;i<4;i++)
    {
    sprintf(filepath,"%s/%s.%s",base_path,filename[i],filetype);//必须是bmp文件
   	glGenTextures(1, &texYuv[i]);
	glBindTexture(GL_TEXTURE_2D, texYuv[i]);
	load_texture_bmp(texYuv[0], filepath);
    }
    return texYuv;
}

int get_surface(surface_data_t*pObj,const char*v_filename,const char*f_filename)
{
    pObj->vertex_num  =6702;
    pObj->indices_num =12810;
    //pObj->vertex_arry = (float*)malloc(pObj->vertex_num*15);
    //pObj->indices_arry =(float*)malloc(pObj->indices_num*3);

    //FILE *fp = fopen("vertex.bin","rb");                                        //导入顶点
    FILE *fp =fopen(v_filename,"rb");
    unsigned int flag =fread(pObj->vertex_arry,pObj->vertex_num*15,1,fp);     //读入顶点数据表,每次读sizeof(vertices)个，读一次
    //printf("%d\n",surface.vertex_num*3);
    fclose(fp);
    //fp = fopen("surface.bin","rb");                                             //导入曲面
    fp =fopen(f_filename,"rb");
    flag =fread(pObj->indices_arry,pObj->indices_num*3,1,fp);
    fclose(fp);
    return 1;//返回结构体
}

void get_vedio_stream()
{

}
int srv_init(surface_data_t *pObj)//初始化纹理数据
{
    static surface_data_t surfaceobj;
    //--------------------------------------获取数据-----------------------------------------------------//
    get_surface(pObj,"../../src/shader/vertex.bin","../../src/shader/surface.bin");
    //-------------------------------------创建着色器----------------------------------------------------//
    srv_program.srv_Uid == renderutils_createProgram(
				srv_vert_shader,
				srv_frag_shader
				);

	if (srv_program.srv_Uid ==0)//如果着色器创建不成功，返回-1
	{
        printf("srv_program shader created failed");
	}
    #ifdef STAND_ALONG
    standalone_init_texture("../../ext/resource/image/","bmp");
    #endif
    return 1;  
}

void srv_update_view(GLfloat view_dist,GLfloat upon,GLfloat angle,GLfloat ratio)
{
    glm::mat4 srf_model = glm::mat4(1.0f);//重置模型矩阵,不做任何旋转操作
        //srf_model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f,0.0f,0.0f));//
        //srf_model = glm::rotate(srf_model, angle, glm::vec3(0.0f, 0.0f, 1.0f));//

    //设置观察矩阵
    glm::mat4 srf_view = glm::lookAt(glm::vec3(
                                            view_dist*cos(degreesToRadians(upon))*sin(degreesToRadians(angle)), 
                                            -view_dist*cos(degreesToRadians(upon))*cos(degreesToRadians(angle)), 
                                            view_dist*sin(degreesToRadians(upon))),
                                     glm::vec3(0.0f, 0.0f, 0.0f), 
                                     glm::vec3(0.0f, 0.0f, 1.0f)
                                     );
    //设置透视矩阵
    glm::mat4 srf_projection = glm::perspective(45.0f, ratio, 0.1f, 1000.0f);

    //传参到着色器,传入shader后在进行乘法
    glUniformMatrix4fv(glGetUniformLocation(srv_program.srv_Uid, "srv_view"), 1, GL_FALSE, &srf_view[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(srv_program.srv_Uid, "srv_projection"), 1, GL_FALSE, &srf_model[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(srv_program.srv_Uid, "srv_model"), 1, GL_FALSE, &srf_projection[0][0]);

    //srv_program.srv_Uid.setMat4("view",srf_view);
    //srvShader.setMat4("projection",srf_projection);
    //srvShader.setMat4("model",srf_model);
}

//缓冲区分配，向着色器传递数组表格
static int surroundview_init_vertices_vbo(
                                          GLuint ArrayBuffer_iD,  //曲面数组缓冲区编号
                                          GLuint vertexBufferID,  //曲面顶点缓冲区编号
                                          GLuint indicesBuffer_ID,//片面索引缓冲区编号
                                          GLuint vertex_num,      //顶点数量
                                          GLuint face_num,        //曲面数量
                                          surface_data_t surface    //曲面信息
                                          )
{
    glGenVertexArrays(1, &ArrayBuffer_iD);           //生成顶点数组对象
    glBindVertexArray(ArrayBuffer_iD);               //绑定顶点数组对象

    glGenBuffers(1, &vertexBufferID);                //生成顶点缓冲区对象
    glBindBuffer(GL_ARRAY_BUFFER, vertexBufferID);   //绑定顶点缓冲区对象
    glBufferData(GL_ARRAY_BUFFER, vertex_num*15*sizeof(*surface.vertex_arry), surface.vertex_arry, GL_STATIC_DRAW);//创建并初始化顶点数据
    //target指定目标缓冲区对象。 符号常量必须为GL_ARRAY_BUFFER或GL_ELEMENT_ARRAY_BUFFER。
    //size指定缓冲区对象的新数据存储的大小（以字节为单位）。
    //data指定将复制到数据存储区以进行初始化的数据的指针，如果不复制数据，则指定NULL。
    //usage指定数据存储的预期使用模式。 符号常量必须为GL_STREAM_DRAW，GL_STATIC_DRAW或GL_DYNAMIC_DRAW。

    glGenBuffers(1, &indicesBuffer_ID);                //生成索引缓冲区对象
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indicesBuffer_ID);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, face_num*3*sizeof(*surface.indices_arry), surface.indices_arry,GL_STATIC_DRAW);
    // glBufferData(GL_ELEMENT_ARRAY_BUFFER, 3, indices, GL_STATIC_DRAW);

    // position attribute--------------------CPU向GPU传入顶点数组的数据并分配属性----------------------------------//
    //参数1：属性索引  参数2：参数的个数  参数3：参数的类型  参数4：相邻的同属性参数在表格中的跨度  参数5：第一个参数在表格中的偏移量
    // vertex
    for(int i =0;i<=8;i++) glEnableVertexAttribArray(i);//使能
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 15 * sizeof(float), (void*)0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 15 * sizeof(float), (void*)(3 * sizeof(float)));
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 15 * sizeof(float), (void*)(5 * sizeof(float)));
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, 15 * sizeof(float), (void*)(7 * sizeof(float)));
    glVertexAttribPointer(4, 2, GL_FLOAT, GL_FALSE, 15 * sizeof(float), (void*)(9 * sizeof(float)));
    glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, 15 * sizeof(float), (void*)(11 * sizeof(float)));
    glVertexAttribPointer(6, 1, GL_FLOAT, GL_FALSE, 15 * sizeof(float), (void*)(12 * sizeof(float)));
    glVertexAttribPointer(7, 1, GL_FLOAT, GL_FALSE, 15 * sizeof(float), (void*)(13 * sizeof(float)));
    glVertexAttribPointer(8, 1, GL_FLOAT, GL_FALSE, 15 * sizeof(float), (void*)(14 * sizeof(float)));
    return 0;
}

//纹理绑定
//可修改为仅绑定两个纹理
void onscreen_mesh_state_restore_program_textures_attribs(GLuint *texYuv, int tex1, int tex2, int tex3,int tex4)
{
	//set the program we need
	glUniform1i(srv_program.sample_tex1, 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texYuv[tex1]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	GL_CHECK(glBindTexture);

	glUniform1i(srv_program.sample_tex2, 1);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texYuv[tex2]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	GL_CHECK(glBindTexture);

    glUniform1i(srv_program.sample_tex3, 2);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texYuv[tex3]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	GL_CHECK(glBindTexture);

    glUniform1i(srv_program.sample_tex4, 3);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texYuv[tex4]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	GL_CHECK(glBindTexture);

}

void srv_draw(surface_data_t surface, int viewport_id,bool tex_load_state)
{
	/*if(prevLUT != pObj->LUT3D || index_buffer_changed == true)
	{
		prevLUT = pObj->LUT3D;
		surroundview_init_vertices_vbo_wrap(pObj);
	}*/ // 初始化数组表可以在init中执行
	//First setup the program onc
    GLuint *texYuv;
    
    if(!tex_load_state)//导入静态图像仿真视角
    {
     texYuv = standalone_init_texture("../resource/image/","bmp");
    }
    else
    {
        /* code :从内存中动态导入图像*/
    }
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texYuv[0]);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, texYuv[1]);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, texYuv[2]);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, texYuv[3]);

    srv_update_view(surface.view_dist,surface.angle,surface.upon,surface.screen_width/surface.screen_height);

    glUseProgram(srv_program.srv_Uid);//启用着色器

    onscreen_mesh_state_restore_program_textures_attribs(texYuv, 1, 2, 3, 4);//导入四张纹理图

    //传采样纹理给片段着色器,也可用shader.h中的set函数来实现
    srv_program.sample_tex1 =glGetUniformLocation(srv_program.srv_Uid, "texture1");
	glUniform1i(srv_program.sample_tex1, 0);//传递前纹理通道数据
    srv_program.sample_tex2 =glGetUniformLocation(srv_program.srv_Uid, "texture2");
    glUniform1i(srv_program.sample_tex2, 1);//传递左纹理通道数据
    srv_program.sample_tex3 =glGetUniformLocation(srv_program.srv_Uid, "texture3");
    glUniform1i(srv_program.sample_tex3, 2);//传递左纹理通道数据
    srv_program.sample_tex4 =glGetUniformLocation(srv_program.srv_Uid, "texture4");
    glUniform1i(srv_program.sample_tex4, 3);//传递左纹理通道数据

	//then change the meshes and draw
    //load 纹理 绑定纹理
    
    //定义缓冲区，分配编号
    GLuint VAO,VBO,EBO;
	//向顶点缓冲区写新数据，顶点坐标，纹理坐标，blending值
    surroundview_init_vertices_vbo(
                                   VAO,                     //曲面数组缓冲区编号
                                   VBO,                     //曲面顶点缓冲区编号
                                   EBO,                     //片面索引缓冲区编号
                                   surface.vertex_num,      //顶点数量
                                   surface.indices_num,     //曲面数量
                                   surface                  //曲面信息
                                   );
    //绘制图元前先要使能顶点数组
    //face_num*3*sizeof(*surface.indices_arry)
    glDrawElements(GL_TRIANGLES, surface.indices_num*3/sizeof(unsigned int), GL_UNSIGNED_INT, 0);
    for(int i =0;i<=8;i++) glDisableVertexAttribArray(i);   //关闭

    glBindBuffer(GL_ARRAY_BUFFER, 0);                       //绑定指向0
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);               //绑定指向0

	glFlush();
}
