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

	numOfAreas = HyperDrumhead::areaNum;

}

HyperDrumSynth::~HyperDrumSynth() {
	for(int i=0; i<in_channels; i++) {
		if(channelExcitation[i] != NULL)
			delete channelExcitation[i];
	}
	delete[] channelExcitation;
	delete[] channelExciteBuff;

}

void HyperDrumSynth::init(string shaderLocation, int *domainSize, int audioRate, int rateMul,
		  	  	  	      int periodSize, float exLevel, float magnifier, string exciteFolder, int *listCoord, 
						  unsigned short exInChannels, unsigned short exInChnOffset,  unsigned short hdhInChannels, unsigned short hdhOutChannels, unsigned short hdhOutChnOffset) {
	AudioGeneratorInOut::init(periodSize, hdhInChannels, 0, hdhOutChannels, hdhOutChnOffset);
	rate            = audioRate;
	rateMultiplier  = (rateMul>0) ? rateMul : 1; // juuust in case

	this->domainSize[0] = domainSize[0];
	this->domainSize[1] = domainSize[1];

	simulationRate  = hyperDrumhead.init(shaderLocation, domainSize, audioRate, rateMul, period_size, magnifier, listCoord, hdhInChannels, hdhOutChannels, hdhOutChnOffset);


	initExcitation(rateMul, exLevel, exciteFolder, exInChannels, exInChnOffset);
}

void HyperDrumSynth::initExcitation(int rate_mul, float excitationLevel, string exciteFolder, unsigned short inChannels, unsigned short inChnOffset) {
	channelExcitation = new HyperDrumExcitation* [in_channels]; // each input channel of the hdh is coupled with an excitation!
	for(int i=0; i<in_channels; i++)
		channelExcitation[i] = NULL;

	channelExciteBuff = new double* [in_channels];

	for(int i=0; i<in_channels; i++)
		channelExcitation[i] = new HyperDrumExcitation(exciteFolder, excitationLevel, rate, period_size*rate_mul, simulationRate, inChannels, inChnOffset); // periodSize*rate_mul cos we have to provide enough samples per each simulation step
}

int HyperDrumSynth::setDomain(int *excitationCoord, int *excitationDimensions, float ***domain, bool preset) {
	if(preset) {
		// 4 different areas
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

int HyperDrumSynth::saveFrame(int areaNum, float *damp, float *prop, double *exVol, double *exFiltFreq, double *exFreq, int *exType, int listPos[][2],
		                      int *frstMotion, bool *frstMotionNeg, int *frstPress, int *postMotion, bool *postMotionNeg, int *postPress,
							  float rangePress[][MAX_PRESS_MODES], float *deltaPress, int pressCnt, string filename) {
	hyperDrumheadFrame frame(areaNum);

	for(int i=0; i<areaNum; i++) {
		frame.damp[i] = damp[i];
		frame.prop[i] = prop[i];
		frame.exVol[i] = exVol[i];
		frame.exFiltFreq[i] = exFiltFreq[i];
		frame.exFreq[i] = exFreq[i];
		frame.exType[i] = exType[i];
		frame.listPos[0][i] = listPos[i][0];
		frame.listPos[1][i] = listPos[i][1];

		printf("%d list: %d, %d\n", i, frame.listPos[0][i], frame.listPos[1][i]);


		frame.firstFingerMotion[i]    = frstMotion[i];
		frame.firstFingerMotionNeg[i] = frstMotionNeg[i];
		frame.firstFingerPress[i]     = frstPress[i];
		frame.thirdFingerMotion[i]    = postMotion[i];
		frame.thirdFingerMotionNeg[i] = postMotionNeg[i];
		frame.thirdFingerPress[i]     = postPress[i];

		for(int j=0; j<pressCnt; j++)
			frame.pressRange[i][j] = rangePress[i][j];

	}

	for(int j=0; j<pressCnt; j++)
		frame.pressDelta[j] = deltaPress[j];

	return hyperDrumhead.saveFrame(&frame, filename);
}

int HyperDrumSynth::openFrame(string filename, hyperDrumheadFrame *&frame, bool reload) {
	int retval = hyperDrumhead.openFrame(filename, frame, reload);

	if(frame==NULL)
		return 3;

	for(int i=0; i<frame->numOfAreas; i++) {
		// apply settings to the synth
		setAreaDampingFactor(i, frame->damp[i]);
		setAreaPropagationFactor(i, frame->prop[i]);
		setAreaExcitationVolume(i, frame->exVol[i]);
		setAreaExLowPassFreq(i, frame->exFiltFreq[i]);
		//setAreaExcitationFreq(i, frame->exFreq[i]);
		setAreaExcitationID(i, frame->exType[i]);
		initAreaListenerPosition(i, frame->listPos[0][i], frame->listPos[1][i]);
		if( (frame->listPos[0][i]<0) && (frame->listPos[1][i]<0) )
			hideAreaListener(i);
	}
	for(int i=frame->numOfAreas; i<AREA_NUM; i++)
		hideAreaListener(i);


	return retval;
}

// screw this
double HyperDrumSynth::getSample() {
	printf("Hey! HyperDrumSynth::getSample() is not implemented, why the hell are you calling it, uh?\n");
	return 0;
}

double **HyperDrumSynth::getFrameBuffer(int numOfSamples, double **input) {
	for(int i=0; i<in_channels; i++)
		channelExciteBuff[i] = channelExcitation[i]->getFrameBuffer(numOfSamples*rateMultiplier, input)[0]; // each excitation outputs channel 0, which is then placed in the correct channel of the input buffer

	return hyperDrumhead.getFrameBuffer(numOfSamples, channelExciteBuff);
}

