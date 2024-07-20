/*
 * HyperDrumExcitation.cpp
 *
 *  Created on: Feb 12, 2018
 *      Author: vic
 */

#include "HyperDrumExcitation.h"
#include "strnatcmp.h" // for natural sorting

#include <dirent.h>  // to handle files in dirs
#include <algorithm> // sort

HyperDrumExcitation::HyperDrumExcitation(string folder, float maxLevel, unsigned int audio_rate, unsigned int periodSize, unsigned int simulationRate) {
	AudioGeneratorInOut::init(periodSize);

	rate 				= audio_rate;
	simulation_rate 	= simulationRate;
	volume = 1;
	volumeInterp[0]	= 1;
	volumeInterp[1] = 0;

	currentExcitationId = 0;

	antiAliasing = (rate != simulation_rate);
	freq = 440;
	lowPassFreq = ( ((double)rate)/2.0 )/simulation_rate;

	audiofiles = NULL;
	audioModulesOut = NULL;
	componentIsPercussive = NULL;

	listFilesInFolder(folder);

	numOfWaveforms = 4;
	numOfInputs    = 4;

	numOfComponents =  numOfWaveforms + numOfFiles + numOfInputs; // impulse, sine, square, noise + files + 4 passthroughs for 4 input channels

	audiofiles 		 = new AudioFile[numOfFiles];
	audioModulesOut   = new AudioModuleOut*[numOfComponents];
	audioModulesInOut = new AudioModuleInOut*[MAX_NUM_OF_INPUTS];
	componentIsPercussive = new bool[numOfComponents];
	componentIsFileStream = new bool[numOfComponents];




	//----------------------------------------------------------------------------------------------

	vector<bool> fileIsPercussive;
	vector<bool> fileIsStream;
	// audio files
	printf("\n");
	for(int i=0; i<numOfFiles; i++) {
		// need full path
		string fileNameAndPath = folder+"/"+fileNames[i];

		printf("Loading file %s", fileNameAndPath.c_str());

		// select channel from name
		size_t chL = fileNames[i].find(string("_L_"));
		size_t chR = fileNames[i].find(string("_R_"));
		if(chL!=string::npos) {
			audiofiles[i].init(fileNameAndPath, simulation_rate, periodSize, maxLevel, 0);
			printf(", left channel only");
		}
		else if(chR!=string::npos) {
			audiofiles[i].init(fileNameAndPath, simulation_rate, periodSize, maxLevel, 1);
			printf(", right channel only");
		}
		else {
			audiofiles[i].init(fileNameAndPath, simulation_rate, periodSize, maxLevel);
			printf(", stereo mix down");
		}

		// is file percussive?
		size_t prc = fileNames[i].find(string("_p_"));
		if(prc!=string::npos)
			fileIsPercussive.push_back(true);
		else
			fileIsPercussive.push_back(false);

		// is file a stream? i.e., it will be retriggered only when new preset is loaded [all waveforms are automatically triggered at init]
		size_t str = fileNames[i].find(string("_str_"));
		if(str!=string::npos)
			fileIsStream.push_back(true);
		else
			fileIsStream.push_back(false);

		// select playback mode from name too (;
		size_t reverse = fileNames[i].find(string("_rev_"));
		if(reverse!=string::npos) {
			audiofiles[i].setAdvanceType(adv_backAndForth_);
			printf(", back and forth mode\n");

		}
		// streams are one shot [default advance type] and retriggered every time preset is loaded
		else if(fileIsStream[i]) {
			printf("\n");
		}
		// if not stream, loop!
		else {
			audiofiles[i].setAdvanceType(adv_loop_);
			printf(", loop mode\n");
		}
	}
	printf("\n");



	// waveforms [oscillators and impulse]

	// oscillators
	sine.init(osc_sin_, simulation_rate, periodSize, maxLevel, freq, 0.0);
	square.init(osc_square_, simulation_rate, periodSize, maxLevel, freq, 0.0);
	noise.init(osc_whiteNoise_, simulation_rate, periodSize, maxLevel, 0.0, 0.0);

	// impulse
	fastBandLimitImpulse(maxLevel*2);

	// passthroughs
	for(int i=0; i<numOfInputs; i++)
		passthrough[i].init(periodSize, 1, i, 0);



	// pack them all with the magical power of polymorphism

	// waveform pulse, percussive
	audioModulesOut[0] = &bandLimitImpulse;
	componentsNames.push_back("Impulse");
	componentIsPercussive[0] = true;
	componentIsFileStream[0] = false;
	// other waveforms, non perc
	audioModulesOut[1] = &sine;
	componentsNames.push_back("Sine");
	componentIsPercussive[1] = false;
	componentIsFileStream[1] = false;
	audioModulesOut[2] = &square;
	componentsNames.push_back("Square");
	componentIsPercussive[2] = false;
	componentIsFileStream[2] = false;
	audioModulesOut[3] = &noise;
	componentsNames.push_back("Noise");
	componentIsPercussive[3] = false;
	componentIsFileStream[3] = false;

	// files
	for(int i=0; i<numOfFiles; i++) {
		audioModulesOut[numOfWaveforms+i] = &audiofiles[i];
		componentsNames.push_back(fileNames[i]);
		componentIsPercussive[numOfWaveforms+i] = fileIsPercussive[i];
		componentIsFileStream[numOfWaveforms+i] = fileIsStream[i];
	}

	// inputs/passthroughs
	for(int i=0; i<numOfInputs; i++) {
		audioModulesInOut[i] = &passthrough[i];
		string name = "Audio Input " + to_string(i);
		componentsNames.push_back(name.c_str());
		componentIsPercussive[numOfWaveforms+numOfFiles+i] = false;
		componentIsFileStream[numOfWaveforms+numOfFiles+i] = false;
	}

	// adsr
	adsr.setPeriodSize(periodSize);
	adsr.setAttackRate(0.0001*rate); //
	adsr.setDecayRate(0.1*rate); // not used
	adsr.setSustainLevel(1);
	adsr.setReleaseRate(0.1*rate); // 0.0001
	//adsr.reset();

	// anti aliasing filter, needed when rate_mul > 1 [useless for now on files...]
	aliasingFilter.setBiquad(bq_type_lowpass, ( ((double)rate)/2.0 )/simulation_rate,  0.7071, 0);

	lowPass.setBiquad(bq_type_lowpass, lowPassFreq,  0.7071, 0);
	//----------------------------------------------------------------------------------------------
}

HyperDrumExcitation::~HyperDrumExcitation() {
	if(audioModulesOut!=NULL)
		delete[] audioModulesOut;
	if(audioModulesInOut!=NULL)
		delete[] audioModulesInOut;
	if(audiofiles!=NULL)
		delete[] audiofiles;
	if(componentIsPercussive!=NULL)
		delete[] componentIsPercussive;
	if(componentIsFileStream!=NULL)
		delete[] componentIsFileStream;


}


double **HyperDrumExcitation::getFrameBuffer(int numOfSamples, double **input) {
	// generic handle
	double *tmpBuf;

	if(currentExcitationId < numOfComponents-numOfInputs)
		tmpBuf = audioModulesOut[currentExcitationId]->getFrameBuffer(numOfSamples)[0];
	else
		tmpBuf = audioModulesInOut[currentExcitationId-numOfWaveforms-numOfFiles]->getFrameBuffer(numOfSamples, input)[0];

	if(currentExcitationId==0 || currentExcitationId>=numOfComponents-numOfInputs || componentIsFileStream[currentExcitationId]) {
		for(int n=0; n<numOfSamples; n++) {
			// apply filter
			tmpBuf[n] = lowPass.process(tmpBuf[n]); // this might change in the control thread while we are here

			// smooth volume
			framebuffer[0][n] =  tmpBuf[n]*volume;
			interpolateParam(volumeInterp[0], volume, volumeInterp[1]);	// remove crackles through interpolation
		}
	}
	else {
		if(antiAliasing) // sinc impulse does not want aliasing, it would ruin its perfection!
			aliasingFilter.process(tmpBuf, numOfSamples);

		// attack adsr
		float *adsrBuf = adsr.processBuffer(numOfSamples);

		for(int n=0; n<numOfSamples; n++) {
			// apply filter
			tmpBuf[n] = lowPass.process(tmpBuf[n]); // this might change in the control thread while we are here

			// apply adsr
			tmpBuf[n] *= adsrBuf[n];

			// smooth volume
			framebuffer[0][n] =  tmpBuf[n]*volume;
			interpolateParam(volumeInterp[0], volume, volumeInterp[1]);	// remove crackles through interpolation
		}
	}

	return framebuffer;
}


//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// private methods
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
int HyperDrumExcitation::listFilesInFolder(string dirname) {
		DIR *dir;
		struct dirent *ent;
		vector<string> waveEndings{".wav", ".WAV", ".wave", ".WAVE"};
		vector<string> aiffEndings{".aiff", ".AIFF", ".aif", ".AIF"};


		numOfFiles = 0;

		// Adapted from http://stackoverflow.com/questions/612097/how-can-i-get-a-list-of-files-in-a-directory-using-c-or-c
		if ((dir = opendir (dirname.c_str())) != NULL) {
			/* print all the files and directories within directory */
			while ((ent = readdir (dir)) != NULL) {
				// Ignore dotfiles and . and .. paths
				if(!strncmp(ent->d_name, ".", 1))
					continue;

				//printf("%s\n", ent->d_name);

				// take only .wav and .aiff files
				string name = string(ent->d_name);
				bool isWav = false;
				bool isAiff = false;

				for(string ending : waveEndings) {
					if(equal(ending.rbegin(), ending.rend(), name.rbegin())) {
						isWav = true;
						break;
					}
				}

				if(!isWav) {
					for(string ending : aiffEndings) {
						if(equal(ending.rbegin(), ending.rend(), name.rbegin())) {
							isAiff = true;
							break;
						}
					}
				}


				if(isWav || isAiff) {
					fileNames.push_back(string(name));
					numOfFiles++;
				}
			}
			closedir(dir);
		} else {
			/* could not open directory */
			printf("Could not open directory %s!\n", dirname.c_str());
			return -1;
		}

		if(numOfFiles==0) {
			printf("Could not find wav or aiff files in directory %s!\n", dirname.c_str());
			return -2;
		}
		else {
			sort( fileNames.begin(), fileNames.end(), naturalsort);
			return 0;
		}
}

void HyperDrumExcitation::fastBandLimitImpulse(float level) {  //Cuomo
	//-------------------------------------
	// let's create a sinc-windowed impulse [band pass filter]
	// composed of a low pass sinc and high pass sinc
	//-------------------------------------

	// expressed as fraction of simulation rate
	double lpf = 20000.0/simulation_rate; // low pass freq, set from outside
	double hpf = 1.0/simulation_rate;   // high pass freq, 2 Hz
	double bw  = 0.008; //0.004 		// roll-off bandwidth...0.008 a bit imprecise but faster, peak is at about 250 samples
	double amp = 1* ((float)simulation_rate)/rate;

	int M = 4/bw;

	double sinc_windowed_impulse[M+1];

	// working vars
	double lp_sinc[M+1];
	double hp_sinc[M+1];

	float sum = 0; // to normalize filters

	//-----------------------------------------------------------------------------
	// generate filter as a band reject filter and then do spectral inversion

	// generate first sinc as low pass [opposite of what we need]
	for(int i=0; i<M+1; i++) {
		if (i!=M/2)
			lp_sinc[i] = sin(2*M_PI*hpf * (i-M/2)) / (i-M/2);
		else
			lp_sinc[i] = 2*M_PI*hpf; // cope with division by zero
		lp_sinc[i] = lp_sinc[i] * (0.54 - 0.46*cos(2*M_PI*i/M)); // multiply by Hamming window
		sum += lp_sinc[i];
	}
	// normalize
	for(int i=0; i<M+1; i++)
		lp_sinc[i] /= sum;


	// generate second sinc as low pass sinc and then invert it [to have opposite of what we need]
	sum = 0;
	for(int i=0; i<M+1; i++) {
		if (i!=M/2)
			hp_sinc[i] = sin(2*M_PI*lpf * (i-M/2)) / (i-M/2);
		else
			hp_sinc[i] = 2*M_PI*lpf;
		hp_sinc[i] = hp_sinc[i] * (0.54 - 0.46*cos(2*M_PI*i/M)); // multiply by Hamming window
		sum += hp_sinc[i];
	}
	// normalize and invert, to turn into high pass sinc
	for(int i=0; i<M+1; i++) {
		hp_sinc[i] /= sum;
		hp_sinc[i] = -hp_sinc[i];
	}
	hp_sinc[M/2] += 1;


	// combine two sincs in frequency reject filter [opposite of what we need]
	// and do spectal inversion for band pass [this is what we need]
	for(int i=0; i<M+1; i++) {
		sinc_windowed_impulse[i] = hp_sinc[i] + lp_sinc[i];
		sinc_windowed_impulse[i] = -sinc_windowed_impulse[i];
	}
	sinc_windowed_impulse[M/2] = sinc_windowed_impulse[M/2] + 1;

	// set amplitude, just in case
	for(int i=0; i<M+1; i++)
		sinc_windowed_impulse[i] *= amp*level;

	//bandLimitImpulse.init(1, sinc_windowed_impulse, M+1, periodSize);


	int len = M+1;

	// make it faster
	int ant=0; // CAN anticipate it, at the expenses of the spectrum...
	for(int i=0; i<len-ant; i++)
		sinc_windowed_impulse[i] = sinc_windowed_impulse[i+ant];
	/*for(int i=len-ant; i<len; i++)
		sinc_windowed_impulse[i] = 0;*/

	bandLimitImpulse.init(sinc_windowed_impulse, len-ant, period_size);



	//VIC manual impulse filter...even more unstable
	/*const int L = 3000;
	double imp[L] = {0};
	imp[0] = 1;

	double yh[L] = {0};

	const int N = 4;
	double bh[N+1] = {0.91110,  -3.64441,   5.46661,  -3.64441,   0.91110};
	double ah[N+1] = { 1.00000,  -3.81387,   5.45872,  -3.47494,   0.83011};

	double y_prevh[N+1] = {0};


	for(int i=0; i<L; i++) {
		yh[i] += imp[i]*bh[0];
		for(int j=1; j<N; j++) {
			yh[i] += bh[j]*imp[j];
			yh[i] -= ah[j]*y_prevh[j];
		}

		for(int j=N; j<1; j++)
			y_prevh[j] = y_prevh[j-1];
		y_prevh[0] = yh[i];


				b0*imp[i] + b1*x_1 + b2*x_2 - a1*y_1 - a2*y_2;
		x_2 = x_1;
		x_1 = imp[i];

		y_2 = y_1;
		y_1 = y[i];
	}


	double y[L] = {0};

	double bl[N+1] = { 0.16578,   0.49733,   0.49733,   0.16578};
	double al[N+1] = { 1.0000,  -6.5301e-3,   3.3334e-1, -5.9366e-4};

	double y_prevl[N+1] = {0};


	for(int i=0; i<L; i++) {
		y[i] += yh[i]*bl[0];
		for(int j=1; j<N; j++) {
			y[i] += bl[j]*yh[j];
			y[i] -= al[j]*y_prevl[j];
		}

		for(int j=N; j<1; j++)
			y_prevl[j] = y_prevl[j-1];
		y_prevl[0] = y[i];


				b0*imp[i] + b1*x_1 + b2*x_2 - a1*y_1 - a2*y_2;
		x_2 = x_1;
		x_1 = imp[i];

		y_2 = y_1;
		y_1 = y[i];

	}
	bandLimitImpulse.init(1, y, L, periodSize);

	*/






	bandLimitImpulse.setAdvanceType(adv_oneShot_);
}
