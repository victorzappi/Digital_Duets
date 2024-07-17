/*
 * VocalTractFDTD.cpp
 *
 *  Created on: 2016-03-14
 *      Author: vic
 */

#include "VocalTractFDTD.h"

// file parser
#include <fstream>      // load file
#include <stdlib.h>	    // atoi, atof
#include <string.h>     // string tok
#include <math.h>       // round
#include <vector>

#include "pitch_shift.h"

//TODO move back absorption and visco thermal stuff in ImplicitFDTD, when we understand more about the differences between the 2 filters
VocalTractFDTD::VocalTractFDTD() {
	name = "FDTD Vocal Tract + Glottal Model";

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
	q   = 0.8;//0.67;        // dimensionless //@VIC it was 1
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
	alpha      = /*0.01;*/ 0.008;/*0.005;*///0.0198 is equal to 0.005 admittance
}

VocalTractFDTD::~VocalTractFDTD() {
	delete excitationFiles;
	delete excitationMods.names;
}

int VocalTractFDTD::init(string shaderLocation, int *domainSize, int *excitationCoord, int *excitationDimensions,
				         float ***domain, int audio_rate, int rate_mul, int periodSize, float mag, int *listCoord, float exLpF) {
	absorption = (shaderLocation.compare("shaders_VocalTract_abs")==0);
	return ImplicitFDTD::init(shaderLocation, domainSize, excitationCoord, excitationDimensions, excitationMods,
			domain, audio_rate, rate_mul, periodSize, mag, listCoord, exLpF);
}
int VocalTractFDTD::init(string shaderLocation, int *domainSize, int audio_rate, int rate_mul, int periodSize,
		                 float mag, int *listCoord, float exLpF) {
	absorption = (shaderLocation.compare("shaders_VocalTract_abs")==0);
	return ImplicitFDTD::init(shaderLocation, excitationMods, domainSize, audio_rate, rate_mul, periodSize, mag, listCoord, exLpF);
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

int VocalTractFDTD::loadContourFile(int domain_size[2], float deltaS, string filename, float ***domainPixels) {

	float **points = NULL;

	int numOfPoints = parseContourPoints(domain_size, deltaS, filename, points);

	if(numOfPoints < 0) {
		if(points != NULL) {
			for(int i=0; i<numOfPoints; i++)
				delete points[i];
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
		delete points[i];
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
				delete lowerContour[i];
			delete[] lowerContour;
		}
		if(upperContour != NULL) {
			for(int i=0; i<numOfPoints; i++)
				delete upperContour[i];
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
		delete lowerContour[i];
	delete[] lowerContour;

	for(int i=0; i<numOfPoints; i++)
		delete upperContour[i];
	delete[] upperContour;

	return 0; //TODO this thing should return a1, but also needs glottal position in contour file
}

//-------------------------------------------------------------------------------------------
// private
//-------------------------------------------------------------------------------------------
void VocalTractFDTD::initPhysicalParameters() {
    c       = C_VT;
    rho     = RHO_VT;
    mu      = MU_VT;
    prandtl = PRANDTL_VT;
    gamma   = GAMMA_VT;

    printf("\n");
    printf("\tc: \t%f\n", c);
    printf("\trho: \t%f\n", rho);
    printf("\tmu: \t%f\n", mu);
    printf("\tprandlt: \t%f\n", prandtl);
    printf("\tgamma: \t%f\n", gamma);

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

	GLint filterIndex_loc = -1;
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
