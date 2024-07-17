/*
 * SimpleSynth.h
 *
 *  Created on: 2016-09-14
 *      Author: Victor Zappi
 */

#ifndef SIMPLESYNTH_H_
#define SIMPLESYNTH_H_


#include <iostream>
#include <string>
#include <stdio.h>
#include <stdlib.h>

#include "AudioGenerator.h"


class SimpleSynth : public AudioGenerator {
public:
	SimpleSynth(unsigned int audio_rate, unsigned short period_size);
	double getSample();
	double *getBuffer(int numOfSamples);

private:
	//------------------------------
	Oscillator sin;
	//------------------------------

};

#endif /* SIMPLESYNTH_H_ */
