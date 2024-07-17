/*
 * DrumheadGPU.cpp
 *
 *  Created on: 2016-03-14
 *      Author: Victor Zappi
 */

#include "DrumheadGPU.h"

#include <math.h>  // isfinite()---->with cmath it does not work!


DrumheadGPU::DrumheadGPU() {
	// damping factor
	mu       = 0.001; // safe init val
	mu_loc   = -1;
	updateMu = false;

	// propagation factor
	rho       = 0.5; // safe init val
	rho_loc   = -1;
	updateRho = false;

	boundGain       = 0; // init as fully clamped
	boundGain_loc   = -1;
	updateBoundGain = false;

	numOfPML = 0;
	numOfOuterCellLayers = 1;

	name = "GPU FDTD Drumhead";

	// time test
	start_ = timeval{0};
	end_   = timeval{0};
}

DrumheadGPU::~DrumheadGPU() {
}

float *DrumheadGPU::getBufferFloatReadTime(int numSamples, float *inputBuffer, int &readTime) {
	// handle domain changes
	updatePixelDrawer();

	// control parameters modified from the outside
	updateUniforms();

	// calculate samples on GPU
	computeSamples(numSamples, inputBuffer);

	// render
	renderWindow();

	// get samples from GPU
	getSamplesReadTime(numSamples, readTime);

	return outputBuffer;
}

float *DrumheadGPU::getBufferFloatNoSampleRead(int numSamples, float *inputBuffer) {
	// handle domain changes
	updatePixelDrawer();

	// control parameters modified from the outside
	updateUniforms();

	// calculate samples on GPU
	computeSamples(numSamples, inputBuffer);

	// render
	renderWindow();

	// no get samples from GPU

	return outputBuffer;
}

void DrumheadGPU::fillTexturePixel(int x, int y, float value, float ***pixels) {
	pixels[x][y][0] = 0;
	pixels[x][y][1] = 0;
	pixels[x][y][2] = getPixelBlueValue((cell_type)value);
	pixels[x][y][3] = value;

}

void DrumheadGPU::fillTextureRow(int index, int start, int end, float value, float ***pixels) {
	for(int i=start; i<=end; i++) {
		pixels[i][index][0] = 0;
		pixels[i][index][1] = 0;
		pixels[i][index][2] = getPixelBlueValue((cell_type)value);
		pixels[i][index][3] = value;
	}
}

void DrumheadGPU::fillTextureColumn(int index, int start, int end, float value, float ***pixels) {
	for(int j=start; j<=end; j++) {
		pixels[index][j][0] = 0;
		pixels[index][j][1] = 0;
		pixels[index][j][2] = getPixelBlueValue((cell_type)value);
		pixels[index][j][3] = value;
	}
}

//-------------------------------------------------------------------------------------------
// private
//-------------------------------------------------------------------------------------------
// let's write the frame, i.e., the pixels of the domain+pml+dead cell layers that are repeated per each state!
// pass excitation and use some built in boundaries that also handle PML
void DrumheadGPU::initFrame(int *excitationCoord, int *excitationDimensions, float ***domain) {
	//-----------------------------------------------------
	// prepare pixels
	//-----------------------------------------------------
	float val;

	// clear out all pixels and set all states to air with no PML absorption
	val = getPixelAlphaValue(cell_air);
	for(int i=0; i<gl_width; i++) {
		for(int j=0; j<gl_height; j++) {
			clearFramePixels[i][j][0] = 0.0;
			clearFramePixels[i][j][1] = 0.0;
			clearFramePixels[i][j][2] = 1.0;
			clearFramePixels[i][j][3] = val;
		}
	}

	// top and bottom dead cell layers [in this case we put full reflective walls]
	int toprow = gl_height;
	int btmrow = -1;
	int start  = -1;
	int stop   = gl_width;
	for(int i=0; i<numOfOuterCellLayers; i++) {
		toprow--;
		btmrow++;
		start++;
		stop--;
		val = getPixelAlphaValue(cell_dead);
		fillTextureRow(toprow, start, stop, val, clearFramePixels);
		fillTextureRow(btmrow, start, stop, val, clearFramePixels);
	}

	// left and right dead cell layers [in this case we put full reflective walls]
	int rightcol = gl_width;
	int leftcol  = -1;
	start    = -1;
	stop     = gl_height;
	for(int i=0; i<numOfOuterCellLayers; i++) {
		rightcol--;
		leftcol++;
		start++;
		stop--;
		val = getPixelAlphaValue(cell_dead);
		fillTextureColumn(rightcol, start, stop, val, clearFramePixels);
		fillTextureColumn(leftcol, start, stop, val, clearFramePixels);
	}

	// copy domain into pixels
	for(int i = 0; i <domain_width; i++) {
		for(int j = 0; j <domain_height; j++) {
			clearFramePixels[numOfPML+numOfOuterCellLayers+i][numOfPML+numOfOuterCellLayers+j][3] = domain[i][j][3]; // the cell type is set from the outside
			clearFramePixels[numOfPML+numOfOuterCellLayers+i][numOfPML+numOfOuterCellLayers+j][2] = getPixelBlueValue(cell_type(domain[i][j][3])); // we deduce the cell beta from it (:
		}
	}

	// add excitation
	float excitationVal = getPixelAlphaValue(cell_excitation);
	for(int i = 0; i <excitationDimensions[0]; i++) {
		fillTextureColumn(numOfPML+numOfOuterCellLayers+excitationCoord[0]+i, numOfPML+numOfOuterCellLayers+excitationCoord[1],
						  numOfPML+numOfOuterCellLayers+excitationCoord[1]+excitationDimensions[1]-1, excitationVal, clearFramePixels);
	}
	//-----------------------------------------------------

	// then put the frame into the image
	fillImage(clearFramePixels);

	return;
}

int DrumheadGPU::initAdditionalUniforms() {
	//--------------------------------------------------------------------------------------------
	// FBO uniforms
	//--------------------------------------------------------------------------------------------

	// location of pressure propagation coefficient, PML values, etc.

	// get locations
	quad_loc= shaderFbo.getUniformLocation("quad");
	if(quad_loc == -1) {
		printf("quad uniform can't be found in fbo shader program\n");
		//return -1;
	}

	mu_loc = shaderFbo.getUniformLocation("mu");
	if(mu_loc == -1) {
		printf("mu uniform can't be found in fbo shader program\n");
		//return -1;
	}

	rho_loc = shaderFbo.getUniformLocation("rho");
	if(rho_loc == -1) {
		printf("rho uniform can't be found in fbo shader program\n");
		//return -1;
	}

	boundGain_loc = shaderFbo.getUniformLocation("boundGain");
	if(boundGain_loc == -1) {
		printf("boundGain uniform can't be found in fbo shader program\n");
		//return -1;
	}

	// set
	// fbo
	shaderFbo.use();

	// set dynamically
	setUpDampingFactor();
	setUpPropagationFactor();
	setUpClampingFactor();

	glUseProgram (0);
	//--------------------------------------------------------------------------------------------



	//--------------------------------------------------------------------------------------------
	// render shader uniforms
	//--------------------------------------------------------------------------------------------
	// get locations
	GLint deltaCoord_by2_loc = shaderRender.getUniformLocation("deltaCoord_by2");
	if(deltaCoord_by2_loc == -1) {
		printf("deltaCoord_by2 uniform can't be found in render shader program\n");
		//return -1;
	}
	GLint deltaCoord_times3_by2_loc = shaderRender.getUniformLocation("deltaCoord_times3_by2");
	if(deltaCoord_times3_by2_loc == -1) {
		printf("deltaCoord_times3_by2 uniform can't be found in render shader program\n");
		//return -1;
	}

	mouseCoord_loc_render = shaderRender.getUniformLocation("mouseFragCoord");
	if(mouseCoord_loc_render == -1) {
		printf("mouseFragCoord uniform can't be found in render shader program\n");
		//return -1;
	}


	// set
	shaderRender.use();

	// set once
	glUniform2f(deltaCoord_by2_loc, deltaCoordX/2, deltaCoordY/2);
	checkError("glUniform2f deltaCoord_by2");

	glUniform2f(deltaCoord_times3_by2_loc, 3*deltaCoordX/2, 3*deltaCoordY/2);
	checkError("glUniform2f deltaCoord_times3_by2");


	// set dynamically
	setUpListenerRender();
	setUpMouseRender();
	checkError("glUniform2i mouseFragCoord");

	glUseProgram (0);
	//--------------------------------------------------------------------------------------------
	return 0;
}

void DrumheadGPU::getSamplesReadTime(int numSamples, int &readTime) {
	if(!slowMotion) {
		//---------------------------------------------------------------------------------------------------------------------------
		// get data
		//---------------------------------------------------------------------------------------------------------------------------
		int readPixelsH = audioRowsUsed;
		int readPixelsW;

		if(audioRowH == 1)
			readPixelsW = (numSamples)/4; // 4, cos each pixel in the column contains 4 values [stored as RGBA]
		else
			readPixelsW = audioRowW; // multi row

		gettimeofday(&start_, NULL);
		outputBuffer = pbo.getPixels(readPixelsW, readPixelsH);
		gettimeofday(&end_, NULL);
		readTime = ( (end_.tv_sec*1000000+end_.tv_usec) - (start_.tv_sec*1000000+start_.tv_usec) );

		// reset simulation if gets mad!
		if(!isfinite(outputBuffer[0])) {
			printf("---out[0]: %.16f\n", outputBuffer[0]);

			reset();
			printf("Simulation went nuts, so I had to reset it_\n");
		}

		//VIC
		/*int a = 0;
		for(int n=0; n<numSamples; n++) {
			//printf("in[%d]: %f\n", n, inputBuffer[n]);
			printf("---out[%d]: %.16f\n", n, outputBuffer[n]);
			a++;
		}
		a++;*/
		//printf("---out[0]: %.16f\n", outputBuffer[0]);
		//---------------------------------------------------------------------------------------------------------------------------
		//---------------------------------------------------------------------------------------------------------------------------
	}
	else
		memset(outputBuffer, 0, numSamples);

	resetAudioRow();
}


void DrumheadGPU::updateUniforms() {
	// update listener in FBO if externally modified
	if(updateListener) {
		setUpListener();
		if(!render)
			updateListener = false;
	}

	// update damping factor in FBO if externally modified
	if(updateMu) {
		setUpDampingFactor();
		updateMu = false;
	}

	// update propagation factor in FBO if externally modified
	if(updateRho) {
		setUpPropagationFactor();
		updateRho = false;
	}

	// update boundary gain factor in FBO if externally modified
	if(updateBoundGain) {
		setUpClampingFactor();
		updateBoundGain = false;
	}
}

void DrumheadGPU::updateUniformsRender() {
	if(updateListener) {
		setUpListenerRender();
		updateListener = false;
	}

	// update mouse position in render shader
	if(updateMouseCoord) {
		setUpMouseRender();
		updateMouseCoord = false;
	}
}

// returns the beta value of the specific cell type [beta is "airness"]
float DrumheadGPU::getPixelBlueValue(int type) {
	if(type==cell_wall || type == cell_dead)
		return 0;
	if(type>=cell_air)
		return 1;
	return -1;
}
