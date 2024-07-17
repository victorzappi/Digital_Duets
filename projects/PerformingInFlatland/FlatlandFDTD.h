/*
 * FlatlandFDTD.h
 *
 *  Created on: 2016-03-14
 *      Author: Victor Zappi
 */

#ifndef VOCALTRACTFDTD_H_
#define VOCALTRACTFDTD_H_

#include "UIAreaButton.h"
#include "UIAreaSlider.h"

#include <ImplicitFDTD.h>
#include <vector> // for vector...
#include <cmath>

enum excitation_model {exMod_noFeedback, exMod_2massKees, exMod_2mass, exMod_bodyCover, exMod_clarinetReed, exMod_saxReed, exMod_num};

// physical values at T=26C [?] and in vocal tract/glottis
#define C_VT   350 //346.71     // DA SPEED OF SOUND!
#define RHO_VT 1.14        // DA MEAN DENSITY!
#define MU_VT  1.86e-5     // DA VISCOSITY!
#define PRANDTL_VT 0.76632 // DA PRANDTL NUMBER [ratio of momentum diffusivity to thermal diffusivity]
#define GAMMA_VT 1.4       // DA ADIABATIC INDEX [ratio of the heat capacity at constant pressure]

#define C_TUBES   346.361308136 // DA SPEED OF SOUND!
#define RHO_TUBES 1.14          // wrong, still to calculate
#define MU_TUBES  1.86e-5       // wrong, still to calculate
#define PRANDTL_TUBES 0.76632   // wrong, still to calculate
#define GAMMA_TUBES 1.4         // DA ADIABATIC INDEX [ratio of the heat capacity at constant pressure]

#define UI_AREA_BUTTONS_NUM 6
#define UI_AREA_SLIDERS_NUM 8

class FlatlandFDTD: public ImplicitFDTD {
public:
	FlatlandFDTD();
	~FlatlandFDTD();
	int init(string shaderLocation, int *domainSize, int *excitationCoord, int *excitationDimensions,
			 float ***domain, int audio_rate, int rate_mul, int periodSize, float mag=1, int *listCoord=NULL, float exLpF=22050);
	int init(string shaderLocation, int *domainSize, int audio_rate, int rate_mul, int periodSize,
			 float mag=1, int *listCoord=NULL, float exLpF=22050);
	float *getBuffer(int numSamples, float *inputBuffer);
	int setA1(float a1);
	int setQ(float q);
	int setPhysicalParameters(float c, float rho, float mu, float prandtl, float gamma);
	int setFilterToggle(int toggle);
	int setHideUI(bool hide);
	void setAreaButtons(float left[UI_AREA_BUTTONS_NUM], float bottom[UI_AREA_BUTTONS_NUM],
			               float w[UI_AREA_BUTTONS_NUM], float h[UI_AREA_BUTTONS_NUM]);
	int checkAreaButtonsTriggered(float gl_win_x, float gl_win_y, bool &newtrigger);
	void untriggerAreaButtons();
	void setAreaSliders(float left[UI_AREA_SLIDERS_NUM], float bottom[UI_AREA_SLIDERS_NUM], float w[UI_AREA_SLIDERS_NUM],
				        float h[UI_AREA_SLIDERS_NUM], float initVal[UI_AREA_SLIDERS_NUM], bool sticky[UI_AREA_SLIDERS_NUM]);
	int checkAreaSlidersValues(float gl_win_x, float gl_win_y, float &newvalue);
	bool *resetAreaSliders(bool stickyOnly);
	void resetAreaSlider(int index);
	float getAreaSliderVal(int index);

	// utils
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

	// UI
	float uiAlphaMin;
	float uiAlphaMax;
	// area buttons
	UIAreaButton areaButton[UI_AREA_BUTTONS_NUM];
	GLint areaButTrig_loc_renderer[UI_AREA_BUTTONS_NUM];
	bool updateAreaButTrig;
	int oldAreaButTriggered;
	// are sliders
	UIAreaSlider areaSlider[UI_AREA_SLIDERS_NUM];
	GLint areaSlidVal_loc_renderer[UI_AREA_SLIDERS_NUM];
	bool updateAreaSlidVal;
	int oldAreaSlidModified;
	float oldAreaSlidValue;
	bool areaSlidResetList[UI_AREA_SLIDERS_NUM];
	GLint hideUI_loc_render;
	bool hideUI;
	bool updateHideUI;

	void initPhysicalParameters();
	int initAdditionalUniforms();
	int initShaders();
	void updateUniforms();
	void updateCriticalUniforms();
	void setUpPhysicalParams();
	void setUpFilterToggle();
	void setUpGlottalParams();
	void setUpA1();
	void criticalSetUpA1();
	int loadContourFileIntoTexture(string filename, float ***pixels);
	void setUpAreaButTrigRender();
	void setUpAreaSlidValRender();
	void setUpHideUIRender();
	void updateUniformsRender();
	void renderWindow();

	// for utils
	static int parseContourPoints(int domain_size[2], float deltaS, string filename, float ** &points);
	static int parseContourPoints_new(int domain_size[2], float deltaS, string filename, float ** &lowerContour, float ** &upperContour, float * &angles);
	static void pixelateContour(int numOfPoints, float **contour, float *angles, float deltaS, vector<int> &coords, int min[2], bool lower);
	static void drawContour(float ***domainPixels, vector<int> lowerCoords, vector<int> upperCoords, int shift[2]);
	static double shiftAndRound(int shift, double val);
};


inline int FlatlandFDTD::setA1(float a1) {
	if(!inited)
		return -1;

	this->a1 = a1;
	// does not need a boolean, cos new a1 is compared with previous one at every sample

	return 0;
}

inline int FlatlandFDTD::setQ(float q) {
	if(!inited)
		return -1;

	if(q <= 0) {
		printf("Trying to set q parameter of 2 Mass model to %f! Must be greater than 0...\n", q);
		return 1;
	}

	this->q = q;
	updateGlottalParams = true;

	return 0;
}

inline int FlatlandFDTD::setFilterToggle(int toggle) {
	if(!inited)
		return -1;

	filter_toggle      = toggle;
	updateFilterToggle = true;

	return 0;
}

inline int FlatlandFDTD::setHideUI(bool hide) {
	if(!inited)
		return -1;

	hideUI       = hide;
	updateHideUI = true;

	return 0;
}

inline void FlatlandFDTD::setAreaButtons(float left[UI_AREA_BUTTONS_NUM], float bottom[UI_AREA_BUTTONS_NUM],
        									float w[UI_AREA_BUTTONS_NUM], float h[UI_AREA_BUTTONS_NUM]) {
	for(int i=0; i<UI_AREA_BUTTONS_NUM; i++)
		areaButton[i].init(left[i], bottom[i], w[i], h[i]);
}

inline void FlatlandFDTD::untriggerAreaButtons() {
	for(int i=0; i<UI_AREA_BUTTONS_NUM; i++)
		areaButton[i].untrigger();

	if(oldAreaButTriggered!=-1) {
		updateAreaButTrig = true;
		oldAreaButTriggered=-1;
	}
}

inline void FlatlandFDTD::setAreaSliders(float left[UI_AREA_SLIDERS_NUM], float bottom[UI_AREA_SLIDERS_NUM], float w[UI_AREA_SLIDERS_NUM],
		                                 float h[UI_AREA_SLIDERS_NUM], float initVal[UI_AREA_SLIDERS_NUM], bool sticky[UI_AREA_SLIDERS_NUM]) {
	for(int i=0; i<UI_AREA_SLIDERS_NUM; i++)
		areaSlider[i].init(left[i], bottom[i], w[i], h[i], initVal[i], sticky[i]);
}

inline void FlatlandFDTD::resetAreaSlider(int index) {
	areaSlider[index].reset();
}

inline float FlatlandFDTD::getAreaSliderVal(int index) {
	return areaSlider[index].getSliderVal();
}
//-----------------------------------------------------------------------------
// private
//-----------------------------------------------------------------------------

// be sure that all methods called in here are inline! and not too many...
inline void FlatlandFDTD::updateCriticalUniforms() {
	criticalSetUpA1();
}

inline void FlatlandFDTD::setUpGlottalParams() {
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

inline void FlatlandFDTD::setUpA1() {
	glUniform3f(a1Input_loc, a1Input[0], a1Input[1], a1Input[2] );
	checkError("glUniform1f a1Input");
}

// override of CoupleFDTD
inline void FlatlandFDTD::criticalSetUpA1() {
	// if nothing to update, don't bother
	if( (a1 == a1Input[0]) && ( a1Input[0] == a1Input[1]) && (a1Input[1] == a1Input[2]) )
		return;

	// shift and update latest value
	a1Input[2] = a1Input[1];
	a1Input[1] = a1Input[0];
	a1Input[0] = a1;

	setUpA1();
}

inline void FlatlandFDTD::setUpFilterToggle() {
	glUniform1i(filter_toggle_loc, filter_toggle);
	//checkError("glUniform1i filter_toggle");
}

inline void FlatlandFDTD::setUpPhysicalParams() {
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


inline void FlatlandFDTD::setUpAreaButTrigRender() {
	for(int i=0; i<UI_AREA_BUTTONS_NUM; i++) {
		glUniform1f(areaButTrig_loc_renderer[i], 0.3+areaButton[i].isTriggered()*0.3); // from [0, 1] to [0.3, 0.6] //VIC turn into variables
		checkError("glUniform1f areaButTrig");
	}
}

inline void FlatlandFDTD::setUpAreaSlidValRender() {
	float size[4];
	for(int i=0; i<UI_AREA_SLIDERS_NUM; i++) {
		areaSlider[i].getSize(size);
		glUniform1f(areaSlidVal_loc_renderer[i], (areaSlider[i].getSliderVal()*size[3] + size[1])*0.5 + deltaCoordY*audioRowH); // from [0, 1] to [bottom*0.5, (bottom+height)*0.5] -> basically we infer the last y frag coordinate of the mouse
		checkError("glUniform1f areaButTrig");
	}
}

inline void FlatlandFDTD::setUpHideUIRender() {
	glUniform1i(hideUI_loc_render, hideUI);
}
#endif /* VOCALTRACTFDTD_H_ */
