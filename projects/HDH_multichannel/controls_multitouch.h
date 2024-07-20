/*
 * controls_multitouch.h
 *
 *  Created on: Jul 2, 2024
 *      Author: Victor Zappi
 */

#ifndef CONTROLS_MULTITOUCH_H_
#define CONTROLS_MULTITOUCH_H_

#include <GL/glew.h>    // include GLEW and new version of GL on Windows [luv Windows]
#include <GLFW/glfw3.h> // GLFW helper library

bool initMultitouchControls(GLFWwindow *window);
void startMultitouchControls();
void stopMultitouchControls();

int swapTouches(int oldTouchID, int newTouchID, int newPosY);



#endif /* CONTROLS_MULTITOUCH_H_ */
