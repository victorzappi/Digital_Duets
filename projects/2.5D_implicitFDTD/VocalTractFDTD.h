/*
 * VocalTractFDTD.h
 *
 *  Created on: 2016-03-14
 *      Author: Victor Zappi
 */

#ifndef VOCALTRACTFDTD_H_
#define VOCALTRACTFDTD_H_

#include <ImplicitFDTD.h>
#include <vector> // for vector...
#include <cmath>

enum excitation_model {ex_noFeedback, ex_2massKees, ex_2mass, ex_bodyCover, ex_clarinetReed, ex_saxReed, ex_num};

// physical values at T=26C [?] and in vocal tract/glottis
#define C_VT   350 //346.71     // DA SPEED OF SOUND!
#define RHO_VT 1.14        // DA MEAN DENSITY!
#define MU_VT  1.86e-5     // DA VISCOSITY!
#define PRANDTL_VT 0.76632 // DA PRANDTL NUMBER [ratio of momentum diffusivity to thermal diffusivity]
#define GAMMA_VT 1.4       // DA ADIABATIC INDEX [ratio of the heat capacity at constant pressure]

#define C_AEROPHONES   347.23     // DA SPEED OF SOUND!
#define RHO_AEROPHONES 1.1760     // DA MEAN DENSITY!
#define MU_AEROPHONES 1.846e-5    // DA VISCOSITY!
#define PRANDTL_AEROPHONES 0.7073 // DA PRANDTL NUMBER!
#define GAMMA_AEROPHONES 1.4017   // DA ADIABATIC INDEX!

// same, but calculated when Aerophones runs at zero Celsius [standard temperature]
#define C_AEROPHONES_ZERO   315.664      // DA SPEED OF SOUND!
#define RHO_AEROPHONES_ZERO 1.38756      // DA MEAN DENSITY!
#define MU_AEROPHONES_ZERO 1.59817e-5    // DA VISCOSITY!
#define PRANDTL_AEROPHONES_ZERO 0.722493 // DA PRANDTL NUMBER!
#define GAMMA_AEROPHONES_ZERO 1.40321    // DA ADIABATIC INDEX!

#define C_TUBES   346.361308136 // DA SPEED OF SOUND!
#define RHO_TUBES 1.14          // wrong, still to calculate
#define MU_TUBES  1.86e-5       // wrong, still to calculate
#define PRANDTL_TUBES 0.76632   // wrong, still to calculate
#define GAMMA_TUBES 1.4         // DA ADIABATIC INDEX [ratio of the heat capacity at constant pressure]

//------------------------------
// width map
//------------------------------
#define WIDTH_TEXT_ISOLATION_LAYER_R 1
#define WIDTH_TEXT_ISOLATION_LAYER_U 1

#define MIN_WIDTH 0.00001//0.0000000001//0.00001
#define WALL_WIDTH MIN_WIDTH
#define OPEN_SPACE_WIDTH 0.5
#define NO_RADIATION_WIDTH -1
//------------------------------


//TODO extract from this class a new Coupled2_5DFDTD
class VocalTractFDTD: public ImplicitFDTD {
public:
	VocalTractFDTD();
	~VocalTractFDTD();
	int init(string shaderLocation, int *domainSize, int *excitationCoord, int *excitationDimensions, float ***domain, int audio_rate,
			 int rate_mul, int periodSize, float ***widthMap, float openSpaceWidth, float mag=1, int *listCoord=NULL, float exLpF=22050);
	int init(string shaderLocation, int *domainSize, int audio_rate, int rate_mul, int periodSize,
			 float ***widthMap, float openSpaceWidth, float mag=1, int *listCoord=NULL, float exLpF=22050);
	float *getBuffer(int numSamples, double *inputBuffer);
	int setA1(float a1);
	int setQ(float q);
	int setPhysicalParameters(void *params);
	int setFilterToggle(int toggle);
	int setAbsorptionCoefficient(float alpha);
	// utils
	static float alphaFromAdmittance(float mu);
	static int loadContourFile(int domain_size[2], float deltaS, string filename, float ***domainPixels); // turns contour file into texture to be used as domain [returned] with setDomain()!
	static int loadContourFile_new(int domain_size[2], float deltaS, string filename, float ***domainPixels); // turns contour file into texture to be used as domain [returned] with setDomain()!

private:
	// physical params
	// these are needed in the exictation models
	GLint rho_loc; // these must be sent to fbo rather than just saved on the CPU side
	GLint c_loc;
	GLint rho_c_loc;
	// viscothermal filter
	GLint mu_loc;
	float mu;
	float prandtl;
	float gamma;
	GLint viscothermal_coefs_loc;
	GLint filter_toggle_loc;
	int filter_toggle;
	bool updateFilterToggle;
	// reed, which is not properly vocal tract...whatever
	GLint inv_ds_loc;

	// glottis, see fbo frag shader for explanation
	float gs;
	float q;
	float Ag0;
	float m1;
	float m2;
	float d1;
	float d2;
	float k1;
	float k2;
	float kc;
	GLint dampingMod_loc;
	GLint pitchFactor_loc;
	GLint restArea_loc;
	GLint masses_loc;
	GLint thicknesses_loc;
	GLint springKs_loc;
	float a1;
	float a1Input[3]; // registers for next A1 and 2 old values
	GLint a1Input_loc;
	//bool updateA1;
	bool updateGlottalParams;

	// excitations
	int numOfExcitationFiles;
	string *excitationFiles;
	excitationModels excitationMods;

	// boundary layers
	bool absorption;
	GLint z_inv_loc;
	float z_inv;
	bool updateZ_inv;
	float alpha;

	Texture2D widthTexture;
	float *widthTexturePixels;

	void initImage(int domainWidth, int domainHeigth);
	int fillVertices();
	void initPhysicalParameters();
	int initAdditionalUniforms();
	int initShaders();
	void computeWallImpedance();
	void updateUniforms();
	void updateCriticalUniforms();
	void setUpPhysicalParams();
	void setUpFilterToggle();
	void setUpGlottalParams();
	void setUpA1();
	void setUpWallImpedance();
	void criticalSetUpA1();
	int loadContourFileIntoTexture(string filename, float ***pixels);
	void fillWidthTexture(float ***widthMap, float openSpaceWidth);

	// for utils
	static int parseContourPoints(int domain_size[2], float deltaS, string filename, float ** &points);
	static int parseContourPoints_new(int domain_size[2], float deltaS, string filename, float ** &lowerContour, float ** &upperContour, float * &angles);
	static void pixelateContour(int numOfPoints, float **contour, float *angles, float deltaS, vector<int> &coords, int min[2], bool lower);
	static void drawContour(float ***domainPixels, vector<int> lowerCoords, vector<int> upperCoords, int shift[2]);
	static double shiftAndRound(int shift, double val);
};

// be sure that all methods called in here are inline! and not too many...
inline void VocalTractFDTD::updateCriticalUniforms() {
	criticalSetUpA1();
}

inline void VocalTractFDTD::setUpGlottalParams() {
	// 2 mass and 2 mass Kees
	glUniform1f(dampingMod_loc, gs);
	//checkError("glUniform1f dampingMod");

	glUniform1f(pitchFactor_loc, q);
	//checkError("glUniform1f pitchFactorFbo");

	glUniform1f(restArea_loc, Ag0);
	//checkError("glUniform1f restArea");

	glUniform2f(masses_loc, m1, m2);
	//checkError("glUniform2f masses");

	glUniform2f(thicknesses_loc, d1, d2);
	//checkError("glUniform2f thicknesses");

	glUniform3f(springKs_loc, k1, k2, kc);
	//checkError("glUniform2f springKs");
}

inline void VocalTractFDTD::setUpA1() {
	glUniform3f(a1Input_loc, a1Input[0], a1Input[1], a1Input[2] );
	checkError("glUniform1f a1Input");
}

// override of CoupleFDTD
inline void VocalTractFDTD::criticalSetUpA1() {
	// if nothing to update, don't bother
	if( (a1 == a1Input[0]) && ( a1Input[0] == a1Input[1]) && (a1Input[1] == a1Input[2]) )
		return;

	// shift and update latest value
	a1Input[2] = a1Input[1];
	a1Input[1] = a1Input[0];
	a1Input[0] = a1;

	setUpA1();
}

inline void VocalTractFDTD::setUpFilterToggle() {
	glUniform1i(filter_toggle_loc, filter_toggle);
	//checkError("glUniform1i filter_toggle");
}

inline void VocalTractFDTD::setUpPhysicalParams() {
	// these are the only differences from ImplicitFDTD::setUpPhysicalParams()
	glUniform1f(rho_loc, rho);
	//checkError("glUniform1f rho");

	glUniform1f(mu_loc, mu);
	//checkError("glUniform1f mu");

	glUniform1f(c_loc, c);
	checkError("glUniform1f rho");

	glUniform1f(rho_c_loc, c*rho);
	checkError("glUniform1f mu");

	if(!absorption) {
		// calculate viscothermal!
		float l_nu = sqrt(mu / rho);
		float l_t = l_nu / sqrt(prandtl);
		float common_factor = 1.0f / (rho * c * c * 1.41421356237/*sqrt(2.0f)*/);

		float viscothermal_A = - l_nu * common_factor;
		float viscothermal_B = -(gamma - 1) * l_t * common_factor;

		glUniform2f(viscothermal_coefs_loc, viscothermal_A, viscothermal_B);
	}

	glUniform1f(inv_ds_loc, 1.0/ds);

	ImplicitFDTD::setUpPhysicalParams();
}

inline void VocalTractFDTD::setUpWallImpedance() {
	glUniform1f(z_inv_loc, z_inv);
}


//-------------------------------------------------------------------------------------------
// private
//-------------------------------------------------------------------------------------------
inline void VocalTractFDTD::computeWallImpedance() {
	z_inv = 1 / (rho*c*( (1+sqrt(1-alpha))/(1-sqrt(1-alpha)) )); // we actually pass the admittance to the shader
}

#endif /* VOCALTRACTFDTD_H_ */
