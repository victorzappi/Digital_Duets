/*
 * FDTDSynth.h
 *
 *  Created on: 2016-11-18
 *      Author: Victor Zappi
 */

#ifndef FDTDSYNTH_H_
#define FDTDSYNTH_H_

#include "FDTD.h"
#include "FDTDExcitation.h"


enum ex_type {ex_sin, ex_square, ex_noise, ex_impTrain, ex_const, ex_file, ex_windowedImp, ex_oscBank};


// this is a synth, with inside another synth that is filtered by the FDTD
class FDTDSynth: public AudioGeneratorOut {
public:


	FDTDSynth();
	FDTDSynth(int *domainSize, int numOfStates, int *excitationCoord, int *excitationDimensions,
			    float ***domain, int audio_rate, int rate_mul, int periodSize);
	void init(string shaderLocation, int *domainSize, int *excitationCoord, int *excitationDimensions,
			  float ***domain, int audioRate, int rateMul, int periodSize, float magnifier=1, int *listCoord=NULL);
	void init(string shaderLocation, int *domainSize, int audioRate, int rateMul, int periodSize,
			  float magnifier, int *listCoord=NULL);
	int setDomain(int *excitationCoord, int *excitationDimensions, float ***domain);

	// overloads of pure virtual methods
	double **getFrameBuffer(int numOfSamples);
	//float *getBufferFloat(int numOfSamples);

	// interface for components...might as well inherit from FDTD but i don't like it conceptually
	int fromWindowToDomain(int *coords);
	int setListenerPosition(int x, int y);
	void getListenerPosition(int &x, int &y);
	int shiftListenerH(int delta);
	int shiftListenerV(int delta);
	int setPhysicalParameters(void *physicalParams);
	int setMaxPressure(float maxP);
	void setExcitationType(ex_type  excitationType);
	void setExcitationVolume(double v);
	void setExcitationFreq(float freq);
	void setExcitationFixedFreq(int id);
	//void setExcitationLowPassFreq(float freq);
	GLFWwindow *getWindow(int *size);
	void setCell(float type, int x, int y);
	void resetCell(int x, int y);
	void clearDomain();
	void resetSimulation();
	void setSlowMotion(bool slowMo);
	void setAllWallBetas(float wallBeta);
	void setMousePos(int x, int y);
	bool monitorSettings(bool fullScreen, int monIndex);

	void retrigger();

private:
	unsigned int simulationRate;
	unsigned int rateMultiplier;

	//------------------------------
	FDTDExcitation excitation;
	FDTD fdtd;
	//------------------------------

	void initExcitation(int rate_mul);
};


// this series of wrapper methods could be avoided deriving this class also from FDTD, but i do not like it conceptually
inline int FDTDSynth::fromWindowToDomain(int *coords) {
 return fdtd.fromRawWindowToDomain(coords);
}
inline int FDTDSynth::setListenerPosition(int x, int y) {
	return fdtd.setListenerPosition(x, y);
}
inline void FDTDSynth::getListenerPosition(int &x, int &y) {
	fdtd.getListenerPosition(x, y);
}
inline int FDTDSynth::shiftListenerH(int delta) {
	return fdtd.shiftListenerH(delta);
}
inline int FDTDSynth::shiftListenerV(int delta) {
	return fdtd.shiftListenerV(delta);
}
inline int FDTDSynth::setPhysicalParameters(void *physicalParams) {
	return fdtd.setPhysicalParameters(physicalParams);
}
inline int FDTDSynth::setMaxPressure(float maxP) {
	return fdtd.setMaxPressure(maxP);
}
inline void FDTDSynth::setExcitationType(ex_type  excitationType) {
	excitation.setExcitationId(excitationType);
}
inline void FDTDSynth::setExcitationVolume(double v) {
	excitation.setVolume(v);
}
inline void FDTDSynth::setExcitationFreq(float freq) {
	excitation.setFreq(freq);
}
inline void FDTDSynth::setExcitationFixedFreq(int id) {
	excitation.setFixedFreq(id);
}
/*inline void FDTDSynth::setExcitationLowPassFreq(float freq) {
	excitation->setLowpassFreq(freq);
}*/
inline GLFWwindow *FDTDSynth::getWindow(int *size) {
	return fdtd.getWindow(size);
}
inline void FDTDSynth::setCell(float type, int x, int y) {
	fdtd.setCell(type, x, y);
}
inline void FDTDSynth::resetCell(int x, int y) {
	fdtd.resetCell(x, y);
}
inline void FDTDSynth::clearDomain() {
	fdtd.clearDomain();
}
inline void FDTDSynth::resetSimulation() {
	fdtd.reset();
}
inline void FDTDSynth::setSlowMotion(bool slowMo) {
	fdtd.setSlowMotion(slowMo);
}
inline void FDTDSynth::setAllWallBetas(float wallBeta) {
	fdtd.setAllWallBetas(wallBeta);
}
inline void FDTDSynth::setMousePos(int x, int y) {
	fdtd.setMousePosition(x, y);
}
inline bool FDTDSynth::monitorSettings(bool fullScreen, int monIndex) {
	return fdtd.monitorSettings(fullScreen, monIndex);
}

inline void FDTDSynth::retrigger() {
	excitation.retrigger();
}
#endif /* FDTDSYNTH_H_ */
