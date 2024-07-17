/*
 * OscSliderV.cpp
 *
 *  Created on: Sep 20, 2017
 *      Author: Victor Zappi
 */

#include "OscSliderV.h"

#include "priority_utils.h"



void OscSliderV::init(int col, int btn, OSCClient *client, fptr_synth method, float min, float max, bool isExp) {
	column       = col;
	button       = btn;
	targetButton = btn;
	oscClient    = client;
	minVal 		 = min;
	maxVal 	     = max;
	slideMethod  = method;
	sliderParam  = NULL;

	// two different kinds of mapping, via function pointer
	if(isExp)
		sliderMapping = &OscSliderV::sliderMappingExp;
	else
		sliderMapping = &OscSliderV::sliderMappingLin;

	hasFunction = true;

	// hardcoded
	expMax   = 2;
	expMin   = -8;
	expRange = expMax-expMin;
	expNorm  = exp(expMax);
	slideDirection = 0;
	buttonCmd = -1;

	// init leds, according to settings
	for(int i=0; i<=button; i++)
		lightButton(i, 1);
	slideMethod((*this.*sliderMapping)());
}

void OscSliderV::init(int col, int btn, OSCClient *client, float *param, float min, float max, bool isExp) {
	column = col;
	button = btn;
	targetButton = btn;
	oscClient = client;
	minVal = min;
	maxVal = max;
	sliderParam = param;
	slideMethod = NULL;

	// two different kinds of mapping, via function pointer
	if(isExp)
		sliderMapping = &OscSliderV::sliderMappingExp;
	else
		sliderMapping = &OscSliderV::sliderMappingLin;

	hasFunction = false;

	// hardcoded
	expMax = 2;
	expMin = -8;
	expRange = expMax-expMin;
	expNorm = exp(expMax);
	slideDirection = 0;
	buttonCmd = -1;

	// init leds, according to settings
	for(int i=0; i<=button; i++)
		lightButton(i, 1);
	*sliderParam = (*this.*sliderMapping)();
}

void OscSliderV::setSlider(int level) {
	// check if button is actually different from current status
	if(level == button)
		return;

	targetButton = level;
	buttonCmd = (targetButton>button); // on [1] or off [0]
	slideDirection = -1 + 2*buttonCmd; // up [1] or down [-1]

	slideLag = 10000 + ( 7-abs(targetButton-button) )*10000; // constant waiting time + additional one according to slide movement

	// just in case
	pthread_cancel(buttonThread);
	// new one
	if(hasFunction)
		pthread_create(&buttonThread, NULL, slideLoopLauncherFunction, this); // this uses lambda function that in turn changes param [any param modified in the function]
	else
		pthread_create(&buttonThread, NULL, slideLoopLauncherParam, this); // this uses method and changes param passed by reference
}


//------------------------------------------------------------------------------------------------------------------------
// private methods
//------------------------------------------------------------------------------------------------------------------------
// slide loops
void *OscSliderV::slideLoopFunction() {
	while(targetButton != button) {
		button+=slideDirection;
		// light up next button or shut current button, according to slide direction
		lightButton(button+(1-buttonCmd), buttonCmd);

		slideMethod((*this.*sliderMapping)()); // call pointer to lambda, which changes something...somewhere...

		usleep(slideLag);
	}
	return (void *)0;
}
void *OscSliderV::slideLoopParam() {
	while(targetButton != button) {
		button+=slideDirection;
		// light up next button or shut current button, according to slide direction
		lightButton(button+(1-buttonCmd), buttonCmd);

		*sliderParam = (*this.*sliderMapping)(); // call method that changes passed param

		usleep(slideLag);
	}
	return (void *)0;
}

// slide loop launchers
void *OscSliderV::slideLoopLauncherFunction(void *context) {
	set_priority(60);
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS,NULL); // now this thread can be cancelled with just its handle

	return ((OscSliderV *)context)->slideLoopFunction();
}
void *OscSliderV::slideLoopLauncherParam(void *context) {
	set_priority(60);
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS,NULL); // now this thread can be cancelled with just its handle

	return ((OscSliderV *)context)->slideLoopParam();
}

// helper for exponential mapping
float OscSliderV::expMapping(float expMin, float expMax, float expRange, float expNorm,
				 float inMin, float inMax, float inRange,
				 float in, float outMin, float outRange) {
	if(in > inMax)
		in = inMax;
	else if(in < inMin)
		in = inMin;
	in = (in-inMin)/inRange;

	return exp(expMin+expRange*in)/expNorm*outRange + outMin;
}
