/*
 * DrumheadGPU.h
 *
 *  Created on: 2016-03-14
 *      Author: Victor Zappi
 */

#ifndef DRUMHEADGPU_H_
#define DRUMHEADGPU_H_

#include <FDTD.h>

#include <sys/time.h> // for time test


class DrumheadGPU: public FDTD {
public:

	enum cell_type {cell_wall, cell_air, cell_excitation, cell_dead, cell_numOfTypes};

	DrumheadGPU();
	~DrumheadGPU();
	float *getBufferFloatReadTime(int numSamples, float *inputBuffer, int &readTime);
	float *getBufferFloatNoSampleRead(int numSamples, float *inputBuffer);
	void setDampingFactor(float dampFact);
	int setPropagationFactor(float propFact);
	void setClampingFactor(float clampFact);
	void disableRender();

	static float getPixelAlphaValue(int type);
	static void fillTexturePixel(int x, int y, float value, float ***pixels);
	static void fillTextureRow(int index, int start, int end, float value, float ***pixels);
	static void fillTextureColumn(int index, int start, int end, float value, float ***pixels);
private:
	void initFrame(int *excitationCoord, int *excitationDimensions, float ***domain);
	int initAdditionalUniforms();
	void initPhysicalParameters(); // simply to kill useless print
	void getSamplesReadTime(int numSamples, int &readTime);
	void setUpDampingFactor();
	void setUpPropagationFactor();
	void setUpClampingFactor();
	void updateUniforms();
	void updateUniformsRender();

	// utils
	static float getPixelBlueValue(int type);

	// damping factor
	float mu;
	GLint mu_loc;
	bool updateMu;

	// propagation factor, that combines spatial scale and speed in the medium
	float rho;
	GLint rho_loc;
	bool updateRho;

	// parameter to set walls between fully clamped or fully free
	float boundGain;
	GLint boundGain_loc;
	bool updateBoundGain;

	// time test
	timeval start_, end_;
};


inline void DrumheadGPU::setDampingFactor(float dampFact) {
	// we take for granted it's greater than 0
	mu = dampFact;
	updateMu = true;
}

inline int DrumheadGPU::setPropagationFactor(float propFact) {
	// stability check...and we take for granted it's greater than zero
	if(propFact > 0.5)
		return 1;

	rho = propFact;
	updateRho = true;

	return 0;
}

inline void DrumheadGPU::setClampingFactor(float clampFact) {
	// we take for granted it's greater than 0 and lower than 1
	boundGain = 1-clampFact;
	updateBoundGain = true;
}

// simply to attain to super class standard way of retrieving type [in this case is much simpler than FDTD class]
inline float DrumheadGPU::getPixelAlphaValue(int type) {
	return type;
}

inline void DrumheadGPU::disableRender() {
	render = false;
}
//-------------------------------------------------------------------------------------------
// private
//-------------------------------------------------------------------------------------------
inline void DrumheadGPU::initPhysicalParameters() {
	// NOP
}
inline void DrumheadGPU::setUpDampingFactor() {
	glUniform1f(mu_loc, mu);
}
inline void DrumheadGPU::setUpPropagationFactor() {
	glUniform1f(rho_loc, rho);
}
inline void DrumheadGPU::setUpClampingFactor() {
	glUniform1f(boundGain_loc, boundGain);
}
#endif /* DRUMHEADGPU_H_ */
