/*
 * Engine.cpp
 *
 *  Created on: Jun 22, 2017
 *      Author: vic
 */

#include "HyperDrumheadEngine.h"


#include <string>
using namespace std;

// save audio
#define HDH_BOUNCE

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

float *outBuffFloat;

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

	outBuffFloat = new float[period_size];

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

	delete[] outBuffFloat;
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

void HDH_AudioEngine::render(float sampleRate, int numOfSamples, int numOutChannels, double **framebufferOut, int numInChannels, double **framebufferIn) {

	//hdh synth
	double *outBuff;
	if(numOfAudioModulesInOut==1) {
		double **inBuff;
		if(isFullDuplex)
			inBuff = framebufferIn;
		else
			inBuff = silenceBuff; //...by setting silent input buffers

		// hdh synth is a mono in/out module, so we get first channel only and save cpu cycles
		outBuff = audioModulesInOut[0]->getFrameBuffer(numOfSamples, inBuff)[0]; // we simply pass pointer to all input channels, no matter the number of in channels of the module
	}
	else
		outBuff = silenceBuff[0];


	// main output channels, hdh only
	memcpy(framebufferOut[0], outBuff, sizeof(double) * numOfSamples);
	memcpy(framebufferOut[1], outBuff, sizeof(double) * numOfSamples);



	// click
	double *clickBuff;
	if(numOfAudioModulesOut==1) {
		// click is a mono out module, so we get first channel only and save cpu cycles
		clickBuff = audioModulesOut[0]->getFrameBuffer(numOfSamples)[0];
	}
	else
		clickBuff = silenceBuff[1];

	// we mix hdh and click
	for(int n=0; n<numOfSamples; n++)
		outBuff[n] += clickBuff[n];
	// and send it to headphones outputs

	// first headphones output channels
	memcpy(framebufferOut[2], outBuff, sizeof(double) * numOfSamples);
	memcpy(framebufferOut[3], outBuff, sizeof(double) * numOfSamples);

	// second headphones output channels
	memcpy(framebufferOut[4], outBuff, sizeof(double) * numOfSamples);
	memcpy(framebufferOut[5], outBuff, sizeof(double) * numOfSamples);


#ifdef HDH_BOUNCE
	if(isRecording) {
		std::copy(outBuff, outBuff+numOfSamples, outBuffFloat);
		sf_write_float(outfile, outBuffFloat, numOfSamples);
	}
#endif
}


void HDH_AudioEngine::cleanUpRender() {
	endBounce();
}

//VIC i can use this simplified method cos i know that on this machine [which is little endian] integers are 32 bits and i am using 32 bit integers as samples!
void HDH_AudioEngine::fromRawToFloat_int(snd_pcm_uframes_t offset, int numSamples) {
	int *insamples = (int *)capture.rawSamples;
	for(int n = 0; n < numSamples; n++) {
			for(unsigned int chn = 0; chn < capture.channels; chn++) {
				capture.frameBuffer[chn][n] = insamples[n*capture.channels + chn]/double(capture.maxVal);
			}
	}
}

void HDH_AudioEngine::fromFloatToRaw_int(snd_pcm_uframes_t offset, int numSamples) {
	int *outsamples = (int *)playback.rawSamples;
	for(int n = 0; n < numSamples; n++) {
		for(unsigned int chn = 0; chn < playback.channels; chn++) {
			outsamples[n*playback.channels + chn] = playback.frameBuffer[chn][n]*double(playback.maxVal);
		}
	}
}

