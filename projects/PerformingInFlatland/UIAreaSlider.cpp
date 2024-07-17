/*
 * UIAreaSlider.cpp
 *
 *  Created on: 2016-09-13
 *      Author: Victor Zappi
 */

#include "UIAreaSlider.h"

UIAreaSlider::UIAreaSlider() {
	sliderVal        = -1;
	sliderDefaultVal = -1;
	initSize(-1, -1, -1, -1);
	sticky = false;
	moved  = false;
	id = cnt++;
}

void UIAreaSlider::init(float leftCoord, float bottomCoord, float w, float h, float initVal, bool stck) {
	sliderVal        = initVal;
	sliderDefaultVal = initVal;
	initSize(leftCoord, bottomCoord, w, h);
	sticky = stck;
}

bool UIAreaSlider::checkSlider(float x, float y) {
	if(isContainedX(x) && isContainedY(y)) {
		sliderVal = (y-bottomPixelCoord)/height;
		//printf("UIAreaSlider %d value: %f\n", id, sliderVal);
		moved = true;
	}
	else {
		// if we got released and we are sticky, we go back to default!
		if(moved && sticky)
			reset();
		moved = false;
	}
	return moved;
}
//----------------------------------------------------------------------
// private
//----------------------------------------------------------------------
int UIAreaSlider::cnt = 0;
