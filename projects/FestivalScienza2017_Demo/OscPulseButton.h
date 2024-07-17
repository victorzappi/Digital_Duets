/*
 * OscPulseButton.h
 *
 *  Created on: Sep 25, 2017
 *      Author: Victor Zappi
 */

#ifndef OSCPULSEBUTTON_H_
#define OSCPULSEBUTTON_H_

#include "OSCClient.h"

class OscPulseButton {
public:
	~OscPulseButton();
	void init(int rw, int col, OSCClient *client, bool *param);
	void setButton(bool on);

private:
	void lightButton(int cmd);
	void *pulseLoop();
	static void *pulseLoopLauncher(void *context);

	int row;
	int column;
	OSCClient *oscClient;
	int pulseTime; // usec
	pthread_t pulseThread;
	bool pulse;
	bool ledIsOn;
	bool *buttonParam;
};


inline OscPulseButton::~OscPulseButton() {
	pthread_cancel(pulseThread); // just in case
}

// wrapper
inline void OscPulseButton::lightButton(int cmd) {
	oscClient->sendMessageNow(oscClient->newMessage.to("/monome/grid/led/set").add(row).add(column).add(cmd).end());
}


#endif /* OSCPULSEBUTTON_H_ */
