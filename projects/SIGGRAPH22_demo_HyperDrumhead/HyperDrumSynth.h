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
			  int periodSize, float exLevel, float magnifier, string exciteFolder=".", int *listCoord=NULL);
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
	void hideAreaListener(int index);
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
	int setCellType(int x, int y, float type);
	float getCellType(int x, int y);
	int setCellCrossExcite(int rstX, int rstY, int exX, int exY, float area/*, float type*/);
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
	void setAreaMicCanDrag(int index, int drag);

	void retrigger();
	void retriggerStreams();


private:
	unsigned int simulationRate;
	unsigned int rateMultiplier;

	int domainSize[2];

	//------------------------------
	HyperDrumExcitation **areaExcitation;
	HyperDrumhead hyperDrumhead;
	//------------------------------

	double **areaExciteBuff;

	int numOfAreas;	// for area excitation

	void initExcitation(int rate_mul, float excitationLevel, string exciteFolder);
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
inline int HyperDrumSynth::initAreaListenerPosition(int index, int x, int y) {
	return hyperDrumhead.initAreaListenerPosition(index, x, y);
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
inline void HyperDrumSynth::setAreaExcitationVolume(int index, double v) {
	areaExcitation[index]->setVolume(v);
}
inline void HyperDrumSynth::setAreaExcitationVolumeFixed(int index, double v) {
	areaExcitation[index]->setVolumeFixed(v);
}
inline void HyperDrumSynth::setAreaExcitationID(int index, int id) {
	areaExcitation[index]->setExcitationID(id);
}
inline bool HyperDrumSynth::getAreaExcitationIsPercussive(int index) {
	return areaExcitation[index]->getIsPercussive();
}
inline bool HyperDrumSynth::getAreaExcitationIsPercussive(int index, int id) {
	return areaExcitation[index]->getIsPercussive(id);
}
inline int HyperDrumSynth::getAreaExcitationId(int index) {
	return areaExcitation[index]->getExcitationId();
}
inline void HyperDrumSynth::setAreaExLowPassFreq(int index, float freq) {
	areaExcitation[index]->setLowpassFreq(freq);
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
inline int HyperDrumSynth::setCellCrossExcite(int rstX, int rstY, int exX, int exY, float area/*, float type*/) {
	return hyperDrumhead.setCellCrossExcite(rstX, rstY, exX, exY, area/*, type*/);
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
	return areaExcitation[index]->getComponentsNum();
}
inline void HyperDrumSynth::getAreaExcitationsNames(int index, string *names) {
	areaExcitation[index]->getComponentsNames(names);
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
	areaExcitation[index]->setRelease(t);
}
inline void HyperDrumSynth::setAreaMicCanDrag(int index, int drag) {
	hyperDrumhead.setAreaMicCanDrag(index, drag);
}

inline void HyperDrumSynth::retrigger() {
	for(int i=0; i<numOfAreas; i++)
		areaExcitation[i]->retrigger();
}

inline void HyperDrumSynth::retriggerStreams() {
	for(int i=0; i<numOfAreas; i++)
		areaExcitation[i]->retriggerStreams();
}

#endif /* HyperDrumSynth_H_ */
