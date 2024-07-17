/*
 * controls.cpp
 *
 *  Created on: 2015-10-30
 *      Author: Victor Zappi
 */


#include "controls.h"

#include <signal.h>  // to catch ctrl-c
#include <string>
#include <array>
#include <vector>
#include <algorithm> // to handle vectors' elements

// for input reading
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/input.h>

// temporarily disable mouse control via multitouch interface
#include "disableMouse.h"

// monome osc controls
#include "monomeControl.h"


#define MAX_NUM_EVENTS 64 // found here http://ozzmaker.com/programming-a-touchscreen-on-the-raspberry-pi/


//back end
#include "HDH_AudioEngine.h"
#include "priority_utils.h"

// applications stuff
#include "HyperDrumSynth.h"
#include "DrumExcitation.h"
//#include "distances.h"

using namespace std;


extern int verbose;

//----------------------------------------
// main variables
//----------------------------------------
// extern
extern HDH_AudioEngine audioEngine;
extern HyperDrumSynth *hyperDrumSynth;
//----------------------------------------

//----------------------------------------
// control variables
//---------------------------------------
//extern int domainSize[2];
extern bool multitouch;
extern int monitorIndex;
extern string touchScreenName;
int touchScreenFileHandler = -1;
int windowPos[2] = {-1, -1};

extern int listenerCoord[2];
extern int areaListenerCoord[AREA_NUM][2];
extern double areaExcitationVol[AREA_NUM];
extern drumEx_type drumExType;

double deltaVolume = 0.05;

double areaExFreq[AREA_NUM] = {440.0};
double deltaFreq[AREA_NUM];
float oscFrequencies[FREQS_NUM] = {50, 100, 200, 400, 800, 1600, 3200, 6400};


extern double areaExLowPassFreq[FREQS_NUM];
double deltaLowPassFreq[AREA_NUM];

extern float areaDampFactor[AREA_NUM];
double deltaDampFactor = areaDampFactor[0]/100;

extern float areaPropagationFactor[AREA_NUM];
double deltaPropFactor = areaPropagationFactor[0]/100;

drumEx_type areaExciteType[AREA_NUM];


int **triggerInput_coord; // to keep track of what finger [in input ID] triggered what excitation point
int listenerInput; // to keep track of what finger [in input ID] is moving listener


// multitouch read
bool *touchOn; // to keep track of new touches and touch removals
float *touchPos[2];
bool *newTouch;
bool *touchNeedsRetrigger;
int *pressure;
// specs
int maxTouches;
int maxXpos;
int maxYpos;
int maxPressure;
int maxArea;

//---------------------------------
// exponential curve mapping [inited in initControlThread()]
// for non linear pressure->X and distance->X control
//---------------------------------
// general exponential curve values
double expMax;
double expMin;
double expRange;
double expNorm;
// special values for impulse case of pressure-volume control
double expMaxImp;
double expMinImp;
double expRangeImp;
double expNormImp;
// special values for boundary proximity-damping control
double expMaxDamp;
double expMinDamp;
double expRangeDamp;
double expNormDamp;

// input ranges for pressure control
double maxPress;
double minPress;
double rangePress;
// special values for impulse case of pressure control
double maxPressImp;
double minPressImp;
double rangePressImp;
// input ranges for excitation proximity control
double maxEProx;
double minEProx;
double rangeEProx;
// input ranges for boundary proximity control
double maxBProx;
double minBProx;
double rangeBProx;


// output ranges for volume control
double maxVolume;
double minVolume;
double rangeVolume;
// special values for impulse case of volume control
double maxVolumeImp;
double minVolumeImp;
double rangeVolumeImp;
// output ranges for filter cutoff control
double maxFilter;
double minFilter;
double rangeFilter;
// output ranges for propagation factor and damping controls are defined in pressFinger_range
//---------------------------------









GLFWwindow *window = NULL;
int winSize[2] = {-1, -1};

float mousePos[2]      = {-1, -1};
bool mouseLeftPressed   = false;
bool mouseRightPressed  = false;
bool mouseCenterPressed = false;
bool shiftIsPressed     = false;
bool altIsPressed       = false;
bool leftCtrlIsPressed  = false;
bool rightCtrlIsPressed = false;
bool spaceIsPressed = false;
/*bool zIsPressed = false;
bool xIsPressed = false;
bool cIsPressed = false;
bool vIsPressed = false;
int stickyZ = -1;
int stickyX = -1;
int stickyC = -1;*/
bool stickyNegated = false; // to erase with stickyZ or stickyX

bool drawBoundary_wait = false; // waiting for new touch? [if button is pressed]
bool *drawBoundary_id;
bool deleteBoundary_wait = false; // waiting for new touch? [if button is pressed]
bool *deleteBoundary_id;

int areaIndex = 0;

float bgain = 0;
float deltaBgain = 0.05;

bool showAreas = false;
//----------------------------------------







// this is called by ctrl-c
void ctrlC_handler(int s){
   printf("\nCaught signal %d\n",s);
   audioEngine.stopEngine();
}




double expMapping(double expMin, double expMax, double expRange, double expNorm,
				 double inMin, double inMax, double inRange,
				 double in, double outMin, double outRange) {
	if(in > inMax)
		in = inMax;
	else if(in < inMin)
		in = inMin;
	in = (in-inMin)/inRange;

	return exp(expMin+expRange*in)/expNorm*outRange + outMin;
}



//----------------------------------------












void getCellDomainCoords(int *coords) {
	// y is upside down
	coords[1] = winSize[1] - coords[1] - 1;

	// we get window coordinates, but this filter works with domain ones
	hyperDrumSynth->fromWindowToDomain(coords);
}

void changeMousePos(int mouseX, int mouseY) {
	int coord[2] = {(int)mouseX, (int)mouseY};
	getCellDomainCoords(coord);

	hyperDrumSynth->setMousePos(coord[0], coord[1]);
}


void slowMotion(bool slowMo) {
	hyperDrumSynth->setSlowMotion(slowMo);
}

void setBoundaryCell(int coordX, int coordY) {
	//float type = hyperDrumSynth->getCellType(coordX, coordY);

	if(hyperDrumSynth->setCell(coordX, coordY, areaIndex, HyperDrumhead::cell_boundary, bgain) != 0)
		return; // boundary setting failed

	// add new boundary to the list and update distances
	//dist_addBound(coordX, coordY);

	// we may have to update the excitations too
	//if(type==HyperDrumhead::cell_excitation)
		//dist_removeExcite(coordX, coordY);
}

void setBoundaryCell(int index, int coordX, int coordY) {
	//float type = hyperDrumSynth->getCellType(coordX, coordY);

	if(hyperDrumSynth->setCell(coordX, coordY, index, HyperDrumhead::cell_boundary, bgain) != 0)
		return; // boundary setting failed

	// add new boundary to the list and update distances
	//dist_addBound(coordX, coordY);

	// we may have to update the excitations too
	//if(type==HyperDrumhead::cell_excitation)
		//dist_removeExcite(coordX, coordY);
}

void setExcitationCell(int coordX, int coordY) {
	//float type = hyperDrumSynth->getCellType(coordX, coordY);
	//bool wasBoundary = (type==HyperDrumhead::cell_boundary)||(type==HyperDrumhead::cell_dead);

	if ( hyperDrumSynth->setCellType(coordX, coordY, HyperDrumhead::cell_excitation)!=0 ) // leave area as it is [safer?]
	//hyperDrumSynth->setCell(coordX, coordY, areaIndex, HyperDrumhead::cell_excitation); // changes also area [more prone to human errors?]
		return;

	// add new excitation to the list and update distances
	//dist_addExcite(coordX, coordY);

	// we may have to update the boundaries too
	//if(wasBoundary)
		//dist_removeBound(coordX, coordY);
}


bool checkBoundaries(int coordX, int coordY) {
	float type = hyperDrumSynth->getCellType(coordX, coordY);

	return (type==HyperDrumhead::cell_boundary)||(type==HyperDrumhead::cell_dead);
}

void setExcitationCell(int index, int coordX, int coordY) {
	/*
	float type = hyperDrumSynth->getCellType(coordX, coordY);
	bool wasBoundary = (type==HyperDrumhead::cell_boundary)||(type==HyperDrumhead::cell_dead);
	*/

	if( hyperDrumSynth->setCell(coordX, coordY, index, HyperDrumhead::cell_excitation)!=0 ) // changes also area [more prone to human errors?]
		return;

	/*
	// add new excitation to the list and update distances
	dist_addExcite(coordX, coordY);

	// we may have to update the boundaries too
	if(wasBoundary)
		dist_removeBound(coordX, coordY);
	*/
}

// this should be on its own medium prio thread?
void fillArea(int coordX, int coordY, int newArea) {
	int area = hyperDrumSynth->getCellArea(coordX, coordY);
	float type = hyperDrumSynth->getCellType(coordX, coordY);
	// if it's a different and is not frame border
	if(area != newArea && type != HyperDrumhead::cell_dead) {
		hyperDrumSynth->setCellArea(coordX, coordY, newArea); // change area

		// if it is not a boundary, continue to check neighbors
		if(type != HyperDrumhead::cell_boundary) {
			fillArea(coordX+1, coordY, newArea);
			fillArea(coordX, coordY+1, newArea);
			fillArea(coordX-1, coordY, newArea);
			fillArea(coordX, coordY-1, newArea);
		}
	}
}

void setAllCellsArea(int coordX, int coordY) {
/*	int area = hyperDrumSynth->getCellArea(coordX, coordX);
	// maybe there is no need...
	if(area==areaIndex)
		return;*/

	fillArea(coordX, coordY, areaIndex);
	//dist_scanExciteDist(); // we have to scan all areas, cos we moved more cells from some areas to another
}

void setCellArea(int coordX, int coordY) {
	hyperDrumSynth->setCellArea(coordX, coordY, areaIndex);
	//dist_scanExciteDist(); // we have to scan all areas, cos we move one cell from one area to another
}

void resetCell(int coordX, int coordY) {

	// check if we clicked on a boundary
	/*
	float type = hyperDrumSynth->getCellType(coordX, coordY);
	bool wasBoundary = (type==HyperDrumhead::cell_boundary)||(type==HyperDrumhead::cell_dead);
	*/

	// if we actually change something
	if( hyperDrumSynth->resetCellType(coordX, coordY)!=0 )
		return;

	/*
	// if it was a boundary [back to air], remove and update
	if(wasBoundary) {
		dist_removeBound(coordX, coordY);
		return;
	}

	// otherwise

	// check if was an excitation [back to air or boundary]
	if(type==HyperDrumhead::cell_excitation)
		dist_removeExcite(coordX, coordY);

	// then check if we reset a boundary
	type = hyperDrumSynth->getCellType(coordX, coordY);
	bool isBoundary = (type==HyperDrumhead::cell_boundary)||(type==HyperDrumhead::cell_dead);

	// in we did, add new boundary to the list and update distances
	if(isBoundary)
		dist_addBound(coordX, coordY);
	*/
}


void checkBoundaryEditing(int id) {
	// if we are waiting for a new drawing touch, here it is!
	if(drawBoundary_wait) {
		drawBoundary_id[id] = true;
		drawBoundary_wait = false;
		stopMonomePulse(0); // just in case it was activated via the monome
	}
	// if we are waiting for a new deleting touch, here it is!
	else if(deleteBoundary_wait) {
		deleteBoundary_id[id] = true;
		deleteBoundary_wait = false;
		stopMonomePulse(1); // just in case it was activated via the monome
	}
}

void clearDomain() {
	hyperDrumSynth->clearDomain();
	//dist_resetBoundDist();
	//dist_resetExciteDist();
}

void resetSimulation() {
	hyperDrumSynth->resetSimulation();
}



inline void triggerInputExcitation(int index) {
	hyperDrumSynth->triggerAreaExcitation(index);
	//printf("excite area %d!\n", index);
}

inline void dampAreaExcitation(int index) {
	hyperDrumSynth->dampAreaExcitation(index);
	//printf("excite area %d!\n", index);
}


void switchExcitationType(drumEx_type exciteType) {
	hyperDrumSynth->setExcitationType(exciteType);
	printf("Current excitation type: %s\n", DrumExcitation::excitationNames[exciteType].c_str());
}

void switchInputExcitationType(drumEx_type exciteType) {
	hyperDrumSynth->setAreaExcitationType(0, exciteType);
	areaExciteType[0] = exciteType;
	printf("Area %d excitation type: %s\n", 0, DrumExcitation::excitationNames[exciteType].c_str());
}

// from GLFW window coordinates
void moveListener(int coordX, int coordY) {
	// finally move listener
	if(hyperDrumSynth->setListenerPosition(coordX, coordY) == 0) {
		hyperDrumSynth->getListenerPosition(listenerCoord[0], listenerCoord[1]);
		//printf("Listener position (in pixels): (%d, %d)\n", listenerCoord[0], listenerCoord[1]); //get pos
	}
}

void shiftListener(char dir, int delta) {
	if(dir == 'h') {
		listenerCoord[0] = hyperDrumSynth->shiftListenerH(delta);
		printf("Listener position (in pixels): (%d, %d)\n", listenerCoord[0], listenerCoord[1]);
	}else if(dir == 'v') {
		listenerCoord[1] = hyperDrumSynth->shiftListenerV(delta);
		printf("Listener position (in pixels): (%d, %d)\n", listenerCoord[0], listenerCoord[1]);
	}
}

void moveAreaListener(int coordX, int coordY) {
	// finally move listener
	if(hyperDrumSynth->setAreaListenerPosition(areaIndex, coordX, coordY) == 0) {
		hyperDrumSynth->getAreaListenerPosition(areaIndex, areaListenerCoord[areaIndex][0], areaListenerCoord[areaIndex][1]);
		//printf("Area %d Listener position (in pixels): (%d, %d)\n", areaIndex, areaListenerCoord[areaIndex][0], areaListenerCoord[areaIndex][1]); //get pos
	}
}

void moveAreaListener(int index, int coordX, int coordY) {
	// finally move listener
	if(hyperDrumSynth->setAreaListenerPosition(index, coordX, coordY) == 0) {
		hyperDrumSynth->getAreaListenerPosition(areaIndex, areaListenerCoord[areaIndex][0], areaListenerCoord[areaIndex][1]);
		//printf("Area %d Listener position (in pixels): (%d, %d)\n", areaIndex, areaListenerCoord[areaIndex][0], areaListenerCoord[areaIndex][1]); //get pos
	}
}

void hideAreaListener() {
	hyperDrumSynth->hideAreaListener(areaIndex);
	printf("Area %d Listener hidden\n", areaIndex);

}

void shiftAreaListener(char dir, int delta) {
	if(dir == 'h')
		areaListenerCoord[areaIndex][0] = hyperDrumSynth->shiftAreaListenerH(areaIndex, delta);
	else if(dir == 'v')
		areaListenerCoord[areaIndex][1] = hyperDrumSynth->shiftAreaListenerV(areaIndex, delta);
	printf("Area %d Listener position (in pixels): (%d, %d)\n", areaIndex, areaListenerCoord[areaIndex][0], areaListenerCoord[areaIndex][1]); //get pos
}


void changeAreaExcitationVolume(double delta) {
	areaExcitationVol[areaIndex] += delta;
	hyperDrumSynth->setAreaExcitationVolume(areaIndex, areaExcitationVol[areaIndex]);
	printf("Area %d excitation volume: %.2f\n", areaIndex, areaExcitationVol[areaIndex]);
}

void setAreaExcitationVolume(int area, double v) {
	hyperDrumSynth->setAreaExcitationVolume(area, v);
	//printf("Area %d excitation volume: %.2f\n", area, v);
}

void setAreaExcitationVolumeFixed(int area, double v) {
	hyperDrumSynth->setAreaExcitationVolumeFixed(area, v);
	//printf("Area %d excitation volume: %.2f\n", area, v);
}

void changeAreaDampingFactor(float deltaDF, bool update=true, int area=-1) {
	if(area==-1)
		area = areaIndex;

	float nextD = areaDampFactor[area]+deltaDF;
	if(nextD >=0) {
		hyperDrumSynth->setAreaDampingFactor(area, nextD);
		if(update) {
			areaDampFactor[area] = nextD;
			deltaDampFactor = nextD/100.0; // keep ratio constant
			printf("Area %d damping factor: %f\n", area, nextD);
		}
		//printf("Area %d damping factor: %f\n", area, nextD);
	}
}

void changeAreaPropagationFactor(float deltaP, bool update=true, int area=-1) {
	if(area==-1)
		area = areaIndex;

	float nextP = areaPropagationFactor[area]+deltaP;
	if(nextP<0)
		nextP = 0.0001;
	else if(nextP>0.5)
		nextP = 0.5;

	hyperDrumSynth->setAreaPropagationFactor(area, nextP);
	if(update) {
		areaPropagationFactor[area] = nextP;
		printf("Area %d propagation factor: %f\n", area, nextP);
	}
	//printf("Area %d propagation factor: %f\n", area, nextP);
}
/*
void changeAreaPropFactorPressRange(float delta){
	pressFinger_range[areaIndex][press_prop] += delta;
	printf("Area %d pressure-propagation factor modifier range: %f\n", areaIndex, pressFinger_range[areaIndex][press_prop]);
}

void changeAreaDampFactorPressRange(float delta){
	pressFinger_range[areaIndex][press_dmp] += delta;
	printf("Area %d pressure-damping modifier range: %f\n", areaIndex, pressFinger_range[areaIndex][press_dmp]);
}*/

void changeAreaExcitationFreq(float deltaF) {
	float nextF = areaExFreq[areaIndex]+deltaF;
	if(nextF >=0) {
		areaExFreq[areaIndex] = nextF;
		hyperDrumSynth->setAreaExcitationFreq(areaIndex, nextF);
		deltaFreq[areaIndex] = nextF/100.0; // keep ratio constant
		printf("Area %d Excitation areaFreq: %f [Hz]\n", areaIndex, areaExFreq[areaIndex]);
	}
}


void changeAreaExcitationFixedFreq(int id) {
	areaExFreq[areaIndex] = oscFrequencies[id];
	deltaFreq[areaIndex] = areaExFreq[areaIndex]/100.0; // keep ratio constant
	hyperDrumSynth->setAreaExcitationFixedFreq(areaIndex, id);

	printf("Area %d Excitation freq: %f [Hz]\n", areaIndex, areaExFreq[areaIndex]);
}

void changeAreaLowPassFilterFreq(float deltaF) {
	float nextF = areaExLowPassFreq[areaIndex]+deltaF;
	if(nextF >=0 && nextF<= 22050) {
		hyperDrumSynth->setAreaExLowPassFreq(areaIndex, nextF);
		areaExLowPassFreq[areaIndex] = nextF;
		deltaLowPassFreq[areaIndex] = nextF/100.0;
		printf("Area %d excitation low pass freq: %f [Hz]\n", areaIndex, areaExLowPassFreq[areaIndex]);
	}
	else
		printf("Requested excitation low pass freq %f [Hz] is out range [0, 22050 Hz]\n", nextF);
}

void setAreaLowPassFilterFreq(int area, double f) {
	hyperDrumSynth->setAreaExLowPassFreq(area, f);
}

/*void setAreaExRelease(int area, double t) {
	hyperDrumSynth->setAreaExcitationRelease(area, t);
}*/

/*void changeAreaIndex(int index) {
	if(index<AREA_NUM) {
		areaIndex = index;
		hyperDrumSynth->setSelectedArea(areaIndex);
		printf("Current Area index: %d\n", areaIndex);
	}
	else
		printf("Cannot switch to Area index %d cos avalable areas are 0 to %d /:\n", index, AREA_NUM-1);
}*/

void changeBgain(float deltaB) {
	bgain += deltaB;
	if(bgain <0)
		bgain = 0;
	else if(bgain>1)
		bgain = 1;
	printf("Current bgain: %f\n", bgain);

}

/*void changeFirstFingerMode(motionFinger_mode mode, int negated=-1) {
	firstFingerMotion[areaIndex] = mode;
	printf("Area %d first finger mode set to %s\n", areaIndex, motion1stFinger_names[mode].c_str());
	if(negated==-1) {
		firstFingerMotionNegated[areaIndex] = false;
		return;
	}

	firstFingerMotionNegated[areaIndex] = negated;
	if(firstFingerMotionNegated[areaIndex])
		printf("[negated]\n");
}

void cycleFirstFingerPressureMode() {
	// increment mode
	int mode = (int)firstFingerPress[areaIndex];
	mode = ((mode+1)%press_cnt);
	firstFingerPress[areaIndex] = (pressFinger_mode)mode;
	printf("Area %d first finger pressure mode set to %s\n", areaIndex, press1stFinger_names[mode].c_str());
}

void changeThirdFingerMode(motionFinger_mode mode, int negated=-1) {
	thirdFingerMotion[areaIndex] = mode;
	printf("Area %d third finger mode set to %s\n", areaIndex, motion3rdFinger_names[mode].c_str());
	if(negated==-1) {
		thirdFingerMotionNegated[areaIndex] = false;
		return;
	}

	thirdFingerMotionNegated[areaIndex] = negated;
	if(thirdFingerMotionNegated[areaIndex])
		printf("[negated]\n");
}

void cycleThirdFingerPressureMode() {
	// increment mode
	int mode = (int)thirdFingerPress[areaIndex];
	mode = ((mode+1)%press_cnt);
	thirdFingerPress[areaIndex] = (pressFinger_mode)mode;
	printf("Area %d third finger pressure mode set to %s\n", areaIndex, press3rdFinger_names[mode].c_str());
}

void saveFrame() {
	int retval=hyperDrumSynth->saveFrame(AREA_NUM, areaDampFactor, areaPropagationFactor, areaExcitationVol, areaExLowPassFreq, areaExFreq, areaExciteType, areaListenerCoord,
										 (int *)firstFingerMotion, firstFingerMotionNegated, (int *)firstFingerPress, (int *)thirdFingerMotion, thirdFingerMotionNegated, (int *)thirdFingerPress,
										 pressFinger_range, pressFinger_delta, press_cnt, "test.frm");
	if(retval==0)
		printf("Current frame saved\n");
	else if(retval==-1)
		printf("HyperDrumhead not inited, can't save frame file...\n");
}

void openFrame() {
	string filename = "test.frm";
	int retval = hyperDrumSynth->openFrame(AREA_NUM, filename, areaDampFactor, areaPropagationFactor, areaExcitationVol, areaExLowPassFreq, areaExFreq, areaExciteType,
										   (int *)firstFingerMotion, firstFingerMotionNegated, (int *)firstFingerPress, (int *)thirdFingerMotion, thirdFingerMotionNegated, (int *)thirdFingerPress,
										   pressFinger_range, pressFinger_delta, press_cnt);
	if(retval==-1) {
		printf("HyperDrumhead not inited, can't open frame file...\n");
		return;
	}
	printf("Loaded frame from file \"%s\"\n", filename.c_str());

	//dist_scanBoundDist();
	//dist_scanExciteDist();
}*/




/*inline void setAreaVolPress(double press, int area) {
	double vol = expMapping(expMin, expMax, expRange, expNorm, minPress, maxPress, rangePress, press, minVolume, rangeVolume);
	setAreaExcitationVolumeFixed(area, vol*areaExcitationVol[area]);
}

inline void modulateAreaVolPress(double press, int area) {
	double vol = expMapping(expMin, expMax, expRange, expNorm, minPress, maxPress, rangePress, press, minVolume, rangeVolume);
	setAreaExcitationVolume(area, vol*areaExcitationVol[area]);
}

inline void setAreaImpPress(double press, int area) {
	double vol = expMapping(expMinImp, expMaxImp, expRangeImp, expNormImp, minPressImp, maxPressImp, rangePressImp, press, minVolumeImp, rangeVolumeImp);
	setAreaExcitationVolumeFixed(area, vol*areaExcitationVol[area]);
}

inline void modulateAreaFiltProx(double prox, int area) {
	double cutOff = expMapping(expMin, expMax, expRange, expNorm, minEProx, maxEProx, rangeEProx, prox, minFilter, rangeFilter);
	setAreaLowPassFilterFreq(area, cutOff*areaExLowPassFreq[area]);
	//printf("low pass %f [prox %f]\n", cutOff*areaExLowPassFreq[area], prox);
}

inline void modulateAreaDampProx(double prox, int area) {
	double deltaD = expMapping(expMinDamp, expMaxDamp, expRangeDamp, expNormDamp, minBProx, maxBProx, rangeBProx, prox, 0, pressFinger_range[area][press_dmp]);
	changeAreaDampingFactor(deltaD, false, area);
	//printf("boundProx: %f, delta: %f\n------------------\n", prox, deltaD);
}

inline void modulateAreaPropPress(double press, int area) {
	float delta = expMapping(expMin, expMax, expRange, expNorm, minPress, maxPress, rangePress, press, 0, pressFinger_range[area][press_prop]);
	changeAreaPropagationFactor(delta, false, area);
}

inline void modulateAreaDampPress(double press, int area) {
	float delta = expMapping(expMin, expMax, expRange, expNorm, minPress, maxPress, rangePress, press, 0, pressFinger_range[area][press_dmp]);
	changeAreaDampingFactor(delta, false, area);
}*/


void handleNewTouch(int coordX, int coordY, int touch) {
	int listenerDist = abs((areaListenerCoord[0][0]-coordX)*(areaListenerCoord[0][0]-coordX) +
			               (areaListenerCoord[0][1]-coordY)*(areaListenerCoord[0][1]-coordY) );
	//printf("listenerDist %d\n", listenerDist);

	if(listenerDist<3 && listenerInput==-2) { // listener touch
		// move listener
		moveAreaListener(0, coordX, coordY);
		listenerInput = touch;
	}
	else { // trigger touch
		if(!checkBoundaries(coordX, coordY)) {
			// add excitation
			setExcitationCell(touch, coordX, coordY);
			// trigger
			triggerInputExcitation(touch);
			// update record
			triggerInput_coord[touch][0] = coordX;
			triggerInput_coord[touch][1] = coordY;
		}
		else
			touchNeedsRetrigger[touch] = true;
	}
}

void handleTouchDrag(int coordX, int coordY, int touch) {
	// listener touch, let's move it
	if(listenerInput==touch)
		moveAreaListener(0, coordX, coordY);
	// trigger touch, let's move it
	else if(triggerInput_coord[touch][0] != -1) {
		if(!checkBoundaries(coordX, coordY)) {
			// remove old excitation
			resetCell(triggerInput_coord[touch][0], triggerInput_coord[touch][1]);
			// put new one, keeping it on
			setExcitationCell(touch, coordX, coordY);
			// update record
			triggerInput_coord[touch][0] = coordX;
			triggerInput_coord[touch][1] = coordY;

			if(touchNeedsRetrigger[touch]) {
				triggerInputExcitation(touch);
				touchNeedsRetrigger[touch] = false;
			}
		}
	}
}

void handleTouchRelease(int touch) {
	// it this was a drawing touch, drawing no more
	if(drawBoundary_id[touch])
		drawBoundary_id[touch] = false;
	// it this was a deleting touch, delete no more
	else if(deleteBoundary_id[touch])
		deleteBoundary_id[touch] = false;
	// it this was a listener touch, don't move listener anymore
	else if(listenerInput==touch)
		listenerInput = -2;
	// if this was a trigger touch, stop excitation
	else if(triggerInput_coord[touch][0] != -2) {
		// remove excitation
		resetCell(triggerInput_coord[touch][0], triggerInput_coord[touch][1]);
		// stop excitation
		dampAreaExcitation(touch); // and stop excitation
		// update record
		triggerInput_coord[touch][0] = -2;
		triggerInput_coord[touch][1] = -2;
	}
}


bool modifierActions(int coordX, int coordY, int touch) { // default is mouse press
	bool retval = false;


	if(drawBoundary_id[touch]) {
		setBoundaryCell(0,coordX, coordY);
		retval = true;
	}
	else if(deleteBoundary_id[touch]) {
		resetCell(coordX, coordY);
		retval = true;
	}

/*	// with space bar we physically select the current area to work on
	if(spaceIsPressed) {
		changeAreaIndex(area);
		retval = true;
	}*/


/*	if(xIsPressed) {
		if(!shiftIsPressed)
			setExcitationCell(area, coordX, coordY);
		else
			resetCell(coordX, coordY);
		retval = true;
	}
	else if(cIsPressed){
		if(!shiftIsPressed) {
			moveAreaListener(coordX, coordY);
		}
		else
			hideAreaListener();
		retval = true;
	}
	else if(vIsPressed) {
		if(shiftIsPressed)
			setAllCellsArea(coordX, coordY);
		else
			setCellArea(coordX, coordY);
		retval = true;
	}*/

/*	if(retval)
		changeMousePos(coordX, coordY);*/

	return retval;
}
/*

void modifierActionsFirstFinger(int slot, int area, int coordX, int coordY, float press) {
	if(firstFingerMotion[area]==motion_bound) {
		if(!shiftIsPressed && !firstFingerMotionNegated[area])
			setBoundaryCell(area,coordX, coordY);
		else
			resetCell(coordX, coordY);
	}
	else if(firstFingerMotion[area]==motion_ex) {
		if(!shiftIsPressed && !firstFingerMotionNegated[area])
			setExcitationCell(area, coordX, coordY);
		else
			resetCell(coordX, coordY);
	}
	else if(firstFingerMotion[area]==motion_list)
		moveAreaListener(area, coordX, coordY);

	// pressure
	if(firstFingerPress[area]==press_prop)
		modulateAreaPropPress(press, area);
	else if(firstFingerPress[area]==press_dmp)
		modulateAreaDampPress(press, area);
}

void modifierActionsThirdFinger(int area, int coordX, int coordY, float press) {
	if(thirdFingerMotion[area]==motion_bound) {
		if(!shiftIsPressed && !thirdFingerMotionNegated[area])
			setBoundaryCell(area,coordX, coordY);
		else
			resetCell(coordX, coordY);

		//changeMousePos(coordX, coordY);

	}
	else if(thirdFingerMotion[area]==motion_ex) {
		if(!shiftIsPressed && !thirdFingerMotionNegated[area])
			setExcitationCell(area, coordX, coordY);
		else
			resetCell(coordX, coordY);

		//changeMousePos(coordX, coordY);

	}
	else if(thirdFingerMotion[area]==motion_list)
		moveAreaListener(area, coordX, coordY);


	// pressure
	if(thirdFingerPress[area]==press_prop)
		modulateAreaPropPress(press, area);
	else if(thirdFingerPress[area]==press_dmp)
		modulateAreaDampPress(press, area);
}

bool checkFirstFinger(int slot, int area) {
	// if first finger is not linked to any moodifier
	if(firstFingerMotion[area]==motion_none && firstFingerPress[area]==press_none)
		return false;

	// if it's first touch, set it and return true
	if(firstFingerArea_touch[area]==-1 && thirdFingerTouch_area[slot]==-1) {
		firstFingerArea_touch[area] = slot;
		firstFingerTouch_area[slot] = area;
		return true;
	}

	// otherwise simply check if this was first touch already
	return (firstFingerArea_touch[area]==slot);
}

bool checkThirdFinger(int slot, int area) {
	// if third finger is not linked to any moodifier
	if(thirdFingerMotion[area]==motion_none && thirdFingerPress[area]==press_none)
		return false;

	// if it's third touch, set it and return true
	if(thirdFingerArea_touch[area]==-1) {
		thirdFingerArea_touch[area] = slot;
		thirdFingerTouch_area[slot] = area;
		return true;
	}

	// otherwise simply check if this was first touch already
	return (thirdFingerArea_touch[area]==slot);
}
*/






void mouseRightPressOrDrag(int coordX, int coordY) {
	resetCell(coordX, coordY);
}

void mouseLeftPress(int coordX, int coordY) {
	handleNewTouch(coordX, coordY, maxTouches);
}

void mouseLeftReleased() {
	mouseLeftPressed = false; // stop dragging
	handleTouchRelease(maxTouches);
}

void mouseLeftDrag(int coordX, int coordY) {
	handleTouchDrag(coordX, coordY, maxTouches);
}

// mouse buttons actions
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
	double xpos;
	double ypos;

	// left mouse down
    if(button == GLFW_MOUSE_BUTTON_LEFT) {
    	if(action == GLFW_PRESS) {
			glfwGetCursorPos (window, &xpos, &ypos);
			//printf("Mouse position: (%f, %f)\n", xpos, ypos);

			// turn to domain coords (:
			int coord[2] = {(int)xpos, (int)ypos};
			getCellDomainCoords(coord);

			checkBoundaryEditing(maxTouches); // maxTouches is mouse "touch" id

			if(!modifierActions(coord[0], coord[1], maxTouches)) {
				if(!mouseLeftPressed)
					mouseLeftPress(coord[0], coord[1]);
				else
					mouseLeftDrag(coord[0], coord[1]);
			}
			mouseLeftPressed = true;  // start dragging
			//printf("left mouse pressed\n");
    	} else if(action == GLFW_RELEASE) {
    		mouseLeftReleased();
    		//printf("left mouse released\n");
    	}
    }
	// right mouse down
    else  if(button == GLFW_MOUSE_BUTTON_RIGHT) {
    	if(action == GLFW_PRESS) {
			glfwGetCursorPos (window, &xpos, &ypos);

			// turn to domain coords (:
			int coord[2] = {(int)xpos, (int)ypos};
			getCellDomainCoords(coord);

			mouseRightPressOrDrag(coord[0], coord[1]);
			mouseRightPressed = true; // start dragging
			//printf("right mouse pressed\n");
		} else if(action == GLFW_RELEASE) {
			mouseRightPressed = false; // stop dragging
			//printf("right mouse released\n");
		}
    }
    // center mouse down
    else if(button == GLFW_MOUSE_BUTTON_MIDDLE) {
    	if(action == GLFW_PRESS) {
    		mouseCenterPressed = true;
    		//printf("central mouse pressed\n");
    	}
    	else {
    		mouseCenterPressed = false;
    		//printf("central mouse released\n");
    	}
    }
}

void cursor_position_callback(GLFWwindow *window, double xpos, double ypos) {
	//printf("Mouse Pos: %f %f\n", xpos, ypos);
	if(xpos != mousePos[0] || ypos != mousePos[1]) {
		mousePos[0] = xpos;
		mousePos[1] = ypos;
		changeMousePos(mousePos[0], mousePos[1]);
	}

	// turn to domain coords (:
	int coord[2] = {(int)xpos, (int)ypos};
	getCellDomainCoords(coord);

	// left mouse dragged
	if(mouseLeftPressed) {
		if(!modifierActions(coord[0], coord[1], maxTouches))
			mouseLeftDrag(coord[0], coord[1]);
	}
	// right mouse dragged
	else if(mouseRightPressed)
		mouseRightPressOrDrag(coord[0], coord[1]);
}

void cursor_enter_callback(GLFWwindow* window, int entered) {
    if (!entered){
        // The cursor exited the client area of the window
    	mousePos[0] = -1;
    	mousePos[1] = -1;
    	changeMousePos(mousePos[0], mousePos[1]);
    }
}

void win_pos_callback(GLFWwindow* window, int posx, int posy) {
	windowPos[0] = posx;
	windowPos[1] = posy;
	//printf("Win pos (%d, %d)\n", windowPos[0], windowPos[1]);
}

void key_callback(GLFWwindow *, int key, int scancode, int action, int mods) {
	if(action == GLFW_PRESS || action == GLFW_REPEAT) {

		// one shot press
		if(action != GLFW_REPEAT) {
			switch(key) {
				case GLFW_KEY_ESCAPE:
					audioEngine.stopEngine();
					break;
				case GLFW_KEY_LEFT_SHIFT:
				case GLFW_KEY_RIGHT_SHIFT:
					shiftIsPressed = true;
					break;
				case GLFW_KEY_LEFT_ALT:
				case GLFW_KEY_RIGHT_ALT:
					altIsPressed = true;
					break;
				case GLFW_KEY_RIGHT_CONTROL:
					rightCtrlIsPressed = true;
					break;
				case GLFW_KEY_LEFT_CONTROL:
					leftCtrlIsPressed = true;
					break;

				case GLFW_KEY_Z:
					drawBoundary_wait = true;
					break;
				case GLFW_KEY_X:
					deleteBoundary_wait = true;
					break;
			/*	case GLFW_KEY_C:
					cIsPressed = true;
					break;
				case GLFW_KEY_V:
					vIsPressed = true;
					break;*/

				case GLFW_KEY_0:
				case GLFW_KEY_KP_0:
					if(leftCtrlIsPressed) {
						if(!shiftIsPressed)
							switchInputExcitationType(drumEx_windowedImp);
						else
							switchInputExcitationType(drumEx_drone);
					}
					else  if(rightCtrlIsPressed)
						changeAreaExcitationFixedFreq(0);
					/*else
						changeAreaIndex(0);*/
					break;
				case GLFW_KEY_1:
				case GLFW_KEY_KP_1:
				    if(leftCtrlIsPressed) {
				    	if(!shiftIsPressed)
				    		switchInputExcitationType(drumEx_sin);
				    	else
				    		switchInputExcitationType(drumEx_pacific);
				    }
					else if(rightCtrlIsPressed)
						changeAreaExcitationFixedFreq(1);
					/*else
						changeAreaIndex(1);*/
					break;
				case GLFW_KEY_2:
				case GLFW_KEY_KP_2:
					 if(leftCtrlIsPressed) {
						if(!shiftIsPressed)
							switchInputExcitationType(drumEx_square);
						else
							switchInputExcitationType(drumEx_const);
					}
					else if(rightCtrlIsPressed)
						changeAreaExcitationFixedFreq(2);
					/*else
						changeAreaIndex(2);*/
					break;
				case GLFW_KEY_3:
				case GLFW_KEY_KP_3:
					if(leftCtrlIsPressed)
						switchInputExcitationType(drumEx_noise);
					else if(rightCtrlIsPressed)
						changeAreaExcitationFixedFreq(3);
					/*else
						changeAreaIndex(3);*/
					break;
				case GLFW_KEY_4:
				case GLFW_KEY_KP_4:
					if(leftCtrlIsPressed)
						switchInputExcitationType(drumEx_disco);
					else if(rightCtrlIsPressed)
						changeAreaExcitationFixedFreq(4);
					/*else
						changeAreaIndex(4);*/
					break;
				case GLFW_KEY_5:
				case GLFW_KEY_KP_5:
					if(leftCtrlIsPressed)
						switchInputExcitationType(drumEx_oscBank);
					else if(rightCtrlIsPressed)
						changeAreaExcitationFixedFreq(5);
					/*else
						changeAreaIndex(5);*/
					break;
				case GLFW_KEY_6:
				case GLFW_KEY_KP_6:
					if(leftCtrlIsPressed)
						switchInputExcitationType(drumEx_burst);
					else if(rightCtrlIsPressed)
						changeAreaExcitationFixedFreq(6);
					/*else
						changeAreaIndex(6);*/
					break;
				case GLFW_KEY_7:
				case GLFW_KEY_KP_7:
					if(leftCtrlIsPressed)
						switchInputExcitationType(drumEx_cello);
					else if(rightCtrlIsPressed)
						changeAreaExcitationFixedFreq(7);
					/*else
						changeAreaIndex(7)*/;
					break;
				case GLFW_KEY_8:
				case GLFW_KEY_KP_8:
					if(leftCtrlIsPressed)
						switchInputExcitationType(drumEx_drumloop);
					/*else
						changeAreaIndex(8);*/
					break;
				case GLFW_KEY_9:
				case GLFW_KEY_KP_9:
					if(leftCtrlIsPressed)
						switchInputExcitationType(drumEx_fxloop);
					/*else
						changeAreaIndex(9);*/
					break;


				// first finger interaction mode [for continuous control only]
				/*case GLFW_KEY_A:
					changeFirstFingerMode(motion_bound, shiftIsPressed);
					break;
				case GLFW_KEY_S:
					if(leftCtrlIsPressed || rightCtrlIsPressed)
						saveFrame();
					else
						changeFirstFingerMode(motion_ex, shiftIsPressed);
					break;
				case GLFW_KEY_D:
					changeFirstFingerMode(motion_list);
					break;
				case GLFW_KEY_F:
					cycleFirstFingerPressureMode();
					break;*/


				// third finger interaction mode [for continuous control only]
				/*case GLFW_KEY_Q:
					changeThirdFingerMode(motion_bound, shiftIsPressed);
					break;
				case GLFW_KEY_W:
					changeThirdFingerMode(motion_ex, shiftIsPressed);
					break;
				case GLFW_KEY_E:
					changeThirdFingerMode(motion_list);
					break;
				case GLFW_KEY_R:
					cycleThirdFingerPressureMode();
					break;*/

				// inhibit additional fingers' movement modifier
/*
				case GLFW_KEY_BACKSPACE:
					if(!shiftIsPressed)
						changeFirstFingerMode(motion_none);
					else
						changeThirdFingerMode(motion_none);
					break;
*/




				case GLFW_KEY_DELETE:
					clearDomain();
					break;

				case GLFW_KEY_ENTER:
					resetSimulation();
					break;

				case GLFW_KEY_SPACE:
					if(leftCtrlIsPressed || rightCtrlIsPressed)
						slowMotion(true);
					else
						spaceIsPressed = true;
					break;

/*				case GLFW_KEY_TAB:
					switchShowAreas();
					break;*/

				default:
					break;
			}
		}

		// pressed and kept pressed
		switch(key) {
			case GLFW_KEY_M:
				changeBgain(deltaBgain/(1 + altIsPressed*9));
				break;
			case GLFW_KEY_N:
				changeBgain(-deltaBgain/(1 + altIsPressed*9));
				break;

			case GLFW_KEY_L:
				/*if(shiftIsPressed)
					 changeAreaDampFactorPressRange(pressFinger_delta[press_dmp]/(1 + altIsPressed*9)); // use delta for pressure-damping modifier
				else*/
					changeAreaDampingFactor(deltaDampFactor/(1 + altIsPressed*9));
				break;
			case GLFW_KEY_K:
				/*if(shiftIsPressed)
					 changeAreaDampFactorPressRange(-pressFinger_delta[press_dmp]/(1 + altIsPressed*9)); // use delta for pressure-damping modifier
				else*/
					changeAreaDampingFactor(-deltaDampFactor/(1 + altIsPressed*9));
				break;

			case GLFW_KEY_P:
				/*if(shiftIsPressed)
					 changeAreaPropFactorPressRange(pressFinger_delta[press_prop]/(1 + altIsPressed*9)); // use delta for pressure-propagation modifier
				else*/
					changeAreaPropagationFactor(deltaPropFactor/(1 + altIsPressed*9));
				break;
			case GLFW_KEY_O:
				/* if(leftCtrlIsPressed || rightCtrlIsPressed)
					openFrame();
				 else if(shiftIsPressed)
					 changeAreaPropFactorPressRange(-pressFinger_delta[press_prop]/(1 + altIsPressed*9)); // use delta for pressure-propagation modifier
				 else*/
					 changeAreaPropagationFactor(-deltaPropFactor/(1 + altIsPressed*9));
				break;

			case GLFW_KEY_I:
				changeAreaExcitationVolume(deltaVolume/(1 + altIsPressed*9));
				break;
			case GLFW_KEY_U:
				changeAreaExcitationVolume(-deltaVolume/(1 + altIsPressed*9));
				break;

			case GLFW_KEY_Y:
				changeAreaLowPassFilterFreq(deltaLowPassFreq[areaIndex]/(1 + altIsPressed*9));
				break;
			case GLFW_KEY_T:
				changeAreaLowPassFilterFreq(-deltaLowPassFreq[areaIndex]/(1 + altIsPressed*9));
				break;

			case GLFW_KEY_J:
				changeAreaExcitationFreq(deltaFreq[areaIndex]/(1 + altIsPressed*9));
				break;
			case GLFW_KEY_H:
				changeAreaExcitationFreq(-deltaFreq[areaIndex]/(1 + altIsPressed*9));
				break;

			default:
				break;
		}
	} // release
	else if(action == GLFW_RELEASE) {
		switch(key) {
			case GLFW_KEY_LEFT_SHIFT:
			case GLFW_KEY_RIGHT_SHIFT:
				shiftIsPressed = false;
				break;
			case GLFW_KEY_LEFT_ALT:
			case GLFW_KEY_RIGHT_ALT:
				altIsPressed = false;
				break;
			case GLFW_KEY_RIGHT_CONTROL:
				rightCtrlIsPressed = false;
				break;
			case GLFW_KEY_LEFT_CONTROL:
				leftCtrlIsPressed = false;
				break;

			// modifiers
	/*		case GLFW_KEY_Z:
				drawBoundary_state = -3;
				break;
			case GLFW_KEY_X:
				deleteBoundary_state = -3;
				break;*/
/*			case GLFW_KEY_C:
				cIsPressed = false;
				break;
			case GLFW_KEY_V:
				vIsPressed = false;
				break;*/

				// sticky modifiers
			/*case GLFW_KEY_A:
				stickyZ = -2; // waiting
				printf("ready for sticky boundaries\n");
				if(shiftIsPressed) {
					stickyNegated = true;
					printf("[negated]\n");
				}
				break;
			case GLFW_KEY_S:
				if(leftCtrlIsPressed || rightCtrlIsPressed) {
					saveFrame();
					break;
				}
				stickyX = -2; // waiting
				printf("ready for sticky excitation\n");
				if(shiftIsPressed) {
					stickyNegated = true;
					printf("[negated]\n");
				}
				break;
			case GLFW_KEY_D:
				stickyC = -2; // waiting
				printf("ready for sticky listener\n");
				break;*/

			case GLFW_KEY_SPACE:
				slowMotion(false);
				spaceIsPressed = false;
				break;

			default:
				break;
		}
	}
}

// to handle window interaction
void initCallbacks() {
	// grab window handle
	window = hyperDrumSynth->getWindow(winSize); // try, but we all know it won't work |:
	while(window == NULL) {
		usleep(1000);			          // wait
		window = hyperDrumSynth->getWindow(winSize); // try again...patiently
	}

	// callbacks
	glfwSetMouseButtonCallback(window, mouse_button_callback); // mouse buttons down
	glfwSetCursorPosCallback(window, cursor_position_callback);
	glfwSetCursorEnterCallback(window, cursor_enter_callback);
	glfwSetKeyCallback(window, key_callback);

	if(multitouch) {
		glfwSetWindowPosCallback(window, win_pos_callback);
		glfwGetWindowPos(window, &windowPos[0], &windowPos[1]);
	}

	// let's catch ctrl-c to nicely stop the application
	struct sigaction sigIntHandler;
	sigIntHandler.sa_handler = ctrlC_handler;
	sigemptyset(&sigIntHandler.sa_mask);
	sigIntHandler.sa_flags = 0;
	sigaction(SIGINT, &sigIntHandler, NULL);
}


int initControlThread() {
	initCallbacks();

	for(int i=0; i<AREA_NUM; i++) {
		deltaLowPassFreq[areaIndex] = areaExLowPassFreq[i]/100.0;
		if(i>0)
			areaExFreq[i] = areaExFreq[0];
		deltaFreq[i] = areaExFreq[i]/100.0;
		areaDampFactor[i] = areaDampFactor[0];
		areaPropagationFactor[i] = areaPropagationFactor[0];
	}


	for(int i=0; i<AREA_NUM; i++) {
		areaExciteType[i] = drumExType;
	}


	// boundary distances handling
	//dist_init();


	// if no multitouch, we're done here (:
	if(!multitouch)
		return 0;

	//---------------------------------------------------------------------------
	// multitouch init
	//---------------------------------------------------------------------------

	// let's search the requested multitouch device
	string event_device_path = "/dev/input/event";
	int eventNum = 0; // to cycle all event devices' indices
	string event_device; // for info about of current device
	char name[256] = "Unknown"; // for the name current device

	do {
		event_device = event_device_path + to_string(eventNum); // build full path of current device

		touchScreenFileHandler = open(event_device.c_str(), O_RDONLY); // open it

		// reached end of files, requested device is not there
		if (touchScreenFileHandler == -1) {
			multitouch = false; // can't use multitouch ):
			break; // exit loop
		}

		// check name of current device
		ioctl(touchScreenFileHandler, EVIOCGNAME(sizeof(name)), name);
		string deviceName(name);
		size_t found = deviceName.find(touchScreenName);

		// if different from the one we look for
		if(found==string::npos) {
			close(touchScreenFileHandler); // close
			touchScreenFileHandler = -1;   // flag not found yet
			eventNum++; // try next one
			continue;
		}

		// otherwise it means we found it! exit loop
		break;
	}
	while(eventNum<100); // scan no more than 100 devices...which is a lot!


	if(!multitouch) {
		printf("Can't find multitouch device \"%s\"\n", touchScreenName.c_str());
		printf("\nSwitching off multitouch capabilities!\n\n");
		return 0;
	}


	printf("\nReading from:\n");
	printf("device file = %s\n", event_device.c_str());
	printf("device name = %s\n\n", name);

	// disable mouse control
	disableMouse(touchScreenName);

	// hide cursor
	int winSize[2];
	glfwSetInputMode(hyperDrumSynth->getWindow(winSize), GLFW_CURSOR, GLFW_CURSOR_HIDDEN);

	// read touch surface specs, same as evtest (:
	int abs[6] = {0};
	ioctl(touchScreenFileHandler, EVIOCGABS(ABS_MT_SLOT), abs);
	maxTouches = abs[2]+1;
	ioctl(touchScreenFileHandler, EVIOCGABS(ABS_MT_POSITION_X), abs);
	maxXpos = abs[2];
	ioctl(touchScreenFileHandler, EVIOCGABS(ABS_MT_POSITION_Y), abs);
	maxYpos = abs[2];
	ioctl(touchScreenFileHandler, EVIOCGABS(ABS_MT_PRESSURE), abs);
	maxPressure = abs[2];
	ioctl(touchScreenFileHandler, EVIOCGABS(ABS_MT_TOUCH_MAJOR), abs);
	maxArea = abs[2];

	// init touch vars
	touchOn     = new bool[maxTouches];
	touchPos[0] = new float[maxTouches];
	touchPos[1] = new float[maxTouches];
	newTouch    = new bool[maxTouches];
	touchNeedsRetrigger = new bool[maxTouches];
	pressure    = new int[maxTouches];
	drawBoundary_id   = new bool[maxTouches+1]; // +1 to treat mouse left down as touch
	deleteBoundary_id = new bool[maxTouches+1]; // +1 to treat mouse left down as touch

	triggerInput_coord = new int *[maxTouches+1]; // +1 to treat mouse left down as touch

	for(int i=0; i<maxTouches; i++) {
		touchOn[i] = false;
		drawBoundary_id[i]   = false;
		deleteBoundary_id[i] = false;

		triggerInput_coord[i] = new int[2];
		triggerInput_coord[i][0] = -1;
		triggerInput_coord[i][1] = -1;

	}
	listenerInput = -2; // unset as -2, cos -1 is border

	drawBoundary_id[maxTouches]   = false;  // last touch
	deleteBoundary_id[maxTouches] = false;  // lsat touch

	triggerInput_coord[maxTouches] = new int[2];
	triggerInput_coord[maxTouches][0] = -1;
	triggerInput_coord[maxTouches][1] = -1;


	// general exponential curve values
	expMax = 2;
	expMin = -8;
	expRange = expMax-expMin;
	expNorm  = exp(expMax);
	// special values for impulse case of pressure-volume control
	expMaxImp = 1;
	expMinImp = -3;
	expRangeImp = expMaxImp-expMinImp;
	expNormImp  = exp(expMaxImp);
	// special values for boundary proximity-damping control
	expMaxDamp = expMax-expMin*0.01;
	expMinDamp = expMin*0.99;
	expRangeDamp = expMaxDamp-expMinDamp;
	expNormDamp  = exp(expMaxDamp);

	// input ranges for pressure
	maxPress = maxPressure*0.7;
	minPress = 20;
	rangePress = maxPress-minPress;
	// special values for impulse case of pressure control
	maxPressImp = 230;
	minPressImp = 20;
	rangePressImp = maxPressImp-minPressImp;
	// input ranges for excitation proximity
	maxEProx = 1;
	minEProx = 0.0005;
	rangeEProx = maxEProx-minEProx;
	// input ranges for boundary proximity
	maxBProx = 1;
	minBProx = 0;
	rangeBProx = maxEProx-minEProx;

	// output ranges for volume control
	maxVolume = 10;
	minVolume = 0.001;
	rangeVolume = maxVolume-minVolume;
	// special values for impulse case of volume control
	maxVolumeImp = 2;
	minVolumeImp = 0.1;
	rangeVolumeImp = maxVolumeImp-minVolumeImp;
	// output ranges for filter cutoff control
	maxFilter = 1;
	minFilter = 0;
	rangeFilter = maxFilter-minFilter;
	// output ranges for propagation factor and damping controls are defined in pressFinger_range

	return 0;
}

void *runControlLoop(void *) {
	// first this here
	if(initControlThread()!=0) // cos it syncs with the audio thread and all its settings
		return (void*)0;

	if(verbose==1)
		cout << "__________set Control Thread priority" << endl;

	// set thread niceness
	//set_niceness(-20, verbose); // even if it should be inherited from calling process...but it's not

	 // set thread priority
	//set_priority(20, verbose);

	if(verbose==1)
		cout << "_________________Control Thread!" << endl;

	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS,NULL); // now this thread can be cancelled from main()





	//----------------------------------------------------------------------------------
	// keyboard controls
	//----------------------------------------------------------------------------------
	if(!multitouch) {
		char keyStroke = '.';
		cout << "Press q to quit." << endl;

		do {
			keyStroke =	getchar();

			while(getchar()!='\n'); // to read the first stroke

			switch (keyStroke) {
	/*			case 'a':
					SimSynth->gate(0);
					SimSynth->gate(1);
					cout << "attack" << endl;
					break;
				case 's':
					SimSynth->gate(0);
					cout << "release" << endl;
					break;
					break;
				case 'a':
					switchExcitationType(drumEx_windowedImp);
					break;
				case 'd':
					shiftListener('h', deltaPos);
					break;
				case 'w':
					shiftListener('v', deltaPos);
					break;
				case 's':
					shiftListener('v', -deltaPos);
					break;

				case 'z':
					break;
				case 'x':
					break;
		*/

				default:
					break;
			}
		}
		while (keyStroke != 'q');

		audioEngine.stopEngine();
		cout << "control thread ended" << endl;

		return (void *)0;
	}



	initMonome(bgain, drawBoundary_wait, deleteBoundary_wait, maxTouches+1);


	//----------------------------------------------------------------------------------
	// multitouch controls
	//----------------------------------------------------------------------------------
	// working vars
	const size_t ev_size = sizeof(struct input_event)*MAX_NUM_EVENTS;
	ssize_t domainSize;

	int currentSlot = 0;

	struct input_event ev[MAX_NUM_EVENTS]; // where to save event blocks

	// we need to know how big is the window [resolution], to check where we are touching
	GLFWmonitor* mon;
	if(monitorIndex==-1)
		mon = glfwGetPrimaryMonitor();
	else {
		int count = -1;
		mon = glfwGetMonitors(&count)[monitorIndex];
	}
	const GLFWvidmode* vmode = glfwGetVideoMode(mon);
	int monitorReso[] = {vmode->width, vmode->height};

	while(true) {
		domainSize = read(touchScreenFileHandler, &ev, ev_size); // we could use select() if we were interested in non-blocking read...i am not, TROLOLOLOLOL

		for (unsigned int i = 0;  i <  (domainSize / sizeof(struct input_event)); i++) {
			// let's consider only ABS events
			if(ev[i].type == EV_ABS) {
				// the first MT_SLOT determines what touch the following events will refer to
				if(ev[i].code == ABS_MT_SLOT) // this always arrives first! [except for the very first touch, which is assumed to be 0]
					currentSlot = ev[i].value;
				else if(ev[i].code == ABS_MT_TRACKING_ID) {
					if(ev[i].value != -1) {
						if(!touchOn[currentSlot]) {
							//printf("touch %d is ON\n", currentSlot);
							newTouch[currentSlot] = true;

							checkBoundaryEditing(currentSlot);
						}
						touchOn[currentSlot] = true;
					}
					else {
						//printf("touch %d OFF\n", currentSlot);
						touchOn[currentSlot] = false;

						handleTouchRelease(currentSlot);
					}
				}
				else if(ev[i].code == ABS_MT_POSITION_X) {
					touchPos[0][currentSlot] = (float(ev[i].value)/maxXpos) * monitorReso[0];
					//printf("touch %d pos X: %f\n", currentSlot, touchPos[0][currentSlot]);
					//printf("touch %d pos X: %d\n", currentSlot, ev[i].value);
				}
				else if(ev[i].code == ABS_MT_POSITION_Y) {
					touchPos[1][currentSlot] = (float(ev[i].value)/maxYpos) * monitorReso[1];
					//printf("touch %d pos Y: %f\n", currentSlot, touchPos[1][currentSlot]);

					/*pressure[currentSlot] = maxPressure*0.82;*/

					// if not touching window, fuck off
					if( !( (touchPos[0][currentSlot] >= windowPos[0])&&(touchPos[0][currentSlot] < windowPos[0]+winSize[0]) ) ||
						!( (touchPos[1][currentSlot] >= windowPos[1])&&(touchPos[1][currentSlot] < windowPos[1]+winSize[1]) ) ) {
						newTouch[currentSlot] = false;
						continue;
					}

					// translate into some kind of mouse pos, relative to window
					float posX = touchPos[0][currentSlot]-windowPos[0];
					float posY = touchPos[1][currentSlot]-windowPos[1];

					//printf("touch pos: %f %f\n", posX, posY);

					// then to domain coords (:
					int coord[2] = {(int)posX, (int)posY};
					getCellDomainCoords(coord);

					// check that no modifiers are active [mainly drawing/deleting boundaries]
					if(!modifierActions(coord[0], coord[1], currentSlot)) {
						// new touch
						if(newTouch[currentSlot]) {
							//printf("touch %d is NEW\n", currentSlot);
							handleNewTouch(coord[0], coord[1], currentSlot);
						}
						// touch is not new
						else {
							//printf("touch %d MOVE\n", currentSlot);
							handleTouchDrag(coord[0], coord[1], currentSlot);
						}
						changeMousePos(-1, -1);
					}
					else
						changeMousePos(touchPos[0][currentSlot], touchPos[1][currentSlot]);

					newTouch[currentSlot] = false; // always
				}
			}
		}
		usleep(1);
	}




	return (void *)0;
}


void cleanUpControlLoop() {
	// deallocate
	delete[] touchOn;
	delete[] touchPos[0];
	delete[] touchPos[1];
	delete[] newTouch;
	delete[] touchNeedsRetrigger;
	delete[] pressure;
	delete[] drawBoundary_id;
	delete[] deleteBoundary_id;
	for(int i=0; i<maxTouches+1; i++) // +1 for mouse "touch"
		delete[] triggerInput_coord[i];
	delete[] triggerInput_coord;

	//dist_cleanup();

	if(touchScreenFileHandler!=-1) {
		// re-enable mouse control
		enableMouse(touchScreenName);

		close(touchScreenFileHandler);
		printf("Multitouch device nicely shut (:\n");
	}

	closeMonome();
}


