/*
 * DrumExcitation.h
 *
 *  Created on: 2015-10-26
 *      Author: vic
 */

#ifndef DRUMEXCITATION_H_
#define DRUMEXCITATION_H_

#define OSC_NUM 10
#define COMPONENTS_NUM 12
#define EXCITATIONS_NUM COMPONENTS_NUM+1 // +1 is osc bank
#define FREQS_NUM 8

#include "AudioGenerator.h"
#include <string>

class DrumExcitation: public AudioGenerator {
public:

	static const std::string excitationNames[EXCITATIONS_NUM];

	DrumExcitation(float maxLevel, unsigned int audio_rate, unsigned short period_size, unsigned int simulationRate);
	~DrumExcitation();
	double getSample();
	double *getBuffer(int numOfSamples);
	void setExcitationId(int id);
	int getExcitationId();
	void setFreq(double f);
	void setFixedFreq(int id);
	void setLowpassFreq(double f);
	void excite();
	void damp();

private:
	void computeBandLimitImpulse();
	void computeGaussianImpulse();

	unsigned int simulation_rate;
	bool glottalExcitation;
	bool useBank;
	bool useMod;
	bool antiAliasing;

	double freq; // oscillators freq

	float fixedFreq[FREQS_NUM];
	float impulseSlowDown;

	int currentExcitationId;

	double lowPassFreq;

	//------------------------------
	Waveform bandLimitImpulse;

	Oscillator constant;
	Oscillator sine;
	Oscillator square;
	Oscillator noise;
	Oscillator impulse;

	AudioFile audiofile[6];
	Oscillator oscBank[OSC_NUM];

	Oscillator modulator;
	ADSR smoothAttack;

	Biquad aliasingFilter;

	Biquad lowPass;
	//------------------------------
};

inline int DrumExcitation::getExcitationId() {
	return currentExcitationId;
}

inline void DrumExcitation::setFixedFreq(int id) {
	if(id>=0 && id<FREQS_NUM)
		setFreq(fixedFreq[id]);
}
inline void DrumExcitation::setLowpassFreq(double f) {
	lowPassFreq = f/simulation_rate;
	lowPass.setFc(lowPassFreq);
}
inline void DrumExcitation::excite() {
	bandLimitImpulse.retrigger();
	for(int i=0; i<6; i++)
		audiofile[i].retrigger();
	smoothAttack.gate(1);
}
inline void DrumExcitation::damp() {
	smoothAttack.gate(0);
}



#endif /* DRUMEXCITATION_H_ */
