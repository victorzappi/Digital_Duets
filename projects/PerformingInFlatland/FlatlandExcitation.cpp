/*
 * Simpleexcitation.cpp
 *
 *  Created on: 2015-10-26
 *      Author: vic
 */

#include "FlatlandExcitation.h"

const std::string FlatlandExcitation::excitationNames[EXCITATIONS_NUM] = {"const", "sine", "square",
		"white noise", "passthru"};

FlatlandExcitation::FlatlandExcitation(float maxLevel, unsigned int audio_rate, unsigned short period_size, unsigned int simulationRate,
								   float impLpFreq, bool use_sinc) {
	AudioGenerator::init(period_size);

	rate 				= audio_rate;
	simulation_rate 	= simulationRate;
	volume = 1;
	volumeInterp[0]	= 1;
	volumeInterp[1] = 0;

	freq = 440;
	freqInterp[0] = freq;
	freqInterp[1] = 0;

	for(int i=0; i<EXCITATIONS_NUM; i++) {
		exVol[i]          = 0;
		exVolInterp[i][0] = 0;
		exVolInterp[i][1] = 0;
	}

	audioModulesOut = new AudioModuleOut*[COMPONENTS_NUM];

	//----------------------------------------------------------------------------------------------
	// boring init...
	double excitorVolume        = maxLevel;
	double excitorFreq          = freq;
	oscillator_type excitorType = osc_sin_;

	// CHOOSE YOUR DESTINY

	// constant, technically the one for 2 mass model
	excitorType                = osc_const_;
	konst.init(simulation_rate, periodSize, excitorType, excitorVolume, excitorFreq, 0.0);

	// sine
	excitorType                = osc_sin_;
	sine.init(simulation_rate, periodSize, excitorType, excitorVolume, excitorFreq, 0.0);

	// square
	excitorType                = osc_square_;
	square.init(simulation_rate, periodSize, excitorType, excitorVolume, excitorFreq/2, 0.0, true);

	// noise
	excitorType                = osc_whiteNoise_;
	noise.init(simulation_rate, periodSize, excitorType, excitorVolume/8, excitorFreq, 0.0);

	// pack them all with the magical power of polymorphism
	audioModulesOut[0] = &konst;
	audioModulesOut[1] = &sine;
	audioModulesOut[2] = &square;
	audioModulesOut[3] = &noise;

	//----------------------------------------------------------------------------------------------

	passThruBuff = NULL;
}


// screw this
double FlatlandExcitation::getSample() {
	return getBuffer(1)[0];
}

//int sustain_cnt = 0; // for kind of natural vowel utterance
double *FlatlandExcitation::getBuffer(int numOfSamples)
{//VIC check this!
	// generic handle
	double *exBuff[EXCITATIONS_NUM-1]; // leave passthru outside

	for(int i=0; i<EXCITATIONS_NUM-1; i++)
		exBuff[i] = audioModulesOut[i]->getBuffer(numOfSamples);

	//double *tmpBuff = NULL;

	// interpolations
	for(int n=0; n<numOfSamples; n++) {
		// smooth frequencies [for all possible excitors]
		if(interpolateParam(freqInterp[0], freq, freqInterp[1])>0) {
			// cycle pitched oscillators only
			((Oscillator*) audioModulesOut[1])->setFrequency(freq);
			((Oscillator*) audioModulesOut[2])->setFrequency(freq/2);
		}

		samplebuffer[n] = exBuff[0][n]*exVol[0];
		interpolateParam(exVolInterp[0][0], exVol[0], exVolInterp[0][1]);
		for(int i=1; i<EXCITATIONS_NUM-1; i++) {
			samplebuffer[n] += exBuff[i][n]*exVol[i];
			interpolateParam(exVolInterp[i][0], exVol[i], exVolInterp[i][1]);
		}
		if(passThruBuff!=NULL) {
			samplebuffer[n] += passThruBuff[n]*exVol[EXCITATIONS_NUM-1];
			interpolateParam(exVolInterp[EXCITATIONS_NUM-1][0], exVol[EXCITATIONS_NUM-1], exVolInterp[EXCITATIONS_NUM-1][1]);
		}

		// smooth volume
		samplebuffer[n] *= volume;
		interpolateParam(volumeInterp[0], volume, volumeInterp[1]);	// remove crackles through interpolation
	}


	//VIC for debugging
/*
	for(int n=0; n<numOfSamples; n++)
		synthsamplebuffer[n] =  n;
*/

	return samplebuffer;
}



//---------------------------------------------------------------------------------------------------------------------------------
// private methods
//---------------------------------------------------------------------------------------------------------------------------------
