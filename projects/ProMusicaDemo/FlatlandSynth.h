/*
 * FlatlandSynth.h
 *
 *  Created on: 2015-11-18
 *      Author: vic
 */

#ifndef FDTDSYNTH_H_
#define FDTDSYNTH_H_

#include "AudioGenerator.h"
#include "ImplicitFDTD.h"
#include "FlatlandExcitation.h"

#include "FlatlandFDTD.h"

#define TRACKS_NUM 7

enum ex_type {ex_const, ex_sin, ex_square, ex_noise, ex_passthru};


// this is a synth, with inside another synth that is filtered by the FDTD
class FlatlandSynth: public AudioGenerator {
public:
	FlatlandSynth();
	FlatlandSynth(int *domainSize, int numOfStates, int *flatlandExcitationCoord, int *flatlandExcitationDimensions,
			    float ***domain, int audio_rate, int rate_mul, int periodSize);
	void init(string shaderLocation, int *domainSize, int *flatlandExcitationCoord, int *flatlandExcitationDimensions,
			  float ***domain, int audioRate, int rateMul, int periodSize, float magnifier=1, int *listCoord=NULL, float exLpF=22050, bool useSinc=true);
	void init(string shaderLocation, int *domainSize, int audioRate, int rateMul, int periodSize,
			  float magnifier, int *listCoord=NULL, float exLpF=22050, bool useSinc=true);
	int setDomain(int *flatlandExcitationCoord, int *flatlandExcitationDimensions, float ***domain);
	int loadContourFile(int domain_size[2], float deltaS, string filename, float ***pixels);

	// overloads of pure virtual methods
	double getSample();
	double *getBuffer(int numOfSamples);
	float *getBufferFloat(int numOfSamples);

	// interface for components
	int fromRawWindowToDomain(int *coords);
	int fromRawWindowToGlWindow(float *coords);
	int setListenerPosition(int x, int y);
	void getListenerPosition(int &x, int &y);
	int shiftListenerH(int delta);
	int shiftListenerV(int delta);
	int setFeedbackPosition(int x, int y);
	void getFeedbackPosition(int &x, int &y);
	int shiftFeedbackH(int delta);
	int shiftFeedbackV(int delta);
	int setPhysicalParameters(float c, float rho, float mu, float prandtl, float gamma);
	int setMaxPressure(float maxP);
	float getDS();
	float setDS(float ds);
	void setExcitationVolume(double v);
	void setExcitationDirection(float angle);
	int setExcitationModel(int index);
	int setExcitationModel(string name);
	void setExcitationFreq(float freq);
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
	int openFrame(string filename);
	int openFrame(frame *f);
	int computeFrame(string filename, frame &f);
	int saveFrameSequence();
	int openFrameSequence(string filename, bool isBinary=true);
	int computeFrameSequence(string foldername);
	int loadFrame(frame *f);
	bool monitorSettings(bool fullScreen, int monIndex);
	int setHideUI(bool hide);
	void setAreaButtons(float left[UI_AREA_BUTTONS_NUM], float bottom[UI_AREA_BUTTONS_NUM],
				               float w[UI_AREA_BUTTONS_NUM], float h[UI_AREA_BUTTONS_NUM]);
	int checkAreaButtonsTriggered(float gl_win_x, float gl_win_y, bool &newtrigger);
	void untriggerAreaButtons();
	void setAreaSliders(float left[UI_AREA_SLIDERS_NUM], float bottom[UI_AREA_SLIDERS_NUM], float w[UI_AREA_SLIDERS_NUM],
			            float h[UI_AREA_SLIDERS_NUM], float initVal[UI_AREA_SLIDERS_NUM], bool sticky[UI_AREA_SLIDERS_NUM]);
	int checkAreaSlidersValues(float gl_win_x, float gl_win_y, float &newvalue);
	bool *resetAreaSliders(bool stickyOnly);
	void resetAreaSlider(int index);
	float getAreaSliderVal(int index);
	bool toggleTrack(int index, bool on);
	bool getTrackStatus(int index);
	void setTrackVol(int index, double vol);
	bool setPassThruTrack(int index);
	int getPassThruTrack();
	void setExcitationVol(int index, double vol);
	void retrigger();

private:
	unsigned int simulationRate;
	unsigned int rateMultiplier;

	//------------------------------
	FlatlandExcitation *flatlandExcitation;
	FlatlandFDTD flatlandFDTD;

	AudioFile tracks[TRACKS_NUM];
	//------------------------------

	bool tracksOn[TRACKS_NUM];
	int passThruTrack;

	double trckVol[TRACKS_NUM]; // parameter
	double trckVolInterp[TRACKS_NUM][2]; // reference and increment


	void initExcitation(int rate_mul, float normExLpF, bool useSinc=true);
	void initTracks();
};

inline void FlatlandSynth::setAreaButtons(float left[UI_AREA_BUTTONS_NUM], float bottom[UI_AREA_BUTTONS_NUM],
        float w[UI_AREA_BUTTONS_NUM], float h[UI_AREA_BUTTONS_NUM]) {
	flatlandFDTD.setAreaButtons(left, bottom, w, h);
}

inline int FlatlandSynth::checkAreaButtonsTriggered(float gl_win_x, float gl_win_y, bool &newtrigger) {
	return flatlandFDTD.checkAreaButtonsTriggered(gl_win_x, gl_win_y, newtrigger);
}
inline void FlatlandSynth::untriggerAreaButtons() {
	flatlandFDTD.untriggerAreaButtons();
}
inline bool *FlatlandSynth::resetAreaSliders(bool stickyOnly) {
	return flatlandFDTD.resetAreaSliders(stickyOnly);
}
inline void FlatlandSynth::resetAreaSlider(int index) {
	return flatlandFDTD.resetAreaSlider(index);
}
inline float FlatlandSynth::getAreaSliderVal(int index) {
	return flatlandFDTD.getAreaSliderVal(index);
}
inline void FlatlandSynth::setAreaSliders(float left[UI_AREA_SLIDERS_NUM], float bottom[UI_AREA_SLIDERS_NUM], float w[UI_AREA_SLIDERS_NUM],
		                                  float h[UI_AREA_SLIDERS_NUM], float initVal[UI_AREA_SLIDERS_NUM], bool sticky[UI_AREA_SLIDERS_NUM]) {
	flatlandFDTD.setAreaSliders(left, bottom, w, h, initVal, sticky);
}
inline int FlatlandSynth::checkAreaSlidersValues(float gl_win_x, float gl_win_y, float &newvalue) {
	return flatlandFDTD.checkAreaSlidersValues(gl_win_x, gl_win_y, newvalue);
}
inline int FlatlandSynth::fromRawWindowToDomain(int *coords) {
	return flatlandFDTD.fromRawWindowToDomain(coords);
}
inline int FlatlandSynth::fromRawWindowToGlWindow(float *coords) {
	return flatlandFDTD.fromRawWindowToGlWindow(coords);
}
inline int FlatlandSynth::setListenerPosition(int x, int y) {
	return flatlandFDTD.setListenerPosition(x, y);
}
inline void FlatlandSynth::getListenerPosition(int &x, int &y) {
	flatlandFDTD.getListenerPosition(x, y);
}
inline int FlatlandSynth::shiftListenerH(int delta) {
	return flatlandFDTD.shiftListenerH(delta);
}
inline int FlatlandSynth::shiftListenerV(int delta) {
	return flatlandFDTD.shiftListenerV(delta);
}
inline int FlatlandSynth::setFeedbackPosition(int x, int y) {
	return flatlandFDTD.setFeedbackPosition(x, y);
}
inline void FlatlandSynth::getFeedbackPosition(int &x, int &y) {
	flatlandFDTD.getFeedbackPosition(x, y);
}
inline int FlatlandSynth::shiftFeedbackH(int delta) {
	return flatlandFDTD.shiftFeedbackH(delta);
}
inline int FlatlandSynth::shiftFeedbackV(int delta) {
	return flatlandFDTD.shiftFeedbackV(delta);
}
inline int FlatlandSynth::setPhysicalParameters(float c, float rho, float mu, float prandtl, float gamma) {
	return flatlandFDTD.setPhysicalParameters(c, rho, mu, prandtl, gamma);
}
inline int FlatlandSynth::setMaxPressure(float maxP) {
	return flatlandFDTD.setMaxPressure(maxP);
}
inline float FlatlandSynth::getDS() {
	return flatlandFDTD.getDS();
}
inline float FlatlandSynth::setDS(float ds) {
	return flatlandFDTD.setDS(ds);
}
inline void FlatlandSynth::setExcitationVolume(double v) {
	flatlandExcitation->setVolume(v);
}
inline void FlatlandSynth::setExcitationDirection(float angle) {
	flatlandFDTD.setExcitationDirection(angle);
}
inline int FlatlandSynth::setExcitationModel(int index) {
	return flatlandFDTD.setExcitationModel(index);
}
inline int FlatlandSynth::setExcitationModel(string name) {
	return flatlandFDTD.setExcitationModel(name);
}
inline void FlatlandSynth::setExcitationFreq(float freq) {
	flatlandExcitation->setFreq(freq);
}
inline int FlatlandSynth::getExcitationModelIndex() {
	return flatlandFDTD.getExcitationModelIndex();
}
inline string FlatlandSynth::getExcitationModelName() {
	return flatlandFDTD.getExcitationModelName();
}
inline int FlatlandSynth::getNumOfExcitationModels() {
	return flatlandFDTD.getNumOfExcitationModels();
}
inline void FlatlandSynth::setA1(float a1) {
	flatlandFDTD.setA1(a1);
}
inline void FlatlandSynth::setFilterToggle(int toggle) {
	flatlandFDTD.setFilterToggle(toggle);
}
inline GLFWwindow *FlatlandSynth::getWindow(int *size) {
	return flatlandFDTD.getWindow(size);
}
inline void FlatlandSynth::setCell(float type, int x, int y) {
	flatlandFDTD.setCell(type, x, y);
}
inline void FlatlandSynth::resetCell(int x, int y) {
	flatlandFDTD.resetCell(x, y);
}
inline void FlatlandSynth::clearDomain() {
	flatlandFDTD.clearDomain();
}
inline void FlatlandSynth::resetSimulation() {
	flatlandFDTD.reset();
}
inline void FlatlandSynth::setSlowMotion(bool slowMo) {
	flatlandFDTD.setSlowMotion(slowMo);
}
inline void FlatlandSynth::setMousePos(int x, int y) {
	flatlandFDTD.setMousePosition(x, y);
}
inline int FlatlandSynth::setQ(float q) {
	return flatlandFDTD.setQ(q);
}
inline void FlatlandSynth::setGlottalLowPassToggle(int toggle) {
	flatlandFDTD.setExcitationLowPassToggle(toggle);
}
inline int FlatlandSynth::saveFrame() {
	return flatlandFDTD.saveFrame();
}
inline int FlatlandSynth::saveFrameSequence() {
	return flatlandFDTD.saveFrameSequence();
}
inline int FlatlandSynth::openFrame(string filename) {
	return flatlandFDTD.openFrame(filename);
}
inline int FlatlandSynth::openFrame(frame *f) {
	return flatlandFDTD.openFrame(f);
}
inline int FlatlandSynth::computeFrame(string filename, frame &f) {
	return flatlandFDTD.computeFrame(filename, f);
}
inline int FlatlandSynth::openFrameSequence(string filename, bool isBinary) {
	return flatlandFDTD.openFrameSequence(filename, isBinary);
}
inline int FlatlandSynth::computeFrameSequence(string foldername) {
	return flatlandFDTD.computeFrameSequence(foldername);
}
inline int FlatlandSynth::loadFrame(frame *f) {
	return flatlandFDTD.loadFrame(f);
}
inline bool FlatlandSynth::monitorSettings(bool fullScreen, int monIndex) {
	return flatlandFDTD.monitorSettings(fullScreen, monIndex);
}
inline int FlatlandSynth::setHideUI(bool hide) {
	return flatlandFDTD.setHideUI(hide);
}
inline bool FlatlandSynth::getTrackStatus(int index) {
	return tracksOn[index];
}
inline void FlatlandSynth::setTrackVol(int index, double vol) {
	prepareInterpolateParam(trckVol[index], trckVolInterp[index], vol);
}
inline bool FlatlandSynth::setPassThruTrack(int index) {
	if(index<0 || index>=TRACKS_NUM) {
		printf("This FlatlandSynth has only %d tracks for pass thru [0 - %d], and you requested number %d...\n I am ignoring it, for your sake\n",
				TRACKS_NUM, TRACKS_NUM-1, index);
		return false;
	}

	passThruTrack = index;
	return true;
}
inline int FlatlandSynth::getPassThruTrack() {
	return passThruTrack;
}
inline void FlatlandSynth::setExcitationVol(int index, double vol) {
	flatlandExcitation->setExcitationVol(index, vol);
}
inline void FlatlandSynth::retrigger() {
}
#endif /* FDTDSYNTH_H_ */
