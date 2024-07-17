/*
 * DrumheadCPU.h
 *
 *  Created on: 2016-12-01
 *      Author: Victor Zappi
 */

#ifndef DRUMHEADCPU_H_
#define DRUMHEADCPU_H_

#include "SSE.h" // safe to include even if not supported

using namespace std;

class DrumheadCPU {
public:
	DrumheadCPU();
	~DrumheadCPU();
	void init(unsigned int audio_rate, unsigned short period_size, int *domain_size, int *list_coord);
	void setDomain(int *exciteCoord, int *excitationDimensions, float ***domain);
	float *getBufferFloat(int numSamples, float *inputBuffer);
	void setDampingFactor(float dampFact);
	int setPropagationFactor(float propFact);
	void setClampingFactor(float clampFact);

private:
	float *(DrumheadCPU::*getBufferFloatInner)(int numSamples, float *inputBuffer);
	float *getBufferScalar(int numSamples, float *inputBuffer);
	float *getBufferSSE(int numSamples, float *inputBuffer);
	void allocateSimulationData();
	void clearSimulationData();
	void initFrame();
	void fillDomain(); // still missing
	// audio
	unsigned int rate;
	unsigned long periodSize;
	float *floatsamplebuffer;

	// simulation
	int domainSize[2];
	int listenerCoord[2];
	float mu;  // damping
	float rho; // propagation factor, that combines spatial scale and speed in the medium
	float boundGain;

	float ***p;
	int **excitation;
	float **beta;

	// SSE stuff
	int sseDomainSize[2];

	vector< vector<vec4> > PP[2];
	vector< vector<vec4> > BU;
	vector< vector<vec4> > BD;
	vector< vector<vec4> > BL;
	vector< vector<vec4> > BR;
	vector< vector<vec4> > EX;

	vec4 *L0;
	vec4 *L1;

	int list_quad;



	int p_index;

	int frameSize[2];
};

inline void DrumheadCPU::setDampingFactor(float dampFact) {
	mu = dampFact;
}

inline int DrumheadCPU::setPropagationFactor(float propFact) {
	// stability check...and we take for granted it's greater than zero
	if(propFact > 0.5)
		return 1;

	rho = propFact;
	return 0;
}

inline void DrumheadCPU::setClampingFactor(float clampFact) {
	// we take for granted it's greater than 0 and lower than 1
	boundGain = 1-clampFact;
}



#endif /* DRUMHEADCPU_H_ */
