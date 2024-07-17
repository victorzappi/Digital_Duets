/*
 * opneGL_functions.cpp
 *
 *  Created on: 2016-12-29
 *      Author: Victor Zappi
 */

#include "openGL_functions.h"

#include <cstdio>
#include <string>

#include "openGL_utils.h"

using namespace std;

GLFWwindow * initOpenGL(int width, int heigth, string windowName, float magnifier) {
	// start GL context and O/S window using the GLFW helper library
	if (!glfwInit()) {
		fprintf (stderr, "ERROR: could not start GLFW3\n");
		return NULL;
	}

	// create window handler
	GLFWwindow* window = glfwCreateWindow (width*magnifier, heigth*magnifier, windowName.c_str(), NULL, NULL);

	if (!window) {
		fprintf (stderr, "ERROR: could not open window with GLFW3\n");
		glfwTerminate();
		return NULL;
	}

	glfwMakeContextCurrent(window);

	// start GLEW extension handler
	glewExperimental = GL_TRUE;
	glewInit ();

	// print version info
	const GLubyte* renderer = glGetString(GL_RENDERER); // get renderer string
	const GLubyte* version  = glGetString(GL_VERSION);  // version as a string
	printf ("Renderer: %s\n", renderer);
	printf ("OpenGL version supported %s\n", version);

	// this disables the VSync with the monitor! FUNDAMENTAL!
	glfwSwapInterval(0);

	return window; // (:
}

bool loadShaderProgram(const char *vertShaderPath, const char *fragShaderPath, GLuint &vs, GLuint &fs, GLuint &shader_program) {
	// load shaders' content [originally they are 2 strings of characters!]
	string vertShaderStr = readFile(vertShaderPath);
	string fragShaderStr = readFile(fragShaderPath);
	const char *vertex_shader   = vertShaderStr.c_str();
	const char *fragment_shader = fragShaderStr.c_str();

	// create and compile shaders
	vs = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vs, 1, &vertex_shader, NULL);
	glCompileShader(vs);
	if(!checkCompileErrors(vs))
		return false;

	fs = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fs, 1, &fragment_shader, NULL);
	glCompileShader(fs);
	if(!checkCompileErrors(fs))
		return false;

	// create GPU program with those vertex and fragment shaders
	shader_program = glCreateProgram();
	glAttachShader(shader_program, fs);
	glAttachShader(shader_program, vs);
	glLinkProgram(shader_program);
	if(!checkLinkErrors(shader_program))
		return false;

	return true;
}

void setUpVbo(int size, float *vertices, GLuint &vbo) {
	glGenBuffers(1, &vbo); // here we create the buffer; we can also create arrays of buffers, but in this case we create only [as specified via the first param]
	glBindBuffer(GL_ARRAY_BUFFER, vbo); // means that from now on [next line...] we work on this VBO
	glBufferData(GL_ARRAY_BUFFER, size, vertices, GL_STATIC_DRAW); // load vertices' attributes
}

void setUpVao(GLuint vbo, GLuint &vao) {
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo); // bind to VBO [means shaders fetch data from there]
}

void setUpTexture(int textureWidth, int textureHeight, float *texturePixels, GLuint &texture) {
	glGenTextures(1, &texture);

	glEnable(GL_TEXTURE_2D); // we're gonna work on a 2D texture
	glActiveTexture(GL_TEXTURE0); // this will be the first of the N textures that we might work on at the same time
	glBindTexture(GL_TEXTURE_2D, texture); // form now on, this is our texture and we will access its parameters

	// texture parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
	GLfloat border_color[] = { 0, 0, 0, 0 };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border_color);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

	// load pixels into texture
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, textureWidth, textureHeight, 0, GL_RGBA, GL_FLOAT, texturePixels);
}

void setUpFbo(GLuint texture, GLuint &fbo) {
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo); // we're gonna work on this FBO we just created

	// Set our texture as our color attachment #0 [ = the freaking FBO can have access to our texture]
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, texture, 0);

	// finally say that the freaking FBO will specifically write to our texture!
	GLenum drawBuffers = GL_COLOR_ATTACHMENT0;
	glDrawBuffers(1, &drawBuffers);

	// Always check that our framebuffer is ok
	GLenum retval = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if(retval != GL_FRAMEBUFFER_COMPLETE) {
		printf("Something went wrong while initializing FBO...%X ):\n", (int)retval);
		return;
	}

	// un-bind FBO for now, it will be re-bound in the main cycle
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

