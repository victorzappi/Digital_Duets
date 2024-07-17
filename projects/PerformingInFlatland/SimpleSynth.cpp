/*
 * SimpleSynth.cpp
 *
 *  Created on: 2016-09-14
 *      Author: Victor Zappi
 */

#include "SimpleSynth.h"
SimpleSynth::SimpleSynth(unsigned int audio_rate, unsigned short period_size) {
	AudioGenerator::init(period_size);

	rate = audio_rate;

	sin.init(rate, periodSize, osc_sin_, 0.0, 880);
}

double SimpleSynth::getSample()
{
	/* old code
	synthsample = (excite.*excite.getSample)();
	synthsample = aliasingFilter.process(synthsample);

	//synthsample = audiofile.getSample();

	interpolateParam(ref_volume, volume, inc_volume);	// remove crackles through interpolation

	return synthsample*volume;			  			 	// multiply by global synth volume*/


	return getBuffer(1)[0];
}

double *SimpleSynth::getBuffer(int numOfSamples) {
	memcpy(samplebuffer, sin.getBuffer(numOfSamples), sizeof(double)*numOfSamples);
	return samplebuffer;
}
