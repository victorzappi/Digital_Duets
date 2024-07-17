/*
 * GlEngine.cpp
 *
 *  Created on: 2016-12-29
 *      Author: Victor Zappi
 */

#include "GlEngine.h"
#include "openGL_functions.h"

//---------------------------------------------------
// editable parameters
//---------------------------------------------------
int domainSize[2] = {80, 80};
float magnifier = 10;
int excitationCoord[2] = {domainSize[0]/2, domainSize[1]/2};
float maxExcitation = 10; // dimensionless
float excitationFreq = 44100; // samples
//---------------------------------------------------
//---------------------------------------------------





#define NUM_OF_QUADS 2

// working vars
int verticesToDraw[NUM_OF_QUADS][2]; //per each quad, index of first vertex to draw and total number of vertices that belong to that quad [always 4]
GLFWwindow* window;
GLuint shader_program_render = 0;
GLuint shader_program_fbo = 0;
int textureWidth;
int textureHeight;
GLuint fbo = 0;
int currentQuad; // 0 [left] or 1 [right]
GLuint state_location;
int state;
// state_0: draw left quad
// state_1: read audio from right quad [cos left might not be ready yet]
// state_2: draw right quad
// state_3: read audio from left quad [cos right might not be ready yet]
GLuint excitationInput_loc;
float excitation = maxExcitation; // we init excitaion with a big value

GLuint vs_fbo = 0;
GLuint fs_fbo = 0;
GLuint vs_render = 0;
GLuint fs_render = 0;
GLuint vbo = 0;
GLuint vao = 0;
GLuint texture;
void GlEngine::initRender() {

	//-----------------------------------------------------------
	// window
	//-----------------------------------------------------------
	window = initOpenGL(domainSize[0], domainSize[1], "real-time FDTD", magnifier);

	// check
	if(window == NULL)
		return ;

	// where shaders are
	const char* vertex_path_fbo      = {"shaders_FDTD/fbo_vs.glsl"};
	const char* fragment_path_fbo    = {"shaders_FDTD/fbo_fs.glsl"};
	const char* vertex_path_render   = {"shaders_FDTD/render_vs.glsl"};
	const char* fragment_path_render = {"shaders_FDTD/render_fs.glsl"};

	//-----------------------------------------------------------
	// FBO shader program
	//-----------------------------------------------------------
	if(!loadShaderProgram(vertex_path_fbo, fragment_path_fbo, vs_fbo, fs_fbo, shader_program_fbo))
		return;

	//-----------------------------------------------------------
	// screen shader program
	//-----------------------------------------------------------
	if(!loadShaderProgram(vertex_path_render, fragment_path_render, vs_render, fs_render, shader_program_render))
		return;


	//-----------------------------------------------------------
	// vertices and their attribute
	//-----------------------------------------------------------

	int numOfAttrElementsPerVertex = 12; // each vertex comes with a set of info, regarding its position and the texture coordinates it can have access to
	// these data will be passed to the shader everytime there is a draw command, but grouped as attribute arrays
	// first we define all the elements of these arrays as a long list
	// later we will group them
	// the elements' list:
	// 2 elements that describe the position of the vertex on the viewport at state N+1 [current quad]
	// 		pos x y
	// 2 elements that describe the coordinate on the texture associated to the vertex, but at state N [previous quad]
	// 		texture coordinate central x y
	// 8 elements that describe the neighbor texture coordinates associated to the vertex, but at state N [previous quad]
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

	// these are the deltas to compute the texture coordinates of neighbors [tex left, tex up, tex, right, tex down]
	float dx = 1.0/(domainSize[0]*NUM_OF_QUADS); // we're gonna work with a big texture that contains the 2 quads side by side
	float dy = 1.0/domainSize[1];

	// now we fill the elements required per each vertex [which later will become attributes]
	// the frame of reference for positions are in the center of the texture and coordinates go from -1.0 to 1.0, on both axis
	// the frame of reference for texture coords are in the bottom left corner of the texture and coordinates go from 0.0 to 1.0, on both axis
	// it's madness, i know, but it's OpenGL
	float vertices[] = {
		// left quadrant
		//4 vertices
		// pos N+1/-1	tex C coord N	tex L coord N	tex U coord N	tex R coord N	tex D coord N
		-1, -1,			0.5f, 0,		0.5f-dx, 0,		0.5f, 0+dy,		0.5f+dx, 0,		0.5f, 0-dy, // bottom left
		-1, 1,			0.5f, 1,		0.5f-dx, 1,		0.5f, 1+dy,		0.5f+dx, 1,		0.5f, 1-dy, // top left
		0, -1,			1.0f, 0,		1-dx,    0,		1,    0+dy,		1+dx,    0,		1, 	  0-dy, // bottom right
		0, 1,			1.0f, 1,		1-dx,    1,		1,    1+dy,		1+dx,    1,		1, 	  1-dy, // top right

		// left quadrant
		//4 vertices
		// pos N+1/-1	tex C coord N	tex L coord N	tex U coord N	tex R coord N	tex D coord N
		0, -1,			0,    0,		0-dx,    0,		0, 	  0+dy,		0+dx,    0,		0,    0-dy, // bottom left
		0, 1,	    	0,    1,		0-dx, 	 1,		0,    1+dy,		0+dx,    1,		0,    1-dy, // top left
		1, -1,			0.5f, 0,		0.5f-dx, 0,		0.5f, 0+dy,		0.5f+dx, 0,		0.5f, 0-dy, // bottom right
		1, 1,			0.5f, 1,		0.5f-dx, 1,		0.5f, 1+dy,		0.5f+dx, 1,		0.5f, 1-dy, // top right
	};
	// the cool thing is that we give elements for only the extremes of the viewport/texture, which will be than correctly replicated
	// for any pixel that lies inside these extremes!

	// since we'll work on only one quad at the time, we organized their vertices in 2 separate arrays

	// left quad is composed of vertices 0 to 3
	verticesToDraw[0][0] = 0;					 // index of first vertex
	verticesToDraw[0][1] = numOfVerticesPerQuad; // number of vertices

	// left quad is composed of vertices 4 to 7
	verticesToDraw[1][0] = 4;
	verticesToDraw[1][1] = numOfVerticesPerQuad;

	currentQuad = 0; // we start to draw from quad 0, left


	//-----------------------------------------------------------
	// Vertex Buffer Object, to pass data to graphics card [in our case, they're positions]
	//-----------------------------------------------------------
	setUpVbo(sizeof (vertices), vertices, vbo);


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

	// then attributes are shared between the 2 programs, so once they are successfully initialized
	// the render shader will automatically have its own subset working


	//--------------------------------------------------------
	// texture pixels
	//--------------------------------------------------------
	textureWidth  = domainSize[0]*NUM_OF_QUADS; // the texture will contain 2 quads side by side
	textureHeight = domainSize[1];
	// texturePixels contains all the cells [2 quads], starting from left bottom corner
	float *texturePixels = new float[textureWidth*textureHeight*4]; // 4 channels per pixel, RGBA
	// R channel contains next pressure value
	// G channel contains current pressure value
	// B channel defines if the cell is reflective [0] or transmissive [1] --> beta term
	// A channel defines if the cell receives external excitation [1] or not [0]

	// domain cells' types [same for each quad], from left bottom corner
	float **cellType[3];
	cellType[0] = new float *[domainSize[0]]; // beta
	cellType[1] = new float *[domainSize[0]]; // excitation
	for(int i=0; i<domainSize[0]; i++) {
		cellType[0][i] = new float[domainSize[1]];
		cellType[1][i] = new float[domainSize[1]];

		// we init them
		for(int j=0; j<domainSize[1]; j++) {
			cellType[0][i][j] = 1; // transmittive beta
			cellType[1][i][j] = 0; // no excitation

			// we also place the excitation cell
			if( (i==excitationCoord[0]) && (j==excitationCoord[1]) )
				cellType[1][i][j] = 1; // excitation
		}
	}

	// then we add two columns of reflective cells, at right and left of domain
	for(int i=0; i<domainSize[0]; i++) {
		cellType[0][i][0] = 0; // we only change beta, the transmission value
		cellType[0][i][domainSize[1]-1] = 0;
	}
	// and finally we add two reflective rows, at top and bottom of domain [they intersect with the columns on the corners]
	for(int j=1; j<domainSize[1]-1; j++) {
		cellType[0][0][j] = 0;
		cellType[0][domainSize[0]-1][j] = 0;
	}

	// wipe all pixels
	memset(texturePixels, 0, sizeof(float)*textureWidth*textureHeight*4);

	// copy domain cells' types in texture pixels [in both quads]
	for(int i=0; i<textureWidth; i++) {
		for(int j=0; j<textureHeight; j++) {
			// first quad
			if(i<domainSize[0]) {
				texturePixels[(j*textureWidth+i)*4+2] = cellType[0][i][j]; // beta
				texturePixels[(j*textureWidth+i)*4+3] = cellType[1][i][j]; // excitation
			}
			// second quad
			else if(i>=domainSize[0]) {
				// we have to pick the same data as for first quad
				texturePixels[(j*textureWidth+i)*4+2] = cellType[0][i-domainSize[0]][j]; // beta
				texturePixels[(j*textureWidth+i)*4+3] = cellType[1][i-domainSize[0]][j]; // excitation
			}
		}
	}

	// clean up
	for(int i=0; i<domainSize[0]; i++) {
		delete cellType[0][i];
		delete cellType[1][i];
	}
	delete[] cellType[0];
	delete[] cellType[1];



	//--------------------------------------------------------
	// texture, same for the 2 programs [FBO and render] but used in different ways [FBO: input/output, render: input]
	//--------------------------------------------------------
	setUpTexture(textureWidth, textureHeight, texturePixels, texture);

	// clean up
	delete texturePixels;


	//--------------------------------------------------------
	// fbo and draw buffer
	//--------------------------------------------------------

    // the FBO program will use the framebuffer to write on the texture
	// the render program use the read from the texture written by the FBO
    fbo = 0;
	setUpFbo(texture, fbo);


	//--------------------------------------------------------
	// uniforms, the shader vars that we can update from the CPU
	//--------------------------------------------------------
	glUseProgram(shader_program_fbo);

	// we say that the uniform called inOutTexture [of type sampler2D] in the FBO shader refers to the texture number zero that we created and initialized
	// the FBO will automatically write on it anyway, but doing so we can ALSO READ from it, which is fundamental
	glUniform1i(glGetUniformLocation(shader_program_fbo, "inOutTexture"), 0); // we do this only once at the beginning

	// to tell the shader what to do
	state_location = glGetUniformLocation(shader_program_fbo, "state");

	// to pass excitation to excitation cells
	excitationInput_loc = glGetUniformLocation(shader_program_fbo, "excitationInput");

	glUseProgram(shader_program_render);

	// then we say that the uniform called inputTexture [of type sampler2D] in the render shader refers to the same texture too
	glUniform1i(glGetUniformLocation(shader_program_render, "inputTexture"), 0);

	glUseProgram(0);
}

void GlEngine::render(float sampleRate, int numChannels, int numSamples, float **sampleBuffers) {
	const int bits = GL_TEXTURE_FETCH_BARRIER_BIT; // needed down here for glMemoryBarrier()

	static int sampleCnt = 0;

	// FBO shader, we a simulation step every sample
	glUseProgram (shader_program_fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo); // render to our framebuffer!
	glViewport(0, 0, textureWidth, textureHeight); // to give access to all texture

	// repeat per each sample ping-ponging quad
	for(int i=0; i<numSamples; i++) {

		glUniform1f(excitationInput_loc, excitation);

		state = currentQuad*2; // either state_0 or state_1, for left or right quad
		glUniform1i(state_location, state); // draw current quad

		glDrawArrays ( GL_TRIANGLE_STRIP, verticesToDraw[currentQuad][0], verticesToDraw[currentQuad][1]); // verticesToDraw contains the vertices of both the quads!

		// next quad and excitation
		currentQuad = 1-currentQuad; // other quad next time
		if(++sampleCnt == excitationFreq) {
			excitation = maxExcitation; // we start with a big value then always pass zero [impulse]
			sampleCnt = 0;
		}
		else
			excitation = 0;

		glMemoryBarrier(bits); // to re-sync all our parallel GPU threads [this is also called implicitly when buffers are swapped]
	}


	// render shader, we display the result once every buffer, no need for super high frame rate!
	glUseProgram (shader_program_render);
	glBindFramebuffer(GL_FRAMEBUFFER, 0); // render to screen
	glViewport(0, /*(-audioRowH-ISOLATION_LAYER)*magnifier*/0, textureWidth*magnifier, textureHeight*magnifier); // we set the viewport as big as the texture [twice as long a window], so a quad occupies the whole window!
	glDrawArrays ( GL_TRIANGLE_STRIP, verticesToDraw[0][0], verticesToDraw[0][1]); // always draw the left quad, cos we do not care if we are one sample behind!


	// update other events like input handling
	glfwPollEvents ();
	// put the stuff we've been drawing onto the display
	glfwSwapBuffers (window);


	// catch key press for esc button
	if (GLFW_PRESS == glfwGetKey(window, GLFW_KEY_ESCAPE)) {
		glfwSetWindowShouldClose(window, 1); // close windows
		stopEngine(); // stop audio engine
	}
}


void GlEngine::cleanUpRender() {
	glDeleteTextures(1, &texture);
	glDeleteFramebuffers(1, &fbo);
	glDeleteVertexArrays(1, &vao);
	glDeleteBuffers(1, &vbo);
	glDeleteProgram(shader_program_render);
	glDeleteShader(fs_render);
	glDeleteShader(vs_render);
	glDeleteProgram(shader_program_fbo);
	glDeleteShader(fs_fbo);
	glDeleteShader(vs_fbo);

	glfwSetWindowShouldClose (window, 1);
	// close GL context and any other GLFW resources
	glfwTerminate();
}
