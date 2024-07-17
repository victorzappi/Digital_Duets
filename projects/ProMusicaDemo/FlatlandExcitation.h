/*
 * Simpleexcitation.h
 *
 *  Created on: 2015-10-26
 *      Author: vic
 */

#ifndef SIMPLEEXCITATION_H_
#define SIMPLEEXCITATION_H_

#define OSC_NUM 10
#define COMPONENTS_NUM 4
#define EXCITATIONS_NUM COMPONENTS_NUM+1 // +1 passthru
#define FREQS_NUM 8


#include "AudioGenerator.h"
#include <string>

class FlatlandExcitation: public AudioGenerator {
public:

	static const std::string excitationNames[EXCITATIONS_NUM];

	FlatlandExcitation(float maxLevel, unsigned int audio_rate, unsigned short period_size, unsigned int simulationRate, float impLpFNorm, bool use_sinc=true);
	double getSample();
	double *getBuffer(int numOfSamples);
	void setFreq(double f);
	void setFixedFreq(int id);
	void setPassThruBuffer(double *buff);
	void setExcitationVol(int index, double vol);
	void retrigger();

private:
	unsigned int simulation_rate;

	double freq;          // parameter
	double freqInterp[2]; // reference and increment

	double exVol[EXCITATIONS_NUM]; // parameter
	double exVolInterp[EXCITATIONS_NUM][2]; // reference and increment

	//------------------------------
	Oscillator konst;
	Oscillator sine;
	Oscillator square;
	Oscillator noise;
	//------------------------------

	double *passThruBuff;
};


inline void FlatlandExcitation::setFreq(double f) {
	/*double ref_freq = f;
	double inc_freq = (ref_freq-freq)/(double)periodSize;

	double interp[2] = {ref_freq, inc_freq};

	memcpy(freqInterp, interp, sizeof(double)*2); // one shot update, made for multi core parallel threads*/
	prepareInterpolateParam(freq, freqInterp, f);

}

inline void FlatlandExcitation::setPassThruBuffer(double *buff) {
	passThruBuff = buff;
}

//VIC test this
// and then map to midi
// set reference and prepare increment for linear interpolation
inline void FlatlandExcitation::setExcitationVol(int index, double vol) {
/*
	double ref_vol = vol;
	double inc_vol = (ref_vol-exVol[index])/(double)periodSize;

	double interp[2] = {ref_vol, inc_vol};

	memcpy(exVolInterp[index], interp, sizeof(double)*2); // one shot update, made for multi core parallel threads
*/
	prepareInterpolateParam(exVol[index], exVolInterp[index], vol);
}

inline void FlatlandExcitation::retrigger() {
}


#endif /* SIMPLEEXCITATION_H_ */
