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

	areaExcitation = new HyperDrumExcitation* [numOfAreas];
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

//@VIC
void HyperDrumSynth::init(string shaderLocation, int *domainSize, float magnifier, int monitorID, bool fullscreen,
						  int audioRate, int rateMul, int period_size, float exLevel, string exciteFolder, int *listCoord) {

	hyperDrumhead.monitorSettings(fullscreen, monitorID); //@VIC

	periodSize      = period_size;
	rate            = audioRate;
	rateMultiplier  = (rateMul>0) ? rateMul : 1; // juuust in case

	this->domainSize[0] = domainSize[0];
	this->domainSize[1] = domainSize[1];

	simulationRate  = hyperDrumhead.init(shaderLocation, domainSize, audioRate, rateMul, period_size, magnifier, listCoord);

	initExcitation(rateMul, exLevel, exciteFolder);
}

void HyperDrumSynth::initExcitation(int rate_mul, float excitationLevel, string exciteFolder) {
	//excitation = new DrumExcitation(excitationLevel, rate, periodSize*rate_mul, simulationRate); // periodSize*rate_mul cos we have to provide enough samples per each simulation step

	for(int i=0; i<numOfAreas; i++)
		areaExcitation[i] = new HyperDrumExcitation(exciteFolder, excitationLevel, rate, periodSize*rate_mul, simulationRate); // periodSize*rate_mul cos we have to provide enough samples per each simulation step
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

/*int HyperDrumSynth::openFrame(int areaNum, string filename, float *damp, float *prop, double *exVol, double *exFiltFreq, double *exFreq, int *exType,
		                      int listPos[][2], int *frstMotion, bool *frstMotionNeg, int *frstPress, int *postMotion, bool *postMotionNeg, int *postPress,
							  float rangePress[][MAX_PRESS_MODES], float *deltaPress, int pressCnt) {
	hyperDrumheadFrame *frame;// = new hyperDrumheadFrame(areaNum);

	int retval = hyperDrumhead.openFrame(filename, frame);

	for(int i=0; i<frame->numOfAreas; i++) {
		// apply settings to the synth
		setAreaDampingFactor(i, frame->damp[i]);
		setAreaPropagationFactor(i, frame->prop[i]);
		setAreaExcitationVolume(i, frame->exVol[i]);
		setAreaExLowPassFreq(i, frame->exFiltFreq[i]);
		//setAreaExcitationFreq(i, frame->exFreq[i]);
		setExcitationID(i, frame->exType[i]);
		initAreaListenerPosition(i, frame->listPos[0][i], frame->listPos[1][i]);
		if( (frame->listPos[0][i]==-1) && (frame->listPos[1][i]==-1) )
			hideAreaListener(i);

		// update control params in controls.cpp too
		listPos[i][0] = frame->listPos[0][i];
		listPos[i][1] = frame->listPos[1][i];
		damp[i]       = frame->damp[i];
		prop[i]       = frame->prop[i];
		exVol[i]      = frame->exVol[i];
		exFiltFreq[i] = frame->exFiltFreq[i];
		exType[i]     = (drumEx_type)frame->exType[i];
		// listener pos is not useful

		frstMotion[i]    = frame->firstFingerMotion[i];
		frstMotionNeg[i] = frame->firstFingerMotionNeg[i];
		frstPress[i]     = frame->firstFingerPress[i];
		postMotion[i]    = frame->thirdFingerMotion[i];
		postMotionNeg[i] = frame->thirdFingerMotionNeg[i];
		postPress[i]     = frame->thirdFingerPress[i];

		for(int j=0; j<pressCnt; j++)
			rangePress[i][j] = frame->pressRange[i][j];
	}

	delete frame;

	return retval;
}*/

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

// and screw this too
double *HyperDrumSynth::getBuffer(int numOfSamples, float **input) {
	printf("Hey! HyperDrumSynth::getBuffer() is not implemented, why the hell are you calling it, uh?\n");
	return NULL;
}

// we override cos we don't need a local buffer! [we use TextureFilter::outputBuffer]
float *HyperDrumSynth::getBufferFloat(int numOfSamples, float **input) {

	//return excitation->getBufferFloat(numOfSamples*rateMultiplier);

	float *excBuff = NULL;//excitation->getBufferFloat(numOfSamples*rateMultiplier);
	//for(int n = 0; n < numSamples; n++)
	//	excBuff[n] *= envBuff[n];

/*	float excBuff[numOfSamples];
	for(int n = 0; n < numOfSamples; n++)
		excBuff[n] = 0;*/

	// input test
	//return excBuff;

	for(int i=0; i<numOfAreas; i++)
		areaExciteBuff[i] = areaExcitation[i]->getBufferFloat(numOfSamples*rateMultiplier, input);

	return hyperDrumhead.getBufferFloatAreas(numOfSamples, excBuff, areaExciteBuff);
}

/*float *HyperDrumSynth::getBufferFloatReadTime(int numOfSamples, int &readTime) {
	return hyperDrumhead.getBufferFloatReadTime(numOfSamples, excitation->getBufferFloat(numOfSamples*rateMultiplier), readTime);
}

float *HyperDrumSynth::getBufferFloatNoSampleRead(int numOfSamples) {
	return hyperDrumhead.getBufferFloatNoSampleRead(numOfSamples, excitation->getBufferFloat(numOfSamples*rateMultiplier));

}*/

/*HyperDrumSynth::~HyperDrumSynth() {

}*/
