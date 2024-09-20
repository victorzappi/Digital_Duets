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
	void addAudioModule(AudioModule *mod);
	std::string initBounce();
	int endBounce();
private:
	bool isRecording;

	void initRender();
	void render(float sampleRate, int numOfSamples, int numOutChannels, double **framebufferOut, int numInChannels, double **framebufferIn);
	void cleanUpRender();

	// void fromRawToFloat_int(snd_pcm_uframes_t offset, int numSamples);
	// void fromFloatToRaw_int(snd_pcm_uframes_t offset, int numSamples);
};

inline void HDH_AudioEngine::addAudioModule(AudioModule *mod) {
	if(numOfAudioModules==1) {
		printf("Cannot add more modules! This HDH engine is optimized for only one module\n");
		return;
	}

	AudioEngine::addAudioModule(mod);
}

#endif /* ENGINE_H_ */
