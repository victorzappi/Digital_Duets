/*
 * HyperDrumhead.cpp
 *
 *  Created on: 2016-03-14
 *      Author: Victor Zappi
 */

#include "HyperDrumhead.h"
#include "QuadHyperDrumHead.h"

#include <math.h>   // isfinite()---->with cmath it does not work!
#include <unistd.h> // usleep

const int HyperDrumhead::areaNum = AREA_NUM; // must be the same as in the fbo frag shader

HyperDrumhead::HyperDrumhead() {
	// VBO/VAO params
	numOfAttributes = 16; // see explanation in fillVertices() + it must be multiple of 4!

	// areas handling
	numOfAreas = HyperDrumhead::areaNum;

	// area listeners
	areaListenerFragCoord_loc        = new GLint* [numOfAreas];
	areaListenerFragCoord_loc_render = new GLint[numOfAreas];
	areaListenerFragCoord = new float** [numOfAreas];
	areaListenerCoord     = new int* [numOfAreas];
	updateAreaListenter   = new bool[numOfAreas];
	for(int i=0; i<numOfAreas; i++) {
		areaListenerFragCoord_loc[i] = new GLint[numOfStates];
		areaListenerFragCoord[i]     = new float* [numOfStates];
		for(int j=0; j<numOfStates; j++)
			areaListenerFragCoord[i][j] = new float[2]; // x and y frag
		areaListenerCoord[i]    = new int[2]; // x and y pos
		areaListenerCoord[i][0] = -1;
		areaListenerCoord[i][1] = -1;
		updateAreaListenter[i] = false;
	}

	// area excitations
	areaExciteInput_loc = new GLint[numOfAreas];

	// area damping factors
	areaMu       = new float[numOfAreas];
	areaMu_loc   = new GLint[numOfAreas];
	updateAreaMu = new bool[numOfAreas];
	for(int i=0; i<numOfAreas; i++) {
		areaMu[i]       = 0.001; // safe init val
		areaMu_loc[i]   = -1;
		updateAreaMu[i] = false;

	}

	// area propagation factors
	areaRho       = new float[numOfAreas];
	areaRho_loc   = new GLint[numOfAreas];
	updateAreaRho = new bool[numOfAreas];
	for(int i=0; i<numOfAreas; i++) {
		areaRho[i]       = 0.5; // safe init val
		areaRho_loc[i]   = -1;
		updateAreaRho[i] = false;
	}

	// pixel drawer
	hyperPixelDrawer = NULL;

	// area visualizer
	showAreas = 0;
	showAreas_loc_render = -1;
	updateShowAreas = false;

	// over-write parent class settings
	numOfPML = 0;
	numOfDeadCellLayers = 1;

	name = "Hyper Drumhead";

	// time test
	start_ = timeval{0};
	end_   = timeval{0};
}

HyperDrumhead::~HyperDrumhead() {
	delete[] areaMu;
	delete[] areaMu_loc;

	delete[] areaRho;
	delete[] areaRho_loc;

	for(int i=0; i<numOfAreas; i++) {
		for(int j=0; j<numOfStates; j++)
			delete[] areaListenerFragCoord[i][j];
		delete[] areaListenerFragCoord_loc[i];
		delete[] areaListenerFragCoord[i];
		delete[] areaListenerCoord[i];
	}
	delete[] areaListenerFragCoord_loc;
	delete[] areaListenerFragCoord_loc_render;
	delete[] areaListenerFragCoord;
	delete[] areaListenerCoord;
	delete[] updateAreaListenter;

	delete[] areaExciteInput_loc;

	if(hyperPixelDrawer != NULL)
		delete hyperPixelDrawer;
}

float *HyperDrumhead::getBufferFloatAreas(int numSamples, float *inputBuffer, float **areaInputBuffer) {
	// handle domain changes
	updatePixelDrawer();

	// control parameters modified from the outside
	updateUniforms();

	// calculate samples on GPU
	computeSamplesAreas(numSamples, inputBuffer, areaInputBuffer);
	//computeSamples(numSamples, inputBuffer);

	// render
	renderWindow();

	// get samples from GPU
	getSamples(numSamples);

	return outputBuffer;
}


float *HyperDrumhead::getBufferFloatReadTime(int numSamples, float *inputBuffer, int &readTime) {
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

float *HyperDrumhead::getBufferFloatNoSampleRead(int numSamples, float *inputBuffer) {
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

int HyperDrumhead::setAreaListenerPosition(int index, int x, int y) {
	// the filter must be initialized to set up a the listener now! Otherwise, it will be done in init
	if(!inited)
		return -1;

	if(computeAreaListenerFrag(index, x, y) != 0) // returns 1 if outside of allowed borders [allowed borders are filter specific!]
		return 1;

	updateAreaListenter[index] = true;

	return 0;
}

void HyperDrumhead::getAreaListenerPosition(int index, int &x, int &y) {
	int pos[2] = {areaListenerCoord[index][0], areaListenerCoord[index][1]};
	fromTextureToDomain(pos);
	x = pos[0];
	y = pos[1];
}

// these 2 methods move in domain reference [shifting the window reference value stored in filter] and return in domain reference [again shifting the window reference value modified in filter]
int HyperDrumhead::shiftAreaListenerV(int index, int delta) {
	int pos[2] = {areaListenerCoord[index][0], areaListenerCoord[index][1]+delta};
	fromTextureToDomain(pos);
	if(setAreaListenerPosition(index, pos[0], pos[1]) != 0)
		printf("Can't move area listener %d here: (%d, %d)!\n", index, pos[0], pos[1]);

	return pos[1];
}

int HyperDrumhead::shiftAreaListenerH(int index, int delta){
	int pos[2] = {areaListenerCoord[index][0]+delta, areaListenerCoord[index][1]};
	fromTextureToDomain(pos);
	if(setAreaListenerPosition(index, pos[0], pos[1]) != 0)
		printf("Can't move area listener %d here: (%d, %d)!\n", index, pos[0], pos[1]);

	return pos[0];
}



int HyperDrumhead::hideListener() {
	// the filter must be initialized to set up a the listener now! Otherwise, it will be done in init
	if(!inited)
		return -1;

	//----------------------------------------------------------------
	// equivalent of computeListenerFrag(), but hardcoded, no checks
	//----------------------------------------------------------------
	listenerCoord[0] = -numOfPML-numOfDeadCellLayers-1;
	listenerCoord[1] = -numOfPML-numOfDeadCellLayers-1;

	computeNextListenerFrag();
	//----------------------------------------------------------------

	updateListener = true;

	return 0;
}

int HyperDrumhead::hideAreaListener(int index) {
	// the filter must be initialized to set up a the listener now! Otherwise, it will be done in init
	if(!inited)
		return -1;

	//----------------------------------------------------------------
	// equivalent of computeListenerFrag(), but hardcoded, no checks
	//----------------------------------------------------------------
	areaListenerCoord[index][0] = -numOfPML-numOfDeadCellLayers-1;
	areaListenerCoord[index][1] = -numOfPML-numOfDeadCellLayers-1;

	computeNextAreaListenerFrag(index);
	//----------------------------------------------------------------

	updateAreaListenter[index] = true;

	return 0;
}

int HyperDrumhead::clearDomain() {
	if(!inited)
		return -1;
	//pixelDrawer->clear();
	hyperPixelDrawer->clear();
	updatePixels      = true;

	return 0;
}

int HyperDrumhead::setCellType(int x, int y, float type) {
	if(!inited)
		return -1;

	int coords[2] = {x, y};
	if( fromDomainToGlWindow(coords)!=0 )
		return 1;

	x = coords[0];
	y = coords[1];

	if(hyperPixelDrawer->setCellType(x, y, (GLfloat)type) != 0)
		return 1;

	updatePixels = true;

	return 0;
}

int HyperDrumhead::setCellArea(int x, int y, float area) {
	if(!inited)
		return -1;

	int coords[2] = {x, y};
	if( fromDomainToGlWindow(coords)!=0 )
		return 1;

	x = coords[0];
	y = coords[1];

	if(hyperPixelDrawer->setCellArea(x, y, (GLfloat)area) != 0)
		return 1;

	updatePixels = true;

	return 0;
}


int HyperDrumhead::setCell(int x, int y, float area, float type, float bgain) {
	if(!inited)
		return -1;

	int coords[2] = {x, y};
	if( fromDomainToGlWindow(coords)!=0 )
		return 1;

	x = coords[0];
	y = coords[1];

	if(hyperPixelDrawer->setCell(x, y, (GLfloat)area, (GLfloat)type, (GLfloat)bgain) != 0)
		return 1;

	updatePixels = true;

	return 0;
}

int HyperDrumhead::resetCellType(int x, int y) {
	if(!inited)
		return -1;

	int coords[2] = {x, y};
	if( fromDomainToGlWindow(coords)!=0 )
		return 1;

	x = coords[0];
	y = coords[1];

	if(hyperPixelDrawer->resetCellType(x, y) != 0)
		return 1;

	updatePixels = true;

	return 0;
}

float HyperDrumhead::getCellArea(int x, int y) {
	if(!inited)
		return -1;

	int coords[2] = {x, y};
	if( fromDomainToGlWindow(coords)!=0 )
		return -2;

	x = coords[0];
	y = coords[1];

	return hyperPixelDrawer->getCellArea(x, y);
}

int HyperDrumhead::setShowAreas(bool show) {
	// the filter must be initialized to set up a the listener now! Otherwise, it will be done in init
	if(!inited)
		return -1;

	showAreas = show;
	updateShowAreas = true;

	return 0;
}

void HyperDrumhead::fillTexturePixel(int x, int y, float value, float ***pixels) {
	pixels[x][y][0] = 0;
	pixels[x][y][1] = 0;
	pixels[x][y][2] = getPixelBlueValue((cell_type)value);
	pixels[x][y][3] = value;

}

void HyperDrumhead::fillTextureRow(int index, int start, int end, float value, float ***pixels) {
	for(int i=start; i<=end; i++) {
		pixels[i][index][0] = 0;
		pixels[i][index][1] = 0;
		pixels[i][index][2] = getPixelBlueValue((cell_type)value);
		pixels[i][index][3] = value;
	}
}

void HyperDrumhead::fillTextureColumn(int index, int start, int end, float value, float ***pixels) {
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
// here we write the stuff we'll pass to the VBO...only difference with FDTD::fillVertices is the usage of QuadHyperDrumHead class
int HyperDrumhead::fillVertices() {
	int numOfDrawingQuads = numOfStates;
	// total number of quads: state quads + audio quad + drawing quads
	int numOfQuads = numOfStates+numOfDrawingQuads+1; // 1 is audio quad

	QuadHyperDrumHead quads[numOfQuads]; // save everything in here

	QuadHyperDrumHead quad0;
	QuadHyperDrumHead quad1;

	QuadHyperDrumHead drawingQuad0;
	QuadHyperDrumHead drawingQuad1;

	QuadHyperDrumHead quadAudio;

	// numOfStates to correctly calculate texture coordinates for excitation quads
	// numOfAttributes to allocate dynamic data structures where to save attributes
	// textureW/H since quads have to normalize pixel ranges, they need to know size of whole texture
	// deltaTexX/Y is to get neighbors
	quad0.init(numOfStates, numOfAttributes, textureWidth, textureHeight, deltaCoordX, deltaCoordY);
	quad1.init(numOfStates, numOfAttributes, textureWidth, textureHeight, deltaCoordX, deltaCoordY);

	drawingQuad0.init(numOfStates, numOfAttributes, textureWidth, textureHeight, deltaCoordX, deltaCoordY);
	drawingQuad1.init(numOfStates, numOfAttributes, textureWidth, textureHeight, deltaCoordX, deltaCoordY);

	quadAudio.init(numOfStates, numOfAttributes, textureWidth, textureHeight, deltaCoordX, deltaCoordY);

	// main quads... they are all the same [HARD CODED, change with numOfStates but not parametrically]
	//	pixels:	   top left coord (x, y)              width and height              x shift for texture coord N [texture coord N-1 is current]
	quad0.setQuad(0, textureHeight,                gl_width, gl_height, 		 	gl_width);  // left quad
	quad1.setQuad(gl_width, textureHeight,         gl_width, gl_height, 			-gl_width); // right quad

	// same vertices as main simulation quads...but they cover the whole texture area!
	drawingQuad0.setQuadDrawer(0, textureHeight,        gl_width, gl_height); // left quad
	drawingQuad1.setQuadDrawer(gl_width, textureHeight, gl_width, gl_height); // right quad

	// we can understand if one row is enough checking if the height of audio rows [means excitation cells are horizontally within last row and not vertically over multiple audio rows]
	if(audioRowH == 1)
		quadAudio.setQuadAudio(0, 1, textureWidth, 1);
	else
		quadAudio.setQuadAudio(0, audioRowH, audioRowW, audioRowH);


	// HARDCODED
	// put all quad in same array
	quads[0].copy(&quad0);
	quads[1].copy(&quad1);

	quads[numOfStates].copy(&drawingQuad0);
	quads[numOfStates+ 1].copy(&drawingQuad1);

	quads[numOfStates+numOfDrawingQuads].copy(&quadAudio);

	// prepare VBO data structure
	int verticesNum = numOfQuads*numOfVerticesPerQuad*numOfAttributes;
	vertices_data   = new float[verticesNum];

	// fill it
	int numOfElements = numOfVerticesPerQuad*numOfAttributes;
	for(int i=0; i<numOfQuads; i++) {
		for(int j=0; j<numOfElements; j++) {
			vertices_data[i*numOfElements + j] = quads[i].getAttribute(j);
			//printf("vertices_data[%d]: %f\n", i*numOfElements + j, vertices_data[i*numOfElements + j]);
		}
	}

	//printf("vertSize: %d\n", verticesNum*sizeof(float));
	return verticesNum*sizeof(float);
}

// let's write the frame, i.e., the pixels of the domain+pml+dead cell layers that are repeated per each state!
// pass excitation and use some built in boundaries that also handle PML
void HyperDrumhead::initFrame(int *excitationCoord, int *excitationDimensions, float ***domain) {
	//-----------------------------------------------------
	// prepare pixels
	//-----------------------------------------------------
	float val;

	// clear out all pixels and set all states to air with no PML absorption
	val = getPixelAlphaValue(cell_air);
	for(int i=0; i<gl_width; i++) {
		for(int j=0; j<gl_height; j++) {
			startFramePixels[i][j][0] = 0.0;
			startFramePixels[i][j][1] = 0.0;
			startFramePixels[i][j][2] = 1.0;
			startFramePixels[i][j][3] = val;
		}
	}

	// top and bottom dead cell layers [in this case we put full reflective walls]
	int toprow = gl_height;
	int btmrow = -1;
	int start  = -1;
	int stop   = gl_width;
	for(int i=0; i<numOfDeadCellLayers; i++) {
		toprow--;
		btmrow++;
		start++;
		stop--;
		val = getPixelAlphaValue(cell_dead);
		fillTextureRow(toprow, start, stop, val, startFramePixels);
		fillTextureRow(btmrow, start, stop, val, startFramePixels);
	}

	// left and right dead cell layers [in this case we put full reflective walls]
	int rightcol = gl_width;
	int leftcol  = -1;
	start    = -1;
	stop     = gl_height;
	for(int i=0; i<numOfDeadCellLayers; i++) {
		rightcol--;
		leftcol++;
		start++;
		stop--;
		val = getPixelAlphaValue(cell_dead);
		fillTextureColumn(rightcol, start, stop, val, startFramePixels);
		fillTextureColumn(leftcol, start, stop, val, startFramePixels);
	}

	// copy domain into pixels
	for(int i = 0; i <domain_width; i++) {
		for(int j = 0; j <domain_height; j++) {
			startFramePixels[numOfPML+numOfDeadCellLayers+i][numOfPML+numOfDeadCellLayers+j][3] = domain[i][j][3]; // the cell type is set from the outside
			startFramePixels[numOfPML+numOfDeadCellLayers+i][numOfPML+numOfDeadCellLayers+j][2] = getPixelBlueValue(cell_type(domain[i][j][3])); // we deduce the cell beta from it (:
			startFramePixels[numOfPML+numOfDeadCellLayers+i][numOfPML+numOfDeadCellLayers+j][1] = domain[i][j][1]; // the area is set from the outside too
		}
	}

	// add excitation
	float excitationVal = getPixelAlphaValue(cell_excitation);
	for(int i = 0; i <excitationDimensions[0]; i++) {
		fillTextureColumn(numOfPML+numOfDeadCellLayers+excitationCoord[0]+i, numOfPML+numOfDeadCellLayers+excitationCoord[1],
						  numOfPML+numOfDeadCellLayers+excitationCoord[1]+excitationDimensions[1]-1, excitationVal, startFramePixels);
	}
	//-----------------------------------------------------

	// then put the frame into the image
	fillImage(startFramePixels);

	return;
}

int HyperDrumhead::initAdditionalUniforms() {
	//--------------------------------------------------------------------------------------------
	// FBO uniforms
	//--------------------------------------------------------------------------------------------

	// location of pressure propagation coefficient, PML values, etc.

	// get locations
	quad_loc = shaderFbo.getUniformLocation("quad");
	if(quad_loc == -1) {
		printf("quad uniform can't be found in fbo shader program\n");
		//return -1;
	}

	string areaExciteInput_uniform_name = "";
	for(int i=0; i<numOfAreas; i++) {
		areaExciteInput_uniform_name = "areaExciteInput["+to_string(i)+"]";
		areaExciteInput_loc[i] = shaderFbo.getUniformLocation(areaExciteInput_uniform_name.c_str());
		if(areaExciteInput_loc[i] == -1) {
			printf("areaExciteInput[%d] uniform can't be found in fbo shader program\n", i);
			//return -1;
		}
	}

	string areaMu_uniform_name = "";
	for(int i=0; i<numOfAreas; i++) {
		areaMu_uniform_name = "areaMu["+to_string(i)+"]";
		areaMu_loc[i] = shaderFbo.getUniformLocation(areaMu_uniform_name.c_str());
		if(areaMu_loc[i] == -1) {
			printf("areaMu[%d] uniform can't be found in fbo shader program\n", i);
			//return -1;
		}
	}

	string areaRho_uniform_name = "";
	for(int i=0; i<numOfAreas; i++) {
		areaRho_uniform_name = "areaRho["+to_string(i)+"]";
		areaRho_loc[i] = shaderFbo.getUniformLocation(areaRho_uniform_name.c_str());
		if(areaRho_loc[i] == -1) {
			printf("areaRho[%d] uniform can't be found in fbo shader program\n", i);
			//return -1;
		}
	}

	string areaListenerFragCoord_uniform_name = "";
	for(int i=0; i<numOfAreas; i++) {
		for(int j=0; j<numOfStates; j++) {
			areaListenerFragCoord_uniform_name = "areaListenerFragCoord["+to_string(i)+"]["+to_string(j)+"]";
			areaListenerFragCoord_loc[i][j] = shaderFbo.getUniformLocation(areaListenerFragCoord_uniform_name.c_str());
			if(areaListenerFragCoord_loc[i][j] == -1) {
				printf("%s uniform can't be found in render shader program\n", areaListenerFragCoord_uniform_name.c_str());
				//return -1;
			}
		}
	}

	GLint maxNumOfAreas_loc = shaderFbo.getUniformLocation("numOfAreas");
	if(maxNumOfAreas_loc == -1) {
		printf("numOfAreas uniform can't be found in fbo shader program\n");
		//return -1;
	}




	// set
	// fbo
	shaderFbo.use();

	// set dynamically
	for(int i=0; i<numOfAreas; i++) {
		setUpAreaDampingFactor(i);
		setUpAreaPropagationFactor(i);
		setUpAreaListener(i);
	}

	glUniform1i(maxNumOfAreas_loc, numOfAreas);

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

	areaListenerFragCoord_uniform_name = "";
	for(int i=0; i<numOfAreas; i++) {
		areaListenerFragCoord_uniform_name = "areaListenerFragCoord["+to_string(i)+"]";
		areaListenerFragCoord_loc_render[i] = shaderRender.getUniformLocation(areaListenerFragCoord_uniform_name.c_str());
		if(areaListenerFragCoord_loc_render[i] == -1) {
			printf("%s uniform can't be found in render shader program\n", areaListenerFragCoord_uniform_name.c_str());
			//return -1;
		}
	}

	showAreas_loc_render = shaderRender.getUniformLocation("showAreas");
	if(showAreas_loc_render == -1) {
		printf("showAreas uniform can't be found in render shader program\n");
		//return -1;
	}

	GLint maxNumOfAreas_loc_render = shaderRender.getUniformLocation("numOfAreas");
	if(maxNumOfAreas_loc_render == -1) {
		printf("numOfAreas uniform can't be found in render shader program\n");
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
	for(int i=0; i<numOfAreas; i++)
		setUpAreaListenerRender(i);
	setUpMouseRender();

	setUpShowAreas();

	glUniform1i(maxNumOfAreas_loc_render, numOfAreas);

	//checkError("glUniform2i mouseFragCoord");

	glUseProgram (0);
	//--------------------------------------------------------------------------------------------
	return 0;
}

int HyperDrumhead::computeAreaListenerFrag(int index, int x, int y) {
	if(computeAreaListenerPos(index, x, y)!=0)
		return -1;
	computeNextAreaListenerFrag(index); // and calculate the position to put in that deck
		return 0;
}

int HyperDrumhead::computeAreaListenerPos(int index, int x, int y) {
	int coords[2] = {x, y};
	if(fromDomainToTexture(coords) != 0)
		return 1;

	areaListenerCoord[index][0] = coords[0];
	areaListenerCoord[index][1] = coords[1];
	return 0;
}

void HyperDrumhead::computeNextAreaListenerFrag(int index) {
	// listener coords [all quads] in texture coord reference

	// this is backwards mapping! now there's only a 3 sample delay [see getBuffer() for ridiculous explanation]
	// HARDCODED
	// quad 1      reads audio       from quad 0
	areaListenerFragCoord[index][1][0] = (float)(areaListenerCoord[index][0]+0.5) / (float)textureWidth;
	areaListenerFragCoord[index][1][1] = (float)(areaListenerCoord[index][1]+0.5) / (float)textureHeight;
	// quad 0      reads audio       from quad 1
	areaListenerFragCoord[index][0][0] = (float)(areaListenerCoord[index][0]+0.5+gl_width) / (float)textureWidth;
	areaListenerFragCoord[index][0][1] = (float)(areaListenerCoord[index][1]+0.5) / (float)textureHeight;
	// see ridiculous explanation of this mapping in getBuffer()
}

void HyperDrumhead::computeSamplesAreas(int numSamples, float *inputBuffer, float **inputAreaBuffer) {
	const int bits = GL_TEXTURE_FETCH_BARRIER_BIT;

	// Iterate over each sample
	for(int n = 0; n < numSamples; n++) {

		updatePixelDrawerFrame(); // this needs sample level precision [but it's inline]

		// CoupledFDTD simulation on GPU!
		//------------------
		// FBO passes, render texture on FBO
		for(int i=0; i<rate_mul; i++) {

		#ifdef TIME_RT_FBO
			gettimeofday(&start_, NULL);
		#endif

			if(slowMotion && render) {
				// prepare FBO passes
				shaderFbo.use();
				checkError("glUseProgram SLO MO");
				glBindFramebuffer(GL_FRAMEBUFFER, fbo); // Render to our framebuffer
				glViewport(0, 0, textureWidth, textureHeight);
			}

			// ok, down here the ridiculous explanation of the order of these operations and of the mapping of listener, excitation and feedback coordinates...
			// here we go...
			//
			// suppose we go to state x
			// this is what we do:
			// 0_save audio     ---> in particular we want to read audio from listener position in state x-1, COS WE ARE SURE IT'S READY [FDTD for x will be computed later]; this explains the weird backwards mapping of the listener coordinates among the state quads [see setUpListener()]
			// 1_compute FDTD   ---> solve for state x. To solve we need excitation cell. To be sure that no one is still messing up with it while solving for state x, we read excitation from state x-1: this explains step 0 AND the weird backwards mapping of the excitation coordinates among the state quads [see fillVertices()]
			// 3_memory barrier ---> cos step 1 take the longest, so we need to be sure that all pixels are done before we restart the loop for next state [x+1]
			// ...
			// although painful, makes a lot of sense.
			// with this structure we have only a 3 sample per each buffer before the computed audio sample is actually sent to the audio card


			state = (state+1)%numOfStates; // offset value for next round
			//send excitation input
			//glUniform1f(excitationInput_loc, inputBuffer[n*rate_mul+i]);
			// send area excitation input
			for(int j=0;j<numOfAreas; j++)
				glUniform1f(areaExciteInput_loc[j], inputAreaBuffer[j][n*rate_mul+i]);
			// send other critical uniforms that need to be updated at every sample
			updateCriticalUniforms();

			//==========================================================================
			// 0_save audio ============================================================
			if(i==rate_mul-1) {
				// prepare audio write coords [where on the audio row - what pixel and channel - the new sample should be saved]
				audioWriteCoords[0] = audioRowWriteTexCoord[0]; // audio column x coord
				audioWriteCoords[1] = audioRowWriteTexCoord[1]; // audio column y coord
				audioWriteCoords[2] = audioRowChannel; 	    // RGBA channel where to save audio

				// send coords
				glUniform3fv(audioWriteCoords_loc, 1, audioWriteCoords);

				// send quad identifier for audio quad and draw FBO
				glUniform1i(quad_loc, quad_audio + state); // audio quad id is != in the shader from CPU, see MACRO function on top
				glDrawArrays ( GL_TRIANGLE_STRIP, drawArraysValues[quad_audio][0], drawArraysValues[quad_audio][1]);
				//checkError("glDrawArrays fbo");
				//------------------

				// update for next audio write coords
				audioRowChannel = (audioRowChannel+1)%4; // cos we have 4 available channels in each pixel where to store audio
				// increment pixel Y pos, once every 4, i.e., only when all the 4 channels in the pixel have been used
				if(audioRowChannel==0) {
					audioRowWriteTexCoord[0] += deltaCoordX;
					// if audio is split into more rows and it's time to move to the next one...
					if( (audioRowH > 1) && ((n+1)%singleRowAudioCapacity) == 0 ) {
						//...reposition write coord
						audioRowWriteTexCoord[0] = 0;			 // x goes back to 0
						audioRowWriteTexCoord[1] += deltaCoordY; // y increases of one row
					}
				}
			}

			//==========================================================================
			// 1_compute FDTD ==========================================================
			// send quad identifier for one of main domain quads and draw FBO
			glUniform1i(quad_loc, quad_0+state);
			glDrawArrays ( GL_TRIANGLE_STRIP, drawArraysValues[quad_0+state][0], drawArraysValues[quad_0+state][1]);
			//checkError("glDrawArrays fbo");

			//==========================================================================
			// 4_memory barrier ========================================================
			glMemoryBarrier(bits); // to re-sync all our parallel GPU threads [this is also called implicitly when buffers are swapped]
			//------------------


			#ifdef TIME_RT_FBO
				gettimeofday(&end_, NULL);
				elapsedTime = ( (end_.tv_sec*1000000+end_.tv_usec) - (start_.tv_sec*1000000+start_.tv_usec) );
				printf("FBO cycle %d:%d\n", i, elapsedTime);

				printf("---------------------------------------------------------\n");
			#endif

			if(slowMotion && render) {
				// in slow motion mode we simply draw every frame and do all those checks that are done once for buffer when running at regular speed
				renderWindowInner();

/*				printf("%d Slow motion time: %f sec\t\tinput: %f\n", slowMo_step, slowMo_time, inputBuffer[n*rate_mul+i]);
				printf("state: %d\n", state);
				printf("-wait time: %f sec-\n", 0.000001*(slowMo_sleep_ns));*/
				slowMo_step++;
				slowMo_time += dt;
				usleep(slowMo_sleep_ns);
			}
		}

		//@debug
		//readAllTextureDebug();
	}
}

void HyperDrumhead::getSamplesReadTime(int numSamples, int &readTime) {
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

void HyperDrumhead::updatePixelDrawer() {
	if(resetPixels) {
		//pixelDrawer->reset();
		hyperPixelDrawer->reset();
		resetPixels = false;
	}

	if(updatePixels) {
		//pixelDrawer->update(state, false);
		hyperPixelDrawer->update(state);
		updatePixels = false;
	}
}

int HyperDrumhead::initPixelDrawer() {
	int types[3] = {cell_excitation, cell_air, cell_boundary};
	hyperPixelDrawer = new HyperPixelDrawer(gl_width, gl_height, 3, shaderFbo, startFramePixels,
			                      numOfStates, numOfStates+1, types);

	int drawArraysMain[numOfStates][2];
	// copy draw array values calculated in this class
	// main quads [drawing version]
	drawArraysMain[0][0] = drawArraysValues[quad_drawer_0][0];
	drawArraysMain[0][1] = drawArraysValues[quad_drawer_0][1];
	drawArraysMain[1][0] = drawArraysValues[quad_drawer_1][0];
	drawArraysMain[1][1] = drawArraysValues[quad_drawer_1][1];


	int drawArraysAudio[1][2];
	// audio quad [for reset]
	drawArraysAudio[0][0] = drawArraysValues[quad_audio][0];
	drawArraysAudio[0][1] = drawArraysValues[quad_audio][1];

	GLint texture = nextAvailableTex;
	nextAvailableTex = nextAvailableTex+1;

	int retval = hyperPixelDrawer->init(texture, shaderLoc, drawArraysMain, drawArraysAudio);

	return retval;
}

void HyperDrumhead::updateUniforms() {
	// update listener in FBO if externally modified
	if(updateListener) {
		setUpListener();
		if(!render)
			updateListener = false;
	}

	for(int i=0; i<numOfAreas; i++) {
		if(updateAreaListenter[i]) {
			setUpAreaListener(i);
			if(!render)
				updateAreaListenter[i] = false;
		}
	}

	// update damping factor in FBO if externally modified
	for(int i=0; i<numOfAreas; i++) {
		if(updateAreaMu[i]) {
			setUpAreaDampingFactor(i);
			updateAreaMu[i] = false;
		}
	}

	// update propagation factor in FBO if externally modified
	for(int i=0; i<numOfAreas; i++) {
		if(updateAreaRho[i]) {
			setUpAreaPropagationFactor(i);
			updateAreaRho[i] = false;
		}
	}
}

void HyperDrumhead::updateUniformsRender() {
	if(updateListener) {
		setUpListenerRender();
		updateListener = false;
	}

	for(int i=0; i<numOfAreas; i++) {
		if(updateAreaListenter[i]) {
			setUpAreaListenerRender(i);
			updateAreaListenter[i] = false;
		}
	}

	// update mouse position in render shader
	if(updateMouseCoord) {
		setUpMouseRender();
		updateMouseCoord = false;
	}

	if(updateShowAreas) {
		setUpShowAreas();
		updateShowAreas = false;
	}
}

// returns the beta value of the specific cell type [beta is "airness"]
float HyperDrumhead::getPixelBlueValue(int type) {
	if(type==cell_boundary || type == cell_dead)
		return 0;
	if(type>=cell_air)
		return 1;
	return -1;
}
