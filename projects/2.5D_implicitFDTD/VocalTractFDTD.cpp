/*
 * VocalTractFDTD.cpp
 *
 *  Created on: 2016-03-14
 *      Author: vic
 */

#include "VocalTractFDTD.h"
#include "Quad.h"

// file parser
#include <fstream>      // load file
#include <stdlib.h>	    // atoi, atof
#include <string.h>     // string tok
#include <math.h>       // round
#include <vector>

#include "pitch_shift.h"

//TODO move back absorption and visco thermal stuff in ImplicitFDTD, when we understand more about the differences between the 2 filters
VocalTractFDTD::VocalTractFDTD() {
	name = "2.5D Implicit FDTD";

	// physical params
	rho_loc   = -1;
	c_loc     = -1;
	rho_c_loc = -1;
	mu_loc    = -1;
	mu        = -1;
	prandtl   = -1;
	gamma     = -1;
	viscothermal_coefs_loc = -1;
	filter_toggle_loc      = -1;
	filter_toggle          = 1;
	updateFilterToggle     = false;
	// reed, which is not properly vocal tract...whatever
	inv_ds_loc = -1;

	// glottis
	// standard values
	gs  = 1;        // dimensionless damping factor
	q   = 1;        // dimensionless
	Ag0 = 0.000005; // see Ishizaka-Flanagan, p 1250.
	m1  = 0.000125;
	m2  = 0.000025;
	d1  = 0.0025;
	d2  = 0.0005;
	k1  = 80;
	k2  = 8;
	kc  = 25;
	dampingMod_loc  = -1;
	pitchFactor_loc = -1;
	restArea_loc    = -1;
	masses_loc      = -1;
	thicknesses_loc = -1;
	springKs_loc    = -1;
	a1              = 0.0001;
	a1Input[0]      = a1;
	a1Input[1]      = a1;
	a1Input[2]      = a1;
	a1Input_loc     = -1;
	updateGlottalParams = false;

	numOfExcitationFiles = 4;
	// excitationFiles...they do not necessarily include a single excitation each...but in case they do!
	excitationFiles      = new string[numOfExcitationFiles];
	excitationFiles[0]   = "/fbo_excitation_2MassKees.glsl";
	excitationFiles[1]   = "/fbo_excitation_2Mass.glsl";
	excitationFiles[2]   = "/fbo_excitation_bodyCover.glsl";
	excitationFiles[3]   = "/fbo_excitation_reed.glsl";
	// no feedback excitation is included in main fbo_fs shader file
	excitationMods.num   = ex_num;
	excitationMods.names = new string[excitationMods.num];
	// this stuff should be in line with how enum excitation_model was defined in VocalTractFDTD.h
	excitationMods.names[0] = "No Feedback";
	excitationMods.names[1] = "2 Mass Kees";
	excitationMods.names[2] = "2 Mass";
	excitationMods.names[3] = "Body Cover";
	excitationMods.names[4] = "Clarinet Reed";
	excitationMods.names[5] = "Sax Reed";
	excitationMods.excitationModel = 0; // start with no feedback

	// boundary layers
	absorption = false;
	z_inv_loc  = -1;
	z_inv      = -1;
	updateZ_inv = false;
	alpha      = 0.001/*0.005*/;//0.0198 is equal to 0.005 admittance

	widthTexturePixels = NULL;

	//numOfPML = 0; //VIC test when no radiation

	//slowMotion = true;
}

VocalTractFDTD::~VocalTractFDTD() {
	delete[] excitationFiles;
	delete[] excitationMods.names;

	if(widthTexturePixels!=NULL)
		delete[] widthTexturePixels;
}

// TODO call fillWidthTexture() after we pass domain!
int VocalTractFDTD::init(string shaderLocation, int *domainSize, int *excitationCoord, int *excitationDimensions, float ***domain,
		                 int audio_rate, int rate_mul, int periodSize, float ***widthMap, float openSpaceWidth, float mag, int *listCoord, float exLpF) {
	absorption = (shaderLocation.compare("shaders_VocalTract_abs")==0);
	int retval = ImplicitFDTD::init(shaderLocation, domainSize, excitationCoord, excitationDimensions, excitationMods,
			                       domain, audio_rate, rate_mul, periodSize, mag, listCoord, exLpF);
	fillWidthTexture(widthMap, openSpaceWidth);
	return retval;
}
int VocalTractFDTD::init(string shaderLocation, int *domainSize, int audio_rate, int rate_mul, int periodSize,
					     float ***widthMap, float openSpaceWidth, float mag, int *listCoord, float exLpF) {
	absorption = (shaderLocation.compare("shaders_VocalTract_abs")==0);
	int retval = ImplicitFDTD::init(shaderLocation, excitationMods, domainSize, audio_rate, rate_mul, periodSize, mag, listCoord, exLpF);
	fillWidthTexture(widthMap, openSpaceWidth);
	return retval;
}

// useless override, but might be handy in the future
float *VocalTractFDTD::getBuffer(int numSamples, double *inputBuffer) {
	return ImplicitFDTD::getBufferFloat(numSamples, inputBuffer);

//	int fftFrameSize = 1024;
//	long osamp = 32;
//
//	float *buff = ImplicitFDTD::getBufferFloat(numSamples, inputBuffer);
//	smbPitchShift(2, numSamples, fftFrameSize, osamp, audio_rate, buff, buff);
//	return buff;
}

int VocalTractFDTD::setPhysicalParameters(void *params) {
	if(!inited)
		return -1;

	this->c       = ((float *)params)[0];
	this->rho     = ((float *)params)[1];
	this->mu      = ((float *)params)[2];
	this->prandtl = ((float *)params)[3];
	this->gamma   = ((float *)params)[4];

	computeDeltaS(); // this is fundamental! when we modify physical params, also ds changes accordingly

	updatePhysParams = true;

    printf("\n");
    printf("\tc: \t\t%f\n", c);
    printf("\trho: \t\t%f\n", rho);
    printf("\tmu: \t\t%f\n", mu);
    printf("\tprandlt: \t%f\n", prandtl);
    printf("\tgamma: \t\t%f\n", gamma);
    printf("\n");

	return 0;
}

int VocalTractFDTD::setFilterToggle(int toggle) {
	if(!inited)
		return -1;

	filter_toggle = toggle;
	updateFilterToggle = true;

	return 0;
}

int VocalTractFDTD::setA1(float a1) {
	if(!inited)
		return -1;

	this->a1 = a1;

	return 0;
}

int VocalTractFDTD::setQ(float q) {
	if(!inited)
		return -1;

	if(q <= 0) {
		printf("Trying to set q parameter of 2 Mass model to %f! Must be greater than 0...\n", q);
		return 1;
	}

	this->q = q;

	updateGlottalParams = true;

	return 0;
}

int VocalTractFDTD::setAbsorptionCoefficient(float alpha) {
	if(!inited || !absorption)
		return -1;

	this->alpha = alpha;
	computeWallImpedance();

	updateZ_inv = true;

	return 0;
}

//---------------------------------------------------------
// static utils
//---------------------------------------------------------

float VocalTractFDTD::alphaFromAdmittance(float mu) {
	return 1/(0.5 + 0.25*(mu+1/mu));
}

int VocalTractFDTD::loadContourFile(int domain_size[2], float deltaS, string filename, float ***domainPixels) {

	float **points = NULL;

	int numOfPoints = parseContourPoints(domain_size, deltaS, filename, points);

	if(numOfPoints < 0) {
		if(points != NULL) {
			for(int i=0; i<numOfPoints; i++)
				delete[] points[i];
			delete[] points;
		}
		return numOfPoints;
	}

	// all black
	for(int i=0; i<domain_size[0]; i++) {
		for(int j=0; j<domain_size[1]; j++) {
			domainPixels[i][j][0]   = 0;
			domainPixels[i][j][1]   = 0;
			domainPixels[i][j][2]   = 0;
			domainPixels[i][j][3]   = ImplicitFDTD::getPixelAlphaValue(cell_air);
		}
	}


	// fill pixels
	// if there is a gap, fill it oversampling the segment
	int row0 = -1;
	int col0 = -1;
	int row1 = -1;
	int col1 = -1;
	int gap[3] = {0, 0, 0};
	float delta[2] = {0, 0};
	for(int i=0; i<numOfPoints; i+=2){
		row0 = round(points[i][0] / deltaS);
		col0 = round(points[i][1] / deltaS);

		row1 = round(points[i+1][0] / deltaS);
		col1 = round(points[i+1][1] / deltaS);

		domainPixels[row0][col0][3]   = ImplicitFDTD::getPixelAlphaValue(cell_wall);
		domainPixels[row1][col1][3]   = ImplicitFDTD::getPixelAlphaValue(cell_wall);

		gap[0] = abs(col0-col1);
		gap[1] = abs(row0-row1);
		// if there is a gap...
		if( gap[0]>1 ||  gap[1]>1) {
			gap[2] = max(gap[0], gap[1]); // gap size is in pixels
			gap[2]++;

			// ...oversample the segment so that it is divided into gap+1 sub segments
			delta[0] = ((points[i+1][0]-points[i][0])/gap[2]);
			delta[1] = ((points[i+1][1]-points[i][1])/gap[2]);

			// simply turn new samples into pixels
			for(int j=1;j<=gap[2]; j++) {
				row0 = round( (points[i][0]+delta[0]*j) / deltaS);
				col0 = round( (points[i][1]+delta[1]*j) / deltaS);
				domainPixels[row0][col0][3] = ImplicitFDTD::getPixelAlphaValue(cell_wall);
			}
		}
	}

	//VIC: problem is
	// if ds too low, constrictions are welded

	for(int i=0; i<numOfPoints; i++)
		delete[] points[i];
	delete[] points;

	return 0;
}

int VocalTractFDTD::loadContourFile_new(int domain_size[2], float deltaS, string filename, float ***domainPixels) {
	float **lowerContour= NULL;
	float **upperContour= NULL;
	float *angles= NULL;

	int numOfPoints = parseContourPoints_new(domain_size, deltaS, filename, lowerContour, upperContour, angles);

	if(numOfPoints < 0) {
		if(lowerContour != NULL) {
			for(int i=0; i<numOfPoints; i++)
				delete[] lowerContour[i];
			delete[] lowerContour;
		}
		if(upperContour != NULL) {
			for(int i=0; i<numOfPoints; i++)
				delete[] upperContour[i];
			delete[] upperContour;
		}
		return numOfPoints;
	}

	// all air
	for(int i=0; i<domain_size[0]; i++) {
		for(int j=0; j<domain_size[1]; j++) {
			domainPixels[i][j][0]   = 0;
			domainPixels[i][j][1]   = 0;
			domainPixels[i][j][2]   = 0;
			domainPixels[i][j][3]   = ImplicitFDTD::getPixelAlphaValue(cell_air);
		}
	}

	vector<int> lowerCoords;
	vector<int> upperCoords;
	int min[] = {0, 0};
	pixelateContour(numOfPoints, lowerContour, angles, deltaS, lowerCoords, min, true);
	pixelateContour(numOfPoints, upperContour, angles, deltaS, upperCoords, min, false);

	drawContour(domainPixels, lowerCoords, upperCoords, min);

	//VIC calculate a1

	for(int i=0; i<numOfPoints; i++)
		delete[] lowerContour[i];
	delete[] lowerContour;

	for(int i=0; i<numOfPoints; i++)
		delete[] upperContour[i];
	delete[] upperContour;

	return 0; //TODO this thing should return a1, but also needs glottal position in contour file
}

//-------------------------------------------------------------------------------------------
// private
//-------------------------------------------------------------------------------------------
// here we set the values and offsets that characterize our image [size, quads displacement, etc]!
// the image is what goes into the texture, i.e., all the quads, contained in the frames or not
void VocalTractFDTD::initImage(int domainWidth, int domainHeigth) {

	// excitation and excitation follow up quads [each is a single cell]
	numOfExQuads    = numOfStates;
	numOfExFolQuads = numOfStates;

	// we don't include PML in simulation domain, to make things easier when we want to update texture from outside

	gl_width  = domainWidth + 2*numOfPML + 2*numOfOuterCellLayers;
	gl_height = domainHeigth + 2*numOfPML + 2*numOfOuterCellLayers;

	// domain is inside window, shifted as follows
	domainShift[0] = (numOfPML+numOfOuterCellLayers);
	domainShift[1] = (numOfPML+numOfOuterCellLayers);


	textureWidth  = gl_width*2 + WIDTH_TEXT_ISOLATION_LAYER_R; // extra layer on the right
	textureHeight = gl_height*2+ WIDTH_TEXT_ISOLATION_LAYER_U; // extra layer on top

	// the audio row is at the bottom portion of the texture, audio samples are stored there.
	// each pixel contains 4 samples.
	// there must be enough space to store all the buffer samples requested by audio card in one period,
	audioRowW = gl_width*2 - numOfExQuads - numOfExFolQuads; // whole texture width - excitation and excitation follow up pixels, cos by default they are on same row as audio
	audioRowH = audioRowsUsed = 1;
	// so, now last row is numOfStates excitation pixels, numOfStates excitation follow up pixels and rest is audio row

	// if audio row space is not enough to store all the audio samples...
	if( period_size > (audioRowH*audioRowW*4) ) {
		//...we want an equal repartition of the audio samples on multiple rows, so that we can easily retrieve them in getBuffer()

		// first of all lets count how many fragments we need to save the whole sample buffer as fragments
		int numOfAudioFrags = period_size/4;

		// then let's start placing the ex quads and followup quads side by side, vertically, at the bottom left of the texture
		audioRowH = numOfExQuads;
		audioRowW = textureWidth-2;

		// we can use only an even number of fragments if we wanna split audio samples on multiple rows,
		// since period sizes are always even
		audioRowsUsed = numOfExQuads;
		if((audioRowsUsed%2)!=0)
			audioRowsUsed--; // if it was not even, we discard one


		// this is the number of frags needed on each row for an equal repartition of the audio samples
		int neededFrags = numOfAudioFrags/audioRowsUsed;
		// do we have enough space?
		while(neededFrags > audioRowW) {
			// if not, lets add rows, always an even number in total!
			audioRowsUsed *= 2;
			audioRowH   = audioRowsUsed;
			neededFrags = numOfAudioFrags/audioRowsUsed;
		}
		// if we have enough space, let's fix the width that we need to contain all samples
		audioRowW = neededFrags;

		// please have a look at fillVertices() method for a graphical explanation of these positioning
	}

	textureHeight += audioRowH+ISOLATION_LAYER; // put extra line to isolate frame from audio [to avoid interferences]

	singleRowAudioCapacity = audioRowW*4; // this is the actual number of samples that we can store in each audio row [NOT FRAGS! each frag contains 4 samples!]

	textureShift[0] = 0;
	textureShift[1] = audioRowH+ISOLATION_LAYER;// (audioRowH+ISOLATION_LAYER-WIDTH_TEXT_ISOLATION_LAYER_U+1)/2;

	//VIC: not sure why we have to include this additional shift when the number of audio rows gets bigger
	// if we don't, quads on top of audio rows are visually ok, but fragments in FBO are all shifted, causing problems when trying to get audio from listener and feedback
	//lowQuadsKludgeRows = (audioRowH+ISOLATION_LAYER+WIDTH_TEXT_ISOLATION_LAYER_U)/2;

	allocateResources();


	//-----------------------------------------------------
	// these are the chunks of data drawn in each quad
	drawArraysValues[quad_0][0] = 0;
	drawArraysValues[quad_0][1] = numOfVerticesPerQuad;

	drawArraysValues[quad_1][0] = 4;
	drawArraysValues[quad_1][1] = numOfVerticesPerQuad;

	drawArraysValues[quad_2][0] = 8;
	drawArraysValues[quad_2][1] = numOfVerticesPerQuad;

	drawArraysValues[quad_3][0] = 12;
	drawArraysValues[quad_3][1] = numOfVerticesPerQuad;

	drawArraysValues[quad_excitation_0][0] = 16;
	drawArraysValues[quad_excitation_0][1] = numOfVerticesPerQuad;

	drawArraysValues[quad_excitation_1][0] = 20;
	drawArraysValues[quad_excitation_1][1] = numOfVerticesPerQuad;

	drawArraysValues[quad_excitation_2][0] = 24;
	drawArraysValues[quad_excitation_2][1] = numOfVerticesPerQuad;

	drawArraysValues[quad_excitation_3][0] = 28;
	drawArraysValues[quad_excitation_3][1] = numOfVerticesPerQuad;

	drawArraysValues[quad_excitation_fu_0][0] = 32;
	drawArraysValues[quad_excitation_fu_0][1] = numOfVerticesPerQuad;

	drawArraysValues[quad_excitation_fu_1][0] = 36;
	drawArraysValues[quad_excitation_fu_1][1] = numOfVerticesPerQuad;

	drawArraysValues[quad_excitation_fu_2][0] = 40;
	drawArraysValues[quad_excitation_fu_2][1] = numOfVerticesPerQuad;

	drawArraysValues[quad_excitation_fu_3][0] = 44;
	drawArraysValues[quad_excitation_fu_3][1] = numOfVerticesPerQuad;

	drawArraysValues[quad_drawer_0][0] = 48;
	drawArraysValues[quad_drawer_0][1] = numOfVerticesPerQuad;

	drawArraysValues[quad_drawer_1][0] = 52;
	drawArraysValues[quad_drawer_1][1] = numOfVerticesPerQuad;

	drawArraysValues[quad_drawer_2][0] = 56;
	drawArraysValues[quad_drawer_2][1] = numOfVerticesPerQuad;

	drawArraysValues[quad_drawer_3][0] = 60;
	drawArraysValues[quad_drawer_3][1] = numOfVerticesPerQuad;

	drawArraysValues[quad_audio][0] = 64;
	drawArraysValues[quad_audio][1] = numOfVerticesPerQuad;
	//-----------------------------------------------------
}

int VocalTractFDTD::fillVertices() {
	int numOfDrawingQuads = numOfStates;
	// total number of quads: state quads + audio quad + excitation quads + excitation follow up quads + drawing quads
	int numOfQuads = numOfStates+numOfExQuads+numOfExFolQuads+numOfDrawingQuads+1; // 1 is audio quad

	QuadImplicitFDTD quads[numOfQuads]; // save everything in here

	QuadImplicitFDTD quad0;
	QuadImplicitFDTD quad1;
	QuadImplicitFDTD quad2;
	QuadImplicitFDTD quad3;

	QuadImplicitFDTD quadsEx[numOfExQuads+numOfExFolQuads];

	QuadImplicitFDTD drawingQuad0;
	QuadImplicitFDTD drawingQuad1;
	QuadImplicitFDTD drawingQuad2;
	QuadImplicitFDTD drawingQuad3;

	QuadImplicitFDTD quadAudio;

	// numOfStates to correctly calculate texture coordinates for excitation quads
	// numOfAttributes to allocate dynamic data structures where to save attributes
	// textureW/H since quads have to normalize pixel ranges, they need to know size of whole texture
	// deltaTexX/Y is to get neighbors
	quad0.init(numOfStates, numOfAttributes, textureWidth, textureHeight, deltaCoordX, deltaCoordY);
	quad1.init(numOfStates, numOfAttributes, textureWidth, textureHeight, deltaCoordX, deltaCoordY);
	quad2.init(numOfStates, numOfAttributes, textureWidth, textureHeight, deltaCoordX, deltaCoordY);
	quad3.init(numOfStates, numOfAttributes, textureWidth, textureHeight, deltaCoordX, deltaCoordY);

	for(int i=0; i<numOfExQuads+numOfExFolQuads; i++)
		quadsEx[i].init(numOfStates, numOfAttributes, textureWidth, textureHeight, deltaCoordX, deltaCoordY);

	drawingQuad0.init(numOfStates, numOfAttributes, textureWidth, textureHeight, deltaCoordX, deltaCoordY);
	drawingQuad1.init(numOfStates, numOfAttributes, textureWidth, textureHeight, deltaCoordX, deltaCoordY);
	drawingQuad2.init(numOfStates, numOfAttributes, textureWidth, textureHeight, deltaCoordX, deltaCoordY);
	drawingQuad3.init(numOfStates, numOfAttributes, textureWidth, textureHeight, deltaCoordX, deltaCoordY);

	quadAudio.init(numOfStates, numOfAttributes, textureWidth, textureHeight, deltaCoordX, deltaCoordY);
	// if one row is enough to store audio buffer samples and have excitation, the excitation stuff is placed as depicted in scheme on top of this file and here:
	/*
	 * _______________________
	 *|_______|_|_|_|_|_|_|_|_| } lower extension [audio rows+excitation+excitation followup]
	 *	^audio^	 ^ side quads ^
	 *	 [side quads are excitation+excitation followup]
	*/


	//TODO update diagram
	// otherwise we reserve to audio more than one row at bottom of global texture and move excitation cells at the top righ of texture, as a column
	/*				 _
	 * 				|_|   < excitation and followup cells in column
	 * 				|_|
	 * 				|_|
	 * _____________ ...  continues...
	 *|             |
	 *|_____________|
	 *	^audio^
	 *	 [side quads are excitation+excitation followup]
	*/

	//	pixels:	              top left coord (x, y)                								width and height                 x and y shift for texture coord N and N-1
	int quad0Attributes[8] = {0, textureHeight-gl_height-WIDTH_TEXT_ISOLATION_LAYER_U,        gl_width, gl_height, 		 	    gl_width, 0, gl_width, gl_height};
	int quad1Attributes[8] = {0, textureHeight-WIDTH_TEXT_ISOLATION_LAYER_U,                  gl_width, gl_height, 				0, -gl_height, gl_width, -gl_height};
	int quad2Attributes[8] = {gl_width, textureHeight-WIDTH_TEXT_ISOLATION_LAYER_U,           gl_width, gl_height, 			    -gl_width, 0, -gl_width, -gl_height};
	int quad3Attributes[8] = {gl_width, textureHeight-gl_height-WIDTH_TEXT_ISOLATION_LAYER_U, gl_width, gl_height, 			    0, gl_height, -gl_width, gl_height};

	// main quads... they are all the same [HARD CODED, change with numOfStates but not parametrically]
	quad0.setQuad(quad0Attributes); // bottom left quad
	quad1.setQuad(quad1Attributes); // top left quad
	quad2.setQuad(quad2Attributes); // top right quad
	quad3.setQuad(quad3Attributes); // bottom left quad

	// same as
	/*quad0.setQuad(0, textureHeight-gl_height,      gl_width, gl_height, 		 	    gl_width, 0, gl_width, gl_height);
	quad1.setQuad(0, textureHeight,                  gl_width, gl_height, 				0, -gl_height, gl_width, -gl_height);
	quad2.setQuad(gl_width, textureHeight,           gl_width, gl_height, 			    -gl_width, 0, -gl_width, -gl_height);
	quad3.setQuad(gl_width, textureHeight-gl_height, gl_width, gl_height, 			    0, gl_height, -gl_width, gl_height);*/

	// same vertices as main simulation quads...but they cover the whole texture area!
	drawingQuad0.setQuadDrawer(0, textureHeight-gl_height-WIDTH_TEXT_ISOLATION_LAYER_U, 		gl_width, gl_height); // bottom left quad
	drawingQuad1.setQuadDrawer(0, textureHeight-WIDTH_TEXT_ISOLATION_LAYER_U, 					gl_width, gl_height); // top left quad
	drawingQuad2.setQuadDrawer(gl_width, textureHeight-WIDTH_TEXT_ISOLATION_LAYER_U, 			gl_width, gl_height); // top right quad
	drawingQuad3.setQuadDrawer(gl_width, textureHeight-WIDTH_TEXT_ISOLATION_LAYER_U-gl_height, 	gl_width, gl_height); // bottom left quad


	// we can understand if one row is enough checking if the height of audio rows [means excitation cells are horizontally within last row and not vertically over multiple audio rows]
	if(audioRowH == 1){
		int rightx = textureWidth-(numOfExQuads+numOfExFolQuads); // save length of audio quad, useful to determine leftmost coordinate of excitation quads
		quadAudio.setQuadAudio(0, 1, rightx, 1);

		// excitation cells
		for(int i=0; i<numOfExQuads; i++) {
			// top left coord (x, y),     width and height    x and y shift for neighbor,   x and y shift for follow up,    index to quad this excitation is linked to
			quadsEx[i].setQuadExcitation( rightx, 1,    1, 1, 	1, 0,    numOfStates, 0, 	i);
			rightx++;
		}

		// excitation follow up cells
		for(int i=0; i<numOfExFolQuads; i++) {
			// top left coord (x, y),     width and height    x and y shift for neighbor, x and y shift for follow up [which is excitation in this case, cos this is followu p],    index to quad this excitation is linked to
			quadsEx[numOfExQuads+i].setQuadExcitation( rightx, 1, 1, 1, 	1, 0, -numOfStates, 0, 	i);
			rightx++;
		}
	}
	else {
		quadAudio.setQuadAudio(0, audioRowH, audioRowW, audioRowH);

		int top = audioRowH;
		for(int i=0; i<numOfExQuads; i++) {
			// top left coord (x, y),     width and height    x and y shift for neighbor,   x and y shift for follow up,    index to quad this excitation is linked to
			quadsEx[i].setQuadExcitation( textureWidth-2, top, 	1, 1, 	0, -1, 		1, 0, 	i);
			top--;
		}

		// excitation follow up cells
		top = audioRowH;
		for(int i=0; i<numOfExFolQuads; i++) {
			// top left coord (x, y),     width and height    x and y shift for neighbor, x and y shift for follow up [which is excitation in this case, cos this is followu p],    index to quad this excitation is linked to
			quadsEx[numOfExQuads+i].setQuadExcitation( textureWidth-1, top, 	1, 1, 	0, -1, 		-1, 0, 	i);
			top--;
		}
	}

	// save these values so that main quads' pixels know where to look for excitation in texture in shader
	// please note that we have to fetch the coordinates pointing to this specific quad among the attributes of the next quad [cos tex coords always point to previous step]
	// PLUS
	// we want quad x to point to excitation quad x-1!!! see ridiculous explanation in getBuffer()
	for(int i=0; i<numOfStates; i++) {
		exFragCoord[i][0] = quadsEx[i].getAttribute(2)+deltaCoordX/2; // x texture coordinate for state N of 3rd entry [bottom left vertex]
		exFragCoord[i][1] = quadsEx[i].getAttribute(3)+deltaCoordY/2; // y texture coordinate for state N of 3rd entry [bottom left vertex]
	}
	//VIC we added deltaCoordX/2 cos this shift otherwise would be needed in the FBO shader [and repeated for each fragment!]. We have to remove it in the render shader though...not sure why they differ!


	// HARDCODED
	// put all quad in same array
	quads[0].copy(&quad0);
	quads[1].copy(&quad1);
	quads[2].copy(&quad2);
	quads[3].copy(&quad3);

	for(int i=0; i<numOfExQuads+numOfExFolQuads; i++)
		quads[numOfStates+i].copy(&quadsEx[i]);

	quads[numOfStates+numOfExQuads+numOfExFolQuads].copy(&drawingQuad0);
	quads[numOfStates+numOfExQuads+numOfExFolQuads + 1].copy(&drawingQuad1);
	quads[numOfStates+numOfExQuads+numOfExFolQuads + 2].copy(&drawingQuad2);
	quads[numOfStates+numOfExQuads+numOfExFolQuads + 3].copy(&drawingQuad3);

	quads[numOfStates+numOfExQuads+numOfExFolQuads+numOfDrawingQuads].copy(&quadAudio);

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

void VocalTractFDTD::initPhysicalParameters() {
    c       = C_VT;
    rho     = RHO_VT;
    mu      = MU_VT;
    prandtl = PRANDTL_VT;
    gamma   = GAMMA_VT;

    printf("\n");
    printf("\tc: \t\t%f\n", c);
    printf("\trho: \t\t%f\n", rho);
    printf("\tmu: \t\t%f\n", mu);
    printf("\tprandlt: \t%f\n", prandtl);
    printf("\tgamma: \t\t%f\n", gamma);
    printf("\n");


    computeWallImpedance();
}

int VocalTractFDTD::initAdditionalUniforms() {
	if(ImplicitFDTD::initAdditionalUniforms()!= 0)
		return -1;

	//--------------------------------------------------------------------------------------------
	// FBO uniforms
	//--------------------------------------------------------------------------------------------

	// get locations
	// location of samplerate and samplerate*samplerate
	GLint rates_loc = shaderFbo.getUniformLocation("rates");
	if(rates_loc == -1) {
		printf("rates uniform can't be found in fbo shader program\n");
		//return -1;
	}

	// physics params
	rho_loc = shaderFbo.getUniformLocation("rho");
	if(rho_loc == -1) {
		printf("rho uniform can't be found in fbo shader program\n");
		//return -1;
	}
	mu_loc = shaderFbo.getUniformLocation("mu");
	if(mu_loc == -1) {
		printf("mu uniform can't be found in fbo shader program\n");
		//return -1;
	}
	c_loc = shaderFbo.getUniformLocation("c");
	if(c_loc == -1) {
		printf("c uniform can't be found in fbo shader program\n");
		//return -1;
	}
	rho_c_loc = shaderFbo.getUniformLocation("rho_c");
	if(rho_c_loc == -1) {
		printf("rho_c uniform can't be found in fbo shader program\n");
		//return -1;
	}

	if(!absorption) {
		// location of number of viscothermal coefficients
		viscothermal_coefs_loc = shaderFbo.getUniformLocation("viscothermal_coefs");
		if(viscothermal_coefs_loc == -1) {
			printf("viscothermal_coefs uniform can't be found in fbo shader program\n");
			//return -1;
		}
	}
	// location of filter toggle
	filter_toggle_loc = shaderFbo.getUniformLocation("filter_toggle");
	if(filter_toggle_loc == -1) {
		printf("filter_toggle uniform can't be found in fbo shader program\n");
		//return -1;
	}

	// glottis params
	dampingMod_loc = shaderFbo.getUniformLocation("dampingMod");
	if(dampingMod_loc == -1) {
		printf("dampingMod uniform can't be found in fbo shader program\n");
		//return -1;
	}
	pitchFactor_loc = shaderFbo.getUniformLocation("pitchFactor");
	if(pitchFactor_loc == -1) {
		printf("pitchFactor uniform can't be found in fbo shader program\n");
		//return -1;
	}
	restArea_loc = shaderFbo.getUniformLocation("restArea");
	if(restArea_loc == -1) {
		printf("restArea uniform can't be found in fbo shader program\n");
		//return -1;
	}
	masses_loc = shaderFbo.getUniformLocation("masses");
	if(masses_loc == -1) {
		printf("masses uniform can't be found in fbo shader program\n");
		//return -1;
	}
	thicknesses_loc = shaderFbo.getUniformLocation("thicknesses");
	if(thicknesses_loc == -1) {
		printf("thicknesses uniform can't be found in fbo shader program\n");
		//return -1;
	}
	springKs_loc = shaderFbo.getUniformLocation("springKs");
	if(springKs_loc == -1) {
		printf("springKs uniform can't be found in fbo shader program\n");
		//return -1;
	}

    a1Input_loc = shaderFbo.getUniformLocation("a1Input");
	if(a1Input_loc == -1) {
		printf("a1Input uniform can't be found in fbo shader program\n");
		//return -1;
	}

	GLint filterIndex_loc;
	// this stuff is only in the shader with the visco thermal filter
	if(!absorption) {
		filterIndex_loc = shaderFbo.getUniformLocation("filterIndex");
		if(filterIndex_loc == -1) {
			printf("filterIndex uniform can't be found in fbo shader program\n");
			//return -1;
		}
	}

	// needed for reed, set in setUpPhysicalParams()
	inv_ds_loc = shaderFbo.getUniformLocation("inv_ds");
	if(inv_ds_loc == -1) {
		printf("inv_ds uniform can't be found in fbo shader program\n");
		//return -1;
	}

	if(absorption) {
		z_inv_loc = shaderFbo.getUniformLocation("z_inv");
		if(z_inv_loc == -1) {
			printf("z_inv uniform can't be found in fbo shader program\n");
			//return -1;
		}
	}

	// width map
	GLint widthTexture_loc =  shaderFbo.getUniformLocation("widthTexture");
	if(widthTexture_loc == -1) {
		printf("widthTexture uniform can't be found in fbo shader program\n");
		//return -1;
	}


	// set
	shaderFbo.use();

	// set once for all
	glUniform3f(rates_loc, (float)simulation_rate, (float)simulation_rate*(float)simulation_rate, 1.0/(float)simulation_rate);
	checkError("glUniform1i rates");

	// this stuff is only in the shader with the visco thermal filter
	if(!absorption) {
		glUniform1i(filterIndex_loc, rate_mul-3); // we have 4 specific viscothermal filters
		checkError("glUniform1i filterIndex");
	}

	glUniform1i(widthTexture_loc, nextAvailableTex - GL_TEXTURE0);  // this is the name of the texture in FBO frag shader
	checkError("widthTexture sampler2D problem in fbo shader program\n");


	// width map
	widthTexture.loadPixels(textureWidth, textureHeight, GL_RGBA32F, GL_RGBA, GL_FLOAT, widthTexturePixels, nextAvailableTex++); // when we're done initializing this texture, let's incrase the counter

	// dynamically
	setUpPhysicalParams();
	setUpFilterToggle();
	setUpGlottalParams();
	setUpA1();
	if(absorption)
		setUpWallImpedance();

	glUseProgram (0);

	return 0;
}


int VocalTractFDTD::initShaders() {
	const char* vertex_path_fbo      = {shaders[0].c_str()};
	const char* fragment_path_fbo    = {shaders[1].c_str()};
	const char* vertex_path_render   = {shaders[2].c_str()};
	const char* fragment_path_render = {shaders[3].c_str()};

	// transform file name into path
	for(int i=0; i<numOfExcitationFiles; i++)
		excitationFiles[i] = shaderLoc + excitationFiles[i];
	//-----------------------------------------------------------
	// FBO shader program, first render pass
	//-----------------------------------------------------------
	if(!shaderFbo.init(vertex_path_fbo, fragment_path_fbo, excitationFiles, numOfExcitationFiles))
		return -1;

	//-----------------------------------------------------------
	// render shader program, second pass
	//-----------------------------------------------------------
	if(!shaderRender.init(vertex_path_render, fragment_path_render))
		return -1;

	return 0;
}

void VocalTractFDTD::updateUniforms() {
	// update viscothermal toggle if externally modified
	if(updateFilterToggle) {
		setUpFilterToggle();
		updateFilterToggle = false;
	}

	if(updateGlottalParams) {
		setUpGlottalParams();
		updateGlottalParams = false;
	}

	if(updateZ_inv) {
		setUpWallImpedance();
		updateZ_inv = false;
	}
	ImplicitFDTD::updateUniforms();
}

void VocalTractFDTD::fillWidthTexture(float ***widthMap, float openSpaceWidth) {
	// put width map in bigger matrix, where also PML and dead layers are included
	float ***pixels = new float**[gl_width]; // +1 cos it must include wall of excitation cells [glottis]

	int w_diff = (gl_width-domain_width)/2;
	int h_diff = (gl_height-domain_height)/2;

	// cycle through segments [from left to right]
	for(int i=0; i<gl_width; i++) {
		pixels[i] = new float*[gl_height]; // leave space for first segment, the one that contains excitation cells
		// cycle through cells in segment [from top to bottom]
		for(int j=0; j<gl_height; j++) {
			pixels[i][j] = new float[2]; // up and right only! center is interpolated in shader
			// if PML and extra layers
			if( (i < w_diff || j < h_diff) || (i >= w_diff+domain_width || j >= h_diff+domain_height)) {
				pixels[i][j][0] = openSpaceWidth;
				pixels[i][j][1] = openSpaceWidth;
			}
			else {
				// width
				pixels[i][j][0] = widthMap[i-w_diff][j-h_diff][1];
				pixels[i][j][1] = widthMap[i-w_diff][j-h_diff][2];
			}
		}
	}


	widthTexturePixels = new float[textureWidth*textureHeight*4];

	// texture is divided in quads, representing a state

	// put openspace everywhere
	for(int i=0; i<textureHeight; i++) {
		for(int j=0; j<textureWidth; j++) {
			widthTexturePixels[(i*textureWidth+j)*4]   = openSpaceWidth;
			widthTexturePixels[(i*textureWidth+j)*4+1] = openSpaceWidth;
			widthTexturePixels[(i*textureWidth+j)*4+2] = openSpaceWidth;
			widthTexturePixels[(i*textureWidth+j)*4+3] = openSpaceWidth;
		}
	}

	int state0X = -1;
	int state0Y = -1;
	float xWidthCenter   = -1;
	float xWidthLeft     = -1;
	float yWidthCenter   = -1;
	float yWidthDown     = -1;
	// copy pixels in sate0 and clear out the rest of the texture
	// making sure to offset pixels to skip audio rows at bottom
	for(int i=0; i<textureHeight; i++) {
		for(int j=0; j<textureWidth; j++) {
			if( (i<gl_height) && (j<gl_width) ) {
				// we skip the texture pixels where the audio is + the 1 line that isolates the frame from the audio
				state0Y = i+audioRowH+ISOLATION_LAYER;
				state0X = j;

				xWidthCenter = pixels[j][i][0]; // width of this cell
				// put in all states, as r channel
				widthTexturePixels[(state0Y*textureWidth             + state0X)*4]          = xWidthCenter; // state0
				widthTexturePixels[((state0Y+gl_height)*textureWidth + state0X)*4]          = xWidthCenter; // state1
				widthTexturePixels[((state0Y+gl_height)*textureWidth + state0X+gl_width)*4] = xWidthCenter; // state2
				widthTexturePixels[(state0Y*textureWidth             + state0X+gl_width)*4] = xWidthCenter; // state3

				yWidthCenter = pixels[j][i][1]; // width of this cell
				// put in all states, as b channel
				widthTexturePixels[(state0Y*textureWidth             + state0X)*4+1]          = yWidthCenter; // state0
				widthTexturePixels[((state0Y+gl_height)*textureWidth + state0X)*4+1]          = yWidthCenter; // state1
				widthTexturePixels[((state0Y+gl_height)*textureWidth + state0X+gl_width)*4+1] = yWidthCenter; // state2
				widthTexturePixels[(state0Y*textureWidth             + state0X+gl_width)*4+1] = yWidthCenter; // state3

				// now we want to sample the neighbors
				// if we are at the extremes [left and bottom], instead of the neighbors' value we put value of current cell [it's PML, so we do not care]
				if(j==0)
					xWidthLeft = pixels[j][i][0];
				else
					xWidthLeft = pixels[j-1][i][0];
				// put left neighbor's value in b channel, in all states
				widthTexturePixels[(state0Y*textureWidth             + state0X)*4+2]          = xWidthLeft;
				widthTexturePixels[((state0Y+gl_height)*textureWidth + state0X)*4+2]          = xWidthLeft;
				widthTexturePixels[((state0Y+gl_height)*textureWidth + state0X+gl_width)*4+2] = xWidthLeft;
				widthTexturePixels[(state0Y*textureWidth             + state0X+gl_width)*4+2] = xWidthLeft;

				if(i==0)
					yWidthDown = pixels[j][i][1];
				else
					yWidthDown = pixels[j][i-1][1];
				// put bottom neighbor's value in a channel, in all states
				widthTexturePixels[(state0Y*textureWidth             + state0X)*4+3]          = yWidthDown;
				widthTexturePixels[((state0Y+gl_height)*textureWidth + state0X)*4+3]          = yWidthDown;
				widthTexturePixels[((state0Y+gl_height)*textureWidth + state0X+gl_width)*4+3] = yWidthDown;
				widthTexturePixels[(state0Y*textureWidth             + state0X+gl_width)*4+3] = yWidthDown;



				// fill values for pixel cells in right isolation layer
				if(j==gl_width-1) {
					// higher part of the layer
					widthTexturePixels[((state0Y+gl_height)*textureWidth + state0X+gl_width+WIDTH_TEXT_ISOLATION_LAYER_R)*4]   = openSpaceWidth; // this is outside the domain, on the right, so it's open space
					widthTexturePixels[((state0Y+gl_height)*textureWidth + state0X+gl_width+WIDTH_TEXT_ISOLATION_LAYER_R)*4+1] = openSpaceWidth;    // the left neighbor of the pixel out on the right it's the center pixel!
					widthTexturePixels[((state0Y+gl_height)*textureWidth + state0X+gl_width+WIDTH_TEXT_ISOLATION_LAYER_R)*4+2] = openSpaceWidth; // the bottom neighbor is again outside of the domain, so open space
					widthTexturePixels[((state0Y+gl_height)*textureWidth + state0X+gl_width+WIDTH_TEXT_ISOLATION_LAYER_R)*4+4] = openSpaceWidth;

					// lower part of the layer
					widthTexturePixels[((state0Y)*textureWidth + state0X+gl_width+WIDTH_TEXT_ISOLATION_LAYER_R)*4]   = openSpaceWidth; // this is outside the domain, on the right, so it's open space
					widthTexturePixels[((state0Y)*textureWidth + state0X+gl_width+WIDTH_TEXT_ISOLATION_LAYER_R)*4+1] = openSpaceWidth;    // the left neighbor of the pixel out on the right it's the center pixel!
					widthTexturePixels[((state0Y)*textureWidth + state0X+gl_width+WIDTH_TEXT_ISOLATION_LAYER_R)*4+2] = openSpaceWidth; // the bottom neighbor is again outside of the domain, so open space
					widthTexturePixels[((state0Y)*textureWidth + state0X+gl_width+WIDTH_TEXT_ISOLATION_LAYER_R)*4+3] = openSpaceWidth;
				}

				// fill values for pixel cells in top isolation layer
				if(i==gl_height-1) {
					// left part of the layer
					widthTexturePixels[((state0Y+gl_height+WIDTH_TEXT_ISOLATION_LAYER_U)*textureWidth + state0X)*4]   = openSpaceWidth; // this is outside the domain, on the top, so it's open space
					widthTexturePixels[((state0Y+gl_height+WIDTH_TEXT_ISOLATION_LAYER_U)*textureWidth + state0X)*4+1] = openSpaceWidth; // the left neighbor of the pixel out on the top it's open space too
					widthTexturePixels[((state0Y+gl_height+WIDTH_TEXT_ISOLATION_LAYER_U)*textureWidth + state0X)*4+2] = openSpaceWidth; // the bottom neighbor is the center pixel!
					widthTexturePixels[((state0Y+gl_height+WIDTH_TEXT_ISOLATION_LAYER_U)*textureWidth + state0X)*4+3] = openSpaceWidth;


					// right part of the layer
					widthTexturePixels[((state0Y+gl_height+WIDTH_TEXT_ISOLATION_LAYER_U)*textureWidth + state0X+gl_width)*4]   = openSpaceWidth; // this is outside the domain, on the right, so it's open space
					widthTexturePixels[((state0Y+gl_height+WIDTH_TEXT_ISOLATION_LAYER_U)*textureWidth + state0X+gl_width)*4+1] = openSpaceWidth;    // the left neighbor of the pixel out on the right it's the center pixel!
					widthTexturePixels[((state0Y+gl_height+WIDTH_TEXT_ISOLATION_LAYER_U)*textureWidth + state0X+gl_width)*4+2] = openSpaceWidth; // the bottom neighbor is again outside of the domain, so open space
					widthTexturePixels[((state0Y+gl_height+WIDTH_TEXT_ISOLATION_LAYER_U)*textureWidth + state0X+gl_width)*4+3] = openSpaceWidth;

				}
			}
		}
	}
/*	printf("widthTexturePixels:\n");
	for(int i = 0; i <textureHeight; i++) {
		for(int j = 0; j <textureWidth; j++)
			printf("%f ", widthTexturePixels[(i*textureWidth+j)*4+0]);
		printf("\n");
	}*/


	unsigned int index = 4;

	string component_name="";
	bool pressureW = false;
	if(index==0)
		component_name = "current x";
	else if(index==1)
		component_name = "current y";
	else if(index==2)
		component_name = "left x";
	else if(index==3)
		component_name = "bottom y";
	else {
		component_name = "current p";
		index = 0;
		pressureW = true;
	}

	printf("Visible %s Width Pixels:\n", component_name.c_str())
	;
	for(int i=audioRowH+ISOLATION_LAYER; i<gl_height+audioRowH+ISOLATION_LAYER; i++) {
		for(int j=0; j<gl_width; j++) {

			if(widthTexturePixels[(i*textureWidth+j)*4+index] == openSpaceWidth) {
				if( (widthTexturePixels[((i+1)*textureWidth+j)*4+index] != openSpaceWidth) &&
					(widthTexturePixels[((i-1)*textureWidth+j)*4+index] != openSpaceWidth) )
					printf("%f ", widthTexturePixels[(i*textureWidth+j)*4+index]);
				else
					printf("________ ");
			}
			else if( widthTexturePixels[(i*textureWidth+j)*4+index] <= WALL_WIDTH)//else if( abs(widthTexturePixels[(i*textureWidth+j)*4+index] - WALL_WIDTH) < /*0.001*/MIN_WIDTH)
				printf("######## ");
			else {
				if(!pressureW)
					printf("%f ", widthTexturePixels[(i*textureWidth+j)*4+index]);
				else {
					float pw = (widthTexturePixels[(i*textureWidth+j)*4]+widthTexturePixels[(i*textureWidth+j)*4+1]+widthTexturePixels[(i*textureWidth+j)*4+2]+widthTexturePixels[(i*textureWidth+j)*4+3])/4;
					printf("%f ", pw);
				}
			}

		}
		printf("\n");
	}


	for(int i = gl_width-1; i >=0; i--) {
		for(int j = gl_height-1; j >=0; j--)
			delete[] pixels[i][j];
		delete[] pixels[i];
		//delete pixels4p[i];
	}
	delete[] pixels;
	//delete[] pixels4p;
}

int VocalTractFDTD::parseContourPoints(int domain_size[2], float deltaS, string filename, float ** &points) { // pass by ref cos array is dynamically allocated in method
	ifstream fin;
	// open file
	fin.open(filename.c_str());
	if (fin.good()) {
		printf("\tParsing file %s\n", filename.c_str());
	}
	else {
		printf("\t*Parser Error: file %s not found*\n", filename.c_str());
		return -1;
	}

	// working vars
	char * token;               // where to save single figures from file
	string s     = "";          // where to save lines from file
	string s_tmp = "";          // where to work with lines
	bool wellFormatted = false;


	// number of segments
	int numOfSeg    = -1;
	int numOfPoints = -1;

	getline(fin, s); 					     // get first line
	token = strtok((char *)s.c_str(), " ");  // look for number of segments "nSegments = x"

	if(token) {
		token = strtok(0, " ");
		// check if second token is there "="
		if(token) {
			token = strtok(0, " ");
			// then check if number if is there
			if(token) {
				numOfSeg = atoi(token);
				wellFormatted = true;
			}
		}
	}



	if(wellFormatted) {
		printf("\t\t-Number of segments: %d\n", numOfSeg);
		numOfPoints = 2*numOfSeg;
		points = new float *[numOfPoints];
		for(int i=0; i<numOfPoints; i++)
			points[i] = new float[2];
	}
	else {
		printf("*Parser Error: wrong format*\n");
		printf("\t-Can't parse number of segments\n");
		return -2;
	}


	float maxX = -1000;
	float maxY = -1000;

	// segments
	wellFormatted = false;

	getline(fin, s); // eat empty line


	for(int i=0; i<numOfSeg; i++) {
		getline(fin, s);                            // 2 points, in this format: "x1,y1;x2,y2"
		s_tmp = string(s.c_str());					// local copy of string cos strtok freaking modifies the original string!
		token = strtok((char *)s_tmp.c_str(), ";"); // first point, in this format: "x1,y1"
		if(token) {
			token = strtok(token, ",");
			// check if x1 is there
			if(token) {
				points[2*i][0] = atof(token);
				token = strtok(0, ",");
				if(points[2*i][0] > maxX)
					maxX = points[2*i][0];
				// check if y1 is there
				if(token) {
					points[2*i][1] = atof(token);
					if(points[2*i][1] > maxY)
						maxY = points[2*i][1];
					wellFormatted  = true;
					//printf("1 - %f, %f\n", points[2*i][0], points[2*i][1]);
				}
			}
		}

		if(wellFormatted) {
			wellFormatted = false;
			token = strtok((char *)s.c_str(), ";"); // again first point, in this format: "x1,y1"... [using original unchanged copy of string]
			token = strtok(0, ";"); //...second point, in this format: "x2,y2"
			if(token) {
				token = strtok(token, ",");
				// check if x2 is there
				if(token) {
					points[2*i + 1][0] = atof(token);
					if(points[2*i + 1][0] > maxX)
						maxX = points[2*i + 1][0];
					token = strtok(0, ",");
					// check if y2 is there
					if(token) {
						points[2*i + 1][1] = atof(token);
						if(points[2*i + 1][1] > maxY)
							maxY = points[2*i + 1][1];
						wellFormatted  = true;
						//printf("2 - %f, %f\n", points[2*i +1 ][0], points[2*i +1][1]);
					}
				}
			}
		}
		if(!wellFormatted) {
			printf("*Parser Error: wrong format*\n");
			printf("\t-Can't parse segment's points at line %d of file [counting starts from 0!]\n", i+2);
			return -3;
		}
	}



	fin.close();

	printf("\tParsing of file %s went well (:\n", filename.c_str());
	printf("\t--------------------------------------------------\n");

	int maxRow = round(maxY/deltaS);
	int maxCol = round(maxX/deltaS);
	if(maxRow>=domain_size[1]) {
		printf("The chosen contour needs at least a domain %d pixels high, while domain is only %d pixels\n", maxRow+1, domain_size[1]);
		return -4;
	}
	else if(maxCol>=domain_size[0]) {
		printf("The chosen contour needs at least a domain %d pixels wide, while domain is only %d pixels\n", maxCol+1, domain_size[0]);
		return -5;
	}

	return numOfPoints;
}


int VocalTractFDTD::parseContourPoints_new(int domain_size[2], float deltaS, string filename, float ** &lowerContour, float ** &upperContour, float * &angles) {
	ifstream fin;
	// open file
	fin.open(filename.c_str());
	if (fin.good()) {
		printf("\tParsing file %s\n", filename.c_str());
	}
	else {
		printf("\t*Parser Error: file %s not found*\n", filename.c_str());
		return -1;
	}

	int numOfPoints = -1;

	// working vars
	char *token;               // where to save single figures from file
	string s     = "";          // where to save lines from file
	bool wellFormatted = false;


	// get first line, which should be num of points in each contour line [lower and upper]
	getline(fin, s);
	token = (char *) s.c_str();

	// check
	if(token) {
		numOfPoints = atoi(token);
		wellFormatted = true;
	}

	if(wellFormatted) {
		printf("\t\t-Number of points: %d\n", numOfPoints);
		lowerContour = new float *[numOfPoints];
		upperContour = new float *[numOfPoints];
		angles       = new float[numOfPoints];
		for(int i=0; i<numOfPoints; i++) {
			lowerContour[i] = new float[2];
			upperContour[i] = new float[2];
		}
	}
	else {
		printf("*Parser Error: wrong format*\n");
		printf("\t-Can't parse number of points\n");
		return -2;
	}

	float maxX = -1000;
	float maxY = -1000;

	//-----------------------------------------------------------------
	// lower contour
	//-----------------------------------------------------------------
	// this line is supposed to have all points coordinates separated by spaces, like:  "x0 y0 x1 y1 x2 y2..."
	getline(fin, s);
	token = strtok((char *)s.c_str(), " "); // first x coord
	if(token) {
		lowerContour[0][0] = atof(token)/1000.0;
		// check for max on x
		if(lowerContour[0][0]>maxX)
			maxX = lowerContour[0][0];

		token = strtok(0, " "); // first y coord
		if(token)
			lowerContour[0][1] = atof(token)/1000.0;
		else
			wellFormatted = false;
	}
	else
		wellFormatted = false;


	if(wellFormatted) {
		// all remaining coords
		for(int i=1; i<numOfPoints; i++) {
			// next x coord
			token = strtok(0, " ");
			if(token) {
				lowerContour[i][0] = atof(token)/1000.0;
				// check for max on x
				if(lowerContour[i][0]>maxX)
					maxX = lowerContour[i][0];
			}
			else {
				wellFormatted = false;
				break;
			}

			// next y coord
			token = strtok(0, " "); // next coord
			if(token)
				lowerContour[i][1] = atof(token)/1000.0;
			else {
				wellFormatted = false;
				break;
			}
		}
	}

	if(!wellFormatted) {
		printf("*Parser Error: wrong format*\n");
		printf("\t-Can't parse lowerContour\n");
		return -3;
	}
	//-----------------------------------------------------------------


	//-----------------------------------------------------------------
	// upper contour
	//-----------------------------------------------------------------
	// this line is supposed to have all points coordinates separated by spaces, like:  "x0 y0 x1 y1 x2 y2..."
	getline(fin, s);
	token = strtok((char *)s.c_str(), " "); // first x coord
	if(token) {
		upperContour[0][0] = atof(token)/1000.0;

		token = strtok(0, " "); // first y coord
		if(token) {
			upperContour[0][1] = atof(token)/1000.0;
			// check for max on y
			if(upperContour[1][0]>maxY)
				maxY = upperContour[1][0];
		}
		else
			wellFormatted = false;
	}
	else
		wellFormatted = false;


	if(wellFormatted) {
		// all remaining coords
		for(int i=1; i<numOfPoints; i++) {
			// next x coord
			token = strtok(0, " ");
			if(token)
				upperContour[i][0] = atof(token)/1000.0;
			else {
				wellFormatted = false;
				break;
			}

			// next y coord
			token = strtok(0, " "); // next coord
			if(token) {
				upperContour[i][1] = atof(token)/1000.0;
				// check for max on y
				if(upperContour[i][1]>maxY)
					maxY = upperContour[i][1];
			}
			else {
				wellFormatted = false;
				break;
			}
		}
	}

	if(!wellFormatted) {
		printf("*Parser Error: wrong format*\n");
		printf("\t-Can't parse upperContour\n");
		return -3;
	}
	//-----------------------------------------------------------------


	//-----------------------------------------------------------------
	// angles
	//-----------------------------------------------------------------
	// this line is supposed to have all angles for each couple of lower and upper contour coordinates separated by spaces, like:  "a0 a1 a2..."
	getline(fin, s);
	token = strtok((char *)s.c_str(), " "); // first angle
	if(token)
		angles[0] = atof(token);
	else
		wellFormatted = false;

	if(wellFormatted) {
		// all remaining angles
		for(int i=1; i<numOfPoints; i++) {
			// next angle
			token = strtok(0, " ");
			if(token)
				angles[i] = atof(token);
			else {
				wellFormatted = false;
				break;
			}
		}
	}

	if(!wellFormatted) {
		printf("*Parser Error: wrong format*\n");
		printf("\t-Can't parse angles\n");
		return -4;
	}
	//-----------------------------------------------------------------

	fin.close();

	printf("\tParsing of file %s went well (:\n", filename.c_str());
	printf("\t--------------------------------------------------\n");

	// check if with current ds we have enough domain pixels to draw the whole contour
	int maxRow = round(maxY/deltaS);
	int maxCol = round(maxX/deltaS);
	if(maxRow>=domain_size[1]) {
		printf("The chosen contour needs at least a domain %d pixels high, while domain is only %d pixels\n", maxRow+1, domain_size[1]);
		return -4;
	}
	else if(maxCol>=domain_size[0]) {
		printf("The chosen contour needs at least a domain %d pixels wide, while domain is only %d pixels\n", maxCol+1, domain_size[0]);
		return -5;
	}

	return numOfPoints;
}


void VocalTractFDTD::pixelateContour(int numOfPoints, float **contour, float *angles, float deltaS, vector<int> &coords, int min[2], bool lower) {
	// fill pixels
	// if there is a gap, fill it oversampling the segment
	int row0 = -1;
	int col0 = -1;
	int row1 = -1;
	int col1 = -1;
	int gap[3] = {0, 0, 0};
	float delta[2] = {0, 0};

	//VIC clean this, obscure it, comment it and turn it into a function
	// then calculate a1 in loadContourFile_new()
	int shiftRow = 0;
	int shiftCol = 0;


	// in order of likelyhood
	if(angles[0]>=180-22.5 && angles[0]<180+22.5) {     // 180
		shiftRow = 0;
		shiftCol = (!lower) ? -1 : 0;
	}
	else if(angles[0]>=90-22.5 && angles[0]<90+22.5) { 	// 90
		shiftRow = (lower) ? -1 : 0;
		shiftCol = 0;
	}
	else if(angles[0]>=135-22.5 && angles[0]<135+22.5) { // 135
		shiftRow = (lower) ? -1 : 0;
		shiftCol = (!lower) ? -1 : 0;
	}
	else if(angles[0]>=225-22.5 && angles[0]<225+22.5) { // 225
		shiftRow = (!lower) ? -1 : 0;
		shiftCol = (!lower) ? -1 : 0;
	}
	else if(angles[0]>=45-22.5 && angles[0]<45+22.5) {   // 45
		shiftRow = (lower) ? -1 : 0;
		shiftCol = (lower) ? -1 : 0;
	}
	else if(angles[0]>=270-22.5 && angles[0]<270+22.5) {  // 270
		shiftRow = (!lower) ? -1 : 0;
		shiftCol = 0;
	}
	else if(angles[0]>=0-22.5 && angles[0]<0+22.5) {       // 0
		shiftRow = 0;
		shiftCol = (lower) ? -1 : 0;
	}
	else if(angles[0]>=0-22.5 && angles[0]<0+22.5) {      // 315
		shiftRow = (!lower) ? -1 : 0;
		shiftCol = (lower) ? -1 : 0;
	}

	// first set of coord
	row0 = shiftAndRound(shiftRow, contour[0][1] / deltaS);
	col0 = shiftAndRound(shiftCol, contour[0][0] / deltaS);

	// save it
	coords.push_back(row0);
	coords.push_back(col0);

	if(row0<min[0])
		min[0] = row0;
	if(col0<min[1])
		min[1] = col0;

	// save all the others checking if there are gaps with previous one
	for(int i=0; i<numOfPoints-1; i++){
		// previous
		row0 = shiftAndRound(shiftRow, contour[i][1] / deltaS);
		col0 = shiftAndRound(shiftCol, contour[i][0] / deltaS);

		// current
		row1 = shiftAndRound(shiftRow, contour[i+1][1] / deltaS);
		col1 = shiftAndRound(shiftCol, contour[i+1][0] / deltaS);

		// save current set of coord
		coords.push_back(row1);
		coords.push_back(col1);

		if(row1<min[0])
			min[0] = row1;
		if(col1<min[1])
			min[1] = col1;

		// gaps with previous
		gap[0] = abs(col0-col1);
		gap[1] = abs(row0-row1);
		// if there is a gap...
		if( gap[0]>1 || gap[1]>1) {
			gap[2] = max(gap[0], gap[1]); // gap size is in pixels
			gap[2]++;

			// ...oversample the segment so that it is divided into gap+1 sub segments
			delta[0] = ((contour[i+1][0]-contour[i][0])/gap[2]); // delta columns
			delta[1] = ((contour[i+1][1]-contour[i][1])/gap[2]); // dealt rows

			// simply turn new samples into pixels and save their coords
			for(int j=1;j<=gap[2]; j++) {
				// new set
				row0 = shiftAndRound(shiftRow,  (contour[i][1]+delta[1]*j) / deltaS);
				col0 = shiftAndRound(shiftCol,      (contour[i][0]+delta[0]*j) / deltaS);

				// save it
				coords.push_back(row0);
				coords.push_back(col0);

				if(row0<min[0])
					min[0] = row0;
				if(col0<min[1])
					min[1] = col0;
			}
		}
	}
	//VIC: problem is
	// if ds too low, constrictions are welded.. can't much about it /:
}


void VocalTractFDTD::drawContour(float ***domainPixels, vector<int> lowerCoords, vector<int> upperCoords, int shift[2]) {
	// lower contour
	for(unsigned int i=0; i<lowerCoords.capacity(); i+=2){
		// first columns than rows!
		domainPixels[lowerCoords[i+1]-shift[1]][lowerCoords[i]-shift[0]][3] = ImplicitFDTD::getPixelAlphaValue(cell_wall);
	}
	// upper contour
	for(unsigned int i=0; i<upperCoords.capacity(); i+=2){
		// first columns than rows!
		domainPixels[upperCoords[i+1]-shift[1]][upperCoords[i]-shift[0]][3] = ImplicitFDTD::getPixelAlphaValue(cell_wall);
	}
}
double VocalTractFDTD::shiftAndRound(int shift, double val) {
	return round(val+shift);
}
