/*
 * HyperPixelDrawer.h
 *
 *  Created on: Mar 13, 2017
 *      Author: vic
 */

#ifndef HYPERPIXELDRAWER_H_
#define HYPERPIXELDRAWER_H_

#include <PixelDrawer.h>

//TODO add frame management [no frame sequence]
//TODO make PixelDrawer compliant to this and swap it!

class PixelDrawer_new {
public:
	PixelDrawer_new(int w, int h, int wrChan, Shader &shader_fbo, float ***clearFrame,
					int stateNum, int totalQuadNum);
	virtual ~PixelDrawer_new();
protected:
	string shaders[2];
	int gl_width;
	int gl_height;
	int numOfStates;
	int numOfResetQuads;
	int writeChannels;
	GLfloat *framePixels;
	GLfloat *clearFramePixels;
	Texture2D texture;
	PboWriter pbo;
	GLint operation_loc;
	Shader shaderDraw;	// shader that updates texture framePixels
	Shader &shaderFbo;	// main fbo shader program [for FDTD]
	int **drawArraysMainQuads;
	int **drawArraysResetQuads;

	bool initShaders(string shaderLocation);
};

class HyperPixelDrawer : public PixelDrawer_new {
public:
	HyperPixelDrawer(int w, int h, int wrChan, Shader &shader_fbo, float ***clearFrame,
					 int stateNum, int totalQuadNum, int *cellTypes);
	~HyperPixelDrawer();
	void clear();
	int init(GLint active_unit, string shaderLocation, int drawArraysMain[][2], int drawArraysReset[][2]);
	GLfloat getCellType(int x, int y);
	GLfloat getCellArea(int x, int y);
	int setCellType(int x, int y, GLfloat type);
	int setCellArea(int x, int y, GLfloat area);
	int setCell(int x, int y, GLfloat area, GLfloat type, GLfloat bgain=-1);
	int resetCellType(int x, int y);
	int getWidth();
	int getHeight();
	void update(int state);
	void reset();

private:
	int excitationCellType;
	int airCellType;
	int boundaryCellType;
};

void inline HyperPixelDrawer::clear() {
	memcpy(framePixels, clearFramePixels, sizeof(GLfloat)*gl_width*gl_height*writeChannels); // copy from clear pixels that we wisely saved in constructor
}

inline int HyperPixelDrawer::getWidth() {
	return gl_width;
}

inline int HyperPixelDrawer::getHeight() {
	return gl_height;
}


/*
#include <GL/glew.h>    // include GLEW and new version of GL on Windows [luv Windows]
#include <GLFW/glfw3.h> // GLFW helper library
#include <vector>
#include <cstring>   // strcmp, memcpy


#include "Texture2D.h"
#include "PixelBufferObject.h"
#include "Shader.h"

class HyperPixelDrawer {
public:
	HyperPixelDrawer(int w, int h, Shader &shader_fbo, float ***clearFrame, int stateNum, int otherQuadNum,
					 int exCType, int airCType, int wallCType);
	void clear();
	~HyperPixelDrawer();
	int init(GLint active_unit, string shaderLocation, int drawArraysMain[][2], int drawArraysReset[][2]);
	void update(int state);
	void reset();
	GLfloat getCellType(int x, int y);
	int setCell(GLfloat area, GLfloat boundGain, GLfloat type, int x, int y);
	int resetCellType(int x, int y);
	int getWidth();
	int getHeight();
private:
	string shaders[2];

	int gl_width;
	int gl_height;
	int numOfStates;
	int numOfResetQuads;
	int writeChannels;
	GLfloat *framePixels;
	GLfloat *clearFramePixels;
	Texture2D texture;
	PboWriter pbo;
	GLint operation_loc;
	Shader shaderDraw;	// shader that updates texture framePixels
	Shader &shaderFbo;	// main fbo shader program [for FDTD]
	int excitationCellType;
	int airCellType;
	int wallCellType;

	int **drawArraysValues;//[13][2]; // 4 drawing quads + 4 clean up excitation quads + 4 clean up excitation follow up quads + 1 audio quad

	int **drawArraysMainQuads;
	int **drawArraysResetQuads;

	bool initShaders(string shaderLocation);
};

inline void HyperPixelDrawer::clear() {
	memcpy(framePixels, clearFramePixels, sizeof(GLfloat)*gl_width*gl_height); // copy from clear pixels that we wisely saved in constructor
}

inline GLfloat HyperPixelDrawer::getCellType(int x, int y) {
	if (x < 0 || x >= gl_width || y < 0 || y >= gl_height)
		return -1;

	return framePixels[y*gl_width*writeChannels + x*writeChannels + 2]; // type is in the last of the 3 write channels
}

inline int HyperPixelDrawer::getWidth() {
	return gl_width;
}

inline int HyperPixelDrawer::getHeight() {
	return gl_height;
}*/

#endif /* HYPERPIXELDRAWER_H_ */
