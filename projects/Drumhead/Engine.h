/*
 * Engine.h
 *
 *  Created on: 2017-01-06
 *      Author: Victor Zappi
 */

#ifndef ENGINE_H_
#define ENGINE_H_

#include "AudioEngine.h"

// a simple AudioEngine child class that does not dump the audio into a file
class Engine : public AudioEngine {
	void initRender() {}
	void render(float sampleRate, int numChannels, int numSamples, float **sampleBuffers) {
		memcpy(sampleBuffers[0], readGeneratorsBuffer(numSamples), sizeof(float) * numSamples);
	}
	void cleanUpRender() {}
};

#endif /* ENGINE_H_ */
