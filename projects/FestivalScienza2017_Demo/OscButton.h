/*
 * OscButton.h
 *
 *  Created on: Sep 25, 2017
 *      Author: Victor Zappi
 */

#ifndef OSCBUTTON_H_
#define OSCBUTTON_H_

#include "OSCClient.h"

class OscButton {
	using fptr_synth = void(*)(); // to pass lambda as method field
	using fptr_synthParam = void(*)(bool); // to pass lambda as method field with param

public:
	void init(int rw, int col, OSCClient *client, fptr_synth method);
	void init(int rw, int col, OSCClient *client, fptr_synthParam method);
	void setButton(bool on);

private:
	void lightButton(int cmd);
	void (OscButton::*set_buttonPtr)(bool);
	void set_button(bool on);
	void set_buttonParam(bool on);

	int row;
	int column;
	OSCClient *oscClient;
	fptr_synth buttonMethod;
	fptr_synthParam buttonMethodParam;

};

// wrapper
inline void OscButton::lightButton(int cmd) {
	oscClient->sendMessageNow(oscClient->newMessage.to("/monome/grid/led/set").add(row).add(column).add(cmd).end());
}

#endif /* OSCBUTTON_H_ */
