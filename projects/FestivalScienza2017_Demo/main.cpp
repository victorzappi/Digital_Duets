/*
 * HyperDrumnhead, a multi-percussive augmented instrument, based on 2D FDTD
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
#include "HDH_AudioEngine.h"
#include "priority_utils.h"


// application stuff
#include "HyperDrumSynth.h"
#include "controls.h"


#define INPUT_NUM 10+1 //VIC: [bad] hardcoded value from USBest Technology SiS touch screen [+1 is for mouse "touch"]


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
bool multitouch = true;

//----------------------------------------
// domain variables, all externs in defineDomain.cpp and controls.cpp
//----------------------------------------
int domainSize[2] = {158, 88};//{94, 52};//m20 //{126, 70};//m15 //{158, 88};//m12 //{190, 106};//m10  //{238, 133};//m8
float magnifier   = 12;
int rate_mul      = 1;
bool fullscreen   = true;
int monitorIndex  = 0; // used only if fullscreen
//----------------------------------------


//----------------------------------------
// control variables, all externs, in controls.cpp and defineDomain.cpp
//----------------------------------------
int listenerCoord[2]   = {5+domainSize[0]/2, domainSize[1]/2};
int areaListenerCoord[AREA_NUM][2] = {-1, -1};
int excitationDim[2]   = {0, 0};
int excitationCoord[2] = {domainSize[0]/2, domainSize[1]/2};
drumEx_type drumExType = drumEx_sin;
float excitationLevel  = 1;	// maximum audio level that can be reached; once passed to the synth, can't be changed
float excitationVol    = 1; // volume allows to move in [0, excitationLevel], with nice INTERPOLATION [which might be a problem in frequency response simulations]
double areaExcitationVol[INPUT_NUM] = {0.3};
float areaDampFactor[AREA_NUM] = {0.03};
float areaPropagationFactor[AREA_NUM] = {0.5}; // will call this rho, it is related to CFL num...as follows
double areaExLowPassFreq[INPUT_NUM] = {19000.0};
double areaExRelease[INPUT_NUM] = {0.1};
bool render            = true;
bool useGPU            = true;
bool preset   		   = false;
string touchScreenName = "USBest Technology SiS"; // instead of xinput -list (: [a portion of the name is fine too!]

int	numOfInputs = INPUT_NUM;


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
string device = "hw:0,0";       // playback device
snd_pcm_format_t format = SND_PCM_FORMAT_S32;    	// sample format
unsigned int rate = 44100;      // stream rate, extern
unsigned int channels = 2;	    // count of channels
int period_size = 128;     		// period length in frames
int buffer_size = 256;		    // ring buffer length in frames [contains period]


//****************************************
//****************************************





//----------------------------------------
// main variables, all externs, in controls.cpp and defineDomain.cpp
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

/*struct option long_option[] = {
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


static void help(void) {
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
			//"-m,--method    transfer method\n"
			"-o,--format    sample format\n"
			"-v,--verbose   show the PCM setup parameters\n"
			"-n,--noresample  do not resample\n"
			"-e,--pevent    enable poll event after each period\n"
			"-s, 		x [and y] size of domain\n"
			"-C, 		use CPU synthesis (as opposed to GPU)\n"
			"-M, 		GPU window magnifier value [floating point]\n"
			"-q, 		disable render to screen\n"
			"-4, 		preset: split domain in 4 different areas\n"
			"-S, 		touch screen device name [default is DISPLAX SKIN ULTRA] -> check with evtest, a portion of the name is enough\n"
			"-I, 		touch screen mouse ID [default is 10]-> to disable mouse touch control\n"
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
	string shaderLocation = getCurrentFilePath() + "/shaders_HyperDrumhead";

	// first check if fullscreen, must be done before init!
	hyperDrumSynth->monitorSettings(fullscreen, monitorIndex);

	// init
	hyperDrumSynth->init(shaderLocation, domainSize, rate, rate_mul, period_size, excitationLevel, magnifier, listenerCoord);
	if(hyperDrumSynth->setDomain(excitationCoord, excitationDim, domainPixels, preset) != 0)
	 return 1;

	hyperDrumSynth->hideListener();

	// apply settings
	for(int i=0; i<AREA_NUM; i++) {
		// init arrays...
		areaListenerCoord[i][0] = -1;
		areaListenerCoord[i][1] = -1;
		areaDampFactor[i]       = areaDampFactor[0];
		areaPropagationFactor[i] = areaPropagationFactor[0];

		//hyperDrumSynth->setAreaListenerPosition(i, areaListenerCoord[i][0], areaListenerCoord[i][1]);
		hyperDrumSynth->hideAreaListener(i);
		hyperDrumSynth->setAreaDampingFactor(i, areaDampFactor[i]);
		hyperDrumSynth->setAreaPropagationFactor(i, areaPropagationFactor[i]);
	}

	for(int i=0; i<numOfInputs; i++) {
		areaExcitationVol[i] = areaExcitationVol[0];
		areaExLowPassFreq[i] = areaExLowPassFreq[0];
		areaExRelease[i]     = areaExRelease[0];

		hyperDrumSynth->setAreaExcitationVolume(i, areaExcitationVol[i]);
		hyperDrumSynth->setAreaExLowPassFreq(i, areaExLowPassFreq[i]);
		hyperDrumSynth->setAreaExcitationType(i, drumExType);
		hyperDrumSynth->setAreaExcitationRelease(i, areaExRelease[i]);
	}

	hyperDrumSynth->setExcitationVolume(excitationVol);
	hyperDrumSynth->setExcitationType(drumExType);

	areaListenerCoord[0][0] = 5+domainSize[0]/2;
	areaListenerCoord[0][1] = domainSize[1]/2;

	hyperDrumSynth->setAreaListenerPosition(0, areaListenerCoord[0][0], areaListenerCoord[0][1]);
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
	while (1) {
		int c;
		if ((c = getopt(at_data.argc, at_data.argv, "hD:d:r:c:f:b:p:m:o:s:vneCM:q4S:")) < 0)
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
			case 's':
				domainSize[0] = atoi(optarg);
				domainSize[1] = domainSize[0];
				listenerCoord[0] = (domainSize[0]/2)+3;
				listenerCoord[1] = domainSize[1]/2;
				excitationCoord[0] = domainSize[0]/2;
				excitationCoord[1] = domainSize[1]/2;
				break;
			case 'C':
				useGPU = false;
				break;
			case 'M':
				magnifier = atof(optarg);
				break;
			case 'q':
				render = false;
				break;
			case '4':
				preset = true;
				excitationCoord[0] += 30;
				excitationCoord[1] += 10;
				areaListenerCoord[0][0] = excitationCoord[0] + 10;
				areaListenerCoord[0][1] = excitationCoord[1];
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
	audioEngine.setPlaybackDevice(device.c_str());
	audioEngine.setPlaybackChannelNum(channels);
	audioEngine.setPlaybackAudioFormat(format);
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
 	//set_niceness(-20, verbose); // even if it should be inherited from calling process...but it's not

	// set thread priority
	//set_priority(10, verbose);

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

	pthread_join(audioThread, NULL);

	pthread_cancel(controlThread);

	if(multitouch)
		cleanUpControlLoop();

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


