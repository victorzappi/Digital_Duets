/*
 * defineDomain.cpp
 *
 *  Created on: 2016-06-06
 *      Author: vic
 */

#include "defineDomain.h"

#include "ArtVocSynth.h"
#include "geometries.h"
#include "VocalTractFDTD.h"



//----------------------------------------
// audio extern var, from main.cpp
//----------------------------------------
extern unsigned int rate;
//----------------------------------------

//----------------------------------------
// domain extern vars, from main.cpp
//----------------------------------------
extern int domainSize[2];
extern float magnifier;
extern int rate_mul;
extern float ds;
extern bool fullscreen;
extern int monitorIndex;
//----------------------------------------

//----------------------------------------
// simulation variables, from main.cpp
//----------------------------------------
extern float radii_factor;
extern bool radiation;
extern int numAudioSamples;
extern float openSpaceWidth;
//----------------------------------------

//----------------------------------------
// control extern var, from main.cpp
//----------------------------------------
extern int listenerCoord[2];
extern int feedbCoord[2];
extern int excitationDim[2];
extern int excitationCoord[2];
extern float excitationAngle;
extern excitation_model excitationModel;
extern subGlotEx_type subGlotExType;
extern float excitationLevel;
extern float excitationVol;
extern float maxPressure;
extern float physicalParams[5];
extern float a1; //TODO this should be calculated and not hardcoded in each function
extern float alpha;
//----------------------------------------


//----------------------------------------
// area function externs, from geometries.cpp
//----------------------------------------
// story 08 vowels
extern float story08Data_a_spacing;
extern float story08Data_a[44];
extern float story08Data_a_len;

extern float story08Data_i_spacing;
extern float story08Data_i[44];
extern float story08Data_i_len;

extern float story08Data_u_spacing;
extern float story08Data_u[44];
extern float story08Data_u_len;

//-------------------------------------------------------
// extern tube measures and settings for mic experiment, from geometries.cpp
//-------------------------------------------------------
extern int duration_experiment;
extern float c_experiment;
extern float rho_experiment;
extern float micXpos_experiment[2];
extern float mu_experiment;

extern float domain_experiment[];

// two centric tubes
extern float tubeslen;
extern float tube0Diameter;


//-------------------------------------------------------
// extern tube measures and settings for symmetry test, from geometries.cpp
//-------------------------------------------------------
extern int symmTest_duration;
extern float symmTest_c;
extern float symmTest_rho;
//float symmTest_alpha5; // alpha for boundaries, better use the global one, for testing
extern float symmTest_micPos;

extern float symmTest_domain[][2];
extern float symmTest_tubeslen;
extern double symmTest_a1;

extern double symmTest_tube0_height_big;
extern double symmTest_tube0_widthProfileDs_big;
extern double symmTest_tube0_widthProfile_big[];
extern double symmTest_tube0_height_small;
extern double symmTest_tube0_widthProfileDs_small;
extern double symmTest_tube0_widthProfile_small[];


extern double symmTest_story_xyz_area;
extern double symmTest_story_xyz_height;
extern double symmTest_story_xyz_widthProfileDs;
extern double symmTest_story_xyz_widthProfile[];

extern double symmTest_story_yz_area;
extern double symmTest_story_yz_height;
extern double symmTest_story_yz_widthProfileDs;
extern double symmTest_story_yz_widthProfile[];



// domainPixels passed by reference since dynamically allocated in the function
void initDomainPixels(float ***&domainPixels) {
	//---------------------------------------------
	// all is in domain frame of ref, which means no PML nor dead layers included
	//---------------------------------------------

	// domain pixels [aka walls!]
	domainPixels = new float **[domainSize[0]];
	float airVal = ImplicitFDTD::getPixelAlphaValue(ImplicitFDTD::cell_air, 0);
	for(int i = 0; i <domainSize[0]; i++) {
		domainPixels[i] = new float *[domainSize[1]];
		for(int j = 0; j <domainSize[1]; j++) {
			domainPixels[i][j]    = new float[4];
			domainPixels[i][j][3] = airVal;
		}
	}
}

void initWidthMapPixels(float ***&widthMapPixels) {
	//---------------------------------------------
	// all is in domain frame of ref, which means no PML nor dead layers included
	//---------------------------------------------
	widthMapPixels = new float **[domainSize[0]];
	// open end is DIrichlet case, no radiation forcing p=0

	for(int i = 0; i <domainSize[0]; i++) {
		widthMapPixels[i] = new float *[domainSize[1]];
		for(int j = 0; j <domainSize[1]; j++) {
			widthMapPixels[i][j] = new float[3];
			widthMapPixels[i][j][0] = openSpaceWidth;
			widthMapPixels[i][j][1] = openSpaceWidth;
			widthMapPixels[i][j][2] = openSpaceWidth;
		}
	}
}




// central excitation, no walls
void openSpaceExcitation(float ***&domainPixels, float ***&widthMapPixels) {
	magnifier    = 3;
	fullscreen   = false;
	monitorIndex = 1;

	domainSize[0] = 100;//308;//320//148;//92;
	domainSize[1] = 100;//228;//240//108;//92;

	rate_mul = 1;

	ds = ImplicitFDTD::estimateDs(rate, rate_mul, physicalParams[0]);

	excitationCoord[0] = domainSize[1]/2;
	excitationCoord[1] = domainSize[1]/2;

	listenerCoord[0] = excitationCoord[0]+3;
	listenerCoord[1] = excitationCoord[1];

	feedbCoord[0] = excitationCoord[0]+1;
	feedbCoord[1] = excitationCoord[1];

	excitationDim[0] = 1;
	excitationDim[1] = 1;

	excitationLevel = 1;
	excitationVol = 1;

	excitationAngle = -1;

	excitationModel = ex_noFeedback;

	subGlotExType = subGlotEx_sin;

	maxPressure = 2000;

	openSpaceWidth = domainSize[0]*ds;

	//-----------------------------------------------------
	initDomainPixels(domainPixels);
	initWidthMapPixels(widthMapPixels);
	//-----------------------------------------------------

	numAudioSamples = 65536; //2^16, that means 2^15 FFT samples
}

void story08Vowel(char vowel, float ***&domainPixels, float ***&widthMapPixels) {
	numAudioSamples = 22050; // 50 ms

	rate_mul = 5;

	float sizes[10][2] = {{50, 20}, {88, 30}, {90, 40}, {106, 36}, {130, 46}, {130, 46}, {130, 46}, {254, 66}, {254, 66}, {280, 240}};

	domainSize[0] = sizes[rate_mul-3][0];
	domainSize[1] = sizes[rate_mul-3][1];


/*
	rate_mul = 4;
	domainSize[0] = 200;//182;
	domainSize[1] = 60;//60;
*/


/*
	rate_mul = 6;
	domainSize[0] = 200;//182;
	domainSize[1] = 60;//60;

*/


/*
	rate_mul = 9;
	domainSize[0] = 150;//182;
	domainSize[1] = 30;//60;


*/



	rate_mul = 12;
	domainSize[0] = 213;//182;
	domainSize[1] = 28+20*(radii_factor);//60;






/*	rate_mul = 20;
	domainSize[0] = 351+50;
	domainSize[1] = 53+20;
	magnifier = 1;*/

/*
	rate_mul = 40;
	domainSize[0] = 700+50;
	domainSize[1] = 107+30;
	magnifier = 1;
*/

/*	rate_mul = 60;
	domainSize[0] = 1049;
	domainSize[1] = 160;
	magnifier = 1;*/


	ds = ImplicitFDTD::estimateDs(rate, rate_mul, physicalParams[0]);

	listenerCoord[0] = (domainSize[0]/2)+1;
	listenerCoord[1] = domainSize[1]/2;

	feedbCoord[0] = 1;
	feedbCoord[1] = listenerCoord[1];

	excitationDim[0] = 1;
	excitationDim[1] = 1; // rest of excitation will be added [drawn in function that builds tube

	excitationCoord[0] = feedbCoord[0]-1;
	excitationCoord[1] = feedbCoord[1];

	excitationLevel = 0.1;
	//excitationLevel = 1; //@VIC
	excitationVol = 1;

	excitationAngle = 0;

	excitationModel = ex_bodyCover;

	subGlotExType = subGlotEx_const;

	float *vowelAreaFunc    = NULL;
	float vowelAreaFunc_len = -1;
	float vowelAreaFunc_spacing = -1;

	switch(vowel) {
		case 'a':
			vowelAreaFunc     = story08Data_a;
			vowelAreaFunc_len = story08Data_a_len;
			vowelAreaFunc_spacing = story08Data_a_spacing;
			break;
		case 'i':
			vowelAreaFunc     = story08Data_i;
			vowelAreaFunc_len = story08Data_i_len;
			vowelAreaFunc_spacing = story08Data_i_spacing;
			break;
		case 'u':
			vowelAreaFunc     = story08Data_u;
			vowelAreaFunc_len = story08Data_u_len;
			vowelAreaFunc_spacing = story08Data_u_spacing;
			break;
		default:
			break;

	}
	a1 = vowelAreaFunc[0];
	a1 /= 10000; // to meters



	float mu = 0.005*2;
	alpha = 1-( ((1-mu)*(1-mu)) / ((1+mu)*(1+mu))  );
	//printf("alpha: %f\n", alpha);

	alpha = 0.005; // originally it was 0.001, but this works better with Dirichlet [instead with radiation 0.02 is good]


	radiation = false; // no radiation, means Dirichlet...
	//openSpaceWidth = OPEN_SPACE_WIDTH;//NO_RADIATION_WIDTH; -> NO_RADIATION_WIDTH would set it to same width of center of mouth opening

	//-----------------------------------------------------
	initDomainPixels(domainPixels);
	initWidthMapPixels(widthMapPixels);
	listenerCoord[0] = createStoryVowelTube(vowelAreaFunc, vowelAreaFunc_len, vowelAreaFunc_spacing, ds, domainSize, domainPixels, widthMapPixels, openSpaceWidth, !radiation);
	//-----------------------------------------------------
	listenerCoord[0] = listenerCoord[0] - 0.003/ds; // move inside of tube


	// now we can finally give a value to open space, cos width map is ready
	/*if(openSpaceWidth==NO_RADIATION_WIDTH)
		openSpaceWidth = widthMapPixels[0][0][0]; // in this case i know (0, 0) is open space! (;*/
}

void twoTubesCentric(float ***&domainPixels, float ***&widthMapPixels, int micNum) {

	float dur = duration_experiment/1000.0;
	numAudioSamples = rate*(dur);

	rate_mul = 5;

	physicalParams[0] = c_experiment;
	physicalParams[1] = rho_experiment;

	ds = ImplicitFDTD::estimateDs(rate, rate_mul, physicalParams[0]);

	domainSize[0] = (domain_experiment[0]/ds); // x dimension is the only one not affected by global scale factor
	domainSize[1] = (domain_experiment[1]/ds)*radii_factor;

	// excitation marks the beginning of the tube, so let's place the tube in the middle of the domain
	excitationCoord[0] = 0.5*(domain_experiment[0]-2*tubeslen/1000.0)/ds; // to m, then to pixels
	excitationCoord[1] = (domainSize[1]/2);

	feedbCoord[0] = excitationCoord[0]+1;
	feedbCoord[1] = excitationCoord[1];

	// a single excitation cell around which the geometry will be built
	excitationDim[0] = 1;
	excitationDim[1] = 1;
	// rest of excitation will be added [drawn] in function that builds tube, to close the tube

	excitationLevel = 0.1;
	excitationVol = 1;

	excitationAngle = 0;

	excitationModel = ex_noFeedback;

	subGlotExType = subGlotEx_sin;

	// a1, for glottal excitation...just in case
	float r = (tube0Diameter/2)/1000.0; // to m
	a1 = r*r*M_PI;

	alpha = VocalTractFDTD::alphaFromAdmittance(mu_experiment);

	//VIC this low alpha and radiation and NO_RADIATION_WITH case gives something quite close...but not enough...need to think

	alpha = 0.0001;

	radiation = true; // radiation, normal case at tube opening
	openSpaceWidth = NO_RADIATION_WIDTH;// domain_experiment[2]*radii_factor; // we set a specific open space width

	//-----------------------------------------------------
	initDomainPixels(domainPixels);
	initWidthMapPixels(widthMapPixels);
	int geomLen = createTwoTubesCentric(ds, domainSize, domainPixels, widthMapPixels, !radiation);
	//-----------------------------------------------------


	float micXpos = micXpos_experiment[micNum]/1000.0; // to m
	float realDist = micXpos + ds/2; // this takes into account the fact that mic is in the center of pixels, while source and opening are on edge
	int distPixels = round(realDist/ds); // distance expressed in pixels
	float micPosErr = micXpos - ( distPixels*ds - ds/2.0 ); //computed resulting error

	listenerCoord[1] = excitationCoord[1]; // always on midline

	printf("----------------------------------------------------------------------------------\n");
	printf("Mic  posistion:\n");
	printf("\n");

	if(micNum==0) { // mic at beginning of tube
		listenerCoord[0] = excitationCoord[0]+distPixels;

		printf("\ton midline, inside tube, %f mm from source (%f mm error)\n", micXpos_experiment[micNum], micPosErr);
	}
	else if(micNum==1) { // mic after opening
		listenerCoord[0] = excitationCoord[0]+geomLen+distPixels;

		printf("\ton midline, outside tube, %f mm from opening (%f mm error)\n", micXpos_experiment[micNum], micPosErr);
	}
	printf("----------------------------------------------------------------------------------\n");


	// now we can finally give a value to open space, cos width map is ready
	if(openSpaceWidth==NO_RADIATION_WIDTH)
		openSpaceWidth = widthMapPixels[0][0][0]; // in this case i know (0, 0) is open space! (;

}



void symmTestTubes(int tubeID, float ***&domainPixels, float ***&widthMapPixels, bool rotated) {


/*	float story08Data_b_spacing = 0.38636363636;
	float story08Data_b[44] = { 1.65129963854, 1.65129963854, 1.65129963854, 1.65129963854, 1.65129963854,
								1.65129963854, 1.65129963854, 1.65129963854, 1.65129963854, 1.65129963854,
								1.65129963854, 1.65129963854, 1.65129963854, 1.65129963854, 1.65129963854,
								1.65129963854, 1.65129963854, 1.65129963854, 1.65129963854, 1.65129963854,
								1.65129963854, 1.65129963854,
								6.83492751697, 6.83492751697, 6.83492751697, 6.83492751697, 6.83492751697,
								6.83492751697, 6.83492751697, 6.83492751697, 6.83492751697, 6.83492751697,
								6.83492751697, 6.83492751697, 6.83492751697, 6.83492751697, 6.83492751697,
								6.83492751697, 6.83492751697, 6.83492751697, 6.83492751697, 6.83492751697,
								6.83492751697, 6.83492751697 };
	float story08Data_b_len = story08Data_b_spacing*44;*/



	float dur = symmTest_duration/1000.0;
	numAudioSamples = rate*(dur);

	rate_mul = 12;
	magnifier = 5;

	physicalParams[0] = symmTest_c;
	physicalParams[1] = symmTest_rho;

	alpha = 0.05; // originally it was 0.001, but this works better with Dirichlet


	ds = ImplicitFDTD::estimateDs(rate, rate_mul, physicalParams[0]);


	double tubeHeight[2] = {0};
	double widthDs[2] = {0};
	double *widthProfile[2];

	switch(tubeID) {
		case symmTest_tube0:
			tubeHeight[0] = symmTest_tube0_height_small;
			tubeHeight[1] = symmTest_tube0_height_big;

			widthDs[0] = symmTest_tube0_widthProfileDs_small;
			widthDs[1] = symmTest_tube0_widthProfileDs_big;

			widthProfile[0] = symmTest_tube0_widthProfile_small;
			widthProfile[1] = symmTest_tube0_widthProfile_big;

			domainSize[0] = (symmTest_domain[0][0]/ds); // we are not messing around with x-scale factor
			domainSize[1] = (symmTest_domain[0][1]/ds)*radii_factor;
			break;

		//case ...

		default:
			break;
	}



	// excitation marks the beginning of the tube, so let's place the tube in the far left part of the domain, but vertically centered
	excitationCoord[0] = 0; // to m, then to pixels
	excitationCoord[1] = (domainSize[1]/2);

	feedbCoord[0] = excitationCoord[0]+1;
	feedbCoord[1] = excitationCoord[1];

	// a single excitation cell around which the geometry will be built
	excitationDim[0] = 1;
	excitationDim[1] = 1;
	// rest of excitation will be added [drawn] in function that builds tube, to close the tube

	excitationLevel = 0.1;
	excitationVol = 1;

	excitationAngle = 0;

	excitationModel = ex_noFeedback;

	subGlotExType = subGlotEx_sin;


	a1 = symmTest_a1;

	radiation = false; // no radiation, means Dirichlet...
	openSpaceWidth = NO_RADIATION_WIDTH; // ...which needs this setup, that will affect the creation of the width map in geometries.cpp

	//-----------------------------------------------------
	initDomainPixels(domainPixels);
	initWidthMapPixels(widthMapPixels);
	listenerCoord[0] = createSymmTestTube(ds, domainSize, tubeHeight, widthDs, widthProfile, domainPixels, widthMapPixels, !radiation);
	//listenerCoord[0] = createStoryVowelTube(story08Data_b, story08Data_b_len, story08Data_b_spacing, ds, domainSize, domainPixels, widthMapPixels, !radiation);
	//-----------------------------------------------------

	listenerCoord[0] = listenerCoord[0] + round( (symmTest_micPos-ds/2)/ds ); // move inside of tube...ds/2 cos mic in reality is on the edge of opening, while in simulation is in center of cell
	listenerCoord[1] = domainSize[1]/2;

	double micXRealPos = symmTest_tubeslen*2 + symmTest_micPos;
	double micXSymPos = (listenerCoord[0]*ds) - ds/2; // in the center of the cell
	double micPosErr = fabs(micXRealPos-micXSymPos);
	printf("----------------------------------------------------------------------------------\n");
	printf("Mic  posistion:\n");
	printf("\n");
	printf("\ton midline, inside tube, %f m inside opening (%f m error)\n", symmTest_micPos, micPosErr);
	printf("----------------------------------------------------------------------------------\n");


	// now we can finally give a value to open space, cos width map is ready
	if(openSpaceWidth==NO_RADIATION_WIDTH)
		openSpaceWidth = widthMapPixels[0][0][0]; // in this case i know (0, 0) is open space! (;

}


void symmTestStoryVowel(char vowel, int symmetryID, float ***&domainPixels, float ***&widthMapPixels, bool rotated) {
	float dur = symmTest_duration/1000.0;
	numAudioSamples = rate*(dur);
	numAudioSamples = 22050; // 500 ms


	float sizes[10][2] = {{50, 20}, {88, 30}, {90, 40}, {106, 36}, {130, 46}, {130, 46}, {130, 46}, {254, 66}, {254, 66}, {280, 240}};

	domainSize[0] = sizes[rate_mul-3][0];
	domainSize[1] = sizes[rate_mul-3][1];

	rate_mul = 12;
	domainSize[0] = 213;//182;
	domainSize[1] = 28*2;//60;

	/*	rate_mul = 20;
		domainSize[0] = 351+50;
		domainSize[1] = 53+20;
		magnifier = 1;*/

	/*
		rate_mul = 40;
		domainSize[0] = 700+50;
		domainSize[1] = 107+30;
		magnifier = 1;
	*/

	ds = ImplicitFDTD::estimateDs(rate, rate_mul, physicalParams[0]);

	// excitation marks the beginning of the tube, so let's place the tube in the far left part of the domain, but vertically centered
	excitationCoord[0] = 0; // to m, then to pixels
	excitationCoord[1] = (domainSize[1]/2);

	feedbCoord[0] = excitationCoord[0]+1;
	feedbCoord[1] = excitationCoord[1];

	// a single excitation cell around which the geometry will be built
	excitationDim[0] = 1;
	excitationDim[1] = 1;
	// rest of excitation will be added [drawn] in function that builds tube, to close the tube

	excitationLevel = 1;
	excitationVol = 1;

	excitationAngle = 0;

	excitationModel = ex_noFeedback;
	subGlotExType = subGlotEx_sin;

	float *vowelAreaFunc    = NULL;
	float vowelAreaFunc_len = -1;
	float vowelAreaFunc_spacing = -1;

	switch(vowel) {
		case 'a':
			vowelAreaFunc     = story08Data_a;
			vowelAreaFunc_len = story08Data_a_len;
			vowelAreaFunc_spacing = story08Data_a_spacing;
			break;
		case 'i':
			vowelAreaFunc     = story08Data_i;
			vowelAreaFunc_len = story08Data_i_len;
			vowelAreaFunc_spacing = story08Data_i_spacing;
			break;
		case 'u':
			vowelAreaFunc     = story08Data_u;
			vowelAreaFunc_len = story08Data_u_len;
			vowelAreaFunc_spacing = story08Data_u_spacing;
			break;
		default:
			break;

	}
	a1 = vowelAreaFunc[0];
	a1 /= 10000; // to meters

	alpha = 0.05; // originally it was 0.001, but this works better with Dirichlet [instead with radiation 0.02 is good]

	radiation = false; // no radiation, means Dirichlet...
	openSpaceWidth = NO_RADIATION_WIDTH; // ...which needs this setup, that will affect the creation of the width map in geometries.cpp

	//-----------------------------------------------------
	initDomainPixels(domainPixels);
	initWidthMapPixels(widthMapPixels);
	switch(symmetryID) {
		case symmTest_xyz:
			//listenerCoord[0] = createStoryVowelTube(vowelAreaFunc, vowelAreaFunc_len, vowelAreaFunc_spacing, ds, domainSize, domainPixels, widthMapPixels, !radiation);
			listenerCoord[0] = createStoryVowelTube_symmTest(vowelAreaFunc, vowelAreaFunc_len, vowelAreaFunc_spacing, ds, domainSize,
					                                         symmTest_story_xyz_area, symmTest_story_xyz_height, symmTest_story_xyz_widthProfileDs, symmTest_story_xyz_widthProfile,
															 domainPixels, widthMapPixels, !radiation);
			break;
		case symmTest_yz:
			//listenerCoord[0] = createYzSymmStoryVowelTube(vowelAreaFunc, vowelAreaFunc_len, vowelAreaFunc_spacing, ds, domainSize, domainPixels, widthMapPixels, !radiation, rotated);
			listenerCoord[0] = createStoryVowelTube_symmTest(vowelAreaFunc, vowelAreaFunc_len, vowelAreaFunc_spacing, ds, domainSize,
								                                         symmTest_story_yz_area, symmTest_story_yz_height, symmTest_story_yz_widthProfileDs, symmTest_story_yz_widthProfile,
																		 domainPixels, widthMapPixels, !radiation);
			break;

			//VIC continue with asymmetric case...but create width profile in matlab by rotating and resampling original profile! -> createWidthProfile_tube2.m
		default:
			break;
	}
	//-----------------------------------------------------

	listenerCoord[0] = listenerCoord[0] + round( (symmTest_micPos-ds/2)/ds ); // move inside of tube...ds/2 cos mic in reality is on the edge of opening, while in simulation is in center of cell
	listenerCoord[1] = domainSize[1]/2;

	double micXRealPos = symmTest_tubeslen*2 + symmTest_micPos;
	double micXSymPos = (listenerCoord[0]*ds) - ds/2; // in the center of the cell
	double micPosErr = fabs(micXRealPos-micXSymPos);
	printf("----------------------------------------------------------------------------------\n");
	printf("Mic  posistion:\n");
	printf("\n");
	printf("\ton midline, inside tube, %f m inside opening (%f m error)\n", symmTest_micPos, micPosErr);
	printf("----------------------------------------------------------------------------------\n");



	// now we can finally give a value to open space, cos width map is ready
	if(openSpaceWidth==NO_RADIATION_WIDTH)
		openSpaceWidth = widthMapPixels[0][0][0]; // in this case i know (0, 0) is open space! (;
}
