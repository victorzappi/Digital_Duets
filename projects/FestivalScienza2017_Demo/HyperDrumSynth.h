/*
 * HyperDrumSynth.h
 *
 *  Created on: 2016-11-18
 *      Author: Victor Zappi
 */

#ifndef HyperDrumSynth_H_
#define HyperDrumSynth_H_

#include "AudioGenerator.h"
#include "ImplicitFDTD.h"
#include "DrumExcitation.h"

#include "HyperDrumhead.h"

enum drumEx_type {drumEx_windowedImp, drumEx_const, drumEx_sin, drumEx_square,
				  drumEx_noise, drumEx_burst, drumEx_cello,
				  drumEx_drumloop, drumEx_fxloop, drumEx_drone, drumEx_pacific, drumEx_disco, drumEx_jeeg, drumEx_oscBank};


// this is a synth, with inside another synth that is filtered by the FDTD
class HyperDrumSynth: public AudioGenerator {
public:
	HyperDrumSynth();
	~HyperDrumSynth();
	void init(string shaderLocation, int *domainSize, int audioRate, int rateMul,
			  int periodSize, float exLevel, float magnifier, int *listCoord=NULL);
	int setDomain(int *excitationCoord, int *excitationDimensions, float ***domain, bool preset=false);

	// overloads of pure virtual methods
	double getSample();
	double *getBuffer(int numOfSamples);
	float *getBufferFloat(int numOfSamples);
	float *getBufferFloatReadTime(int numOfSamples, int &readTime);
	float *getBufferFloatNoSampleRead(int numOfSamples);

	// interface for components
	int fromWindowToDomain(int *coords);
	int setListenerPosition(int x, int y);
	int setAreaListenerPosition(int index, int x, int y);
	void getListenerPosition(int &x, int &y);
	void getAreaListenerPosition(int index, int &x, int &y);
	void hideListener();
	void hideAreaListener(int index);
	int shiftListenerH(int delta);
	int shiftListenerV(int delta);
	int shiftAreaListenerH(int index, int delta);
	int shiftAreaListenerV(int index, int delta);
	void setExcitationVolume(double v);
	void setAreaExcitationVolume(int index, double v);
	void setAreaExcitationVolumeFixed(int index, double v);
	void setExcitationType(drumEx_type exciteType);
	void setAreaExcitationType(int index, drumEx_type exciteType);
	int getExcitationId();
	void setExcitationFreq(float freq);
	void setAreaExcitationFreq(int index, float freq);
	void setExcitationFixedFreq(int id);
	void setAreaExcitationFixedFreq(int index, int id);
	void setExcitationLowPassFreq(float freq);
	void setAreaExLowPassFreq(int index, float freq);
	void triggerExcitation();
	void triggerAreaExcitation(int index);
	void dampAreaExcitation(int index);
	GLFWwindow *getWindow(int *size);
	int setCellType(int x, int y, float type);
	float getCellType(int x, int y);
	void setCellArea(int x, int y, float area);
	int setCell(int x, int y, float area, float type, float bgain=-1);
	int resetCellType(int x, int y);
	int getCellArea(int x, int y);
	void clearDomain();
	void resetSimulation();
	void setSlowMotion(bool slowMo);
	void setMousePos(int x, int y);
	int saveFrame(int areaNum, float *damp, float *prop, double *exVol, double *exFiltFreq, double *exFreq, drumEx_type *exType, int listPos[][2],
			      int *frstMotion, bool *frstMotionNeg, int *frstPress, int *thrdMotion, bool *thrdMotionNeg, int *thrdPress,
				  float rangePress[][MAX_PRESS_MODES], float *deltaPress, int pressCnt, string filename="");
/*	int saveFrame();
	int saveFrameSequence();*/
	int openFrame(int areaNum, string filename,float *damp, float *prop, double *exVol, double *exFiltFreq,double *exFreq, drumEx_type *exType,
                  int *frstMotion, bool *frstMotionNeg, int *frstPress, int *thrdMotion, bool *thrdMotionNeg, int *thrdPress,
				  float rangePress[][MAX_PRESS_MODES], float *deltaPress, int pressCnt);
/*  int openFrameSequence(string filename, bool isBinary=true);
	int computeFrameSequence(string foldername);*/
	bool monitorSettings(bool fullScreen, int monIndex);
	void setAreaDampingFactor(int index, float dampFact);
	void setAreaPropagationFactor(int index, float propFact);
	void disableRender();
	void setShowAreas(bool show);
	void setSelectedArea(int area);
	void setAreaExcitationRelease(int index, double t);
	void retrigger();

private:
	unsigned int simulationRate;
	unsigned int rateMultiplier;

	int domainSize[2];

	//------------------------------
	DrumExcitation **areaExcitation;
	DrumExcitation *excitation;
	HyperDrumhead hyperDrumhead;
	//------------------------------

	float **areaExciteBuff;

	int numOfAreas;	// this still here only so that we do not have to change HyperPixelDrawer [always 1]
	int numOfInputs; // for area excitation

	// slow motion
	bool slowMotion;

	void initExcitation(int rate_mul, float excitationLevel);
};



inline int HyperDrumSynth::fromWindowToDomain(int *coords) {
 return hyperDrumhead.fromRawWindowToDomain(coords);
}
inline int HyperDrumSynth::setListenerPosition(int x, int y) {
	return hyperDrumhead.setListenerPosition(x, y);
}
inline int HyperDrumSynth::setAreaListenerPosition(int index, int x, int y) {
	return hyperDrumhead.setAreaListenerPosition(index, x, y);
}
inline void HyperDrumSynth::getListenerPosition(int &x, int &y) {
	hyperDrumhead.getListenerPosition(x, y);
}
inline void HyperDrumSynth::getAreaListenerPosition(int index, int &x, int &y) {
	hyperDrumhead.getAreaListenerPosition(index, x, y);
}
inline void HyperDrumSynth::hideListener() {
	hyperDrumhead.hideListener();
}
inline void HyperDrumSynth::hideAreaListener(int index) {
	hyperDrumhead.hideAreaListener(index);
}
inline int HyperDrumSynth::shiftListenerH(int delta) {
	return hyperDrumhead.shiftListenerH(delta);
}
inline int HyperDrumSynth::shiftListenerV(int delta) {
	return hyperDrumhead.shiftListenerV(delta);
}
inline int HyperDrumSynth::shiftAreaListenerH(int index, int delta) {
	return hyperDrumhead.shiftAreaListenerH(index, delta);
}
inline int HyperDrumSynth::shiftAreaListenerV(int index, int delta) {
	return hyperDrumhead.shiftAreaListenerV(index, delta);
}
inline void HyperDrumSynth::setExcitationVolume(double v) {
	excitation->setVolume(v);
}
inline void HyperDrumSynth::setAreaExcitationVolume(int index, double v) {
	areaExcitation[index]->setVolume(v);
}
inline void HyperDrumSynth::setAreaExcitationVolumeFixed(int index, double v) {
	areaExcitation[index]->setVolumeFixed(v);
}
inline void HyperDrumSynth::setExcitationType(drumEx_type  exciteType) {
	excitation->setExcitationId(exciteType);
}
inline void HyperDrumSynth::setAreaExcitationType(int index, drumEx_type  exciteType) {
	areaExcitation[index]->setExcitationId(exciteType);
}
inline int HyperDrumSynth::getExcitationId() {
	return excitation->getExcitationId();
}
inline void HyperDrumSynth::setExcitationFreq(float freq) {
	excitation->setFreq(freq);
}
inline void HyperDrumSynth::setAreaExcitationFreq(int index, float freq) {
	areaExcitation[index]->setFreq(freq);
}
inline void HyperDrumSynth::setExcitationFixedFreq(int id) {
	excitation->setFixedFreq(id);
}
inline void HyperDrumSynth::setAreaExcitationFixedFreq(int index, int id) {
	areaExcitation[index]->setFixedFreq(id);
}
inline void HyperDrumSynth::setExcitationLowPassFreq(float freq) {
	excitation->setLowpassFreq(freq);
}
inline void HyperDrumSynth::setAreaExLowPassFreq(int index, float freq) {
	areaExcitation[index]->setLowpassFreq(freq);
}
inline void HyperDrumSynth::triggerExcitation() {
	excitation->excite();
}
inline void HyperDrumSynth::triggerAreaExcitation(int index) {
	areaExcitation[index]->excite();
}
inline void HyperDrumSynth::dampAreaExcitation(int index) {
	areaExcitation[index]->damp();
}
inline GLFWwindow *HyperDrumSynth::getWindow(int *size) {
	return hyperDrumhead.getWindow(size);
}
inline int HyperDrumSynth::setCellType(int x, int y, float type) {
	return hyperDrumhead.setCellType(x, y, type);
}
inline float HyperDrumSynth::getCellType(int x, int y) {
	return hyperDrumhead.getCellType(x, y);
}
inline void HyperDrumSynth::setCellArea(int x, int y, float area) {
	hyperDrumhead.setCellArea(x, y, area);
}
inline int HyperDrumSynth::setCell(int x, int y, float area, float type, float bgain) {
	return hyperDrumhead.setCell(x, y, area, type, bgain);
}
inline int HyperDrumSynth::resetCellType(int x, int y) {
	return hyperDrumhead.resetCellType(x, y);
}
inline int HyperDrumSynth::getCellArea(int x, int y) {
	return hyperDrumhead.getCellArea(x, y);
}
inline void HyperDrumSynth::clearDomain() {
	hyperDrumhead.clearDomain();
	hyperDrumhead.reset();
}
inline void HyperDrumSynth::resetSimulation() {
	hyperDrumhead.reset();
}
inline void HyperDrumSynth::setSlowMotion(bool slowMo) {
	slowMotion = slowMo;
	hyperDrumhead.setSlowMotion(slowMo);
}
inline void HyperDrumSynth::setMousePos(int x, int y) {
	hyperDrumhead.setMousePosition(x, y);
}
/*
inline int HyperDrumSynth::saveFrame() {
	return hyperDrumhead.saveFrame();
}
inline int HyperDrumSynth::saveFrameSequence() {
	return hyperDrumhead.saveFrameSequence();
}
inline int HyperDrumSynth::openFrameSequence(string filename, bool isBinary) {
	return hyperDrumhead.openFrameSequence(filename, isBinary);
}
inline int HyperDrumSynth::computeFrameSequence(string foldername) {
	return hyperDrumhead.computeFrameSequence(foldername);
}*/
inline bool HyperDrumSynth::monitorSettings(bool fullScreen, int monIndex) {
	return hyperDrumhead.monitorSettings(fullScreen, monIndex);
}

inline void HyperDrumSynth::setAreaDampingFactor(int index, float dampFact) {
	hyperDrumhead.setAreaDampingFactor(index, dampFact);
}
inline void HyperDrumSynth::setAreaPropagationFactor(int index, float propFact) {
	hyperDrumhead.setAreaPropagationFactor(index, propFact);
}
inline void HyperDrumSynth::disableRender() {
	hyperDrumhead.disableRender();
}
inline void HyperDrumSynth::setShowAreas(bool show) {
	hyperDrumhead.setShowAreas(show);
}
inline void HyperDrumSynth::setSelectedArea(int area) {
	hyperDrumhead.setSelectedArea(area);
}
inline void HyperDrumSynth::setAreaExcitationRelease(int index, double t) {
	areaExcitation[index]->setRelease(t);
}
inline void HyperDrumSynth::retrigger() {
	triggerExcitation();
}

#endif /* HyperDrumSynth_H_ */
