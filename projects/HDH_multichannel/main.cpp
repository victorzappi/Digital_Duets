/*
 * HyperDrumnhead, a multi-percussive augmented instrument, based on 2D FDTD
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <iostream>
#include <string>
#include <vector>

// elapsed time
#include <sys/time.h>

// back end
#include "priority_utils.h"


// application stuff
#include "HyperDrumSynth.h"
#include "controls.h"
#include "HyperDrumheadEngine.h"


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
bool fullDuplex = false;
bool useTouchscreen = false;
bool useOsc = false;
bool useMidi = false;


//----------------------------------------
// domain variables, all externs in defineDomain.cpp and controls.cpp
//----------------------------------------
// 1920x1080
// {320-2, 180-2} x 6 = 56604 <---
// {240-2, 135-2} x 8 = 31654

// 1280x960
// {256-2 x 192-2} x 5 = 48260 <---
int domainSize[2] = {254, 190};
float magnifier   =	4;
int rate_mul      = 1;
bool fullscreen   = false;
int monitorIndex  = 0;
//----------------------------------------


//----------------------------------------
// control variables, all externs, in defineDomain.cpp, controls.cpp, other controls_xxx.cpp files and presetCycle.cpp
//----------------------------------------
int listenerCoord[2]   = {1+domainSize[0]/2, domainSize[1]/2};
int areaListenerCoord[AREA_NUM][2] = {-1, -1};
int excitationDim[2]   = {0, 0}; // no starting excitation
int excitationCoord[2] = {domainSize[0]/2, domainSize[1]/2};
int drumExId = 0;
float excitationLevel  = 0.17;//0.17;	// maximum audio level that can be reached; once passed to the synth, can't be changed
float excitationVol    = 1; // volume allows to move in [0, excitationLevel], with nice INTERPOLATION [which might be a problem in frequency response simulations]
double areaExcitationVol[AREA_NUM] = {1};
float areaDamp[AREA_NUM] = {0.001};
float areaProp[AREA_NUM] = {0.5}; // will call this rho, it is related to CFL num...as follows
double areaExLowPassFreq[AREA_NUM] = {19000.0};
double areaExRelease[AREA_NUM] = {0.5};
bool render             = true;
bool builtinPreset      = false;
string touchScreenName = "DISPLAX SKIN ULTRA"; // instead of xinput -list (: [a portion of the name is fine too!]

int startPresetIdx = -1;
bool usePresetCycler = false;
int presetCyclerInteval_s = 60;
vector<vector<int>> presetCyclerGroups = {
	{0, 1, 2, 3, 4, 5},
	{6, 7, 8, 9, 10, 11},
	{12, 13, 14, 15, 16, 17},
	{18, 19, 20, 21, 22, 23},
	{24, 25, 26, 27, 28, 29},
	{30, 31, 32, 33, 34, 35},

	{36, 37, 38, 39, 40, 41},
	{42, 43, 44, 45, 46, 47},
	{48, 49, 50, 51, 52, 53},
	{54, 55, 56, 57, 58, 59},
	{60, 61, 62, 63, 64, 65},
	{66, 67, 68, 69, 70, 71},

	{72, 73, 74, 75, 76},
	{77, 78, 79, 80, 81},
	{82, 83, 84, 85, 86},
	{87, 88, 89, 90, 91},
	{92, 93, 94, 95, 96},
	{97, 98, 99, 100, 101},

	{102}
};
// one value per each group ['row'] in presetCyclerGroups, making sure they add up to 1
vector<float> presetCyclerProbabilities = {
	0.065,
	0.065,
	0.065,
	0.065,
	0.065,
	0.065,

	0.065,
	0.065,
	0.065,
	0.065,
	0.065,
	0.065,

	0.0352,
	0.0352,
	0.0352,
	0.0352,
	0.0352,
	0.0352,

	0.0088
};





// in the propagation equation rho is a factor that scales the effect of neighbors [have a look at the shader in fbo_fs.glsl]:
// rho = (c * (dt/ds))^2 where c is the speed of sound in the medium
// dt is fixed
// dt = 1/fs
// but we can change c and ds, to describe different materials and sizes!
// c and ds are lumped together, in rho
// because their effects are not independent in reality!
// as in the simulation (:
// let me show it!
// we know that according to CFL condition stability is obtained for
// dt <= ds/(c*sqrt(2))
// or
// c/ds <= 1/dt * 1/sqrt(2)
// if we choose the biggest value possible
// (c/ds)_max = val_max = 1/dt * 1/sqrt(2)
// rho will be
// rho = (val_max * (dt/ds))^2 = (ds/dt * 1/sqrt(2) * (dt/ds))^2 = 1/2 = 0.5
// and for any other smaller values of rho the simulation is stable
// but
// more importantly for us
// since dt is fixed
// rho = 0.5 can be obtained with an infinite number of different c and ds values
// e.g.
// c = 343 m/s [air] and ds = 7.7 mm
// c = 4540 m/s [gold!] and ds = 0.1029478458 m
// variations on medium and size will give us same [but opposite] results (:
// that's why we can lump them in rho!
// just pay attention to CFL condition
//----------------------------------------


//----------------------------------------
// audio variables
//----------------------------------------
string playbackCardName = "";//@VIC "Scarlett 18i8 USB";
snd_pcm_format_t playbackFormat = SND_PCM_FORMAT_S32;    	// sample playbackFormat
unsigned int playbackChannels = 2;//@VIC 8;	    // count of playbackChannels
string captureCardName = "";//@VIC "Scarlett 18i8 USB";
snd_pcm_format_t captureFormat = SND_PCM_FORMAT_S32;    	// sample captureFormat
unsigned int captureChannels = 1;//@VIC 10;	    // count of playbackChannels
unsigned int rate = 44100;      // stream rate, extern
int period_size = 512;//128;     		// period length in frames
int buffer_size = 2*period_size;		    // ring buffer length in frames [contains period]

string device = ""; //"hw:1,0"; // playback device, use only if wanna specify both card and device




//----------------------------------------
// location variables
//----------------------------------------
string exciteFileFolder = getCurrentFilePath() + "/audiofiles";//"audiofiles_oren";//"audiofiles";
string presetFileFolder = getCurrentFilePath() + "/presets";//"presets_oren";//"presets";



//----------------------------------------
// application channels
//----------------------------------------
unsigned short exInChannels = 2; 
unsigned short exInChnOffset = 0;
unsigned short hdhInChannels = 2; // same as nume of excitations, one per each hdh input channel
unsigned short hdhdOutChannels = 2;
unsigned short hdhOutChnOffset= 0; //@VIC 2;


//****************************************
//****************************************




//----------------------------------------
// main variables, all externs in controls.cpp and defineDomain.cpp
//----------------------------------------
HDH_AudioEngine audioEngine;
HyperDrumSynth *hyperDrumSynth;
//----------------------------------------



//----------------------------------------
// audio variables
//----------------------------------------
audioThread_data at_data;

// only via command line
int method;						// alsa low level buffer transfer method
int formatInt;
int verbose;                    // verbose flag
int resample;      	            // enable alsa-lib resampling
int period_event;               // produce poll event after each period



//----------------------------------------
// sync and help variables
//----------------------------------------
// extern in controls.cpp
bool drumSynthInited = false;
unsigned short excitationChannels = hdhInChannels;




/*struct option long_option[] = {
	{"help", 0, NULL, 'h'},
	{"device", 1, NULL, 'D'},
	{"rate", 1, NULL, 'r'},
	{"playbackChannels", 1, NULL, 'c'},
	{"buffer", 1, NULL, 'b'},
	{"period", 1, NULL, 'p'},
	{"method", 1, NULL, 'm'},
	{"playbackFormat", 1, NULL, 'o'},
	{"verbose", 1, NULL, 'v'},
	{"noresample", 1, NULL, 'n'},
	{"pevent", 1, NULL, 'e'},
	{NULL, 0, NULL, 0},
};*/

int morehelp;
//----------------------------------------


static void help(void) {
	int k;
	printf(
			"Usage: pcm [OPTION]... [FILE]...\n"
			"***PLEASE DO NOT USE LONG OPTIONS!***\n"
			"-h,--help      help\n"
			"-C,--cardname    audio card name [device set to 0]-> check with aplay -L, a portion of the name is enough\n"
			"-d,--device    playback device -> use only of you wanna choose both card and device\n"
			"-r,--rate      stream rate in Hz\n"
			"-c,--playbackChannels  count of playbackChannels in stream\n"
			"-b,--buffer    ring buffer size in frames\n"
			"-p,--period    period size in frames\n"
			//"-m,--method    transfer method\n"
			"-o,--playbackFormat    sample playbackFormat\n"
			"-v,--verbose   show the PCM setup parameters\n"
			"-n,--noresample  do not resample\n"
			"-e,--pevent    enable poll event after each period\n"
			"-s, 		x [and y] size of domain\n"
			"-M, 		GPU window magnifier value [floating point]\n"
			"-P,--preset index of preset to load\n"
			"-4, 		built-in preset: split domain in 4 different areas\n"
			"-S, 		touch screen device name [default is DISPLAX SKIN ULTRA] -> check with evtest, a portion of the name is enough\n"
			"\n");
	printf("Recognized sample formats are:");
	for (k = 0; k < SND_PCM_FORMAT_LAST; ++k) {
		const char *s = snd_pcm_format_name( (snd_pcm_format_t) k);
		if (s)
			printf(" %s", s);
	}
	printf("\n");
/*	printf("Recognized transfer methods are:");
	for (k = 0; audioEngine.transfer_methods[k].name; k++)
		printf(" %s", audioEngine.transfer_methods[k].name);
	printf("\n");*/
}




int initDrumSynth(float ***domainPixels) {
	string shaderLocation = getCurrentFilePath() + "/shaders_HDH_multichannel";
	
	// first check if fullscreen, must be done before init!
	hyperDrumSynth->monitorSettings(fullscreen, monitorIndex);

	hyperDrumSynth->init(shaderLocation, domainSize, rate, rate_mul, period_size, excitationLevel, magnifier, exciteFileFolder, listenerCoord, 
						 exInChannels, exInChnOffset, hdhInChannels, hdhdOutChannels, hdhOutChnOffset);
	if(hyperDrumSynth->setDomain(excitationCoord, excitationDim, domainPixels, builtinPreset) != 0)
		return 1;

	hyperDrumSynth->hideListener();

	// apply settings
	for(int i=0; i<AREA_NUM; i++) {
		// init arrays...
		areaListenerCoord[i][0] = -1;
		areaListenerCoord[i][1] = -1;
		areaExcitationVol[i]    = areaExcitationVol[0];
		areaDamp[i]       = areaDamp[0];
		areaProp[i] = areaProp[0];
		areaExLowPassFreq[i]    = areaExLowPassFreq[0];
		areaExRelease[i]        = areaExRelease[0];
		// for(int k=0; k<hdhdOutChannels; k++)
		// 	hyperDrumSynth->hideAreaListener(i, k);
		hyperDrumSynth->setAreaExcitationVolume(i, areaExcitationVol[i]);
		hyperDrumSynth->setAreaDampingFactor(i, areaDamp[i]);
		hyperDrumSynth->setAreaPropagationFactor(i, areaProp[i]);
		hyperDrumSynth->setAreaExLowPassFreq(i, areaExLowPassFreq[i]);
		hyperDrumSynth->setAreaExcitationID(i, drumExId);
		hyperDrumSynth->setAreaExcitationRelease(i, areaExRelease[i]);
	}

	// these need to be initialized manually so that control.cpp knows where the first listener is located
	areaListenerCoord[0][0] = listenerCoord[0];
	areaListenerCoord[0][1] = listenerCoord[1];

	return 0;
}

// domainPixels passed by reference since dynamically allocated in the function
void initDomainPixels(float ***&domainPixels) {
	//---------------------------------------------
	// all is in domain frame of ref, which means no PML nor dead layers included
	//---------------------------------------------

	// domain pixels [aka walls!]
	domainPixels = new float **[domainSize[0]];
	float airVal = HyperDrumhead::getPixelAlphaValue(HyperDrumhead::cell_air);
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

	int retval = initDrumSynth(domainPixels);
	drumSynthInited = true;

	// get rid of the pixels we dynamically allocated in defineDomain.cpp and copied in our synth already
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
	while (1) {
		int c;
		if ((c = getopt(at_data.argc, at_data.argv, "hD:d:C:r:c:f:b:p:m:o:s:vneM:qP:4S:")) < 0)
			break;
		switch (c) {
			case 'h':
				morehelp++;
				break;
			case 'd':
				device = strdup(optarg);
				break;
			case 'C':
				playbackCardName.assign(optarg);
				break;
			case 'r':
				rate = atoi(optarg);
				rate = rate < 4000 ? 4000 : rate;
				rate = rate > 196000 ? 196000 : rate; //check.......
				break;
			case 'c':
				playbackChannels = atoi(optarg);
				playbackChannels = playbackChannels < 1 ? 1 : playbackChannels;
				playbackChannels = playbackChannels > 1024 ? 1024 : playbackChannels;
				break;
			case 'b':
				buffer_size = atoi(optarg);
				break;
			case 'p':
				period_size = atoi(optarg);
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
					playbackFormat = static_cast<snd_pcm_format_t>(formatInt);
					const char *format_name = snd_pcm_format_name( (snd_pcm_format_t) playbackFormat);
					if (format_name)
						if (!strcasecmp(format_name, optarg))
							break;
				}
				if (playbackFormat == SND_PCM_FORMAT_LAST)
						playbackFormat = SND_PCM_FORMAT_S16;
				if (!snd_pcm_format_linear(playbackFormat) &&
					!(playbackFormat == SND_PCM_FORMAT_FLOAT_LE || playbackFormat == SND_PCM_FORMAT_FLOAT_BE)) {
					printf("Invalid (non-linear/float) playbackFormat %s\n", optarg);
					pthread_exit(NULL);
				}
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
			case 's':
				domainSize[0] = atoi(optarg);
				domainSize[1] = domainSize[0];
				listenerCoord[0] = (domainSize[0]/2)+3;
				listenerCoord[1] = domainSize[1]/2;
				excitationCoord[0] = domainSize[0]/2;
				excitationCoord[1] = domainSize[1]/2;
				break;
			case 'M':
				magnifier = atof(optarg);
				break;
			case 'q':
				render = false;
				break;
			case 'P':
				startPresetIdx = atoi(optarg);
				builtinPreset = false; // exclusive
				break;
			case '4':
				builtinPreset = true;
				excitationCoord[0] += 30;
				excitationCoord[1] += 10;
				areaListenerCoord[0][0] = excitationCoord[0] + 10;
				areaListenerCoord[0][1] = excitationCoord[1];
				startPresetIdx = -1; // exclusive
				break;
			case 'S':
				touchScreenName = string(optarg);
				break;
		}
	}

	if (morehelp) {
		help();
		return 1;
	}
	return 0;
}

int initAudioThread() {
	if(verbose==1)
		cout << "---------------->Init Audio Thread" << endl;

	// put here so that we can change rate before initializing audio engine ---> overwrites command line arguments
	if(initSimulation()!= 0)
		return -1;

	// set audio parameters
	if(device=="") {
		audioEngine.setPlaybackCardName(playbackCardName);
		audioEngine.setCaptureCardName(captureCardName);
	}
	else {
		audioEngine.setPlaybackDevice(device.c_str());
		audioEngine.setCaptureDevice(device.c_str());
	}

	audioEngine.setFullDuplex(fullDuplex);
	audioEngine.setPlaybackChannelNum(playbackChannels);
	audioEngine.setPlaybackAudioFormat(playbackFormat);
	audioEngine.setCaptureChannelNum(captureChannels);
	audioEngine.setCaptureAudioFormat(captureFormat);
	audioEngine.setPeriodSize(period_size);
	audioEngine.setBufferSize(buffer_size);
	audioEngine.setRate(rate);

	// last touch
	audioEngine.initEngine();
	audioEngine.addAudioModule(hyperDrumSynth);

	return 0;
}

void *runAudioEngine(void *) {
	if(initAudioThread()>0)
		return (void *)0;

	if(verbose==1)
		cout << "__________set Audio Thread priority" << endl;

	// set thread niceness
 	set_niceness(-20, verbose); // even if it should be inherited from calling process...but it's not

	// set thread priority
	set_priority(0, verbose);

	if(verbose==1)
		cout << "_________________Audio Thread!" << endl;

	audioEngine.startEngine();

	if(hyperDrumSynth!=NULL)
		delete hyperDrumSynth;

	cout << "audio thread ended" << endl;

	return (void *)0;
}

int runRealTimeThreads() {
	pthread_t audioThread;
	pthread_t controlThread;
	if(verbose==1)
		cout << "main(): creating audio thread" << endl;

	if( pthread_create(&audioThread, NULL, runAudioEngine, NULL) ) {
	  cout << "Error:unable to create thread" << endl;
	  return -1;
	}

	if(verbose==1)
		cout << "main(): creating control thread" << endl;

	if( pthread_create(&controlThread, NULL, runControlLoop, NULL) ) {
	  cout << "Error:unable to create thread" << endl;
	  return -1;
	}

	pthread_join(audioThread, NULL); // control thread will stop itself and audio thread too, via esc keyboard button

	return 0;
}

int main(int argc, char *argv[]) {
	at_data.argc = argc;
	at_data.argv = argv;


	// read settings from command line arguments
	if(parseArguments()!=0)
		return -1;

	// set niceness of calling process
	set_niceness(-20); // theoretically POSIX standards grant niceness is shared among all called threads...this rule is broken for now on linux...but still

	hyperDrumSynth = new HyperDrumSynth(); // needs to be created for both threads' sake

	if(!render)
		hyperDrumSynth->disableRender();


	if(runRealTimeThreads()<0)
		exit(-1);


	printf("Program ended\nBye bye_\n");
	return 0;
}
