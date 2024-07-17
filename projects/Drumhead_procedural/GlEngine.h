/*
 * GlEngine.h
 *
 *  Created on: 2016-12-29
 *      Author: Victor Zappi
 */

#ifndef GLENGINE_H_
#define GLENGINE_H_

#include "AudioEngine.h"

class GlEngine : public AudioEngine {
public:
	void initRender();
	void render(float sampleRate, int numChannels, int numSamples, float **sampleBuffers);
	void cleanUpRender();
};

#endif /* GLENGINE_H_ */
