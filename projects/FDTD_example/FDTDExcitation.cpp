/*
 * Simpleexcitation.cpp
 *
 *  Created on: 2015-10-26
 *      Author: vic
 */

#include "FDTDExcitation.h"

const std::string FDTDExcitation::excitationNames[EXCITATIONS_NUM] = {"sine", "square",
		"white noise", "impulse train", "const", "audio file", "band limited impulse", "oscillator bank"};

FDTDExcitation::FDTDExcitation() {
	simulation_rate = 0;
	useBank = false;
	useMod = false;
	antiAliasing = false;

	freq = 0;

	for(int i=0; i<FREQS_NUM; i++)
		fixedFreq[i] = 0;

	impulseSlowDown = 0;;

	currentExcitationId = -1;
}

void FDTDExcitation::init(unsigned int audio_rate, unsigned int simulationRate, unsigned short periodSize, float maxLevel) {
	AudioGeneratorOut::init(periodSize);

	rate 				= audio_rate;
	simulation_rate 	= simulationRate;
	volume = 1;
	volumeInterp[0]	= 1;
	volumeInterp[1] = 0;

	useBank = false;
	useMod  = false;

	antiAliasing = (rate != simulation_rate);

	fixedFreq[0] = 50.0;
	fixedFreq[1] = 100.0;
	fixedFreq[2] = 200.0;
	fixedFreq[3] = 400.0;
	fixedFreq[4] = 800.0;
	fixedFreq[5] = 1600.0;
	fixedFreq[6] = 3200.0;
	fixedFreq[7] = 6400.0;

	impulseSlowDown = 0.02;

	currentExcitationId = 0;

	freq = 440;

	audioModulesOut = new AudioModuleOut*[COMPONENTS_NUM];

	//----------------------------------------------------------------------------------------------
	// boring init...
	double excitorVolume        = maxLevel;
	double excitorFreq          = freq;
	oscillator_type excitorType = osc_sin_;

	// CHOOSE YOUR DESTINY
	// sine
	excitorType                = osc_sin_;
	sine.init(excitorType, simulation_rate, period_size, excitorVolume, excitorFreq, 0.0);

	// square
	excitorType                = osc_square_;
	square.init(excitorType, simulation_rate, period_size, excitorVolume, excitorFreq, 0.0, true);

	// noise
	excitorType                = osc_whiteNoise_;
	noise.init(excitorType, simulation_rate, period_size, excitorVolume, excitorFreq, 0.0);

	// impulse train
	excitorFreq                = excitorFreq*impulseSlowDown; // let's slow down the impulse a bit, cos we are more interested into low freqs with it
	excitorType                = osc_impTrain_;
	impulse.init(excitorType, simulation_rate, period_size, excitorVolume, excitorFreq, 0.0, true);

	// constant
	excitorType                = osc_const_;
	cons.init(excitorType, simulation_rate, period_size, excitorVolume, excitorFreq, 0.0);

	// audio files
	std::string filename = "audiofiles/drum7.wav";
	//std::string filename = "audiofiles/bowedC_.wav";
	audiofile.init(filename, simulation_rate, period_size, maxLevel);
	audiofile.setAdvanceType(adv_loop_);

	// special impulse
	computeBandLimitImpulse();

	// pack them all with the magical power of polymorphism
	audioModulesOut[0] = &sine;
	audioModulesOut[1] = &square;
	audioModulesOut[2] = &noise;
	audioModulesOut[3] = &impulse;
	audioModulesOut[4] = &cons;
	audioModulesOut[5] = &audiofile;
	audioModulesOut[6] = &bandLimitImpulse;

	// osc bank
	excitorVolume = maxLevel/(OSC_NUM);
	excitorFreq   = 440.0;
	excitorType   = osc_sin_;

	for(int i=0; i<OSC_NUM; i++)
		oscBank[i].init(excitorType, simulation_rate, period_size, excitorVolume, excitorFreq/(1+i), 0.0);

	// modulation
	modulator.init(osc_sin_, simulation_rate, period_size, 1.0, 220, 0.0, true);

	// adsr
	smoothAttack.setPeriodSize(period_size);
	smoothAttack.setAttackRate(0.2*rate);
	smoothAttack.setDecayRate(0.1*rate); // not used
	smoothAttack.setSustainLevel(1);
	smoothAttack.setReleaseRate(0.1*rate); // not used
	smoothAttack.gate(1);

	// anti aliasing filter, needed when rate_mul > 1 [useless for now on files...]
	aliasingFilter.setBiquad(bq_type_lowpass, ( ((double)rate)/2.0 )/simulation_rate,  0.7071, 0);
	//----------------------------------------------------------------------------------------------
}

FDTDExcitation::~FDTDExcitation() {
	if(audioModulesOut != NULL)
		delete[] audioModulesOut;
}

//int sustain_cnt = 0; // for kind of natural vowel utterance
double **FDTDExcitation::getFrameBuffer(int numOfSamples) {
	// generic handle
	double *tmpBuf;

	if(!useBank)
		tmpBuf = audioModulesOut[currentExcitationId]->getFrameBuffer(numOfSamples)[0];
	// osc bank
	else if(useBank) {
		double * oscBankBuff;
		tmpBuf = oscBank[0].getFrameBuffer(numOfSamples)[0];
		for(int i=1; i<OSC_NUM; i++) {
			oscBankBuff = oscBank[i].getFrameBuffer(numOfSamples)[0];
			for(int n=0; n<numOfSamples; n++)
				tmpBuf[n] += oscBankBuff[n];
		}
	}

	// modulation
	if(useMod) {
		double *modBuff =  modulator.getFrameBuffer(numOfSamples)[0];
		for(int n=0; n<numOfSamples; n++)
			tmpBuf[n] *= modBuff[n];
	}

	// anti aliasing
	if(antiAliasing)
		aliasingFilter.process(tmpBuf, numOfSamples);

	// attack adsr
	float *adsrBuf = smoothAttack.processBuffer(numOfSamples);

	//memcpy(samplebuffer, tmpBuf, sizeof(double)*numOfSamples);  // this would be used if we plan not to have sample based controls at all. it's fast

	// sample based controls
	for(int n=0; n<numOfSamples; n++) {
		// apply adsr
		tmpBuf[n] *= adsrBuf[n];

		// smooth volume
		framebuffer[0][n] =  tmpBuf[n]*volume;
		interpolateParam(volumeInterp[0], volume, volumeInterp[1]);	// remove crackles through interpolation
	}


	//VIC for debugging
/*
	for(int n=0; n<numOfSamples; n++)
		samplebuffer[n] =  n;
*/

	return framebuffer;
}

void FDTDExcitation::setFreq(double f) {
	freq = f;
	// cycle oscillators only, no sinc windowed inpulse, file impulse and osc bank
	for(int i=0; i<COMPONENTS_NUM-2; i++) {
		// impulse osc has special treatment
		if(i!=3)
			((Oscillator*) audioModulesOut[i])->setFrequency(freq);
		else
			((Oscillator*) audioModulesOut[i])->setFrequency(freq*impulseSlowDown);
	}
}

void FDTDExcitation::setExcitationId(int id) {
	if(id<COMPONENTS_NUM) {
		currentExcitationId = id;
		useBank = false;

		if(currentExcitationId == 6)
			bandLimitImpulse.retrigger();
	}
	else if(id == COMPONENTS_NUM) {
		useBank = true;
	}
	else {
		printf("This FDTDExcitation has only %d excitation types [0 - %d], and you requested number %d...\n I am ignoring it, for your sake\n",
				EXCITATIONS_NUM, EXCITATIONS_NUM-1, id);
	}
}



//---------------------------------------------------------------------------------------------------------------------------------
// private methods
//---------------------------------------------------------------------------------------------------------------------------------
void FDTDExcitation::computeBandLimitImpulse() {
	//-------------------------------------
	// let's create a sinc-windowed impulse [band pass filter]
	// composed of a low pass sinc and high pass sinc
	//-------------------------------------

	// expressed as fraction of simulation rate
	double lpf = 22000.0/simulation_rate; // low pass freq, set from outside
	double hpf = 2.0/simulation_rate;           // high pass freq, 2 Hz
	double bw  = 0.001; 					    // roll-off bandwidth
	double amp = 1* ((float)simulation_rate)/rate;

	int M = 4/bw;

	double sinc_windowed_impulse[M+1];

	// working vars
	double lp_sinc[M+1];
	double hp_sinc[M+1];

	float sum = 0; // to normalize filters

	//-----------------------------------------------------------------------------
	// generate filter as a band reject filter and then do spectral inversion

	// generate first sinc as low pass [opposite of what we need]
	for(int i=0; i<M+1; i++) {
		if (i!=M/2)
			lp_sinc[i] = sin(2*M_PI*hpf * (i-M/2)) / (i-M/2);
		else
			lp_sinc[i] = 2*M_PI*hpf; // cope with division by zero
		lp_sinc[i] = lp_sinc[i] * (0.54 - 0.46*cos(2*M_PI*i/M)); // multiply by Hamming window
		sum += lp_sinc[i];
	}
	// normalize
	for(int i=0; i<M+1; i++)
		lp_sinc[i] /= sum;


	// generate second sinc as low pass sinc and then invert it [to have opposite of what we need]
	sum = 0;
	for(int i=0; i<M+1; i++) {
		if (i!=M/2)
			hp_sinc[i] = sin(2*M_PI*lpf * (i-M/2)) / (i-M/2);
		else
			hp_sinc[i] = 2*M_PI*lpf;
		hp_sinc[i] = hp_sinc[i] * (0.54 - 0.46*cos(2*M_PI*i/M)); // multiply by Hamming window
		sum += hp_sinc[i];
	}
	// normalize and invert, to turn into high pass sinc
	for(int i=0; i<M+1; i++) {
		hp_sinc[i] /= sum;
		hp_sinc[i] = -hp_sinc[i];
	}
	hp_sinc[M/2] += 1;


	// combine two sincs in frequency reject filter [opposite of what we need]
	// and do spectal inversion for band pass [this is what we need]
	for(int i=0; i<M+1; i++) {
		sinc_windowed_impulse[i] = hp_sinc[i] + lp_sinc[i];
		sinc_windowed_impulse[i] = -sinc_windowed_impulse[i];
	}
	sinc_windowed_impulse[M/2] = sinc_windowed_impulse[M/2] + 1;

	// set amplitude, just in case
	for(int i=0; i<M+1; i++)
		sinc_windowed_impulse[i] *= amp; // no we're ready to go!

	bandLimitImpulse.init(sinc_windowed_impulse, M+1, period_size);
	bandLimitImpulse.setAdvanceType(adv_oneShot_);

}

