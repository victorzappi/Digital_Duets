/*
 * DrumExcitation.cpp
 *
 *  Created on: 2016-10-26
 *      Author: Victor Zappi
 */

#include "DrumExcitation.h"

const std::string DrumExcitation::excitationNames[EXCITATIONS_NUM] = {"generic osc", "const", "sine", "square",
		"white noise", "impulse train", "audio file", "band limited impulse", "oscillator bank"};

DrumExcitation::DrumExcitation(float maxLevel, unsigned int audio_rate, unsigned short periodSize, unsigned int simulationRate) {
	AudioGeneratorOut::init(periodSize);

	rate 				= audio_rate;
	simulation_rate 	= simulationRate;
	volume = 1;
	volumeInterp[0]	= 1;
	volumeInterp[1] = 0;

	glottalExcitation = false;
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

	lowPassFreq = ( ((double)rate)/2.0 )/simulation_rate;

	audioModulesOut = new AudioModuleOut*[COMPONENTS_NUM];

	//----------------------------------------------------------------------------------------------
	// boring init...
	double excitorVolume        = maxLevel;
	double excitorFreq          = freq;
	oscillator_type excitorType = osc_sin_;

	// CHOOSE YOUR DESTINY

	// generic excitation osc, kind of a sandbox to mess up with
	excitorType                = osc_sin_;
	excite.init(excitorType, simulation_rate, period_size, excitorVolume, excitorFreq, 0.0);

	// constant, technically the one for 2 mass model
	excitorType                = osc_const_;
	glottalExcite.init(excitorType, simulation_rate, period_size, excitorVolume, excitorFreq, 0.0);

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

	// audio files
	std::string filename = "audiofiles/drum7.wav";
	//std::string filename = "audiofiles/bowedC_.wav";
	audiofile.init(filename, simulation_rate, period_size, maxLevel);
	audiofile.setAdvanceType(adv_loop_);

	// special impulse
	computeBandLimitImpulse();

	// impulse from file
	filename = "audiofiles/plate_impulse.wav";

	// pack them all with the magical power of polymorphism
	audioModulesOut[0] = &excite;
	audioModulesOut[1] = &glottalExcite;
	audioModulesOut[2] = &sine;
	audioModulesOut[3] = &square;
	audioModulesOut[4] = &noise;
	audioModulesOut[5] = &impulse;
	audioModulesOut[6] = &audiofile;
	audioModulesOut[7] = &bandLimitImpulse;

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
	// for kind of natural vowel utterance
	/*smoothAttack.setAttackRate(0.502*rate);
	smoothAttack.setDecayRate(0.2*rate); // not used
	smoothAttack.setSustainLevel(0.95);
	smoothAttack.setReleaseRate(1.5*rate); // not used*/
	smoothAttack.gate(1);

	// anti aliasing filter, needed when rate_mul > 1 [useless for now on files...]
	aliasingFilter.setBiquad(bq_type_lowpass, ( ((double)rate)/2.0 )/simulation_rate,  0.7071, 0);

	lowPass.setBiquad(bq_type_lowpass, lowPassFreq,  0.7071, 0);
	//----------------------------------------------------------------------------------------------
}

//int sustain_cnt = 0; // for kind of natural vowel utterance
double **DrumExcitation::getFrameBuffer(int numOfSamples)
{
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
	if(antiAliasing && currentExcitationId != 7 && currentExcitationId != 8) // sinc impulse and file impulse do not want aliasing, it would ruin their perfection!
		aliasingFilter.process(tmpBuf, numOfSamples);

	// attack adsr
	float *adsrBuf = smoothAttack.processBuffer(numOfSamples);
	// for kind of natural vowel utterance
	/*if(smoothAttack.getState()==env_sustain) {
		sustain_cnt++;
		if(sustain_cnt == 100) {
			smoothAttack.gate(0);
			printf("release\n");
		}
	}*/
	// sinc impulse and file impulse do not want attack adsr either, again it would ruin their perfection!
	if(currentExcitationId == 7 || currentExcitationId == 8) {
		for(int n=0; n<numOfSamples; n++)
			adsrBuf[n] = 1;
	}


	//memcpy(samplebuffer, tmpBuf, sizeof(double)*numOfSamples); // this would be used if we plan not to have interpolations at all. it's fast

	// interpolations
	for(int n=0; n<numOfSamples; n++) {
		// apply filter
		tmpBuf[n] = lowPass.process(tmpBuf[n]); // this might change in the control thread while we are here

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

/*	for(int n=0; n<numOfSamples; n++)
		printf("samplebuffer[%d]: %f\n", n, samplebuffer[n]);*/

	return framebuffer;
}

void DrumExcitation::setFreq(double f) {
	freq = f;
	// cycle oscillators only, no sinc windowed inpulse, file impulse and osc bank
	for(int i=0; i<COMPONENTS_NUM-2; i++) {
		// impulse osc has special treatment
		if(i!=5)
			((Oscillator*) audioModulesOut[i])->setFrequency(freq);
		else
			((Oscillator*) audioModulesOut[i])->setFrequency(freq*impulseSlowDown);
	}
}

void DrumExcitation::setExcitationId(int id) {
	if(id<COMPONENTS_NUM) {
		currentExcitationId = id;
		useBank = false;

		if(currentExcitationId == 7)
			bandLimitImpulse.retrigger();

	}
	else if(id == COMPONENTS_NUM) {
		useBank = true;
	}
	else {
		printf("This DrumExcitation has only %d excitation types [0 - %d], and you requested number %d...\n I am ignoring it, for your sake\n",
				EXCITATIONS_NUM, EXCITATIONS_NUM-1, id);
	}
}

//---------------------------------------------------------------------------------------------------------------------------------
// private methods
//---------------------------------------------------------------------------------------------------------------------------------
void DrumExcitation::computeBandLimitImpulse() {
	/*if(useSinc)*/ {
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

		bandLimitImpulse.init(sinc_windowed_impulse, M+1, period_size, 1);
		bandLimitImpulse.setAdvanceType(adv_oneShot_);
	}
	/*else {
		//-------------------------------------
		// let's create a gaussian filter
		// which is almost an impulse...
		//-------------------------------------
		double maxFrequency = 1000;   // low pass freq, set from outside

		double dt = 1.0/simulation_rate;
		double centerFrequency = 0.75 * maxFrequency / 1.51742712939;
		double sigma = 1.0 / (M_PI * centerFrequency);
		double delay = 5 * sigma;
		double ampNorm = sigma * sqrt(exp(1) / 2.0); // This gives max amplitude to 1

		int n = 2*delay / dt + 1; // number of samples

		// fill it
		double gauss_filter[n];
		double t = -delay;
		for (int i=0; i<n; i++) {
			gauss_filter[i] = ampNorm * (-2*t/ (sigma*sigma) ) * exp(-t*t / (sigma*sigma) );
			t += dt;
			//gauss_filter[i] = 0;
		}

		//gauss_filter[0] = 1;

		bandLimitImpulse.init(0.1, gauss_filter, n, period_size);
		bandLimitImpulse.setAdvanceType(adv_oneShot_);
	}*/
}
