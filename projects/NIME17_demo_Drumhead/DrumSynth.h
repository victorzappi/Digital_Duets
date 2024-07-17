/*
 * DrumSynth.h
 *
 *  Created on: 2016-11-18
 *      Author: Victor Zappi
 */

#ifndef DRUMSYNTH_H_
#define DRUMSYNTH_H_

#include "AudioGenerator.h"
#include "ImplicitFDTD.h"
#include "DrumExcitation.h"

#include "DrumheadGPU.h"
#include "DrumheadCPU.h"


enum drumEx_type {drumEx_const, drumEx_sin, drumEx_square, drumEx_noise,
				  drumEx_impTrain, drumEx_file, drumEx_windowedImp, drumEx_oscBank};


// this is a synth, with inside another synth that is filtered by the FDTD
class DrumSynth: public AudioGenerator {
public:
	DrumSynth(bool useGpu);
	void init(string shaderLocation, int *domainSize, int audioRate, int rateMul,
			  int periodSize, float exLevel, float magnifier, int *listCoord=NULL);
	int setDomain(int *excitationCoord, int *excitationDimensions, float ***domain);

	// overloads of pure virtual methods
	double getSample();
	double *getBuffer(int numOfSamples);
	float *getBufferFloat(int numOfSamples);
	float *getBufferFloatReadTime(int numOfSamples, int &readTime);
	float *getBufferFloatNoSampleRead(int numOfSamples);

	// interface for components
	int fromWindowToDomain(int *coords);
	int setListenerPosition(int x, int y);
	void getListenerPosition(int &x, int &y);
	int shiftListenerH(int delta);
	int shiftListenerV(int delta);
	void setExcitationVolume(double v);
	void setSubGlotExType(drumEx_type subGlotExType);
	int getExcitationId();
	void setExcitationFreq(float freq);
	void setExcitationFixedFreq(int id);
	void setExcitationLowPassFreq(float freq);
	void triggerExcitation();
	void dampExcitation();
	//void setFilterToggle(int toggle);
	GLFWwindow *getWindow(int *size);
	void setCell(float type, int x, int y);
	void resetCell(int x, int y);
	void clearDomain();
	void resetSimulation();
	void setSlowMotion(bool slowMo);
	void setMousePos(int x, int y);
	void setAllWallBetas(float wallBeta);/*
	int saveFrame();
	int saveFrameSequence();
	int openFrame(string filename);
	int openFrameSequence(string filename, bool isBinary=true);
	int computeFrameSequence(string foldername);
	bool setFullscreen(bool fullScreen, int monIndex);*/
	void setDampingFactor(float dampFact);
	void setPropagationFactor(float propFact);
	void setClampingFactor(float clampFact);
	bool getUseGPU();
	void disableRender();

private:
	unsigned int simulationRate;
	unsigned int rateMultiplier;

	bool useGPU;

	//------------------------------
	DrumExcitation *excitation;
	DrumheadGPU drumheadGPU;
	DrumheadCPU drumheadCPU;
	//------------------------------


	void initExcitation(int rate_mul, float excitationLevel);
};



inline int DrumSynth::fromWindowToDomain(int *coords) {
 return drumheadGPU.fromRawWindowToDomain(coords);
}
inline int DrumSynth::setListenerPosition(int x, int y) {
	return drumheadGPU.setListenerPosition(x, y);
}
inline void DrumSynth::getListenerPosition(int &x, int &y) {
	drumheadGPU.getListenerPosition(x, y);
}
inline int DrumSynth::shiftListenerH(int delta) {
	return drumheadGPU.shiftListenerH(delta);
}
inline int DrumSynth::shiftListenerV(int delta) {
	return drumheadGPU.shiftListenerV(delta);
}
inline void DrumSynth::setExcitationVolume(double v) {
	excitation->setVolume(v);
}
inline void DrumSynth::setSubGlotExType(drumEx_type  subGlotExType) {
	excitation->setExcitationId(subGlotExType);
}
inline int DrumSynth::getExcitationId() {
	return excitation->getExcitationId();
}
inline void DrumSynth::setExcitationFreq(float freq) {
	excitation->setFreq(freq);
}
inline void DrumSynth::setExcitationFixedFreq(int id) {
	excitation->setFixedFreq(id);
}
inline void DrumSynth::setExcitationLowPassFreq(float freq) {
	excitation->setLowpassFreq(freq);
}
inline void DrumSynth::triggerExcitation() {
	excitation->excite();
}
inline void DrumSynth::dampExcitation() {
	excitation->damp();
}
inline GLFWwindow *DrumSynth::getWindow(int *size) {
	return drumheadGPU.getWindow(size);
}
inline void DrumSynth::setCell(float type, int x, int y) {
	drumheadGPU.setCell(type, x, y);
}
inline void DrumSynth::resetCell(int x, int y) {
	drumheadGPU.resetCell(x, y);
}
inline void DrumSynth::clearDomain() {
	drumheadGPU.clearDomain();
	drumheadGPU.reset();
}
inline void DrumSynth::resetSimulation() {
	drumheadGPU.reset();
}
inline void DrumSynth::setSlowMotion(bool slowMo) {
	drumheadGPU.setSlowMotion(slowMo);
}
inline void DrumSynth::setMousePos(int x, int y) {
	drumheadGPU.setMousePosition(x, y);
}
inline void DrumSynth::setAllWallBetas(float wallBeta) {
	drumheadGPU.setAllWallBetas(wallBeta);
}/*
inline int DrumSynth::saveFrame() {
	return drumheadGPU.saveFrame();
}
inline int DrumSynth::saveFrameSequence() {
	return drumheadGPU.saveFrameSequence();
}
inline int DrumSynth::openFrame(string filename) {
	return drumheadGPU.openFrame(filename);
}
inline int DrumSynth::openFrameSequence(string filename, bool isBinary) {
	return drumheadGPU.openFrameSequence(filename, isBinary);
}
inline int DrumSynth::computeFrameSequence(string foldername) {
	return drumheadGPU.computeFrameSequence(foldername);
}
inline bool DrumSynth::setFullscreen(bool fullScreen, int monIndex) {
	return drumheadGPU.setFullscreen(fullScreen, monIndex);
}
*/
inline void DrumSynth::setDampingFactor(float dampFact) {
	if(useGPU)
		drumheadGPU.setDampingFactor(dampFact);
	else
		drumheadCPU.setDampingFactor(dampFact);
}
inline void DrumSynth::setPropagationFactor(float propFact) {
	if(useGPU)
		drumheadGPU.setPropagationFactor(propFact);
	else
		drumheadCPU.setPropagationFactor(propFact);
}
inline void DrumSynth::setClampingFactor(float clampFact) {
	if(useGPU)
		drumheadGPU.setClampingFactor(clampFact);
	else
		drumheadCPU.setClampingFactor(clampFact);
}
inline bool DrumSynth::getUseGPU() {
	return useGPU;
}
inline void DrumSynth::disableRender() {
	drumheadGPU.disableRender();
}
#endif /* DRUMSYNTH_H_ */
