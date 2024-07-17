/*
 * HyperPixelDrawer.cpp
 *
 *  Created on: Mar 13, 2017
 *      Author: vic
 */

#include "HyperPixelDrawer.h"

enum operation_ {operation_write, operation_reset};

PixelDrawer_new::PixelDrawer_new(int w, int h, int wrChan, Shader &shader_fbo, float ***clearFrame,
								 int stateNum, int totalQuadNum) : pbo(texture), shaderFbo(shader_fbo) {
	shaders[0] = "";
	shaders[1] = "";

	operation_loc = -1;

	gl_width  = w;
	gl_height = h;

	writeChannels = wrChan;

	// to save a copy of the original image [will come handy for clear()]
	clearFramePixels = new GLfloat[gl_width*gl_height*writeChannels];

	numOfStates     = stateNum;
	numOfResetQuads = totalQuadNum-stateNum;

	// allocate and wipe
	framePixels = new GLfloat[gl_width*gl_height*writeChannels];
	memset(framePixels, 0, sizeof(GLfloat)*gl_width*gl_height*writeChannels);

	drawArraysMainQuads  = new int *[numOfStates];
	for(int i=0; i<numOfStates; i++)
		drawArraysMainQuads[i] = new int[2];
	drawArraysResetQuads = new int *[numOfResetQuads];
	for(int i=0; i<numOfResetQuads; i++)
		drawArraysResetQuads[i] = new int[2];
}

PixelDrawer_new::~PixelDrawer_new() {
	for(int i=0; i<numOfStates; i++)
		delete[] drawArraysMainQuads[i];
	delete[] drawArraysMainQuads;
	for(int i=0; i<numOfResetQuads; i++)
		delete[] drawArraysResetQuads[i];
	delete[] drawArraysResetQuads;

	delete[] framePixels;
	delete[] clearFramePixels;
}


//------------------------------------------------------------------
// protected methods
//------------------------------------------------------------------
bool PixelDrawer_new::initShaders(string shaderLocation) {
	shaders[0] = shaderLocation+"/pixelDrawer_vs.glsl";
	shaders[1] = shaderLocation+"/pixelDrawer_fs.glsl";
	const char* vertex_path_fbo   = {shaders[0].c_str()};
	const char* fragment_path_fbo = {shaders[1].c_str()};

	return shaderDraw.init(vertex_path_fbo, fragment_path_fbo);
}









//----------------------------------------------------------------------------------------------------------------------------------
HyperPixelDrawer::HyperPixelDrawer(int w, int h, int wrChan, Shader &shader_fbo, float ***clearFrame,
								   int stateNum, int totalQuadNum, int *cellTypes)
								   : PixelDrawer_new(w, h, wrChan, shader_fbo, clearFrame, stateNum, totalQuadNum) {

	excitationCellType = cellTypes[0];
	airCellType        = cellTypes[1];
	boundaryCellType   = cellTypes[2];

	// save an exact copy of the original image [will come handy for clear()]
	// BUT
	// then we will get rid of everything we don't care about to turn it in just an empty domain...
	for(int i=0; i<gl_height; i++) {
		for(int j=0; j<gl_width; j++) {
			clearFramePixels[i*writeChannels*gl_width+j*writeChannels]   = (GLfloat)clearFrame[j][i][1]; // area
			clearFramePixels[i*writeChannels*gl_width+j*writeChannels+1] = (GLfloat)clearFrame[j][i][2]; // boundary gain [gamma]
			clearFramePixels[i*writeChannels*gl_width+j*writeChannels+2] = (GLfloat)clearFrame[j][i][3]; // type
		}
	}
}

HyperPixelDrawer::~HyperPixelDrawer() {
}

int HyperPixelDrawer::init(GLint active_unit, string shaderLocation, int drawArraysMain[][2], int drawArraysReset[][2]) {
	//-----------------------------------------------------------
	// frame
	//-----------------------------------------------------------
	// first put the original domain in our frame data structure
	clear();
	// then we  get rid of everything we don't care about to turn it in just an empty domain
	for(int i=0; i<gl_height; i++) {
		for(int j=0; j<gl_width; j++) {
			clearFramePixels[i*writeChannels*gl_width+j*writeChannels]   = 0; // area = 0
			clearFramePixels[i*writeChannels*gl_width+j*writeChannels+1] = 1; // boundary gain [gamma] = 1 [clamped]
			// filter out types we are not interested in
			if(clearFramePixels[i*writeChannels*gl_width+j*writeChannels+2] == excitationCellType ||
		       clearFramePixels[i*writeChannels*gl_width+j*writeChannels+2] == boundaryCellType)
				clearFramePixels[i*writeChannels*gl_width+j*writeChannels+2] = airCellType;
		}
	}


	//-----------------------------------------------------------
	// textue
	//-----------------------------------------------------------
	GLint textureInternalFormat = GL_RGB32F;
	//texture.loadPixels(gl_width, gl_height, GL_R32F, GL_RED, GL_FLOAT, framePixels, active_unit);
	texture.loadPixels(gl_width, gl_height, textureInternalFormat, GL_RGB, GL_FLOAT, framePixels, active_unit);
	// verify type and format
	if(false) {
		printf("\nFormat check for: %d\n", textureInternalFormat);

		GLint format, type;

		glGetInternalformativ(GL_TEXTURE_2D, textureInternalFormat, GL_TEXTURE_IMAGE_FORMAT, 1, &format);
		glGetInternalformativ(GL_TEXTURE_2D, textureInternalFormat, GL_TEXTURE_IMAGE_TYPE, 1, &type);

		printf("\tGL_TEXTURE_2D ---> Preferred GL_TEXTURE_IMAGE_FORMAT: %X\n", format);
		printf("\tGL_TEXTURE_2D ---> Preferred GL_TEXTURE_IMAGE_TYPE: %X\n", type);

		glGetInternalformativ(GL_TEXTURE_2D, textureInternalFormat, GL_READ_PIXELS_FORMAT, 1, &format);
		glGetInternalformativ(GL_TEXTURE_2D, textureInternalFormat, GL_READ_PIXELS_TYPE, 1, &type);

		printf("\tGL_TEXTURE_2D ---> Preferred GL_READ_PIXELS_FORMAT: %X\n", format);
		printf("\tGL_TEXTURE_2D ---> Preferred GL_READ_PIXELS_TYPE: %X\n", type);

		glGetInternalformativ(GL_FRAMEBUFFER, textureInternalFormat, GL_TEXTURE_IMAGE_FORMAT, 1, &format);
		glGetInternalformativ(GL_FRAMEBUFFER, textureInternalFormat, GL_TEXTURE_IMAGE_TYPE, 1, &type);

		printf("\tGL_FRAMEBUFFER ---> Preferred GL_TEXTURE_IMAGE_FORMAT: %X\n", format);
		printf("\tGL_FRAMEBUFFER ---> Preferred GL_TEXTURE_IMAGE_TYPE: %X\n", type);

		glGetInternalformativ(GL_FRAMEBUFFER, textureInternalFormat, GL_READ_PIXELS_FORMAT, 1, &format);
		glGetInternalformativ(GL_FRAMEBUFFER, textureInternalFormat, GL_READ_PIXELS_TYPE, 1, &type);

		printf("\tGL_FRAMEBUFFER ---> Preferred GL_READ_PIXELS_FORMAT: %X\n", format);
		printf("\tGL_FRAMEBUFFER ---> Preferred GL_READ_PIXELS_TYPE: %X\n", type);
	}


	//-----------------------------------------------------------
	// shader program
	//-----------------------------------------------------------
	initShaders(shaderLocation);

	shaderDraw.use();

	// these is the drawing textures we will use in the shader to update the main texture [color attachment of FBO]
	glUniform1i(shaderDraw.getUniformLocation("drawingTexture"), active_unit - GL_TEXTURE0);

	operation_loc = shaderDraw.getUniformLocation("operation");
	glUniform1i(operation_loc, operation_write); // most likely case
	if(operation_loc == -1)
		printf("operation uniform can't be found in pixel drawer fragment shader program\n");


	shaderFbo.use();


	//-----------------------------------------------------------
	// pbo
	//-----------------------------------------------------------
	pbo.init(gl_width, gl_height, framePixels, writeChannels);

	// copy vertices indices and strides for the 4 drawing quads and 4 excitation quads and 4 excitation follow up quads and audio quad
	for(int i=0; i<numOfStates; i++) {
		drawArraysMainQuads[i][0] = drawArraysMain[i][0];
		drawArraysMainQuads[i][1] = drawArraysMain[i][1];
	}
	for(int i=0; i<numOfResetQuads; i++) {
		drawArraysResetQuads[i][0] = drawArraysReset[i][0];
		drawArraysResetQuads[i][1] = drawArraysReset[i][1];
	}

	int retval = pbo.trigger(); // test if working

	// apparently this does not happen anymore since we moved from a char PBO to a float PBO...
	if(retval == GL_INVALID_OPERATION) {
		printf("\t***Heads up, we might have stumbled upon an OpenGL bug here!***\n");
		printf("\tThe pbo that is supposed to update the pixels can't be set up.\n");
		printf("\tBut don't panic! Try to slightly increase or decrease domain width [by 1 or 2 pixels].\n");
		printf("\tSorry bout that...it's not totally my fault\n");
	}

	return retval;
}

GLfloat HyperPixelDrawer::getCellType(int x, int y) {
	if (x < 0 || x >= gl_width || y < 0 || y >= gl_height)
		return -1;

	return framePixels[y*writeChannels*gl_width+x*writeChannels + 2];
}

GLfloat HyperPixelDrawer::getCellArea(int x, int y) {
	if (x < 0 || x >= gl_width || y < 0 || y >= gl_height)
		return -1;

	return framePixels[y*writeChannels*gl_width+x*writeChannels];
}

int HyperPixelDrawer::setCellType(int x, int y, GLfloat type) {
	//printf("(%d, %d)", x, y);
	if (x < 0 || x >= gl_width || y < 0 || y >= gl_height)
		return -1;

	int index = y*writeChannels*gl_width+x*writeChannels + 2;

	// nothing to change
	if(framePixels[index] == type)
		return 1;

	framePixels[index] = type;

	//printf(" = %f\n", type);

	return 0;
}

int HyperPixelDrawer::setCellArea(int x, int y, GLfloat area) {
	//printf("(%d, %d)", x, y);
	if (x < 0 || x >= gl_width || y < 0 || y >= gl_height)
		return -1;

	int index = y*writeChannels*gl_width+x*writeChannels;

	// nothing to change
	if(framePixels[index] == area)
		return 1;

	framePixels[index] = area;

	//printf(" = %f\n", area);

	return 0;
}

int HyperPixelDrawer::setCell(int x, int y, GLfloat area, GLfloat type, GLfloat bgain) {
	//printf("(%d, %d)", x, y);
	if (x < 0 || x >= gl_width || y < 0 || y >= gl_height)
		return -1;

	int index = y*writeChannels*gl_width+x*writeChannels;

	framePixels[index]   = area;
	framePixels[index+2] = type;
	if(bgain==-1)
		return 0;
	// in case we also want to change boundary gain...possibly on boundary cells, but whatever
	framePixels[index+1] = bgain;
	return 0;

}

int HyperPixelDrawer::resetCellType(int x, int y) {
	if (x < 0 || x >= gl_width || y < 0 || y >= gl_height)
		return -1;

	int index = y*writeChannels*gl_width+x*writeChannels + 2;

	// nothing to change
	if(framePixels[index] == clearFramePixels[index])
		return 1;

	framePixels[index] = clearFramePixels[index]; // back to original value

	return 0;
}

void HyperPixelDrawer::update(int state) {
	pbo.trigger();

	shaderDraw.use();

	glUniform1i(operation_loc, operation_write); // drawing

	// don't touch pressure, stored in first channel of each fragment
	glColorMask(GL_FALSE, GL_TRUE, GL_TRUE, GL_TRUE);
	glDrawArrays(GL_TRIANGLE_STRIP, drawArraysMainQuads[state][0], drawArraysMainQuads[state][1]); // drawing quads

	glTextureBarrierNV();

	shaderFbo.use();
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE); // FBO needs full write access
}

void HyperPixelDrawer::reset() {
	shaderDraw.use();

	glUniform1i(operation_loc, operation_write); // back to drawing

	// first put to zero in first channel of fragment [pressure], as hardcoded in the shader
	glColorMask(GL_TRUE, GL_FALSE, GL_FALSE, GL_FALSE);
	for(int i=0; i<numOfStates; i++)
		glDrawArrays(GL_TRIANGLE_STRIP, drawArraysMainQuads[i][0], drawArraysMainQuads[i][1]); // main quads

	glTextureBarrierNV();

	// then put all channels to zero in the audio quad [only other quad in this case]
	glUniform1i(operation_loc, operation_reset);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE); // need full write access
	for(int i=0; i<numOfResetQuads; i++)
		glDrawArrays(GL_TRIANGLE_STRIP, drawArraysResetQuads[i][0], drawArraysResetQuads[i][1]); // reset quads

	glTextureBarrierNV();

	shaderFbo.use(); // back to FBO, with full access already (:
}








/*
enum operation_ {operation_write, operation_resetPressure, operation_resetRest};

HyperPixelDrawer::HyperPixelDrawer(int w, int h, Shader &shader_fbo, float ***clearFrame, int stateNum, int otherQuadNum,
		 	 	 	 	 	 	   int exCType, int airCType, int wallCType) : pbo(texture), shaderFbo(shader_fbo) {
	shaders[0] = "";
	shaders[1] = "";

	operation_loc = -1;

	writeChannels = 1; // we write 3 channels: [area, boudnGain, type]

	gl_width  = w;
	gl_height = h;

	numOfStates     = stateNum;
	numOfResetQuads = otherQuadNum+1; // +1 is audio quad

	drawArraysMainQuads = new int *[numOfStates];
	for(int i=0; i<numOfStates; i++)
		drawArraysMainQuads[i] = new int[2];
	drawArraysResetQuads = new int *[numOfResetQuads];
	for(int i=0; i<numOfResetQuads; i++)
		drawArraysResetQuads[i] = new int[2];

	drawArraysValues = new int *[numOfStates + numOfResetQuads]; // drawing quads [one per each state] + other quads [which include audio quad]
	for(int i=0; i<3*numOfStates + 1; i++)
		drawArraysValues[i] = new int[2];

	// allocate and wipe
	framePixels = new GLfloat[gl_width*gl_height*writeChannels];
	memset(framePixels, 0, sizeof(GLfloat)*gl_width*gl_height*writeChannels);


	excitationCellType = exCType;
	airCellType        = airCType;
	boundaryCellType       = wallCType;

	// create and save a copy of the original image [will come handy for clear()]
	// it still contains possible initial excitation and walls, so that the first clearTexture() call saves these data into the texture
	clearFramePixels = new GLfloat[gl_width*gl_height];
	for(int i=0; i<gl_height; i++) {
		for(int j=0; j<gl_width; j++)
			clearFramePixels[i*gl_width+j] = (GLfloat)clearFrame[j][i][3]; // all air + PML + dead layers
	}
}

HyperPixelDrawer::~HyperPixelDrawer() {
	for(int i=0; i<numOfStates; i++)
		delete[] drawArraysMainQuads[i];
	delete[] drawArraysMainQuads;
	for(int i=0; i<numOfResetQuads; i++)
		delete[] drawArraysResetQuads[i];
	delete[] drawArraysResetQuads;
	for(int i=0; i<3*numOfStates + 1; i++)
		delete[] drawArraysValues[i];
	delete[] drawArraysValues;

	delete[] framePixels;
	delete[] clearFramePixels;
}

int HyperPixelDrawer::init(GLint active_unit, string shaderLocation, int drawArraysMain[][2], int drawArraysReset[][2]) {
	clear();
	GLint textureInternalFormat = GL_R32F;
	//texture.loadPixels(gl_width, gl_height, GL_R8UI, GL_RED_INTEGER, GL_UNSIGNED_BYTE, framePixels, active_unit);
	texture.loadPixels(gl_width, gl_height, textureInternalFormat, GL_RED, GL_FLOAT, framePixels, active_unit);

	// verify type and format
	if(false) {
		printf("\nFormat check for: %d\n", textureInternalFormat);

		GLint format, type;

		glGetInternalformativ(GL_TEXTURE_2D, textureInternalFormat, GL_TEXTURE_IMAGE_FORMAT, 1, &format);
		glGetInternalformativ(GL_TEXTURE_2D, textureInternalFormat, GL_TEXTURE_IMAGE_TYPE, 1, &type);

		printf("\tGL_TEXTURE_2D ---> Preferred GL_TEXTURE_IMAGE_FORMAT: %X\n", format);
		printf("\tGL_TEXTURE_2D ---> Preferred GL_TEXTURE_IMAGE_TYPE: %X\n", type);

		glGetInternalformativ(GL_TEXTURE_2D, textureInternalFormat, GL_READ_PIXELS_FORMAT, 1, &format);
		glGetInternalformativ(GL_TEXTURE_2D, textureInternalFormat, GL_READ_PIXELS_TYPE, 1, &type);

		printf("\tGL_TEXTURE_2D ---> Preferred GL_READ_PIXELS_FORMAT: %X\n", format);
		printf("\tGL_TEXTURE_2D ---> Preferred GL_READ_PIXELS_TYPE: %X\n", type);

		glGetInternalformativ(GL_FRAMEBUFFER, textureInternalFormat, GL_TEXTURE_IMAGE_FORMAT, 1, &format);
		glGetInternalformativ(GL_FRAMEBUFFER, textureInternalFormat, GL_TEXTURE_IMAGE_TYPE, 1, &type);

		printf("\tGL_FRAMEBUFFER ---> Preferred GL_TEXTURE_IMAGE_FORMAT: %X\n", format);
		printf("\tGL_FRAMEBUFFER ---> Preferred GL_TEXTURE_IMAGE_TYPE: %X\n", type);

		glGetInternalformativ(GL_FRAMEBUFFER, textureInternalFormat, GL_READ_PIXELS_FORMAT, 1, &format);
		glGetInternalformativ(GL_FRAMEBUFFER, textureInternalFormat, GL_READ_PIXELS_TYPE, 1, &type);

		printf("\tGL_FRAMEBUFFER ---> Preferred GL_READ_PIXELS_FORMAT: %X\n", format);
		printf("\tGL_FRAMEBUFFER ---> Preferred GL_READ_PIXELS_TYPE: %X\n", type);
	}

	initShaders(shaderLocation);

	shaderDraw.use();

	// these is the drawing textures we will use in the shader to update the main texture [color attachment of FBO]
	glUniform1i(shaderDraw.getUniformLocation("drawingTexture"), active_unit - GL_TEXTURE0);

	operation_loc = shaderDraw.getUniformLocation("operation");
	glUniform1i(operation_loc, operation_write); // most likely case
	if(operation_loc == -1)
		printf("operation uniform can't be found in pixel drawer fragment shader program\n");

	shaderFbo.use();

	glUniform1i(operation_loc, operation_write); // drawing
	glColorMask(GL_FALSE, GL_TRUE, GL_TRUE, GL_TRUE); // the 3 channels we write

	pbo.init(gl_width, gl_height, framePixels);

	// copy vertices indices and strides for the 4 drawing quads and 4 excitation quads and 4 excitation follow up quads and audio quad
	for(int i=0; i<numOfStates; i++) {
		drawArraysMainQuads[i][0] = drawArraysMain[i][0];
		drawArraysMainQuads[i][1] = drawArraysMain[i][1];
	}
	for(int i=0; i<numOfResetQuads; i++) {
		drawArraysResetQuads[i][0] = drawArraysReset[i][0];
		drawArraysResetQuads[i][1] = drawArraysReset[i][1];
	}

	int retval = pbo.trigger(); // test if working

	// apparently this does not happen anymore since we moved from a char PBO to a float PBO...
	if(retval == GL_INVALID_OPERATION) {
		printf("\t***Heads up, we might have stumbled upon an OpenGL bug here!***\n");
		printf("\tThe pbo that is supposed to update the pixels can't be set up.\n");
		printf("\tBut don't panic! Try to slightly increase or decrease domain width [by 1 or 2 pixels].\n");
		printf("\tSorry bout that...it's not totally my fault\n");
	}

	// transform the init image into a clear image!
	for(int i=0; i<gl_height; i++) {
		for(int j=0; j<gl_width; j++) {
			// turn into air any excitation and wall cells...cos we do not want them in the original clear frame [they make noise q:]
			if(clearFramePixels[i*gl_width+j] == excitationCellType || clearFramePixels[i*gl_width+j] == boundaryCellType)
				clearFramePixels[i*gl_width+j] = airCellType;
		}
	}
	return retval;
}

void HyperPixelDrawer::update(int state) {
	pbo.trigger();

	shaderDraw.use();

	glDrawArrays(GL_TRIANGLE_STRIP, drawArraysMainQuads[state][0], drawArraysMainQuads[state][1]); // drawing quads
	glTextureBarrierNV();

	shaderFbo.use();
}

void HyperPixelDrawer::reset() {
	shaderDraw.use();

	// first put to zero the r channel in the main quads [pressure]
	glUniform1i(operation_loc, operation_resetPressure);
	for(int i=0; i<numOfStates; i++)
		glDrawArrays(GL_TRIANGLE_STRIP, drawArraysMainQuads[i][0], drawArraysMainQuads[i][1]); // main quads
	glTextureBarrierNV();

	// then put all channels to zero in the other quads [excitation, audio]
	glUniform1i(operation_loc, operation_resetRest); // reset audio quad
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	for(int i=0; i<numOfResetQuads; i++)
		glDrawArrays(GL_TRIANGLE_STRIP, drawArraysResetQuads[i][0], drawArraysResetQuads[i][1]); // reset quads
	glTextureBarrierNV();

	glColorMask(GL_FALSE, GL_TRUE, GL_TRUE, GL_TRUE); // back to the original layout
	glUniform1i(operation_loc, operation_write); // back to drawing

	shaderFbo.use();
}

int HyperPixelDrawer::setCell(GLfloat area, GLfloat boundGain, GLfloat type, int x, int y) {
	//printf("(%d, %d)", x, y);
	if (x < 0 || x >= gl_width || y < 0 || y >= gl_height)
		return -1;

	framePixels[y*gl_width*writeChannels+x*writeChannels]   = area;
	framePixels[y*gl_width*writeChannels+x*writeChannels+1] = boundGain;
	framePixels[y*gl_width*writeChannels+x*writeChannels+2] = type;

	//printf(" = %f\n", type);

	return 0;
}

int HyperPixelDrawer::resetCellType(int x, int y) {
	if (x < 0 || x >= gl_width || y < 0 || y >= gl_height)
		return -1;

	int index = y*gl_width*writeChannels+x*writeChannels+2;

	// nothing to change
	if(framePixels[index] == clearFramePixels[y*gl_width+x])
		return 1;

	framePixels[index] = clearFramePixels[index]; // back to original value

	return 0;
}

//------------------------------------------------------------------
// private methods
//------------------------------------------------------------------
bool HyperPixelDrawer::initShaders(string shaderLocation) {
	shaders[0] = shaderLocation+"/pixelDrawer_vs.glsl";
	shaders[1] = shaderLocation+"/pixelDrawer_fs.glsl";
	const char* vertex_path_fbo      = {shaders[0].c_str()};
	const char* fragment_path_fbo    = {shaders[1].c_str()};

	return shaderDraw.init(vertex_path_fbo, fragment_path_fbo);
}*/
