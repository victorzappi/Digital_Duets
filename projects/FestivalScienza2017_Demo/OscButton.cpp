/*
 * OscButton.cpp
 *
 *  Created on: Sep 25, 2017
 *      Author: Victor Zappi
 */

#include "OscButton.h"

void OscButton::init(int rw, int col, OSCClient *client, fptr_synth method) {
	row    = rw;
	column = col;
	oscClient = client;
	buttonMethod = method;
	buttonMethodParam = NULL;

	set_buttonPtr = &OscButton::set_button;

	lightButton(1); // starts as on!

}

void OscButton::init(int rw, int col, OSCClient *client, fptr_synthParam method) {
	row    = rw;
	column = col;
	oscClient = client;
	buttonMethodParam = method;
	buttonMethod = NULL;

	set_buttonPtr = &OscButton::set_buttonParam;

	lightButton(1); // starts as on!
}

// this exposes the function pointer, which in turn is mapped to 2 methods that use one of the passed lambdas!
void OscButton::setButton(bool on) {
	(*this.*set_buttonPtr)(on);
}



//------------------------------------------------------------------------------------------------------------------------
// private methods
//------------------------------------------------------------------------------------------------------------------------

// methods that use the lambdas
void OscButton::set_button(bool on) {
	lightButton(!on); // when button triggered, led turns off

	if(on)
		buttonMethod();
}
void OscButton::set_buttonParam(bool on) {
	lightButton(!on); // when button triggered, led turns off

	buttonMethodParam(on);
}
