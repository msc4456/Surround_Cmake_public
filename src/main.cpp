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
#define MAX_SRV_VIEWS 10

using namespace std;

//定义绘图窗口尺寸
const GLuint src_wind_h =700;
const GLuint src_wind_w =800;

//定义亮度统计因子
float factor1,factor2,factor3,factor4;

surface_data_t srv_renderObj;

glm::mat4 mProjection;
// Camera matrix
glm::mat4 mView;
// Model matrix : an identity matrix (model will be at the origin)
glm::mat4 mModel_bowl;  // Changes for each model !
// Our ModelViewProjection : multiplication of our 3 matrices
glm::mat4 mMVP_bowl;
glm::mat4 mMVP_car;

render_state_t pObj;

typedef struct _srv_coords_t
{
	float camx;
	float camy;
	float camz;
	float targetx;
	float targety;
	float targetz;
	float anglex;
	float angley;
	float anglez;
} srv_coords_t;

srv_coords_t srv_coords_vp[MAX_VIEWPORTS];

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
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window);


srv_coords_t srv_coords[] = {
        {0.000000, 88.167992, 291.995667, 0.000000, 25.990564, 33.987663, -0.999456, 0.000000, 0.000000}, // Front adaptive bowl view
        //fengyuhong add two same top view
        {0.000000, 0.000000, 300.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000}, // Top down view
        //{0.000000, 0.000000, 300.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000}, // Top down view
        //{0.000000, 0.000000, 300.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000}, // Top down view
        {0.000000, -260.000000, 160.000000, -5.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000},// Following eight views pan around the vehicle
        {0.000000, -260.000000, 160.000000, -5.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.897000},
        {0.000000, -260.000000, 160.000000, -5.000000, 0.000000, 0.000000, 0.000000, 0.000000, 1.794000},
        {0.000000, -260.000000, 160.000000, -5.000000, 0.000000, 0.000000, 0.000000, 0.000000, 2.691000},
        {0.000000, -260.000000, 160.000000, -5.000000, 0.000000, 0.000000, 0.000000, 0.000000, 3.588000},
        {0.000000, -260.000000, 160.000000, -5.000000, 0.000000, 0.000000, 0.000000, 0.000000, 4.485000},
        {0.000000, -260.000000, 160.000000, -5.000000, 0.000000, 0.000000, 0.000000, 0.000000, 5.382000},
        {0.000000, -260.000000, 160.000000, -5.000000, 0.000000, 0.000000, 0.000000, 0.000000, 6.280000},
        {0.000000, 88.200000, 192.000000, -1.000000, 26.000000, 34.000000, -1.500000, 0.000000, -3.1416},//Back
        {-0.000000, 200.000000, 220.000000, -7.000000, 63.000000, 0.000000, -1.500000, -0.000000, -1.570169}, //Left
        {-0.000000, 200.000000, 220.000000, -7.000000, 63.000000, 0.000000, -1.500000, 0.000000, 1.570169}, //Right
        {-0.000000, -29.049999, 440.000000, -67.374092, -39.541992, 0.000000, 0.000000, -0.000000, 0.000000}, //Left blindspot
        {-0.000000, -29.049999, 440.000000, 67.374092, -39.541992, 0.000000, 0.000000, -0.000000, 0.000000}, //Right blindspot 
        {0.000000, 0.000000, 300.000000, 0.000000, 60.000000, 0.000000, -1.000000, 0.000000, 3.100000}, //zoomed out
        {0.000000, 0.000000, 380.000000, 0.000000, 60.000000, 0.000000, -1.000000, 0.000000, 3.100000}, //zoomed out
        {0.000000, 0.000000, 440.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000}, //zoomed in
        {0.000000, 190.300049, 150.000000, -1.000000, 80.000000, 35.000000, -1.000000, 0.000000, 0.000000}, // FIle DUMP 0
        {0.000000, 190.300049, 150.000000, -1.000000, 80.000000, 35.000000, -1.750000, 0.000000, 0.000000}, // File DUMP 1
        {0.000000, 200.000000, 240.000000, -1.000000, 80.000000, 35.000000, -1.500000, 0.000000, 0.000000}, //Front
        {-15.149982, 149.549988, 257.200226, 3.312001, 38.720036, 35.000000, -1.484400, 0.008200, -3.228336},//Back
        {-0.000000, 197.199997, 240.000000, -1.000000, 66.135956, 35.000000, -1.521800, -0.000000, -1.565569}, //Left
        {-0.000000, 200.000000, 240.000000, -1.000000, 68.776001, 35.000000, -1.500000, 0.000000, 1.570169}, //Right
        {-0.000000, -29.049999, 440.000000, -67.374092, -39.541992, 0.000000, 0.000000, -0.000000, 0.000000}, //Left blindspot
        {-0.000000, -29.049999, 440.000000, 67.374092, -39.541992, 0.000000, 0.000000, -0.000000, 0.000000}, //Right blindspot
        {-15.149982, 149.549988, 257.200226, 3.312001, 38.720036, 35.000000, -1.484400, 0.008200, -3.228336},//Back
        {-0.000000, 197.199997, 240.000000, -1.000000, 66.135956, 35.000000, -1.521800, -0.000000, -1.565569}, //Left
        {-0.000000, 200.000000, 240.000000, -1.000000, 68.776001, 35.000000, -1.500000, 0.000000, 1.570169}, //Right
        {0.000000, 230.300049, 240.000000, -1.000000, 80.000000, 35.000000, -1.500000, 0.000000, 0.000000}, //
        {0.000000, 250.300049, 240.000000, -1.000000, 80.000000, 35.000000, -1.500000, 0.000000, 0.000000}
};

int num_srv_views;

void srv_views_init()
{
	ifstream file("srv_views.txt");
	int nlines = 0;
	string line;

	num_srv_views = (int)(sizeof(srv_coords)/sizeof(srv_coords_t));	

	if(!file.is_open())
	{
		printf("3DSRV: Cannot open srv_views.txt. Using default views\n");
		return;
	}

	while (!file.eof() && (nlines < MAX_SRV_VIEWS))
	{
		getline(file, line);
		sscanf(line.c_str(), "%f, %f, %f, %f, %f, %f, %f, %f, %f",
					&(srv_coords[nlines].camx),
					&(srv_coords[nlines].camy),
					&(srv_coords[nlines].camz),
					&(srv_coords[nlines].anglex),
					&(srv_coords[nlines].angley),
					&(srv_coords[nlines].anglez),
					&(srv_coords[nlines].targetx),
					&(srv_coords[nlines].targety),
					&(srv_coords[nlines].targetz));
		nlines++;
	}
	num_srv_views = nlines;
	file.close();
}



void render_renderFrame(render_state_t *pObj, GLuint*texYuv)
{
	glClear(GL_COLOR_BUFFER_BIT);
    glPolygonMode(GL_FRONT,GL_LINES);
	glViewport(srv_viewports[1].x,
		       srv_viewports[1].y,
			   srv_viewports[1].height,
			   srv_viewports[1].width);

	srv_draw(pObj, texYuv, 1);
    //car_draw(1,upon,angle,view_dist);
		//boxes_draw((ObjectBox *)pObj->BoxLUT, (Pose3D_f *)pObj->BoxPose3D, texYuv);
    usleep(33000);
}



void  render_updateView()
{
        /*int i = 1;
		mProjection= glm::perspective(degreesToRadians(40), (float)srv_viewports[i].width/ (float)srv_viewports[i].height, 1.0f, 5000.0f);
        printf("persoective degree is %f\n",degreesToRadians(40));
        printf("view port is  %f\n",(float)srv_viewports[i].width/ (float)srv_viewports[i].height); 

		mView       = glm::lookAt(glm::vec3(srv_coords[i].camx, srv_coords[i].camy, srv_coords[i].camz), // Camera is at (4,3,3), in World Space
				                  glm::vec3(srv_coords[i].targetx,srv_coords[i].targety,srv_coords[i].targetz), // and looks at the origin
				                  glm::vec3(0,1,0)  // Head is up (set to 0,-1,0 to look upside-down)
                                 );
        printf("lookAt() is cam:%f,%f,%f,target:%f %f %f \n",srv_coords[i].camx,srv_coords[i].camx,srv_coords[i].camx,
                                                             srv_coords[i].targetx,srv_coords[i].targety,srv_coords[i].targetz);*/


		mProjection= glm::perspective(0.6f,0.8f, 1.0f, 5000.0f);

		mView      = glm::lookAt(glm::vec3(0, 0, 300), // Camera is at (4,3,3), in World Space
				                  glm::vec3(0, 0, 0), // and looks at the origin
				                  glm::vec3(0, 1, 0)  // Head is up (set to 0,-1,0 to look upside-down)
                                 );
		mView = glm::rotate(mView, 0.0f, glm::vec3(1.0, 0.0, 0.0));
		mView = glm::rotate(mView, 0.0f, glm::vec3(0.0, 1.0, 0.0));
		mView = glm::rotate(mView, 0.0f, glm::vec3(0.0, 0.0, 1.0));

		mModel_bowl     = glm::scale(glm::mat4(1.0f), glm::vec3(1.0f, 1.0f, (80.0f/540.0f)));  // Shashank: Updated scale works good without any scale on Z
		//mModel_car = glm::translate(mModel_car, glm::vec3(0.0, 0.0, 800.0));
		mMVP_bowl       = mProjection * mView * mModel_bowl;
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
    srv_views_init();
   
	//fengyuhong 20190224 add
	printf("render_setup:srv_views_init ok\n");

    //STEP2 - initialise the vertices
    //car_init();
    //GL_CHECK(car_init_vertices_vbo);

	//fengyuhong 20190224 add
	printf("render_setup:car_init ok\n");

    //STEP3 - initialise the individual views
    //screen1_init_vbo();
    GL_CHECK(screen1_init_vbo);

    //boxes_init();
    render_updateView();

    //cull
    //glEnable(GL_CULL_FACE);
    //glCullFace(GL_BACK);

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
    GLuint texYuv[4];
    // texture 1
    // ---------
    glGenTextures(1, &texYuv[0]);
    glBindTexture(GL_TEXTURE_2D, texYuv[0]); 
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
    glGenTextures(1, &texYuv[1]);
    glBindTexture(GL_TEXTURE_2D, texYuv[1]);
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
    glGenTextures(1, &texYuv[2]);
    glBindTexture(GL_TEXTURE_2D, texYuv[2]);
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
    glGenTextures(1, &texYuv[3]);
    glBindTexture(GL_TEXTURE_2D, texYuv[3]);
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
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        std::cout << "Failed to load right texture" << std::endl;
    }

    stbi_image_free(data);
    
    if(render_setup(&pObj)==-1)
    {
        printf("render_setup error!!!!\n");
        return -1;
    }

    //--------------------------------------渲染循环---------------------------------------//
    while (!glfwWindowShouldClose(window))
    {

        processInput(window);

        // render
        glClearColor(0.0f, 0.5f, 0.5f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        glDepthFunc(GL_LEQUAL);
        glEnable(GL_DEPTH_TEST);
        //glEnable(GL_CULL_FACE);
        //glCullFace(GL_FRONT);
        render_updateView();
        // render container
        glPolygonMode(GL_FRONT,GL_LINES);

        render_renderFrame(&pObj,texYuv);
        //----------------------------------------绘制车模型----------------------------------//
       
        //car_draw(1,upon,angle,view_dist);
        glFlush();
        usleep(22000);
        //---------------------------------------交换缓存，数据处理----------------------------//
        //交换缓存
        glfwSwapBuffers(window);
        glfwPollEvents();       //接收事件信息并返回
    }
    
    // optional: de-allocate all resources once they've outlived their purpose:
    // -------------------------------------------释放内存-----------------------------------//
    //car_deinit();//释放模型
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