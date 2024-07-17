/*
 * freqResponse.cpp
 *
 *  Created on: 2016-06-25
 *      Author: Victor Zappi
 */
#include "freqResponse.h"

#include <fftw3.h>  // fft stuff
#include <cstring>  // for memcpy, also needed in cvalarray.h

#include "sndfile.h" // save audio (;

#include "SimpleExcitation.h"
#include "ArtVocSynth.h"
#include "FFT.h"
#include "SSE.h" // safe to include even if not supported
#include "QuadraticMaxima.h"

extern ArtVocSynth *artVocSynth;
extern unsigned int rate;

#define MAX_FORMANTS 100

#define TIME_IMP_RESP
#ifdef TIME_IMP_RESP
#include <sys/time.h> // for time test
int elapsedTime;
timeval start_, end_;
#endif

// searchFormants() and search3dB() adapted from JASS source code [jass/src/jass/utils/formantsPlotter.java]

// search for -3dB cut
float search3dB(float mag[], float freq[], float max, int idx, int numSamples, bool up) {
	float x_tmp;
	QuadraticMaxima q;

	float y0 = 0;
	float y1 = 0;
	float y2 = 0;

	float x0 = 0;
	float x1 = 0;
	float x2 = 0;

	if (up == true) {
		for (int k=idx; k<numSamples-1; k++) {
			y0 = mag[k-1];
			y1 = mag[k];
			y2 = mag[k+1];

			x0 = freq[k-1];
			x1 = freq[k];
			x2 = freq[k+1];

			q.set(x0, x1, x2, y0, y1, y2);
			x_tmp = q.findX_left(max-3); //replace with 'findLower3dB()?

			//looking for the upper 3dB point, we first look at the left solution.
			if ( (x_tmp >= x0) && (x_tmp <= x1) ) {
				return x_tmp;
			}
			else {
				x_tmp = q.findX_right(max-3);
				if ( (x_tmp >= x0) && (x_tmp <= x1) ) {
					return x_tmp;
				}
			}
		}
	}
	else {
		for (int k=idx; k>0; k--) {
			y0 = mag[k-1];
			y1 = mag[k];
			y2 = mag[k+1];

			x0 = freq[k-1];
			x1 = freq[k];
			x2 = freq[k+1];

			q.set(x0, x1, x2, y0, y1, y2);

			// looking for the lower 3dB point, we first look at the RIGHT (closest) solution
			x_tmp = q.findX_right(max-3);

			if ( (x_tmp >= x0) && (x_tmp <= x1) ) {
				return x_tmp;
			}
			else {
				x_tmp = q.findX_left(max-3);
				if ( (x_tmp >= x0) && (x_tmp <= x1) ) {
					return x_tmp;
				}
			}
		}
	}
	//if we didn't find anything, return -1;
	return -1;
}

// find formants
void searchFormants(float mag[], float freq[], int numSamples, float refFormants[]=NULL, int refFormantsNum=0) {
	float formant[MAX_FORMANTS];
	float bandwidth[MAX_FORMANTS];
	//int formantIndex[MAX_FORMANTS];
	QuadraticMaxima q;
	int  nFormants = 0;
    for(int i=1;i<numSamples-1;i++) {
        float prev = mag[i-1];
        float next = mag[i+1];
        float db = mag[i];

	    float x0 = freq[i-1];
	    float x1 = freq[i];
	    float x2 = freq[i+1];

	    if (nFormants<MAX_FORMANTS) {
	    	q.set(x0, x1, x2, prev, db, next);
			if (q.containsPeak()) {
			    float upper3dB;
			    float lower3dB;
			    formant[nFormants] = q.getPeakX()/2; //VIC not sure why i had to divide by 2 to get the actual formant freq

			    upper3dB = q.getUpper3dB();
			    lower3dB = q.getLower3dB();
			    // if -3dB cut is more than a step away...
			    if (q.getUpper3dB() > x2)
			    	upper3dB = search3dB(mag, freq, q.getPeakY(), i+1, numSamples, true);
			    if (q.getLower3dB() < x0)
			    	lower3dB = search3dB(mag, freq, q.getPeakY(), i-1, numSamples, false);
			    if ( (upper3dB == -1) || (lower3dB ==-1) )
			    	bandwidth[nFormants] = -1;
			    else
			    	bandwidth[nFormants] = (upper3dB - lower3dB);

			    if(bandwidth[nFormants]>=10 && bandwidth[nFormants]<2000)
			    	nFormants++;//formantIndex[nFormants++] = i; // where in the array...
			}
	    }
	}
    if(refFormants == NULL) {
		cout << "Number: \t";
		for(int i=0; i<nFormants; i++)
			cout << "\t\t" << i+1;
		cout << endl;
		cout << "Formants [Hz]: \t";
		for(int i=0; i<nFormants; i++)
			cout << "\t\t" << formant[i];
		cout << endl;
		cout << "Bandwidths [Hz]: ";
		for(int i=0; i<nFormants; i++)
			cout << "\t\t" << bandwidth[i];
		cout << endl;
    }
    else {
    	int *fIndices = new int[refFormantsNum];
    	float *fAbsErrors = new float[refFormantsNum];
    	float *fPercErrors = new float[refFormantsNum];

    	int fIndex = 0;
    	for(int i=0; i<refFormantsNum && fIndex<MAX_FORMANTS; i++) {
    		fPercErrors[fIndex] = 100;
    		do {
    			fAbsErrors[fIndex]  = (formant[fIndex]-refFormants[i]);
				fPercErrors[fIndex] = 100*fAbsErrors[fIndex]/refFormants[i];
				fIndices[i] = fIndex;
    		}
    		while(fabs(fPercErrors[fIndex++])>30  && fIndex<MAX_FORMANTS-1 );
    	}
    	cout << "Number: \t";
		for(int i=0; i<refFormantsNum; i++)
			cout << "\t\t" << i+1;
		cout << endl;
		cout << "Formants [Hz]: \t";
		for(int i=0; i<refFormantsNum; i++)
			cout << "\t\t" << formant[fIndices[i]];
		cout << endl;
		cout << "Bandwidths [Hz]: ";
		for(int i=0; i<refFormantsNum; i++)
			cout << "\t\t" << bandwidth[fIndices[i]];
		cout << endl;

    	cout << "Errors: \t\t";
    	for(int i=0; i<refFormantsNum; i++)
    	    cout << "\t" << fAbsErrors[fIndices[i]]<< " Hz";
    	cout << endl;
    	cout << "\t\t\t";
    	for(int i=0; i<refFormantsNum; i++)
			cout << "\t" << fPercErrors[fIndices[i]]<< " %";
		cout << endl;


		delete[] fIndices;
		delete[] fPercErrors;
		delete[] fAbsErrors;
    }
 }

// bounce impulse response! [audio used to calculate freq response]
void saveImpulseResponse(float audioSamples[], int numOfSamples, int rateMul, string expName) {
	string outfilepath_ = expName + "_" + to_string(rateMul)+".wav";

	SF_INFO sfinfo;
	SNDFILE * outfile;
	sfinfo.channels = 1;
	sfinfo.samplerate = rate;
	sfinfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
	outfile = sf_open(outfilepath_.c_str(), SFM_WRITE, &sfinfo);

	// copy audio buffer to file
	sf_count_t count = sf_write_float(outfile, audioSamples, numOfSamples);
	if(count<1)
		printf("): written samples count: %d\n", (int)count);

	// to properly save audio
	sf_write_sync(outfile);
	sf_close(outfile);
}

#ifdef SSE_SUPPORT
inline void computeMagnitudeSSE(vector<vec4>& data, int numOfSamples, vector<vec4>& mag, vector<vec4>& freq) {
	int size = data.size();
	for (int i=0;i<size;i+=2) {
		vec4 aa  = data[i];
		vec4 bb  = data[i+1];

		// square
		aa *=aa;
		bb *=bb;

		// re combine
		vec4 re_2(aa[0], aa[2], bb[0], bb[2]);
		vec4 im_2(aa[1], aa[3], bb[1], bb[3]);

		int write_index = i/2;

		// magnitude in dB
		mag[write_index] =  10*log_ps(Sqrt(re_2 + im_2));

		int index = write_index*4;

		// find all indices
		vec4 cc(index, index, index, index);
		vec4 dd(0, 1, 2, 3);

		// frequencies in Hz
		freq[write_index] = (cc+dd)*rate/numOfSamples;
	}
}
#else
	inline void computeMagnitude(float data[][2], int numOfSamples, float *magnitude, float *frequency) {
		for (int i=0;i<numOfSamples;i++) {
			magnitude[i] = 10*log(sqrtf(data[i][0]*data[i][0] + data[i][1]*data[i][1]));
			frequency[i] = ((float)i*rate)/numOfSamples;
		}
	}
#endif


void calculateFreqResponse(int numAudioSamples, int periodSize, int rateMul, string expName, float refFromants[], int refFormantsNum) {
	// at this period size, how many buffers from the synth we need to get the requested number of bins?
	int numOfFullBuffers = ( (float)numAudioSamples )/( (float)periodSize);
	int residue = numAudioSamples%periodSize;
	// NB we don't change the period size to match the number of bins because this would affect the pixel layout of the simulation!

	// buffer to contain all the audio samples we need
	float audioSamples[numAudioSamples];

	// fill buffer with multiple audio calls
	int i;
#ifdef TIME_IMP_RESP
	gettimeofday(&start_, NULL);
#endif
	for(i=0;i<numOfFullBuffers; i++)
		memcpy(audioSamples+i*periodSize, artVocSynth->getBufferFloat(periodSize), periodSize*sizeof(float));

	// just in case period_size and numAudioSamples are not integer multiples
	if(residue>0)
		memcpy(audioSamples+i*periodSize, artVocSynth->getBufferFloat(periodSize), residue*sizeof(float));
#ifdef TIME_IMP_RESP
		gettimeofday(&end_, NULL);
		elapsedTime = ( (end_.tv_sec*1000000+end_.tv_usec) - (start_.tv_sec*1000000+start_.tv_usec) );
		printf("---------------------------------------------------------\n");
		printf("Simulation lasted: %d ms\n", elapsedTime/1000);
		printf("---------------------------------------------------------\n");
	#endif

	// prepare where to save fft samples
	int numOfFFTsamples = ((float)numAudioSamples)/2.0 + 1; // number of samples computed in fft
#ifdef SSE_SUPPORT
	residue  = numOfFFTsamples%4;             // with this always multiple of 4, ready for magnitude calculation via sse
	int scalarSize = numOfFFTsamples+residue; // here we go, multiple of 4
	float fftSamples[scalarSize][2];          // the extra locations at the end of fftSample will be ignored
#else
	float fftSamples[numOfFFTsamples][2];
#endif

	// ...
	calculateFFT(audioSamples, numAudioSamples, fftSamples);

	// generic easy-to-use structures to save magnitude and frequencies
	float magnitude[numOfFFTsamples];
	float frequency[numOfFFTsamples];

#ifdef SSE_SUPPORT
	// sse-ready data structure
	int vectorSize = (scalarSize*2)/4;      // we will put real AND imaginary part of fft in one single vector of vec4...
	vector<vec4> sseFftSamples(vectorSize); // ...this guy
	vector<vec4> sseMag(scalarSize/2);      // here magnitude will be temporary stored [half the size]
	vector<vec4> sseFreq(scalarSize/2);     // same for frequencies

	// copy all fft samples into sse data structure
	memcpy(&sseFftSamples[0], fftSamples, scalarSize*2*sizeof(float)); // formatted as [re, im, re, im, ...] -> i know it's a shit but no other solutions

	computeMagnitudeSSE(sseFftSamples, numOfFFTsamples, sseMag, sseFreq);

	// copy sse magnitude and frequencies into generic float array, in posterum
	memcpy(magnitude, &sseMag[0], numOfFFTsamples*sizeof(float));
	memcpy(frequency, &sseFreq[0], numOfFFTsamples*sizeof(float));
	// fuck me we're done...it took ages but this code goes VERY fast
#else
	computeMagnitude(fftSamples, numOfFFTsamples, magnitude, frequency);
#endif

	// print formants and bandwidths
	//searchFormants(magnitude, frequency, numOfFFTsamples, refFromants, refFormantsNum); //VIC often does not work properly! ):

/*	ofstream myfile;
	myfile.open ("porcodio.txt");
	for(int i=0; i<numOfFFTsamples; i++)
		myfile << magnitude[i] << endl;
	myfile.close();*/


	// save audio
	saveImpulseResponse(audioSamples, numAudioSamples, rateMul, expName);
}
