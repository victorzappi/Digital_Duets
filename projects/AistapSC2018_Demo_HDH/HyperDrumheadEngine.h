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
class HDH_AudioEngine : public AudioEngine {
public:
	std::string initBounce();
	int endBounce();
private:
	bool isRecording;

	void initRender();
	void render(float sampleRate, int numChannels, int numSamples, float **sampleBuffers);
	void cleanUpRender();

	void fromRawToFloat_int(snd_pcm_uframes_t offset, int numSamples);
	void fromFloatToRaw_int(snd_pcm_uframes_t offset, int numSamples);
};

#endif /* ENGINE_H_ */
