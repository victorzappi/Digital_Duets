/*
 * DrumSynth.cpp
 *
 *  Created on: 2016-11-18
 *      Author: Victor Zappi
 */

#include "DrumSynth.h"


DrumSynth::DrumSynth(bool useGpu) {
	simulationRate = -1;
	rateMultiplier = -1;

	useGPU = useGpu;

	excitation = NULL;
}

void DrumSynth::init(string shaderLocation, int *domainSize, int audioRate, int rateMul,
		             int period_size, float exLevel, float magnifier, int *listCoord) {
	periodSize      = period_size;
	rate            = audioRate;
	rateMultiplier  = (rateMul>0) ? rateMul : 1; // juuust in case

	if(useGPU)
		simulationRate  = drumheadGPU.init(shaderLocation, domainSize, audioRate, rateMul, period_size, magnifier, listCoord);
	else {
		drumheadCPU.init(rate, periodSize, domainSize, listCoord);
		simulationRate = rate;
	}

	initExcitation(rateMul, exLevel);
}

void DrumSynth::initExcitation(int rate_mul, float excitationLevel) {
	excitation = new DrumExcitation(excitationLevel, rate, periodSize*rate_mul, simulationRate); // periodSize*rate_mul cos we have to provide enough samples per each simulation step
}

int DrumSynth::setDomain(int *excitationCoord, int *excitationDimensions, float ***domain) {
	if(useGPU) {
		if(drumheadGPU.setDomain(excitationCoord, excitationDimensions, domain)!=0) {
			printf("Cannot load domain! Vocal Tract FDTD simulation already has one\n");
			return 1;
		}
	}
	else {
		drumheadCPU.setDomain(excitationCoord, excitationDimensions, domain);
	}
	return 0;
}

// screw this
double DrumSynth::getSample() {
	printf("Hey! DrumSynth::getSample() is not implemented, why the hell are you calling it, uh?\n");
	return 0;
}

// and screw this too
double *DrumSynth::getBuffer(int numOfSamples) {
	printf("Hey! DrumSynth::getBuffer() is not implemented, why the hell are you calling it, uh?\n");
	return NULL;
}

// we override cos we don't need a local buffer! [we use TextureFilter::outputBuffer]
float *DrumSynth::getBufferFloat(int numOfSamples) {
	float *excBuff = excitation->getBufferFloat(numOfSamples*rateMultiplier);
	//for(int n = 0; n < numSamples; n++)
	//	excBuff[n] *= envBuff[n];

/*	float excBuff[numOfSamples];
	for(int n = 0; n < numOfSamples; n++)
		excBuff[n] = 0;*/

	// input test
	//return excBuff;

	if(useGPU)
		return drumheadGPU.getBufferFloat(numOfSamples, excBuff);
	else
		return drumheadCPU.getBufferFloat(numOfSamples, excBuff);
}

float *DrumSynth::getBufferFloatReadTime(int numOfSamples, int &readTime) {
	float *excBuff = excitation->getBufferFloat(numOfSamples*rateMultiplier);
	if(useGPU)
		return drumheadGPU.getBufferFloatReadTime(numOfSamples, excBuff, readTime);
	else {
		readTime = 0;
		return drumheadCPU.getBufferFloat(numOfSamples, excBuff);
	}
}

float *DrumSynth::getBufferFloatNoSampleRead(int numOfSamples) {
float *excBuff = excitation->getBufferFloat(numOfSamples*rateMultiplier);
	if(useGPU)
		return drumheadGPU.getBufferFloatNoSampleRead(numOfSamples, excBuff);
	else {
		return drumheadCPU.getBufferFloat(numOfSamples, excBuff);
	}
}

/*DrumSynth::~DrumSynth() {

}*/
