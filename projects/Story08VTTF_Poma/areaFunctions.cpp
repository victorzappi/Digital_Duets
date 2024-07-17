/*
 * areaFunctions.cpp
 *
 *  Created on: 2016-06-03
 *      Author: Victor Zappi
 */

#include "ImplicitFDTD.h"
#include <math.h> // M_PI


extern int excitationCoord[2];
extern int excitationDim[2];
extern float spacing_factor;
extern bool auto_spacing_correction;
extern float radii_factor;
extern bool auto_radii_correction;

//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------

//-------------------------------------------------------
// story 08 vowels, all in cm
//-------------------------------------------------------

// these are freaking segments, not area samples as in Fant!

float story08Data_a_spacing = 0.388*spacing_factor; // in Story 08 each vowel is sampled with different delta, but they're all 44 segments
float story08Data_a[44] = { 0.56, 0.62, 0.66, 0.78, 0.97,
					      1.16, 1.12, 0.82, 0.55, 0.45,
					      0.37, 0.29, 0.21, 0.15, 0.16,
					      0.25, 0.34, 0.43, 0.54, 0.61,
					      0.67, 0.98, 1.76, 2.75, 3.52,
					      4.08, 4.74, 5.61, 6.60, 7.61,
					      8.48, 9.06, 9.29, 9.26, 9.06,
					      8.64, 7.91, 6.98, 6.02, 5.13,
					      4.55, 4.52, 4.71, 4.72 };
float story08Data_a_len = story08Data_a_spacing*44;//0.1709;
float story08Data_a_formants[8] = {696, 1068, 3031, 4064, 5039, 5703, 7065, 7595};

float story08Data_i_spacing = 0.384*spacing_factor;
float story08Data_i[44] = { 0.51, 0.59, 0.62, 0.72, 1.24,
						  2.30, 3.30, 3.59, 3.22, 2.86,
						  3.00, 3.61, 4.39, 4.95, 5.17,
						  5.16, 5.18, 5.26, 5.20, 5.02,
						  4.71, 4.13, 3.43, 2.83, 2.32,
						  1.83, 1.46, 1.23, 1.08, 0.94,
						  0.80, 0.67, 0.55, 0.46, 0.40,
						  0.36, 0.35, 0.35, 0.38, 0.51,
						  0.74, 0.92, 0.96, 0.91 };
float story08Data_i_len = story08Data_i_spacing*44;//0.1690;
float story08Data_i_formants[8] = {263, 2111, 3010, 4138, 5019, 5753, 6575, 7645};

float story08Data_u_spacing = 0.445*spacing_factor;
float story08Data_u[44] = { 0.54, 0.61, 0.66, 0.75, 1.13,
						  1.99, 2.83, 2.90, 2.52, 2.40,
						  2.83, 3.56, 3.99, 3.89, 3.50,
						  3.04, 2.64, 2.44, 2.31, 2.07,
						  1.80, 1.52, 1.14, 0.74, 0.42,
						  0.22, 0.14, 0.20, 0.47, 0.89,
						  1.15, 1.42, 2.17, 3.04, 3.69,
						  4.70, 5.74, 5.41, 3.82, 2.34,
						  1.35, 0.65, 0.29, 0.16 };
float story08Data_u_len = story08Data_u_spacing*44;//0.1959;
float story08Data_u_formants[8] = {259, 757, 2264, 3603, 4173, 5062, 6126, 6594};

//-------------------------------------------------------
// story 96 vowels, all in cm
//-------------------------------------------------------
float story96Data_spacing = 0.396825*spacing_factor; // in Story 96 all vowels are sampled every 0.00396825 m

// these are freaking segments, not area samples as in Fant!
float story96Data_a[44] = { 0.45, 0.20, 0.26, 0.21, 0.32,
					      0.30, 0.33, 1.05, 1.12, 0.85,
					      0.63, 0.39, 0.26, 0.28, 0.23,
					      0.32, 0.29, 0.28, 0.40, 0.66,
					      1.20, 1.05, 1.62, 2.09, 2.56,
					      2.78, 2.86, 3.02, 3.75, 4.60,
					      5.09, 6.02, 6.55, 6.29, 6.27,
					      5.94, 5.28, 4.70, 3.87, 4.13,
					      4.25, 4.27, 4.69, 5.03 };
float story96Data_a_len = story96Data_spacing*44;//0.1746;

float story96Data_i[42] = { 0.33, 0.30, 0.36, 0.34, 0.68,
						  0.50, 2.43, 3.15, 2.66, 2.49,
						  3.39, 3.80, 3.78, 4.35, 4.50,
						  4.43, 4.68, 4.52, 4.15, 4.09,
						  3.51, 2.95, 2.03, 1.66, 1.38,
						  1.05, 0.60, 0.35, 0.32, 0.12,
						  0.10, 0.16, 0.25, 0.24, 0.38,
						  0.28, 0.36, 0.65, 1.58, 2.05,
						  2.01, 1.58 };
float story96Data_i_len = story96Data_spacing*42;//0.1667;

float story96Data_u[46] = { 0.40, 0.38, 0.28, 0.43, 0.55,
						  1.72, 2.91, 2.88, 2.37, 2.10,
						  3.63, 5.86, 5.63, 5.43, 4.80,
						  4.56, 4.29, 3.63, 3.37, 3.16,
						  3.31, 3.22, 2.33, 2.07, 2.07,
						  1.52, 0.74, 0.23, 0.15, 0.22,
						  0.22, 0.37, 0.60, 0.76, 0.86,
						  1.82, 2.35, 2.55, 3.73, 5.47,
						  4.46, 2.39, 1.10, 0.77, 0.41,
						  0.86 };
float story96Data_u_len = story96Data_spacing*46;//0.1825;


//-------------------------------------------------------
// fant vowels, all this shit is in cm
//-------------------------------------------------------
float fantData_spacing = 0.5*spacing_factor; // in Fant all vowels are sampled every .5 cm

float fantData_a[35] = {
						5,   	5,  	5,   	5,
						6.5, 	8,   	8,   	8,
						8,   	8,   	8, 		8,
						8,   	6.5, 	5,   	4,
						3.2, 	1.6, 	2.6, 	2.6,
						2,   	1.6, 	1.3, 	1,
						.65, 	.65, 	.65, 	1,
						1.6, 	2.6, 	4,   	1,
						1.3, 	1.6, 	2.6
					};
float fantData_a_len = 34*fantData_spacing;

float fantData_o[38] = {
						3.2,	3.2,	3.2,	3.2,
						6.5,	13,		13, 	16,
						13, 	10.5, 	10.5,	8,
						8, 		6.5, 	6.5, 	5,
						5, 		4, 		3.2, 	2,
						1.6, 	2.6, 	1.3,	.65,
						.65, 	1, 		1, 		1.3,
						1.6, 	2, 		3.2, 	4,
						5, 		5, 		1.3, 	1.3,
						1.6, 	2.6
					};
float fantData_o_len = 37*fantData_spacing;


float fantData_u[40] = {
						.65, 	.65, 	.32, 	.32,
						2, 		5, 		10.5, 	13,
						13, 	13, 	13, 	10.5,
						8, 		6.5, 	5, 		3.2,
						2.6, 	2, 		2, 		2,
						1.6, 	1.3, 	2, 		1.6,
						1, 		1, 		1, 		1.3,
						1.6, 	3.2, 	5, 		8,
						8, 		10.5, 	10.5, 	10.5,
						2, 		2, 		2.6, 	2.6
					};
float fantData_u_len = 39*fantData_spacing;


float fantData_i_[39] = {
						6.5, 	6.5, 	2, 		6.5,
						8, 		8, 		8, 		5,
						3.2,	2.6, 	2, 		2,
						1.6, 	1.3, 	1, 		1,
						1.3, 	1.6, 	2.6, 	2,
						4, 		5, 		6.5, 	6.5,
						8,		10.5, 	10.5, 	10.5,
						10.5, 	10.5, 	13, 	13,
						10.5, 	10.5, 	6, 		3.2,
						3.2, 	3.2,	3.2
					};
float fantData_i__len = 38*fantData_spacing;


float fantData_i[34] = {
						4, 		4, 		3.2, 	1.6,
						1.3, 	1, 		.65, 	.65,
						.65,	.65, 	.65, 	.65,
						.65, 	1.3, 	2.6, 	4,
						6.5, 	8, 		8, 		10.5,
						10.5, 	10.5, 	10.5,	10.5,
						10.5, 	10.5, 	10.5, 	10.5,
						8,	 	8, 		2, 		2,
						2.6, 	3.2
					};
float fantData_i_len = 33*fantData_spacing;

float fantData_e[34] = {
						8, 		8, 		5, 		5,
						4, 		2.6, 	2, 		2.6,
						2.6, 	3.2,	4, 		4,
						4, 		5, 		5, 		6.5,
						8, 		6.5, 	8, 		10.5,
						10.5, 	10.5, 	10.5, 	10.5,
						8, 		8,		6.5, 	6.5,
						6.5, 	6.5, 	1.3, 	1.6,
						2, 		2.6
					};
float fantData_e_len = 33*fantData_spacing;
//----------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------



void applyChamberExpansion(/*in/out*/float *diameters, float *s_3d, int numOfSeg) {
	float a_2d[numOfSeg]; // regular 2D radii, while the input parameter s_3d are 3D areas!

	float old_r[numOfSeg];

	// look for segment with largest cross section
	float a_3d_d = -1;
	int d = -1;
	for(int i=0; i<numOfSeg; i++) {
		a_2d[i] = diameters[i]/2;
		old_r[i] = diameters[i];
		if(a_2d[i]>a_3d_d) {
			a_3d_d = a_2d[i];
			d = i;
		}
	}

	float cross = 1;
	// this is the first corrected radius!
	a_2d[d] = cross*(a_3d_d * (0.5*M_PI)/1.84) + (1-cross)*old_r[d]/2; //0.8536

	// now we correct all the other ones and transform them into diameters
	for(int i=d-1; i>=0; i--) {
		a_2d[i] =  cross*(a_2d[i+1] * s_3d[i]/s_3d[i+1]) + (1-cross)*old_r[i]/2;
		diameters[i] = a_2d[i]*2;
/*		if(diameters[i]>0.2)
			diameters[i] = 2*old_r[i];*/
	}

	for(int i=d+1; i<numOfSeg; i++) {
		a_2d[i] = cross*(a_2d[i-1] * s_3d[i]/s_3d[i-1]) + (1-cross)*old_r[i]/2;
		diameters[i] = a_2d[i]*2;
/*		if(diameters[i]>0.2)
			diameters[i] = 2*old_r[i];*/
	}

	diameters[d] = a_2d[d]*2;

/*
	for(int i=0; i<numOfSeg; i++) {
		printf("diff[%d]: %f\n", i, old_r[i]-diameters[i]);
	}

	printf("newR = [");
	for(int i=0; i<numOfSeg-1; i++) {
		printf("%f, ", diameters[i]/2);
	}
	printf("%f]\n", diameters[numOfSeg-1]/2);
*/

/*	for(int i=0; i<numOfSeg; i++) {
		diameters[i] = 2 * old_r[i] * (0.5*M_PI)/1.84;
	}*/

/*
	float min = 1000;
	int argmin = -1;
	for(int i=0; i<numOfSeg; i++) {
		if(s_3d[i]<min) {
			min = s_3d[i];
			argmin = i;
		}
	}
	printf("min d: %f\n", sqrt(min/M_PI)*2);
	printf("min area: %f\n", min);
	printf("min section: %d\n", argmin);

	min = 1000;
	argmin = -1;
	for(int i=0; i<numOfSeg; i++) {
		if(diameters[i]<min) {
			min = diameters[i];
			argmin = i;
		}
	}
	printf("min d: %f\n", min);
	printf("min area: %f\n", (min/2)*(min/2)*M_PI);
	printf("min section: %d\n", argmin);
*/

}

void fromAreasToDiameters(float *areas_in, int nSeg, float *diameters_out, bool flip=true) {
	if(flip) {
		for(int i=0; i<nSeg; i++) {
			diameters_out[nSeg-1-i] = areas_in[i]/M_PI;
			diameters_out[nSeg-1-i] = sqrt(diameters_out[nSeg-1-i])*2*radii_factor;
		}
	}
	else {
		for(int i=0; i<nSeg; i++) {
			diameters_out[i] = areas_in[i]/M_PI;
			diameters_out[i] = sqrt(diameters_out[i])*2*radii_factor;
			//diameters_out[i] = areas_in[i];
			//printf("%d - %f\n", i, diameters_out[i]/2.0);
		}
	}

	if(auto_radii_correction)
		applyChamberExpansion(diameters_out, areas_in, nSeg);
}

float interpolateArea(float *tract, float len, int nSegments, float x_pos, float seg_space) {
	int segment = floor( x_pos / (len/(nSegments-1)) );

	float y0  = tract[segment];
	float x0 = segment*seg_space;
	float y1 = tract[segment+1];
	float x1 = (segment+1)*seg_space;

	float a = (x_pos-x0)/(x1-x0);
	float b = (x1-x_pos)/(x1-x0);

	float retval = y0*(1-a) + y1*(1-b);
	return retval;
}


inline float resampleArea(float *tract, float len, int nSegments, float x_pos, float seg_space) {
	int segment = floor( x_pos / (len/nSegments) );
	segment = (segment>nSegments-1) ? nSegments-1 : segment;
	return tract[segment];
}

bool createSymmetricTubeDomain(int tract[], int tract_s, int domainSize[], float ***domainPixels, bool residue=false) {
	if(tract_s+1 > domainSize[0]) {
		printf("The specified tube needs at least a domain %d pixels long, while domain is only %d pixels\n", tract_s+1, domainSize[0]);
		return false;
	}

	float wallVal = ImplicitFDTD::getPixelAlphaValue(ImplicitFDTD::cell_wall);

	int jumpH = 0;
	int jumpL = 0;
	int shift[tract_s];
	int res[tract_s];

	int contourPos[2] = {0, 0};
	printf("Tube's length: %d [not including excitation]\n", tract_s);
	printf("Segments' widths:");
	for (int i=0; i<tract_s; i++) {
		shift[i] = (tract[i]-1)/2;
		//VIC WTF!? (tract[i]%2!=0)
		res[i] = (!residue) ? 0 : (tract[i]%2==0); // !residue tells us to force a perfect symmetric representation [residue=false, more error] or not [residue=true]

		contourPos[0] = excitationCoord[1]+1+res[i]+shift[i];
		contourPos[1] = excitationCoord[1]-1-shift[i];

		printf("\t%d", contourPos[0]-contourPos[1]-1);

		if(contourPos[0] > domainSize[1]) {
			printf("So far the specified tube needs at least a domain %d pixels high, while domain is only %d pixels\n", contourPos[0], domainSize[1]);
			return false;
		}
		else if(contourPos[1] < 0) {
			printf("Please either enlarge the domain or move the excitation up, both of at least %d pixels\n", -contourPos[1]);
			return false;
		}

		ImplicitFDTD::fillTextureColumn( excitationCoord[0]+i+1, excitationCoord[1]+1+res[i]+shift[i], contourPos[0], wallVal, domainPixels);
		ImplicitFDTD::fillTextureColumn( excitationCoord[0]+i+1, excitationCoord[1]-1-shift[i], contourPos[1], wallVal, domainPixels);

		if (i>0) {
			jumpH = shift[i]+res[i] - shift[i-1]-res[i-1];
			jumpL = shift[i]- shift[i-1];

			// upper contour
			// i am taller than the previous wall cell
			if(jumpH > 1) {
				// put extra wall on top of prev neighbor on top side of tube
				ImplicitFDTD::fillTextureColumn(  excitationCoord[0]+i, excitationCoord[1]+1+shift[i-1]+res[i-1], excitationCoord[1]+shift[i-1]+res[i-1]+jumpH, wallVal, domainPixels);
			}
			// i am shorter than the previous wall cell
			else if (jumpH < -1) {
				jumpH = -jumpH;
				// put extra wall on top of this pixel on top side of tube
				ImplicitFDTD::fillTextureColumn(  excitationCoord[0]+i+1, excitationCoord[1]+1+shift[i]+res[i], excitationCoord[1]+shift[i]+res[i]+jumpH, wallVal, domainPixels);
			}

			// lower contour
			// i am taller than the previous wall cell
			if(jumpL > 1)// put extra wall below prev neighbor on lower side of tube
				ImplicitFDTD::fillTextureColumn(  excitationCoord[0]+i, excitationCoord[1]-shift[i-1]-jumpL, excitationCoord[1]-1-shift[i-1], wallVal, domainPixels);
			// i am shorter than the previous wall cell
			else if(jumpL < -1) { // put extra wall below this pixel on lower side of tube
				jumpL = -jumpL;
				ImplicitFDTD::fillTextureColumn(  excitationCoord[0]+i+1, excitationCoord[1]-shift[i]-jumpL, excitationCoord[1]-1-shift[i], wallVal, domainPixels);
			}
		}
	}

	printf("\n");

/*	// close the fucking tube
	// with a wall of excitations...
	float excitationVal = ImplicitFDTD::cell_excitation;
	ImplicitFDTD::fillTextureColumn( excitationCoord[0], excitationCoord[1]+excitationDim[1],  excitationCoord[1]+excitationDim[1]+shift[0]+res[0]-1, excitationVal, domainPixels);
	ImplicitFDTD::fillTextureColumn( excitationCoord[0], excitationCoord[1]-shift[0],  excitationCoord[1]-1, excitationVal, domainPixels);*/
	// ...and 2 wall cells on corners
	ImplicitFDTD::fillTexturePixel( excitationCoord[0],  excitationCoord[1]+excitationDim[1]+shift[0]+res[0], wallVal, domainPixels);
	ImplicitFDTD::fillTexturePixel( excitationCoord[0],  excitationCoord[1]-shift[0]-1, wallVal, domainPixels);

	//@VIC
	// open space
	if(true) {
		// Dirichlet [Peter Gustav Lejeune] uniform boundary condition with p=0
		float noPressVal = ImplicitFDTD::getPixelAlphaValue(ImplicitFDTD::cell_nopressure);
		ImplicitFDTD::fillTextureColumn( excitationCoord[0]+tract_s+1, excitationCoord[1]-shift[tract_s-1]-1,
										excitationCoord[1]+res[tract_s-1]+shift[tract_s-1]+1, noPressVal, domainPixels);
	}
	return true;
}

bool createUpwardsTubeDomain(int tract[], int tract_s, int domainSize[], float ***domainPixels) {
	if(tract_s+1 > domainSize[0]) {
		printf("The specified tube needs at least a domain %d pixels long, while domain is only %d pixels\n", tract_s+1, domainSize[0]);
		return false;
	}

	float wallVal = ImplicitFDTD::getPixelAlphaValue(ImplicitFDTD::cell_wall);
	int jump = 0;
	int bottom = excitationCoord[1] - (tract[0]+1)/2; // flat bottom line y coord
	// draw flat bottom line
	ImplicitFDTD::fillTextureRow( bottom, excitationCoord[0], excitationCoord[0]+tract_s, wallVal, domainPixels);

	int contourPos = 0;
	int step = -1;
	for (int i=0; i<tract_s; i++) {
		step = (tract[i]>0) ? tract[i] : 1;
		contourPos = bottom+step+1;
		if(contourPos > domainSize[1]) {
			printf("So far the specified tube needs at least a domain %d pixels high, while domain is only %d pixels\n", contourPos, domainSize[1]);
			return false;
		}
		ImplicitFDTD::fillTextureColumn( excitationCoord[0]+i+1, contourPos, contourPos, wallVal, domainPixels);

		// fill gaps
		if(i>0) {
			jump = (tract[i] - tract[i-1]);
			if(jump > 1)        // i am taller than the previous wall cell
				ImplicitFDTD::fillTextureColumn(  excitationCoord[0]+i, bottom+tract[i-1]+1, bottom+tract[i-1]+1+jump-1, wallVal, domainPixels);
			else if (jump < -1) // i am shorter than the previous wall cell
				ImplicitFDTD::fillTextureColumn( excitationCoord[0]+i+1,  bottom+tract[i]+1, bottom+tract[i]+1-jump-1, wallVal, domainPixels);
		}
	}

	// close the fucking tube
	// with a wall of excitations...
	float excitationVal = ImplicitFDTD::cell_excitation;
	jump = (tract[1] - tract[0]);
	jump = (jump>1) ? jump : 0;
	ImplicitFDTD::fillTextureColumn( excitationCoord[0], excitationCoord[1]+excitationDim[1]-1, bottom+tract[0]+jump/*+1*/, excitationVal, domainPixels);
	ImplicitFDTD::fillTextureColumn( excitationCoord[0], bottom+1/*-1*/,  excitationCoord[1]-1, excitationVal, domainPixels);
	// and 2 wall cells on corners
	ImplicitFDTD::fillTexturePixel( excitationCoord[0], bottom+tract[0]+jump+1, wallVal, domainPixels);
	ImplicitFDTD::fillTexturePixel( excitationCoord[0], bottom, wallVal, domainPixels);

	// open space
	if(false) {
		// Dirichlet [Peter Gustav Lejeune] uniform boundary condition with p=0
		float noPressVal = ImplicitFDTD::getPixelAlphaValue(ImplicitFDTD::cell_nopressure);
		ImplicitFDTD::fillTextureColumn( excitationCoord[0]+tract_s+1, bottom, contourPos, noPressVal, domainPixels);
	}
	return true;
}


int createStoryVowelTube(float *vowelAreaFunc, float vowelAreaFunc_len, float vowel_spacing, float ds, int domainSize[], float ***domainPixels, int asymm, bool nasal) {
	static float refVowelLen = vowelAreaFunc_len/100.0;

	float ds_cm    = ds*100;
	float x_pos_cm = 0;
	int xPixels    = round((vowelAreaFunc_len)/ds_cm);
	int nSegments  = (vowelAreaFunc_len/vowel_spacing); // recover original number of segments of raw data

	float diameters[nSegments];
	fromAreasToDiameters(vowelAreaFunc, nSegments, diameters, false);

	float diameters_interp[xPixels];
	int yPixels[xPixels];
	for(int i=0; i<xPixels; i++) {
		diameters_interp[i] = resampleArea(diameters, vowelAreaFunc_len, nSegments, x_pos_cm, vowel_spacing); // calculates the area in the requested x position and returns diameter of circle with same area
		diameters_interp[i] /= 100; // back to meters
		x_pos_cm += ds_cm; // next point

		yPixels[i] =  round(diameters_interp[i]/ds); // in meters
		int a = 0;
		a++;
	}

	// if no space correction, simply create tube
	if(!auto_spacing_correction) {
		if(asymm<2) {
			if(!createSymmetricTubeDomain(yPixels, xPixels, domainSize, domainPixels, asymm==1))
				return -1;
		}
		else if(asymm==2) {
			if(!createUpwardsTubeDomain(yPixels, xPixels, domainSize, domainPixels))
				return -1;
		}

		// carve little nasal opening right in the middle of the tube
		if(nasal) {
			int nasalPos = round((xPixels)/2.0);
			ImplicitFDTD::fillTextureColumn(excitationCoord[0]+nasalPos, excitationCoord[1], domainSize[1]-1, ImplicitFDTD::getPixelAlphaValue(ImplicitFDTD::cell_air), domainPixels);
		}
		return xPixels+1;
	}


	// if space correction, this becomes a recursive function! until we satisfy the length requirements or we die!
	int retval = -1;

	float ds_squared = ds*ds;
	float ds_by_2_squared = (ds/2.0)*(ds/2.0);
	float len = 0;

	if(asymm==0)
		len = xPixels*ds;
	else {
		len += ds;
		for(int i=1; i<xPixels; i++) {
			if(asymm<2) {
				if( yPixels[i]%2 == yPixels[i-1]%2  )
					len += ds;
				else
					len += sqrt(ds_squared + ds_by_2_squared);
			}
			else {
				float diff_ds = ds * fabs(yPixels[i]/2.0 - yPixels[i-1]/2.0);
				len += sqrt(ds_squared + diff_ds*diff_ds);
			}
		}
	}
	printf("Computed len: %f [m], Real len: %f [m]\n", len, refVowelLen);
	printf("diff: %f, ds: %f\n", len-refVowelLen, ds);
	printf("xPixels: %d\n", xPixels);

	// if something went wrong, we stop and surrender!
	if(vowelAreaFunc_len < 2*ds) {
		retval = -1;
		printf("auto_spacing_correction screwed up /:\n");
	}
	// if the length is not correct, we shrink the areas [recursive call]
	else if( fabs(len-refVowelLen) > /*max(*/ds/2.0/*, 0.0015)*/ ) {
		float new_scale = 0.999;
		retval = createStoryVowelTube(vowelAreaFunc, vowelAreaFunc_len*new_scale, vowel_spacing*new_scale, ds, domainSize, domainPixels, asymm, nasal);
	}
	// if we found correct length shrinking the areas, we create the tube
	else {

		auto_spacing_correction = false;
		float new_scale = 1;
		retval = createStoryVowelTube(vowelAreaFunc, vowelAreaFunc_len*new_scale, vowel_spacing*new_scale, ds, domainSize, domainPixels, asymm, nasal);



/*		if(asymm<2) {
			if(!createSymmetricTubeDomain(yPixels, xPixels, domainSize, domainPixels, asymm==1))
				return -1;
		}
		else if(asymm==2) {
			if(!createUpwardsTubeDomain(yPixels, xPixels, domainSize, domainPixels))
				return -1;
		}

		// carve little nasal opening right in the middle of the tube
		if(nasal) {
			int nasalPos = round((xPixels)/2.0);
			ImplicitFDTD::fillTextureColumn(excitationCoord[0]+nasalPos, excitationCoord[1], domainSize[1]-1, ImplicitFDTD::getPixelAlphaValue(ImplicitFDTD::cell_air), domainPixels);
		}
		return xPixels+1;*/
	}
	return retval;
}

int createFantVowelTube(float *vowelAreaFunc, float vowelAreaFunc_len, float vowel_spacing, float ds, int domainSize[], float ***domainPixels, int asymm, bool nasal) {
	float ds_cm = ds*100;
	float x_pos = 0;
	int xPixels = round((vowelAreaFunc_len)/ds_cm);
	int nSegments = (vowelAreaFunc_len/vowel_spacing)+1;

	float diameters[nSegments];
	fromAreasToDiameters(vowelAreaFunc, nSegments, diameters);

	float diameters_interp[xPixels];
	int yPixels[xPixels];
	for(int i=0; i<xPixels; i++) {
		diameters_interp[i] = interpolateArea(diameters, vowelAreaFunc_len, nSegments, x_pos, vowel_spacing);
		diameters_interp[i] /= 100; // back to meters
		x_pos += ds_cm; // next point

		yPixels[i] =  round(diameters_interp[i]/ds); // in meters
	}

	// symmetric or symmetric with residue
	if(asymm<2) {
		if(!createSymmetricTubeDomain(yPixels, xPixels, domainSize, domainPixels, asymm==1))
			return -1;
	}
	else if(asymm==2){ // flat bottom
		if(!createUpwardsTubeDomain(yPixels, xPixels, domainSize, domainPixels))
			return -1;
	}

	// carve little nasal opening right in the middle of the tube
	if(nasal) {
		int nasalPos = round((xPixels)/2.0);
		ImplicitFDTD::fillTextureColumn(excitationCoord[0]+nasalPos, excitationCoord[1], domainSize[1]-1, ImplicitFDTD::getPixelAlphaValue(ImplicitFDTD::cell_air), domainPixels);
	}

	return xPixels+1;
}


int createStraightTube(float length, float width, float ds, int domainSize[], float ***domainPixels, int &widthPixel, bool open) {
	int xPixels = round(length/ds);
	int yPixels = round(width/ds);

	if(xPixels > domainSize[0]) {
		printf("The specified tube needs at least a domain %d pixels long, while domain is only %d pixels\n", xPixels, domainSize[0]);
		return -1;
	} else if(yPixels > domainSize[1]) {
		printf("The specified tube needs at least a domain %d pixels high, while domain is only %d pixels\n", yPixels, domainSize[1]);
		return -1;
	}


	printf("Tube size:\n");
	printf("\tlength:\t%f mm (%d pixels => %f mm error)\n", length, xPixels-1, (xPixels-1)*ds-length);
	printf("\twidth:\t%f mm (%d pixels => %f mm error)\n", width, yPixels, yPixels*ds-width);



	int halfWidth = (yPixels+1)/2;
	float wallVal   = ImplicitFDTD::getPixelAlphaValue(ImplicitFDTD::cell_wall);
	float exVal   = ImplicitFDTD::getPixelAlphaValue(ImplicitFDTD::cell_excitation);
	if( (yPixels%2)!=0 ) {
		ImplicitFDTD::fillTextureRow( excitationCoord[1]+halfWidth,	excitationCoord[0], excitationCoord[0]+xPixels, wallVal, domainPixels);
		ImplicitFDTD::fillTextureRow( excitationCoord[1]-halfWidth, 	excitationCoord[0], excitationCoord[0]+xPixels, wallVal, domainPixels);

		// close fucking tube with excitation
		if(!open)
			ImplicitFDTD::fillTextureColumn( excitationCoord[0],  excitationCoord[1]-halfWidth+1, excitationCoord[1]+halfWidth-1, exVal, domainPixels);
	}
	else {
		ImplicitFDTD::fillTextureRow( excitationCoord[1]+halfWidth+1,	excitationCoord[0], excitationCoord[0]+xPixels, wallVal, domainPixels);
		ImplicitFDTD::fillTextureRow( excitationCoord[1]-halfWidth, 		excitationCoord[0], excitationCoord[0]+xPixels, wallVal, domainPixels);

		// close fucking tube with excitation
		if(!open)
			ImplicitFDTD::fillTextureColumn( excitationCoord[0],  excitationCoord[1]-halfWidth+1, excitationCoord[1]+halfWidth, exVal, domainPixels);

		printf("\t[asymmetric width]\n");
	}

	widthPixel = yPixels;
	return xPixels+1;
}




