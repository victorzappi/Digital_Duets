/*
 * HyperDrumhead.h
 *
 *  Created on: 2016-03-14
 *      Author: Victor Zappi
 */

#ifndef HYPERDRUMHEAD_H_
#define HYPERDRUMHEAD_H_

#include <FDTD.h>
#include "HyperPixelDrawer.h"

#include <sys/time.h> // for time test

#define AREA_NUM 1


class HyperDrumhead: public FDTD {
public:

	enum cell_type {cell_boundary, cell_air, cell_excitation, cell_dead, cell_numOfTypes};

	static const int areaNum;

	HyperDrumhead();
	~HyperDrumhead();
	float *getBufferFloatAreas(int numSamples, float *inputBuffer, float **areaInputBuffer);
	float *getBufferFloatNoSampleRead(int numSamples, float *inputBuffer);
	GLFWwindow *getWindow(int *size); //VIC drawer
	void setAreaDampingFactor(int index, float dampFact);
	int setAreaPropagationFactor(int index, float propFact);
	void disableRender();
	int setAreaListenerPosition(int index, int x, int y);
	void getAreaListenerPosition(int index, int &x, int &y);
	int shiftAreaListenerV(int index, int delta = 1);
	int shiftAreaListenerH(int index, int delta = 1);
	int hideListener();
	int hideAreaListener(int index);
	int clearDomain(); //VIC drawer
	int saveFrame(hyperDrumheadFrame *frame, string filename=""); //VIC drawer
	int openFrame(string filename, hyperDrumheadFrame *&frame); //VIC drawer
	int setCellType(int x, int y, float type);
	float getCellType(int x, int y);
	int setCellArea(int x, int y, float area);
	int setCell(int x, int y, float area, float type, float bgain=-1);
	int resetCellType(int x, int y);
	float getCellArea(int x, int y);
	int setShowAreas(bool show);
	int setSelectedArea(int area);
	static float getPixelAlphaValue(int type);
	static void fillTexturePixel(int x, int y, float value, float ***pixels);
	static void fillTextureRow(int index, int start, int end, float value, float ***pixels);
	static void fillTextureColumn(int index, int start, int end, float value, float ***pixels);
private:
	void postInit();
	int fillVertices();
	void initFrame(int *excitationCoord, int *excitationDimensions, float ***domain);
	int initAdditionalUniforms();
	void initPhysicalParameters(); // simply to kill useless print
	int computeAreaListenerPos(int index, int x, int y);
	int computeAreaListenerFrag(int index, int x, int y);
	int computeAreaListenerFragCrossfade(int index, int x, int y);
	void computeNextAreaListenerFrag(int index);
	void computeSamplesAreas(int numSamples, float *inputBuffer, float **areainputBuffer);
	void renderWindow();
	void getSamples(int numSamples);
	void updatePixelDrawer(); //VIC drawer
	void updatePixelDrawerFrame(); //VIC drawer
	int initPixelDrawer(); //VIC drawer
	void setUpAreaDampingFactor(int index);
	void setUpAreaPropagationFactor(int index);
	void updateUniforms();
	void updateUniformsRender();
	void setUpAreaListener(int index);
	void setUpAreaListenerRender(int index);
	void setUpShowAreas();
	void setUpSelectedArea();

	// make them private
	using FDTD::setCell;
	using FDTD::resetCell;

	// utils
	static float getPixelBlueValue(int type);

	// areas handling
	int numOfAreas;

	// input handling
	int numOfInputs;

	// area listeners
	GLint **areaListenerFragCoord_loc;
	GLint *areaListenerFragCoord_loc_render;
	float ***areaListenerFragCoord;
	int **areaListenerCoord;
	bool *updateAreaListenter;

	GLint **areaListenerBFragCoord_loc;
	GLint *areaListenerBFragCoord_loc_render;
	float ***areaListenerBFragCoord;
	int **areaListenerBCoord;
	bool *updateAreaListenerRender;

	GLint **nextAreaListenerFragCoord_loc;
	float ***nextAreaListenerFragCoord;

	bool doCrossfade;
	GLint crossfade_loc;
	float crossfade;
	float crossfadeDelta;
	char currentDeck;


	// area excitations
	GLint *areaExciteInput_loc;

	// area damping factors
	float *areaMu;
	GLint *areaMu_loc;
	bool *updateAreaMu;

	// area propagation factors, that combine spatial scale and speed in the medium
	float *areaRho;
	GLint *areaRho_loc;
	bool *updateAreaRho;

	// pixel drawer
	HyperPixelDrawer *hyperPixelDrawer;

	// area visualizer
	int showAreas;
	GLint showAreas_loc_render;
	bool updateShowAreas;
	GLint selectedArea;
	GLint selectedArea_loc_render;
	bool updateSelectedArea;

	// for more constant FPS
	int blindBuffNum;
	int blindBuffCnt;
	int fps;

	// slow motion
	int slowFrameCnt;

	float *silenceBuffer;

	// time test
	timeval start_, end_;
};

inline GLFWwindow *HyperDrumhead::getWindow(int *size) {
	if(hyperPixelDrawer == NULL)
		return NULL;

	return TextureFilter::getWindow(size);
}

inline void HyperDrumhead::setAreaDampingFactor(int index, float dampFact) {
	// we take for granted it's greater than 0
	areaMu[index] = dampFact; //@VIC this is freaking temp!!!
	updateAreaMu[index] = true;
}

inline int HyperDrumhead::setAreaPropagationFactor(int index, float propFact) {
	int retval = 0;
	// stability check...and we take for granted it's greater than zero
	if(propFact > 0.5) {
		propFact = 0.5;
		retval = 1;
	}
	areaRho[index] = propFact; //@VIC this is freaking temp!!!
	updateAreaRho[index] = true;

	return retval;
}

// simply to attain to super class standard way of retrieving type [in this case is much simpler than FDTD class]
inline float HyperDrumhead::getPixelAlphaValue(int type) {
	return type;
}

inline void HyperDrumhead::disableRender() {
	render = false;
}

inline int HyperDrumhead::saveFrame(hyperDrumheadFrame *frame, string filename) {
	if(inited && (hyperPixelDrawer != NULL))
		return hyperPixelDrawer->dumpFrame(frame, filename);
	else
		return -1;
}

inline int HyperDrumhead::openFrame(string filename, hyperDrumheadFrame *&frame) {
	if(inited && (hyperPixelDrawer != NULL)) {
		int retval = hyperPixelDrawer->openFrame(filename, frame);
		if(retval == 0)
			/*updateFramePixels*/updatePixels = true; //VIC updateFramePixels would be used when frame loader is part of frame sequencer, thus when there is continuous transition between air and wall
		return retval;
	}
	return -1;
}
//-------------------------------------------------------------------------------------------
// private
//-------------------------------------------------------------------------------------------
inline void HyperDrumhead::postInit() {
	FDTD::postInit();
	blindBuffNum = (audio_rate/period_size) / fps;

	crossfadeDelta = 1.0/(period_size-1);

	silenceBuffer = new float[period_size];
	memset(silenceBuffer, 0, period_size*sizeof(float));
}
inline void HyperDrumhead::initPhysicalParameters() {
	// NOP
}
inline void HyperDrumhead::setUpAreaDampingFactor(int index) {
	glUniform1f(areaMu_loc[index], areaMu[index]);
}
inline void HyperDrumhead::setUpAreaPropagationFactor(int index) {
	glUniform1f(areaRho_loc[index], areaRho[index]);
}
inline void HyperDrumhead::setUpAreaListener(int index) {
	for(int j=0; j<numOfStates; j++)
		glUniform2f(nextAreaListenerFragCoord_loc[index][j], nextAreaListenerFragCoord[index][j][0], nextAreaListenerFragCoord[index][j][1]); // listener for state 0 holds value for sate numOfStates-1 (:
}
inline void HyperDrumhead::setUpAreaListenerRender(int index) {
	// we always render quad 0, no matter what state it is, cos we go so fast that we don't care if we're slightly out of date...
	// this makes easier to detect the listener, cos we don't have to cope with multi-state coordinates
	glUniform2f(areaListenerFragCoord_loc_render[index], nextAreaListenerFragCoord[index][0][0], nextAreaListenerFragCoord[index][0][1]); // listener for state 0 holds value for sate numOfStates-1 (:
}
inline void HyperDrumhead::setUpShowAreas() {
	glUniform1i(showAreas_loc_render, showAreas);
}
inline void HyperDrumhead::setUpSelectedArea() {
	glUniform1i(selectedArea_loc_render, selectedArea);
}
inline void HyperDrumhead::updatePixelDrawerFrame() {
/*	if(updateFramePixels) {
		int excitationCells = -1;
		int retval = hyperPixelDrawer->updateFrame(state, excitationCells);
		// still has to finish
		if (retval == 0)
			updateFramePixels = true;
		// still needs to run but update excitations cells
		else if (retval == 1) {
			updateFramePixels = true;
			excitationCellNum = excitationCells;
			setUpExCellNum(); // we call it directly here because this happens during the sample cycle, before the fbo update section and after the render update [that puts updateExCellNum oto false]
			if(render)
				updateExCellNum = true;
		}
		// it's finished, simply update number of excitation cells
		else if (retval == 2){
			updateFramePixels = false;
			excitationCellNum = excitationCells;
			setUpExCellNum(); // we call it directly here because this happens during the sample cycle, before the fbo update section and after the render update [that puts updateExCellNum oto false]
			if(render)
				updateExCellNum = true;
		}
	}*/
}
#endif /* HYPERDRUMHEAD_H_ */
