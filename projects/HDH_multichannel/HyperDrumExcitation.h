/*
 * HyperDrumExcitation.h
 *
 *  Created on: Feb 12, 2018
 *      Author: Victor Zaappi
 */

#ifndef HYPERDRUMEXCITATION_H_
#define HYPERDRUMEXCITATION_H_

#include "AudioGenerator.h"
#include <string>
#include <vector>


using namespace std;

class HyperDrumExcitation : public AudioGeneratorInOut {

public:
	HyperDrumExcitation(string folder, float maxLevel, unsigned int audio_rate, unsigned int periodSize, unsigned int simulationRate, 
						unsigned short inChannels, unsigned short inChnOffset);
	~HyperDrumExcitation();
	int getComponentsNum();
	void getComponentsNames(string *names);
	void setExcitationID(int id);
	int getExcitationId();
	void setLowpassFreq(double f);
	void excite();
	void damp();
	void setVolumeFixed(double v);
	void setRelease(double t);
	bool getIsPercussive();
	bool getIsPercussive(int id);

	double **getFrameBuffer(int numOfSamples, double **input);

	void retrigger();
	void retriggerStreams();

private:
	int listFilesInFolder(string dirname);
	void fastBandLimitImpulse(float level);

	unsigned int simulation_rate;
	int currentExcitationId;
	bool antiAliasing;
	double freq;
	double lowPassFreq;
	int numOfWaveforms;
	int numOfFiles;
	int numOfInputs;
	int numOfComponents;

	vector<string> fileNames;
	vector<string> componentsNames;

	bool *componentIsPercussive;
	bool *componentIsFileStream;

	//----------------------
	Waveform bandLimitImpulse;

	Oscillator sine;
	Oscillator square;
	Oscillator noise;

	AudioFile *audiofiles;

	Passthrough *passthrough;

	ADSR adsr;

	Biquad aliasingFilter;
	Biquad lowPass;
	//------------------------------

};

inline int HyperDrumExcitation::getComponentsNum() {
	return numOfComponents;
}

inline void HyperDrumExcitation::getComponentsNames(string *names) {
	for(int i=0; i<numOfComponents; i++)
		names[i] = componentsNames[i];
}

inline void HyperDrumExcitation::setExcitationID(int id) {
	if(id>=numOfComponents) {
		printf("This HyperDrumExcitation has only %d excitation types [0 - %d], and you requested number %d...\n I am ignoring it, for your sake\n",
			    numOfComponents, numOfComponents-1, id);
		return;
	}

	currentExcitationId = id;
	//excite();
}

inline int HyperDrumExcitation::getExcitationId() {
	return currentExcitationId;
}

inline void HyperDrumExcitation::setLowpassFreq(double f) {
	lowPassFreq = f/simulation_rate;
	lowPass.setFc(lowPassFreq);
}

inline void HyperDrumExcitation::excite() {
	// retrigger all except for stream audio files and passthroughs
	for(int i=0; i<numOfComponents-numOfInputs; i++) {
		if(!componentIsFileStream[i]) {
			audioModulesOut[i]->retrigger();
		}
	}
	adsr.gate(1);
}
inline void HyperDrumExcitation::damp() {
	adsr.gate(0);
}
// no interpolation
inline void HyperDrumExcitation::setVolumeFixed(double v) {
	volume = v;
	volumeInterp[0] = v;
}
inline void HyperDrumExcitation::setRelease(double t) {
	adsr.setReleaseRate(t*rate);
}
inline bool HyperDrumExcitation::getIsPercussive() {
	return componentIsPercussive[currentExcitationId];
}
inline bool HyperDrumExcitation::getIsPercussive(int id) {
	return componentIsPercussive[id];
}

inline void HyperDrumExcitation::retrigger() {
	for(int i=0; i<numOfComponents; i++)
		audioModulesOut[i]->retrigger();
}

inline void HyperDrumExcitation::retriggerStreams() {
	for(int i=0; i<numOfComponents-numOfInputs; i++) {
		if(componentIsFileStream[i])
			audioModulesOut[i]->retrigger();
	}
}



#endif /* HYPERDRUMEXCITATION_H_ */
