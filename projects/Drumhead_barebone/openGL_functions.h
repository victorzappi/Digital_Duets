/*
This code is distributed under the 2-Clause BSD License

Copyright 2017 Victor Zappi
*/

/* This file wraps up all the generic OpenGL functions that we need */

#ifndef OPENGL_FUNCTIONS_H_
#define OPENGL_FUNCTIONS_H_

#include <GL/glew.h>    // include GLEW and new version of GL on Windows [luv Windows]
#include <GLFW/glfw3.h> // GLFW helper library

#include <string>

GLFWwindow * initOpenGL(int width, int heigth, std::string windowName, float magnifier);
bool loadShaderProgram(const char *vertShaderPath, const char *fragShaderPath, GLuint &vs, GLuint &fs, GLuint &shader_program);
void setUpVbo(int size, float *vertices, GLuint &vbo);
void setUpVao(GLuint vbo, GLuint &vao);
void setUpTexture(int textureWidth, int textureHeight, float *texturePixels, GLuint &texture);
void setUpFbo(GLuint texture, GLuint &fbo);
void setUpPbo(int size, GLuint &pbo);



#endif /* OPENGL_FUNCTIONS_H_ */
