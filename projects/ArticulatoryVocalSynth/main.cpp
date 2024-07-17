/*
 * full real-time 2D wave propagation as audio filter
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
#include "SimpleExcitation.h"
#include "ArtVocSynth.h"
#include "controls.h"

#include "defineDomain.h"
#include "areaFunctions.h"

#include "freqResponse.h"

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

enum simulation_type {sim_openSpaceEx, sim_empty, sim_singleWall, sim_doubleWall, sim_shrinkingTube, sim_customTube, sim_fantVowel, sim_story96Vowel, sim_story08Vowel, sim_contour};
//****************************************
// change these:
//****************************************
simulation_type simulationID = sim_story08Vowel;
char fant_vowel  = 'a';
char story_vowel = 'a';

int asymmetric = 0; // 0: symmetric, 1: symmetric but with residue, 2: purely asymmetric [flat bottom]
bool nasal = false; // carve 1 pixel nasal opening at the center of the tract

bool glottalLowPassToggle = true; // this acts only on the excitation filter implemented in the shader
float glottalLowPassFreq  = 22000; //44100*0.5;  // this determines the bandwidth of both the shader excitation filter and the sinc windowed impulse

bool filterToggle = true; // use any boundary filter at all?
bool absorption   = true; // use absorption rather than viscothermal?

string contour_nameAndPath = getCurrentFilePath() + "/data/Contours.txt";//"data/foo_contour.txt";//"data/vowel_a_simplePoints.txt";

float spacing_factor = 1; // to arbitrarily shrink the length of the vocal tract tubes
bool auto_spacing_correction = true; // when non symmetric, this sizes down the tube to account for the extra length due to asymmetries
float radii_factor = 1; // to arbitrarily shrink the length of the vocal tract tubes
bool auto_radii_correction   = false;

bool impulseTest = false; // force the use of an impulse as excitation
bool useSinc     = true;  // use the sinc, which is a band limited impulse, low pass limited at glottalLowPassFreq
bool freqResp    = false;  // this forces impulse test, but with no audio feedback [simulation runs silent and faster]
bool fileImpulse = false; // use a recorded impulse, instead of synthesizing one
int numFFTaudioSamples = 26460;//65536; //2^16, that means 2^15 FFT samples

bool wacomPen = false; // set true if using wacom tablet input device

/* IMPORTANT
* domainSize
* and
* rate_mul
* are supposed to be modified separately in each specific simulation function in defineDomain.cpp
*/
//****************************************
//****************************************



//----------------------------------------
// main variables, all externs, in controls.cpp and defineDomain.cpp
//----------------------------------------
AudioEngine audioEngine;
ArtVocSynth *artVocSynth;
//----------------------------------------


//----------------------------------------
// audio variables
//----------------------------------------
audioThread_data at_data;


// manually inited
string device = "hw:0,0";       // playback device ["hw:2,0" = m-audio eight track]
snd_pcm_format_t format = SND_PCM_FORMAT_S32;    	// sample format
unsigned int rate = 44100;      // stream rate, extern
unsigned int channels = 2;	    // count of channels
int method;						// alsa low level buffer transfer method
int period_size = 256;     	// period length in frames
int buffer_size = 512;		    // ring buffer length in frames [contains period]
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


//----------------------------------------
// domain variables, all externs in defineDomain.cpp
//----------------------------------------
int domainSize[2] = {92, 40};
float magnifier   = 5;
int rate_mul      = 4;
float ds          = -1;
bool fullscreen   = false;
int monitorIndex  = -1;
//----------------------------------------


//----------------------------------------
// control variables, all externs, in controls.cpp and defineDomain.cpp
//----------------------------------------
int listenerCoord[2]   = {-1, -1};
int feedbCoord[2]      = {-1, -1};
int excitationDim[2]   = {-1, -1};
int excitationCoord[2] = {-1, -1};
float excitationAngle  = -2;
excitation_model excitationModel = ex_noFeedback;
subGlotEx_type subGlotExType     = subGlotEx_sin;
float excitationVol    = 1;
float maxPressure      = 2000; // as in Aerophones
float physicalParams[5] = {C_VT, RHO_VT, MU_VT, PRANDTL_VT, GAMMA_VT};
//float physicalParams[5] = {C_AEROPHONES, RHO_AEROPHONES, MU_AEROPHONES, PRANDTL_AEROPHONES, GAMMA_AEROPHONES}; // official Aerophones params
//float physicalParams[5] = {C_TUBES, RHO_TUBES, MU_TUBES, PRANDTL_TUBES, GAMMA_TUBES}; // real Aerophones params, at zero degrees
float a1               = 0.0001; // first area [m2] -> for excitation models
//----------------------------------------



//----------------------------------------
// these are extern vars from areaFucntions.cpp
//----------------------------------------
extern float story08Data_a_formants[8];
extern float story08Data_i_formants[8];
extern float story08Data_u_formants[8];
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



// pass domainPixels by reference cos it will be dynamically allocated in the switched function [to which is passed by ref in turn...]
void defineDomain(int simID, float ***&domainPixels) {
	switch(simID) {
		case sim_openSpaceEx:
			openSpaceExcitation(domainPixels);
			break;
		case sim_empty:
			emptyDomain(domainPixels);
			break;
		case sim_singleWall:
			singleWall(domainPixels);
			break;
		case sim_doubleWall:
			doubleWall(domainPixels);
			break;
		case sim_shrinkingTube:
			shrinkingTube(domainPixels);
			break;
		case sim_customTube:
			customTube(domainPixels);
			break;
		case sim_fantVowel:
			fantVowel(fant_vowel, domainPixels, asymmetric, nasal);
			break;
		case sim_story96Vowel:
			story96Vowel(story_vowel, domainPixels, asymmetric, nasal);
			break;
		case sim_story08Vowel:
			story08Vowel(story_vowel, domainPixels, asymmetric, nasal);
			break;
		case sim_contour:
			countour(contour_nameAndPath, domainPixels);
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
	artVocSynth->monitorSettings(fullscreen, monitorIndex);

	// init all!
	//VIC apparently if we use this, when we encounter the PBO writer bug [look at PixelDrawer::init() in PixelDrawer.cpp]
	// the system just does not output audio, without giving any warnings!
	// sooo, it's better to use the split init as below
	//artVocSynth->init(shaderLocation, domainSize, numOfStates, excitationCoord, excitationDim, domainPixels, audio_rate, rate_mul, period, magnifier, listenerCoord);
	//ds = artVocSynth->getDS(); // update ds, just in case

	// split init, also an option...and what an option!
	artVocSynth->init(shaderLocation, domainSize, audio_rate, rate_mul, period, magnifier, listenerCoord, glottalLowPassFreq, useSinc);
	ds = artVocSynth->getDS(); // update ds, can use to calculate domains if not done yet
	if(artVocSynth->setDomain(excitationCoord, excitationDim, domainPixels) != 0)
	 return 1;

	// apply settings
	//artVocSynth->setListenerPosition(listenerCoord[0], listenerCoord[1]); // this triggers crossfade, while if we init listener in init() it doesn't
	artVocSynth->setFeedbackPosition(feedbCoord[0], feedbCoord[1]);
	artVocSynth->setExcitationDirection(excitationAngle);
	artVocSynth->setPhysicalParameters( (void *)physicalParams);
	artVocSynth->setMaxPressure(maxPressure);
	artVocSynth->setExcitationVolume(excitationVol);
	artVocSynth->setFilterToggle(filterToggle);
	artVocSynth->setGlottalLowPassToggle(glottalLowPassToggle);

	artVocSynth->setA1(a1);

	if(impulseTest) {
		excitationModel = ex_noFeedback;
		if(!fileImpulse)
			subGlotExType = subGlotEx_windowedImp;
		else
			subGlotExType = subGlotEx_fileImp;
	}
	artVocSynth->setExcitationModel(excitationModel); // activate excitation type
	artVocSynth->setSubGlotExType(subGlotExType);

	return 0;
}

int initSimulation(int simID) {

	float ***domainPixels;

	defineDomain(simID, domainPixels);

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
	audioEngine.addAudioModule(artVocSynth);

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

	delete artVocSynth;

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

void runFreqResponce() {
	if(verbose==1)
		cout << "__________set Audio Thread priority" << endl;
	// set thread priority
	set_priority(0, verbose);

	if(verbose==1)
		cout << "_________________Frequency response on main thread" << endl;

	// read settings from command line arguments
	parseArguments();

	impulseTest = true; // force impulse input

	// init openg gl simulation
	initSimulation(simulationID);

	// choose formants to compare against [if any] and calculate frequency response!
	if(simulationID == sim_story08Vowel) {
		float *refFormants;
		float numOfFormants= -1;
		if(story_vowel == 'a') {
			refFormants = story08Data_a_formants;
			numOfFormants= sizeof(story08Data_a_formants)/sizeof(story08Data_a_formants[0]);
		}
		else if(story_vowel == 'i') {
			refFormants = story08Data_i_formants;
			numOfFormants= sizeof(story08Data_i_formants)/sizeof(story08Data_i_formants[0]);
		}
		else /*if(story_vowel == 'u')*/ {
			refFormants = story08Data_u_formants;
			numOfFormants= sizeof(story08Data_u_formants)/sizeof(story08Data_u_formants[0]);
		}
		calculateFreqResponse(numFFTaudioSamples, period_size, refFormants, numOfFormants);
	}
	else
		calculateFreqResponse(numFFTaudioSamples, period_size); // no formants to compare against

	delete artVocSynth;
}

int main(int argc, char *argv[]) {
	at_data.argc = argc;
	at_data.argv = argv;

	artVocSynth = new ArtVocSynth(); // needs to be created for both threads' sake

	if(!freqResp) {
		if(runRealTimeThreads()<0)
			exit(-1);
	}
	else
		runFreqResponce();

	printf("Program ended\nBye bye_\n");
	return 0;
}
