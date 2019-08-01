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
		x : 0,
		y : 0,
		width : 960,
		height: 1080,
		//fengyuhong 20190224
		//animate: true,
		animate: false,
	},
	{
		x : 960,
		y : 0,
		width : 960,
		height: 1080,
		animate: false,
	}
};

render_state_t *pObj;

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window);


void render_renderFrame(render_state_t *pObj, GLuint*texYuv)
{
	glClear(GL_COLOR_BUFFER_BIT);
	glViewport(srv_viewports[1].x,
		       srv_viewports[1].y,
			   srv_viewports[1].width,
			   srv_viewports[1].height);

	srv_draw(pObj, texYuv, 1);
    car_draw(1,upon,angle,view_dist);
		//boxes_draw((ObjectBox *)pObj->BoxLUT, (Pose3D_f *)pObj->BoxPose3D, texYuv);
    usleep(33000);
}


int render_setup(render_state_t *pObj)
{
    if(srv_setup(pObj) == -1)
	{
		return -1;
	}
	
	//fengyuhong 20190224 add
	printf("render_setup:srv_setup ok\n");

    //Initialize views
    //srv_views_init();
    /*  */ //????????????????????????????????????????????????????????????

	//fengyuhong 20190224 add
	printf("render_setup:srv_views_init ok\n");

    //STEP2 - initialise the vertices
    car_init();
    GL_CHECK(car_init_vertices_vbo);

	//fengyuhong 20190224 add
	printf("render_setup:car_init ok\n");

    //STEP3 - initialise the individual views
    //screen1_init_vbo();
    GL_CHECK(screen1_init_vbo);

    num_viewports = 2;//sizeof(srv_viewports)/sizeof(srv_viewport_t);

    for (int i = 0; i < num_viewports; i++)
    {
		//fengyuhong 20190225
	    //current_index[i] = (0+i)%num_srv_views;
		current_index[i] = 1;//0
	    set_coords(i, current_index[i]);
    }

	//fengyuhong 201902224 add
	printf("render_setup:set_coords for viewports ok\n");

    //boxes_init();
    render_updateView();

    // Default mode for key/joystick input
    MODE_CAM(srv_coords_vp[0]);

    //cull
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

#if defined(STANDALONE) || defined(SRV_USE_JOYSTICK)
#ifdef _WIN32
	hThread = CreateThread(
		NULL,                   // default security attributes
		0,                      // use default stack size
		scan_thread_function,       // thread function name
		NULL,          // argument to thread function
		0,                      // use default creation flags
		&dwThreadId);   // returns the thread identifier


								// Check the return value for success.
								// If CreateThread fails, terminate execution.
								// This will automatically clean up threads and memory.

	if (hThread == NULL)
	{
		printf("CreateThread failed");
		ExitProcess(3);
	}
#else
    pthread_create(&scan_thread, NULL, scan_thread_function, (void *)&scan_thread_data);
#endif
#endif
    return 0;
}

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
   
   //----------------------------------------纹理绑定----------------------------------------------------//
    unsigned char*texYuv[4];
    int width, height, nrChannels;

    texYuv[0] = stbi_load("../../ext/resource/image/front0.bmp", &width, &height, &nrChannels, 0);
    //stbi_image_free(texYuv[0]);
    texYuv[1]  = stbi_load("../../ext/resource/image/left0.bmp", &width, &height, &nrChannels, 0);
    //stbi_image_free(texYuv[1]);
    texYuv[2]  = stbi_load("../../ext/resource/image/rear0.bmp", &width, &height, &nrChannels, 0);
    //stbi_image_free(texYuv[2]);
    texYuv[3]  = stbi_load("../../ext/resource/image/right0.bmp", &width, &height, &nrChannels, 0);
    //stbi_image_free(texYuv[3]);
     
    
    render_setup(pObj);

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
        

        render_renderFrame(pObj,texYuv);
        //----------------------------------------绘制车模型----------------------------------//
       
        car_draw(1,upon,angle,view_dist);
        glFlush();

        //usleep(22000);
        //---------------------------------------交换缓存，数据处理----------------------------//
        //交换缓存
        glfwSwapBuffers(window);
        glfwPollEvents();       //接收事件信息并返回
    }
    
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