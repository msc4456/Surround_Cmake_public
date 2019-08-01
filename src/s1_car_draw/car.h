/*
 *******************************************************************************
 *
 * Copyright (C) 2013 Texas Instruments Incorporated - http://www.ti.com/
 * ALL RIGHTS RESERVED
 *
 *******************************************************************************
 */

#ifndef _CAR_H_
#define _CAR_H_

#include <glm/vec3.hpp> // glm::vec3
#include <glm/vec4.hpp> // glm::vec4, glm::ivec4
#include <glm/mat4x4.hpp> // glm::mat4
#include <glm/gtc/matrix_transform.hpp> // glm::translate, glm::rotate, glm::scale, glm::perspective
#include <glm/gtc/type_ptr.hpp> // glm::value_ptr
#include <glm/gtx/string_cast.hpp>// 用于打印 mat4 查看当前状态

#ifdef __cplusplus
extern "C" {
#endif

#define degreesToRadians(x) x*(3.141592f/180.0f)

#define GL_CHECK(x);

int car_init();
int car_deinit();
void car_draw(int viewport_id,float upon,float angle,float view_dist);
void car_updateView(int viewport_id);
void car_change();
void car_getScreenLimits(int *xmin, int *xmax, int *ymin, int *ymax);
int load_texture_bmp(unsigned int tex, const char *filename);

#ifdef __cplusplus
}

#endif /* __cplusplus */

#endif /*   _CAR_H_   */
