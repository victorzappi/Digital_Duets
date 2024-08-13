/*
 * HyperDrumhead.cpp
 *
 *  Created on: 2016-03-14
 *      Author: Victor Zappi
 */

#include "HyperDrumhead.h"
#include <math.h>   // isfinite()---->with cmath it does not work!
#include <unistd.h> // usleep
#include "HyperDrumQuad.h"

const int HyperDrumhead::areaNum = AREA_NUM; // must be the same as in the fbo frag shader

HyperDrumhead::HyperDrumhead() {
	// VBO/VAO params
	numOfAttributes = 16; // see explanation in fillVertices() + it must be multiple of 4!

	// areas handling
	numOfAreas = HyperDrumhead::areaNum;

	// area listeners, first part [cos we don't know out_channels yet] 
	areaListenerFragCoord_loc        = new GLint** [numOfAreas];
	areaListenerFragCoord_loc_render = new GLint* [numOfAreas];
	areaListenerFragCoord = new float*** [numOfAreas];
	areaListenerCoord     = new int** [numOfAreas];
	
	areaListenerBFragCoord_loc        = new GLint** [numOfAreas];
	areaListenerBFragCoord_loc_render = new GLint* [numOfAreas];
	areaListenerBFragCoord    = new float*** [numOfAreas];
	areaListenerBCoord        = new int** [numOfAreas];
	
	updateAreaListener   = new bool* [numOfAreas];
	updateAreaListenerRender = new bool* [numOfAreas];
	areaListenerCanDrag            = new int* [numOfAreas];
	areaListenerCanDrag_loc_render = new GLint* [numOfAreas];
	updateAreaListenerCanDrag      = new bool* [numOfAreas];

	areaListInited        = false; // this is to handle multiple area listener preset loading [crossfade]
	updateAllAreaListeners = false; // this is to handle multiple area listener preset loading [crossfade]


	areaEraserFragCoord_loc_render= new GLint[numOfAreas];
	areaEraserFragCoord = new float* [numOfAreas];
	areaEraserCoord = new int* [numOfAreas];
	updateAreaEraser = new bool[numOfAreas];
	for(int i=0; i<numOfAreas; i++) {
		areaEraserFragCoord_loc_render[i] = -1;
		areaEraserFragCoord[i] = new float[2];
		areaEraserCoord[i] = new int[2];
		areaEraserCoord[i][0] = -2;
		areaEraserCoord[i][1] =	-2;
		updateAreaEraser[i] = false;
	}


	doCrossfade    = false;
	crossfade_loc  = -1;
	crossfade      = 0;
	crossfadeDelta = 0;
	currentDeck    = 0; // 0 = deck A, 1 = deck B

	// area excitations
	updateExcitations = false; 	// for fast excitation creation

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

	// excitation crossfade
	doExciteCrossfade     = false;
	exciteCrossfadeDelta  = 0;

	// pixel drawer
	hyperPixelDrawer = NULL;

	// area visualizer
	showAreas = 0;
	showAreas_loc_render = -1;
	updateShowAreas = false;
	selectedArea = 0;
	selectedArea_loc_render = -1;
	updateSelectedArea = false;

	// over-write parent class settings
	numOfPML = 0;
	numOfOuterCellLayers = 1;

	name = "Hyper Drumhead";

	blindBuffNum = -1;
	blindBuffCnt = 0;
	fps = 63;

	// time test
	start_ = timeval{0, 0};
	end_   = timeval{0, 0};
}

HyperDrumhead::~HyperDrumhead() {
	delete[] areaMu;
	delete[] areaMu_loc;

	delete[] areaRho;
	delete[] areaRho_loc;

	for(int i=0; i<numOfAreas; i++) {
		// this inner allocations are done in init()
		if(inited) {
			for (int k = 0; k < out_channels; k++) {
				delete[] areaListenerFragCoord_loc[i][k];
				for(int j=0; j<numOfStates; j++) 
					delete[] areaListenerFragCoord[i][k][j];	
				delete[] areaListenerFragCoord[i][k];
				delete[] areaListenerCoord[i][k];

				delete[] areaListenerBFragCoord_loc[i][k];
				for(int j=0; j<numOfStates; j++) 
					delete[] areaListenerBFragCoord[i][k][j];	
				delete[] areaListenerBFragCoord[i][k];
				delete[] areaListenerBCoord[i][k];
			}
		}
		delete[] areaListenerFragCoord_loc[i];
		delete[] areaListenerFragCoord_loc_render[i];
		delete[] areaListenerFragCoord[i];
		delete[] areaListenerCoord[i];
		
		delete[] areaListenerBFragCoord_loc[i];
		delete[] areaListenerBFragCoord_loc_render[i];
		delete[] areaListenerBFragCoord[i];
		delete[] areaListenerBCoord[i];

		delete[] updateAreaListener[i];
		delete[] updateAreaListenerRender[i];
		delete[] areaListenerCanDrag[i];
		delete[] areaListenerCanDrag_loc_render[i];
		delete[] updateAreaListenerCanDrag[i];
	}

	delete[] areaListenerFragCoord_loc;
	delete[] areaListenerFragCoord_loc_render;
	delete[] areaListenerFragCoord;
	delete[] areaListenerCoord;

	delete[] areaListenerBFragCoord_loc;
	delete[] areaListenerBFragCoord_loc_render;
	delete[] areaListenerBFragCoord;
	delete[] areaListenerBCoord;

	delete[] updateAreaListener;
	delete[] updateAreaListenerRender;
	delete[] areaListenerCanDrag;
	delete[] areaListenerCanDrag_loc_render;
	delete[] updateAreaListenerCanDrag;


	delete[] movingExciteCoords[0];
	delete[] movingExciteCoords[1];
	delete[] movingExcitation;
	delete[] nextExciteCoordX;
	delete[] nextExciteCoordY;
	delete[] resetExciteCoordX;
	delete[] resetExciteCoordY;
	delete[] resetMovingExcite;


	delete[] areaEraserFragCoord_loc_render;
	for(int i=0; i<numOfAreas; i++) {
		delete[] areaEraserFragCoord[i];
		delete[] areaEraserCoord[i];
	}
	delete[] areaEraserFragCoord;
	delete[] areaEraserCoord;
	delete[] updateAreaEraser;

	if(updateExciteCrossfade != NULL)
		delete[] updateExciteCrossfade;
	if(updateExciteCrossfadeRender != NULL)
		delete[] updateExciteCrossfadeRender;
	if(exciteCrossfade_loc != NULL)
		delete[] exciteCrossfade_loc;
	if(exciteCrossfade_loc_render != NULL)
		delete[] exciteCrossfade_loc_render;
	if(exciteCrossfade != NULL)
		delete[] exciteCrossfade;
	if(exciteCurrentDeck != NULL)
		delete[] exciteCurrentDeck;
	if(areaExciteInput_loc != NULL)
		delete[] areaExciteInput_loc;

	if(hyperPixelDrawer != NULL)
		delete hyperPixelDrawer;

}

int HyperDrumhead::init(string shaderLocation, int *domainSize, int audioRate, int rateMul, unsigned int periodSize, float mag, int *listCoord,
		       unsigned short inChannels, unsigned short outChannels, unsigned short outChnOffset) { 

	int retval = FDTD::init(shaderLocation, domainSize, audioRate, rateMul, periodSize, mag, listCoord,
		       inChannels, 0, outChannels,  outChnOffset); // always 0 in channel offset
	
		// channel excitations
		areaExciteInput_loc = new GLint[in_channels];
		updateExciteCrossfade = new bool[in_channels];
		updateExciteCrossfadeRender = new bool[in_channels];
		exciteCrossfade_loc   = new GLint[in_channels];
		exciteCrossfade_loc_render = new GLint[in_channels];
		exciteCrossfade       = new float[in_channels];
		exciteCurrentDeck     = new char[in_channels];
		movingExciteCoords[0] = new pair<int, int>[in_channels];
		movingExciteCoords[1] = new pair<int, int>[in_channels];
		movingExcitation = new bool[in_channels];
		resetMovingExcite = new bool[in_channels];
		nextExciteCoordX = new atomic<int>[in_channels];
		nextExciteCoordY = new atomic<int>[in_channels];
		resetExciteCoordX = new atomic<int>[in_channels];
		resetExciteCoordY = new atomic<int>[in_channels];

		for(int i=0; i<in_channels; i++) {
			updateExciteCrossfade[i] = false;
			exciteCrossfade_loc[i]   = -1;
			exciteCrossfade_loc_render[i] = -1;
			exciteCrossfade[i]       = 0;
			exciteCurrentDeck[i]     = 0;
			movingExciteCoords[0][i].first  = -2;
			movingExciteCoords[0][i].second = -2;
			movingExciteCoords[1][i].first  = -2;
			movingExciteCoords[1][i].second = -2;

			nextExciteCoordX[i].store(-2);
			nextExciteCoordY[i].store(-2);
			resetExciteCoordX[i].store(-2);
			resetExciteCoordY[i].store(-2);
		}

	// area listeners, second part [cos we know out_channels now] 
	for(int i=0; i<numOfAreas; i++) {
		areaListenerFragCoord_loc[i]        = new GLint* [out_channels];
		areaListenerFragCoord_loc_render[i] = new GLint [out_channels];
		areaListenerFragCoord[i] = new float** [out_channels];
		areaListenerCoord[i]     = new int* [out_channels];

		areaListenerBFragCoord_loc[i]        = new GLint* [out_channels];
		areaListenerBFragCoord_loc_render[i] = new GLint [out_channels];
		areaListenerBFragCoord[i]    = new float** [out_channels];
		areaListenerBCoord[i]        = new int* [out_channels];
		
		updateAreaListener[i]   = new bool [out_channels];
		updateAreaListenerRender[i] = new bool [out_channels];
		areaListenerCanDrag[i]            = new int [out_channels];
		areaListenerCanDrag_loc_render[i] = new GLint [out_channels];
		updateAreaListenerCanDrag[i]      = new bool [out_channels];

		for (int k = 0; k < out_channels; k++) {
			areaListenerFragCoord_loc[i][k] = new GLint[numOfStates];
			areaListenerFragCoord[i][k]     = new float* [numOfStates];
			for(int j=0; j<numOfStates; j++)
				areaListenerFragCoord[i][k][j] = new float[2]; // x and y frag
			areaListenerCoord[i][k]    = new int[2]; // x and y pos
			areaListenerCoord[i][k][0] = -1;
			areaListenerCoord[i][k][1] = -1;

			areaListenerBFragCoord_loc[i][k] = new GLint[numOfStates];
			areaListenerBFragCoord[i][k]     = new float* [numOfStates];
			for(int j=0; j<numOfStates; j++)
				areaListenerBFragCoord[i][k][j] = new float[2]; // x and y frag
			areaListenerBCoord[i][k]    = new int[2]; // x and y pos
			areaListenerBCoord[i][k][0] = -1;
			areaListenerBCoord[i][k][1] = -1;

			updateAreaListener[i][k] = false;
			updateAreaListenerRender[i][k] = false;
			areaListenerCanDrag[i][k] = 0;
			areaListenerCanDrag_loc_render[i][k] = -1;
			updateAreaListenerCanDrag[i][k] = false;
		}
	}

	nextAreaListenerFragCoord     = areaListenerFragCoord;
	nextAreaListenerFragCoord_loc = areaListenerFragCoord_loc;

	return retval;
}


double **HyperDrumhead::getFrameBuffer(int numOfSamples, double **input) {

	// move excitations to new coordinates
	for(int i=0; i<in_channels; i++) { //@VIC
		//if(movingExcitation[i])
			moveExcitation(i);
	}

	// handle domain changes
	updatePixelDrawer();

	// control parameters modified from the outside
	updateUniforms();

	// calculate samples on GPU
	computeSamples(numOfSamples, input);

	// render, reducing fps
	if(blindBuffCnt++ >= blindBuffNum) {
		renderWindow();
		blindBuffCnt = 0;
	}

	// get samples from GPU
	getSamples(numOfSamples);

	// if an excitation was scheduled for removal via a touch release...
	for(int i=0; i<in_channels; i++) { //@VIC
		if(resetMovingExcite[i])
			deleteMovingExcite(i); // ...call this to check if it is time to remove it!
	}

	// unpack multichannel buffer to framebuffer
	for(int k=0; k<out_channels; k++)
		copy(outputBuffer+k*numOfSamples, outputBuffer+k*numOfSamples+numOfSamples, framebuffer[k+out_chn_offset]);

	return framebuffer;
}

int HyperDrumhead::setAreaListenerPosition(int index, int chn , int x, int y) {
	// the filter must be initialized to set up a the listener now! Otherwise, it will be done in init
	if(!inited)
		return -1;

	//if(computeAreaListenerFrag(index, x, y) != 0) // returns 1 if outside of allowed borders [allowed borders are filter specific!]
	if(computeAreaListenerFragCrossfade(index, chn, x, y) != 0)
		return 1;

	updateAreaListener[index][chn] = true;
	if(areaListInited)
		updateAllAreaListeners= true;


	return 0;
}

int HyperDrumhead::initAreaListenerPosition(int index, int chn, int x, int y) {
	// the filter must be initialized to set up a the listener now! Otherwise, it will be done in init
	if(!inited)
		return -1;

	//if(computeAreaListenerFrag(index, x, y) != 0) // returns 1 if outside of allowed borders [allowed borders are filter specific!]
	if(initAreaListenerFragCrossfade(index, chn, x, y) != 0)
		return 1;

	updateAreaListener[index][chn] = true;
	areaListInited = true;

	return 0;
}

void HyperDrumhead::getAreaListenerPosition(int index, int chn, int &x, int &y) {
	int pos[2] = {areaListenerCoord[index][chn][0], areaListenerCoord[index][chn][1]};
	fromTextureToDomain(pos);
	x = pos[0];
	y = pos[1];
}

// these 2 methods move in domain reference [shifting the window reference value stored in filter] and return in domain reference [again shifting the window reference value modified in filter]
int HyperDrumhead::shiftAreaListenerV(int index, int chn, int delta) {
	int pos[2] = {areaListenerCoord[index][chn][0], areaListenerCoord[index][chn][1]+delta};
	fromTextureToDomain(pos);
	if(setAreaListenerPosition(index, chn, pos[0], pos[1]) != 0)
		printf("Can't move area listener %d-channel %d here: (%d, %d)!\n", index, chn, pos[0], pos[1]);

	return pos[1];
}

int HyperDrumhead::shiftAreaListenerH(int index, int chn, int delta){
	int pos[2] = {areaListenerCoord[index][chn][0]+delta, areaListenerCoord[index][chn][1]};
	fromTextureToDomain(pos);
	if(setAreaListenerPosition(index, chn, pos[0], pos[1]) != 0)
		printf("Can't move area listener %d-channel %d here: (%d, %d)!\n", index, chn, pos[0], pos[1]);

	return pos[0];
}



int HyperDrumhead::hideListener() {
	// the filter must be initialized to set up a the listener now! Otherwise, it will be done in init
	if(!inited)
		return -1;

	//----------------------------------------------------------------
	// equivalent of computeListenerFrag(), but hardcoded, no checks
	//----------------------------------------------------------------
	listenerCoord[0] = -numOfPML-numOfOuterCellLayers-1;
	listenerCoord[1] = -numOfPML-numOfOuterCellLayers-1;

	computeNextListenerFrag();
	//----------------------------------------------------------------

	updateListener = true;

	return 0;
}

int HyperDrumhead::hideAreaListener(int index, int chn) {
	// the filter must be initialized to set up a the listener now! Otherwise, it will be done in init
	if(!inited)
		return -1;

	//----------------------------------------------------------------
	// equivalent of computeListenerFrag(), but hardcoded, no checks
	//----------------------------------------------------------------
	areaListenerCoord[index][chn][0] = -numOfPML-numOfOuterCellLayers-2;
	areaListenerCoord[index][chn][1] = -numOfPML-numOfOuterCellLayers-2;

	computeHiddenAreaListenerFragCrossfade(index, chn);
	//----------------------------------------------------------------

	updateAreaListener[index][chn] = true;

	return 0;
}

int HyperDrumhead::clearDomain() {
	if(!inited)
		return -1;
	//pixelDrawer->clear();
	hyperPixelDrawer->clear();
	updatePixels = true;

	return 0;
}

int HyperDrumhead::setCellType(int x, int y, float type, int channel) {
	if(!inited)
		return -1;

	int coords[2] = {x, y};
	if( fromDomainToGlWindow(coords)!=0 )
		return 1;

	x = coords[0];
	y = coords[1];

	if(hyperPixelDrawer->setCellType(x, y, (GLfloat)type, (GLfloat)channel) != 0) // channel is used for excitations only
		return 1;

	// for fast excitation creation
	if(type != cell_excitation && type != cell_excitationA && type != cell_excitationB)
		updatePixels = true;
	else
		updateExcitations = true;

	return 0;
}

float HyperDrumhead::getCellType(int x, int y) {
	if(!inited)
		return -1;

	int coords[2] = {x, y};
	if( fromDomainToGlWindow(coords)!=0 )
		return 1;

	x = coords[0];
	y = coords[1];

	return hyperPixelDrawer->getCellType(x, y);
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


int HyperDrumhead::setCell(int x, int y, float area, float type, float bgain_or_channel) {
	if(!inited)
		return -1;

	int coords[2] = {x, y};
	if( fromDomainToGlWindow(coords)!=0 )
		return 1;

	x = coords[0];
	y = coords[1];

	if(hyperPixelDrawer->setCell(x, y, (GLfloat)area, (GLfloat)type, (GLfloat)bgain_or_channel) != 0)
		return 1;

	// for fast excitation creation
	if(type != cell_excitation && type != cell_excitationA && type != cell_excitationB)
		updatePixels = true;
	else
		updateExcitations = true;

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
	// the filter must be initialized
	if(!inited)
		return -1;

	showAreas = show;
	updateShowAreas = true;

	return 0;
}

int HyperDrumhead::setSelectedArea(int area) {
	// the filter must be initialized
	if(!inited)
		return -1;

	selectedArea = area;
	updateSelectedArea = true;

	return 0;
}

int HyperDrumhead::setAreaListenerCanDrag(int index, int chn, int drag) {
	// the filter must be initialized
	if(!inited)
		return -1;

	areaListenerCanDrag[index][chn] = drag;
	updateAreaListenerCanDrag[index][chn] = true;

	return 0;
}

int HyperDrumhead::setAreaEraserPosition(int index, int x, int y) {
	// the filter must be initialized
	if(!inited)
		return -1;

	if(computeAreaEraserFrag(index, x, y) != 0)
		return 1;

	updateAreaEraser[index] = true;

	return 0;
}

int HyperDrumhead::hideAreaEraser(int index) {
	// the filter must be initialized to set up a the listener now! Otherwise, it will be done in init
	if(!inited)
		return -1;

	//----------------------------------------------------------------
	// equivalent of computeAreaEraserFrag(), but hardcoded, no checks
	//----------------------------------------------------------------
	areaEraserCoord[index][0] = -numOfPML-numOfOuterCellLayers-2;
	areaEraserCoord[index][1] = -numOfPML-numOfOuterCellLayers-2;

	areaEraserFragCoord[index][0] = (float)(areaEraserCoord[index][0]+0.5+gl_width) / (float)textureWidth;
	areaEraserFragCoord[index][1] = (float)(areaEraserCoord[index][1]+0.5) / (float)textureHeight;
	//----------------------------------------------------------------

	updateAreaEraser[index] = true;

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
// same as FDTD::initImage() BUT factoring in multi channel audio output!
void HyperDrumhead::initImage(int domainWidth, int domainHeigth) {
	// we don't include PML in simulation domain, to make things easier when we want to update texture from outside

	gl_width  = domainWidth + 2*numOfPML + 2*numOfOuterCellLayers;
	gl_height = domainHeigth + 2*numOfPML + 2*numOfOuterCellLayers;

	// domain is inside window, shifted as follows
	domainShift[0] = (numOfPML+numOfOuterCellLayers);
	domainShift[1] = (numOfPML+numOfOuterCellLayers);


	textureWidth  = gl_width*2;
	textureHeight = gl_height;

	int numOfAudioFrags = period_size/4;

	// the audio row is at the bottom portion of the texture, audio samples are stored there.
	// each pixel contains 4 samples.
	// there must be enough space to store all the buffer samples requested by audio card in one period,
	audioRowW = min(numOfAudioFrags, gl_width*2); // cannot go beyond whole texture width
	audioRowH = 1;
	// so, now last row is numOfStates excitation pixels, numOfStates excitation follow up pixels and rest is audio row

	// if audio row space is not enough to store all the audio samples...
	if( period_size > (audioRowH*audioRowW*4) ) {
		//...we want an equal repartition of the audio samples on multiple rows, so that we can easily retrieve them in getBuffer()

		int needFragsPerRow = numOfAudioFrags/audioRowH;
		// do we have enough space?
		while(needFragsPerRow > audioRowW) {
			// if not, lets add rows
			audioRowH *= 2; // perios_size is a power of 2, hence by doubling up we are sure that we can split the samples evenly on the rows [i.e., all row has same number of samples] 
			needFragsPerRow = numOfAudioFrags/audioRowH; 
		}
		// if we have enough space, let's fix the width that we need to contain all samples
		audioRowW = needFragsPerRow;

		// please have a look at fillVertices() method for a graphical explanation of these positioning
	}

	// then we replicate the space per each audio output channel!
	audioRowH *= out_channels; // <--------- only extra line, not present in FDTD::initImage() 
	textureHeight += audioRowH+ISOLATION_LAYER; // put extra line to isolate frame from audio [to avoid interferences]

	singleRowAudioCapacity = audioRowW*4; // this is the actual number of samples that we can store in each audio row [NOT FRAGS! each frag contains 4 samples!]

	textureShift[0] = 0;
	textureShift[1] = audioRowH+ISOLATION_LAYER;

	allocateResources();

	//-----------------------------------------------------
	// these are the chunks of data drawn in each quad
	for(int quad=0,vert=0; quad<getNumOfQuads(); quad++,vert+=4) {
		drawArraysValues[quad][0] = vert;
		drawArraysValues[quad][1] = numOfVerticesPerQuad;
	}
}


// here we write the stuff we'll pass to the VBO...only difference with FDTD::fillVertices is the usage of HyperDrumQuad class
int HyperDrumhead::fillVertices() {
	int numOfDrawingQuads = numOfStates;
	// total number of quads: state quads + audio quad + drawing quads
	int numOfQuads = numOfStates+numOfDrawingQuads+1; // 1 is audio quad

	HyperDrumQuad quads[numOfQuads]; // save everything in here

	HyperDrumQuad quad0;
	HyperDrumQuad quad1;

	HyperDrumQuad drawingQuad0;
	HyperDrumQuad drawingQuad1;

	HyperDrumQuad quadAudio;

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
	// if(audioRowH == 1)
	// 	quadAudio.setQuadAudio(0, 1, textureWidth, 1);
	// else
	// 	quadAudio.setQuadAudio(0, audioRowH, audioRowW, audioRowH);
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
	// we start with preparing an empty frame, still without dead cells and PML
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

	// now we have to add dead cells and PML [actually we do not have PML in the HyperDrumhead, but we leave the code as generic as possible]

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
		fillTextureRow(toprow, start, stop, val, startFramePixels);
		fillTextureRow(btmrow, start, stop, val, startFramePixels);
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
		fillTextureColumn(rightcol, start, stop, val, startFramePixels);
		fillTextureColumn(leftcol, start, stop, val, startFramePixels);
	}


	// then we copy the full domain [received from outside] into the frame, preserving the dead cells and the PML structure!
	for(int i = 0; i <domain_width; i++) {
		for(int j = 0; j <domain_height; j++) {
			startFramePixels[numOfPML+numOfOuterCellLayers+i][numOfPML+numOfOuterCellLayers+j][3] = domain[i][j][3]; // the cell type is set from the outside
			startFramePixels[numOfPML+numOfOuterCellLayers+i][numOfPML+numOfOuterCellLayers+j][2] = getPixelBlueValue(/*cell_type*/(domain[i][j][3])); // we deduce the cell gamma from it (: [it depends on the type]
			startFramePixels[numOfPML+numOfOuterCellLayers+i][numOfPML+numOfOuterCellLayers+j][1] = domain[i][j][1]; // the area is set from the outside too
		}
	}

	// finally we add excitation
	float excitationVal = getPixelAlphaValue(cell_excitation);
	for(int i = 0; i <excitationDimensions[0]; i++) {
		fillTextureColumn(numOfPML+numOfOuterCellLayers+excitationCoord[0]+i, numOfPML+numOfOuterCellLayers+excitationCoord[1],
						  numOfPML+numOfOuterCellLayers+excitationCoord[1]+excitationDimensions[1]-1, excitationVal, startFramePixels);
	}
	//-----------------------------------------------------

	// and we conclude by putting the frame into the image [a.k.a. texture]
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
	for(int i=0; i<in_channels; i++) {
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

	GLint numOfAreas_loc = shaderFbo.getUniformLocation("numOfAreas");
	if(numOfAreas_loc == -1) {
		printf("numOfAreas uniform can't be found in fbo shader program\n");
		//return -1;
	}

	GLint numOfOutChannels_loc = shaderFbo.getUniformLocation("numOfOutChannels");
	if(numOfOutChannels_loc == -1) {
		printf("numOfOutChannels uniform can't be found in fbo shader program\n");
		//return -1;
	}

	string areaListenerFragCoord_uniform_name = "";
	for(int i=0; i<numOfAreas; i++) {
		for(int k=0; k<out_channels; k++) {
			for(int j=0; j<numOfStates; j++) {
				areaListenerFragCoord_uniform_name = "areaListenerFragCoord["+to_string(i)+"]["+to_string(k)+"]["+to_string(j)+"]";
				areaListenerFragCoord_loc[i][k][j] = shaderFbo.getUniformLocation(areaListenerFragCoord_uniform_name.c_str());
				if(areaListenerFragCoord_loc[i][k][j] == -1) {
					printf("%s uniform can't be found in render shader program\n", areaListenerFragCoord_uniform_name.c_str());
					//return -1;
				}
			}
		}
	}

	string areaListenerBFragCoord_uniform_name = "";
	for(int i=0; i<numOfAreas; i++) {
		for(int k=0; k<out_channels; k++) {
			for(int j=0; j<numOfStates; j++) {
				areaListenerBFragCoord_uniform_name = "areaListenerBFragCoord["+to_string(i)+"]["+to_string(k)+"]["+to_string(j)+"]";
				areaListenerBFragCoord_loc[i][k][j] = shaderFbo.getUniformLocation(areaListenerBFragCoord_uniform_name.c_str());
				if(areaListenerBFragCoord_loc[i][k][j] == -1) {
					printf("%s uniform can't be found in render shader program\n", areaListenerBFragCoord_uniform_name.c_str());
					//return -1;
				}
			}
		}
	}

	GLint audioWriteChannelOffset_loc = shaderFbo.getUniformLocation("audioWriteChannelOffset");
	if(audioWriteChannelOffset_loc == -1) {
		printf("audioWriteChannelOffset uniform can't be found in fbo shader program\n");
		//return -1;
	}

	// location of crossfade control
	crossfade_loc = shaderFbo.getUniformLocation("crossfade");
	if(crossfade_loc == -1) {
		printf("crossfade uniform can't be found in fbo shader program\n");
		//return -1;
	}

	// location of excitation crossfade controls
	string exciteCrossfade_uniform_name = "";
	for(int i=0; i<in_channels; i++) {
		exciteCrossfade_uniform_name = "exciteCrossfade["+to_string(i)+"]";
		exciteCrossfade_loc[i] = shaderFbo.getUniformLocation(exciteCrossfade_uniform_name.c_str());
		if(exciteCrossfade_loc[i] == -1) {
			printf("exciteCrossfade[%d] uniform can't be found in fbo shader program\n", i);
			//return -1;
		}
	}

	// set
	// fbo
	shaderFbo.use();

	// set once
	glUniform1i(numOfAreas_loc, numOfAreas);
	glUniform1i(numOfOutChannels_loc, out_channels);
	glUniform1f(audioWriteChannelOffset_loc, deltaCoordY * audioRowH/out_channels);


	// set dynamically
	for(int i=0; i<numOfAreas; i++) {
		setUpAreaDampingFactor(i);
		setUpAreaPropagationFactor(i);
		for(int k=0; k<out_channels; k++)
			setUpAreaListener(i, k);
	}

	// these are special uniforms that will be updated at sample level
	glUniform1f(crossfade_loc, crossfade);
	checkError("glUniform1f crossfade");

	for(int i=0; i<in_channels; i++) {
		glUniform1f(exciteCrossfade_loc[i], exciteCrossfade[i]);
		checkError("glUniform1f exciteCrossfade");
	}

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


	GLint numOfAreas_loc_render = shaderRender.getUniformLocation("numOfAreas");
	if(numOfAreas_loc_render == -1) {
		printf("numOfAreas uniform can't be found in render shader program\n");
		//return -1;
	}


	GLint numOfOutChannels_loc_render = shaderRender.getUniformLocation("numOfOutChannels");
	if(numOfOutChannels_loc_render == -1) {
		printf("numOfOutChannels uniform can't be found in render shader program\n");
		//return -1;
	}

	areaListenerFragCoord_uniform_name = "";
	for(int i=0; i<numOfAreas; i++) {
		for(int k=0; k<out_channels; k++) {
			areaListenerFragCoord_uniform_name = "areaListenerFragCoord["+to_string(i)+"]["+to_string(k)+"]";
			areaListenerFragCoord_loc_render[i][k] = shaderRender.getUniformLocation(areaListenerFragCoord_uniform_name.c_str());
			if(areaListenerFragCoord_loc_render[i][k] == -1) {
				printf("%s uniform can't be found in render shader program\n", areaListenerFragCoord_uniform_name.c_str());
				//return -1;
			}
		}
	}

	showAreas_loc_render = shaderRender.getUniformLocation("showAreas");
	if(showAreas_loc_render == -1) {
		printf("showAreas uniform can't be found in render shader program\n");
		//return -1;
	}

	selectedArea_loc_render = shaderRender.getUniformLocation("selectedArea");
	if(selectedArea_loc_render == -1) {
		printf("selectedArea uniform can't be found in render shader program\n");
		//return -1;
	}

	for(int i=0; i<in_channels; i++) {
		exciteCrossfade_uniform_name = "exciteCrossfade["+to_string(i)+"]";
		exciteCrossfade_loc_render[i] = shaderRender.getUniformLocation(exciteCrossfade_uniform_name.c_str());
		if(exciteCrossfade_loc_render[i] == -1) {
			printf("exciteCrossfade[%d] uniform can't be found in render shader program\n", i);
			//return -1;
		}
	}

	string areaListenerCanDrag_uniform_name = "";
	for(int i=0; i<numOfAreas; i++) {
		for(int k=0; k<out_channels; k++) {
			areaListenerCanDrag_uniform_name = "areaListenerCanDrag["+to_string(i)+"]["+to_string(k)+"]";
			areaListenerCanDrag_loc_render[i][k] = shaderRender.getUniformLocation(areaListenerCanDrag_uniform_name.c_str());
			if(areaListenerCanDrag_loc_render[i][k] == -1) {
				printf("%s uniform can't be found in render shader program\n", areaListenerCanDrag_uniform_name.c_str());
				//return -1;
			}
		}
	}

	string areaEraserFragCoord_uniform_name = "";
	for(int i=0; i<numOfAreas; i++) {
		areaEraserFragCoord_uniform_name = "areaEraserFragCoord["+to_string(i)+"]";
		areaEraserFragCoord_loc_render[i] = shaderRender.getUniformLocation(areaEraserFragCoord_uniform_name.c_str());
		if(areaEraserFragCoord_loc_render[i] == -1) {
			printf("%s uniform can't be found in render shader program\n", areaEraserFragCoord_uniform_name.c_str());
			//return -1;
		}
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
	for(int i=0; i<numOfAreas; i++) {
		for(int k=0; k<out_channels; k++)
			setUpAreaListenerRender(i, k);
	}
	setUpMouseRender();

	setUpShowAreas();
	setUpSelectedArea();

	glUniform1i(numOfAreas_loc_render, numOfAreas);
	glUniform1i(numOfOutChannels_loc_render, out_channels);


	for(int i=0; i<numOfAreas; i++) {
		for(int k=0; k<out_channels; k++)
			setUpAreaListenerCanDrag(i, k);
		setUpAreaEraser(i);
	}

	for(int i=0; i<in_channels; i++) {
		setUpExcitationCrossfadeRender(i);
		checkError("glUniform1f exciteCrossfade");
	}


	//checkError("glUniform2i mouseFragCoord");

	glUseProgram (0);
	//--------------------------------------------------------------------------------------------
	return 0;
}

int HyperDrumhead::computeAreaListenerFrag(int index, int chn, int x, int y) {
	if(computeAreaListenerPos(index, chn, x, y)!=0)
		return -1;

	computeNextAreaListenerFrag(index, chn); // and calculate the position to put in that deck
	return 0;
}

int HyperDrumhead::computeAreaListenerFragCrossfade(int index, int chn, int x, int y) {
	// listener pos coords in the specific area
	if(computeAreaListenerPos(index, chn, x, y) != 0 )
		return 1;


	// target the next deck in all areas
	if(currentDeck==0) {
		nextAreaListenerFragCoord     = areaListenerBFragCoord;
		nextAreaListenerFragCoord_loc = areaListenerBFragCoord_loc;
	}
	else {
		nextAreaListenerFragCoord     = areaListenerFragCoord;
		nextAreaListenerFragCoord_loc = areaListenerFragCoord_loc;
	}

	// and calculate the position to put in that deck [in getBuffer() the actual crossfade between new and old position will happen]
	computeNextAreaListenerFrag(index, chn);
	return 0;
}

int HyperDrumhead::computeHiddenAreaListenerFragCrossfade(int index, int chn) {


	// target the next deck in all areas...but we update both decks
	if(currentDeck==0) {
		nextAreaListenerFragCoord     = areaListenerFragCoord;
		nextAreaListenerFragCoord_loc = areaListenerFragCoord_loc;
		computeNextAreaListenerFrag(index, chn);

		nextAreaListenerFragCoord     = areaListenerBFragCoord;
		nextAreaListenerFragCoord_loc = areaListenerBFragCoord_loc;
		computeNextAreaListenerFrag(index, chn);
	}
	else {
		nextAreaListenerFragCoord     = areaListenerBFragCoord;
		nextAreaListenerFragCoord_loc = areaListenerBFragCoord_loc;
		computeNextAreaListenerFrag(index, chn);

		nextAreaListenerFragCoord     = areaListenerFragCoord;
		nextAreaListenerFragCoord_loc = areaListenerFragCoord_loc;
		computeNextAreaListenerFrag(index, chn);
	}

	// and calculate the position to put in that deck [in getBuffer() the actual crossfade between new and old position will happen]
	computeNextAreaListenerFrag(index, chn);
	return 0;
}


int HyperDrumhead::initAreaListenerFragCrossfade(int index, int chn, int x, int y) {
	// listener pos coords in the specific area
	computeAreaListenerPos(index, chn, x, y);

	// target the next deck in all areas...but we init both decks
	if(currentDeck==0) {
		nextAreaListenerFragCoord     = areaListenerFragCoord;
		nextAreaListenerFragCoord_loc = areaListenerFragCoord_loc;
		computeNextAreaListenerFrag(index, chn);

		nextAreaListenerFragCoord     = areaListenerBFragCoord;
		nextAreaListenerFragCoord_loc = areaListenerBFragCoord_loc;
		computeNextAreaListenerFrag(index, chn);
	}
	else {
		nextAreaListenerFragCoord     = areaListenerBFragCoord;
		nextAreaListenerFragCoord_loc = areaListenerBFragCoord_loc;
		computeNextAreaListenerFrag(index, chn);

		nextAreaListenerFragCoord     = areaListenerFragCoord;
		nextAreaListenerFragCoord_loc = areaListenerFragCoord_loc;
		computeNextAreaListenerFrag(index, chn);
	}

	return 0;
}


//VIC ulsess now
int HyperDrumhead::computeAreaListenerPos(int index, int chn, int x, int y) {
	int coords[2] = {x, y};
	if(fromDomainToTexture(coords) != 0)
		return 1;

	areaListenerCoord[index][chn][0] = coords[0];
	areaListenerCoord[index][chn][1] = coords[1];
	return 0;
}

void HyperDrumhead::computeNextAreaListenerFrag(int index, int chn) {
	// listener coords [all quads] in texture coord reference

	// this is backwards mapping! now there's only a 3 sample delay [see getBuffer() for ridiculous explanation]
	// HARDCODED
	// quad 1      reads audio       from quad 0
	nextAreaListenerFragCoord[index][chn][1][0] = (float)(areaListenerCoord[index][chn][0]+0.5) / (float)textureWidth;
	nextAreaListenerFragCoord[index][chn][1][1] = (float)(areaListenerCoord[index][chn][1]+0.5) / (float)textureHeight;

	// quad 0      reads audio       from quad 1
	nextAreaListenerFragCoord[index][chn][0][0] = (float)(areaListenerCoord[index][chn][0]+0.5+gl_width) / (float)textureWidth;
	nextAreaListenerFragCoord[index][chn][0][1] = (float)(areaListenerCoord[index][chn][1]+0.5) / (float)textureHeight;
	// see ridiculous explanation of this mapping in getBuffer()
}


void HyperDrumhead::computeSamples(int numOfSamples, double **input) {
	const int bits = GL_TEXTURE_FETCH_BARRIER_BIT;

	// Iterate over each sample
	for(int n = 0; n < numOfSamples; n++) {

		updatePixelDrawerCritical(); // this needs sample level precision [but it's inline]

		if(doCrossfade) {
			if(currentDeck==0)
				crossfade+=crossfadeDelta;
			else
				crossfade-=crossfadeDelta;
			//printf("crossfade %f\n", crossfade);
			glUniform1f(crossfade_loc, crossfade);
		}

		if(doExciteCrossfade) {
			for(int i=0; i<in_channels; i++) {
				if(updateExciteCrossfade[i]) {
					if(exciteCurrentDeck[i]==0)
						exciteCrossfade[i]+=exciteCrossfadeDelta;
					else
						exciteCrossfade[i]-=exciteCrossfadeDelta;
					// printf("exciteCrossfade[%d] %f\n", i, exciteCrossfade[i]);
					glUniform1f(exciteCrossfade_loc[i], exciteCrossfade[i]);
				}
			}
		}

		// FDTD simulation on GPU!
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
			// send area excitation input
			for(int j=0;j<in_channels; j++)
				glUniform1f(areaExciteInput_loc[j], input[j][n*rate_mul+i]);
			// send other critical uniforms that need to be updated at every sample
			updateUniformsCritical();

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

/*				printf("%d Slow motion time: %f sec\t\tinput: %f\n", slowMo_step, slowMo_time, inputArea[0][n*rate_mul+i]);
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

void HyperDrumhead::getSamples(int numSamples) {
	if(!slowMotion) {
		//---------------------------------------------------------------------------------------------------------------------------
		// get data
		//---------------------------------------------------------------------------------------------------------------------------
		int readPixelsH = audioRowH;
		int readPixelsW = audioRowW; // notice that we are not using numSamples as our size reference, we are using instead audioRowW that is based on period_size
		// this means that we are ignoring the fact that numSamples MIGHT be lower than period_size, and we are possibly retrieving more samples than what computed
		// we could otherwise do:
		// readPixelsW = ceil( singleRowAudioCapacity/(numSamples/4) ); <---- actual number of rows that were updated to accommodate numSamples
		// BUT it is VERY UNLIKELY that numSamples < period_size
		// the excess samples are taken care of in the caller function [getFrameBuffer()] though

		//if(audioRowH == 1)
			//readPixelsW = (numSamples)/4; // 4, cos each pixel in the column contains 4 values [stored as RGBA]
		//else
			//readPixelsW = audioRowW; // multi row

		outputBuffer = pbo.getPixels(readPixelsW, readPixelsH);

		// reset simulation if gets mad!
		if(!isfinite(outputBuffer[0])) {
			printf("---out[0]: %.16f\n", outputBuffer[0]);

			reset();
			printf("Simulation went nuts, so I had to reset it_\n");
		}

		if(doCrossfade) {
			if(currentDeck==0) {
				currentDeck = 1;
				crossfade   = 1;
			}
			else {
				currentDeck = 0;
				crossfade   = 0;
			}
			glUniform1f(crossfade_loc, crossfade);
			doCrossfade    = false;
			/*for(int i=0; i<numOfAreas; i++)
				updateAreaListener[i] = false;*/
		}

		if(doExciteCrossfade) {
			for(int i=0; i<in_channels; i++) {
				if(updateExciteCrossfade[i]) {
					if(exciteCurrentDeck[i]==0) {
						exciteCurrentDeck[i] = 1;
						exciteCrossfade[i]   = 1;
					}
					else {
						exciteCurrentDeck[i] = 0;
						exciteCrossfade[i]   = 0;
					}
					updateExciteCrossfade[i] = false;
					// remove previous moving excitation from crossfaded areas
					if(movingExcitation[i])
						deletePrevMovingExcite(i);
					//printf("\tarea[%d] deck arrived to %d\n", i, exciteCurrentDeck[i]);
				}
				glUniform1f(exciteCrossfade_loc[i], exciteCrossfade[i]);
				doExciteCrossfade = false; // we're done for sure at this point
			}
			//printf("exciteCrossfade[%d] %f\n", i, exciteCrossfade);
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

void HyperDrumhead::getSamplesReadTime(int numSamples, int &readTime) {
	if(!slowMotion) {
		//---------------------------------------------------------------------------------------------------------------------------
		// get data
		//---------------------------------------------------------------------------------------------------------------------------
		int readPixelsH = audioRowH;
		int readPixelsW = audioRowW;

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

		if(doCrossfade) {
			if(currentDeck==0) {
				currentDeck = 1;
				crossfade   = 1;
			}
			else {
				currentDeck = 0;
				crossfade   = 0;
			}
			glUniform1f(crossfade_loc, crossfade);
			doCrossfade    = false;
			/*for(int i=0; i<numOfAreas; i++)
				updateAreaListener[i] = false;*/
		}

		if(doExciteCrossfade) {
			for(int i=0; i<in_channels; i++) {
				if(updateExciteCrossfade[i]) {
					if(exciteCurrentDeck[i]==0) {
						exciteCurrentDeck[i] = 1;
						exciteCrossfade[i]   = 1;
					}
					else {
						exciteCurrentDeck[i] = 0;
						exciteCrossfade[i]   = 0;
					}
					updateExciteCrossfade[i] = false;
					// remove previous moving exciatation from crossfaded areas
					if(movingExcitation[i])
						deletePrevMovingExcite(i);
					//printf("\tarea[%d] deck arrived to %d\n", i, exciteCurrentDeck[i]);
				}
				glUniform1f(exciteCrossfade_loc[i], exciteCrossfade[i]);
				doExciteCrossfade = false; // we're done for sure at this point
			}
			//printf("exciteCrossfade[%d] %f\n", i, exciteCrossfade);
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
		updateFramePixels = false;
		updateExcitations = false;
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
		for(int k=0; k<out_channels; k++) {
			if(updateAreaListener[i][k] || updateAllAreaListeners) {
				setUpAreaListener(i, k);
				doCrossfade = true;
				updateAreaListener[i][k] = false;
				updateAreaListenerRender[i][k] = true;
			}
		}
	}
	if(updateAllAreaListeners) {
		areaListInited = false;
		updateAllAreaListeners = false;
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

	for(int i=0; i<in_channels; i++) {
		if(updateExciteCrossfade[i]) {
			doExciteCrossfade = true;
			break; // one is enough here! the stopping logic is in getSamples(...)
		}
	}
}

void HyperDrumhead::updateUniformsRender() {
	if(updateListener) {
		setUpListenerRender();
		updateListener = false;
	}

	for(int i=0; i<numOfAreas; i++) {
		for(int k=0; k<out_channels; k++) {
			if(updateAreaListenerRender[i][k]) {
				setUpAreaListenerRender(i, k);
				updateAreaListenerRender[i][k] = false;
			}
		}
	}

	for(int i=0; i<in_channels; i++) {
		if(updateExciteCrossfadeRender[i]) {
			setUpExcitationCrossfadeRender(i);
			updateExciteCrossfadeRender[i] = false;
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

	if(updateSelectedArea) {
		setUpSelectedArea();
		updateSelectedArea = false;
	}


	for(int i=0; i<numOfAreas; i++) {
		for(int k=0; k<out_channels; k++) {
			if(updateAreaListenerCanDrag[i][k]) {
				setUpAreaListenerCanDrag(i, k);
				updateAreaListenerCanDrag[i][k] = false;
			}
		}

		if(updateAreaEraser[i]) {
			setUpAreaEraser(i);
			updateAreaEraser[i] = false;
		}
	}
}

int HyperDrumhead::computeAreaEraserFrag(int index, int x, int y) {
	if(computeAreaEraserPos(index, x, y)!=0)
		return -1;

	// quad 0      reads audio       from quad 1
	areaEraserFragCoord[index][0] = (float)(areaEraserCoord[index][0]+0.5+gl_width) / (float)textureWidth;
	areaEraserFragCoord[index][1] = (float)(areaEraserCoord[index][1]+0.5) / (float)textureHeight;
	// see ridiculous explanation of this mapping in getBuffer()

	return 0;
}

int HyperDrumhead::computeAreaEraserPos(int index, int x, int y) {
	int coords[2] = {x, y};
	if(fromDomainToTexture(coords) != 0)
		return 1;

	areaEraserCoord[index][0] = coords[0];
	areaEraserCoord[index][1] = coords[1];
	return 0;
}

// returns the gamma value of the specific cell type [boundary gain: 0 is clamped, 1 is open]
float HyperDrumhead::getPixelBlueValue(int type) {
	if(type==cell_boundary || type == cell_dead)
		return 1; // set starting bgain to 1 [open] to support injection of continuous excitation [which needs bgain > 0]
	if(type>=cell_air)
		return 1; // useless
	return -1;
}

void HyperDrumhead::setFirstMovingExciteCoords(int index, int exX, int exY) {
	int deck = (int)exciteCurrentDeck[index];
	// we select the current deck and do not crossfade
	cell_type type = (deck==0) ? cell_excitationA : cell_excitationB;

	if(setCell(exX, exY, -1, type, index))
		return;

	// printf("\tcurrent set %d %d\n", exX, exY);

	movingExcitation[index] = false;

	// set current
	movingExciteCoords[1][index].first = exX;
	movingExciteCoords[1][index].second = exY;

	// same as next
	movingExciteCoords[0][index].first = exX;
	movingExciteCoords[0][index].second = exY;

	nextExciteCoordX[index].store(exX);
	nextExciteCoordY[index].store(exY);

}


void HyperDrumhead::setNextMovingExciteCoords(int index, int exX, int exY) {
	if(!inited)
		return;

	// this update can happen more than once in between render calls, but we move only to the latest one
	nextExciteCoordX[index].store(exX);
	nextExciteCoordY[index].store(exY);

	// printf("\tNEXT UPDATED %d %d\n", exX, exY);
}

void HyperDrumhead::moveExcitation(int index) {
	if(!inited)
		return;

	// read latest destination!
	int exX = nextExciteCoordX[index].load();
	int exY = nextExciteCoordY[index].load();

	// am i already there?
	if(exX == movingExciteCoords[1][index].first && exY == movingExciteCoords[1][index].second )
		return;

	// one more check
	int coords[2];
	coords[0] = exX;
	coords[1] = exY;

	// is it out of bounds?
	if( fromDomainToGlWindow(coords)!=0 ) {
		// we don't do anything!

		// leave next empty for future updates
//		movingExciteCoords[0][index].first = -2;
//		movingExciteCoords[0][index].second = -2;
//		movingExcitation[index] = false;
		return;
	}

	// ok, let's officially set it as next
	movingExciteCoords[0][index].first = exX;
	movingExciteCoords[0][index].second = exY;

	// printf("\tnext set %d %d\n", movingExciteCoords[0][index].first, movingExciteCoords[0][index].second);

	// this is need to delete the previous excitation once we reach the destination excitation, at the end of render loop
	movingExcitation[index] = true;

	// checks current deck and determines crossfade target
	int deck = (int)exciteCurrentDeck[(int)index];
	// printf("----current deck: %d, will go to %d, so PUT excitation %s\n", deck, 1-deck, (deck==0) ? "B" : "A");
	cell_type type = (deck==0) ? cell_excitationB : cell_excitationA; // if 0 goes to B, if 1 goes to A
	// add new excitatiton [destination]
	if(hyperPixelDrawer->setCell(coords[0], coords[1], (GLfloat)-1, (GLfloat)type, (GLfloat)index) != 0)
		return;

	// update pixelDrawer and trigger area excitation crossfade
	updateExcitations = true;
	updateExciteCrossfade[int(index)] = true;
	updateExciteCrossfadeRender[int(index)] = true;

	// printf("\tmoving from current %d %d to next %d %d\n", movingExciteCoords[1][index].first, movingExciteCoords[1][index].second, movingExciteCoords[0][index].first, movingExciteCoords[0][index].second);
}



void HyperDrumhead::deletePrevMovingExcite(int index) {
	// printf("\tarrived at %d %d \n", movingExciteCoords[0][index].first, movingExciteCoords[0][index].second);

	// reset current excitation, now old
	int coords[2];
	coords[0] = movingExciteCoords[1][index].first;
	coords[1] = movingExciteCoords[1][index].second;

	if( fromDomainToGlWindow(coords)==0 ) {
		hyperPixelDrawer->resetCellType(coords[0], coords[1]);
		updatePixels = true; // reset will be in effect in the next render call
		// printf("\treset %d %d\n", movingExciteCoords[1][index].first, movingExciteCoords[1][index].second);
	}

	// update current, so that it is equal to next [arrived]
	movingExciteCoords[1][index].first = movingExciteCoords[0][index].first;
	movingExciteCoords[1][index].second = movingExciteCoords[0][index].second;

	// signal we are done
	movingExcitation[index] = false;
}

void HyperDrumhead::deleteMovingExcite(int index) {
	int coords[2];

	coords[0] = resetExciteCoordX[index].load();
	coords[1] = resetExciteCoordY[index].load();

	// printf("\ttry scheduled removal of %d %d --- compared to %d %d\n", movingExciteCoords[0][index].first, movingExciteCoords[0][index].second, coords[0], coords[1]);

	//is the current excitatation the one that was scheduled for removal via touch release?
	if( movingExciteCoords[0][index].first != coords[0] || movingExciteCoords[0][index].second != coords[1])
		return;

	// reset
	fromDomainToGlWindow(coords);
	hyperPixelDrawer->resetCellType(coords[0], coords[1]); // reset it
	updatePixels = true; // reset will be in effect in the next render call
	// printf("\tcompleted scheduled removal of %d %d\n", movingExciteCoords[1][index].first, movingExciteCoords[1][index].second);
	// signal removal schedule is done
	resetMovingExcite[index] = false;

}

void HyperDrumhead::resetMovingExcitation(int index, int exX, int exY) {
	resetExciteCoordX[index].store(exX);
	resetExciteCoordY[index].store(exY);
	//printf("\tSCHEDULED removal of %d %d\n", exX, exY);
	resetMovingExcite[index] = true;
}
