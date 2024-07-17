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
#include "ImplicitFDTD.h"
#include "priority_utils.h"


// application stuff
#include "SimpleExcitation.h"
#include "ArtVocSynth.h"
#include "controls.h"

#include "defineDomain.h"
#include "freqResponse.h"
#include "geometries.h"

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



enum simulation_type {sim_openSpaceEx, sim_story08Vowel, sim_2TubesCentric_micA, sim_2TubesCentric_micB,
					  sim_symmTest_completeSymmetry, sim_symmTest_yzSymmetry, sim_symmTest_zSymmetry,
					  sim_symmTest_noSymmetry, sim_symmTest_story_xyzSymmetry, sim_symmTest_story_yzSymmetry};
//****************************************
// change these:
//****************************************
simulation_type simulationID = sim_story08Vowel;//sim_symmTest_story_yzSymmetry; //VIC do z symmetry...not sure how, considering that we want radii_factor to work

char story_vowel = 'a';

float spacing_factor = 1; // to arbitrarily shrink the length of the vocal tract tubes

float radii_factor = 1; // to arbitrarily shrink the height of the vocal tract tubes and the domain
float width_factor = 1; // to arbitrarily shrink the width of the vocal tract tubes
float openSpaceWidth = OPEN_SPACE_WIDTH; //NO_RADIATION_WIDTH; -> NO_RADIATION_WIDTH would set it to same width of center of mouth opening

// good combos:
//radii_factor = 1;
//width_factor = 0.01;
//openSpaceWidth = OPEN_SPACE_WIDTH;
//useRealRadii = true/false; in geometries.cpp

//radii_factor = 0.62;
//width_factor = 1;
//openSpaceWidth = NO_RADIATION_WIDTH;
//useRealRadii = true; in geometries.cpp

bool impulseTest = false; // force the use of an impulse as excitation
bool freqResp    = false;  // this forces impulse test, but with no audio feedback [simulation runs silent and faster]
float glottalLowPassFreq  = 20000;  // this determines the bandwidth of both the shader excitation filter and the sinc windowed impulse // was 10000 for POMA
//****************************************
//****************************************




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
int period_size = 512;     	// period length in frames
int buffer_size = 1024;		    // ring buffer length in frames [contains period]
// only via command line
int formatInt;
int verbose;                    // verbose flag
int resample;      	            // enable alsa-lib resampling
int period_event;               // produce poll event after each period

int morehelp;
//----------------------------------------



// the next variable are changed in each preset as defined in defineDomain.cpp

//----------------------------------------
// simulation variables, some externs in defineDomain.cpp
//----------------------------------------
bool glottalLowPassToggle = true; // this acts only on the excitation filter implemented in the shader
bool filterToggle = true; // use any boundary filter at all?
bool radiation = false; // extern
int numAudioSamples = 22050;//26460;//65536; //2^16, that means 2^15 FFT samples // extern


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
excitation_model excitationModel = ex_noFeedback;
subGlotEx_type subGlotExType     = subGlotEx_sin;
float excitationLevel  = 1;	// maximum audio level that can be reached; once passed to the synth, can't be changed
float excitationVol    = 1; // volume allows to move in [0, excitationLevel], with nice INTERPOLATION [which might be a problem in frequency response simulations]
float maxPressure      = 2000; // as in Aerophones
float physicalParams[5] = {C_VT, RHO_VT, MU_VT, PRANDTL_VT, GAMMA_VT};
float a1               = 0.0001; // first area [m2] -> for excitation models
float alpha            = 0.001; //VIC be careful! this is modified in presets within defineDomain.cpp
//----------------------------------------

//----------------------------------------
// these are extern vars from areaFucntions.cpp
//----------------------------------------
extern float story08Data_a_formants[8];
extern float story08Data_i_formants[8];
extern float story08Data_u_formants[8];
//----------------------------------------



//----------------------------------------
// main variables, all externs, in controls.cpp and defineDomain.cpp
//----------------------------------------
AudioEngine audioEngine;
ArtVocSynth *artVocSynth;
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
void defineDomain(int simID, float ***&domainPixels, float ***&widthMapPixels) {
	switch(simID) {
		case sim_openSpaceEx:
			openSpaceExcitation(domainPixels, widthMapPixels);
			break;
		case sim_story08Vowel:
			story08Vowel(story_vowel, domainPixels, widthMapPixels);
			break;
		case sim_2TubesCentric_micA:
			twoTubesCentric(domainPixels, widthMapPixels, 0);
			break;
		case sim_2TubesCentric_micB:
			twoTubesCentric(domainPixels, widthMapPixels, 1);
			break;
		case sim_symmTest_completeSymmetry:
			symmTestTubes(symmTest_tube0, domainPixels, widthMapPixels);
			break;
		case sim_symmTest_story_xyzSymmetry:
			symmTestStoryVowel(story_vowel, symmTest_xyz, domainPixels, widthMapPixels);
			break;
		case sim_symmTest_story_yzSymmetry:
			symmTestStoryVowel(story_vowel, symmTest_yz, domainPixels, widthMapPixels);
			break;
		default:
			break;
	}
}

int initVocalSynth(float ***domainPixels, float ***widthMapPixels) {
	// time settings
	// use the ones coming from global initialization
	unsigned short period = period_size;
	int audio_rate        = rate;
	// period and rate are passed first to the synth [here] then to the audio engine [in initAudioThread()]

	string shaderLocation = getCurrentFilePath() + "/shaders_VocalTract_abs";

	// first check if fullscreen, must be done before init!
	artVocSynth->monitorSettings(fullscreen, monitorIndex);
	artVocSynth->init(shaderLocation, domainSize, audio_rate, rate_mul, period, widthMapPixels, openSpaceWidth,
					  excitationLevel, magnifier, listenerCoord, glottalLowPassFreq, true);
	ds = artVocSynth->getDS(); // update ds, can use to calculate domains if not done yet
	if(artVocSynth->setDomain(excitationCoord, excitationDim, domainPixels) != 0)
	 return 1;

	// apply settings
	//artVocSynth->setListenerPosition(listenerCoord[0], listenerCoord[1]); // this triggers crossfade, while if we init listener in init() it doesn't
	artVocSynth->setFeedbackPosition(feedbCoord[0], feedbCoord[1]);
	artVocSynth->setExcitationDirection(excitationAngle);
	artVocSynth->setPhysicalParameters( (void *)physicalParams);
	artVocSynth->setAbsorptionCoefficient(alpha);
	artVocSynth->setMaxPressure(maxPressure);
	artVocSynth->setExcitationVolume(excitationVol);
	artVocSynth->setFilterToggle(filterToggle);
	artVocSynth->setGlottalLowPassToggle(glottalLowPassToggle);

	artVocSynth->setA1(a1);

	if(impulseTest) {
		excitationModel = ex_noFeedback;
		subGlotExType = subGlotEx_windowedImp;
	}
	artVocSynth->setExcitationModel(excitationModel); // activate excitation type
	artVocSynth->setSubGlotExType(subGlotExType);

	return 0;
}

int initSimulation(int simID) {

	float ***domainPixels;
	float ***widthMapPixels;

	defineDomain(simID, domainPixels, widthMapPixels);

	int retval = initVocalSynth(domainPixels, widthMapPixels);

	// get rid of the pixels we dynamically allocated in defineDomain.ccp and copied in our synth already
	for(int i = 0; i <domainSize[0]; i++) {
		for(int j = 0; j <domainSize[1]; j++) {
			delete[] domainPixels[i][j];
			delete[] widthMapPixels[i][j];
		}
		delete[] domainPixels[i];
		delete[] widthMapPixels[i];
	}
	delete[] domainPixels;
	delete[] widthMapPixels;

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
				rate = rate > 196000 ? 196000 : rate; // check
				//audioEngine.setRate(rate); // done anyway in caller function
				break;
			case 'c':
				channels = atoi(optarg);
				channels = channels < 1 ? 1 : channels;
				channels = channels > 1024 ? 1024 : channels;
				//audioEngine.setChannelNum(channels);  // done anyway in caller function
				break;
			case 'b':
				buffer_size = atoi(optarg);
				//audioEngine.setBufferSize(buffer_size); // done anyway in caller function
				break;
			case 'p':
				period_size = atoi(optarg);
				//audioEngine.setPeriodSize(period_size); // done anyway in caller function
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
				//audioEngine.setAudioFormat(format); // done anyway in caller function
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

	pthread_join(audioThread, NULL);

	pthread_cancel(controlThread);
	return 0;
}

void runFreqResponse() {
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

	// name of geometry helps recognize experiment
	string geometryName = "";

	// choose formants to compare against [if any] and calculate frequency response!
	if(simulationID == sim_story08Vowel) {
		geometryName = string(1,story_vowel); // story vowel experiment
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
		calculateFreqResponse(numAudioSamples, period_size, rate_mul, geometryName, refFormants, numOfFormants);
	}
	else if(simulationID == sim_2TubesCentric_micA) {
		geometryName = "centricTubes_mA";
		calculateFreqResponse(numAudioSamples, period_size, rate_mul, geometryName); // no formants to compare against
	}
	else if(simulationID == sim_2TubesCentric_micB) {
		geometryName = "centricTubes_mB";
		calculateFreqResponse(numAudioSamples, period_size, rate_mul, geometryName); // no formants to compare against
	}
	else if(simulationID == sim_openSpaceEx) {
		geometryName = "openSpace";
		calculateFreqResponse(numAudioSamples, period_size, rate_mul, geometryName); // no formants to compare against
	}
	else if(simulationID == sim_symmTest_completeSymmetry) {
		geometryName = "symmetryTest_0";
		calculateFreqResponse(numAudioSamples, period_size, rate_mul, geometryName); // no formants to compare against
	}
	else if(simulationID ==  sim_symmTest_story_xyzSymmetry) {
			geometryName = string(1,story_vowel) + "_xyzSymmetry"; // story vowel experiment
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
			calculateFreqResponse(numAudioSamples, period_size, rate_mul, geometryName, refFormants, numOfFormants);
		}
	else if(simulationID ==  sim_symmTest_story_yzSymmetry) {
		geometryName = string(1,story_vowel) + "_yzSymmetry"; // story vowel experiment
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
		calculateFreqResponse(numAudioSamples, period_size, rate_mul, geometryName, refFormants, numOfFormants);
	}

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
		runFreqResponse();

	printf("Program ended\nBye bye_\n");
	return 0;
}
