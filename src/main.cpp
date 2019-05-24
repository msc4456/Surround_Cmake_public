#include <glad/glad.h>
#include <GLFW/glfw3.h>
#define STB_IMAGE_IMPLEMENTATION
//#include "learnopengl/stb_image.h"
#include <unistd.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
//#include "learnopengl/shader_s.h"
#include "shader/shader_m.h"
//#include "shader/model.h"
#include "common/stb_image.h"
#include "common/luma_calc.h"
#include "common/Drawpara.h"
#include "s1_car_draw/car.h"
#include "s2_srv_draw/srv.h"
#include <iostream>
#include <cstdio>

#define LUMA_AVG
#define MAX_VIEWPORTS 2

using namespace std;

//定义绘图窗口尺寸
const GLuint src_wind_h =700;
const GLuint src_wind_w =800;

//定义亮度统计因子
float factor1,factor2,factor3,factor4;

surface_data_t srv_renderObj;

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
    


    //---------------------------------------数据绑定----------------------------------------------------//
    float vertices[6702*15];
    FILE *fp = fopen("../../src/shader/vertex.bin","rb");                    //导入顶点
    GLuint flag =fread(vertices,sizeof(vertices),1,fp);     //读入顶点数据表,每次读sizeof(vertices)个，读一次
    fclose(fp);
    //for (int i=0;i<15;i++){ printf("%f\n",vertices[i]); } //打印前15个数据
    
    unsigned int indices[12810*3];                          //读入曲面到indices-曲面索引表

    fp = fopen("../../src/shader/surface.bin","rb");                         //导入曲面
    flag =fread(indices,sizeof(indices),1,fp);
    fclose(fp);
    //for (int i=0;i<3;i++){ printf("%u\n",indices[i]); }    //打印前3个索引

    //srv_init(&srv_renderObj);
    
    // 建立编译着色器
    // ------------------------------------
    Shader srvShader("../../src/shader/texture.vs", "../../src/shader/texture.fs");//以可执行文件Surroundview_sim为起点索引相对路径
    

   
   //----------------------------------------纹理绑定----------------------------------------------------//
    unsigned int texture1, texture2, texture3, texture4;
    // texture 1
    // ---------
    glGenTextures(1, &texture1);
    glBindTexture(GL_TEXTURE_2D, texture1); 
     // set the texture wrapping parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);	// set texture wrapping to GL_CLAMP_TO_EDGE (default wrapping method)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    // set texture filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // load image, create texture and generate mipmaps
    int width, height, nrChannels;

    unsigned char *data = stbi_load("../../ext/resource/image/front0.bmp", &width, &height, &nrChannels, 0);
    if (data)
    {
        factor1 = luma_average(width,height,data);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        std::cout << "Failed to load front texture" << std::endl;
    }
    stbi_image_free(data);

    // texture 2
    // ---------
    glGenTextures(1, &texture2);
    glBindTexture(GL_TEXTURE_2D, texture2);
    // set the texture wrapping parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);	// set texture wrapping to GL_CLAMP_TO_EDGE (default wrapping method)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    // set texture filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // load image, create texture and generate mipmaps
    // data = stbi_load(FileSystem::getPath("resources/textures/awesomeface.png").c_str(), &width, &height, &nrChannels, 0);
    data = stbi_load("../../ext/resource/image/left0.bmp", &width, &height, &nrChannels, 0);
    if (data)
    {
        // note that the awesomeface.png has transparency and thus an alpha channel, so make sure to tell OpenGL the data type is of GL_RGBA
        factor2 = luma_average(width,height,data);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        std::cout << "Failed to load left texture" << std::endl;
    }
    stbi_image_free(data);

    // texture 3
    // ---------
    glGenTextures(1, &texture3);
    glBindTexture(GL_TEXTURE_2D, texture3);
    // set the texture wrapping parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);	// set texture wrapping to GL_CLAMP_TO_EDGE (default wrapping method)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    // set texture filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // load image, create texture and generate mipmaps
    // data = stbi_load(FileSystem::getPath("resources/textures/awesomeface.png").c_str(), &width, &height, &nrChannels, 0);
    data = stbi_load("../../ext/resource/image/rear0.bmp", &width, &height, &nrChannels, 0);
    if (data)
    {
        // note that the awesomeface.png has transparency and thus an alpha channel, so make sure to tell OpenGL the data type is of GL_RGBA
        factor3 = luma_average(width,height,data);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        std::cout << "Failed to load rear texture" << std::endl;
    }
    stbi_image_free(data);

    // texture 4
    // ---------
    glGenTextures(1, &texture4);
    glBindTexture(GL_TEXTURE_2D, texture4);
    // set the texture wrapping parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);	// set texture wrapping to GL_CLAMP_TO_EDGE (default wrapping method)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    // set texture filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // load image, create texture and generate mipmaps
    // data = stbi_load(FileSystem::getPath("resources/textures/awesomeface.png").c_str(), &width, &height, &nrChannels, 0);
    data = stbi_load("../../ext/resource/image/right0.bmp", &width, &height, &nrChannels, 0);
    if (data)
    {
        // note that the awesomeface.png has transparency and thus an alpha channel, so make sure to tell OpenGL the data type is of GL_RGBA
        factor4 = luma_average(width,height,data);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        std::cout << "Failed to load right texture" << std::endl;
    }
    stbi_image_free(data);

    srvShader.use();                 // don't forget to activate/use the shader before setting uniforms!

    srvShader.setInt("texture1", 0); // 此处也可以直接使用glUniform1i(glGetUniformLocation(srvShader.ID, "texture1"), 0);设置统一变量
    srvShader.setInt("texture2", 1);
    srvShader.setInt("texture3", 2);
    srvShader.setInt("texture4", 3);
     
    //--------------------------------------亮度均衡-----------------------------------------//
    luma_adjust2(&factor1,&factor2,&factor3,&factor4);

    #ifdef LUMA_AVG
    srvShader.setFloat("gain1",factor1);
    srvShader.setFloat("gain2",factor2);
    srvShader.setFloat("gain3",factor3);
    srvShader.setFloat("gain4",factor4);

    // srvShader.setFloat("gain1",gain1);
    // srvShader.setFloat("gain2",gain2);
    // srvShader.setFloat("gain3",gain3);
    // srvShader.setFloat("gain4",gain4);
    #else
    srvShader.setFloat("gain1",1);
    srvShader.setFloat("gain2",1);
    srvShader.setFloat("gain3",1);
    srvShader.setFloat("gain4",1);  
    #endif

    //判断导入是否成功
    if(car_init()){ printf("load_3d model files failed\n");return -1;}//返回零则初始化成功
    unsigned int VBO, VAO, EBO;         //定义顶点缓冲区，顶点数组对象，索引缓冲区对象

    //--------------------------------------渲染循环---------------------------------------//
    while (!glfwWindowShouldClose(window))
    {

        processInput(window);

        // render
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        glDepthFunc(GL_LEQUAL);
        glEnable(GL_DEPTH_TEST);
        //glEnable(GL_CULL_FACE);
        //glCullFace(GL_FRONT);

        // render container
        srvShader.use();
        
         //----------------------------------绘制3D曲面------------------------------------//
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
        glm::mat4 srf_projection = glm::perspective(45.0f, singleRatio, 0.1f, 1000.0f);

        //传参到着色器
        
        srvShader.setMat4("view",srf_view);
        srvShader.setMat4("projection",srf_projection);
        srvShader.setMat4("model",srf_model);
    

        glGenVertexArrays(1, &VAO);           //生成顶点数组对象
        glBindVertexArray(VAO);               //绑定顶点数组对象

        glGenBuffers(1, &VBO);                //生成顶点缓冲区对象
        glBindBuffer(GL_ARRAY_BUFFER, VBO);   //绑定顶点缓冲区对象
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);//创建并初始化顶点数据
        //target指定目标缓冲区对象。 符号常量必须为GL_ARRAY_BUFFER或GL_ELEMENT_ARRAY_BUFFER。
        //size指定缓冲区对象的新数据存储的大小（以字节为单位）。
        //data指定将复制到数据存储区以进行初始化的数据的指针，如果不复制数据，则指定NULL。
        //usage指定数据存储的预期使用模式。 符号常量必须为GL_STREAM_DRAW，GL_STATIC_DRAW或GL_DYNAMIC_DRAW。

        glGenBuffers(1, &EBO);                //生成索引缓冲区对象
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
        // glBufferData(GL_ELEMENT_ARRAY_BUFFER, 3, indices, GL_STATIC_DRAW);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture1);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, texture2);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, texture3);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, texture4);

        // position attribute--------------------CPU向GPU传入顶点数组的数据并分配属性----------------------------------//
        //参数1：属性索引  参数2：参数的个数  参数3：参数的类型  参数4：相邻的同属性参数在表格中的跨度  参数5：第一个参数在表格中的偏移量
        // vertex
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 15 * sizeof(float), (void*)0);//从VBO中
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 15 * sizeof(float), (void*)(3 * sizeof(float)));
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 15 * sizeof(float), (void*)(5 * sizeof(float)));
        glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, 15 * sizeof(float), (void*)(7 * sizeof(float)));
        glVertexAttribPointer(4, 2, GL_FLOAT, GL_FALSE, 15 * sizeof(float), (void*)(9 * sizeof(float)));
        glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, 15 * sizeof(float), (void*)(11 * sizeof(float)));
        glVertexAttribPointer(6, 1, GL_FLOAT, GL_FALSE, 15 * sizeof(float), (void*)(12 * sizeof(float)));
        glVertexAttribPointer(7, 1, GL_FLOAT, GL_FALSE, 15 * sizeof(float), (void*)(13 * sizeof(float)));
        glVertexAttribPointer(8, 1, GL_FLOAT, GL_FALSE, 15 * sizeof(float), (void*)(14 * sizeof(float)));

        //绘制基本单元 glDrawElements 和glDrawArrays具备相同的作用，前者占用更大的内存，后者可以跳跃式的绘制
        for(int k=0;k<=8;k++) glEnableVertexAttribArray(k);
        glDrawElements(GL_TRIANGLES, sizeof(indices)/sizeof(unsigned int), GL_UNSIGNED_INT, 0);
        for(int k=0;k<=8;k++) glDisableVertexAttribArray(k);

        glDeleteBuffers(1,&VBO);//地址指向零之前，需要删除buffer，否则会导致内存和显存的持续泄漏
        glDeleteBuffers(1,&EBO);//

        glBindBuffer(GL_ARRAY_BUFFER,0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0);

        usleep(33000);
        //----------------------------------------绘制车模型----------------------------------//
       
        car_draw(1,upon,angle,view_dist);
        glFlush();

        //usleep(22000);
        //---------------------------------------交换缓存，数据处理----------------------------//
        //交换缓存
        glfwSwapBuffers(window);
        glfwPollEvents();       //接收事件信息并返回
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    
    // optional: de-allocate all resources once they've outlived their purpose:
    // -------------------------------------------释放内存-----------------------------------//
    car_deinit();//释放模型
    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();//清理所有未完成的GLFW资源的内存
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window)
{
    usleep(100);
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) 
    {
       upon = upon + 2.0f;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) 
    {
       upon = upon - 2.0f;
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) 
    {  
       angle = angle + 2.0f;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) 
    {
       angle = angle - 2.0f;
    }
    if (glfwGetKey(window, GLFW_KEY_8) == GLFW_PRESS) 
    {
       view_dist = view_dist - 0.1f;
    }
    if (glfwGetKey(window, GLFW_KEY_5) == GLFW_PRESS) 
    {
       view_dist = view_dist + 0.1f;
    }
    if(upon>90.0)   upon  = 90.0;
    if(upon<21.0)    upon  = 21.0;
    if(angle>90.0)  angle = 90.0;
    if(angle<-90.0) angle = -90.0;
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
		height = 1;                  // 
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