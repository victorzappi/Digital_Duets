/*
 * QuadHyperDrumHead.h
 *
 *  Created on: Mar 6, 2017
 *      Author: vic
 */

#ifndef QUADHYPERDRUMHEAD_H_
#define QUADHYPERDRUMHEAD_H_

#include "Quad.h"
#include <stdio.h>


class QuadHyperDrumHead: public QuadExplicitFDTD {
public:
	void init(int stateNum, int attrNum, int texW, int texH, float deltaTx, float deltaTy); // stateNum and attrNum are passed as a check
	void setQuad(int topleftX, int topleftY, int width, int height, int coordNShiftX);

private:
	// 2 for texture pos (x, y) of this cell at previous state
	float texCoord5x[4];
	float texCoord5y[4];

	// texCoord6 is not used
};

#endif /* QUADHYPERDRUMHEAD_H_ */
