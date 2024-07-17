/*
[2-Clause BSD License]

Copyright 2017 Victor Zappi

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer
in the documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* A big "thank you" to Andrew Allen, Nikunj Raghuvanshi, Arvind Vasudevan and Sidney Fels */








/*
 * This application is an off-line simulation of a simple drumhead running on the GPU.
 * The drumhead is excited with an impulse [hit] repeatedly, the synthesized audio is ignored and then application exits...
 * sounds like a useless application, but it's a good starting point for real-time audio simulations.
 * The used solver is based on an FDTD scheme and it is implemented as a shader program.
 */

/* The main file lays down the specific code structure needed to run the GPU FDTD solver and contains no OpenGL generic calls [these are wrapped in the other files] */

#include <cstdio>
#include <unistd.h> // getopt
#include <cstdlib>  // atoi
#include <cstring> // memset

#include <GL/glew.h>    // basic graphics lib
#include <GLFW/glfw3.h> // GLFW helper library

#include "openGL_functions.h"

#define NUM_OF_TIMESTEPS 2

#define quad0 0
#define quad1 1
#define quad2 2



//****************************************
// editable parameters [also via cmd line]
//****************************************
int domainSize[2]    = {80, 80};
float magnifier      = 10; 	// how big of a render we want
int excitationPos[2] = {domainSize[0]/2, domainSize[1]/2}; // ~ center
int listenerPos[2]   = {excitationPos[0]+3, excitationPos[1]};
float maxExcitation  = 10; // dimensionless
float excitationFreq = 1; // Hz, so it's 1 strike per second
int buffer_size      = 128;
int samplerate       = 44100;
int duration		 = 15; // seconds
//****************************************
//****************************************


bool visualDebug = true; // to render the whole texture, including normally hidden portions


//----------------------------------------
// simulation working vars
//----------------------------------------
float *totalAudioBuffer; // to store some audio
int quad;
/*
* quad0: quad on left side full viewport
* quad0: quad on right side full viewport
* quad1: thin quad on top of full viewport
*
* have a look down, at the full quad layout
*/
int state;
/*
* state0: draw quad0 [left]
* state1: read audio from quad1 [right] cos quad0 might not be ready yet
* state2: draw quad1 [right]
* state3: read audio from quad0 [left] cos quad1 might not be ready yet
*/
int textureWidth;
int textureHeight;
float excitation = maxExcitation; // we start with a strike!
float deltaCoordX; // width of each fragment
float deltaCoordY; // height of each fragment
float wrCoord[2] = {0, 0}; // to store audio [sampled simulation output] in next available texture fragment and channel
int vertices[NUM_OF_TIMESTEPS+1][2]; // vertices and attributes will be stored here, for quad0, quad1 and quad2 [2 time steps + audio]


GLFWwindow* window;
GLuint shader_program_render = 0;
GLuint vs_render = 0;
GLuint fs_render = 0;
GLuint shader_program_fbo = 0;
GLuint vs_fbo = 0;
GLuint fs_fbo = 0;
GLuint vbo = 0;
GLuint vao = 0;
GLuint texture = 0;
GLuint fbo = 0;
GLuint pbo = 0;
// locations of uniforms
GLuint state_loc;
GLuint excitation_loc;
GLuint wrCoord_loc;

//----------------------------------------
//----------------------------------------



int excitationDt; // delta t between hits in samples
int totalSampleNum; // duration of simulation in samples
int morehelp;





/*
 *   full quad/texture layout
 *   _______________________
 *  |_______________________|<----quad2 [audio row]
 *  |_______________________|<----isolation row [empty]
 *  |           |           |              ^
 *  |           |           |       layout "ceiling"
 *  |   quad0   |   quad1   |
 *  |           |           |
 *  |___________|___________|
 *
 */

int clng = 2; // audio row [quad2, at the very top of texture] + isolation row [below audio row] = the "ceiling" of the layout


//--------------------------------------------------------------------------------------------------
// help
//--------------------------------------------------------------------------------------------------
static void help(void) {
	printf( "Usage:\n"
			"-h,		help\n"
			"-W [int],      domain width [pixel num] - default is 80 pixels\n"
			"-H [int],      domain height [pixel num] - default is 80 pixels\n"
			"-m [float],    window magnifier [multiplier] - default is times 10\n"
			"-x [int],      excitation x position [pixel index] - default is halfway domain width\n"
			"-y [int],      excitation y position [pixel index] - default is halfway domain height\n"
			"-X [int],      listener x position [pixel index] - default is 3 pixels on the right of excitation\n"
			"-Y [int],      listener y position [pixel index] - default is same as excitation\n"
			"-a [float],    excitation peak amplitude [dimensionless] - default is 10\n"
			"-f [float],    excitation strike frequency [Hz] - default is 1 Hz\n"
			"-b [int],      buffer size [samples] - default is 128 samples\n"
			"-r [int],      sample rate [samples] - default is 44100 samples per second\n"
			"-d [int],      duration of simulation [sec] - default is 15 sec [on some GPUs simulation runs faster than real-time!]\n"
			"\n");
}

int parseArguments(int argc, char *argv[]) {
	morehelp = 0;
	excitationFreq = 1;
	while (1) {
		int c;
		if ((c = getopt(argc, argv, "hW:H:m:x:y:X:Y:a:f:b:r:d:")) < 0)
			break;
		switch (c) {
			case 'h':
				morehelp++;
				break;
			case 'W':
				domainSize[0] = atoi(optarg);
				break;
			case 'H':
				domainSize[1] = atoi(optarg);
				break;
			case 'm':
				magnifier = atof(optarg);
				break;
			case 'x':
				excitationPos[0] = atoi(optarg);
				break;
			case 'y':
				excitationPos[1] = atoi(optarg);
				break;
			case 'X':
				listenerPos[0] = atoi(optarg);
				break;
			case 'Y':
				listenerPos[1] = atoi(optarg);
				break;
			case 'a':
				maxExcitation = atof(optarg);
				break;
			case 'f':
				excitationFreq = atof(optarg);
				break;
			case 'b':
				buffer_size = atoi(optarg);
				break;
			case 'r':
				samplerate = atoi(optarg);
				break;
			case 'd':
				duration = atoi(optarg);
				break;
			default:
				printf("Uhm...option \"-%c\" not recognized...\n", c);
				morehelp++;
				break;
		}
	}

	if (morehelp) {
		help();
		return 1;
	}

	excitationDt = samplerate/excitationFreq;
	totalSampleNum = duration*samplerate;

	return 0;
}
//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------





//--------------------------------------------------------------------------------------------------
// simulation
//--------------------------------------------------------------------------------------------------
void initSimulation() {
	//-----------------------------------------------------------
	// window
	//-----------------------------------------------------------
	if(!visualDebug)
		window = initOpenGL(domainSize[0], domainSize[1], "real-time FDTD", magnifier); // window as big as the domain [1 quad]
	else
		window = initOpenGL(domainSize[0]*NUM_OF_TIMESTEPS, domainSize[1]+clng, "real-time FDTD", magnifier); // window as big as the whole texture/full viewport!

	// check
	if(window == NULL)
		return ;

	// where shaders are
	const char* vertex_path_fbo      = {"shaders_FDTD/fbo_vs.glsl"}; // vertex shader of solver program
	const char* fragment_path_fbo    = {"shaders_FDTD/fbo_fs.glsl"}; // fragment shader of solver program
	const char* vertex_path_render   = {"shaders_FDTD/render_vs.glsl"}; // vertex shader of render program
	const char* fragment_path_render = {"shaders_FDTD/render_fs.glsl"}; // fragment shader of render program



	//-----------------------------------------------------------
	// FBO shader program [solver]
	//-----------------------------------------------------------
	if(!loadShaderProgram(vertex_path_fbo, fragment_path_fbo, vs_fbo, fs_fbo, shader_program_fbo))
		return;



	//-----------------------------------------------------------
	// screen shader program [render]
	//-----------------------------------------------------------
	if(!loadShaderProgram(vertex_path_render, fragment_path_render, vs_render, fs_render, shader_program_render))
		return;



	//-----------------------------------------------------------
	// vertices and their attributes
	//-----------------------------------------------------------
	int numOfAttrElementsPerVertex = 12; // each vertex comes with a set of info, regarding its position and the texture coordinates it can have access to
	// these data will be passed to the shader every time there is a draw command, but grouped as attribute arrays
	// first we define all the elements of these arrays as a long list
	// later we will group them
	// the elements' list:
	// 2 elements that describe the position of the vertex on the viewport at step N+1 [current quad]
	// 		pos x y
	// 2 elements that describe the coordinate on the texture associated to the vertex, but at step N [previous quad]
	// 		texture coordinate central x y
	// 8 elements that describe the neighbor texture coordinates associated to the vertex, but at step N [previous quad]
	// 		texture coordinate left x y
	// 		texture coordinate up x y
	// 		texture coordinate right x y
	// 		texture coordinate down x y

	// we draw quads, each composed of 4 vertices [each coming with the aforementioned 12 elements]
	int numOfVerticesPerQuad = 4; // vertices are grouped in quartets, in a VERY SPECIFIC order:
	// bottom left
	// top left
	// bottom right
	// top right
	// using this order we can draw a quad passing ONLY 4 VERTICES
	// any other order would not work and would oblige us to use 6 VERTICES!
	// this is really important to optimize the drawing cycle


	// we're gonna set the texture coordinates, so let's calculate how big the texture needs to be
	textureWidth  = domainSize[0]*NUM_OF_TIMESTEPS; // the texture will contain the 2 main quads [quad0 and quad1] side by side
	textureHeight = domainSize[1]+clng; // the texture will stack the main quads, an empty isolation row and the audio row [quad2] ---> see layout diagram towards the top of this file

	// now we fill the elements required per each vertex [which later will become attributes]
	// the frame of reference for positions is in the center of the texture and coordinates go from -1.0 to 1.0, on both axis
	// the frame of reference for texture coords is in the bottom left corner of the texture and coordinates go from 0.0 to 1.0, on both axis
	// it's madness, i know, but it's OpenGL

	// these are the deltas to compute the texture coordinates of neighbors [tex left, tex up, tex, right, tex down] AKA width and height of fragments
	float dx = 1.0/(float)textureWidth;
	float dy = 1.0/(float)textureHeight; // this is also combined with clng to make space for audio row and isolation row

	// this is the delta to compute the vertex y positions so that we have space for audio row and isolation row
	float dy_v = 2.0/(float)textureHeight;

	float all_attributes[] = {
		// quad0 [left quadrant]
		// 4 vertices
		// pos N+1/-1		tex C coord N		tex L coord N		tex U coord N		tex R coord N		tex D coord N
		-1, -1,				0.5f, 0,			0.5f-dx, 0,			0.5f, 0+dy,			0.5f+dx, 0,			0.5f, 0-dy,         // bottom left
		-1, 1-clng*dy_v,	0.5f, 1-clng*dy,	0.5f-dx, 1-clng*dy,	0.5f, 1+dy-clng*dy,	0.5f+dx, 1-clng*dy,	0.5f, 1-dy-clng*dy, // top left [leaving space for clng]
		0, -1,				1.0f, 0,			1-dx,    0,			1,    0+dy,			1+dx,    0,			1, 	  0-dy,         // bottom right
		0, 1-clng*dy_v,		1.0f, 1-clng*dy,	1-dx,    1-clng*dy,	1,    1+dy-clng*dy,	1+dx,    1-clng*dy,	1, 	  1-dy-clng*dy, // top right [leaving space for clng]

		// quad1 [right quadrant]
		// 4 vertices
		// pos N+1/-1		tex C coord N		tex L coord N		tex U coord N		tex R coord N		tex D coord N
		0, -1,				0,    0,			0-dx,    0,			0, 	  0+dy,			0+dx,    0,			0,    0-dy,         // bottom left
		0, 1-clng*dy_v,	    0,    1-clng*dy,	0-dx, 	 1-clng*dy,	0,    1+dy-clng*dy,	0+dx,    1-clng*dy,	0,    1-dy-clng*dy, // top left [leaving space for clng]
		1, -1,				0.5f, 0,			0.5f-dx, 0,			0.5f, 0+dy,			0.5f+dx, 0,			0.5f, 0-dy,         // bottom right
		1, 1-clng*dy_v,		0.5f, 1-clng*dy,	0.5f-dx, 1-clng*dy,	0.5f, 1+dy-clng*dy,	0.5f+dx, 1-clng*dy,	0.5f, 1-dy-clng*dy, // top right [leaving space for clng]

		// quad2 [audio quadrant]
		// 4 vertices
		// pos [no concept of time step]		tex C coords are the only ones required...
		-1, 1-dy_v,			0,    1-dy,			0,    0,			0,    0,			0,    0,			0,    0, // bottom left [1 pixel below top]
		-1, 1,	    		0,    1,			0,    0,			0,    0,			0,    0,			0,    0, // top left
		1, 1-dy_v,			1,    1-dy,			0,    0,			0,    0,			0,    0,			0,    0, // bottom right [1 pixel below top]
		1, 1,				1,    1,			0,    0,			0,    0,			0,    0,			0,    0, // top right
	};
	// the cool thing is that we give elements for only the extremes of each quad, which will be than correctly replicated
	// for any pixel that lies inside these extremes!

	
	// this is list of vertices we're gonna draw
	// since we'll work on only one quad at the time, we organized their vertices in 2 separate arrays

	// quad0 is composed of vertices 0 to 3
	vertices[0][0] = 0;					 // index of first vertex
	vertices[0][1] = numOfVerticesPerQuad; // number of vertices

	// quad1 is composed of vertices 4 to 7
	vertices[1][0] = 4;
	vertices[1][1] = numOfVerticesPerQuad;

	// quad2 is composed of vertices 8 to 11
	vertices[2][0] = 8;
	vertices[2][1] = numOfVerticesPerQuad;

	quad = 0; // we start to draw from quad 0, left



	//-----------------------------------------------------------
	// Vertex Buffer Object, to pass data to graphics card [in our case, they're positions]
	//-----------------------------------------------------------
	setUpVbo(sizeof (all_attributes), all_attributes, vbo);



	//-----------------------------------------------------------
	// Vertex Attribute Object, tells the shader how to interpret the VBO's content [vertices and in attributes]
	//-----------------------------------------------------------
	setUpVao(vbo, vao);

	// we pack 2 coordinates [vec2 + vec2] in each attribute [fast]
	int numOfElementsPerAttr = 4; // so each attribute will have 4 elements [vec4], taken from the vertices data structure!


	// we need 3 attributes to group 12 elements in quartets
	// all these 3 attributes must be declared at least in one of the shaders in the program

	// we search for them in the FBO, cos it uses all of them [the render shader uses only 1]

	// per each attribute we go through all the vertices data structure and extract the 12 elements that we need
	// grouping them in 3 vec4

	// 0 contains pos [vec2] and texc [vec2]
	GLint pos_and_texc_loc = glGetAttribLocation(shader_program_fbo, "pos_and_texc");
	glEnableVertexAttribArray(pos_and_texc_loc);
	//                     attribute location   4 float elements                           access stride between vertices             elements to skip
	glVertexAttribPointer (pos_and_texc_loc, numOfElementsPerAttr, GL_FLOAT, GL_FALSE, numOfAttrElementsPerVertex * sizeof(GLfloat), (void*)(0*numOfElementsPerAttr * sizeof(GLfloat)));

	// 1 contains texl [vec2] and texu [vec2]
	GLint texl_and_texu_loc = glGetAttribLocation(shader_program_fbo, "texl_and_texu");
	glEnableVertexAttribArray(texl_and_texu_loc);
	glVertexAttribPointer (texl_and_texu_loc, numOfElementsPerAttr, GL_FLOAT, GL_FALSE, numOfAttrElementsPerVertex * sizeof(GLfloat), (void*)(1*numOfElementsPerAttr * sizeof(GLfloat)));

	// 2 contains texr [vec2] and texrd [vec2]
	GLint texr_and_texd_loc = glGetAttribLocation(shader_program_fbo, "texr_and_texd");
	glEnableVertexAttribArray(texr_and_texd_loc);
	glVertexAttribPointer (texr_and_texd_loc, numOfElementsPerAttr, GL_FLOAT, GL_FALSE, numOfAttrElementsPerVertex * sizeof(GLfloat), (void*)(2*numOfElementsPerAttr * sizeof(GLfloat)));

	// the attributes are shared between the 2 programs, so once they are successfully initialized
	// the render shader will automatically have its own subset working



	//--------------------------------------------------------
	// texture pixels
	//--------------------------------------------------------
	// texturePixels contains all the points [2 quads], starting from left bottom corner
	float *texturePixels = new float[textureWidth*textureHeight*4]; // 4 channels per pixel, RGBA
	// R channel contains current pressure value
	// G channel contains previous pressure value
	// B channel defines if the point is boundary [0] or regular [1] --> beta term
	// A channel defines if the point receives external excitation [1] or not [0]

	// wipe all pixels
	memset(texturePixels, 0, sizeof(float)*textureWidth*textureHeight*4);


	// domain points' types [same for each quad], from left bottom corner
	float **pointType[3];
	pointType[0] = new float *[domainSize[0]]; // beta
	pointType[1] = new float *[domainSize[0]]; // excitation
	for(int i=0; i<domainSize[0]; i++) {
		pointType[0][i] = new float[domainSize[1]];
		pointType[1][i] = new float[domainSize[1]];

		// we init them
		for(int j=0; j<domainSize[1]; j++) {
			pointType[0][i][j] = 1; // regular point beta
			pointType[1][i][j] = 0; // no excitation

			// we also place the excitation point
			if( (i==excitationPos[0]) && (j==excitationPos[1]) )
				pointType[1][i][j] = 1; // excitation
		}
	}

	// then we add two columns of boundary points, at right and left of domain
	for(int i=0; i<domainSize[0]; i++) {
		pointType[0][i][0] = 0; // we only change beta, the transmission value
		pointType[0][i][domainSize[1]-1] = 0;
	}
	// and finally we add two boundary rows, at top and bottom of domain [they intersect with the columns on the corners]
	for(int j=1; j<domainSize[1]-1; j++) {
		pointType[0][0][j] = 0;
		pointType[0][domainSize[0]-1][j] = 0;
	}


	// copy domain points' types in texture pixels [in both quads]
	for(int i=0; i<textureWidth; i++) {
		for(int j=0; j<textureHeight-clng; j++) {
			// first quad
			if(i<domainSize[0]) {
				texturePixels[(j*textureWidth+i)*4+2] = pointType[0][i][j]; // beta
				texturePixels[(j*textureWidth+i)*4+3] = pointType[1][i][j]; // excitation
			}
			// second quad
			else if(i>=domainSize[0]) {
				// we have to pick the same data as for first quad
				texturePixels[(j*textureWidth+i)*4+2] = pointType[0][i-domainSize[0]][j]; // beta
				texturePixels[(j*textureWidth+i)*4+3] = pointType[1][i-domainSize[0]][j]; // excitation
			}
		}
	}

	// clean up
	for(int i=0; i<domainSize[0]; i++) {
		delete[] pointType[0][i];
		delete[] pointType[1][i];
	}
	delete[] pointType[0];
	delete[] pointType[1];



	//--------------------------------------------------------
	// texture, same for the 2 programs [FBO and render] but used in different ways [FBO: input/output, render: input]
	//--------------------------------------------------------
	setUpTexture(textureWidth, textureHeight, texturePixels, texture);

	// clean up
	delete[] texturePixels;



	//--------------------------------------------------------
	// fbo and draw buffer
	//--------------------------------------------------------

	// the FBO program will use the framebuffer to write on the texture
	// the render program use the read from the texture written by the FBO
	setUpFbo(texture, fbo);



	//--------------------------------------------------------
	// pbo, to read from texture
	//--------------------------------------------------------
	setUpPbo(sizeof(float)*buffer_size, pbo);



	//--------------------------------------------------------
	// uniforms, the shader vars that we can update from the CPU
	//--------------------------------------------------------

	// some values useful for uniform

	// width and height of each fragment...we have calculated them already, to compute texture coordinates
	// we change names to make it easier to grasp
	deltaCoordX = dx;
	deltaCoordY = dy;

	// here we compute the texture coordinates of the listener point
	// so that the shader can recognize it
	float listenerFragCoord[2][2];
	// quad 1      reads audio       from quad 0
	listenerFragCoord[1][0] = (float)(listenerPos[0]+0.5) / (float)textureWidth;
	listenerFragCoord[1][1] = (float)(listenerPos[1]+0.5) / (float)textureHeight;
	// quad 0      reads audio       from quad 1
	listenerFragCoord[0][0] = (float)(listenerPos[0]+0.5+domainSize[0]) / (float)textureWidth;
	listenerFragCoord[0][1] = (float)(listenerPos[1]+0.5) / (float)textureHeight;
	// 0.5 is needed for a perfect match of the fragment we want to sample, don't ask too many questions...it's dangerous |:


	// for FBO shader
	glUseProgram(shader_program_fbo);

	//-------------------------------------------
	// static uniforms, that we update once, now

	// fragments' width
	GLuint deltaCoordX_loc =  glGetUniformLocation(shader_program_fbo, "deltaCoordX");
	glUniform1f(deltaCoordX_loc, deltaCoordX);

	// listener's fragment coordinates [in both simulation quads]
	GLuint listenerFragCoord_loc[2];
	char name[22];
	for(int i=0; i<NUM_OF_TIMESTEPS; i++) {
		sprintf(name, "listenerFragCoord[%d]", i);
		listenerFragCoord_loc[i] = glGetUniformLocation(shader_program_fbo, name);
		glUniform2f(listenerFragCoord_loc[i], listenerFragCoord[i][0], listenerFragCoord[i][1]);
	}

	//-------------------------------------------
	// dynamic uniforms, which we will be updated within the simulation loop

	// to tell the shader what to do
	state_loc = glGetUniformLocation(shader_program_fbo, "state");

	// to pass excitation to excitation points
	excitation_loc = glGetUniformLocation(shader_program_fbo, "excitation");

	wrCoord_loc = glGetUniformLocation(shader_program_fbo, "wrCoord");

	//-------------------------------------------
	// texture

	// we say that the uniform called inOutTexture [of type sampler2D] in the FBO shader refers to the texture number zero that we created and initialized
	// the FBO will automatically write on it anyway, but doing so we can ALSO READ from it, which is fundamental
	glUniform1i(glGetUniformLocation(shader_program_fbo, "inOutTexture"), 0); // we do this only once at the beginning



	// for render shader
	glUseProgram(shader_program_render);

	//-------------------------------------------
	// static uniforms
	GLuint deltaCoord_loc_render =  glGetUniformLocation(shader_program_render, "deltaCoord");
	glUniform2f(deltaCoord_loc_render, deltaCoordX, deltaCoordY);

	// listener's fragment coordinates [only quad0, cos we only render that one]
	GLuint listenerFragCoord_loc_render = glGetUniformLocation(shader_program_render, "listenerFragCoord");
	glUniform2f(listenerFragCoord_loc_render, listenerFragCoord[0][0], listenerFragCoord[0][1]);

	//-------------------------------------------
	// texture
	// then we say that the uniform called inputTexture [of type sampler2D] in the render shader refers to the same texture too
	glUniform1i(glGetUniformLocation(shader_program_render, "inputTexture"), 0);



	// done, we'll re-activate the proper shader program later
	glUseProgram(0);

}

void runSimulation() {
	int sampleCnt = 0;

	int bufferNum = totalSampleNum/buffer_size; // how many times we call the buffer loop to synthesize more or less all the requested samples

	// allocate a global audio buffer big enough to contain all the samples synthesized during the simulation
	totalAudioBuffer = new float[bufferNum*buffer_size];

	for(int i=0; i<bufferNum; i++) {
		// switch to FBO shader
		glUseProgram(shader_program_fbo); // activate FBO shader program
		glBindFramebuffer(GL_FRAMEBUFFER, fbo); // render to our framebuffer!
		glViewport(0, 0, textureWidth, textureHeight); // full viewport, to give access to all texture

		// this simulates the buffer callback of a standard real-time audio application

		// repeat per each sample, ping-ponging quad
		for(int n=0; n<buffer_size; n++) {
			// pass next excitation value
			glUniform1f(excitation_loc, excitation);

			// simulation step [state0 or state2]
			state = quad*2;
			glUniform1i(state_loc, state);
			glDrawArrays(GL_TRIANGLE_STRIP, vertices[quad][0], vertices[quad][1]); // draw quad0 or quad1

			// audio step [state1 or state3]
			glUniform2fv(wrCoord_loc, 1, wrCoord); // fragment and channel
			glUniform1i(state_loc, state+1);
			glDrawArrays(GL_TRIANGLE_STRIP, vertices[quad2][0], vertices[quad2][1]); // draw quad2, appending audio from quad1 or from quad0

			// prepare next cycle
			quad = 1-quad;
			wrCoord[1] = int(wrCoord[1]+1)%4;
			if(wrCoord[1]==0)
				wrCoord[0]+=deltaCoordX;

			// next excitation
			if(++sampleCnt == excitationDt) {
				excitation = maxExcitation; // strike!
				sampleCnt = 0;
			}
			else
				excitation = 0;

			// re-sync all parallel GPU threads [this is also called implicitly when buffers are swapped]
			glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);
		}
		// reset write coordinates at the end of each buffer
		wrCoord[0] = 0;
		wrCoord[1] = 0;



		// retrieve all the pixels [audio samples] stored in quad2 ---> intrinsically uses pbo!
		glReadPixels(0, textureHeight-1, buffer_size/4, 1, GL_RGBA, GL_FLOAT, 0); // quad2 is a single row on top of the texture, with 4 samples in each pixel
		float *sampleBuffer = (float*)glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY); // we need to do this...
		glUnmapBuffer(GL_PIXEL_PACK_BUFFER); //...and this, to have a physical copy of the samples in CPU memory
		// now we can line them up in the big audio buffer
		memcpy(totalAudioBuffer+i*(buffer_size), sampleBuffer, buffer_size*sizeof(float)); // buffer by buffer



		// switch to render shader, we display the result once every buffer, no need for super high frame rate!
		glUseProgram(shader_program_render); // activate render shader program
		glBindFramebuffer(GL_FRAMEBUFFER, 0); // disable FBO to render to screen
		// we set the viewport as big as the texture [twice as long a window + ceiling], so a single quad occupies the whole window!
		glViewport(0, 0, textureWidth*magnifier, textureHeight*magnifier); // we also magnify, to see better (:
		
		// always draw the quad0, cos we do not care if it is not the latest time step...this update is anyway much faster than most display's refresh rate!
		glDrawArrays(GL_TRIANGLE_STRIP, vertices[quad0][0], vertices[quad0][1]);

		// if we are in visual debug the window is instead as big as the whole texture/viewport, so we draw everything (:
		if(visualDebug) {
			glDrawArrays(GL_TRIANGLE_STRIP, vertices[quad1][0], vertices[quad1][1]);
			glDrawArrays(GL_TRIANGLE_STRIP, vertices[quad2][0], vertices[quad2][1]);
		}

		// update other events like input handling...in the "unlikely" case we want to do some interaction
		glfwPollEvents();
		// put the stuff we've been drawing onto the buffer that will be displayed
		glfwSwapBuffers(window);

		// catch key press for esc button
		if(GLFW_PRESS == glfwGetKey(window, GLFW_KEY_ESCAPE))
			break; // premature stop!
	}
}
//--------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------



int main(int argc, char *argv[]) {
	parseArguments(argc, argv);

	// check if we have enough space in the texture to save all the audio
	if(domainSize[0]*NUM_OF_TIMESTEPS*4 < buffer_size) {
		perror("The buffer size is bigger than what the texture can contain,\n");
		perror("so the audio that you'll get from the simulation will be unpredictable...how about increasing the domain size?\n");
		perror("I'm just saying...\n");
	}

	initSimulation();

	runSimulation();

	// do what you want with the full audio buffer, but do it now

	//...

	delete[] totalAudioBuffer; // ...cos i am wiping it here

	// gently deallocate and close everything
	glDeleteTextures(1, &texture);
	glDeleteBuffers(1, &pbo);
	glDeleteFramebuffers(1, &fbo);
	glDeleteVertexArrays(1, &vao);
	glDeleteBuffers(1, &vbo);
	glDeleteProgram(shader_program_render);
	glDeleteShader(fs_render);
	glDeleteShader(vs_render);
	glDeleteProgram(shader_program_fbo);
	glDeleteShader(fs_fbo);
	glDeleteShader(vs_fbo);

	// close GL window, context and any other GLFW resources
	glfwSetWindowShouldClose (window, 1);
	glfwTerminate();

	printf("Program ended\nBye bye_\n");
	return 0;
}
