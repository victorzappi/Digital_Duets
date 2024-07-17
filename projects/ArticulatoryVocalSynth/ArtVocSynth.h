/*
 * ArtVocSynth.h
 *
 *  Created on: 2015-11-18
 *      Author: vic
 */

#ifndef FDTDSYNTH_H_
#define FDTDSYNTH_H_

#include "AudioGenerator.h"
#include "ImplicitFDTD.h"
#include "SimpleExcitation.h"

#include "VocalTractFDTD.h"


enum subGlotEx_type {subGlotEx_sandbox, subGlotEx_const, subGlotEx_sin, subGlotEx_square, subGlotEx_noise,
					 subGlotEx_impTrain, subGlotEx_file, subGlotEx_windowedImp, subGlotEx_fileImp, subGlotEx_oscBank};


// this is a synth, with inside another synth that is filtered by the FDTD
class ArtVocSynth: public AudioGeneratorOut {
public:

	ArtVocSynth();
	~ArtVocSynth();
	void init(string shaderLocation, int *domainSize, int *excitationCoord, int *excitationDimensions,
			  float ***domain, int audioRate, int rateMul, int periodSize, float magnifier=1, int *listCoord=NULL, float exLpF=22050, bool useSinc=true);
	void init(string shaderLocation, int *domainSize, int audioRate, int rateMul, int periodSize,
			  float magnifier, int *listCoord=NULL, float exLpF=22050, bool useSinc=true);
	int setDomain(int *excitationCoord, int *excitationDimensions, float ***domain);
	int loadContourFile(int domain_size[2], float deltaS, string filename, float ***pixels);

	// overloads of pure virtual methods
	double getSample();
	double *getBuffer(int numOfSamples);
	float *getBufferFloat(int numOfSamples); // for offline computation

	// interface for components
	int fromWindowToDomain(int *coords);
	int setListenerPosition(int x, int y);
	void getListenerPosition(int &x, int &y);
	int shiftListenerH(int delta);
	int shiftListenerV(int delta);
	int setFeedbackPosition(int x, int y);
	void getFeedbackPosition(int &x, int &y);
	int shiftFeedbackH(int delta);
	int shiftFeedbackV(int delta);
	int setPhysicalParameters(void *params);
	int setMaxPressure(float maxP);
	float getDS();
	float setDS(float ds);
	void setExcitationVolume(double v);
	void setSubGlotExType(subGlotEx_type subGlotExType);
	int getExcitationId();
	void setExcitationDirection(float angle);
	int setExcitationModel(int index);
	int setExcitationModel(string name);
	void setExcitationFreq(float freq);
	void setExcitationFixedFreq(int id);
	void setExcitationLowPassFreq(float freq);
	int getExcitationModelIndex();
	string getExcitationModelName();
	int getNumOfExcitationModels();
	void setA1(float a1);
	void setFilterToggle(int toggle);
	GLFWwindow *getWindow(int *size);
	void setCell(float type, int x, int y);
	void resetCell(int x, int y);
	void clearDomain();
	void resetSimulation();
	void setSlowMotion(bool slowMo);
	void setMousePos(int x, int y);
	int setQ(float q);
	void setGlottalLowPassToggle(int toggle);
	int saveFrame();
	int saveFrameSequence();
	int openFrame(string filename);
	int openFrameSequence(string filename, bool isBinary=true);
	int computeFrameSequence(string foldername);
	bool monitorSettings(bool fullScreen, int monIndex);
	void retrigger();

private:
	unsigned int simulationRate;
	unsigned int rateMultiplier;

	//------------------------------
	SimpleExcitation *excitation;
	VocalTractFDTD vocalTractFDTD;

	// domain control tests
	Oscillator osc;
	Oscillator oscKr;
	AudioFile audiofile;
	ADSR env;
	//------------------------------

	void initExcitation(int rate_mul, float normExLpF, bool useSinc=true);
};



inline int ArtVocSynth::fromWindowToDomain(int *coords) {
 return vocalTractFDTD.fromRawWindowToDomain(coords);
}
inline int ArtVocSynth::setListenerPosition(int x, int y) {
	return vocalTractFDTD.setListenerPosition(x, y);
}
inline void ArtVocSynth::getListenerPosition(int &x, int &y) {
	vocalTractFDTD.getListenerPosition(x, y);
}
inline int ArtVocSynth::shiftListenerH(int delta) {
	return vocalTractFDTD.shiftListenerH(delta);
}
inline int ArtVocSynth::shiftListenerV(int delta) {
	return vocalTractFDTD.shiftListenerV(delta);
}
inline int ArtVocSynth::setFeedbackPosition(int x, int y) {
	return vocalTractFDTD.setFeedbackPosition(x, y);
}
inline void ArtVocSynth::getFeedbackPosition(int &x, int &y) {
	vocalTractFDTD.getFeedbackPosition(x, y);
}
inline int ArtVocSynth::shiftFeedbackH(int delta) {
	return vocalTractFDTD.shiftFeedbackH(delta);
}
inline int ArtVocSynth::shiftFeedbackV(int delta) {
	return vocalTractFDTD.shiftFeedbackV(delta);
}
inline int ArtVocSynth::setPhysicalParameters(void *params) {
	return vocalTractFDTD.setPhysicalParameters(params);
}
inline int ArtVocSynth::setMaxPressure(float maxP) {
	return vocalTractFDTD.setMaxPressure(maxP);
}
inline float ArtVocSynth::getDS() {
	return vocalTractFDTD.getDS();
}
inline float ArtVocSynth::setDS(float ds) {
	return vocalTractFDTD.setDS(ds);
}
inline void ArtVocSynth::setExcitationVolume(double v) {
	excitation->setVolume(v);
}
inline void ArtVocSynth::setSubGlotExType(subGlotEx_type  subGlotExType) {
	excitation->setExcitationId(subGlotExType);
}
inline int ArtVocSynth::getExcitationId() {
	return excitation->getExcitationId();
}
inline void ArtVocSynth::setExcitationDirection(float angle) {
	vocalTractFDTD.setExcitationDirection(angle);
}
inline int ArtVocSynth::setExcitationModel(int index) {
	return vocalTractFDTD.setExcitationModel(index);
}
inline int ArtVocSynth::setExcitationModel(string name) {
	return vocalTractFDTD.setExcitationModel(name);
}
inline void ArtVocSynth::setExcitationFreq(float freq) {
	excitation->setFreq(freq);
}
inline void ArtVocSynth::setExcitationFixedFreq(int id) {
	excitation->setFixedFreq(id);
}
inline void ArtVocSynth::setExcitationLowPassFreq(float freq) {
	excitation->setLowpassFreq(freq);
}
inline int ArtVocSynth::getExcitationModelIndex() {
	return vocalTractFDTD.getExcitationModelIndex();
}
inline string ArtVocSynth::getExcitationModelName() {
	return vocalTractFDTD.getExcitationModelName();
}
inline int ArtVocSynth::getNumOfExcitationModels() {
	return vocalTractFDTD.getNumOfExcitationModels();
}
inline void ArtVocSynth::setA1(float a1) {
	vocalTractFDTD.setA1(a1);
}
inline void ArtVocSynth::setFilterToggle(int toggle) {
	vocalTractFDTD.setFilterToggle(toggle);
}
inline GLFWwindow *ArtVocSynth::getWindow(int *size) {
	return vocalTractFDTD.getWindow(size);
}
inline void ArtVocSynth::setCell(float type, int x, int y) {
	vocalTractFDTD.setCell(type, x, y);
}
inline void ArtVocSynth::resetCell(int x, int y) {
	vocalTractFDTD.resetCell(x, y);
}
inline void ArtVocSynth::clearDomain() {
	vocalTractFDTD.clearDomain();
}
inline void ArtVocSynth::resetSimulation() {
	vocalTractFDTD.reset();
}
inline void ArtVocSynth::setSlowMotion(bool slowMo) {
	vocalTractFDTD.setSlowMotion(slowMo);
}
inline void ArtVocSynth::setMousePos(int x, int y) {
	vocalTractFDTD.setMousePosition(x, y);
}
inline int ArtVocSynth::setQ(float q) {
	return vocalTractFDTD.setQ(q);
}
inline void ArtVocSynth::setGlottalLowPassToggle(int toggle) {
	vocalTractFDTD.setExcitationLowPassToggle(toggle);
}
inline int ArtVocSynth::saveFrame() {
	return vocalTractFDTD.saveFrame();
}
inline int ArtVocSynth::saveFrameSequence() {
	return vocalTractFDTD.saveFrameSequence();
}
inline int ArtVocSynth::openFrame(string filename) {
	return vocalTractFDTD.openFrame(filename);
}
inline int ArtVocSynth::openFrameSequence(string filename, bool isBinary) {
	return vocalTractFDTD.openFrameSequence(filename, isBinary);
}
inline int ArtVocSynth::computeFrameSequence(string foldername) {
	return vocalTractFDTD.computeFrameSequence(foldername);
}
inline bool ArtVocSynth::monitorSettings(bool fullScreen, int monIndex) {
	return vocalTractFDTD.monitorSettings(fullScreen, monIndex);
}

inline void ArtVocSynth::retrigger() {
}

#endif /* FDTDSYNTH_H_ */
