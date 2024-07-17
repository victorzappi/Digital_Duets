/*
 * FDTDSynth.cpp
 *
 *  Created on: 2016-11-18
 *      Author: Victor Zappi
 */

#include "FDTDSynth.h"


FDTDSynth::FDTDSynth() {
	simulationRate = -1;
	rateMultiplier = -1;
}

void FDTDSynth::init(string shaderLocation, int *domainSize, int *excitationCoord, int *excitationDimensions,
			float ***domain, int audioRate, int rateMul, int periodSize, float magnifier, int *listCoord) {
	AudioGeneratorOut::init(periodSize);

	rate            = audioRate;
	rateMultiplier  = (rateMul>0) ? rateMul : 1; // juuust in case
	simulationRate  = fdtd.init(shaderLocation, domainSize, excitationCoord, excitationDimensions,
										  domain, audioRate, rateMul, periodSize, magnifier, listCoord);

	initExcitation(rateMul);
}

void FDTDSynth::init(string shaderLocation, int *domainSize, int audioRate, int rateMul, int periodSize,
					   float magnifier, int *listCoord) {
	AudioGeneratorOut::init(periodSize);

	rate            = audioRate;
	rateMultiplier  = (rateMul>0) ? rateMul : 1; // juuust in case
	simulationRate  = fdtd.init(shaderLocation, domainSize, audioRate, rateMul, periodSize,
									      magnifier, listCoord);

	initExcitation(rateMul);
}

void FDTDSynth::initExcitation(int rate_mul) {
	float maxExcitationLevel = 1;
	//excitation = new FDTDExcitation();
	excitation.init(rate, simulationRate, period_size*rate_mul, maxExcitationLevel); // periodSize*rate_mul cos we have to provide enough samples per each simulation step
}

int FDTDSynth::setDomain(int *excitationCoord, int *excitationDimensions, float ***domain) {
	if(fdtd.setDomain(excitationCoord, excitationDimensions, domain)!=0) {
		printf("Cannot load domain! Vocal Tract FDTD simulation already has one\n");
		return 1;
	}
	return 0;
}

double **FDTDSynth::getFrameBuffer(int numOfSamples) {
	//@VIC
	double **excFrame = excitation.getFrameBuffer(numOfSamples*rateMultiplier);
	//float *buff = fdtd.getBufferFloat(numOfSamples, excBuff);

	//std::copy(buff, buff+numOfSamples, framebuffer[0]);

	return fdtd.getFrameBuffer(numOfSamples, excFrame);
}

