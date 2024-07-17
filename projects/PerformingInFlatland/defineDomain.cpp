/*
 * defineDomain.cpp
 *
 *  Created on: 2016-06-06
 *      Author: vic
 */

#include "FlatlandSynth.h"

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
// control extern var, from main.cpp
//----------------------------------------
extern int listenerCoord[2];
extern int feedbCoord[2];
extern int excitationDim[2];
extern int excitationCoord[2];
extern float excitationAngle;
extern excitation_model exModel;
extern float excitationVol;
extern float maxPressure;
extern float physicalParams[5];
//----------------------------------------



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


// central excitation, no walls
void openSpaceExcitation(float ***&domainPixels) {
	magnifier    = 10;
	fullscreen   = true;
	monitorIndex = 1;

	domainSize[0] = 148;//308;//320//148;//92;
	domainSize[1] = 108;//228;//240//108;//92;

	rate_mul = 1;

	ds = ImplicitFDTD::estimateDs(rate, rate_mul, physicalParams[0]);

	listenerCoord[0] = (domainSize[0]/2)+8;
	listenerCoord[1] = domainSize[1]/2;

	feedbCoord[0] = 0;
	feedbCoord[1] = 0;

	excitationDim[0] = 1;
	excitationDim[1] = 1;

	excitationCoord[0] = domainSize[0]/2;
	excitationCoord[1] = domainSize[1]/2;

	excitationVol = 1;

	excitationAngle = -1;

	exModel = exMod_noFeedback;

	maxPressure = 2000;

	//-----------------------------------------------------
	initDomainPixels(domainPixels);
	//-----------------------------------------------------
}

void emptyDomain(float ***&domainPixels) {
	excitationDim[0] = 0;
	excitationDim[1] = 0;

	excitationAngle = -1;

	//-----------------------------------------------------
	initDomainPixels(domainPixels);
	//-----------------------------------------------------
}
