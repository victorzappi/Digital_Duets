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
#include <list>

// for input reading
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/input.h>

// temporarily disable mouse control via multitouch interface
#include "disableMouse.h"



#define MAX_NUM_EVENTS 64 // found here http://ozzmaker.com/programming-a-touchscreen-on-the-raspberry-pi/


//back end
#include "HyperDrumheadEngine.h"
#include "priority_utils.h"

// applications stuff
#include "HyperDrumSynth.h"
#include "controls_midi.h"
#include "controls_osc.h"
#include "controls_GLFW.h"
#include "HyperPresetManager.h"
//VIC dist #include "distances.h"


#define MIN_DIST 5


using namespace std;


extern int verbose;

//----------------------------------------
// main variables
//----------------------------------------
// extern
extern HDH_AudioEngine audioEngine;
extern HyperDrumSynth *hyperDrumSynth;
extern bool audioeReady;
//----------------------------------------

// location variable
extern string presetFileFolder;

//----------------------------------------
// control variables
//---------------------------------------
extern int domainSize[2];
extern bool multitouch;
extern int monitorIndex;
extern string touchScreenName;
int touchScreenFileHandler = -1;
int windowPos[2] = {-1, -1};

extern int listenerCoord[2];
extern int areaListenerCoord[AREA_NUM][2];
extern double areaExcitationVol[AREA_NUM];
extern int drumExId;


double areaModulatedVol[AREA_NUM];

int deltaPos        = 1;
double deltaVolume = 0.05;

double masterVolume = 1;

double areaExFreq[AREA_NUM] = {440.0};
double deltaFreq[AREA_NUM];
float oscFrequencies[FREQS_NUM] = {50, 100, 200, 400, 800, 1600, 3200, 6400};

bool areaOnHold[AREA_NUM];
int keepHoldArea_touch[AREA_NUM];
int *keepHoldTouch_area;
bool holdOnPressure[AREA_NUM][2];
int areaLastPressure[AREA_NUM];


extern double areaExLowPassFreq[FREQS_NUM];
double deltaLowPassFreq[AREA_NUM];

//VIC dist extern double areaExRelease[AREA_NUM];

extern float areaDampFactor[AREA_NUM];
double areaDeltaDampFactor[AREA_NUM];// = areaDampFactor[0]/10;

extern float areaPropagationFactor[AREA_NUM];
double areaDeltaPropFactor[AREA_NUM];// = areaPropagationFactor[0]/100;

int areaExciteID[AREA_NUM];
list<int> triggerArea_touches[AREA_NUM]; // to keep track of what finger triggered every area
list<pair<int, int>> *triggerArea_touch_coords[AREA_NUM]; // as well as position...note it's a pointer cos more excitations at the same time if area's percussive
int *triggerTouch_area; // reverse
bool *touchChangedAreaOnPreset;

string fingerMotion_names[motion_cnt] = {"trigger [and pressure]", "boundaries", "excitation", "listener"};
motionFinger_mode firstPreFingerMotion[AREA_NUM] = {motion_none};
motionFinger_mode *preFingersTouch_motion; // get pre motion for each pre finger
bool firstPreFingerMotionNegated[AREA_NUM] = {false}; // to erase with pre two modes
bool *preFingersTouch_motionNegated;
int firstPreFingerArea_touch[AREA_NUM]; // to keep track of what is pre finger in every area
list<int> preFingersArea_touches[AREA_NUM];
int *preFingersTouch_area; // reverse

//string fingerMotion_names[motion_cnt] = {"inactive [pressure only]", "boundaries", "excitation", "listener"};
motionFinger_mode postFingerMotion[AREA_NUM] = {motion_list};
bool postFingerMotionNegated[AREA_NUM] = {false}; // to erase with pre two modes
int postFingerArea_touch[AREA_NUM]; // to keep track of what is post finger in every area
int *postFingerTouch_area; // reverse




string fingerPress_names[press_cnt] = {"inactive", "propagation factor", "absorption"};
pressFinger_mode firstPreFingerPress[AREA_NUM] = {press_none};
pressFinger_mode *preFingersTouch_press; // get pre motion for each pre finger


//string fingerPress_names[press_cnt] = {"inactive", "propagation factor", "absorption"};
pressFinger_mode postFingerPress[AREA_NUM] = {press_none};

float pressFinger_range[AREA_NUM][MAX_PRESS_MODES]; // defined in HyperPixelDrawer.h, cos they are saved!
float pressFinger_delta[AREA_NUM][press_cnt];

bool nextPreFingerMotion_dynamic_wait = false;
motionFinger_mode nextPreFingerMotion_dynamic = motion_none;
bool nextPreFingerPress_dynamic_wait = false;
pressFinger_mode preFingersPress_dynamic = press_none;
bool nextPreFingerMotion_dynamic_negated = false;
bool *is_preFingerMotion_dynamic;
bool *is_preFingerPress_dynamic;


// for displax fucking line breakage
int *touch_elderTwin;   // to keep track of twin touches that are created on broken lines
int *touch_youngerTwin;
int *touch_coords[2];


// multitouch read
bool *touchOn; // to keep track of new touches and touch removals
float *touchPos[2];
bool *newTouch;
int *pressure;
bool *posReceived[2];
bool *touchOnBoundary[2]; // to see if new touch happens on boundary and to keep track of previous position [was it on boundary?] -> for continuous excitation
int *prevTouchedArea;
int *boundTouch_areaListener;
float *touchPos_prev[2]; // to fix displax bug!
bool *extraTouchDone; // to fix displax bug!
//float **extraTouchPos;
bool *touchIsInhibited; // to fix problems with broken lines on displax!
int excitationDeck[AREA_NUM];
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


HyperPresetManager presetManager;
/*
bool updateNextPreset = false;
bool updatePrevPreset = false;
*/

GLFWwindow *window = NULL;
int winSize[2] = {-1, -1};

float mousePos[2]      = {-1, -1};
bool mouseLeftPressed   = false;
bool mouseRightPressed  = false;
bool mouseCenterPressed = false;

// fixed modifiers and controls [activated via keyboard only]
bool modNegated  = false;
bool modIndex    = false;
bool modBound    = false;
bool modExcite   = false;
bool modListener = false;
bool modArea     = false;


bool mouseButton = false;
bool cursorPos   = false;
bool cursorEnter = false;
bool winPos      = false;


int areaIndex = 0;

float bgain = 1;
float deltaBgain = 0.05;

bool showAreas = false;
//----------------------------------------


string *excitationNames;
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

void setMasterVolume(double v, bool exp) {
	//printf("vol in %f\n", v);
	if(!exp)
		masterVolume = v;
	else
		masterVolume = expMapping(expMinImp, expMaxImp, expRangeImp, expNormImp, 0, 1, 1, v, 0, 1);

	//printf("masterVolume %f\n", masterVolume);

	for(int i=0; i<AREA_NUM; i++)
		hyperDrumSynth->setAreaExcitationVolume(i, areaExcitationVol[i]*masterVolume);
}

//----------------------------------------

// can't be inline cos used also in other files [and cannot put in controls.h cos its needs hyperDrumSynth]
inline void setAreaDampingFactor(int area, float damp) {
	hyperDrumSynth->setAreaDampingFactor(area, damp);
}

// can't be inline cos used also in other files [and cannot put in controls.h cos its needs hyperDrumSynth]
inline void setAreaPropagationFactor(int area, float prop) {
	hyperDrumSynth->setAreaPropagationFactor(area, prop);

}

// returns in terms of domain size
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
	//VIC dist float type = hyperDrumSynth->getCellType(coordX, coordY);

	if(hyperDrumSynth->setCell(coordX, coordY, areaIndex, HyperDrumhead::cell_boundary, bgain) != 0)
		return; // boundary setting failed

	// add new boundary to the list and update distances
	//VIC dist dist_addBound(coordX, coordY);

	// we may have to update the excitations too
	//VIC dist if(type==HyperDrumhead::cell_excitation)
	//VIC dist dist_removeExcite(coordX, coordY);
}

void setBoundaryCell(int index, int coordX, int coordY) {
	//VIC dist float type = hyperDrumSynth->getCellType(coordX, coordY);

	if(hyperDrumSynth->setCell(coordX, coordY, index, HyperDrumhead::cell_boundary, bgain) != 0)
		return; // boundary setting failed

	// add new boundary to the list and update distances
	//VIC dist dist_addBound(coordX, coordY);

	// we may have to update the excitations too
	//VIC dist if(type==HyperDrumhead::cell_excitation)
	//VIC dist dist_removeExcite(coordX, coordY);
}

void setExcitationCell(int coordX, int coordY) {
	//VIC dist float type = hyperDrumSynth->getCellType(coordX, coordY);
	//VIC dist bool wasBoundary = (type==HyperDrumhead::cell_boundary)||(type==HyperDrumhead::cell_dead);

	if ( hyperDrumSynth->setCellType(coordX, coordY, HyperDrumhead::cell_excitation)!=0 ) // leave area as it is [safer?]
	//hyperDrumSynth->setCell(coordX, coordY, areaIndex, HyperDrumhead::cell_excitation); // changes also area [more prone to human errors?]
		return;

	// add new excitation to the list and update distances
	//VIC dist dist_addExcite(coordX, coordY);

	// we may have to update the boundaries too
	//VIC dist if(wasBoundary)
	//VIC dist dist_removeBound(coordX, coordY);
}

void setExcitationCell(int index, int coordX, int coordY) {
	//VIC dist float type = hyperDrumSynth->getCellType(coordX, coordY);
	//VIC dist bool wasBoundary = (type==HyperDrumhead::cell_boundary)||(type==HyperDrumhead::cell_dead);

	if( hyperDrumSynth->setCell(coordX, coordY, index, HyperDrumhead::cell_excitation)!=0 ) // changes also area [more prone to human errors?]
		return;

	// add new excitation to the list and update distances
	//VIC dist dist_addExcite(coordX, coordY);

	// we may have to update the boundaries too
	//VIC dist if(wasBoundary)
	//VIC dist dist_removeBound(coordX, coordY);
}

void setExcitationCellDeck(int index, int coordX, int coordY) {
	//VIC dist float type = hyperDrumSynth->getCellType(coordX, coordY);
	//VIC dist bool wasBoundary = (type==HyperDrumhead::cell_boundary)||(type==HyperDrumhead::cell_dead);

	excitationDeck[index] = hyperDrumSynth->getAreaExcitationDeck(index); 	//@VIC

	//printf("area %d set deck to %d\n", index, excitationDeck[index]);

	// target is current deck, cos we do not need crossfade on first trigger touch
	HyperDrumhead::cell_type type = (excitationDeck[index]==0) ? HyperDrumhead::cell_excitationA : HyperDrumhead::cell_excitationB;
	excitationDeck[index] = 1-excitationDeck[index]; // this is the next deck though, to which we'll crossfade at the next touch

	if( hyperDrumSynth->setCell(coordX, coordY, index, type)!=0 ) // changes also area [more prone to human errors?]
		return;

	// add new excitation to the list and update distances
	//VIC dist dist_addExcite(coordX, coordY);

	// we may have to update the boundaries too
	//VIC dist if(wasBoundary)
	//VIC dist dist_removeBound(coordX, coordY);
}

void setExcitationCrossfade(int index, int coordExX, int coordExY, int coordRstX, int coordRstY) {
	//printf("area %d next deck to %d\n", index, excitationDeck[index]);
	//printf("area %d switch deck!\n", index);
	excitationDeck[index] = hyperDrumSynth->setCellCrossExcite(coordRstX, coordRstY, coordExX, coordExY, index); // returns next deck
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
	//VIC dist dist_scanExciteDist(); // we have to scan all areas, cos we moved more cells from some areas to another
}

void setCellArea(int coordX, int coordY) {
	hyperDrumSynth->setCellArea(coordX, coordY, areaIndex);
	//VIC dist dist_scanExciteDist(); // we have to scan all areas, cos we move one cell from one area to another
}

void resetCell(int coordX, int coordY) {
	// check if we clicked on a boundary
	float type = hyperDrumSynth->getCellType(coordX, coordY);
	bool wasBoundary = (type==HyperDrumhead::cell_boundary)||(type==HyperDrumhead::cell_dead);

	// if we actually change something
	if( hyperDrumSynth->resetCellType(coordX, coordY)!=0 )
		return;

	// if it was a boundary [back to air], remove and update
	if(wasBoundary) {
		//VIC dist dist_removeBound(coordX, coordY);
		return;
	}

	// otherwise

	// check if was an excitation [back to air or boundary]
	//VIC dist if(type==HyperDrumhead::cell_excitation)
	//VIC dist dist_removeExcite(coordX, coordY);

	// then check if we reset a boundary
	type = hyperDrumSynth->getCellType(coordX, coordY);
	//VIC dist bool isBoundary = (type==HyperDrumhead::cell_boundary)||(type==HyperDrumhead::cell_dead);

	// in we did, add new boundary to the list and update distances
	//VIC dist if(isBoundary)
	//VIC dist dist_addBound(coordX, coordY);
}

void clearDomain() {
	hyperDrumSynth->clearDomain();
	//VIC dist dist_resetBoundDist();
	//VIC dist dist_resetExciteDist();
}

void resetSimulation() {
	hyperDrumSynth->resetSimulation();
}

void switchShowAreas() {
	showAreas = !showAreas;
	hyperDrumSynth->setShowAreas(showAreas);
}

int getAreaIndex(int coordX, int coordY) {
	return hyperDrumSynth->getCellArea(coordX, coordY);
}

void triggerAreaExcitation(int coordX, int coordY) {
	int index = getAreaIndex(coordX, coordY);
	hyperDrumSynth->triggerAreaExcitation(index);
	//printf("excite area %d!\n", index);
}

// can't be inline cos used also in other files [and cannot put in controls.h cos its needs hyperDrumSynth]
/*inline*/ void triggerAreaExcitation(int index) {
	hyperDrumSynth->triggerAreaExcitation(index);
	//printf("++++++++++++++++++excite area %d!\n", index);
}

// can't be inline cos used also in other files [and cannot put in controls.h cos its needs hyperDrumSynth]
/*inline*/ void dampAreaExcitation(int index) {
	hyperDrumSynth->dampAreaExcitation(index);
	//printf("damp area %d!\n", index);
}

void excite(bool all=false) {
	if(all) {
		hyperDrumSynth->triggerExcitation();
		//printf("excite all!\n");
	}
	else {
		hyperDrumSynth->triggerAreaExcitation(areaIndex);
		//printf("excite area %d!\n", areaIndex);
	}
}

/*
void switchExcitationType(drumEx_type exciteType) {
	hyperDrumSynth->setExcitationType(exciteType);
	printf("Current excitation type: %s\n", excitationNames[exciteType].c_str());
}
*/
bool checkPercussive(int area) {
	return hyperDrumSynth->getAreaExcitationIsPercussive(area);
}

bool checkPercussive(int area, int id) {
	return hyperDrumSynth->getAreaExcitationIsPercussive(area, id);
}

void setAreaExcitationID(int area, int id) {
	hyperDrumSynth->setAreaExcitationID(area, id);
	areaExciteID[areaIndex] = hyperDrumSynth->getAreaExcitationId(areaIndex);
	printf("Area %d excitation type: %s\n", area, excitationNames[areaExciteID[area]].c_str());
}

void setAreaExcitationID(int id) {
	hyperDrumSynth->setAreaExcitationID(areaIndex, id);
	areaExciteID[areaIndex] = hyperDrumSynth->getAreaExcitationId(areaIndex);
	if(/*areaExciteID[areaIndex]==PERCUSSIVE*/ checkPercussive(areaIndex))
		areaModulatedVol[areaIndex] = 1; // reset, cos otherwise last modulated value might persist
	printf("Area %d excitation type: %s\n", areaIndex, excitationNames[areaExciteID[areaIndex]].c_str());

	/*int currentSlot = triggerArea_touch[areaIndex];
	triggerArea_touch[areaIndex] = -1; // reset area
	if(currentSlot>-1) // to cope with mouse pointer case
		triggerTouch_area[currentSlot] = -1; // reset touch
	 */

	// reset all touches that may be triggering
	/*for(int slot : triggerArea_touches[areaIndex])
		triggerTouch_area[slot] = -1;
	triggerArea_touches[areaIndex].clear(); // then reset area*/
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

void moveAreaListener(int area, int coordX, int coordY) {
	// finally move listener
	if(hyperDrumSynth->setAreaListenerPosition(area, coordX, coordY) == 0) {
		hyperDrumSynth->getAreaListenerPosition(area, areaListenerCoord[area][0], areaListenerCoord[area][1]);
		//printf("Area %d Listener position (in pixels): (%d, %d)\n", area, areaListenerCoord[area][0], areaListenerCoord[area][1]); //get pos
	}
}

inline void hideAreaListener(int area) {
	hyperDrumSynth->hideAreaListener(area);
	areaListenerCoord[area][0] = -2;
	areaListenerCoord[area][1] = -2;
	printf("Area %d Listener hidden\n", area);
}

inline void hideAreaListener() {
	hyperDrumSynth->hideAreaListener(areaIndex);
	areaListenerCoord[areaIndex][0] = -2;
	areaListenerCoord[areaIndex][1] = -2;
	printf("Area %d Listener hidden\n", areaIndex);
}

inline void moveAreaEraser(int area, int coordX, int coordY) {
	hyperDrumSynth->setAreaEraserPosition(area, coordX, coordY);
}

inline void hideAreaEraser(int area) {
	hyperDrumSynth->hideAreaEraser(area);
}


void shiftAreaListener(char dir, int delta) {
	if(dir == 'h')
		areaListenerCoord[areaIndex][0] = hyperDrumSynth->shiftAreaListenerH(areaIndex, delta);
	else if(dir == 'v')
		areaListenerCoord[areaIndex][1] = hyperDrumSynth->shiftAreaListenerV(areaIndex, delta);
	printf("Area %d Listener position (in pixels): (%d, %d)\n", areaIndex, areaListenerCoord[areaIndex][0], areaListenerCoord[areaIndex][1]); //get pos
}


void changeAreaExcitationVolume(double delta) {
	double nextVol =  areaExcitationVol[areaIndex] + deltaVolume*delta;
	areaExcitationVol[areaIndex] = (nextVol>=0) ? nextVol : 0;

	hyperDrumSynth->setAreaExcitationVolume(areaIndex, areaExcitationVol[areaIndex]*areaModulatedVol[areaIndex]*masterVolume); // modulated volume to make this transition smooth while on continuous excitation
	printf("Area %d excitation volume: %.2f [master volume: %2.f]\n", areaIndex, areaExcitationVol[areaIndex], masterVolume);
}

void setAreaExcitationVolume(int area, double v) {
	hyperDrumSynth->setAreaExcitationVolume(area, v*masterVolume);
	//printf("Area %d excitation volume: %.2f [master volume: %2.f]\n", area, v, masterVolume);
}

void setAreaExcitationVolumeFixed(int area, double v) {
	hyperDrumSynth->setAreaExcitationVolumeFixed(area, v*masterVolume);
	//printf("Area %d excitation volume: %.2f [master volume: %2.f]\n", area, v, masterVolume);
}

void changeAreaDampingFactor(float deltaD/*, bool update, int area*/) {
/*	if(area==-1)
		area = areaIndex;*/

	float nextD = areaDampFactor[areaIndex]+areaDeltaDampFactor[areaIndex]*deltaD;
	if(nextD <0)
		nextD = 0;
	/*if(nextD >=0)*/ {
		setAreaDampingFactor(areaIndex, nextD);//hyperDrumSynth->setAreaDampingFactor(area, nextD);
		//if(update) {
			areaDampFactor[areaIndex] = nextD;
			areaDeltaDampFactor[areaIndex] = nextD*0.1; // keep ratio constant
			printf("Area %d damping factor: %f\n", areaIndex, nextD);
		//}
		//printf("Area %d damping factor: %f\n", area, nextD);
	}
}

void changeAreaPropagationFactor(float deltaP/*, bool update, int area*/) {
	/*if(area==-1)
		area = areaIndex;*/

	float nextP = areaPropagationFactor[areaIndex]+areaDeltaPropFactor[areaIndex]*deltaP;
	if(nextP<=0)
		nextP = 0.0001;
	else if(nextP>0.5)
		nextP = 0.5;

	setAreaPropagationFactor(areaIndex, nextP); //hyperDrumSynth->setAreaPropagationFactor(area, nextP);
	//if(update) {
		areaPropagationFactor[areaIndex] = nextP;
		areaDeltaPropFactor[areaIndex] = nextP*0.01;
		printf("Area %d propagation factor: %f\n", areaIndex, nextP);
	//}
	//printf("Area %d propagation factor: %f\n", area, nextP);
}

void changeAreaPropFactorPressRange(float delta){
	pressFinger_range[areaIndex][press_prop] += areaDeltaPropFactor[areaIndex]*delta;
	printf("Area %d pressure-propagation factor modifier range: %f [current value: %f]\n", areaIndex, pressFinger_range[areaIndex][press_prop], areaPropagationFactor[areaIndex]);
}

void changeAreaDampFactorPressRange(float delta){
	pressFinger_range[areaIndex][press_dmp] += areaDeltaDampFactor[areaIndex]*delta;
	printf("Area %d pressure-damping modifier range: %f [current value: %f]\n", areaIndex, pressFinger_range[areaIndex][press_dmp], areaDampFactor[areaIndex]);
}

void changeAreaExcitationFreq(float deltaF) {
	float nextF = areaExFreq[areaIndex]+deltaF;
	if(nextF >=0) {
		areaExFreq[areaIndex] = nextF;
		//hyperDrumSynth->setAreaExcitationFreq(areaIndex, nextF);
		deltaFreq[areaIndex] = nextF/100.0; // keep ratio constant
		printf("Area %d Excitation areaFreq: %f [Hz]\n", areaIndex, areaExFreq[areaIndex]);
	}
}


void changeAreaExcitationFixedFreq(int id) {
	areaExFreq[areaIndex] = oscFrequencies[id];
	deltaFreq[areaIndex] = areaExFreq[areaIndex]/100.0; // keep ratio constant
	//hyperDrumSynth->setAreaExcitationFixedFreq(areaIndex, id);

	printf("Area %d Excitation freq: %f [Hz]\n", areaIndex, areaExFreq[areaIndex]);
}

void changeAreaLowPassFilterFreq(float deltaF) {
	float nextF = areaExLowPassFreq[areaIndex]+deltaLowPassFreq[areaIndex]*deltaF;
	if(nextF >=0 && nextF<= 22050) {
		hyperDrumSynth->setAreaExLowPassFreq(areaIndex, nextF);
		areaExLowPassFreq[areaIndex] = nextF;
		deltaLowPassFreq[areaIndex] = nextF/10.0;
		printf("Area %d excitation low pass freq: %f [Hz]\n", areaIndex, areaExLowPassFreq[areaIndex]);
	}
	else
		printf("Requested excitation low pass freq %f [Hz] is out range [0, 22050 Hz]\n", nextF);
}

void setAreaLowPassFilterFreq(int area, double f) {
	hyperDrumSynth->setAreaExLowPassFreq(area, f);
}

void setAreaExRelease(int area, double t) {
	hyperDrumSynth->setAreaExcitationRelease(area, t);
}

void setAreaExReleaseNorm(int area, double in) {
	double t = in*10; // 10 seconds is max release
	hyperDrumSynth->setAreaExcitationRelease(area, t);
}

void changeAreaIndex(int index) {
	if(index<AREA_NUM) {
		areaIndex = index;
		hyperDrumSynth->setSelectedArea(areaIndex);
		printf("Current Area index: %d\n", areaIndex);
		printf("Material:\n");
		printf("\tPropagation [pitch]: %f [press range: %f]\n", areaPropagationFactor[areaIndex], pressFinger_range[areaIndex][press_prop]);
		printf("\tDamping [resonance]: %f [press range: %f]\n", areaDampFactor[areaIndex], pressFinger_range[areaIndex][press_dmp]);
		printf("\tExcitation: %s [id %d%s]\n", excitationNames[areaExciteID[areaIndex]].c_str(), areaExciteID[areaIndex], (checkPercussive(areaIndex)) ? ", percussive" : "");
		printf("Excitation:\n");
		printf("\tExcitation volume: %f\n", areaExcitationVol[areaIndex]);
		printf("\tExcitation filter cut-off: %f dB\n", areaExLowPassFreq[areaIndex]);
		printf("Controls:\n");
		printf("\tFirst pre finger motion set to: %s", fingerMotion_names[firstPreFingerMotion[areaIndex]].c_str());
		printf("%s\n", (firstPreFingerMotionNegated[areaIndex]) ? "[negated]" : "");
		printf("\tFirst pre finger pressure set to: %s\n", fingerPress_names[firstPreFingerPress[areaIndex]].c_str());
		printf("\tPost finger motion set to: %s", fingerMotion_names[postFingerMotion[areaIndex]].c_str());
		printf("%s\n\n", (postFingerMotionNegated[areaIndex]) ? "[negated]" : "");
	}
	else
		printf("Cannot switch to Area index %d cos avalable areas are 0 to %d /:\n", index, AREA_NUM-1);
}

void setBgain(float gain) {
	bgain = gain;
	if(bgain <0)
		bgain = 0;
	else if(bgain>1)
		bgain = 1;
}

void changeBgain(float deltaB) {
	bgain += deltaBgain*deltaB;
	if(bgain <0)
		bgain = 0;
	else if(bgain>1)
		bgain = 1;
	printf("Current bgain: %f\n", bgain);

}

void changeFirstPreFingerMotion(motionFinger_mode mode, bool negated) {
	firstPreFingerMotion[areaIndex] = mode;
	printf("Area %d first pre finger motion mode set to %s\n", areaIndex, fingerMotion_names[mode].c_str());

/*	if(negated==-1)
		firstPreFingerMotionNegated[areaIndex] = false;
	else {*/
		firstPreFingerMotionNegated[areaIndex] = negated;
		if(firstPreFingerMotionNegated[areaIndex])
			printf("[negated]\n");
	//}

	// if dismissing, do it on pressure too
	if(mode == motion_none) {
		firstPreFingerPress[areaIndex] = press_none;
		printf("Area %d first pre finger pressure mode set to %s\n", areaIndex, fingerPress_names[mode].c_str());
	}
}

void cycleFirstPreFingerPressure() {
	// increment mode
	int mode = (int)firstPreFingerPress[areaIndex];
	mode = ((mode+1)%press_cnt);
	firstPreFingerPress[areaIndex] = (pressFinger_mode)mode;
	printf("Area %d first pre finger pressure mode set to %s\n", areaIndex, fingerPress_names[mode].c_str());
}

void changePostFingerMode(motionFinger_mode mode, bool negated) {
	postFingerMotion[areaIndex] = mode;
	printf("Area %d post finger motion mode set to %s\n", areaIndex, fingerMotion_names[mode].c_str());

	postFingerMotionNegated[areaIndex] = negated;
	if(negated) {
		if(postFingerMotionNegated[areaIndex])
			printf("[negated]\n");
	}

	// if dismissing, do it on pressure too
	if(mode == motion_none) {
		postFingerPress[areaIndex] = press_none;
		printf("Area %d post finger pressure mode set to %s\n", areaIndex, fingerPress_names[mode].c_str());
	}
}

void cyclePostFingerPressureMode() {
	// increment mode
	int mode = (int)postFingerPress[areaIndex];
	mode = ((mode+1)%press_cnt);
	postFingerPress[areaIndex] = (pressFinger_mode)mode;
	printf("Area %d post finger pressure mode set to %s\n", areaIndex, fingerPress_names[mode].c_str());
}

void changePreFingersMotionDynamic(motionFinger_mode mode, bool negated) {

	// dismiss if set already
	if(nextPreFingerMotion_dynamic_wait && nextPreFingerMotion_dynamic == mode && nextPreFingerMotion_dynamic_negated == negated) {
		nextPreFingerMotion_dynamic_wait = false;
		nextPreFingerMotion_dynamic = motion_none;
		printf("Next pre finger motion mode set dismissed\n");
		return;
	}

	nextPreFingerMotion_dynamic = mode;
	nextPreFingerMotion_dynamic_negated = negated;
	nextPreFingerMotion_dynamic_wait = true;
	if(preFingersPress_dynamic == press_none)
		printf("Next pre finger motion mode set to %s\n", fingerMotion_names[mode].c_str());
	else
		printf("Add to next pre finger motion mode %s\n", fingerMotion_names[mode].c_str());
	if(nextPreFingerMotion_dynamic_negated)
		printf("[negated]\n");
}

void changePreFingersPressureDynamic(pressFinger_mode mode) {
	// dismiss if set already
	if(nextPreFingerPress_dynamic_wait && preFingersPress_dynamic == mode) {
		nextPreFingerPress_dynamic_wait = false;
		preFingersPress_dynamic = press_none;
		printf("Next pre finger pressure mode set dismissed\n");
		return;
	}

	preFingersPress_dynamic = mode;
	nextPreFingerPress_dynamic_wait = true;
	if(nextPreFingerMotion_dynamic == motion_none)
		printf("Next pre finger pressure mode set to %s\n", fingerPress_names[mode].c_str());
	else
		printf("Add to next pre finger pressure mode set to %s\n", fingerPress_names[mode].c_str());

}


void cyclePreFingerPressureModeUnlock() {
	// dismiss if set already
/*	if(nextPreFingerMotion_dynamic_wait) {
		nextPreFingerMotion_dynamic_wait = false;
		return;
	}*/

	int mode = (int)preFingersPress_dynamic;
	mode = ((mode+1)%press_cnt);
	preFingersPress_dynamic = (pressFinger_mode)mode;
	nextPreFingerPress_dynamic_wait = true;
	printf("Next pre finger pressure mode set to %s\n", fingerPress_names[mode].c_str());
}

void savePreset() {
/*	int retval=hyperDrumSynth->saveFrame(AREA_NUM, areaDampFactor, areaPropagationFactor, areaExcitationVol, areaExLowPassFreq, areaExFreq, areaExciteID, areaListenerCoord,
										 (int *)firstPreFingerMotion, firstPreFingerMotionNegated, (int *)firstPreFingerPress, (int *)postFingerMotion, postFingerMotionNegated, (int *)postFingerPress,
										 pressFinger_range, pressFinger_delta, press_cnt, "test.frm");
	if(retval==0)
		printf("Current frame saved\n");
	else if(retval==-1)
		printf("HyperDrumhead not inited, can't save frame file...\n");*/

	hyperDrumheadFrame frame(AREA_NUM);  // create with constructor since conveniently destroyed at the end of scope (:

		for(int i=0; i<AREA_NUM; i++) {
			frame.damp[i] = areaDampFactor[i];
			frame.prop[i] = areaPropagationFactor[i];
			frame.exVol[i] = areaExcitationVol[i];
			frame.exFiltFreq[i] = areaExLowPassFreq[i];
			//frame.exFreq[i] = areaExFreq[i];
			frame.exType[i] = areaExciteID[i];
			frame.listPos[0][i] = areaListenerCoord[i][0];
			frame.listPos[1][i] = areaListenerCoord[i][1];

			//printf("%d list: %d, %d\n", i, frame.listPos[0][i], frame.listPos[1][i]);


			frame.firstFingerMotion[i]    = firstPreFingerMotion[i];
			frame.firstFingerMotionNeg[i] = firstPreFingerMotionNegated[i];
			frame.firstFingerPress[i]     = firstPreFingerPress[i];
			frame.thirdFingerMotion[i]    = postFingerMotion[i];
			frame.thirdFingerMotionNeg[i] = postFingerMotionNegated[i];
			frame.thirdFingerPress[i]     = postFingerPress[i];

			for(int j=0; j<press_cnt; j++)
				frame.pressRange[i][j] = pressFinger_range[i][j];

		}

		for(int j=0; j<press_cnt; j++)
			frame.pressDelta[j] = pressFinger_delta[0][j]; //VIC NOT USED ANYMORE...but cannot remove it for now otherwise saved presets go nuts!

		presetManager.savePreset(&frame); // frame address, cos method wants pointer
}



//DEBUG
void openFrame() {
	return;
	/*string filename = "test.frm";
	int retval = hyperDrumSynth->openFrame(AREA_NUM, filename, areaDampFactor, areaPropagationFactor, areaExcitationVol, areaExLowPassFreq, areaExFreq, areaExciteID,
										   areaListenerCoord, (int *)firstPreFingerMotion, firstPreFingerMotionNegated, (int *)firstPreFingerPress, (int *)postFingerMotion, postFingerMotionNegated, (int *)postFingerPress,
										   pressFinger_range, pressFinger_delta, press_cnt);

	if(retval==-1) {
		printf("HyperDrumhead not inited, can't open frame file...\n");
		return;
	}
	printf("Loaded frame from file \"%s\"\n", filename.c_str());*/

	hyperDrumheadFrame *frame = presetManager.loadPreset(0); // this method returns a dynamically allocated frame!

	if(frame==NULL)
		return;

	for(int i=0; i<frame->numOfAreas; i++) {
		// apply settings to the synth --->MOVE IN SYNTH
/*		setAreaDampingFactor(i, frame->damp[i]);
		setAreaPropagationFactor(i, frame->prop[i]);
		setAreaExcitationVolume(i, frame->exVol[i]);
		setAreaLowPassFilterFreq(i, frame->exFiltFreq[i]);
		//setAreaExcitationFreq(i, frame->exFreq[i]);
		setAreaExcitationID(i, frame->exType[i]);
		hyperDrumSynth->initAreaListenerPosition(i, frame->listPos[0][i], frame->listPos[1][i]);
		if( (frame->listPos[0][i]==-1) && (frame->listPos[1][i]==-1) )
			hideAreaListener(i);*/

		// update local control params
		setAreaExcitationID(i, frame->exType[i]);
		areaListenerCoord[i][0]  = frame->listPos[0][i];
		areaListenerCoord[i][1]  = frame->listPos[1][i];
		areaDampFactor[i]        = frame->damp[i];
		areaPropagationFactor[i] = frame->prop[i];
		areaExcitationVol[i]     = frame->exVol[i];
		areaExLowPassFreq[i] = frame->exFiltFreq[i];
		areaExciteID[i]     =  frame->exType[i];
		// listener pos is not useful

		firstPreFingerMotion[i]    = (motionFinger_mode)frame->firstFingerMotion[i];
		firstPreFingerMotionNegated[i] = frame->firstFingerMotionNeg[i];
		firstPreFingerPress[i]     = (pressFinger_mode) frame->firstFingerPress[i];
		postFingerMotion[i]    = (motionFinger_mode)frame->thirdFingerMotion[i];
		postFingerMotionNegated[i] = frame->thirdFingerMotionNeg[i];
		postFingerPress[i]     = (pressFinger_mode) frame->thirdFingerPress[i];

		for(int j=0; j<press_cnt; j++)
			pressFinger_range[i][j] = frame->pressRange[i][j];
	}

	setMasterVolume(masterVolume); // just to re set it to all loaded areas

	for(int i=0; i<AREA_NUM; i++) {
		// don't forget that all excitations are wiped by default!

		if(areaOnHold[i]) {
			for(int touch : triggerArea_touches[i])
				triggerTouch_area[touch] = -1;
			triggerArea_touches[i].clear();

			hyperDrumSynth->setAreaMicCanDrag(i, 0); // update mic color

			areaOnHold[i] = false; // hold released
		}
	}
	/*
	 * dampAreaExcitation(touchedArea); // stop excitation
				for(int touch : triggerArea_touches[touchedArea])
					removeAreaExcitations(touchedArea, touch);
				// if there are no new touches controlling propagation and damping factors
				if(preFingersTouch_area[touchedArea]==-1) {
					// reset them, cos they might have been changed
					hyperDrumSynth->setAreaPropagationFactor(touchedArea, areaPropagationFactor[touchedArea]);
					hyperDrumSynth->setAreaDampingFactor(touchedArea, areaDampFactor[touchedArea]);
				}

				for(int touch : triggerArea_touches[touchedArea])
					triggerTouch_area[touch] = -1;
				triggerArea_touches[touchedArea].clear();

				//areaOnHold[touchedArea] = false; // hold will be released when more down, when the trigger is handled...

				hyperDrumSynth->setAreaMicCanDrag(touchedArea, 0); // update mic color
	 * */

	//VIC dist dist_scanBoundDist();
	//VIC dist dist_scanExciteDist();
}



inline void setAreaVolPress(double press, int area) {
	double vol = expMapping(expMin, expMax, expRange, expNorm, minPress, maxPress, rangePress, press, minVolume, rangeVolume);
	setAreaExcitationVolumeFixed(area, vol*areaExcitationVol[area]*masterVolume);
	/*printf("p: %f\n", press);
	printf("***vol: %f***  [master volume: %2.f]\n", vol, masterVolume);*/
}

/*inline*/ void modulateAreaVolPress(double press, int area) {
	areaModulatedVol[area] = expMapping(expMin, expMax, expRange, expNorm, minPress, maxPress, rangePress, press, minVolume, rangeVolume);
	setAreaExcitationVolume(area, areaModulatedVol[area]*areaExcitationVol[area]*masterVolume);
	//printf("***areaModulatedVol[%d]: %f***  [master volume: %2.f]\n", area, areaModulatedVol[area], masterVolume);

	// stores last pressure, in case we pass it to other area via preset change and hold
	areaLastPressure[area] = pressure[triggerArea_touches[area].front()];
}

inline void setAreaImpPress(double press, int area) {

	double pressoInput = maxPressImp*0.6 + maxPressImp*0.4*press/maxPressImp; // kludge to give sense of somewhat consistent velocity via pressure

	double vol = expMapping(expMinImp, expMaxImp, expRangeImp, expNormImp, minPressImp, maxPressImp, rangePressImp, pressoInput, minVolumeImp, rangeVolumeImp);
	setAreaExcitationVolumeFixed(area, vol*areaExcitationVol[area]*masterVolume);
	//printf("p: %f\n", press);
/*	printf("***press: %f***\n", press);
	printf("minPressImp: %f, maxPressImp %f\n", minPressImp, maxPressImp);
	printf("minVolumeImp: %f, rangeVolumeImp %f\n", minVolumeImp, rangeVolumeImp);*/
	//printf("pressoInput: %f\n", pressoInput);
	//printf("***vol: %f***  [master volume: %2.f]\n", vol, masterVolume);
	//printf("***cnt: %d***\n", cnt++);

}



//inline void modulateAreaFiltProx(double prox, int area) {
//	double cutOff = expMapping(expMin, expMax, expRange, expNorm, minEProx, maxEProx, rangeEProx, prox, minFilter, rangeFilter);
//	setAreaLowPassFilterFreq(area, cutOff*areaExLowPassFreq[area]);
//	//printf("low pass %f [prox %f]\n", cutOff*areaExLowPassFreq[area], prox);
//}

//inline void modulateAreaDampProx(double prox, int area) {
//	double deltaD = expMapping(expMinDamp, expMaxDamp, expRangeDamp, expNormDamp, minBProx, maxBProx, rangeBProx, prox, 0, pressFinger_range[area][press_dmp]);
//	changeAreaDampingFactor(deltaD, false, area);
//	//printf("boundProx: %f, delta: %f\n------------------\n", prox, deltaD);
//}

inline void modulateAreaPropPress(double press, int area) {
	float delta/* = expMapping(expMin, expMax, expRange, expNorm, minPress, maxPress, rangePress, press, 0, pressFinger_range[area][press_prop])*/;
	if(press<minPress)
		press = minPress;
	if(press>maxPress)
		press = maxPress;
	delta = (press-minPress)/rangePress;

	delta *= pressFinger_range[area][press_prop];
	//changeAreaPropagationFactor(delta, false, area);

	float nextP = areaPropagationFactor[area]+delta;
	if(nextP<=0)
		nextP = 0.0001;
	else if(nextP>0.5)
		nextP = 0.5;

	setAreaPropagationFactor(area, nextP);
	//printf("press %f - delta %f\n", press, delta);
}

inline void modulateAreaDampPress(double press, int area) {
	float delta/* = expMapping(expMin, expMax, expRange, expNorm, minPress, maxPress, rangePress, press, 0, pressFinger_range[area][press_dmp])*/;
	if(press<minPress)
			press = minPress;
		if(press>maxPress)
			press = maxPress;
	delta = (press-minPress)/rangePress;

	delta *= pressFinger_range[area][press_dmp];
	//changeAreaDampingFactor(delta, false, area);

	float nextD = areaDampFactor[areaIndex]+delta;
	if(nextD <0)
		nextD = 0;
	setAreaDampingFactor(area, nextD);
	//printf("press %f - delta %f\n", press, delta);
}





// for MIDI and OSC controls

// move listener pos on x using a normalized control value
void setAreaListPosNormX(int area, float xPos) {
	if(hyperDrumSynth->setAreaListenerPosition(area, xPos*domainSize[0],  listenerCoord[1]) == 0) {
		hyperDrumSynth->getAreaListenerPosition(area, listenerCoord[0], listenerCoord[1]);
		//printf("Listener position (in pixels): (%d, %d)\n", listenerCoord[0], listenerCoord[1]); //get pos
	}
}
// move listener pos on x using a normalized control value
void setAreaListPosNormY(int area, float yPos) {
	if(hyperDrumSynth->setAreaListenerPosition(area, listenerCoord[0], yPos*domainSize[1]) == 0) {
		hyperDrumSynth->getAreaListenerPosition(area, listenerCoord[0], listenerCoord[1]);
		//printf("Listener position (in pixels): (%d, %d)\n", listenerCoord[0], listenerCoord[1]); //get pos
	}
}

void setAreaLowPassFilterFreqNorm(int area, double in) {
	setAreaLowPassFilterFreq(area, 22050*in);
}


void setAreaVolNorm(int area, double in) {
	double press = maxPressure*in;
	if(areaExciteID[area] == 0)
		setAreaImpPress(press, area);
	else
		setAreaVolPress(press, area);
}
void modulateAreaVolNorm(int area, double in) {
	if(/*areaExciteID[area] == PERCUSSIVE*/checkPercussive(area))
		return;
	double press = maxPressure*in;
	modulateAreaVolPress(press, area);
}

//VIC needs exp!!!
void setAreaDampingFactorNorm(int area, float in) {
	float damp = in;
	setAreaDampingFactor(area, damp);
}

void setAreaPropagationFactorNorm(int area, float in) {
	float prop = 0.5*in;
	setAreaPropagationFactor(area, prop);
}








bool modifierActions(int area, int coordX, int coordY) {
	bool retval = false;

	// with space bar we physically select the current area to work on
	if(modIndex) {
		changeAreaIndex(area);
		retval = true;
	}
	else if(modBound) {
		if(!modNegated)
			setBoundaryCell(area,coordX, coordY);
		else
			resetCell(coordX, coordY);
		retval = true;
	}
	else if(modExcite) {
		if(!modNegated)
			setExcitationCell(area, coordX, coordY);
		else
			resetCell(coordX, coordY);
		retval = true;
	}
	else if(modListener){
		if(!modNegated) {
			moveAreaListener(coordX, coordY);
		}
		else
			hideAreaListener();
		retval = true;
	}
	else if(modArea) {
		if(modNegated)
			setAllCellsArea(coordX, coordY);
		else
			setCellArea(coordX, coordY);
		retval = true;
	}

	return retval;
}

void holdAreaPrepareRelease(int index) {
	// if there are no new touches controlling propagation and damping factors
	if(preFingersTouch_area[index]==-1) {
		// reset them, cos they might have been changed
		hyperDrumSynth->setAreaPropagationFactor(index, areaPropagationFactor[index]);
		hyperDrumSynth->setAreaDampingFactor(index, areaDampFactor[index]);
	}

	for(int touch : triggerArea_touches[index])
		triggerTouch_area[touch] = -1;
	triggerArea_touches[index].clear();

	//areaOnHold[touchedArea] = false; // hold will be released when more down, when the trigger is handled...

	hyperDrumSynth->setAreaMicCanDrag(index, 0); // update mic color
}

// this simple puts on hold with no checks
void holdArea(int index) {
	/*if( (areaExciteID[index]!=PERCUSSIVE && !triggerArea_touches[index].empty()) || (areaExciteID[index]==PERCUSSIVE && !preFingersArea_touches[index].empty()) )*/ {
		areaOnHold[index] = true;
		printf("Area %d on hold!\n", index);

		if(!checkPercussive(index)/*areaExciteID[index]!=PERCUSSIVE*/)
			hyperDrumSynth->setAreaMicCanDrag(index, 1); // update mic color

		// discard all prefinger pressure touches
		for(int touch : preFingersArea_touches[index]) {

			// if this was a dynamic pre finger, reset temporary modifiers
		/*	if motion modifiers
			if(is_preFingerMotion_dynamic[currentSlot]) {
				// keep them!
			}*/
			if(is_preFingerPress_dynamic[touch]) {
				is_preFingerPress_dynamic[touch] = false;

				// cos impulse areas release hold when pressure modifier applied twice
				if(checkPercussive(index) /*areaExciteID[index]==PERCUSSIVE*/) {
					if(preFingersTouch_press[touch]==press_prop)
						holdOnPressure[index][0] = true;
					else if(preFingersTouch_press[touch]==press_dmp)
						holdOnPressure[index][1] = true;
				}

				bool wasFirstPreFinger = (firstPreFingerArea_touch[index] == touch);
				if(!wasFirstPreFinger)
					preFingersTouch_press[touch] = press_none;
				// consider also the case of dynamic modification of fixed pre finger
				else
					preFingersTouch_press[touch] = firstPreFingerPress[index];

				// wipe touch's area
				preFingersTouch_area[touch] = -1;

				// wipe also case of fixed pre finger [have to do it outside of the loop, otherwise iterator gets mad]
				if(wasFirstPreFinger)
					firstPreFingerArea_touch[touch] = -1;
			}
		}

		// and wipe from area all touches that don't belong
		for(int j=0; j<maxTouches; j++) {
			if(preFingersTouch_area[j] != index)
				preFingersArea_touches[index].remove(j);
		}

		//for(int touch : triggerArea_touches[index])
		//	triggerTouch_area[touch] = -1;
	}
}

// this does the proper checks
void holdAreas() {
	for(int i=0; i<AREA_NUM; i++) {
		if( (!checkPercussive(i) && !triggerArea_touches[i].empty()) || (checkPercussive(i) && !preFingersArea_touches[i].empty()) )
			holdArea(i);
	}
}

inline int computeAreaListDist(int area, int coordX, int coordY) {
	return abs((areaListenerCoord[area][0]-coordX)*(areaListenerCoord[area][0]-coordX) + (areaListenerCoord[area][1]-coordY)*(areaListenerCoord[area][1]-coordY) );
}

inline int computeAreaExcitetDist(int area, int coordX, int coordY) {
	int minDist=99;//pair<int, int> exciteCoord =
	int dist;
	for(int touch : triggerArea_touches[area]) {
		for(pair<int, int> coord : triggerArea_touch_coords[area][touch]) {
			dist = abs((coord.first-coordX)*(coord.first-coordX) + (coord.second-coordY)*(coord.second-coordY) );
			if(dist<minDist)
				minDist = dist;
		}
	}
	return minDist;
}


bool checkBoundaries(int coordX, int coordY) {
	float type = hyperDrumSynth->getCellType(coordX, coordY);

	return (type==HyperDrumhead::cell_boundary)||(type==HyperDrumhead::cell_dead);
}

void modifierActionsPreFinger(int slot, int area, int coordX, int coordY, float press) {
	// at this point i am sure that this slot/area combination determines a pre finger behavior...cos checked in checkPreFinger(...) already
	if(preFingersTouch_motion[slot]==motion_bound) {
		if(!modNegated && !preFingersTouch_motionNegated[slot]/*!firstPreFingerMotionNegated[area]*/)
			setBoundaryCell(area,coordX, coordY);
		else {
			resetCell(coordX, coordY);
			moveAreaEraser(area, coordX, coordY);
		}
	}
	else if(preFingersTouch_motion[slot]==motion_ex) {
		if(!modNegated && !preFingersTouch_motionNegated[slot]/*!firstPreFingerMotionNegated[area]*/)
			setExcitationCell(area, coordX, coordY);
		else {
			resetCell(coordX, coordY);
			moveAreaEraser(area, coordX, coordY);
		}
	}
	else if(preFingersTouch_motion[slot]==motion_list && !checkBoundaries(coordX, coordY) ) {
		if(boundTouch_areaListener[slot]!=-1) {
			//boundTouch_areaListener[slot] = area;
			moveAreaListener(boundTouch_areaListener[slot], coordX, coordY);
		}
		else
			printf("#####WARNING no link between touch %d and area %d\n", slot, area);

		//printf("pre listener on area %d\n", area);
	}

	// no pressure modulation if finger is moving, to avoid wobbly effect
	float posX_old = touchPos_prev[0][slot]-windowPos[0];
	float posY_old = touchPos_prev[1][slot]-windowPos[1];
	int coord_old[2] = {(int)posX_old, (int)posY_old};
	getCellDomainCoords(coord_old);

	if(coordX==coord_old[0] && coordY==coord_old[1]) {
		// pressure
		if(preFingersTouch_press[slot]==press_prop)
			modulateAreaPropPress(press, area);
		else if(preFingersTouch_press[slot]==press_dmp)
			modulateAreaDampPress(press, area);
	}

}

void modifierActionsPostFinger(int slot, int area, int coordX, int coordY, float press) {
	if(postFingerMotion[area]==motion_bound) {
		if(!modNegated && !postFingerMotionNegated[area])
			setBoundaryCell(area,coordX, coordY);
		else {
			resetCell(coordX, coordY);
			moveAreaEraser(area, coordX, coordY);
		}

		//changeMousePos(coordX, coordY);

	}
	else if(postFingerMotion[area]==motion_ex) {
		if(!modNegated && !postFingerMotionNegated[area])
			setExcitationCell(area, coordX, coordY);
		else {
			resetCell(coordX, coordY);
			moveAreaEraser(area, coordX, coordY);
		}

		//changeMousePos(coordX, coordY);

	}
	else if(postFingerMotion[area]==motion_list && !checkBoundaries(coordX, coordY))
		moveAreaListener(boundTouch_areaListener[slot], coordX, coordY);

	// no pressure modulation if finger is moving, to avoid wobbly effect
	float posX_old = touchPos_prev[0][slot]-windowPos[0];
	float posY_old = touchPos_prev[1][slot]-windowPos[1];
	int coord_old[2] = {(int)posX_old, (int)posY_old};
	getCellDomainCoords(coord_old);

	if(coordX==coord_old[0] && coordY==coord_old[1]) {
		// pressure
		if(postFingerPress[area]==press_prop)
			modulateAreaPropPress(press, area);
		else if(postFingerPress[area]==press_dmp)
			modulateAreaDampPress(press, area);
	}
}

bool checkPreFinger(int slot, int area, int *coord=NULL, bool set=false) {

	// if there are no pre fingers in this area AND first pre fingers are not fixed AND we are not waiting for a dynamic pre finger change
	if( preFingersArea_touches[area].empty() /*.size()==0*/ &&
		(firstPreFingerMotion[area]==motion_none && firstPreFingerPress[area]==press_none) && !nextPreFingerMotion_dynamic_wait && !nextPreFingerPress_dynamic_wait)
		return false; // no pre finger

	// if this is a new touch and one of the above conditions are not satisfied, we try to see if this is new pre finger touch!
	if(set) {
		// if this touch is not pre finger for this area already AND it is not its post finger either
		if(preFingersTouch_area[slot] == -1 && postFingerTouch_area[slot]==-1) {

			// if dynamic pre finger change pending, set temporary modifiers
			if(nextPreFingerMotion_dynamic_wait || nextPreFingerPress_dynamic_wait) {

				if(nextPreFingerMotion_dynamic_wait) {
					is_preFingerMotion_dynamic[slot] = true; // save id for when this finger is released

					// apply temp behavior
					preFingersTouch_motion[slot] = nextPreFingerMotion_dynamic;
					preFingersTouch_motionNegated[slot] = nextPreFingerMotion_dynamic_negated;

					// save records
					preFingersArea_touches[area].push_back(slot);
					preFingersTouch_area[slot] = area;

					nextPreFingerMotion_dynamic_wait = false; // done

					if(preFingersTouch_motion[slot] == motion_list && boundTouch_areaListener[slot]==-1) {

						//VIC to catch bug
/*						int boundTo = -1;
						if(boundTouch_areaListener[slot]!=-1)
							boundTo = boundTouch_areaListener[slot];*/

						// hook to listener...either a super close one or the one from the area!
						bool hooked = false;
						for(int i=0; i<AREA_NUM; i++){
							if(computeAreaListDist(i, coord[0], coord[1])<MIN_DIST) {
								boundTouch_areaListener[slot] = i;
								hooked = true;
								break;
							}
						}

						// in case we are far from any lister, we hook up the one from the area
						if(!hooked)
							boundTouch_areaListener[slot] = area;

	/*					if(boundTouch_areaListener[slot] == -1) {
							printf("######## warning! boundTouch_areaListener[%d] is -1\n", slot);
							boundTouch_areaListener[slot] = boundTo;
						}*/
					}
				}

				// may be both
				if(nextPreFingerPress_dynamic_wait) {
					is_preFingerPress_dynamic[slot] = true; // save id for when this finger is released

					// apply temp behavior
					preFingersTouch_press[slot] = preFingersPress_dynamic;

					// add only if not just added
					if(!nextPreFingerMotion_dynamic_wait) {
						preFingersArea_touches[area].push_back(slot);
						preFingersTouch_area[slot] = area;
					}

					nextPreFingerPress_dynamic_wait = false; // done
				}
				return true;
			}
			// otherwise, if there is a fixed first pre finger behavior set and we the first pre finger has not yet arrived, let's set it
			else if( (firstPreFingerMotion[area]!=motion_none || firstPreFingerPress[area]!=press_none) && firstPreFingerArea_touch[area]==-1 ) {
				firstPreFingerArea_touch[area] = slot;  // save id for when this finger is released

				// apply fixed behavior
				preFingersTouch_motion[slot] = firstPreFingerMotion[area];
				preFingersTouch_motionNegated[slot] = firstPreFingerMotionNegated[area];
				preFingersTouch_press[slot] = firstPreFingerPress[area];

				// save records
				preFingersArea_touches[area].push_back(slot);
				preFingersTouch_area[slot] = area;

				if(firstPreFingerMotion[area] == motion_list)
					boundTouch_areaListener[slot] = area;

				return true;
			}
		}
	}

	// otherwise simply check if this was pre touch already
	bool touchIsPreFinger = (std::find(preFingersArea_touches[area].begin(), preFingersArea_touches[area].end(), slot) != preFingersArea_touches[area].end());
	return touchIsPreFinger;
}


bool checkPostFinger(int slot, int area, bool noSet=false) {
	if(!noSet) {
		// if post finger is not linked to any moodifier
		if(postFingerMotion[area]==motion_none && postFingerPress[area]==press_none)
			return false;

		// if we recognize it as new post touch, set it and return true
		if(!touchIsInhibited[slot] && postFingerArea_touch[area]==-1) { // touchIsInhibited serves to prevent twin touches from doing something stupid [displax bug!]
			postFingerArea_touch[area] = slot;
			postFingerTouch_area[slot] = area;

			if(postFingerMotion[slot] == motion_list)
				boundTouch_areaListener[slot] = area;

			return true;
		}
	}

	// otherwise simply check if this was post touch already
	return (postFingerArea_touch[area]==slot);
}


void removeAreaExcitations(int area, int slot) {
	// reset all excitations created by this touch [due to displax bugs...cos it should be just one goddammit!]
	for(pair<int, int> coord : triggerArea_touch_coords[area][slot])
		resetCell(coord.first, coord.second);
	triggerArea_touch_coords[area][slot].clear(); // then empty out list
}


void updateControlStatus(hyperDrumheadFrame *frame) {
	//printf("update control status\n");
	int prevExciteID[AREA_NUM];
	for(int i=0; i<frame->numOfAreas; i++) {
		prevExciteID[i] = areaExciteID[i];

		// update local control params
		/*setAreaExcitationID(i, frame->exType[i]);*/
		areaExciteID[i] = frame->exType[i];
		areaListenerCoord[i][0]  = frame->listPos[0][i];
		areaListenerCoord[i][1]  = frame->listPos[1][i];
		areaDampFactor[i]        = frame->damp[i];
		areaPropagationFactor[i] = frame->prop[i];
		areaExcitationVol[i]     = frame->exVol[i];
		areaExLowPassFreq[i] = frame->exFiltFreq[i];
		// listener pos is not useful

		firstPreFingerMotion[i]    = (motionFinger_mode)frame->firstFingerMotion[i];
		firstPreFingerMotionNegated[i] = frame->firstFingerMotionNeg[i];
		firstPreFingerPress[i]     = (pressFinger_mode) frame->firstFingerPress[i];
		postFingerMotion[i]    = (motionFinger_mode)frame->thirdFingerMotion[i];
		postFingerMotionNegated[i] = frame->thirdFingerMotionNeg[i];
		postFingerPress[i]     = (pressFinger_mode) frame->thirdFingerPress[i];

		for(int j=0; j<press_cnt; j++)
			pressFinger_range[i][j] = frame->pressRange[i][j];

		// update deltas and ranges too
		areaDeltaDampFactor[i] = areaDampFactor[i]*0.1;
		areaDeltaPropFactor[i] = areaPropagationFactor[i]*0.01;

		pressFinger_delta[i][press_prop] = areaDeltaPropFactor[i]*0.1;
		pressFinger_delta[i][press_dmp]  = areaDeltaDampFactor[i]*0.1;
	}

	for(int i=frame->numOfAreas; i<AREA_NUM; i++) {
		areaListenerCoord[i][0]  = -2;
		areaListenerCoord[i][1]  = -2;
	}

	setMasterVolume(masterVolume); // just to re set it to all loaded areas

	// keep excitation in each area, but remove modifer bindings
	for(int i=0; i<AREA_NUM; i++) {


		// remove all modifier bindings! makes interaction cleaner
		for(int touch : preFingersArea_touches[i]) {
			preFingersTouch_area[touch] = -1;
			is_preFingerMotion_dynamic[touch] = false;
			is_preFingerPress_dynamic[touch] = false;
			boundTouch_areaListener[touch] = -1;
		}
		preFingersArea_touches[i].clear();
		firstPreFingerArea_touch[i] = -1;

		if(postFingerArea_touch[i] != -1) {
			int touch = postFingerArea_touch[i];
			postFingerTouch_area[touch] = -1;
			postFingerArea_touch[i] = -1;
			if(postFingerMotion[i] == motion_list)
				boundTouch_areaListener[i] = -1;
		}


		// pass all triggers to area finger i touching
		list<int> touches;// = triggerArea_touches[i];

		std::copy(triggerArea_touches[i].begin(), triggerArea_touches[i].end(),
		          std::back_insert_iterator<std::list<int> >(touches));


		// don't forget that all excitations are wiped by default!
		for(int touch : touches) {
			pair<int, int> coord = triggerArea_touch_coords[i][touch].back();
			//printf("area %d touch %d (%d, %d)\n", i, touch, coord.first, coord.second);

			// which can be different from previous area it was touching!
			int newAreaID = getAreaIndex(coord.first, coord.second);
			int newExciteID = areaExciteID[newAreaID];

			//setExcitationCell(i, coord.first, coord.second);
			setExcitationCrossfade(newAreaID, coord.first, coord.second, coord.first, coord.second);

			// new volume [no interpolation]
			areaLastPressure[newAreaID] = areaLastPressure[i];
			setAreaVolPress((double)areaLastPressure[newAreaID], newAreaID);


			// if area has changed...
			if(newAreaID != i) {
				//...move all triggers in new one
				triggerArea_touches[newAreaID].push_back(touch);
				triggerTouch_area[touch] = newAreaID;
				//printf("******touch %d triggers area %d\n", touch, newAreaID);
				for(pair<int, int> coord : triggerArea_touch_coords[i][touch])
					triggerArea_touch_coords[newAreaID][touch].push_back(coord);

				//...and wipe from old area
				triggerArea_touches[i].remove(touch);
				triggerArea_touch_coords[i][touch].clear();

				// trigger new area
				triggerAreaExcitation(newAreaID);
				// and stop previous one
				dampAreaExcitation(i);
				excitationDeck[i] = 1-excitationDeck[i]; // cos we the abrupt change in excitation status messes up deck

				// this prevents handleTouchDrag() from activating trigger touch when jump from one area to another is detected!
				touchChangedAreaOnPreset[touch] = true;
			}

			if(areaOnHold[i]) {
				if(newAreaID != i) {
					holdAreaPrepareRelease(i);
					areaOnHold[i] = false;
				}
				holdArea(newAreaID);
			}

			// however, if we are transitioning to or from a percussive area...
			if( checkPercussive(i, prevExciteID[i])/*(prevExciteID[i]==PERCUSSIVE)*/ || checkPercussive(newAreaID, newExciteID)/*(newExciteID==PERCUSSIVE)*/ ) {
				//...delete the triggers we just added and damp excitation
				resetCell(coord.first, coord.second);
				dampAreaExcitation(i);
				dampAreaExcitation(newAreaID);

				holdAreaPrepareRelease(i);
				areaOnHold[i] = false;
				holdAreaPrepareRelease(newAreaID);
				areaOnHold[newAreaID] = false;
			}
		}
	}

//VIC dist dist_scanBoundDist();
//VIC dist dist_scanExciteDist();
}

void loadNextPreset() {
/*	if(updatePrevPreset)
		updateNextPreset = false;
	else
		updateNextPreset = true;*/

	hyperDrumheadFrame *frame = presetManager.loadNextPreset(); // this method returns a dynamically allocated frame!

	if(frame!=NULL)
		updateControlStatus(frame);
}

void loadPrevPreset() {
	/*if(updateNextPreset)
		updateNextPreset = false;
	else
		updatePrevPreset = true;*/
	hyperDrumheadFrame *frame = presetManager.loadPrevPreset(); // this method returns a dynamically allocated frame!
	if(frame!=NULL)
		updateControlStatus(frame);
}

void reloadPreset() {
	hyperDrumheadFrame *frame = presetManager.reloadPreset(); // this method returns a dynamically allocated frame!

	if(frame!=NULL)
		updateControlStatus(frame);
}


int swapTouches(int oldTouchID, int newTouchID, int newPosY) {
	//printf("%d (%f, %f)\n", newTouchID, touchPos[0][newTouchID], newPosY);
	//printf("%d (%f, %f)\n", oldTouchID, touchPos[0][oldTouchID], touchPos[1][oldTouchID]);
	//printf("%d-%d :%f\n", newTouchID, oldTouchID, dist);
	//printf("%d and %d\n\n", newTouchID, oldTouchID);

	//printf("swap old %d with new %d\n", oldTouchID, newTouchID);


	// should not be considered as new anymore, taking care of the other extra touch bug too [see next else clause]
	newTouch[newTouchID]       = false;
	extraTouchDone[newTouchID] = true;

	extraTouchDone[oldTouchID] = true;
	//extraTouchPos[oldTouchID][0] = -1;
	//extraTouchPos[oldTouchID][1] = -1;

	// let's average out the twin touches' positions
	touchPos[0][newTouchID] = (touchPos[0][newTouchID] + touchPos[0][oldTouchID])/2;
	newPosY = (newPosY + touchPos[1][oldTouchID])/2;

	// copy prev position
	touchPos_prev[0][newTouchID] = touchPos_prev[0][oldTouchID];
	touchPos_prev[1][newTouchID] = touchPos_prev[1][oldTouchID];

	// copy other info
	posReceived[0][newTouchID] = posReceived[0][oldTouchID];
	posReceived[1][newTouchID] = posReceived[1][oldTouchID];
	prevTouchedArea[newTouchID] = prevTouchedArea[oldTouchID];
	touchOnBoundary[0][newTouchID] = touchOnBoundary[0][oldTouchID];
	touchOnBoundary[1][newTouchID] = touchOnBoundary[1][oldTouchID];

	keepHoldTouch_area[newTouchID] = keepHoldTouch_area[oldTouchID];
	keepHoldTouch_area[oldTouchID] = -1;
	if(keepHoldTouch_area[newTouchID]!=-1)
		keepHoldArea_touch[keepHoldTouch_area[newTouchID]] = newTouchID;


	// it is important to assign to the new twin touch ALL THE CONTROL BINDINGS of the first one

	// first excitation
	if(triggerTouch_area[oldTouchID] != -1) {
		int area = triggerTouch_area[oldTouchID]; // area triggered by old twin touch
		// swap with current twin's id
		//triggerArea_touch[area] = newTouchID;
		triggerArea_touches[area].remove(oldTouchID);
		triggerArea_touches[area].push_back(newTouchID);
		triggerTouch_area[newTouchID] = area;
		for(pair<int, int> coord : triggerArea_touch_coords[area][oldTouchID])
			triggerArea_touch_coords[area][newTouchID].push_back(coord);

		// wipe
		triggerTouch_area[oldTouchID] = -1;
		triggerArea_touch_coords[area][oldTouchID].clear();
		//printf("pass touches list on %d from %d to %d\n", area, oldTouchID, newTouchID);
	}

	//then pre/post finger controls [there's no else cos if area is on hold touch can be both trigger and pre finger!]
	if(preFingersTouch_area[oldTouchID]!=-1) {
		// it's one of the pre fingers on area, so assign it the area and
		int area = preFingersTouch_area[oldTouchID];
		preFingersTouch_area[newTouchID] = area;

		// it's the first one maybe!
		if(firstPreFingerArea_touch[area] == oldTouchID)
			firstPreFingerArea_touch[area] = newTouchID;

		is_preFingerMotion_dynamic[newTouchID] = is_preFingerMotion_dynamic[oldTouchID];
		preFingersTouch_motion[newTouchID] = preFingersTouch_motion[oldTouchID];
		preFingersTouch_motionNegated[newTouchID] = preFingersTouch_motionNegated[oldTouchID];
		// update also listener binding
		if(is_preFingerMotion_dynamic[newTouchID] && preFingersTouch_motion[newTouchID]==motion_list) {
			boundTouch_areaListener[newTouchID] = boundTouch_areaListener[oldTouchID];
			boundTouch_areaListener[oldTouchID] = -1;
			//printf("pass pre finger list on %d from %d to %d\n", boundTouch_areaListener[newTouchID], oldTouchID, newTouchID);
		}

		is_preFingerPress_dynamic[newTouchID] = is_preFingerPress_dynamic[oldTouchID];
		preFingersTouch_press[newTouchID] = preFingersTouch_press[oldTouchID];

		// wipe old
		preFingersTouch_area[oldTouchID] = -1;
		// swap the touch id in the areas array
		preFingersArea_touches[area].remove(oldTouchID);
		preFingersArea_touches[area].push_back(newTouchID);
		is_preFingerMotion_dynamic[oldTouchID] = false;
	}

	if(postFingerTouch_area[oldTouchID]!=-1) {//if(is_preFingerPress_dynamic[oldTouchID]) {
		// it's post finger on area, so assign it thwe area and swap the touch id in the areas array
		int area = postFingerTouch_area[oldTouchID];
		postFingerTouch_area[newTouchID] = area;

		postFingerArea_touch[area] = newTouchID;

		// update also listener binding
		if(postFingerMotion[area]==motion_list) {
			boundTouch_areaListener[newTouchID] = boundTouch_areaListener[oldTouchID];
			boundTouch_areaListener[oldTouchID] = -1;
		}

		// wipe old
		postFingerTouch_area[oldTouchID] = -1;
	}

	return newPosY;
}




void handleNewTouch(int currentSlot, int touchedArea, int coord[2]) {
	//printf("---------New touch %d\n", currentSlot);
	/*if(extraTouch[currentSlot]) {
		printf("\tEXTRA!\n");
	}*/

	touch_coords[0][currentSlot] = coord[1];
	touch_coords[1][currentSlot] = coord[0];

	// regular excitation
	if(checkPercussive(touchedArea)) {

		// check if this is first touch on this area and there are special modifiers linked to first touch [movement or pressure]
		if( checkPreFinger(currentSlot, touchedArea, coord, newTouch[currentSlot]) ) {
			modifierActionsPreFinger(currentSlot, touchedArea, coord[0], coord[1], (float)pressure[currentSlot]);
			if(areaOnHold[touchedArea]) {
				if( preFingersTouch_press[currentSlot]==press_prop && holdOnPressure[touchedArea][0]) {

					hyperDrumSynth->setAreaDampingFactor(touchedArea, areaDampFactor[touchedArea]);
					areaOnHold[touchedArea] = false; // and release hold
					holdOnPressure[touchedArea][0] = false;
					holdOnPressure[touchedArea][1] = false;
				}
				else if( preFingersTouch_press[currentSlot]==press_dmp && holdOnPressure[touchedArea][1] ) {

					hyperDrumSynth->setAreaPropagationFactor(touchedArea, areaPropagationFactor[touchedArea]);
					areaOnHold[touchedArea] = false; // and release hold
					holdOnPressure[touchedArea][0] = false;
					holdOnPressure[touchedArea][1] = false;
				}
			}
		}
		else {
			// update local excitation status
			triggerArea_touches[touchedArea].push_back(currentSlot);
			triggerTouch_area[currentSlot] = touchedArea;
			//printf("******touch %d triggers area %d\n", currentSlot, touchedArea);


			if(!checkBoundaries(coord[0], coord[1])) {
				// add excitation
				setExcitationCell(touchedArea, coord[0], coord[1]);

				pair<int, int> crd(coord[0], coord[1]);
				triggerArea_touch_coords[touchedArea][currentSlot].push_back(crd);
			}

			// this stuff is done anyway for fixed excitations
			// new volume [no interpolation]
			setAreaImpPress((double)pressure[currentSlot]/*maxPressImp*/, touchedArea);
			//printf("raw pressure %d\n\n", ev[i].value);

			// new low pass freq [interpolation does not exist here]
			//VIC dist double exProx = 1-dist_getExciteDist(coord[0], coord[1]);
			//VIC dist modulateAreaFiltProx(exProx, touchedArea);

			// trigger
			triggerAreaExcitation(touchedArea);
			//printf("trigger %d\n", currentSlot);

			// to make the extra touch singular, as part of the displax bug fix
	/*		if(extraTouch[currentSlot]) {
				extraTouch[currentSlot] = false;
				//printf("\tEXTRA!\n");
			}*/
		}
	}
	// continuous excitation, which needs extra care for areas on hold
	else {
		// first we handle hold case
		if(areaOnHold[touchedArea]) {
			//printf("New touch %d on HOLD %d\n", currentSlot, touchedArea);
			// if on hold and new touch is on listener, we automatically set a listener motion pre finger
			int listenerDist = computeAreaListDist(touchedArea, coord[0], coord[1]);
			//printf("dist: %d\n", listenerDist);

			if(listenerDist<MIN_DIST && postFingerArea_touch[touchedArea]==-1 && !touchIsInhibited[currentSlot]) {
				if(!nextPreFingerMotion_dynamic_wait || nextPreFingerMotion_dynamic != motion_list) { // to avoid deselection if already selected
					changePreFingersMotionDynamic(motion_list);
					boundTouch_areaListener[currentSlot] = touchedArea;
					//printf("touch %d on mic %d\n", currentSlot, boundTouch_areaListener[currentSlot]);
				}
			}


			int exciteDist = computeAreaExcitetDist(touchedArea, coord[0], coord[1]);
			// when on hold can select listener...under the hood it is just like a move list post finger
			if(exciteDist<MIN_DIST && (keepHoldArea_touch[touchedArea]==-1)) {
				keepHoldArea_touch[touchedArea] = currentSlot;
				keepHoldTouch_area[currentSlot] = touchedArea;
				//printf("touch %d keeps area %d\n", currentSlot, touchedArea);
				//printf("=========================keepHoldTouch_area[%d]: %d\n", currentSlot, keepHoldTouch_area[currentSlot]);

				int triggerTouch = triggerArea_touches[touchedArea].front();
				if(currentSlot!=triggerTouch) {
					/*triggerArea_touches[touchedArea].remove(triggerTouch);
					triggerArea_touches[touchedArea].push_back(currentSlot);
					triggerTouch_area[currentSlot] = touchedArea;
					for(pair<int, int> coord : triggerArea_touch_coords[touchedArea][triggerTouch])
						triggerArea_touch_coords[touchedArea][currentSlot].push_back(coord);
					triggerArea_touch_coords[touchedArea][triggerTouch].clear();*/

					list<int> triggers = triggerArea_touches[touchedArea];
					for(int touch : triggers) {
						removeAreaExcitations(touchedArea, touch);
						triggerArea_touches[touchedArea].remove(touch);
						triggerTouch_area[touch] = -1;
						printf("---touch %d no more trigger of area %d\n", touch, touchedArea);
					}
				}

			}
			// now check pre fingers [including the listener we might have just added]
			else if(checkPreFinger(currentSlot, touchedArea, coord, true)) {
				modifierActionsPreFinger(currentSlot, touchedArea, coord[0], coord[1], (float)pressure[currentSlot]);
				return;
			}
			// otherwise we release all the excitations that were stuck there and GO ON with other checks [we actually release hold during those further checks]
			else if(keepHoldArea_touch[touchedArea]==-1) {

				dampAreaExcitation(touchedArea); // stop excitation
				for(int touch : triggerArea_touches[touchedArea])
					removeAreaExcitations(touchedArea, touch);

				holdAreaPrepareRelease(touchedArea);

				//areaOnHold[touchedArea] = false; // hold will be released more down, when the trigger is handled...
			}
		}


		if( checkPreFinger(currentSlot, touchedArea, coord, newTouch[currentSlot]) )
			modifierActionsPreFinger(currentSlot, touchedArea, coord[0], coord[1], (float)pressure[currentSlot]);
		else if(triggerArea_touches[touchedArea].empty()) {

			// update local excitation status
			triggerArea_touches[touchedArea].push_back(currentSlot);
			triggerTouch_area[currentSlot] = touchedArea;
			//printf("******touch %d triggers area %d\n", currentSlot, touchedArea);

			// add moving excitation only if not on boundary
			if(!checkBoundaries(coord[0], coord[1])) {

				// if regular first touch
				if(!areaOnHold[touchedArea])
					setExcitationCellDeck(touchedArea, coord[0], coord[1]); // add excitation and prepare crossfade
				// but if area was on hold
				else {
					// crossfade from previous excitation on hold!
					pair<int, int> coord_prev(-2, -2);
					pair<int, int> coord_next(coord[0], coord[1]);
					// adds next [we'll crossfade there from current] and deletes previous
					setExcitationCrossfade(touchedArea, coord_next.first, coord_next.second, coord_prev.first, coord_prev.second);
					if(keepHoldArea_touch[touchedArea]==-1)
						areaOnHold[touchedArea] = false; // and release hold
				}
				// add a fake previous trigger
				pair<int, int> crd(-2, -2);
				triggerArea_touch_coords[touchedArea][currentSlot].push_back(crd);
				// then add real one [will see in HandleTouchDrag(...) why]
				crd.first  = coord[0];
				crd.second = coord[1];
				triggerArea_touch_coords[touchedArea][currentSlot].push_back(crd);
			}
			else {
				touchOnBoundary[0][currentSlot] = true;
				touchOnBoundary[1][currentSlot] = true;
			}

			int press = pressure[currentSlot];
			if(keepHoldArea_touch[touchedArea]!=-1)
				press = areaLastPressure[touchedArea];
			// new volume [no interpolation]
			setAreaVolPress((double)press, touchedArea);

			// new low pass freq [interpolation does not exist here]
			//VIC dist double exProx = 1-dist_getExciteDist(coord[0], coord[1]);
			//VIC dist modulateAreaFiltProx(exProx, touchedArea);

			//VIC dist double boundProx = 1-dist_getBoundDist(coord[0], coord[1]);
			//VIC dist modulateAreaDampProx(boundProx, touchedArea);

			// trigger and update status
			if(keepHoldArea_touch[touchedArea]==-1)
				triggerAreaExcitation(touchedArea);
		}
		// new touch, not impulse, post finger, check if linked to movement or pressure modifier
		else /*if( keepHoldArea_touch[touchedArea]==-1 )*/ if(keepHoldArea_touch[touchedArea] != currentSlot){
			if( checkPostFinger(currentSlot, touchedArea) )
				modifierActionsPostFinger(currentSlot, touchedArea, coord[0], coord[1], (float)pressure[currentSlot]);
		}
	}
}

void handleTouchRelease(int currentSlot) {
	//printf("---------Released touch %d\n", currentSlot);
	// if this touch actually triggered a continuous excitation


	// an old twin is leaving us
	if(touch_youngerTwin[currentSlot]!=-1) {
		printf("old twin %d of %d let us in peace\n", currentSlot, touch_youngerTwin[currentSlot]);

		int young_touch = touch_youngerTwin[currentSlot];
		//printf("[this]\ttouch %d touch_youngerTwin: %d, touch_elderTwin: %d\n", currentSlot, touch_youngerTwin[currentSlot], touch_elderTwin[currentSlot]);
		//printf("[young]\ttouch %d touch_youngerTwin: %d, touch_elderTwin: %d\n", young_touch, touch_youngerTwin[young_touch], touch_elderTwin[young_touch]);

		touch_elderTwin[young_touch] = -1; // young twin is now only child
		touch_youngerTwin[currentSlot] =-1; // and elder twin dies
		//printf("[this]\ttouch %d touch_youngerTwin: %d, touch_elderTwin: %d\n", currentSlot, touch_youngerTwin[currentSlot], touch_elderTwin[currentSlot]);
		//printf("[young]\ttouch %d touch_youngerTwin: %d, touch_elderTwin: %d\n", young_touch, touch_youngerTwin[young_touch], touch_elderTwin[young_touch]);

	}

	if(touch_elderTwin[currentSlot]!=-1) {

		/*printf("OUCH! %d was new twin, it died while his old %d was still alive...consider swapping!", currentSlot, touch_elderTwin[currentSlot]);
		if(touch_youngerTwin[ touch_elderTwin[currentSlot] ] == currentSlot)
			printf("\n");
		else
			printf("\t- confusing! cos old one does not recognize his sibling!\n");
*/
		int revert_touch = touch_elderTwin[currentSlot];

		//printf("[this]\ttouch %d touch_youngerTwin: %d, touch_elderTwin: %d\n", currentSlot, touch_youngerTwin[currentSlot], touch_elderTwin[currentSlot]);
		//printf("[rev]\ttouch %d touch_youngerTwin: %d, touch_elderTwin: %d\n", revert_touch, touch_youngerTwin[revert_touch], touch_elderTwin[revert_touch]);

		swapTouches(currentSlot, revert_touch, touch_coords[1][currentSlot]);
		touch_youngerTwin[ revert_touch ] = -1;
		touch_elderTwin[currentSlot] = -1;

		//printf("[this]\ttouch %d touch_youngerTwin: %d, touch_elderTwin: %d\n", currentSlot, touch_youngerTwin[currentSlot], touch_elderTwin[currentSlot]);
		//printf("[rev]\ttouch %d touch_youngerTwin: %d, touch_elderTwin: %d\n", revert_touch, touch_youngerTwin[revert_touch], touch_elderTwin[revert_touch]);

	}

	touch_coords[0][currentSlot] = -2;
	touch_coords[1][currentSlot] = -2;


	if(triggerTouch_area[currentSlot] != -1) {
		int area = triggerTouch_area[currentSlot]; // the area we have triggered when this touch first happened

		//VIC dist float exDist = dist_getExciteDist(coord[0], coord[1]);
		//VIC dist setAreaExRelease(area, exDist*areaExRelease[area]);

		// we skip anything related to touches and excitations if on hold
		if( /*areaExciteID[area]==PERCUSSIVE*/ checkPercussive(area) || !areaOnHold[area]) {
			// remove this touch from list of triggering touches
			triggerArea_touches[area].remove(currentSlot);
			triggerTouch_area[currentSlot] = -1;

			dampAreaExcitation(area); // and stop excitation
			removeAreaExcitations(area, currentSlot);

			//printf("touch %d no more trigger of area %d\n", currentSlot, area);
		}

		extraTouchDone[currentSlot] = false;

		if(/*areaExciteID[area] != PERCUSSIVE*/!checkPercussive(area))
			excitationDeck[area] = 1-excitationDeck[area];

		/*if(triggerArea_touches[area].empty())
			printf("!!!!!empty!!!!!\n\n");*/

		// reset to consider next time this touch is detected
		touchOnBoundary[0][currentSlot] = false;
		touchOnBoundary[1][currentSlot] = false;
	}

	int holdArea = keepHoldTouch_area[currentSlot];
	if(holdArea!=-1) {
		keepHoldArea_touch[holdArea] = -1;
		keepHoldTouch_area[currentSlot] = -1;
	}



	// if this touch was a pre finger in an area [independently from previous check] //VIC why independent from previous check?
	if(preFingersTouch_area[currentSlot]!=-1) {
		int area = preFingersTouch_area[currentSlot]; // area where it was pre finger

		// check if it was linked to any pressure modifier
		/*if(firstPreFingerPress[area]==press_prop)
			hyperDrumSynth->setAreaPropagationFactor(area, areaPropagationFactor[area]); // in case, reset it
		else if(firstPreFingerPress[area]==press_dmp)
			hyperDrumSynth->setAreaDampingFactor(area, areaDampFactor[area]); // in case, reset it*/

		// if was erasing finger, remove area eraser lines
		if( (preFingersTouch_motion[currentSlot] == motion_bound || preFingersTouch_motion[currentSlot] == motion_ex) && preFingersTouch_motionNegated[currentSlot] )
			hideAreaEraser(area);

		//bool wasFirstPreFinger = (firstPreFingerArea_touch[area] == currentSlot);

		// if this was a dynamic pre finger, reset temporary modifiers
		if(is_preFingerMotion_dynamic[currentSlot]) {
			/*if(preFingersTouch_motion[currentSlot]==motion_list)
				printf("which was listener\n");*/
			is_preFingerMotion_dynamic[currentSlot] = false;
			boundTouch_areaListener[currentSlot] = -1;


			/*if(!wasFirstPreFinger) {
				preFingersTouch_motion[currentSlot] = motion_none;
				preFingersTouch_motionNegated[currentSlot] = false;
			}
			// consider also the case of dynamic modification of fixed pre finger
			else {
				preFingersTouch_motion[currentSlot] = firstPreFingerMotion[area];
				preFingersTouch_motionNegated[currentSlot] = firstPreFingerMotionNegated[area];
			}*/

			preFingersTouch_motion[currentSlot] = motion_none;
			preFingersTouch_motionNegated[currentSlot] = false;

		}
		if(is_preFingerPress_dynamic[currentSlot]) {
			is_preFingerPress_dynamic[currentSlot] = false;
			/*if(!areaOnHold[area])*/ {
				// check if it was linked to any pressure modifier
				if(preFingersTouch_press[currentSlot]==press_prop)
					hyperDrumSynth->setAreaPropagationFactor(area, areaPropagationFactor[area]); // in case, reset it
				else if(preFingersTouch_press[currentSlot]==press_dmp)
					hyperDrumSynth->setAreaDampingFactor(area, areaDampFactor[area]); // in case, reset it
			}
/*			if(!wasFirstPreFinger)
				preFingersTouch_press[currentSlot] = press_none;
			// consider also the case of dynamic modification of fixed pre finger
			else
				preFingersTouch_press[currentSlot] = firstPreFingerPress[area];*/
			preFingersTouch_press[currentSlot] = press_none;
		}

		if(firstPreFingerArea_touch[area] == currentSlot) {
			firstPreFingerArea_touch[area] = -1;
			boundTouch_areaListener[currentSlot] = -1;

			// for sure remove bond with motion
			preFingersTouch_motion[currentSlot] = motion_none;
			preFingersTouch_motionNegated[currentSlot] = false;

			// check if it was linked to any pressure modifier
			if(firstPreFingerPress[area]==press_prop)
				hyperDrumSynth->setAreaPropagationFactor(area, areaPropagationFactor[area]); // in case, reset it
			else if(firstPreFingerPress[area]==press_dmp)
				hyperDrumSynth->setAreaDampingFactor(area, areaDampFactor[area]); // in case, reset it
		}

		// wipe
		preFingersArea_touches[preFingersTouch_area[currentSlot]].remove(currentSlot);
		preFingersTouch_area[currentSlot] = -1;

		// wipe also case of fixed pre finger
		/*if(wasFirstPreFinger)
			firstPreFingerArea_touch[area] = -1;*/

	}
	else if(postFingerTouch_area[currentSlot]!=-1) {
		int area = postFingerTouch_area[currentSlot]; // the are where it was post finger

		// if was erasing finger, remove area eraser lines
		if( (postFingerMotion[area] == motion_bound || postFingerMotion[area] == motion_ex) && postFingerMotionNegated[area] )
			hideAreaEraser(area);

		// check if it was linked to any pressure modifier
		if(postFingerPress[area]==press_prop)
			hyperDrumSynth->setAreaPropagationFactor(area, areaPropagationFactor[area]); // in case, reset it
		else if(postFingerPress[area]==press_dmp)
			hyperDrumSynth->setAreaDampingFactor(area, areaDampFactor[area]); // in case, reset it

		// all modifiers, motion and press, are kept, cos this is not dynamic assignment

		// finally wipe
		postFingerArea_touch[area] = -1;
		postFingerTouch_area[currentSlot] = -1;
	}

	// to fix problem with displax broken lines
/*	if(touchIsInhibited[currentSlot])
		printf("touch %d no more inhibited\n", currentSlot);*/
	touchIsInhibited[currentSlot] = false; // just in case
}

void handleTouchDrag(int currentSlot, int touchedArea, int coord[2]) {
	//printf("dragged touch %d on area %d\n", currentSlot, touchedArea);
	// touch crosses area!

	if(prevTouchedArea[currentSlot]>=AREA_NUM || prevTouchedArea[currentSlot]<0)
		printf("==============WARNING! Touch %d has prev touched area %d\n", currentSlot, prevTouchedArea[currentSlot]);

	if(prevTouchedArea[currentSlot] != touchedArea && prevTouchedArea[currentSlot]<AREA_NUM) {

		bool wasPrefinger  = checkPreFinger(currentSlot, prevTouchedArea[currentSlot]);
		bool wasPostFinger = checkPostFinger(currentSlot, prevTouchedArea[currentSlot], true);

		// general case is release...
		if( !wasPrefinger && !wasPostFinger ) {
			// takes into account case in which preset made touch jump area...
			if(!touchChangedAreaOnPreset[currentSlot]) {
				handleTouchRelease(currentSlot);
				//...and new touch if new area has continuous excitation and is not on hold
				if(/*areaExciteID[touchedArea] != PERCUSSIVE*/!checkPercussive(touchedArea) && !areaOnHold[touchedArea])
					handleNewTouch(currentSlot, touchedArea, coord);
			}
			else { //...in case we don't do anything, cos touch shoould be inactive
				prevTouchedArea[currentSlot] = touchedArea;
				touchChangedAreaOnPreset[currentSlot] = false;
			}
		}
		// if was pre finger in previous area
		else if(wasPrefinger) {

			// if was erasing finger, remove area eraser lines
			if( (preFingersTouch_motion[currentSlot] == motion_bound || preFingersTouch_motion[currentSlot] == motion_ex) && preFingersTouch_motionNegated[currentSlot] )
				hideAreaEraser(prevTouchedArea[currentSlot]);

			// swap area bindings
			preFingersArea_touches[prevTouchedArea[currentSlot]].remove(currentSlot);
			preFingersArea_touches[touchedArea].push_back(currentSlot);
			preFingersTouch_area[currentSlot] = touchedArea;

			// this would prevent area listeners to be moved in foreign areas
			/*if(preFingersTouch_motion[currentSlot] == motion_list)
				boundTouch_areaListener[currentSlot] = touchedArea;*/

			// also for the stupid and unlikely case of first/fixed pre finger
			if(firstPreFingerArea_touch[prevTouchedArea[currentSlot]] == currentSlot) {
				firstPreFingerArea_touch[prevTouchedArea[currentSlot]] = -1;
				firstPreFingerArea_touch[touchedArea] = currentSlot;
			}
		}
		// if was post finger in previous area
		else /*if(wasPostFinger)*/ {

			// if was erasing finger, remove area eraser lines
			if( (postFingerMotion[prevTouchedArea[currentSlot]] == motion_bound || postFingerMotion[prevTouchedArea[currentSlot]] == motion_ex) && postFingerMotionNegated[prevTouchedArea[currentSlot]] )
				hideAreaEraser(prevTouchedArea[currentSlot]);

			// swap area bindings
			postFingerArea_touch[prevTouchedArea[currentSlot]] = -1;
			postFingerArea_touch[touchedArea] = currentSlot;
			postFingerTouch_area[currentSlot] = touchedArea;
		}
		extraTouchDone[currentSlot] = true; // to allow for post finger to be still in control even if area with impulse excitation!
	}

	//printf("Dragged touch %d\n", currentSlot);

	if(/*areaExciteID[touchedArea] == PERCUSSIVE*/checkPercussive(touchedArea)) {
		// check if this is the first touch on this area and it's linked to a modifier [movement or pressure]
		if( checkPreFinger(currentSlot, touchedArea) )
			modifierActionsPreFinger(currentSlot, touchedArea, coord[0], coord[1], (float)pressure[currentSlot]);
		// detect drag that is instead a second touch [displax bug]
		else if(!extraTouchDone[currentSlot]) { // only one per slide of same touch
			if(abs(touchPos_prev[0][currentSlot]-touchPos[0][currentSlot])>8 || abs(touchPos_prev[1][currentSlot]-touchPos[1][currentSlot])>8) {
				// yes! remove previous one and consider this a NEW one
				handleTouchRelease(currentSlot);
				handleNewTouch(currentSlot, touchedArea, coord);
				extraTouchDone[currentSlot] = true;
			}
		}
		else if( checkPostFinger(currentSlot, touchedArea, true)) {
			modifierActionsPostFinger(currentSlot, touchedArea, coord[0], coord[1], (float)pressure[currentSlot]); // not new touch, not impulse, post finger [or more]
		}
	}
	// otherwise if not impulse [continuous control]
	else if(!areaOnHold[touchedArea] || keepHoldArea_touch[touchedArea]!=-1) {
		bool isTriggerTouch = (std::find(triggerArea_touches[touchedArea].begin(), triggerArea_touches[touchedArea].end(), currentSlot) != triggerArea_touches[touchedArea].end());


		// check if this is the first touch on this area and it's linked to a modifier [movement or pressure]
		if( checkPreFinger(currentSlot, touchedArea) /*&& keepHoldArea_touch[touchedArea]==-1*/)
			modifierActionsPreFinger(currentSlot, touchedArea, coord[0], coord[1], (float)pressure[currentSlot]);
		// the touch is the triggering touch, the one that has taken the area
		else if(isTriggerTouch || keepHoldArea_touch[touchedArea]==currentSlot) {
			bool isOnBoundary = checkBoundaries(coord[0], coord[1]);
			// if initially on boundary, before it was but now not anymore...
			if(touchOnBoundary[0][currentSlot] && touchOnBoundary[1][currentSlot] && !isOnBoundary) {
				//...this is like a new touch!
				handleTouchRelease(currentSlot);
				handleNewTouch(currentSlot, touchedArea, coord);
			}
			//...maybe it's post finger started when area was on hold! -> we check BUT don't assign
/*			else if(checkPostFinger(currentSlot, touchedArea, true)) {
				modifierActionsPostFinger(currentSlot, touchedArea, coord[0], coord[1], (float)pressure[currentSlot]);
			}*/
			///...otherwise simply not on boundary...
			else if(!isOnBoundary) { // this prevents excitation drag from going over boundaries and delete it
				///...do all you have to do to properly move excitation
				float posX_old = touchPos_prev[0][currentSlot]-windowPos[0];
				float posY_old = touchPos_prev[1][currentSlot]-windowPos[1];
				int coord_old[2] = {(int)posX_old, (int)posY_old};
				getCellDomainCoords(coord_old);

				/*printf("================ TOUCH %d ================\n", currentSlot);
				printf("pos %f --- prev %f ================ %s\n", touchPos[0][currentSlot], touchPos_prev[0][currentSlot], (touchPos[0][currentSlot]!=touchPos_prev[0][currentSlot]) ? "DIFF" : "EQUAL");
				printf("coord x %d --- prev %d\n", coord[0], coord_old[0]);
				printf("coord y %d --- prev %d\n", coord[1], coord_old[1]);
				printf("================ %s ================\n\n", (coord[0]!=coord_old[0] || coord[1]!=coord_old[1]) ? "DIFF" : "EQUAL");*/

				// we move excitation only if the touch has actually moved!
				if(coord[0]!=coord_old[0] || coord[1]!=coord_old[1]) {

					// these are previous and next
					pair<int, int> coord_prev = triggerArea_touch_coords[touchedArea][currentSlot].front();
					pair<int, int> coord_next(coord[0], coord[1]);
					// adds next [we'll crossfade there from current] and deletes previous
					setExcitationCrossfade(touchedArea, coord_next.first, coord_next.second, coord_prev.first, coord_prev.second);

					// remove previous and make shift [current -> prev, next -> current]
					triggerArea_touch_coords[touchedArea][currentSlot].pop_front();
					triggerArea_touch_coords[touchedArea][currentSlot].push_back(coord_next);
				}
				else if(keepHoldArea_touch[touchedArea]==-1)	// continuous volume modulation [with interpolation] only if finger is not moving or not on kept hold, to avoid wobbly effect
					modulateAreaVolPress((double)pressure[currentSlot], touchedArea);
			}
			//...otherwise [generically on boundary, no matter if initially there or just arrived]...
			else if(keepHoldArea_touch[touchedArea]==-1)
				modulateAreaVolPress((double)pressure[currentSlot], touchedArea); //...at least do volume modulation [useful for fixed excitation]

			// continuous modulation of low pass freq [interpolation does not exist here]
			//VIC dist double exProx = 1-dist_getExciteDist(coord[0], coord[1]);
			//VIC dist modulateAreaFiltProx(exProx, touchedArea);

			//VIC dist double boundProx = 1-dist_getBoundDist(coord[0], coord[1]);
			//VIC dist modulateAreaDampProx(boundProx, touchedArea);
			//float delta = expMapping(expMin*0.8, expMax*0.8, expRange, expNorm, 0, 1, 1, boundProx, 0, pressFinger_range[touchedArea][press_dmp]);
			//changeAreaDampingFactor(delta, false, touchedArea);
			//printf("boundProx: %f, delta: %f\n", boundProx, delta);

			//VIC don't forget to deal with drag across different areas [in impulse case no prob, even extra touch automatically handles that]

		}
		// else check if it is post finger and if linked to movement or pressure modifier
		else /*if(keepHoldArea_touch[touchedArea]==-1)*/ if(keepHoldArea_touch[touchedArea] != currentSlot) {
			if( checkPostFinger(currentSlot, touchedArea) )
				modifierActionsPostFinger(currentSlot, touchedArea, coord[0], coord[1], (float)pressure[currentSlot]); // not new touch, not impulse, post finger [or more]
		}
	}
	else if( checkPreFinger(currentSlot, touchedArea) ) {
		modifierActionsPreFinger(currentSlot, touchedArea, coord[0], coord[1], (float)pressure[currentSlot]); // not new touch, not impulse, post finger [or more]
	}
	// else check if it is post finger and if linked to movement -> we check BUT don't assign
	else if( checkPostFinger(currentSlot, touchedArea, true) ) {
		modifierActionsPostFinger(currentSlot, touchedArea, coord[0], coord[1], (float)pressure[currentSlot]); // not new touch, not impulse, post finger [or more]
		// same as modifierActionsPostFinger but checks listenr only
		//if(postFingerMotion[touchedArea]==motion_list && !checkBoundaries(coord[0], coord[1]))
			//moveAreaListener(touchedArea, coord[0], coord[1]);
	}
}


void mouseLeftDrag(float xpos, float ypos);

void mouseRightPressOrDrag(float xpos, float ypos) {
	int coord[2] = {(int)xpos, (int)ypos};
	getCellDomainCoords(coord);
	//resetCell(coord[0], coord[1]);

	if(!modNegated) {
		moveAreaListener(coord[0], coord[1]);
	}
	else
		hideAreaListener();
}

void mouseLeftPress(float xpos, float ypos) {
	// turn to domain coords (:
	int coord[2] = {(int)xpos, (int)ypos};
	getCellDomainCoords(coord);

	int touchedArea = getAreaIndex(coord[0], coord[1]);

	if(!modifierActions(touchedArea, coord[0], coord[1])) {
		//printf("area index: %d\n", touchedArea);

		// percussive: click to trigger
		if(checkPercussive(touchedArea)) {

			// add excitation
			if(!checkBoundaries(coord[0], coord[1])) {
				// add excitation
				setExcitationCell(touchedArea, coord[0], coord[1]);

				pair<int, int> crd(coord[0], coord[1]);
				triggerArea_touch_coords[touchedArea][0].push_back(crd);
			}

			triggerTouch_area[0] = touchedArea;
			triggerArea_touches[touchedArea].push_back(0);

			// trigger
			triggerAreaExcitation(touchedArea);
		}
		// continuous
		else {
			/*
			// mouse down excites area
			//if(triggerArea_touch[touchedArea]==-1) {
			if(triggerArea_touches[touchedArea].empty()) {
				triggerAreaExcitation(touchedArea);
				//triggerArea_touch[touchedArea] = -2; // area taken by [excited via] mouse
				triggerArea_touches[touchedArea].push_back(-2);
			} // mouse down damps already excited area
			else {
				bool mouseTriggered = (std::find(triggerArea_touches[touchedArea].begin(), triggerArea_touches[touchedArea].end(), -2) != triggerArea_touches[touchedArea].end());

				if(mouseTriggered) {
					dampAreaExcitation(touchedArea);
					triggerArea_touches[touchedArea].clear();
					//triggerArea_touch[touchedArea] = -1; // area not taken
				}
			}*/

			// first click to trigger
			if(triggerArea_touches[touchedArea].empty()) {
				//printf("empty continuous area\n");

				// update local excitation status
				triggerArea_touches[touchedArea].push_back(0);
				triggerTouch_area[0] = touchedArea;
				//printf("******touch %d triggers area %d\n", 0, touchedArea);

				// add moving excitation only if not on boundary
				if(!checkBoundaries(coord[0], coord[1])) {

					//printf("\t not boundary\n");
					setExcitationCellDeck(touchedArea, coord[0], coord[1]); // add excitation and prepare crossfade

					// add a fake previous trigger
					pair<int, int> crd(-2, -2);
					triggerArea_touch_coords[touchedArea][0].push_back(crd);
					// then add real one [will see in HandleTouchDrag(...) why]
					crd.first  = coord[0];
					crd.second = coord[1];
					triggerArea_touch_coords[touchedArea][0].push_back(crd);
				}
				else {
					touchOnBoundary[0][0] = true;
					touchOnBoundary[1][0] = true;
				}


				triggerAreaExcitation(touchedArea);

			}
			else {
				pair<int, int> exciteCoord = triggerArea_touch_coords[touchedArea][0].back();
				int dist =  abs((exciteCoord.first-coord[0])*(exciteCoord.first-coord[0]) + (exciteCoord.second-coord[1])*(exciteCoord.second-coord[1]) );
				//printf("exciteCoord: (%d, %d)\n", exciteCoord.first, exciteCoord.second);
				//printf("mouseCoord: (%d, %d)\n", coord[0], coord[1]);


				if(dist>=MIN_DIST/2) {

					bool mouseTriggered = (std::find(triggerArea_touches[touchedArea].begin(), triggerArea_touches[touchedArea].end(), 0) != triggerArea_touches[touchedArea].end());

					if(mouseTriggered) {

						//printf("******touch %d detriggers area %d\n", 0, touchedArea);

						triggerArea_touches[touchedArea].remove(0);
						triggerTouch_area[0] = -1;

						dampAreaExcitation(touchedArea); // and stop excitation
						removeAreaExcitations(touchedArea, 0);


						excitationDeck[touchedArea] = 1-excitationDeck[touchedArea];

						// reset to consider next time this touch is detected
						touchOnBoundary[0][0] = false;
						touchOnBoundary[1][0] = false;
					}
				}
				//else
					//printf("NOTHING\n");

			}
		}
	}

	touchPos_prev[0][0] = xpos;
	touchPos_prev[1][0] = ypos;
}

void mouseLeftRelease() {
	if(triggerTouch_area[0] != -1) {
		int area = triggerTouch_area[0]; // the area we have triggered when this touch first happened

		if(checkPercussive(area)) {
			// remove this touch from list of triggering touches
			triggerArea_touches[area].remove(0);
			triggerTouch_area[0] = -1;

			dampAreaExcitation(area); // and stop excitation
			removeAreaExcitations(area, 0);


			/*if(!checkPercussive(area))
				excitationDeck[area] = 1-excitationDeck[area];*/

			// reset to consider next time this touch is detected
			touchOnBoundary[0][0] = false;
			touchOnBoundary[1][0] = false;
		}
	}

	touchPos_prev[0][0] = -1;
	touchPos_prev[1][0] = -1;

}


void mouseLeftDrag(float xpos, float ypos) {
	// turn to domain coords (:
	int coord[2] = {(int)xpos, (int)ypos};
	getCellDomainCoords(coord);

	int touchedArea = getAreaIndex(coord[0], coord[1]);


	if(!modifierActions(touchedArea, coord[0], coord[1])) {



		if(!checkPercussive(touchedArea)) {
			bool isTriggerTouch = (std::find(triggerArea_touches[touchedArea].begin(), triggerArea_touches[touchedArea].end(), 0) != triggerArea_touches[touchedArea].end());


			// the touch is the triggering touch, the one that has taken the area
			if(isTriggerTouch) {
				bool isOnBoundary = checkBoundaries(coord[0], coord[1]);
				// if initially on boundary, before it was but now not anymore...
				if(touchOnBoundary[0][0] && touchOnBoundary[1][0] && !isOnBoundary) {
					//...this is like a new touch!
					mouseLeftRelease();
					mouseLeftPress(xpos, ypos);
				}
				//...otherwise simply not on boundary...
				else if(!isOnBoundary) { // this prevents excitation drag from going over boundaries and delete it
					///...do all you have to do to properly move excitation
					float posX_old = touchPos_prev[0][0];
					float posY_old = touchPos_prev[1][0];
					int coord_old[2] = {(int)posX_old, (int)posY_old};
					getCellDomainCoords(coord_old);


					// we move excitation only if the touch has actually moved!
					if(coord[0]!=coord_old[0] || coord[1]!=coord_old[1]) {

						//printf("coord(%d, %d) --- coord_old(%d, %d)\n", coord[0], coord[1], coord_old[0], coord_old[1]);

						// these are previous and next
						pair<int, int> coord_prev = triggerArea_touch_coords[touchedArea][0].front();
						pair<int, int> coord_next(coord[0], coord[1]);
						// adds next [we'll crossfade there from current] and deletes previous
						setExcitationCrossfade(touchedArea, coord_next.first, coord_next.second, coord_prev.first, coord_prev.second);

						// remove previous and make shift [current -> prev, next -> current]
						triggerArea_touch_coords[touchedArea][0].pop_front();
						triggerArea_touch_coords[touchedArea][0].push_back(coord_next);
					}
				}
			}
		}
	}

	touchPos_prev[0][0] = xpos;
	touchPos_prev[1][0] = ypos;
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

	//if(multitouch) {
		glfwSetWindowPosCallback(window, win_pos_callback);
		glfwGetWindowPos(window, &windowPos[0], &windowPos[1]);
	//}

	// let's catch ctrl-c to nicely stop the application
	struct sigaction sigIntHandler;
	sigIntHandler.sa_handler = ctrlC_handler;
	sigemptyset(&sigIntHandler.sa_mask);
	sigIntHandler.sa_flags = 0;
	sigaction(SIGINT, &sigIntHandler, NULL);
}


int initControlThread() {
	//@VIC
	// sync with audio thread
	while(!audioeReady)
		usleep(10);

	for(int i=0; i<AREA_NUM; i++) {
		areaModulatedVol[i] = 1;

		deltaLowPassFreq[areaIndex] = areaExLowPassFreq[i]/100.0;
		if(i>0)
			areaExFreq[i] = areaExFreq[0];
		deltaFreq[i] = areaExFreq[i]/100.0;

		areaDampFactor[i] = areaDampFactor[0];
		areaPropagationFactor[i] = areaPropagationFactor[0];

		areaDeltaDampFactor[i] = areaDampFactor[i]*0.1;
		areaDeltaPropFactor[i] = areaPropagationFactor[i]*0.01;

		areaExciteID[i] = drumExId;

		excitationDeck[i] = 0;
	}

	// get names of area excitations [use case of area 0...they're all the same]
	excitationNames = new string[hyperDrumSynth->getAreaExcitationsNum(0)];
	hyperDrumSynth->getAreaExcitationsNames(0, excitationNames);

	// boundary distances handling
	//VIC dist dist_init();

	initMidiControls();
	initOscControls();
	presetManager.init(presetFileFolder, hyperDrumSynth);

	initCallbacks(); //@VIC



	if(!multitouch) {
		maxTouches = 1;
		triggerTouch_area = new int[AREA_NUM];
		for(int i=0; i<AREA_NUM; i++) {
			triggerTouch_area[i] = -1;
			triggerArea_touch_coords[i] = new list<pair<int, int>>[maxTouches];

		}

		touchPos_prev[0] = new float[maxTouches];
		touchPos_prev[1] = new float[maxTouches];
		touchOnBoundary[0] = new bool[maxTouches];
		touchOnBoundary[1] = new bool[maxTouches];
		for(int i=0; i<maxTouches; i++) {
			touchPos_prev[0][i] = -1;
			touchPos_prev[1][i] = -1;
			touchOnBoundary[0][i] = false;
			touchOnBoundary[1][i] = false;
		}
		return 0;
	}
	// if no multitouch, we're done here (:


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

	// read touch surface specs
	int abs[6] = {0};
	ioctl(touchScreenFileHandler, EVIOCGABS(ABS_MT_SLOT), abs);
	maxTouches = abs[2]+1;
	ioctl(touchScreenFileHandler, EVIOCGABS(ABS_MT_POSITION_X), abs);
	maxXpos = abs[2];
	ioctl(touchScreenFileHandler, EVIOCGABS(ABS_MT_POSITION_Y), abs);
	maxYpos = abs[2];
	ioctl(touchScreenFileHandler, EVIOCGABS(ABS_MT_PRESSURE), abs);
	maxPressure = abs[2];
	maxPressure = 700; //VIC cos reading is wrong...cos it's not fucking pressure but area...ANOTHER DISPLAX BUG
	ioctl(touchScreenFileHandler, EVIOCGABS(ABS_MT_TOUCH_MAJOR), abs);
	maxArea = abs[2];

	// init touch vars
	touchOn     = new bool[maxTouches];
	touchPos[0] = new float[maxTouches];
	touchPos[1] = new float[maxTouches];
	//touchPos_prev[0] = new float[maxTouches];
	//touchPos_prev[1] = new float[maxTouches];
	newTouch    = new bool[maxTouches];
	posReceived[0] = new bool[maxTouches];
	posReceived[1] = new bool[maxTouches];
	//touchOnBoundary[0] = new bool[maxTouches];
	//touchOnBoundary[1] = new bool[maxTouches];
	prevTouchedArea = new int[maxTouches];
	boundTouch_areaListener = new int[maxTouches];
	extraTouchDone = new bool[maxTouches];
	touchIsInhibited = new bool[maxTouches];
	pressure    = new int[maxTouches];
	triggerTouch_area   = new int[maxTouches];
	touchChangedAreaOnPreset = new bool[maxTouches];
	preFingersTouch_area   = new int[maxTouches];
	preFingersTouch_motion = new motionFinger_mode[maxTouches];
	preFingersTouch_motionNegated = new bool[maxTouches];
	preFingersTouch_press  = new pressFinger_mode[maxTouches];
	postFingerTouch_area   = new int[maxTouches];
	is_preFingerMotion_dynamic = new bool[maxTouches];
	is_preFingerPress_dynamic  = new bool[maxTouches];
	keepHoldTouch_area = new int[maxTouches];
	touch_elderTwin   = new int[maxTouches];
	touch_youngerTwin = new int[maxTouches];
	touch_coords[0] = new int[maxTouches];
	touch_coords[1] = new int[maxTouches];
	for(int i=0; i<maxTouches; i++) {
		posReceived[0][i] = false;
		posReceived[1][i] = false;
		//touchOnBoundary[0][i] = false;
		//touchOnBoundary[1][i] = false;
		boundTouch_areaListener[i] = -1;
		triggerTouch_area[i] = -1;
		touchChangedAreaOnPreset[i] = false;
		preFingersTouch_area[i] = -1;
		preFingersTouch_motion[i] = motion_none;
		preFingersTouch_motionNegated[i] = false;
		preFingersTouch_press[i] = press_none;
		postFingerTouch_area[i] = -1;
		is_preFingerMotion_dynamic[i] = false;
		is_preFingerPress_dynamic[i] = false;
		//VIC all this shit to fix fucking bugs and faulty behaviors of displax
		extraTouchDone[i] = false;
		touchIsInhibited[i] = false;
		//touchPos_prev[0][i] = -1;
		//touchPos_prev[1][i] = -1;
		keepHoldTouch_area[i] = -1;
		touch_elderTwin[i]   = -1;
		touch_youngerTwin[i] = -1;
		touch_coords[0][i] = -1;
		touch_coords[1][i] = -1;
	}




	for(int i=0; i<AREA_NUM; i++) {
		firstPreFingerMotion[i] = firstPreFingerMotion[0];
		postFingerMotion[i] = postFingerMotion[0];

		firstPreFingerMotionNegated[i] = firstPreFingerMotionNegated[0];
		postFingerMotionNegated[i] = postFingerMotionNegated[0];

		firstPreFingerArea_touch[i] = -1;
		postFingerArea_touch[i] = -1;

		firstPreFingerPress[i] = firstPreFingerPress[0];
		postFingerPress[i] = postFingerPress[0];

		// pressure modifiers control parameters
		// both start as decrement
		pressFinger_range[i][press_prop] = -0.4;
		pressFinger_range[i][press_dmp]  = -0.000950;

		areaOnHold[i] = false;
		holdOnPressure[i][0] = false;
		holdOnPressure[i][1] = false;

		pressFinger_delta[i][press_prop] = areaDeltaPropFactor[i]*0.1;//0.01;
		pressFinger_delta[i][press_dmp]  = areaDeltaDampFactor[i]*0.1; //0.001;

		keepHoldArea_touch[i] = -1;
	}


	// general exponential curve values
	expMax = 2;
	expMin = -8;
	expRange = expMax-expMin;
	expNorm = exp(expMax);
	// special values for impulse case of pressure-volume control
	expMaxImp = 1;
	expMinImp = -3;
	expRangeImp = expMaxImp-expMinImp;
	expNormImp = exp(expMaxImp);
	// special values for boundary proximity-damping control
	expMaxDamp = expMax-expMin*0.01;
	expMinDamp = expMin*0.99;
	expRangeDamp = expMaxDamp-expMinDamp;
	expNormDamp = exp(expMaxDamp);

	// input ranges for pressure
	maxPress = /*maxPressure*0.7*/maxPressure*0.5;
	minPress =/* 20*/200;
	rangePress = maxPress-minPress;
	// special values for impulse case of pressure control
	maxPressImp =/* 230;//30;//230;*/ 230;
	minPressImp = /*20*/20;
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
	maxVolume = 1;
	minVolume = /*0.001*/0.01;
	rangeVolume = maxVolume-minVolume;
	// special values for impulse case of volume control
	maxVolumeImp = /*2*/6;
	minVolumeImp = 0.001;
	rangeVolumeImp = maxVolumeImp-minVolumeImp;
	// output ranges for filter cutoff control
	maxFilter = 1;
	minFilter = 0;
	rangeFilter = maxFilter-minFilter;
	// output ranges for propagation factor and damping controls are defined in pressFinger_range

	return 0;
}

void *runControlLoop(void *) {
	// pre this here
	if(initControlThread()!=0) // cos it syncs with the audio thread and all its settings
		return (void*)0;

	if(verbose==1)
		cout << "__________set Control Thread priority" << endl;

	// set thread niceness
	 set_niceness(-20, verbose); // even if it should be inherited from calling process...but it's not

	 // set thread priority
	set_priority(1, verbose);

	if(verbose==1)
		cout << "_________________Control Thread!" << endl;

	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS,NULL); // now this thread can be cancelled from main()


	//----------------------------------------------------------------------------------
	// keyboard controls
	//----------------------------------------------------------------------------------
	if(!multitouch) {
		/*char keyStroke = '.';
		cout << "Press q to quit." << endl;

		do {
			keyStroke =	getchar();

			while(getchar()!='\n'); // to read the pre stroke

			switch (keyStroke) {
				case 'a':
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
					switchExcitationType(PERCUSSIVE);
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


				default:
					break;
			}
		}
		while (keyStroke != 'q');*/



		while(1) {
			if(mouseButton)
				mouseButtonFunction();
			if(cursorPos)
				cursorPosFunction();
			if(cursorEnter)
				cursorEnterFunction();
			if(winPos)
				winPosFunction();

			usleep(10);
		}

		audioEngine.stopEngine();
		cout << "control thread ended" << endl;

		return (void *)0;
	}

	//----------------------------------------------------------------------------------
	// multitouch controls
	//----------------------------------------------------------------------------------
	// working vars
	const size_t ev_size = sizeof(struct input_event)*MAX_NUM_EVENTS;
	ssize_t readSize;

	int currentSlot = 0;

	struct input_event ev[MAX_NUM_EVENTS]; // where to save event blocks

	// we need to know how big is the window [resolution], to check where we are touching
	GLFWmonitor* mon;
	if(monitorIndex==-1)
		mon = glfwGetPrimaryMonitor();
	else {
		int count = -1;
		mon = glfwGetMonitors(&count)[monitorIndex];

		// if we likely have more monitors. let's use the virtual position of current one to normalize window position
		int monitorPos[2] = {-1, -1};
		glfwGetMonitorPos(mon, &monitorPos[0], &monitorPos[1]);

		// now put window position with respect to current monitor and not to all workspace arrangement
		// [otherwise having monitor on left or right of the other ones would mess up with hit positions]
		windowPos[0] -= monitorPos[0];
		windowPos[1] -= monitorPos[1];
	}
	const GLFWvidmode* vmode = glfwGetVideoMode(mon);
	int monitorReso[] = {vmode->width, vmode->height};

	/*bool needsX[100] = {false};
	bool needsY[100] = {false};
	bool needsPressure[100] = {false};*/

	while(true) {
		readSize = read(touchScreenFileHandler, &ev, ev_size); // we could use select() if we were interested in non-blocking read...i am not, TROLOLOLOLOL

	/*	if(updateNextPreset) {
			hyperDrumheadFrame *frame = presetManager.loadNextPreset(); // this method returns a dynamically allocated frame!
			if(frame!=NULL)
				updateControlStatus(frame);
			updateNextPreset = false;
		}
		else if(updatePrevPreset) {
			hyperDrumheadFrame *frame = presetManager.loadPrevPreset(); // this method returns a dynamically allocated frame!
			if(frame!=NULL)
				updateControlStatus(frame);
			updatePrevPreset = false;
		}*/

		for(unsigned int i = 0;  i <  (readSize / sizeof(struct input_event)); i++) {
			// let's consider only ABS events
			if(ev[i].type == EV_ABS) {
				// the first MT_SLOT determines what touch the following events will refer to
				if(ev[i].code == ABS_MT_SLOT) { // this always arrives first! [except for the very first touch, which is assumed to be 0]
					currentSlot = ev[i].value;
					//printf("ABS_MT_SLOT %d\n", ev[i].value);
				}
				else if(ev[i].code == ABS_MT_TRACKING_ID) {
					//printf("ABS_MT_TRACKING_ID %d\n", ev[i].value);
					if(ev[i].value != -1) {
						if(!touchOn[currentSlot]) {
							//printf("----->touch %d is ON\n", currentSlot);
							newTouch[currentSlot] = true;

							/*if(needsX[currentSlot])
								printf("X FUUUUUUUUCKKKKKKKKKKKKKKKKKKKKKKK!\n");zzzzzzz

							if(needsY[currentSlot])
								printf("Y FUUUUUUUUCKKKKKKKKKKKKKKKKKKKKKKK!\n");

							if(needsPressure[currentSlot])
								printf("PRESSURE FUUUUUUUUCKKKKKKKKKKKKKKKKKKKKKKK!\n");

							needsX[currentSlot] = true;
							needsY[currentSlot] = true;
							needsPressure[currentSlot] = true;*/
						}
						touchOn[currentSlot] = true;
					}
					else {
						//printf("----->touch %d is OFF\n", currentSlot);
						touchOn[currentSlot] = false;
						extraTouchDone[currentSlot] = false;

						handleTouchRelease(currentSlot);
					}
				}
				else if(ev[i].code == ABS_MT_POSITION_X) {
					float pos = (float(ev[i].value)/maxXpos) * monitorReso[0];

					//VIC this is a freaking bug of the displax skin...some new touches are detected as old touches moving
			/*		if(!newTouch[currentSlot] && touchOn[currentSlot] && !extraTouchDone[currentSlot]) {
							//printf("diffX[%d]: %f\n", currentSlot, abs(pos-touchPos[0][currentSlot]));
							if(abs(pos-touchPos[0][currentSlot])>8) {
								extraTouch[currentSlot] = true;
								extraTouchDone[currentSlot]  = true;
								//extraTouchPos[currentSlot][0] = pos;
								//extraTouchPos[currentSlot][1] = touchPos[1][currentSlot];
								//printf("diffX[%d] -> NEW++++++++++++++++++++++++++++++++++++ \n", currentSlot);
							}
					}*/

					if(newTouch[currentSlot])
						touchPos_prev[0][currentSlot] = pos;
					else
						touchPos_prev[0][currentSlot] = touchPos[0][currentSlot];
					touchPos[0][currentSlot] = pos;
					//printf("touch %d pos X: %f\n", currentSlot, touchPos[0][currentSlot]);
					//printf("touch %d pos X: %d\n", currentSlot, ev[i].value);
					//printf("touch_prev %d pos X: %f\n", currentSlot, touchPos_prev[0][currentSlot]);

					posReceived[0][currentSlot] = true;

/*					if(needsX[currentSlot])
						needsX[currentSlot] = false;*/
				}
				else if(ev[i].code == ABS_MT_POSITION_Y) {
					float pos = (float(ev[i].value)/maxYpos) * monitorReso[1];

					//VIC some lines are faulty on the displax skin, so a continuous touch crossing them is assigned a new ID! FUCKING HELL
					// actually for a split second 2 touches are detected across the line [twin touches], then only one remains
					// SO, we're gonna try to figure out this moment of split touches, as follows
					// when there is a new touch, let's check if the distance with all the other ones is below a certain threshold
					// in case, it means that it is the same touch crossing a faulty line, temporary split in 2!
					if(newTouch[currentSlot]) {
						// cycle all other active touches
						for(int j=0; j<maxTouches; j++) {
							if(j!=currentSlot && touchOn[j]) {
								float dist = (touchPos[0][currentSlot]-touchPos[0][j])*(touchPos[0][currentSlot]-touchPos[0][j]);
								dist +=  (pos - touchPos[1][j])*(pos - touchPos[1][j]);
								//printf("%d-%d :%f\n\n", currentSlot, j, dist);

								//if(dist<35 && dist>10)

								// this is it! 2 touches too close to each other to be 2 fingers
								// in particular this is the newly added touch, across the faulty line...the one that will remain
								if(dist<1500) { // empirical value


									pos = swapTouches(j, currentSlot, pos);
									touch_elderTwin[currentSlot] = j;
									touch_youngerTwin[j] = currentSlot;
									// prevent old twin from doing something stupid
									touchIsInhibited[j] = true;
									//printf("touch %d is DISMISSED [current slot is %d]\n", j, currentSlot);
									break;
								}
							}
						}
					}
		/*			//VIC this is a freaking bug of the displax skin...some new touches are detected as old touches moving, very annoying when playing percussions
					else if(touchOn[currentSlot] && !extraTouchDone[currentSlot]) {
						//printf("diffY[%d]: %f\n\n", currentSlot, abs(pos-touchPos[1][currentSlot]));
						// so in some cases we force an extra hit [only one! which is a partial but good fix]
						if(abs(pos-touchPos[1][currentSlot])>8) {
							extraTouch[currentSlot] = true;
							extraTouchDone[currentSlot]  = true;
							//extraTouchPos[currentSlot][0] = touchPos[0][currentSlot];
							//extraTouchPos[currentSlot][1] = pos;
							//printf("diffY[%d] -> NEW++++++++++++++++++++++++++++++++++++ \n", currentSlot);

						}
					}*/

					if(newTouch[currentSlot])
						touchPos_prev[1][currentSlot] = pos;
					else
						touchPos_prev[1][currentSlot] = touchPos[1][currentSlot];
					touchPos[1][currentSlot] = pos;
					//printf("touch %d pos Y: %f\n", currentSlot, touchPos[1][currentSlot]);
					//printf("touch %d pos Y: %d\n\n", currentSlot, ev[i].value);

					posReceived[1][currentSlot] = true;

/*					if(needsY[currentSlot])
						needsY[currentSlot] = false;*/

				}
				else if(ev[i].code == ABS_MT_PRESSURE) {
					//printf("Pressure[%d] %d\n", currentSlot, ev[i].value);
					pressure[currentSlot] = ev[i].value;

/*					if(needsPressure[currentSlot])
						needsPressure[currentSlot] = false;*/

					// if not touching window, fuck off
					if( !( (touchPos[0][currentSlot] >= windowPos[0])&&(touchPos[0][currentSlot] < windowPos[0]+winSize[0]) ) ||
						!( (touchPos[1][currentSlot] >= windowPos[1])&&(touchPos[1][currentSlot] < windowPos[1]+winSize[1]) ) ) {
						newTouch[currentSlot] = false;
						continue;
					}

					if(!posReceived[0][currentSlot])
						touchPos_prev[0][currentSlot] = touchPos[0][currentSlot];
					if(!posReceived[1][currentSlot])
						touchPos_prev[1][currentSlot] = touchPos[1][currentSlot];

					posReceived[0][currentSlot] = false;
					posReceived[1][currentSlot] = false;

					// translate into some kind of mouse pos, relative to window
					float posX = touchPos[0][currentSlot]-windowPos[0];
					float posY = touchPos[1][currentSlot]-windowPos[1];

					//printf("touch pos: %f %f\n", posX, posY);
					//printf("touch pos old: %f %f\n", posX_old, posY_old);

					// then to domain coords (:
					int coord[2] = {(int)posX, (int)posY};
					getCellDomainCoords(coord);

					//printf("touch coord: %d %d\n", coord[0], coord[1]);
					//printf("touch coord old: %d %d\n\n", coord_old[0], coord_old[1]);

					int touchedArea = getAreaIndex(coord[0], coord[1]);

					if(newTouch[currentSlot]) {
						prevTouchedArea[currentSlot] = touchedArea;
						//printf("%d is new touch and prev touched area is set to this area %d\n", currentSlot, touchedArea);
					}

					if(!modifierActions(touchedArea, coord[0], coord[1])) {
						// if no generic modifiers are active
						if( newTouch[currentSlot] /*|| ( (areaExciteID[touchedArea] == PERCUSSIVE) && extraTouch[currentSlot] )*/ ) { // when playing percussions we may handle the extra touch to fix the displax bug
							 handleNewTouch(currentSlot, touchedArea, coord);
						}
						/*else if( (areaExciteID[touchedArea] == PERCUSSIVE) && extraTouch[currentSlot]) {
							handleTouchRelease(currentSlot);
							handleNewTouch(currentSlot, touchedArea, coord);
							extraTouch[currentSlot] = false;
						}*/
						// touch is not new
						else {
							handleTouchDrag(currentSlot, touchedArea, coord);
						}
						changeMousePos(-1, -1); // always
					}
					else
						changeMousePos(touchPos[0][currentSlot], touchPos[1][currentSlot]);

					prevTouchedArea[currentSlot] = touchedArea;
					newTouch[currentSlot] = false;
				}
				/*else if(ev[i].code == ABS_MT_TOUCH_MAJOR) {
					printf("Area %d\n", ev[i].value);
				}*/
			}
		}

		//VIC fuck me, sometimes the extra touch is recognized, but there is no associated pressure call [not even y pos sometimes], so no trigger...this was a fix
		// BUT
		// created incompatibilities with pre/post finger shit [could fix]
		// and above all did not introdcude any advantages
		/*for(int i=0; i<maxTouches; i++) {
			if(extraTouch[i]) {
				int touchedArea = triggerTouch_area[i];

				if(areaExciteID[touchedArea] == PERCUSSIVE) {

					// translate into some kind of mouse pos, relative to window
					float posX = extraTouchPos[i][0]-windowPos[0];
					float posY = extraTouchPos[i][1]-windowPos[1];

					//printf("touch pos: %f %f\n", posX, posY);

					// then to domain coords (:
					int coord[2] = {(int)posX, (int)posY};
					getCellDomainCoords(coord);

					if(!modifierActions(touchedArea, coord[0], coord[1]))
						handleNewTouch(i, touchedArea, coord);
				}
			}
		}*/
		usleep(1);
	}




	return (void *)0;
}


void cleanUpControlLoop() {
	// deallocate, ue'!
	if(multitouch) {
		delete[] touchOn;
		delete[] touchPos[0];
		delete[] touchPos[1];
		delete[] newTouch;
		delete[] pressure;
		delete[] posReceived[0];
		delete[] posReceived[1];
		delete[] prevTouchedArea;
		delete[] boundTouch_areaListener;
		delete[] extraTouchDone;
		//delete[] touchPos_prev[0];
		//delete[] touchPos_prev[1];
		//delete[] touchOnBoundary[0];
		//delete[] touchOnBoundary[1];
		delete[] touchIsInhibited;
		delete[] triggerTouch_area;
		delete[] touchChangedAreaOnPreset;
		delete[] preFingersTouch_area;
		delete[] preFingersTouch_motion;
		delete[] preFingersTouch_motionNegated;
		delete[] preFingersTouch_press;
		delete[] postFingerTouch_area;
		delete[] is_preFingerMotion_dynamic;
		delete[] is_preFingerPress_dynamic;
		delete[] touch_elderTwin;
		delete[] touch_youngerTwin;
		delete[] touch_coords[0];
		delete[] touch_coords[1];


/*
	for(int i=0; i<maxTouches; i++) {
		delete[] extraTouchPos[i];
	}
	delete[] extraTouchPos;
*/

	}

	delete[] touchPos_prev[0];
	delete[] touchPos_prev[1];
	delete[] touchOnBoundary[0];
	delete[] touchOnBoundary[1];

	for(int i=0; i<AREA_NUM; i++)
		delete[] triggerArea_touch_coords[i];

	//VIC dist dist_cleanup();

	stopOsc();


	if(touchScreenFileHandler!=-1) {
		// re-enable mouse control
		enableMouse(touchScreenName);

		close(touchScreenFileHandler);
		printf("Multitouch device nicely shut (:\n");
	}
}


