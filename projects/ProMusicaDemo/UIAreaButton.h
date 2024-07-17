/*
 * UIAreaButton.h
 *
 *  Created on: 2016-09-12
 *      Author: Victor Zappi
 */

#ifndef UIAREABUTTON_H_
#define UIAREABUTTON_H_

#include <cstdio> // for printf...debug

// multitouch support still missing

// a rough class for rough men
// it works with normalized coordinates
class UIAreaButton {
public:
	UIAreaButton();
	~UIAreaButton();
	void init(float leftCoord, float bottomCoord, float w, float h); // bottom left origin, same as OpenGL [to make life simpler]
	bool checkTrigger(float x, float y);
	void getSize(float size[]);
	void untrigger();
	bool isTriggered();
private:
	static int cnt;
	int id;
	bool triggered;
	float bottomPixelCoord;
	float leftPixelCoord;
	float width;
	float height;
	void initSize(float leftCoord, float bottomCoord, float w, float h);
	bool isContainedX(float x);
	bool isContainedY(float y);
};

inline UIAreaButton::~UIAreaButton() {
	if(cnt > 1)
		cnt--;
}

inline void UIAreaButton::getSize(float size[]) {
	size[0] = leftPixelCoord;
	size[1] = bottomPixelCoord;
	size[2] = width;
	size[3] = height;
}

inline void UIAreaButton::untrigger() {
	triggered = false;
}

inline bool UIAreaButton::isTriggered() {
	return triggered;
}

//----------------------------------------------------------------------
// private
//----------------------------------------------------------------------
inline void UIAreaButton::initSize(float leftCoord, float bottomCoord, float w, float h) {
	leftPixelCoord   = leftCoord;
	bottomPixelCoord = bottomCoord;
	width  			 = w;
	height           = h;
}

inline bool UIAreaButton::isContainedX(float x) {
	if( (x>=leftPixelCoord) && (x<leftPixelCoord+width) )
		return true;
	return false;
}

inline bool UIAreaButton::isContainedY(float y) {
	if( (y>=bottomPixelCoord) && (y<bottomPixelCoord+height) )
		return true;
	return false;
}

#endif /* UIAREABUTTON_H_ */
