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

//曲面绘制数据信息结构体
typedef struct{
    float*vertex_arry;
    float*indices_arry;
    unsigned int vertex_num;
    unsigned int indices_num;
    GLfloat view_dist;
    GLfloat angle;
    GLfloat upon;
    GLfloat screen_width;
    GLfloat screen_height;
}surface_data_t;

//srv functions
int srv_init(surface_data_t *pObj);
void srv_draw(GLuint *texYuv, int viewport_id);
#endif /*   _SRV_H_    */
