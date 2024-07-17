/*
 * OscPulseButton.cpp
 *
 *  Created on: Sep 25, 2017
 *      Author: Victor Zappi
 */

#include "OscPulseButton.h"

#include "priority_utils.h"


void OscPulseButton::init(int rw, int col, OSCClient *client, bool *param) {
	row    = rw;
	column = col;
	oscClient = client;
	buttonParam = param;

	pulseTime = 400000; // usec

	// starts as lit fixed
	pulse   = false;
	ledIsOn = true;
	lightButton(1);
}

void OscPulseButton::setButton(bool on) {
	*buttonParam = on; // set linked param

	if(on) {
		// pulse
		pulse = true;
		pthread_cancel(pulseThread); // just in case
		pthread_create(&pulseThread, NULL, pulseLoopLauncher, this);
	}
	else
		pulse = false; // stop pulse
}



//------------------------------------------------------------------------------------------------------------------------
// private methods
//------------------------------------------------------------------------------------------------------------------------
// loop
void *OscPulseButton::pulseLoop() {
	while(pulse) {
		ledIsOn = !ledIsOn;
		lightButton(ledIsOn);
		usleep(pulseTime);
	}

	lightButton(1); // when stop pulse put led back on

	return (void *)0;
}

// pulse loop launcher
void *OscPulseButton::pulseLoopLauncher(void *context) {
	set_priority(60);
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS,NULL); // now this thread can be cancelled with just its handle

	return ((OscPulseButton *)context)->pulseLoop();
}

