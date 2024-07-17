/*
This code is distributed under the 2-Clause BSD License

Copyright 2017 Victor Zappi
*/

/* a bunch of useful utils, kindly taken and acknowledged from different sources */

#include "openGL_utils.h"

#include <string>
#include <fstream>  // filestream
#include <iostream> // cerr


using namespace std;

// adapted from: http://www.nexcius.net/2012/11/20/how-to-load-a-glsl-shader-in-opengl-using-c/
string readFile(const char *filePath) {
    string content;
    ifstream fileStream(filePath, ios::in);

    if(!fileStream.is_open()) {
        cerr << "Could not read file " << filePath << ". File does not exist." << std::endl;
        return "";
    }

    std::string line = "";
    while(!fileStream.eof()) {
        std::getline(fileStream, line);
        content.append(line + "\n");
    }

    fileStream.close();
    return content;
}


// from https://capnramses.github.io/opengl/shaders.html
void _print_shader_info_log(GLuint shader_index) {
  int max_length = 2048;
  int actual_length = 0;
  char log[2048];
  glGetShaderInfoLog(shader_index, max_length, &actual_length, log);
  printf("shader info log for GL index %u:\n%s\n", shader_index, log);
}

// from https://capnramses.github.io/opengl/shaders.html
bool checkCompileErrors(GLuint shader_index) {
	// check for compile errors
	int params = -1;
	glGetShaderiv (shader_index, GL_COMPILE_STATUS, &params);
	if (GL_TRUE != params) {
		fprintf (stderr, "ERROR: GL shader index %i did not compile\n", shader_index);
		_print_shader_info_log(shader_index);
		return false;
	}
	return true;
}

// from https://capnramses.github.io/opengl/shaders.html
void _print_program_info_log(GLuint program) {
  int max_length = 2048;
  int actual_length = 0;
  char log[2048];
  glGetProgramInfoLog (program, max_length, &actual_length, log);
  printf("program info log for GL index %u:\n%s", program, log);
}

// from https://capnramses.github.io/opengl/shaders.html
bool checkLinkErrors(GLuint program)  {
	// check if link was successful
	int params = -1;
	glGetProgramiv (program, GL_LINK_STATUS, &params);
	if (GL_TRUE != params) {
		fprintf (
			stderr,
			"ERROR: could not link shader programme GL index %u\n",
			program
		);
	    _print_program_info_log (program);
		return false;
	}
	return true;
}
