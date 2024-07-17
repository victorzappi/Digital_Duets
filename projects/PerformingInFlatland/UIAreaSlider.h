/*
 * UIAreaSlider.h
 *
 *  Created on: 2016-09-13
 *      Author: Victor Zappi
 */

#ifndef UIAREASLIDER_H_
#define UIAREASLIDER_H_

#include <cstdio> // for printf...debug

// multitouch support still missing

// a rough class for rough men
// it works with normalized coordinates
class UIAreaSlider {
public:
	UIAreaSlider();
	~UIAreaSlider();
	void init(float leftCoord, float bottomCoord, float w, float h, float initVal=0, bool sticky=false); // bottom left origin, same as OpenGL [to make life simpler]
	bool checkSlider(float x, float y);
	void getSize(float size[]);
	bool reset();
	float getSliderVal();
	bool isSticky();
private:
	static int cnt;
	int id;
	float sliderVal;
	float sliderDefaultVal;
	float bottomPixelCoord;
	float leftPixelCoord;
	float width;
	float height;
	bool moved;
	bool sticky; // means that returns to default position when released
	void initSize(float leftCoord, float bottomCoord, float w, float h);
	bool isContainedX(float x);
	bool isContainedY(float y);
};

inline UIAreaSlider::~UIAreaSlider() {
	if(cnt > 1)
		cnt--;
}

inline void UIAreaSlider::getSize(float size[]) {
	size[0] = leftPixelCoord;
	size[1] = bottomPixelCoord;
	size[2] = width;
	size[3] = height;
}

inline bool UIAreaSlider::reset() {
	if(sliderVal != sliderDefaultVal) {
		sliderVal = sliderDefaultVal;
		return true;
	}

	return false;
}

inline float UIAreaSlider::getSliderVal() {
	return sliderVal;
}

//----------------------------------------------------------------------
// private
//----------------------------------------------------------------------
inline void UIAreaSlider::initSize(float leftCoord, float bottomCoord, float w, float h) {
	leftPixelCoord   = leftCoord;
	bottomPixelCoord = bottomCoord;
	width  			 = w;
	height           = h;

}

inline bool UIAreaSlider::isContainedX(float x) {
	if( (x>=leftPixelCoord) && (x<leftPixelCoord+width) )
		return true;
	return false;
}

inline bool UIAreaSlider::isContainedY(float y) {
	if( (y>=bottomPixelCoord) && (y<bottomPixelCoord+height) )
		return true;
	return false;
}

inline bool UIAreaSlider::isSticky() {
	return sticky;
}

#endif /* UIAREASLIDER_H_ */

