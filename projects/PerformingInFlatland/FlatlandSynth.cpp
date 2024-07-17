/*
 * FlatlandSynth.cpp
 *
 *  Created on: 2015-11-18
 *      Author: vic
 */

#include "FlatlandSynth.h"


FlatlandSynth::FlatlandSynth() {
	simulationRate = -1;
	rateMultiplier = -1;

	flatlandExcitation = NULL;

	passThruTrack = -1;
}

void FlatlandSynth::init(string shaderLocation, int *domainSize, int *excitationCoord, int *excitationDimensions,
			float ***domain, int audioRate, int rateMul, int period_size, float magnifier, int *listCoord, float exLpF, bool useSinc) {
	// normExLpF is passed both to the vocal tract and the subglottal flatlandExcitation
	// in both cases it serves to calculate the low pass cut off frequency of the filter that assures
	// that we are injecting in the simulation too high frequencies
	//
	// in the case of the vocal tract, this filtering is applied to the output of any feedback flatlandExcitation model [implemented in shader]
	// in the case of the subglottal flatlandExcitation, this filtering happens in the sinc windowed impulse used to calculate the impulse response

	//-----------------------------------------------------------------------------
	periodSize      = period_size;
	rate            = audioRate;
	rateMultiplier  = (rateMul>0) ? rateMul : 1; // juuust in case
	simulationRate  = flatlandFDTD.init(shaderLocation, domainSize, excitationCoord, excitationDimensions,
										  domain, audioRate, rateMul, periodSize, magnifier, listCoord, exLpF);

	initExcitation(rateMul, exLpF, useSinc);
	initTracks();
}

void FlatlandSynth::init(string shaderLocation, int *domainSize, int audioRate, int rateMul, int period_size,
					   float magnifier, int *listCoord, float exLpF, bool useSinc) {
	// normExLpF is passed both to the vocal tract and the subglottal flatlandExcitation
	// in both cases it serves to calculate the low pass cut off frequency of the filter that assures
	// that we are injecting in the simulation too high frequencies
	//
	// in the case of the vocal tract, this filtering is applied to the output of any feedback flatlandExcitation model [implemented in shader]
	// in the case of the subglottal flatlandExcitation, this filtering happens in the sinc windowed impulse used to calculate the impulse response

	//-----------------------------------------------------------------------------
	periodSize      = period_size;
	rate            = audioRate;
	rateMultiplier  = (rateMul>0) ? rateMul : 1; // juuust in case
	simulationRate  = flatlandFDTD.init(shaderLocation, domainSize, audioRate, rateMul, period_size,
									      magnifier, listCoord, exLpF);

	initExcitation(rateMul, exLpF, useSinc);
	initTracks();
}

int FlatlandSynth::setDomain(int *excitationCoord, int *excitationDimensions, float ***domain) {
	if(flatlandFDTD.setDomain(excitationCoord, excitationDimensions, domain)!=0) {
		printf("Cannot load domain! Vocal Tract FDTD simulation already has one\n");
		return 1;
	}
	return 0;
}

int FlatlandSynth::loadContourFile(int domain_size[2], float deltaS, string filename, float ***pixels) {
	int a1 = flatlandFDTD.loadContourFile_new(domain_size, deltaS, filename, pixels);
	if(a1<0)
		printf("Cannot load contour!\n");
	return a1;
}

// screw this
double FlatlandSynth::getSample() {
	printf("Hey! FlatlandSynth::getSample() is not implemented, why the hell are you calling it, uh?\n");
	return 0;
}

// and screw this too
double *FlatlandSynth::getBuffer(int numOfSamples) {
	printf("Hey! FlatlandSynth::getBuffer() is not implemented, why the hell are you calling it, uh?\n");
	return NULL;
}

// we override cos we don't need a local buffer! [we use TextureFilter::outputBuffer]
float *FlatlandSynth::getBufferFloat(int numOfSamples) {
	double *trackBuff[TRACKS_NUM];
	for(int i=0; i<TRACKS_NUM; i++)
		trackBuff[i] = tracks[i].getBuffer(numOfSamples);
	flatlandExcitation->setPassThruBuffer(trackBuff[passThruTrack]);

	float *excBuff   = flatlandExcitation->getBufferFloat(numOfSamples*rateMultiplier);
	float *outBuffer = flatlandFDTD.getBuffer(numOfSamples, excBuff);

	for(int n=0; n<numOfSamples; n++) {
		for(int i=0; i<TRACKS_NUM; i++) {
			if(tracksOn[i])
				outBuffer[n] += trackBuff[i][n]*0.8*trckVol[i];
			interpolateParam(trckVolInterp[i][0], trckVol[i], trckVolInterp[i][1]);
		}
	}

	return outBuffer;
}

bool FlatlandSynth::toggleTrack(int index, bool on) {
	if(index<0 || index>=TRACKS_NUM) {
		printf("Can't toggle track %d, cos there are only %d tracks!!!\n", index, TRACKS_NUM);
		return false;
	}

	tracksOn[index] = on;

	return true;
}


//----------------------------------------------------------------------------------------
// private
//----------------------------------------------------------------------------------------
void FlatlandSynth::initExcitation(int rate_mul, float normExLpF, bool useSinc) {
	float maxExcitationLevel = 0.5;
	flatlandExcitation = new FlatlandExcitation(maxExcitationLevel, rate, periodSize*rate_mul, simulationRate, normExLpF, useSinc); // periodSize*rate_mul cos we have to provide enough samples per each simulation step
}

void FlatlandSynth::initTracks() {
	std::string filename = "audiofiles/bounced_tracks/0_dark_theme.wav";
	tracks[0].init(1, filename, periodSize, rate);
	filename = "audiofiles/bounced_tracks/1_filtered_drums.wav";
	tracks[1].init(1, filename, periodSize, rate);
	filename = "audiofiles/bounced_tracks/2_quiet_drums.wav";
	tracks[2].init(1, filename, periodSize, rate);
	filename = "audiofiles/bounced_tracks/3_straight_drums.wav";
	tracks[3].init(1, filename, periodSize, rate);
	filename = "audiofiles/bounced_tracks/4_complex_drums.wav";
	tracks[4].init(1, filename, periodSize, rate);
	filename = "audiofiles/bounced_tracks/5_mod_pad.wav";
	tracks[5].init(1, filename, periodSize, rate);
	filename = "audiofiles/bounced_tracks/6_bright_theme.wav";
	tracks[6].init(1, filename, periodSize, rate);

	// first 2 tracks and last track are on, but with zero volume
	// rest is off with max volume
	tracksOn[0] = true;
	tracksOn[1] = true;
	for(int i=2; i<TRACKS_NUM-1; i++)
		tracksOn[i] = false;
	tracksOn[TRACKS_NUM-1] = true;

	trckVol[0]= 0;
	trckVolInterp[0][0] = 0;
	trckVolInterp[0][1] = 0;
	trckVol[1]= 0;
	trckVolInterp[1][0] = 0;
	trckVolInterp[1][1] = 0;
	for(int i=2; i<TRACKS_NUM-1; i++) {
		trckVol[i]= 1;
		trckVolInterp[i][0] = 1;
		trckVolInterp[i][1] = 0;
	}
	trckVol[TRACKS_NUM-1]= 0;
	trckVolInterp[TRACKS_NUM-1][0] = 0;
	trckVolInterp[TRACKS_NUM-1][1] = 0;

	passThruTrack = 1;
}


/*FlatlandSynth::~FlatlandSynth() {

}*/
