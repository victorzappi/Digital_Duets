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
#include "Engine.h"
#include "priority_utils.h"


// application stuff
#include "HyperDrumSynth.h"
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
bool multitouch = true;

//----------------------------------------
// domain variables, all externs in defineDomain.cpp
//----------------------------------------
int domainSize[2] = {158, 88};//m12 //{126, 70};//m15
float magnifier   = 12;
int rate_mul      = 1;
bool fullscreen   = true;
int monitorIndex  = 0; // used only if fullscreen
//----------------------------------------


//----------------------------------------
// control variables, all externs, in controls.cpp and defineDomain.cpp
//----------------------------------------
int listenerCoord[2]   = {5+domainSize[0]/2, domainSize[1]/2};
int areaListenerCoord[10][2] = {{5+domainSize[0]/2, domainSize[1]/2}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}};
int excitationDim[2]   = {1, 1};
int excitationCoord[2] = {domainSize[0]/2, domainSize[1]/2};
drumEx_type drumExType = drumEx_windowedImp;
float excitationLevel  = 1;	// maximum audio level that can be reached; once passed to the synth, can't be changed
float excitationVol    = 1; // volume allows to move in [0, excitationLevel], with nice INTERPOLATION [which might be a problem in frequency response simulations]
float areaExcitationVol[10] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
float areaDampFactor[10] = {0.001, 0.001, 0.001, 0.001, 0.001, 0.001, 0.001, 0.001, 0.001, 0.001};
float areaPropagationFactor[10] = {0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5}; // will call this rho, it is related to CFL num...as follows
float clampFactor      = 1;
float areaExLowPassFreq[10] = {19000.0, 19000.0, 19000.0, 19000.0, 19000.0, 19000.0, 19000.0, 19000.0, 19000.0, 19000.0};
bool render            = true;
bool useGPU            = true;
bool timeTest          = false;
bool readTimeTest      = false;
bool timeTestNoRead    = false;
int numOfTestRuns      = 1;
bool preset   		   = false;
int touchScreenEvent   = 9;
int touchScreenMouseID = 9;


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
string device = "hw:0,0";       // playback device ["hw:2,0" = usb card]
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
Engine audioEngine;
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

int runTest(); // here simply to define time test functions at the bottom of the file...


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
			"-t, 		number of test runs [5 1-sec excitations each]-> enables time test\n"
			"-R, 		number of test runs [5 1-sec excitations each]-> enables read time test\n"
			"-T, 		number of test runs [5 1-sec excitations each]-> enables time test without reading back samples\n"
			"-q, 		disable render to screen\n"
			"-4, 		preset: split domain in 4 different areas\n"
			"-E, 		touch screen event number [default is 10]-> to catch touch events\n"
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
	hyperDrumSynth->setFullscreen(fullscreen, monitorIndex);

	// init
	hyperDrumSynth->init(shaderLocation, domainSize, rate, rate_mul, period_size, excitationLevel, magnifier, listenerCoord);
	if(hyperDrumSynth->setDomain(excitationCoord, excitationDim, domainPixels, preset) != 0)
	 return 1;

	hyperDrumSynth->hideListener();

	// apply settings
	for(int i=0; i<AREA_NUM; i++) {
		//hyperDrumSynth->setAreaListenerPosition(i, areaListenerCoord[i][0], areaListenerCoord[i][1]);
		hyperDrumSynth->hideAreaListener(i);
		hyperDrumSynth->setAreaExcitationVolume(i, areaExcitationVol[i]);
		hyperDrumSynth->setAreaDampingFactor(i, areaDampFactor[i]);
		hyperDrumSynth->setAreaPropagationFactor(i, areaPropagationFactor[i]);
		hyperDrumSynth->setAreaExLowPassFreq(i, areaExLowPassFreq[i]);
		hyperDrumSynth->setAreaExcitationType(i, drumExType);
	}
	hyperDrumSynth->setExcitationVolume(excitationVol);
	hyperDrumSynth->setExcitationType(drumExType);

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
		if ((c = getopt(at_data.argc, at_data.argv, "hD:d:r:c:f:b:p:m:o:s:vneCM:qt:R:T:4E:I:")) < 0)
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
			case 't':
				timeTest = true;
				numOfTestRuns = atoi(optarg);

				readTimeTest = false;
				timeTestNoRead = false;
				break;
			case 'R':
				readTimeTest = true;
				numOfTestRuns = atoi(optarg);

				timeTest = false;
				timeTestNoRead = false;
				break;
			case 'T':
				timeTestNoRead = true;
				numOfTestRuns = atoi(optarg);

				timeTest = false;
				timeTest = false;
				break;
			case '4':
				preset = true;
				excitationCoord[0] += 30;
				excitationCoord[1] += 10;
				areaListenerCoord[0][0] = excitationCoord[0] + 10;
				areaListenerCoord[0][1] = excitationCoord[1];
				break;
			case 'E':
				touchScreenEvent = atoi(optarg);
				break;
			case 'I':
				touchScreenMouseID = atoi(optarg);
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
	audioEngine.setDevice(device.c_str());
	audioEngine.setChannelNum(channels);
	audioEngine.setAudioFormat(format);
	audioEngine.setPeriodSize(period_size);
	audioEngine.setBufferSize(buffer_size);
	audioEngine.setRate(rate);

	// last touch
	audioEngine.initEngine();
	audioEngine.addGenerator(hyperDrumSynth);

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

	if(!timeTest && !readTimeTest && !timeTestNoRead) {
		if(runRealTimeThreads()<0)
			exit(-1);
	}
	else
		runTest();

	printf("Program ended\nBye bye_\n");
	return 0;
}




//--------------------------------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------------------------------
void test() {
	const long double testDuration = 5; // in sec
	const long double hitInterval = 1; // in sec

	// all in sec
	const long double sampleDuration = 1.0/(rate*rate_mul);
	const long double periodDuration = sampleDuration * period_size;

	const int numOfCycles = round(testDuration/periodDuration);

	int hitDeltaCycles = hitInterval*(rate*rate_mul)/period_size;
	int hitCnt = 0;


	printf("\n");
	if(timeTest) {
		if(useGPU)
			printf("***GPU TIME TEST***\n");
		else
			printf("***CPU TIME TEST***\n");
		printf("\n");

		printf("\tTest duration info\n");
		printf("\tRun duration: %Lf [ms]\n", testDuration*1000.0);
		printf("\tHit interval: %Lf [ms]\n", hitInterval*1000.0);
		printf("\tNumber of runs: %d\n", numOfTestRuns);
		printf("\n");
		printf("\tSample rate [and multiplier]: %d [%d]\n", rate, rate_mul);
		printf("\tPeriod size: %d\n", period_size);
		printf("\tBuffer size: %d\n", buffer_size);
		printf("\tNumber of cycles: %d\n", numOfCycles);

		// working vars
		int elapsedTime = 0;
		timeval start_;
		timeval end_;

		for(int n=0; n<numOfTestRuns; n++) {
			hitCnt = 0;
			gettimeofday(&start_, NULL);
			for(int i=0; i<numOfCycles; i++) {
				if(hitCnt++ == hitDeltaCycles) {
					hitCnt = 0;
					hyperDrumSynth->setExcitationType(drumEx_windowedImp);
				}
				hyperDrumSynth->getBufferFloat(period_size);
			}
			gettimeofday(&end_, NULL);
			elapsedTime += ( (end_.tv_sec*1000000+end_.tv_usec) - (start_.tv_sec*1000000+start_.tv_usec) );
		}

		printf("\n");
		printf("\tExecution time: %f [ms] averaged over %d runs\n", (elapsedTime/numOfTestRuns)/1000.0, numOfTestRuns);
	}
	else if(readTimeTest){
		if(useGPU)
			printf("***GPU READ TIME TEST***\n");
		else
			printf("***CPU READ TIME TEST [meeehhh]***\n");
		printf("\n");
		printf("\tTest duration info\n");
		printf("\tRun duration: %Lf [ms]\n", testDuration*1000.0);
		printf("\tHit interval: %Lf [ms]\n", hitInterval*1000.0);
		printf("\tNumber of runs: %d\n", numOfTestRuns);

		printf("\n");
		printf("\tSample rate [and multiplier]: %d [%d]\n", rate, rate_mul);
		printf("\tPeriod size: %d\n", period_size);
		printf("\tBuffer size: %d\n", buffer_size);
		printf("\tNumber of cycles: %d\n", numOfCycles);

		// working var
		int elapsedTime = 0;
		int readTime = 0;

		for(int n=0; n<numOfTestRuns; n++) {
			hitCnt = 0;
			for(int i=0; i<numOfCycles; i++) {
				if(hitCnt++ == hitDeltaCycles) {
					hitCnt = 0;
					hyperDrumSynth->setExcitationType(drumEx_windowedImp);
				}
				hyperDrumSynth->getBufferFloatReadTime(period_size, readTime);
				elapsedTime += readTime;
			}
		}
		printf("\n");
		printf("\tRead time: %f [ms] averaged over %d runs\n", (elapsedTime/numOfTestRuns)/1000.0, numOfTestRuns);
	}
	else if(timeTestNoRead){
		if(useGPU)
			printf("***GPU TIME TEST [no sample readback]***\n");
		else
			printf("***CPU TIME TEST [no sample readback...meeehhh]***\n");
		printf("\n");

		printf("\tTest duration info\n");
		printf("\tRun duration: %Lf [ms]\n", testDuration*1000.0);
		printf("\tHit interval: %Lf [ms]\n", hitInterval*1000.0);
		printf("\tNumber of runs: %d\n", numOfTestRuns);
		printf("\n");
		printf("\tSample rate [and multiplier]: %d [%d]\n", rate, rate_mul);
		printf("\tPeriod size: %d\n", period_size);
		printf("\tBuffer size: %d\n", buffer_size);
		printf("\tNumber of cycles: %d\n", numOfCycles);

		// working vars
		int elapsedTime = 0;
		timeval start_;
		timeval end_;

		for(int n=0; n<numOfTestRuns; n++) {
			hitCnt = 0;
			gettimeofday(&start_, NULL);
			if(hitCnt++ == hitDeltaCycles) {
				hitCnt = 0;
				hyperDrumSynth->setExcitationType(drumEx_windowedImp);
			}
			for(int i=0; i<numOfCycles; i++)
				hyperDrumSynth->getBufferFloatNoSampleRead(period_size);
			gettimeofday(&end_, NULL);
			elapsedTime = ( (end_.tv_sec*1000000+end_.tv_usec) - (start_.tv_sec*1000000+start_.tv_usec) );
		}

		printf("\n");
		printf("\tExecution time: %f [ms] averaged over %d runs\n", (elapsedTime/numOfTestRuns)/1000.0, numOfTestRuns);
	}

	printf("\n");
}

void *runTestRoutine(void *) {
	if(verbose==1)
		cout << "__________set Test Thread priority" << endl;

	// put here so that we can change rate before initializing audio engine ---> overwrites command line arguments
	if(initSimulation()!= 0)
		return (void *)-1;

	// set thread niceness
 	set_niceness(-20, verbose); // even if it should be inherited from calling process...but it's not

	// set thread priority
	set_priority(0, verbose);

	if(verbose==1)
		cout << "_________________Test Thread!" << endl;

	test();

	cout << "test thread ended" << endl;

	return (void *)0;
}

int runTest() {
	pthread_t testThread;
	if( pthread_create(&testThread, NULL, runTestRoutine, NULL) ) {
	  cout << "Error:unable to create thread" << endl;
	  return -1;
	}
	pthread_join(testThread, NULL);
	return 0;
}
//--------------------------------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------------------------------
