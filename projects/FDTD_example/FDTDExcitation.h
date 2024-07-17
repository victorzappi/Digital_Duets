/*
 * Simpleexcitation.h
 *
 *  Created on: 2015-10-26
 *      Author: vic
 */

#ifndef SIMPLEEXCITATION_H_
#define SIMPLEEXCITATION_H_

#define OSC_NUM 10
#define COMPONENTS_NUM 7
#define EXCITATIONS_NUM COMPONENTS_NUM+1 // +1 is osc bank
#define FREQS_NUM 8

#include "AudioGenerator.h"
#include <string>

class FDTDExcitation: public AudioGeneratorOut {
public:

	static const std::string excitationNames[EXCITATIONS_NUM];

	FDTDExcitation();
	~FDTDExcitation();
	void init(unsigned int audio_rate, unsigned int simulationRate, unsigned short periodSize, float maxLevel=1);
	double **getFrameBuffer(int numOfSamples);
	void setExcitationId(int id);
	int getExcitationId();
	void setFreq(double f);
	void setFixedFreq(int id);
	void retrigger();

private:
	void computeBandLimitImpulse();

	unsigned int simulation_rate;
	bool useBank;
	bool useMod;
	bool antiAliasing;
	// handy union to interpret the whole buffer as both double and float...saves memory but slower than using floatsamplebuffer
	/*union ExcitationBuffer {
		float *f = NULL; // only one init is admitted [and enough (:]
		double *d;
	} excitationBuffer;*/

	double freq; // oscillators freq

	float fixedFreq[FREQS_NUM];
	float impulseSlowDown;

	int currentExcitationId;

	//------------------------------
	Oscillator sine;
	Oscillator square;
	Oscillator noise;
	Oscillator impulse;
	Oscillator cons;

	AudioFile audiofile;
	Waveform bandLimitImpulse;
	Oscillator oscBank[OSC_NUM];

	Oscillator modulator;
	ADSR smoothAttack;

	Biquad aliasingFilter;

	//------------------------------
};

inline int FDTDExcitation::getExcitationId() {
	return currentExcitationId;
}

inline void FDTDExcitation::setFixedFreq(int id) {
	if(id<COMPONENTS_NUM-1)
		setFreq(fixedFreq[id]);
}

inline void FDTDExcitation::retrigger() {
	for(int i=0; i<COMPONENTS_NUM; i++)
		audioModulesOut[i]->retrigger();
}



#endif /* SIMPLEEXCITATION_H_ */
