/*
 * DrumheadCPU.cpp
 *
 *  Created on: 2016-12-01
 *      Author: Victor Zappi
 */

#include "DrumheadCPU.h"
#include <cstring> // memset, memcpy
#include <cstdio>

using namespace std;

DrumheadCPU::DrumheadCPU() {
	// audio
	rate  = -1;
	periodSize = -1;
	floatsamplebuffer = NULL;

	// simulation
	domainSize[0] = 0;
	domainSize[1] = 0;

	listenerCoord[0] = -1;
	listenerCoord[1] = -1;

	mu = 0.001;
	rho = 0.5;
	boundGain = 0;

	p = NULL;
	excitation = NULL;
	beta = NULL;

	sseDomainSize[0] = 0;
	sseDomainSize[1] = 0;

	L0 = NULL;
	L1 = NULL;
	list_quad = -1;

	p_index = 0;

	frameSize[0] = -1;
	frameSize[1] = -1;

	getBufferFloatInner = NULL;
}
void DrumheadCPU::init(unsigned int audio_rate, unsigned short period_size, int *domain_size, int *list_coord) {
	// audio
	rate  = audio_rate;
	periodSize = period_size;
	floatsamplebuffer = new float[periodSize];
	memset(floatsamplebuffer, 0, sizeof(float)*periodSize);

	// simulation params
	domainSize[0] = domain_size[0];
	domainSize[1] = domain_size[1];

	listenerCoord[0] = list_coord[0];
	listenerCoord[1] = list_coord[1];

	sseDomainSize[0] = (domainSize[0]/2)+1;
	sseDomainSize[1] = (domainSize[1]/2)+1;

	// oh yeah
	allocateSimulationData();
	initFrame();

	getBufferFloatInner = &DrumheadCPU::getBufferSSE;
	getBufferFloatInner = &DrumheadCPU::getBufferScalar;
}


void DrumheadCPU::setDomain(int *exciteCoord, int *excitationDimensions, float ***domain) {
	if(p==NULL) {
		printf("Before setting the domain, synth must be inited!\n");
		return;
	}

	// fill domain with stuff passed from outside and set excitation
	// lots of +1 cos domain is contained in the frame and all our matrices are as big as the frame
	for(int i=0; i<domainSize[0]; i++) {
		for(int j=0; j<domainSize[1]; j++) {
			// wall
			if(domain[i][j][3] == 0)
				beta[i+1][j+1] = 0;
			// air
			else if(domain[i][j][3] == 1)
				beta[i+1][j+1] = 1;
			// rest is ignored

			// excitation is passed as special case, for human intelligibility
			if( (i >= exciteCoord[0] && i < exciteCoord[0]+excitationDimensions[0]) &&
		        (j >= exciteCoord[1] && j < exciteCoord[1]+excitationDimensions[1]) )
				excitation[i+1][j+1] = 1;
			else
				excitation[i+1][j+1] = 0;
		}
	}

	// SSE stuff
	// cycle all domain cells
	for(int i=1; i<domainSize[0]+1; i+=2) {
		for(int j=1; j<domainSize[1]+1; j+=2) {
			// prepare fixed quadruple data struct for neighbor beta adn excitation, as:
			// 0 bottom left [me]
			// 1 bottom right
			// 2 top left
			// 3 top right

			// /2 cos grouped in quads
			// + 1 to skip first border cells

			// beta of top neighbors
			BU[i/2 + 1][j/2 + 1][0] = beta[i][j+1];
			BU[i/2 + 1][j/2 + 1][1] = beta[i+1][j+1];
			BU[i/2 + 1][j/2 + 1][2] = beta[i][j+2];
			BU[i/2 + 1][j/2 + 1][3] = beta[i+1][j+2];

			// beta of bottom neighbors
			BD[i/2 + 1][j/2 + 1][0] = beta[i][j-1];
			BD[i/2 + 1][j/2 + 1][1] = beta[i+1][j-1];
			BD[i/2 + 1][j/2 + 1][2] = beta[i][j];
			BD[i/2 + 1][j/2 + 1][3] = beta[i+1][j];

			// beta of left neighbors
			BL[i/2 + 1][j/2 + 1][0] = beta[i-1][j];
			BL[i/2 + 1][j/2 + 1][1] = beta[i][j];
			BL[i/2 + 1][j/2 + 1][2] = beta[i-1][j+1];
			BL[i/2 + 1][j/2 + 1][3] = beta[i][j+1];

			// beta of right neighbors
			BR[i/2 + 1][j/2 + 1][0] = beta[i+1][j];
			BR[i/2 + 1][j/2 + 1][1] = beta[i+2][j];
			BR[i/2 + 1][j/2 + 1][2] = beta[i+1][j+1];
			BR[i/2 + 1][j/2 + 1][3] = beta[i+2][j+1];

			// excitation
			EX[i/2 + 1][j/2 + 1][0] = excitation[i][j];
			EX[i/2 + 1][j/2 + 1][1] = excitation[i+1][j];
			EX[i/2 + 1][j/2 + 1][2] = excitation[i][j+1];
			EX[i/2 + 1][j/2 + 1][3] = excitation[i+1][j+1];
		}
	}
}

float *DrumheadCPU::getBufferFloat(int numSamples, float *inputBuffer) {
	return (this->*getBufferFloatInner)(numSamples, inputBuffer);
}

DrumheadCPU::~DrumheadCPU() {
	clearSimulationData();
}

//---------------------------------------------------------------------------------------------------------------------------------
// private methods
//---------------------------------------------------------------------------------------------------------------------------------
float *DrumheadCPU::getBufferScalar(int numSamples, float *inputBuffer) {
	// time to solve stuff
	float P, C, U, D, L, R, CB;
	int p_index_other;

	// for all samples
	for(int n=0; n<numSamples; n++) {
		p_index_other = 1-p_index;
		// inside domain [no outer cell later]
		for(int i=1; i<frameSize[0]-1; i++) {
			for(int j=1; j<frameSize[1]-1; j++) {

				C = p[i][j][p_index];
				P = p[i][j][p_index_other];

				CB = C*boundGain;

				U = beta[i][j+1]*p[i][j+1][p_index] + (1-beta[i][j+1])*CB;
				D = beta[i][j-1]*p[i][j-1][p_index] + (1-beta[i][j-1])*CB;
				L = beta[i-1][j]*p[i-1][j][p_index] + (1-beta[i-1][j])*CB;
				R = beta[i+1][j]*p[i+1][j][p_index] + (1-beta[i+1][j])*CB;

				p[i][j][p_index_other] = (2 * C + (mu - 1) * P + (L + R + U + D - 4 * C) * rho) / (mu + 1);

				p[i][j][p_index_other] += inputBuffer[n]*excitation[i][j];
			}
		}
		floatsamplebuffer[n] = p[listenerCoord[0]][listenerCoord[1]][p_index_other];
		p_index = 1-p_index;
	}

	//return inputBuffer;
	return floatsamplebuffer;
}

float *DrumheadCPU::getBufferSSE(int numSamples, float *inputBuffer) {
	int p_index_other;

	// for all samples
	for(int n=0; n<numSamples; n++) {
		p_index_other = 1-p_index;
		//printf("*****\n");

		// cycle all the quads except for first and last col/row
		for(int i=1; i<sseDomainSize[0]; i++) {
			for(int j=1; j<sseDomainSize[1]; j++) {
				// first update quadruple data struct for neighbor pressure, as:
				// 0 bottom left [me]
				// 1 bottom right
				// 2 top left
				// 3 top right

				vec4 C = PP[p_index][i][j];
				vec4 P = PP[p_index_other][i][j];
				vec4 E = EX[i][j];

				vec4 CB = C*boundGain;

				vec4 IN = vec4(inputBuffer[n]);

				vec4 PU;
				vec4 PD;
				vec4 PL;
				vec4 PR;

				vec4 BBU = BU[i][j];
				vec4 BBD = BD[i][j];
				vec4 BBL = BL[i][j];
				vec4 BBR = BR[i][j];


				// pressure of top neighbors
				PU[0] = PP[p_index][i][j][2];
				PU[1] = PP[p_index][i][j][3];
				PU[2] = PP[p_index][i][j+1][0];
				PU[3] = PP[p_index][i][j+1][1];

				// pressure of bottom neighbors
				PD[0] = PP[p_index][i][j-1][2];
				PD[1] = PP[p_index][i][j-1][3];
				PD[2] = PP[p_index][i][j][0];
				PD[3] = PP[p_index][i][j][1];

				// pressure of left neighbors
				PL[0] = PP[p_index][i-1][j][1];
				PL[1] = PP[p_index][i][j][0];
				PL[2] = PP[p_index][i-1][j][3];
				PL[3] = PP[p_index][i][j][2];

				// pressure of right neighbors
				PR[0] = PP[p_index][i][j][1];
				PR[1] = PP[p_index][i+1][j][0];
				PR[2] = PP[p_index][i][j][3];
				PR[3] = PP[p_index][i+1][j][2];

				vec4 U = BBU*PU + (1-BBU)*CB;
				vec4 D = BBD*PD + (1-BBD)*CB;
				vec4 L = BBL*PL + (1-BBL)*CB;
				vec4 R = BBR*PR + (1-BBR)*CB;

				PP[p_index_other][i][j] = ( (2 * C + (mu - 1) * P + (L + R + U + D - 4 *  C) / 2) / (mu + 1)) + E * IN;
			}
		}
		p_index = 1-p_index;
		float res[2] = {(*L0)[list_quad], (*L1)[list_quad]};  // point at both the pressure data structures
		floatsamplebuffer[n] = res[p_index]; // get the latest one
	}

	//return inputBuffer;
	return floatsamplebuffer;
}

void DrumheadCPU::allocateSimulationData() {
	clearSimulationData(); // just in case...

	// +2 is the extra wall layer
	frameSize[0] = domainSize[0]+2;
	frameSize[1] = domainSize[1]+2;

	p          = new float **[frameSize[0]];
	excitation = new int *[frameSize[0]];
	beta       = new float *[frameSize[0]];

	for(int i=0; i<frameSize[0]; i++) {
		p[i]          = new float *[frameSize[1]];
		excitation[i] = new int [frameSize[1]];
		beta[i]       = new float [frameSize[1]];

		// for now no excitations
		memset(excitation[i], 0, frameSize[1]*sizeof(float));

		for(int j=0; j<frameSize[1]; j++) {
			p[i][j] = new float[2]; // current and previous p
			p[i][j][0] = 0;
			p[i][j][1] = 0;

			beta[i][j] = 1; // all air at the beginning
		}
	}

	// SSE stuff
	// resize and init
	BU.resize(domainSize[0]/2 + 2, vector<vec4>(domainSize[1]/2 + 2, vec4(1)) );  // all air
	BD.resize(domainSize[0]/2 + 2, vector<vec4>(domainSize[1]/2 + 2, vec4(1)) );  // all air
	BL.resize(domainSize[0]/2 + 2, vector<vec4>(domainSize[1]/2 + 2, vec4(1)) );  // all air
	BR.resize(domainSize[0]/2 + 2, vector<vec4>(domainSize[1]/2 + 2, vec4(1)) );  // all air
	EX.resize(domainSize[0]/2 + 2, vector<vec4>(domainSize[1]/2 + 2, vec4(0)) );   // no excitation

	PP[0].resize(domainSize[0]/2 + 2, vector<vec4>(domainSize[1]/2 + 2, vec4(0)) ); // no starting pressure
	PP[1].resize(domainSize[0]/2 + 2, vector<vec4>(domainSize[1]/2 + 2, vec4(0)) ); // no starting pressure

	// SSE listener
	int lx = (listenerCoord[0]-1)/2;
	int ly = (listenerCoord[1]-1)/2;

	int rx = (listenerCoord[0]-1)%2;
	int ry = (listenerCoord[1]-1)%2;

	list_quad = (ry<<1) | rx;

	L0 = &PP[0][lx+1][ly+1];
	L1 = &PP[1][lx+1][ly+1];

}

void DrumheadCPU::clearSimulationData() {
	if(p!=NULL) {
		for(int i=0; i<frameSize[0]; i++) {
			for(int j=0; j<frameSize[0]; j++)
				delete p[i][j];
			delete[] p[i];
			delete excitation[i];
			delete beta[i];
		}
		delete[] p;
		delete[] excitation;
		delete[] beta;

	}
}

void DrumheadCPU::initFrame() {
	// set frame walls
	for(int i=0; i<frameSize[0]; i++) {
		// left vertical boundary
		beta[i][0] = 0;

		// right vertical boundary
		beta[i][frameSize[1]-1] = 0;
	}
	for(int i=0; i<frameSize[1]; i++) {
		// top horizontal boundary
		beta[0][i] = 0;

		// bottom horizontal boundary
		beta[frameSize[0]-1][i] = 0;
	}
}
