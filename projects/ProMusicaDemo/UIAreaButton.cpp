/*
 * UIAreaButton.cpp
 *
 *  Created on: 2016-09-12
 *      Author: Victor Zappi
 */

#include "UIAreaButton.h"

UIAreaButton::UIAreaButton() {
	triggered = false;
	initSize(-1, -1, -1, -1);
	id = cnt++;
}

void UIAreaButton::init(float leftCoord, float bottomCoord, float w, float h) {
	triggered = false;
	initSize(leftCoord, bottomCoord, w, h);

}
bool UIAreaButton::checkTrigger(float x, float y) {
	if(isContainedX(x) && isContainedY(y)) {
		//printf("UIAreaButton %d triggered\n", id);
		triggered = true;
	}
	else {
		//printf("UIAreaButton %d nope\n", id);
		triggered = false;
	}

	return triggered;
}

//----------------------------------------------------------------------
// private
//----------------------------------------------------------------------
int UIAreaButton::cnt = 0;
