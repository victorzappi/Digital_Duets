/*
 * openGL_utils.h
 *
 *  Created on: 2016-12-29
 *      Author: Victor Zappi
 */

#ifndef OPENGL_UTILS_H_
#define OPENGL_UTILS_H_

#include <GL/glew.h>    // include GLEW and new version of GL on Windows [luv Windows]
#include <GLFW/glfw3.h> // GLFW helper library

#include <string>

// load shader from file
std::string readFile(const char *filePath);
bool checkCompileErrors(GLuint shader_index);
bool checkLinkErrors(GLuint program);

#endif /* OPENGL_UTILS_H_ */
