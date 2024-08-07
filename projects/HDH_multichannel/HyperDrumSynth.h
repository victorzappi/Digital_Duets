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
//#include "DrumExcitation.h"
#include "HyperDrumExcitation.h"

#include "HyperDrumhead.h"

enum drumEx_type {drumEx_windowedImp, drumEx_const, drumEx_sin, drumEx_square,
				  drumEx_noise, drumEx_impTrain, drumEx_burst, drumEx_cello,
				  drumEx_drumloop, drumEx_fxloop, drumEx_drone, drumEx_pacific, drumEx_oscBank};


// this is a synth, with inside another synth that is filtered by the FDTD
class HyperDrumSynth: public AudioGeneratorInOut {
public:
	HyperDrumSynth();
	~HyperDrumSynth();
	void init(string shaderLocation, int *domainSize, int audioRate, int rateMul,
			  int periodSize, float exLevel, float magnifier, string exciteFolder=".", int *listCoord=NULL, 
			  unsigned short exInChannels=1, unsigned short exInChnOffset=0,  unsigned short hdhInChannels=1, unsigned short hdhOutChannels=1, unsigned short hdhOutChnOffset=0);
	int setDomain(int *excitationCoord, int *excitationDimensions, float ***domain, bool preset=false);

	// overloads of pure virtual methods
	double getSample();
	//double *getBuffer(int numOfSamples, double **input);
	double **getFrameBuffer(int numOfSamples, double **input);
	//float *getBufferFloat(int numOfSamples, float **input);
	//double *getBuffer(int numOfSamples);
	//float *getBufferFloat(int numOfSamples);
//	float *getBufferFloatReadTime(int numOfSamples, int &readTime);
//	float *getBufferFloatNoSampleRead(int numOfSamples);

	// interface for components
	int fromWindowToDomain(int *coords);
	int setListenerPosition(int x, int y);
	int setAreaListenerPosition(int index, int x, int y);
	int initAreaListenerPosition(int index, int x, int y);
	void getListenerPosition(int &x, int &y);
	void getAreaListenerPosition(int index, int &x, int &y);
	void hideListener();
	void hideAreaListener(int index, int chn=0);
	int shiftListenerH(int delta);
	int shiftListenerV(int delta);
	int shiftAreaListenerH(int index, int delta);
	int shiftAreaListenerV(int index, int delta);
	void setAreaExcitationVolume(int index, double v);
	void setAreaExcitationVolumeFixed(int index, double v);
	void setAreaExcitationID(int index, int id);
	int getAreaExcitationId(int index);
	bool getAreaExcitationIsPercussive(int index);
	bool getAreaExcitationIsPercussive(int index, int id);
	void setAreaExcitationFreq(int index, float freq);
	void setAreaExcitationFixedFreq(int index, int id);
	void setAreaExLowPassFreq(int index, float freq);
	void triggerAreaExcitation(int index);
	void dampAreaExcitation(int index);
	GLFWwindow *getWindow(int *size);
	int setCellType(int x, int y, float type, int channel=-1);
	float getCellType(int x, int y);
	void setFirstMovingExciteCoords(int index, int exX, int exY);
	void setNextMovingExciteCoords(int index, int exX, int exY);
	void resetMovingExcitation(int index, int exX, int exY);
	void setCellArea(int x, int y, float area);
	int setCell(int x, int y, float area, float type, float bgain=-1);
	int resetCellType(int x, int y);
	int getCellArea(int x, int y);
	void clearDomain();
	void resetSimulation();
	void setSlowMotion(bool slowMo);
	void setMousePos(int x, int y);
	int getAreaExcitationsNum(int index);
	void getAreaExcitationsNames(int index, string *names);
	void setAreaEraserPosition(int index, int x, int y);
	void hideAreaEraser(int index);
	int saveFrame(int areaNum, float *damp, float *prop, double *exVol, double *exFiltFreq, double *exFreq, int *exType, int listPos[][2],
			      int *frstMotion, bool *frstMotionNeg, int *frstPress, int *postMotion, bool *postMotionNeg, int *postPress,
				  float rangePress[][MAX_PRESS_MODES], float *deltaPress, int pressCnt, string filename="");
	int saveFrame(hyperDrumheadFrame *frame, string filename);
/*	int saveFrame();
	int saveFrameSequence();*/
	/*int openFrame(int areaNum, string filename,float *damp, float *prop, double *exVol, double *exFiltFreq,double *exFreq, int *exType,
			      int listPos[][2], int *frstMotion, bool *frstMotionNeg, int *frstPress, int *postMotion, bool *postMotionNeg, int *postPress,
				  float rangePress[][MAX_PRESS_MODES], float *deltaPress, int pressCnt);*/
	int openFrame(string filename, hyperDrumheadFrame *&frame, bool reload=false);
/*  int openFrameSequence(string filename, bool isBinary=true);
	int computeFrameSequence(string foldername);*/
	bool monitorSettings(bool fullScreen, int monIndex);
	void setAreaDampingFactor(int index, float dampFact);
	float getAreaDampingFactor(int index);
	void setAreaPropagationFactor(int index, float propFact);
	float getAreaPropagationFactor(int index);
	void disableRender();
	void setShowAreas(bool show);
	void setSelectedArea(int area);
	void setAreaExcitationRelease(int index, double t);
	void setUpAreaListenerCanDrag(int index, int drag);
	int getExcitationsChannels();

	void retrigger();
	void retriggerStreams();


private:
	unsigned int simulationRate;
	unsigned int rateMultiplier;

	int domainSize[2];

	//------------------------------
	HyperDrumExcitation **channelExcitation;
	HyperDrumhead hyperDrumhead;
	//------------------------------

	double **channelExciteBuff;

	int numOfAreas;	// for area excitation

	void initExcitation(int rate_mul, float excitationLevel, string exciteFolder, unsigned short inChannels, unsigned short inChnOffset);
};



inline int HyperDrumSynth::fromWindowToDomain(int *coords) {
 return hyperDrumhead.fromRawWindowToDomain(coords);
}
inline int HyperDrumSynth::setListenerPosition(int x, int y) {
	return hyperDrumhead.setListenerPosition(x, y);
}
inline int HyperDrumSynth::setAreaListenerPosition(int index, int x, int y) {
	return hyperDrumhead.setAreaListenerPosition(index, 1, x, y); //VIC BE CAREFUL, here channel 1 is hard wired for now 
}
inline int HyperDrumSynth::initAreaListenerPosition(int index, int x, int y) {
	return hyperDrumhead.initAreaListenerPosition(index, 0, x, y);
}
inline void HyperDrumSynth::getListenerPosition(int &x, int &y) {
	hyperDrumhead.getListenerPosition(x, y);
}
inline void HyperDrumSynth::getAreaListenerPosition(int index, int &x, int &y) {
	hyperDrumhead.getAreaListenerPosition(index, 0, x, y);
}
inline void HyperDrumSynth::hideListener() {
	hyperDrumhead.hideListener();
}
inline void HyperDrumSynth::hideAreaListener(int index, int chn) {
	hyperDrumhead.hideAreaListener(index, chn);
}
inline int HyperDrumSynth::shiftListenerH(int delta) {
	return hyperDrumhead.shiftListenerH(delta);
}
inline int HyperDrumSynth::shiftListenerV(int delta) {
	return hyperDrumhead.shiftListenerV(delta);
}
inline int HyperDrumSynth::shiftAreaListenerH(int index, int delta) {
	return hyperDrumhead.shiftAreaListenerH(index, 0, delta);
}
inline int HyperDrumSynth::shiftAreaListenerV(int index, int delta) {
	return hyperDrumhead.shiftAreaListenerV(index, 0, delta);
}
inline void HyperDrumSynth::setAreaExcitationVolume(int index, double v) {
	channelExcitation[index]->setVolume(v);
}
inline void HyperDrumSynth::setAreaExcitationVolumeFixed(int index, double v) {
	channelExcitation[index]->setVolumeFixed(v);
}
inline void HyperDrumSynth::setAreaExcitationID(int index, int id) {
	channelExcitation[index]->setExcitationID(id);
}
inline bool HyperDrumSynth::getAreaExcitationIsPercussive(int index) {
	return channelExcitation[index]->getIsPercussive();
}
inline bool HyperDrumSynth::getAreaExcitationIsPercussive(int index, int id) {
	return channelExcitation[index]->getIsPercussive(id);
}
inline int HyperDrumSynth::getAreaExcitationId(int index) {
	return channelExcitation[index]->getExcitationId();
}
inline void HyperDrumSynth::setAreaExLowPassFreq(int index, float freq) {
	channelExcitation[index]->setLowpassFreq(freq);
}
inline void HyperDrumSynth::triggerAreaExcitation(int index) {
	channelExcitation[index]->excite();
}
inline void HyperDrumSynth::dampAreaExcitation(int index) {
	channelExcitation[index]->damp();
}
inline GLFWwindow *HyperDrumSynth::getWindow(int *size) {
	return hyperDrumhead.getWindow(size);
}
inline int HyperDrumSynth::setCellType(int x, int y, float type, int channel) {
	return hyperDrumhead.setCellType(x, y, type, channel);
}
inline float HyperDrumSynth::getCellType(int x, int y) {
	return hyperDrumhead.getCellType(x, y);
}
inline void HyperDrumSynth::setFirstMovingExciteCoords(int index, int exX, int exY) {
	hyperDrumhead.setFirstMovingExciteCoords(index, exX, exY);
}
inline void HyperDrumSynth::setNextMovingExciteCoords(int index, int exX, int exY) {
	hyperDrumhead.setNextMovingExciteCoords(index, exX, exY);
}
inline void HyperDrumSynth::resetMovingExcitation(int index, int exX, int exY) {
	hyperDrumhead.resetMovingExcitation(index, exX, exY);
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
	hyperDrumhead.setSlowMotion(slowMo);
}
inline void HyperDrumSynth::setMousePos(int x, int y) {
	hyperDrumhead.setMousePosition(x, y);
}
inline int HyperDrumSynth::getAreaExcitationsNum(int index) {
	return channelExcitation[index]->getComponentsNum();
}
inline void HyperDrumSynth::getAreaExcitationsNames(int index, string *names) {
	channelExcitation[index]->getComponentsNames(names);
}
inline void HyperDrumSynth::setAreaEraserPosition(int index, int x, int y) {
	hyperDrumhead.setAreaEraserPosition(index, x, y);
}
inline void HyperDrumSynth::hideAreaEraser(int index) {
	hyperDrumhead.hideAreaEraser(index);
}
inline int HyperDrumSynth::saveFrame(hyperDrumheadFrame *frame, string filename) {
	return hyperDrumhead.saveFrame(frame, filename);
}
inline bool HyperDrumSynth::monitorSettings(bool fullScreen, int monIndex) {
	return hyperDrumhead.monitorSettings(fullScreen, monIndex);
}

inline void HyperDrumSynth::setAreaDampingFactor(int index, float dampFact) {
	hyperDrumhead.setAreaDampingFactor(index, dampFact);
}
inline float HyperDrumSynth::getAreaDampingFactor(int index) {
	return hyperDrumhead.getAreaDampingFactor(index);
}
inline void HyperDrumSynth::setAreaPropagationFactor(int index, float propFact) {
	hyperDrumhead.setAreaPropagationFactor(index, propFact);
}
inline float HyperDrumSynth::getAreaPropagationFactor(int index) {
	return hyperDrumhead.getAreaPropagationFactor(index);
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
	channelExcitation[index]->setRelease(t);
}
inline void HyperDrumSynth::setUpAreaListenerCanDrag(int index, int drag) {
	hyperDrumhead.setAreaListenerCanDrag(index, 0, drag);
}
inline void HyperDrumSynth::retrigger() {
	for(int i=0; i<numOfAreas; i++)
		channelExcitation[i]->retrigger();
}
inline void HyperDrumSynth::retriggerStreams() {
	for(int i=0; i<numOfAreas; i++)
		channelExcitation[i]->retriggerStreams();
}
inline int HyperDrumSynth::getExcitationsChannels() {
	return in_channels; // this is HDH input channels, a.k.a., number of exictations that go into HDH, which can be called excitation channels
}


#endif /* HyperDrumSynth_H_ */
