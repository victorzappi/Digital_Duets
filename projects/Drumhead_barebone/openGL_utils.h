/*
This code is distributed under the 2-Clause BSD License

Copyright 2017 Victor Zappi
*/

/* a bunch of useful utils, kindly taken and acknowledged from different sources */

#ifndef OPENGL_UTILS_H_
#define OPENGL_UTILS_H_

#include <GL/glew.h>    // include GLEW and new version of GL on Windows [luv Windows]
#include <GLFW/glfw3.h> // GLFW helper library

#include <string>

// load shader from file
std::string readFile(const char *filePath); // adapted from: http://www.nexcius.net/2012/11/20/how-to-load-a-glsl-shader-in-opengl-using-c/
bool checkCompileErrors(GLuint shader_index); // from https://capnramses.github.io/opengl/shaders.html
bool checkLinkErrors(GLuint program); // from https://capnramses.github.io/opengl/shaders.html

#endif /* OPENGL_UTILS_H_ */
