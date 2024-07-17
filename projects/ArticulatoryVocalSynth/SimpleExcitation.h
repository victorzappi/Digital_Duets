/*
 * Simpleexcitation.h
 *
 *  Created on: 2015-10-26
 *      Author: vic
 */

#ifndef SIMPLEEXCITATION_H_
#define SIMPLEEXCITATION_H_

#define OSC_NUM 10
#define COMPONENTS_NUM 9
#define EXCITATIONS_NUM COMPONENTS_NUM+1 // +1 is osc bank
#define FREQS_NUM 8

#include "AudioGenerator.h"
#include <string>

class SimpleExcitation: public AudioGeneratorOut {
public:

	static const std::string excitationNames[EXCITATIONS_NUM];

	SimpleExcitation(float maxLevel, unsigned int audio_rate, unsigned short period_size, unsigned int simulationRate, float impLpFNorm, bool use_sinc=true);
	~SimpleExcitation();
	double getSample();
	double *getBuffer(int numOfSamples);
	void setExcitationId(int id);
	int getExcitationId();
	void setFreq(double f);
	void setFixedFreq(int id);
	void setLowpassFreq(double f);
	void retrigger();

private:
	unsigned int simulation_rate;
	bool glottalExcitation;
	bool useBank;
	bool useMod;
	bool antiAliasing;
	// handy union to interpret the whole buffer as both double and float...saves memory but slower than using floatsamplebuffer
	/*union ExcitationBuffer {
		float *f = NULL; // only one init is admitted [and enough (:]
		double *d;
	} excitationBuffer;*/

	float level;

	double freq; // oscillators freq

	float fixedFreq[FREQS_NUM];
	float impulseSlowDown;

	int currentExcitationId;

	double lowPassFreq;

	float impulseLpFreq;
	bool useSinc;

	//------------------------------
	Oscillator excite;
	Oscillator glottalExcite;
	Oscillator sine;
	Oscillator square;
	Oscillator noise;
	Oscillator impulse;

	AudioFile audiofile;
	Waveform bandLimitImpulse;
	AudioFile fileImpulse;
	Oscillator oscBank[OSC_NUM];

	Oscillator modulator;
	ADSR smoothAttack;

	Biquad aliasingFilter;

	Biquad lowPass;
	//------------------------------

	void computeBandLimitImpulse();
};

inline int SimpleExcitation::getExcitationId() {
	return currentExcitationId;
}

inline void SimpleExcitation::setFixedFreq(int id) {
	if(id>=0 && id<FREQS_NUM)
		setFreq(fixedFreq[id]);
}

inline void SimpleExcitation::setLowpassFreq(double f) {
	lowPassFreq = f/simulation_rate;
	lowPass.setFc(lowPassFreq);
}

inline void SimpleExcitation::retrigger() {

}




#endif /* SIMPLEEXCITATION_H_ */
