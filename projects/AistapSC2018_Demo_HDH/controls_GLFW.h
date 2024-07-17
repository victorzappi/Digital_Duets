/*
 * controls_keyboard.h
 *
 *  Created on: Feb 16, 2018
 *      Author: Victor Zappi
 */

#ifndef CONTROLS_GLFW_H_
#define CONTROLS_GLFW_H_

#include <GL/glew.h>    // include GLEW and new version of GL on Windows [luv Windows]
#include <GLFW/glfw3.h> // GLFW helper library


void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void cursor_position_callback(GLFWwindow *window, double xpos, double ypos);
void cursor_enter_callback(GLFWwindow* window, int entered);
void win_pos_callback(GLFWwindow* window, int posx, int posy);

void mouseButtonFunction();
void cursorPosFunction();
void cursorEnterFunction();
void winPosFunction();

void key_callback(GLFWwindow *, int key, int scancode, int action, int mods);


#endif /* CONTROLS_GLFW_H_ */
