/*
 * Engine.cpp
 *
 *  Created on: Jun 22, 2017
 *      Author: vic
 */

#include "HyperDrumheadEngine.h"

#define SCARLETT_HEADPHONES

#ifdef SCARLETT_HEADPHONES
#define SCARLETT_DOUBLE_HEADPHONES
#endif

#include <string>
using namespace std;

// save audio
//#define HDH_BOUNCE

#ifdef HDH_BOUNCE
#include <dirent.h>  // to handle files in dirs
#include <fstream>   // read/write to file ifstream/ofstream
#include <sstream>   // ostringstream

// use a namespace as a lazy fix of double definition of this stuff in AudioEngine class
namespace hdh_ae {

	SF_INFO sfinfo;
	SNDFILE * outfile;
	string outfiledir = ".";
	string outfilename = "bounce_";

	int getNumOfWavFiles(string dirname) {
		DIR *dir;
		struct dirent *ent;
		int fileCnt = 0;

		outfilename = "bounce_";

		// Adapted from http://stackoverflow.com/questions/612097/how-can-i-get-a-list-of-files-in-a-directory-using-c-or-c
		if ((dir = opendir (dirname.c_str())) != NULL) {
			/* print all the files and directories within directory */
			while ((ent = readdir (dir)) != NULL) {
				// Ignore dotfiles and . and .. paths
				if(!strncmp(ent->d_name, ".", 1))
					continue;

				//printf("%s\n", ent->d_name);

				// take only .wav files
				string name = string(ent->d_name);
				int len		= name.length();
				if( (name[len-4]=='.') && (name[len-3]=='w') && (name[len-2]=='a') && (name[len-1]=='v') )
					fileCnt++;
			}
			closedir (dir);
		} else {
			/* could not open directory */
			printf("Could not open directory %s!\n", dirname.c_str());
			return -1;
		}
		return fileCnt;
	}
}

using namespace hdh_ae;
#endif



string HDH_AudioEngine::initBounce() {
#ifdef HDH_BOUNCE
	if(isRecording)
		return NULL;

	// change out file name according to how many wav files we have in the dir
	// so that we don't over write
	int fileNum = getNumOfWavFiles(outfiledir);
	ofstream dumpfile;
	ostringstream convert;
	convert << fileNum;
	outfilename += convert.str() + ".wav";

	// save audio
	sfinfo.channels = 1;
	sfinfo.samplerate = rate;
	sfinfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
	outfile = sf_open(outfilename.c_str(), SFM_WRITE, &sfinfo);

	isRecording = true;

	return outfilename;
#else
	return "";
#endif

}






int HDH_AudioEngine::endBounce() {
#ifdef HDH_BOUNCE
	if(!isRecording)
		return -1;

	isRecording = false;

	// to properly save audio
	sf_write_sync(outfile);
	sf_close(outfile);
#endif
	return 0;
}


//----------------------------------------------------------------------------------------------------------
// private methods
//----------------------------------------------------------------------------------------------------------

void HDH_AudioEngine::initRender() {
	isRecording = false;
	initBounce();
	isRecording = true;
}

void HDH_AudioEngine::render(float sampleRate, int numChannels, int numSamples, float **sampleBuffers) {
#ifndef SCARLETT_HEADPHONES
	memcpy(sampleBuffers[1], readAudioModulesBuffers(numSamples, sampleBuffers[0]), sizeof(float) * numSamples);
	//memcpy(sampleBuffers[0], readGeneratorsBuffer(numSamples), sizeof(float) * numSamples);
	//memcpy(sampleBuffers[1], sampleBuffers[1], sizeof(float) * numSamples);
	//memcpy(sampleBuffers[1], readGeneratorsBuffer(numSamples, sampleBuffers[0]), sizeof(float) * numSamples);
#ifdef HDH_BOUNCE
	sf_write_float(outfile, sampleBuffers[0], numSamples);
#endif

#else
	//memcpy(sampleBuffers[2], readGeneratorsBuffer(numSamples), sizeof(float) * numSamples);
	memcpy(sampleBuffers[3], readAudioModulesBuffers(numSamples, sampleBuffers[2]), sizeof(float) * numSamples);

	//memcpy(sampleBuffers[0], sampleBuffers[2], sizeof(float) * numSamples);
	//memcpy(sampleBuffers[1], sampleBuffers[2], sizeof(float) * numSamples);

	//memcpy(sampleBuffers[3], sampleBuffers[2], sizeof(float) * numSamples);

#ifdef SCARLETT_DOUBLE_HEADPHONES
	memcpy(sampleBuffers[4], sampleBuffers[2], sizeof(float) * numSamples);
	memcpy(sampleBuffers[5], sampleBuffers[2], sizeof(float) * numSamples);
#endif

	//memcpy(sampleBuffers[0], sampleBuffers[2], sizeof(float) * numSamples);

#ifdef HDH_BOUNCE
	if(isRecording)
		sf_write_float(outfile, sampleBuffers[2], numSamples);
#endif

#endif
}


void HDH_AudioEngine::cleanUpRender() {
	endBounce();
}

void HDH_AudioEngine::fromRawToFloat_int(snd_pcm_uframes_t offset, int numSamples) {
	int *insamples = (int *)capture.rawSamples;
	for(int n = 0; n < numSamples; n++) {
			for(unsigned int chn = 0; chn < capture.channels; chn++) {
				capture.sampleBuffers[chn][n] = insamples[n*capture.channels + chn]/float(capture.maxVal);
			}
	}
}

void HDH_AudioEngine::fromFloatToRaw_int(snd_pcm_uframes_t offset, int numSamples) {

	int *outsamples = (int *)playback.rawSamples;
	for(int n = 0; n < numSamples; n++) {
		for(unsigned int chn = 0; chn < playback.channels; chn++) {
			outsamples[n*playback.channels + chn] = playback.sampleBuffers[chn][n]*float(playback.maxVal);
		}
	}
}

