/*
 * full real-time 2D wave propagation as audio filter
 */
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <iostream>
#include <string>

// elapsed time
#include <sys/time.h>

// thread and priority
#include <pthread.h>
#include <sched.h>

// back end
#include "AudioEngine.h"
#include "ImplicitFDTD.h"
#include "priority_utils.h"


// application stuff
#include "FlatlandExcitation.h"
#include "FlatlandSynth.h"
#include "controls.h"

#include "defineDomain.h"

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



//VIC why reed always makes sound, even with no input pressure? needs filter?

enum simulation_type {sim_openSpaceEx, sim_empty, sim_singleWall, sim_doubleWall, sim_shrinkingTube, sim_customTube, sim_fantVowel, sim_storyVowel, sim_contour};
//****************************************
// change these:
//****************************************
simulation_type simulationID = sim_openSpaceEx;

bool glottalLowPassToggle = true;  // this acts only on the excitation filter implemented in the shader
float glottalLowPassFreq = 22000;//44100*0.5;  // this determines the bandwith of both the shader excitation filter and the sinc windowed impulse

bool filterToggle = true;
bool absorption   = true;

bool wacomPen = true; // set true if using wacom tablet input device
//****************************************
//****************************************



//----------------------------------------
// main variables, all externs, in controls.cpp and defineDomain.cpp
//----------------------------------------
AudioEngine audioEngine;
FlatlandSynth *flatlandSynth;
//----------------------------------------


//----------------------------------------
// audio variables
//----------------------------------------
audioThread_data at_data;


// manually inited
string device           = "plughw:2,0";       	  // playback device [generally "hw:2,0" = m-audio eight track]
snd_pcm_format_t format = SND_PCM_FORMAT_S32; // sample format
unsigned int rate       = 44100;      		  // stream rate, extern
unsigned int channels   = 2;	    		  // count of channels [8 for m-audio eight track]
int period_size         = 128;       	      // period length in frames
int buffer_size         = 1024;	              // ring buffer length in frames [contains period]
// only via command line
int method;						// alsa low level buffer transfer method
int formatInt;
int verbose;                    // verbose flag
int resample;      	            // enable alsa-lib resampling
int period_event;               // produce poll event after each period

const char* gMidiPort0 = "/dev/midi3"; // THE USED MIDI CONTROLLER IS A NOVATION LAUNCH CONTROL XL

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


//----------------------------------------
// domain variables, all externs in defineDomain.cpp
//----------------------------------------
int domainSize[2] = {92, 40};
float magnifier   = 5;
int rate_mul      = 4;
float ds          = -1;
bool fullscreen   = false;
int monitorIndex  = -1; // used only if fullscreen
//----------------------------------------


//----------------------------------------
// control variables, all externs, in controls.cpp and defineDomain.cpp
//----------------------------------------
int listenerCoord[2]   = {-1, -1};
int feedbCoord[2]      = {-1, -1};
int excitationDim[2]   = {-1, -1};
int excitationCoord[2] = {-1, -1};
float excitationAngle  = -2;
excitation_model exModel = exMod_noFeedback;
float excitationVol    = 1;
float maxPressure      = 2000; // as in Aerophones
float physicalParams[5] = {C_VT, RHO_VT, MU_VT, PRANDTL_VT, GAMMA_VT};
//float physicalParams[5] = {C_AEROPHONES, RHO_AEROPHONES, MU_AEROPHONES, PRANDTL_AEROPHONES, GAMMA_AEROPHONES}; // official Aerophones params
//float physicalParams[5] = {C_TUBES, RHO_TUBES, MU_TUBES, PRANDTL_TUBES, GAMMA_TUBES}; // real Aerophones params, at zero degrees
float a1               = 0.0001; // first area [m2] -> for excitation models
//----------------------------------------


// UI
float areaButtonLeft[UI_AREA_BUTTONS_NUM];
float areaButtonBottom[UI_AREA_BUTTONS_NUM];
float areaButtonW[UI_AREA_BUTTONS_NUM];
float areaButtonH[UI_AREA_BUTTONS_NUM];

float areaSliderLeft[UI_AREA_SLIDERS_NUM];
float areaSliderBottom[UI_AREA_SLIDERS_NUM];
float areaSliderW[UI_AREA_SLIDERS_NUM];
float areaSliderH[UI_AREA_SLIDERS_NUM];
float areaSliderInitVal[UI_AREA_SLIDERS_NUM];
bool areaSliderSticky[UI_AREA_SLIDERS_NUM];

static void help(void)
{
	int k;
	printf(
			"Usage: pcm [OPTION]... [FILE]...\n"
			"***PLEASE DO NOT USE LONG OPTIONS!***\n"
			"-h,--help      help\n"
			"-d,--device    playback device\n"
			"-r,--rate      stream rate in Hz\n"
			"-c,--channels  count of channels in stream\n"
			"-b,--buffer    ring buffer size in frames\n"
			"-p,--period    period size in frames\n"
			"-m,--method    transfer method\n"
			"-o,--format    sample format\n"
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

void initUI() {

	// UI, kind of part of domain

	// buttons
	float buttonsleftStart   = 0.0;
	float buttonsBottomStart = 0.0;

	float buttonsW = 0.09;
	float buttonsH = 0.11;

	areaButtonLeft[0]   = buttonsleftStart;
	areaButtonBottom[0] = buttonsBottomStart;
	areaButtonW[0]      = buttonsW;
	areaButtonH[0]      = buttonsH;

	areaButtonLeft[1]   = areaButtonLeft[0] + areaButtonW[0] + 0.001;
	areaButtonBottom[1] = buttonsBottomStart;
	areaButtonW[1]      = buttonsW;
	areaButtonH[1]      = buttonsH;

	areaButtonLeft[2]   = buttonsleftStart;
	areaButtonBottom[2] = areaButtonBottom[0] + areaButtonH[0] + 0.0019;
	areaButtonW[2]      = buttonsW;
	areaButtonH[2]      = buttonsH;

	areaButtonLeft[3]   = areaButtonLeft[2] + areaButtonW[2] + 0.001;
	areaButtonBottom[3] = areaButtonBottom[1] + areaButtonH[1] + 0.0019;
	areaButtonW[3]      = buttonsW;
	areaButtonH[3]      = buttonsH;

	areaButtonLeft[4]   = buttonsleftStart;
	areaButtonBottom[4] = areaButtonBottom[2] + areaButtonH[2] + 0.0015; //VIC not sure why with a constant y spacing it looks irregular /:
	areaButtonW[4]      = buttonsW;
	areaButtonH[4]      = buttonsH;

	areaButtonLeft[5]   = areaButtonLeft[4] + areaButtonW[4] + 0.001;
	areaButtonBottom[5] = areaButtonBottom[3] + areaButtonH[3] + 0.0015; //VIC not sure why with a constant y spacing it looks irregular /:
	areaButtonW[5]      = buttonsW;
	areaButtonH[5]      = buttonsH;

	// sliders
	float slidersLeftStart   = 0.006;
	float slidersBottomStart = areaButtonBottom[4] + areaButtonH[4] + 0.01;

	float slidersW = (buttonsW - 3*slidersLeftStart)/2;
	float slidersH = ( (1 - slidersBottomStart) - 2*0.01) / 2;
	// first row
	areaSliderLeft[0]    = slidersLeftStart;
	areaSliderBottom[0]  = slidersBottomStart;
	areaSliderW[0]       = slidersW;
	areaSliderH[0]       = slidersH;
	areaSliderInitVal[0] = 0.0;
	areaSliderSticky[0]  = false;

	areaSliderLeft[1]    = areaSliderLeft[0] + areaSliderW[0] + 0.006;
	areaSliderBottom[1]  = areaSliderBottom[0];
	areaSliderW[1]       = slidersW;
	areaSliderH[1]       = slidersH;
	areaSliderInitVal[1] = 0.48; // 1 for q
	areaSliderSticky[1]  = false;

	areaSliderLeft[2]    = areaSliderLeft[1] + areaSliderW[1] + 0.006 + 0.001 + 0.006;
	areaSliderBottom[2]  = areaSliderBottom[1];
	areaSliderW[2]       = slidersW;
	areaSliderH[2]       = slidersH;
	areaSliderInitVal[2] = 0.60835; // for 440 Hz
	areaSliderSticky[2]  = false;

	areaSliderLeft[3]    = areaSliderLeft[2] + areaSliderW[2] + 0.006;
	areaSliderBottom[3]  = areaSliderBottom[2];
	areaSliderW[3]       = slidersW;
	areaSliderH[3]       = slidersH;
	areaSliderInitVal[3] = 0.5;
	areaSliderSticky[3]  = false;
	// second row
	areaSliderLeft[4]    = slidersLeftStart;
	areaSliderBottom[4]  = areaSliderBottom[0] + areaSliderH[0] + 0.01;
	areaSliderW[4]       = slidersW;
	areaSliderH[4]       = slidersH;
	areaSliderInitVal[4] = areaSliderInitVal[0];
	areaSliderSticky[4]  = true;

	areaSliderLeft[5]    = areaSliderLeft[4] + areaSliderW[4] + 0.006;
	areaSliderBottom[5]  = areaSliderBottom[4];
	areaSliderW[5]       = slidersW;
	areaSliderH[5]       = slidersH;
	areaSliderInitVal[5] = areaSliderInitVal[1];
	areaSliderSticky[5]  = true;

	areaSliderLeft[6]    = areaSliderLeft[5] + areaSliderW[5] + 0.006 + 0.001 + 0.006;
	areaSliderBottom[6]  = areaSliderBottom[5];
	areaSliderW[6]       = slidersW;
	areaSliderH[6]       = slidersH;
	areaSliderInitVal[6] = areaSliderInitVal[2];
	areaSliderSticky[6]  = true;

	areaSliderLeft[7]    = areaSliderLeft[6] + areaSliderW[6] + 0.006;
	areaSliderBottom[7]  = areaSliderBottom[6];
	areaSliderW[7]       = slidersW;
	areaSliderH[7]       = slidersH;
	areaSliderInitVal[7] = areaSliderInitVal[3];
	areaSliderSticky[7]  = true;
}

// pass domainPixels by reference cos it will be dynamically allocated in the switched function [to which is passed by ref in turn...]
void defineDomain(int simID, float ***&domainPixels) {
	switch(simID) {
		case sim_openSpaceEx:
			openSpaceExcitation(domainPixels);
			break;
		case sim_empty:
			emptyDomain(domainPixels);
			break;
		default:
			break;
	}
}

int initVocalSynth(float ***domainPixels) {

	// time settings
	// use the ones coming from global initialization
	unsigned short period = period_size;
	int audio_rate        = rate;
	// period and rate are passed first to the synth [here] then to the audio engine [in initAudioThread()]

	// this old, from when audio engine was set BEFORE synth [which made not possible modifying time settings according to the loaded domain]
	//unsigned short period = audioEngine.getPeriodSize();
	//int audio_rate = audioEngine.getRate();

	string shaderLocation = getCurrentFilePath() + "/shaders_VocalTract";
	if(absorption) {
		shaderLocation = getCurrentFilePath() + "/shaders_VocalTract_abs";
		printf("---using absorption filter---\n");
	}
	else
		printf("---using visco-thermal filter---\n");

	// first check if fullscreen, must be done before init!
	flatlandSynth->monitorSettings(fullscreen, monitorIndex);

	// and UI too
	flatlandSynth->setAreaButtons(areaButtonLeft, areaButtonBottom, areaButtonW, areaButtonH);
	flatlandSynth->setAreaSliders(areaSliderLeft, areaSliderBottom, areaSliderW, areaSliderH, areaSliderInitVal, areaSliderSticky);

	// init all!
	//VIC apparently if we use this, when we encounter the PBO writer bug [look at PixelDrawer::init() in PixelDrawer.cpp]
	// the system just does not output audio, without giving any warnings!
	// sooo, it's better to use the split init as below
	//flatlandSynth->init(shaderLocation, domainSize, numOfStates, excitationCoord, excitationDim, domainPixels, audio_rate, rate_mul, period, magnifier, listenerCoord);
	//ds = flatlandSynth->getDS(); // update ds, just in case

	// split init, also an option...and what an option!
	flatlandSynth->init(shaderLocation, domainSize, audio_rate, rate_mul, period, magnifier, listenerCoord, glottalLowPassFreq);
	ds = flatlandSynth->getDS(); // update ds, can use to calculate domains if not done yet
	if(flatlandSynth->setDomain(excitationCoord, excitationDim, domainPixels) != 0)
	 return 1;

	// apply settings
	//flatlandSynth->setListenerPosition(listenerCoord[0], listenerCoord[1]); // this triggers crossfade, while if we init listener in init() it doesn't
	flatlandSynth->setFeedbackPosition(feedbCoord[0], feedbCoord[1]);
	flatlandSynth->setExcitationDirection(excitationAngle);
	flatlandSynth->setPhysicalParameters(physicalParams[0], physicalParams[1], physicalParams[2], physicalParams[3], physicalParams[4]);
	flatlandSynth->setMaxPressure(maxPressure);
	flatlandSynth->setExcitationVolume(excitationVol);
	flatlandSynth->setFilterToggle(filterToggle);
	flatlandSynth->setGlottalLowPassToggle(glottalLowPassToggle);

	flatlandSynth->setA1(a1);

	flatlandSynth->setExcitationModel(exModel); // activate excitation type

	return 0;
}

int initSimulation(int simID) {

	float ***domainPixels;

	defineDomain(simID, domainPixels);

	initUI();

	int retval = initVocalSynth(domainPixels);

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
		if ((c = getopt(at_data.argc, at_data.argv, "hD:d:r:c:f:b:p:m:o:vne")) < 0)
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
			case 'c':
				channels = atoi(optarg);
				channels = channels < 1 ? 1 : channels;
				channels = channels > 1024 ? 1024 : channels;
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
			case 'm':
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
				}
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
	if(initSimulation(simulationID)!= 0)
		return -1;

	// set audio parameters
	audioEngine.setPlaybackDevice(device.c_str());
	audioEngine.setPlaybackChannelNum(channels);
	audioEngine.setPlaybackAudioFormat(format);
	audioEngine.setPeriodSize(period_size);
	audioEngine.setBufferSize(buffer_size);
	audioEngine.setRate(rate);

	// last touch
	audioEngine.initEngine();
	audioEngine.addAudioModule(flatlandSynth);

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


int main(int argc, char *argv[])
{
	at_data.argc = argc;
	at_data.argv = argv;

	flatlandSynth = new FlatlandSynth(); // needs to be created for both threads' sake

	if(runRealTimeThreads()<0)
		exit(-1);

	printf("Program ended\nBye bye_\n");
	return 0;
}
