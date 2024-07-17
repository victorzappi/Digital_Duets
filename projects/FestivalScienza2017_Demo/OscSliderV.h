/*
 * OscSliderV.h
 *
 *  Created on: Sep 20, 2017
 *      Author: Victor Zappi
 */

#ifndef OSCSLIDERV_H_
#define OSCSLIDERV_H_

#include "OSCClient.h"

#include <cmath> // exp

using namespace std;

class OscSliderV {
	using fptr_synth = void(*)(float); // to pass lambda as method field
public:
	~OscSliderV();
	void init(int col, int btn, OSCClient *client, fptr_synth method, float min, float max, bool isExp=true);
	void init(int col, int btn, OSCClient *client, float *param, float min, float max, bool isExp=true);
	void setSlider(int level);

private:
	void lightButton(int b, int cmd);
	void *slideLoopFunction();
	void *slideLoopParam();
	float (OscSliderV::*sliderMapping)();
	float sliderMappingExp();
	float sliderMappingLin();
	static void *slideLoopLauncherFunction(void *context);
	static void *slideLoopLauncherParam(void *context);
	float expMapping(float expMin, float expMax, float expRange, float expNorm,
					 float inMin, float inMax, float inRange,
					 float in, float outMin, float outRange);

	int column;
	int button; // lit row
	int targetButton;
	int slideLag;
	int slideDirection;
	int buttonCmd;
	bool hasFunction;
	float minVal;
	float maxVal;
	float expMax;
	float expMin;
	float expRange;
	float expNorm;
	OSCClient *oscClient;
	pthread_t buttonThread;
	fptr_synth slideMethod;
	float *sliderParam;
};

inline OscSliderV::~OscSliderV() {
	pthread_cancel(buttonThread); // just in case
}

// wrapper
inline void OscSliderV::lightButton(int b, int cmd) {
	oscClient->sendMessageNow(oscClient->newMessage.to("/monome/grid/led/set").add(b).add(column).add(cmd).end());
}

// mapping methods
inline float OscSliderV::sliderMappingExp() {
	float retval = expMapping(expMin, expMax, expRange, expNorm, 0, 7, 7, button, minVal, maxVal);
	//printf("slider %d val %f\n", column, retval);
	return retval;
}
inline float OscSliderV::sliderMappingLin() {
	float retval = minVal + (button/7.0)*(maxVal-minVal);
	//printf("slider %d val %f\n", column, retval);
	return retval;
}


#endif /* OSCSLIDERV_H_ */
