/*
 * controls_keyboard.h
 *
 *  Created on: Feb 16, 2018
 *      Author: Victor Zappi
 */

#ifndef CONTROLS_KEYBOARD_H_
#define CONTROLS_KEYBOARD_H_

#include <GL/glew.h>    // include GLEW and new version of GL on Windows [luv Windows]
#include <GLFW/glfw3.h> // GLFW helper library


void key_callback(GLFWwindow *, int key, int scancode, int action, int mods);


#endif /* CONTROLS_KEYBOARD_H_ */
