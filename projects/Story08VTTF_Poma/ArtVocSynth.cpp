/*
 * ArtVocSynth.cpp
 *
 *  Created on: 2015-11-18
 *      Author: vic
 */

#include "ArtVocSynth.h"


ArtVocSynth::ArtVocSynth() {
	simulationRate = -1;
	rateMultiplier = -1;

	excitation = NULL;
}

void ArtVocSynth::init(string shaderLocation, int *domainSize, int *excitationCoord, int *excitationDimensions,
			float ***domain, int audioRate, int rateMul, int period_size, float magnifier, int *listCoord, float exLpF, bool useSinc) {
	// normExLpF is passed both to the vocal tract and the subglottal excitation
	// in both cases it serves to calculate the low pass cut off frequency of the filter that assures
	// that we are injecting in the simulation too high frequencies
	//
	// in the case of the vocal tract, this filtering is applied to the output of any feedback excitation model [implemented in shader]
	// in the case of the subglottal excitation, this filtering happens in the sinc windowed impulse used to calculate the impulse response

	//-----------------------------------------------------------------------------
	AudioGeneratorOut::init(period_size);
	rate            = audioRate;
	rateMultiplier  = (rateMul>0) ? rateMul : 1; // juuust in case
	simulationRate  = vocalTractFDTD.init(shaderLocation, domainSize, excitationCoord, excitationDimensions,
										  domain, audioRate, rateMul, periodSize, magnifier, listCoord, exLpF);

	initExcitation(rateMul, exLpF, useSinc);
}

void ArtVocSynth::init(string shaderLocation, int *domainSize, int audioRate, int rateMul, int period_size,
					   float magnifier, int *listCoord, float exLpF, bool useSinc) {
	// normExLpF is passed both to the vocal tract and the subglottal excitation
	// in both cases it serves to calculate the low pass cut off frequency of the filter that assures
	// that we are injecting in the simulation too high frequencies
	//
	// in the case of the vocal tract, this filtering is applied to the output of any feedback excitation model [implemented in shader]
	// in the case of the subglottal excitation, this filtering happens in the sinc windowed impulse used to calculate the impulse response

	//-----------------------------------------------------------------------------
	AudioGeneratorOut::init(period_size);
	rate            = audioRate;
	rateMultiplier  = (rateMul>0) ? rateMul : 1; // juuust in case
	simulationRate  = vocalTractFDTD.init(shaderLocation, domainSize, audioRate, rateMul, period_size,
									      magnifier, listCoord, exLpF);

	initExcitation(rateMul, exLpF, useSinc);
}

void ArtVocSynth::initExcitation(int rate_mul, float normExLpF, bool useSinc) {
	float maxExcitationLevel = 1;//0.25;//0.2/*1*/; //@VIC
	excitation = new SimpleExcitation(maxExcitationLevel, rate, periodSize*rate_mul, simulationRate, normExLpF, useSinc); // periodSize*rate_mul cos we have to provide enough samples per each simulation step


	//TODO [kinda] check periods and rates! then add in getBufferFloat()
	// domain control tests -> fun shit

	osc.init(osc_saw_, rate, periodSize, 0.5/*1.0 and above for awesome instability*/, 5, 0, true);
	oscKr.init(osc_saw_, rate/periodSize, periodSize, 0.5/*1.0 and above for awesome instability*/, 5, 0, true);

	env.setPeriodSize(periodSize);
	env.setAttackRate(25 * rate);
	env.setDecayRate(0.1 * rate);
	env.setSustainLevel(1.0);
	env.setReleaseRate(0.8 * rate);
	env.gate(1); // adsr activated! ready to fade in

	std::string filename = "audiofiles/drum7.wav";
	//std::string filename = "audiofiles/bowedC_.wav";
	audiofile.setAdvanceType(adv_loop_);
	audiofile.init(filename, periodSize, rate, maxExcitationLevel);
}

int ArtVocSynth::setDomain(int *excitationCoord, int *excitationDimensions, float ***domain) {
	if(vocalTractFDTD.setDomain(excitationCoord, excitationDimensions, domain)!=0) {
		printf("Cannot load domain! Vocal Tract FDTD simulation already has one\n");
		return 1;
	}
	return 0;
}

int ArtVocSynth::loadContourFile(int domain_size[2], float deltaS, string filename, float ***pixels) {
	int a1 = vocalTractFDTD.loadContourFile_new(domain_size, deltaS, filename, pixels);
	if(a1<0)
		printf("Cannot load contour!\n");
	return a1;
}

// screw this
double ArtVocSynth::getSample() {
	printf("Hey! ArtVocSynth::getSample() is not implemented, why the hell are you calling it, uh?\n");
	return 0;
}

double *ArtVocSynth::getBuffer(int numOfSamples) {
	double *excBuff = excitation->getBuffer(numOfSamples*rateMultiplier);
	float *buff = vocalTractFDTD.getBufferFloat(numOfSamples, excBuff);
	std::copy(buff, buff+numOfSamples, samplebuffer);
	return samplebuffer;
}


// we override cos we don't need a local buffer! [we use TextureFilter::outputBuffer]
float *ArtVocSynth::getBufferFloat(int numOfSamples) {

	// for crazy domain scale modulations!
	// modulation with osc
	//double *dbuff = osc.getBuffer(numSamples);
	//vocalTractFDTD.setDS(0.09 + dbuff[numSamples-1]*0.02 ); // here I compute all the samples and choose the one that I prefer

	//vocalTractFDTD.setDS(0.09 + (oscKr.*oscKr.getSample)()*0.02 ); // here I work at a much bigger rate and compute only one control sample per buffer

	// modulation with audio file
	//double *dbuff = audiofile.getBuffer(numSamples);
	//vocalTractFDTD.setDS(0.09 + dbuff[0]*0.002 );

	// excitation fade in
	//float *envBuff = env.processBuffer(numSamples);

	double *excBuff = excitation->getBuffer(numOfSamples*rateMultiplier);
	//for(int n = 0; n < numSamples; n++)
	//	excBuff[n] *= envBuff[n];

	return vocalTractFDTD.getBufferFloat(numOfSamples, excBuff);

	// input test
	//return excBuff;
}


/*ArtVocSynth::~ArtVocSynth() {

}*/
