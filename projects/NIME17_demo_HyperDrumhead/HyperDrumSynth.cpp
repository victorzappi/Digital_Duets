/*
 * HyperDrumSynth.cpp
 *
 *  Created on: 2016-11-18
 *      Author: Victor Zappi
 */

#include "HyperDrumSynth.h"


HyperDrumSynth::HyperDrumSynth() {
	simulationRate = -1;
	rateMultiplier = -1;

	domainSize[0] = -1;
	domainSize[1] = -1;

	excitation = NULL;

	numOfAreas = HyperDrumhead::areaNum;

	areaExcitation = new DrumExcitation* [numOfAreas];
	for(int i=0; i<numOfAreas; i++)
		areaExcitation[i] = NULL;

	areaExciteBuff = new float* [numOfAreas];
}

HyperDrumSynth::~HyperDrumSynth() {
	for(int i=0; i<numOfAreas; i++) {
		if(areaExcitation[i] != NULL)
			delete areaExcitation[i];
	}
	delete[] areaExcitation;
	delete[] areaExciteBuff;

	delete excitation;
}

void HyperDrumSynth::init(string shaderLocation, int *domainSize, int audioRate, int rateMul,
		                  int period_size, float exLevel, float magnifier, int *listCoord) {
	periodSize      = period_size;
	rate            = audioRate;
	rateMultiplier  = (rateMul>0) ? rateMul : 1; // juuust in case

	this->domainSize[0] = domainSize[0];
	this->domainSize[1] = domainSize[1];

	simulationRate  = hyperDrumhead.init(shaderLocation, domainSize, audioRate, rateMul, period_size, magnifier, listCoord);

	initExcitation(rateMul, exLevel);
}

void HyperDrumSynth::initExcitation(int rate_mul, float excitationLevel) {
	excitation = new DrumExcitation(excitationLevel, rate, periodSize*rate_mul, simulationRate); // periodSize*rate_mul cos we have to provide enough samples per each simulation step

	/*for(int i=0; i<numOfAreas; i++)
		areaExciteBuff[i] = new float[periodSize];*/

	for(int i=0; i<numOfAreas; i++)
		areaExcitation[i] = new DrumExcitation(excitationLevel, rate, periodSize*rate_mul, simulationRate); // periodSize*rate_mul cos we have to provide enough samples per each simulation step
}

int HyperDrumSynth::setDomain(int *excitationCoord, int *excitationDimensions, float ***domain, bool preset) {
	if(preset) {
		for(int i=0; i<domainSize[0]; i++) {
			for(int j=0; j<domainSize[1]; j++) {
				if(i<domainSize[0]/2 && j>=domainSize[1]/2)
					domain[i][j][1] = 1;
				else if(i<domainSize[0]/2 && j<domainSize[1]/2)
					domain[i][j][1] = 2;
				else if(i>=domainSize[0]/2 && j<domainSize[1]/2)
					domain[i][j][1] = 3;
			}
		}
	}

	if(hyperDrumhead.setDomain(excitationCoord, excitationDimensions, domain)!=0) {
		printf("Cannot load domain! Hyper Drumhead simulation has one already\n");
		return 1;
	}

	return 0;
}

// screw this
double HyperDrumSynth::getSample() {
	printf("Hey! HyperDrumSynth::getSample() is not implemented, why the hell are you calling it, uh?\n");
	return 0;
}

// and screw this too
double *HyperDrumSynth::getBuffer(int numOfSamples) {
	printf("Hey! HyperDrumSynth::getBuffer() is not implemented, why the hell are you calling it, uh?\n");
	return NULL;
}

// we override cos we don't need a local buffer! [we use TextureFilter::outputBuffer]
float *HyperDrumSynth::getBufferFloat(int numOfSamples) {
	float *excBuff = NULL;//excitation->getBufferFloat(numOfSamples*rateMultiplier);
	//for(int n = 0; n < numSamples; n++)
	//	excBuff[n] *= envBuff[n];

/*	float excBuff[numOfSamples];
	for(int n = 0; n < numOfSamples; n++)
		excBuff[n] = 0;*/

	// input test
	//return excBuff; //@VIC

	for(int i=0; i<numOfAreas; i++)
		areaExciteBuff[i] = areaExcitation[i]->getBufferFloat(numOfSamples*rateMultiplier);

	return hyperDrumhead.getBufferFloatAreas(numOfSamples, excBuff, areaExciteBuff);
}

float *HyperDrumSynth::getBufferFloatReadTime(int numOfSamples, int &readTime) {
	return hyperDrumhead.getBufferFloatReadTime(numOfSamples, excitation->getBufferFloat(numOfSamples*rateMultiplier), readTime);
}

float *HyperDrumSynth::getBufferFloatNoSampleRead(int numOfSamples) {
	return hyperDrumhead.getBufferFloatNoSampleRead(numOfSamples, excitation->getBufferFloat(numOfSamples*rateMultiplier));

}

/*HyperDrumSynth::~HyperDrumSynth() {

}*/
