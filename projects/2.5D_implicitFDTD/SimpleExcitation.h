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

class SimpleExcitation: public AudioGenerator {
public:

	static const std::string excitationNames[EXCITATIONS_NUM];

	SimpleExcitation(float maxLevel, unsigned int audio_rate, unsigned int period_size, unsigned int simulationRate, float impLpFNorm, bool use_sinc=true);
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

	double freq;          // parameter
	double freqInterp[2]; // reference and increment

	float fixedFreq[FREQS_NUM];
	float impulseSlowDown;

	int currentExcitationId;

	double lowPassFreq;		     // parameter
	double lowPassFreqInterp[2]; // reference and increment

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

inline void SimpleExcitation::setFreq(double f) {
	double ref_freq = f;
	double inc_freq = (ref_freq-freq)/(double)periodSize;

	double interp[2] = {ref_freq, inc_freq};

	memcpy(freqInterp, interp, sizeof(double)*2); // one shot update, made for multi core parallel threads

}
inline void SimpleExcitation::setFixedFreq(int id) {
	if(id>=0 && id<FREQS_NUM)
		setFreq(fixedFreq[id]);
}
inline void SimpleExcitation::setLowpassFreq(double f) {
	double ref_lowPassFreq  = f/simulation_rate;
	double inc_lowPassFreq = (ref_lowPassFreq-lowPassFreq)/(double)periodSize;

	double interp[2] = {ref_lowPassFreq, inc_lowPassFreq};

	memcpy(lowPassFreqInterp, interp, sizeof(double)*2); // one shot update, made for multi core parallel threads
}

inline void SimpleExcitation::retrigger() {

}


#endif /* SIMPLEEXCITATION_H_ */
