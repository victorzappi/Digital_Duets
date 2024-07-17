/*
 * QuadHyperDrumHead.cpp
 *
 *  Created on: Mar 6, 2017
 *      Author: vic
 */

#include "QuadHyperDrumHead.h"

void QuadHyperDrumHead::init(int stateNum, int attrNum, int texW, int texH, float deltaTx, float deltaTy) {
	numOfStates 	= stateNum;
	numOfAttributes = attrNum;
	textureW 		= texW;
	textureH 		= texH;
	deltaTexX 		= deltaTx;
	deltaTexY 		= deltaTy;

	if(numOfStates != 2)
		printf("Explicit FDTD method is designed for 2 states, not %d!\n", numOfStates);

	if(numOfAttributes != 16)
		printf("Explicit FDTD method is designed for 16 attributes, not %d!\n", numOfAttributes);
}

// receives coordinates of extremes and shift values in pixels, with origin in bottom left corner [all is HUMAN INTELLIGIBLE]
// then
// normalizes vertices positions [-1.0, 1.0] and tex coords [0.0, 1]
// this is for main quads [also audio, but audio is simpler...no need for neighbors]
void QuadHyperDrumHead::setQuad(int topleftX, int topleftY, int width, int height, int coordNShiftX) {
	deallocateStructures();
	allocateStructures();
	// start with vertices
	setVertexPositions(topleftX, topleftY, width, height);

	// now we work out tex coords for states N and N-1

	// tmp vars to save vertices positions normalized and scaled between [0.0, 1] == u v texture coord for state N+1
	float scaledVx;
	float scaledVy;

	// tmp vars to convert the pixel shift to frag shift [0.0, 1,0]
	float coordNShiftFragX;

	for(int i=0; i<4; i++) {
		// scale position
		scaledVx = (vertx[i]+1)/2.0f;
		scaledVy = (verty[i]+1)/2.0f;

		// convert shift to texture N
		coordNShiftFragX = ((float)coordNShiftX)/((float)textureW);

		// center texture N
		texCoord0x[i] = scaledVx + coordNShiftFragX;
		texCoord0y[i] = scaledVy;

		// left texture N
		texCoord1x[i] = texCoord0x[i] - deltaTexX;
		texCoord1y[i] = texCoord0y[i];

		// up texture N
		texCoord2x[i] = texCoord0x[i];
		texCoord2y[i] = texCoord0y[i] + deltaTexY;

		// right texture N
		texCoord3x[i] = texCoord0x[i] + deltaTexX;
		texCoord3y[i] = texCoord0y[i];

		// down texture N
		texCoord4x[i] = texCoord0x[i];
		texCoord4y[i] = texCoord0y[i] - deltaTexY;

		// center texture N-1
		texCoord5x[i] = scaledVx;
		texCoord5y[i] = scaledVy;


		// now that they're ready, let's save all attributes in order
		int index = i*numOfAttributes;
		attributes[index]    = vertx[i];
		attributes[index+1]  = verty[i];
		attributes[index+2]  = texCoord0x[i];
		attributes[index+3]  = texCoord0y[i];

		attributes[index+4]  = texCoord1x[i];
		attributes[index+5]  = texCoord1y[i];
		attributes[index+6]  = texCoord2x[i];
		attributes[index+7]  = texCoord2y[i];

		attributes[index+8]  = texCoord3x[i];
		attributes[index+9]  = texCoord3y[i];
		attributes[index+10] = texCoord4x[i];
		attributes[index+11] = texCoord4y[i];

		attributes[index+12]  = texCoord5x[i];
		attributes[index+13]  = texCoord5y[i];
		attributes[index+14] = 0; // not used, but needed to make num of attr multiple of 4 [so that they can be quickly packed in vec4]
		attributes[index+15] = 0; // not used
	}
}
