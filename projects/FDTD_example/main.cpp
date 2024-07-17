/*
 * full real-time 2D wave propagation as audio filter, using FDTD solver with implicit method
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <iostream>
#include <string>

// elapsed time
#include <sys/time.h>

// thread and priority
#include <pthread.h>
#include <sched.h>

// back end
#include "AudioEngine.h"
#include "priority_utils.h"


// application stuff
#include "FDTDSynth.h"
#include "controls.h"

using namespace std;

// Helper function to extract absolute directory path of current file
string getCurrentFilePath() {
	string filePath = string(__FILE__);
    size_t lastSlash = filePath.find_last_of('/');
    if (lastSlash == std::string::npos) {
        return "";
    }
    return filePath.substr(0, lastSlash);
}




//****************************************
// change these:
//****************************************

bool wacomPen = false; // set true if using wacom tablet input device

//----------------------------------------
// domain variables, all externs in defineDomain.cpp
//----------------------------------------
int domainSize[2] = {180, 180};
float magnifier   = 2;
int rate_mul      = 1;
bool fullscreen   = false; // fullscreen simply hides the window's bar, but frame size [(domain+PML)*magnifier] must be manually set to match screen resolution
int monitorIndex  = -1;    // used only if fullscreen
//----------------------------------------

//----------------------------------------
// control variables, all externs, in controls.cpp and defineDomain.cpp
//----------------------------------------
int excitationCoord[2] = {domainSize[0]/2, domainSize[1]/2};
int excitationDim[2]   = {1, 1};
ex_type excitationType = ex_sin;
int listenerCoord[2]   = {excitationCoord[0], excitationCoord[1]+1};
float excitationVol    = 0.8;
float maxPressure      = 2000;
float physicalParams[1] = {343.2}; // [m/s] speed of sound in dry air at 20 celsius
//----------------------------------------

//----------------------------------------
// audio variables
//----------------------------------------
string device           = "hw:0,0";            // playback device ["hw:2,0" = USB interface most likely]
snd_pcm_format_t format = SND_PCM_FORMAT_S32;  // sample format
unsigned int rate       = 44100;               // stream rate, extern
unsigned int channels   = 2;	               // count of channels
int period_size         = 128;     	           // period length in frames
int buffer_size         = 256;		           // ring buffer length in frames [contains period]
//****************************************
//****************************************



//----------------------------------------
// main variables, all externs, in controls.cpp and defineDomain.cpp
//----------------------------------------
MonoEngine_int32LE audioEngine;
FDTDSynth *fdtdSynth;
//----------------------------------------

//----------------------------------------
// other audio variables
//----------------------------------------
audioThread_data at_data;

int method;						// alsa low level buffer transfer method
// only via command line
int formatInt;
int verbose;                    // verbose flag
int resample;      	            // enable alsa-lib resampling
int period_event;               // produce poll event after each period

/*struct option long_option[] =
{
	{"help", 0, NULL, 'h'},
	{"device", 1, NULL, 'D'},
	{"rate", 1, NULL, 'r'},
	{"channels", 1, NULL, 'c'},
	{"buffer", 1, NULL, 'b'},
	{"period", 1, NULL, 'p'},
	{"method", 1, NULL, 'm'},
	{"format", 1, NULL, 'o'},
	{"verbose", 1, NULL, 'v'},
	{"noresample", 1, NULL, 'n'},
	{"pevent", 1, NULL, 'e'},
	{NULL, 0, NULL, 0},
};*/

int morehelp;
//----------------------------------------



static void help(void)
{
	int k;
	printf(
			"Usage: pcm [OPTION]... [FILE]...\n"
			"***PLEASE DO NOT USE LONG OPTIONS!***\n"
			"-h,--help      help\n"
			"-d,--device    playback device\n"
			"-r,--rate      stream rate in Hz\n"
			//"-c,--channels  count of channels in stream\n"
			"-b,--buffer    ring buffer size in frames\n"
			"-p,--period    period size in frames\n"
			//"-m,--method    transfer method\n"
			//"-o,--format    sample format\n"
			"-v,--verbose   show the PCM setup parameters\n"
			"-n,--noresample  do not resample\n"
			"-e,--pevent    enable poll event after each period\n"
			"\n");
	printf("Recognized sample formats are:");
	for (k = 0; k < SND_PCM_FORMAT_LAST; ++k) {
		const char *s = snd_pcm_format_name( (snd_pcm_format_t) k);
		if (s)
			printf(" %s", s);
	}
	printf("\n");
	printf("Recognized transfer methods are:");
	for (k = 0; audioEngine.transfer_methods[k].name; k++)
		printf(" %s", audioEngine.transfer_methods[k].name);
	printf("\n");
}




int initFDTDSynth(float ***domainPixels) {
	string shaderLocation = getCurrentFilePath() + "/shaders_FDTD";

	// first check if fullscreen, must be done before init!
	fdtdSynth->monitorSettings(fullscreen, monitorIndex);

	// split init, also an option...and what an option!
	fdtdSynth->init(shaderLocation, domainSize, rate, rate_mul, period_size, magnifier, listenerCoord);
	if(fdtdSynth->setDomain(excitationCoord, excitationDim, domainPixels) != 0)
	 return 1;

	// apply settings
	fdtdSynth->setPhysicalParameters((void *)physicalParams);
	fdtdSynth->setMaxPressure(maxPressure);
	fdtdSynth->setExcitationVolume(excitationVol);
	fdtdSynth->setExcitationType(excitationType);

	return 0;
}

// domainPixels passed by reference since dynamically allocated in the function
void initDomainPixels(float ***&domainPixels) {
	//---------------------------------------------
	// all is in domain frame of ref, which means no PML nor dead layers included
	//---------------------------------------------

	// domain pixels [aka walls!]
	domainPixels = new float **[domainSize[0]];
	float airVal = FDTD::getPixelAlphaValue(FDTD::cell_air, 0);
	for(int i = 0; i <domainSize[0]; i++) {
		domainPixels[i] = new float *[domainSize[1]];
		for(int j = 0; j <domainSize[1]; j++) {
			domainPixels[i][j]    = new float[4];
			domainPixels[i][j][3] = airVal;
		}
	}
}

int initSimulation() {

	float ***domainPixels;

	initDomainPixels(domainPixels);

	//FDTD::fillTextureRow(10, 10, 30, FDTD::cell_wall, domainPixels);

	int retval = initFDTDSynth(domainPixels);

	// get rid of the pixels we dynamically allocated in defineDomain.ccp and copied in our synth already
	for(int i = 0; i <domainSize[0]; i++) {
		for(int j = 0; j <domainSize[1]; j++)
			delete[] domainPixels[i][j];
		delete[] domainPixels[i];
	}
	delete[] domainPixels;


	return retval;
}


int parseArguments() {
	morehelp = 0;
	while (1)
	{
		int c;
		if ((c = getopt(at_data.argc, at_data.argv, "hD:d:r:f:b:p:vne")) < 0)
			break;
		switch (c) {
			case 'h':
				morehelp++;
				break;
			case 'd':
				device = strdup(optarg);
				//audioEngine.setDevice(device.c_str()); // done anyway outside
				break;
			case 'r':
				rate = atoi(optarg);
				rate = rate < 4000 ? 4000 : rate;
				rate = rate > 196000 ? 196000 : rate; //check.......
				//audioEngine.setRate(rate); // done anyway outside
				break;
			/*case 'c':
				channels = atoi(optarg);
				channels = channels < 1 ? 1 : channels;
				channels = channels > 1024 ? 1024 : channels;*/
				//audioEngine.setChannelNum(channels);  // done anyway outside
				break;
			case 'b':
				buffer_size = atoi(optarg);
				//audioEngine.setBufferSize(buffer_size); // done anyway outside
				break;
			case 'p':
				period_size = atoi(optarg);
				//audioEngine.setPeriodSize(period_size); // done anyway outside
				break;
			/*case 'm':
				for (method = 0; audioEngine.transfer_methods[method].name; method++) {
					if (!strcasecmp(audioEngine.transfer_methods[method].name, optarg))
						break;
				}
				if (audioEngine.transfer_methods[method].name == NULL)
					method = 0;
				audioEngine.setTransferMethod(method);
				break;
			case 'o':
				for (formatInt =  SND_PCM_FORMAT_S8; formatInt < SND_PCM_FORMAT_LAST; formatInt++) {
					format = static_cast<snd_pcm_format_t>(formatInt);
					const char *format_name = snd_pcm_format_name( (snd_pcm_format_t) format);
					if (format_name)
						if (!strcasecmp(format_name, optarg))
							break;
				}
				if (format == SND_PCM_FORMAT_LAST)
						format = SND_PCM_FORMAT_S16;
				if (!snd_pcm_format_linear(format) &&
					!(format == SND_PCM_FORMAT_FLOAT_LE || format == SND_PCM_FORMAT_FLOAT_BE)) {
					printf("Invalid (non-linear/float) format %s\n", optarg);
					pthread_exit(NULL);
				}*/
				//audioEngine.setAudioFormat(format); // done anyway outside
				break;
			case 'v':
				verbose = 1;
				audioEngine.setVerbose(verbose);
				break;
			case 'n':
				resample = 0;
				audioEngine.setResample(resample);
				break;
			case 'e':
				period_event = 1;
				audioEngine.setPeriodEvent(period_event);
				break;
		}
	}

	if (morehelp) {
		help();
		return 1;
	}
	return 0;
}

int initAudioThread()
{
	if(verbose==1)
		cout << "---------------->Init Audio Thread" << endl;

	// read settings from command line arguments
	if(parseArguments()!=0)
		return 1;

	// put here so that we can change rate before initializing audio engine ---> overwrites command line arguments
	if(initSimulation()!= 0)
		return -1;

	// set audio parameters
	audioEngine.setPlaybackDevice(device.c_str());
	//audioEngine.setPlaybackChannelNum(channels);
	//audioEngine.setPlaybackAudioFormat(format);
	audioEngine.setPeriodSize(period_size);
	audioEngine.setBufferSize(buffer_size);
	audioEngine.setRate(rate);

	// last touch
	audioEngine.initEngine();
	audioEngine.addAudioModule(fdtdSynth);

	return 0;
}

void *runAudioEngine(void *)
{
	if(verbose==1)
		cout << "__________set Audio Thread priority" << endl;
	// set thread priority
	set_priority(0, verbose);

	if(initAudioThread()>0)
		return (void *)0;

	if(verbose==1)
		cout << "_________________Audio Thread!" << endl;

	audioEngine.startEngine();

	if(fdtdSynth != NULL)
		delete fdtdSynth; // destructor must be called from the same thread in which all GL buffershit was created

	cout << "audio thread ended" << endl;

	return (void *)0;
}

int runRealTimeThreads() {
	pthread_t audioThread;
	pthread_t controlThread;
	if(verbose==1)
		cout << "main() : creating audio thread" << endl;

	if ( pthread_create(&audioThread, NULL, runAudioEngine, NULL) ) {
	  cout << "Error:unable to create thread" << endl;
	  return -1;
	}

	if(verbose==1)
		cout << "main() : creating control thread" << endl;

	if ( pthread_create(&controlThread, NULL, runControlLoop, NULL) ) {
	  cout << "Error:unable to create thread" << endl;
	 return -1;
	}

	pthread_join( audioThread, NULL);

	pthread_cancel(controlThread);
	return 0;
}

int main(int argc, char *argv[]) {
	at_data.argc = argc;
	at_data.argv = argv;

	fdtdSynth = new FDTDSynth(); // needs to be created for both threads' sake

	if(runRealTimeThreads()<0)
		exit(-1);

	printf("Program ended\nBye bye_\n");
	return 0;
}
