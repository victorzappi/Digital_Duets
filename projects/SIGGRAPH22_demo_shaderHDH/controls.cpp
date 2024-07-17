/*
 * controls.cpp
 *
 *  Created on: 2015-10-30
 *      Author: Victor Zappi
 */


#include "controls.h"

#include <csignal>  // to catch ctrl-c
#include <string>
#include <array>
#include <vector>
#include <algorithm> // to handle vectors' elements
#include <list>

// for input reading
#include <cstdlib>
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
#include "controls_keyboard.h"
#include "HyperPresetManager.h"
//VIC dist #include "distances.h"


#define MIN_DIST 20//5


using namespace std;


extern int verbose;

//----------------------------------------
// main variables, extern
//----------------------------------------
extern HDH_AudioEngine audioEngine;
extern HyperDrumSynth *hyperDrumSynth;
//----------------------------------------
// click vars
extern AudioFile *clickTrack;
extern int presetClickIndex;
//----------------------------------------



// location variable
extern string presetFileFolder;

//----------------------------------------
// control variables, some externs from main.cpp
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

bool areaExciteOnHold[AREA_NUM];
int keepHoldArea_touch[AREA_NUM];
int *keepHoldTouch_area;
bool areaPressOnHold[AREA_NUM][2];
int areaLastPressure[AREA_NUM];


extern double areaExLowPassFreq[AREA_NUM];
double deltaLowPassFreq[AREA_NUM];

//VIC dist extern double areaExRelease[AREA_NUM];

extern float areaDamp[AREA_NUM];
double areaDeltaDamp[AREA_NUM];
const float maxDamp = 0.05;
const float minDamp = 0;

extern float areaProp[AREA_NUM];
double areaDeltaProp[AREA_NUM];
const float maxProp = 0.5;
const float minProp = 0.0001;


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




string fingerPress_names[press_cnt] = {"inactive", "propagation factor", "damping"};
pressFinger_mode firstPreFingerPress[AREA_NUM] = {press_none};
pressFinger_mode *preFingersTouch_press;

string fingerPinch_names[press_cnt] = {"inactive", "propagation factor", "damping"};
pinchFinger_mode *preFingersTouch_pinch;

pressFinger_mode postFingerPress[AREA_NUM] = {press_none};

float pressFinger_range[AREA_NUM][MAX_PRESS_MODES]; // defined in HyperPixelDrawer.h, cos they are saved!
float pressFinger_delta[AREA_NUM][press_cnt];


bool nextPreFingerMotion_dynamic_wait = false;
motionFinger_mode nextPreFingerMotion_dynamic = motion_none;
bool nextPreFingerPress_dynamic_wait = false;
bool nextPreFingerPress_dynamic_endWait = false;
bool nextPreFingerPress_dynamic_assigned = false;
pressFinger_mode preFingersPress_dynamic = press_none;
bool nextPreFingerMotion_dynamic_negated = false;
bool nextPreFingerMotion_dynamic_endWait = false;
bool nextPreFingerMotion_dynamic_assigned = false;

pinchFinger_mode preFingersPinch_dynamic = pinch_none;
bool nextPreFingerPinch_dynamic_wait = false;

bool *is_preFingerMotion_dynamic;
bool *is_preFingerPress_dynamic;

bool *is_preFingerPinch_dynamic;
bool *is_preFingerPinchRef_dynamic;




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
int *boundTouch_areaDamp;
int *boundTouch_areaProp;
int *boundTouch_areaPinchRef;
int *boundTouch_areaPinch;
int boundAreaListener_touch[AREA_NUM];
int boundAreaProp_touch[AREA_NUM];
int boundAreaDamp_touch[AREA_NUM];
int boundAreaPinchRef_touch[AREA_NUM];
int boundAreaPinch_touch[AREA_NUM];
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


// specific pinch vars
int pinchRef_pendingArea = -1; // there can be only one at a time
int *pinchRef_pos[2];
float pinchRef_areaDamp[AREA_NUM];
float pinchRef_areaProp[AREA_NUM];
float pinchRef_areaDist[AREA_NUM];
// be careful! pinch distance is computed differently than other distances, to make it more lightweight
const int maxPinchDist = 50;
const int minPinchDist = 10;

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
// special values for impulse case of master volume control
double expMaxMasterVol;
double expMinMasterVol;
double expRangeMasterVol;
double expNormMasterVol;

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

int areaIndex = 0;

float bgain = 1;
float deltaBgain = 0.05;

bool showAreas = false;
//----------------------------------------


string *excitationNames;
//----------------------------------------




// this is called by ctrl-c [notes: only intended to work if OpenGL window is not rendered AND it does not work in Eclipse]
void ctrlC_handler(int s){
   printf("\nCaught signal %d\n",s);
   audioEngine.stopEngine();
}



//TODO revise this! much better implementation
double expMapping(double expMin, double expMax, double expRange, double expNorm,
				 double inMin, double inMax, double inRange,
				 double in, double outMin, double outRange) {
	if(in > inMax)
		in = inMax;
	else if(in < inMin)
		in = inMin;
	in = (in-inMin)/inRange;

	return (exp(expMin+expRange*in)/expNorm)*outRange + outMin;
}

void setMasterVolume(double v, bool exp) {
	//printf("vol in %f\n", v);
	if(!exp)
		masterVolume = v;
	else
	{
		//VIC kludge for now, until expMapping is revised!
		if(v>0)
			masterVolume = expMapping(expMinMasterVol, expMaxMasterVol, expRangeMasterVol, expNormMasterVol, 0, 1, 1, v, 0, 1);
		else
			masterVolume = 0;
	}

	//printf("masterVolume %f\n", masterVolume);

	for(int i=0; i<AREA_NUM; i++)
		hyperDrumSynth->setAreaExcitationVolume(i, areaExcitationVol[i]*masterVolume);
}

//----------------------------------------

inline void setAreaDamp(int area, float damp, bool permanent=false) {
	hyperDrumSynth->setAreaDampingFactor(area, damp);
	if(permanent) {
		areaDamp[area] = damp;
		areaDeltaDamp[area] = damp*0.1; // keep ratio constant
	}
}

inline void setAreaProp(int area, float prop, bool permanent=false) {
	hyperDrumSynth->setAreaPropagationFactor(area, prop);
	if(permanent) {
		areaProp[area] = prop;
		areaDeltaProp[area] = prop*0.01;
	}

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

void excite(/*bool all=false*/) {
	/*if(all) {
		hyperDrumSynth->triggerExcitation();
		//printf("excite all!\n");
	}*/
	//else {
		hyperDrumSynth->triggerAreaExcitation(areaIndex);
		//printf("excite area %d!\n", areaIndex);
	//}
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
	areaExciteID[/*areaIndex*/area] = hyperDrumSynth->getAreaExcitationId(/*areaIndex*/area);
	printf("Area %d excitation type: %s\n", area, excitationNames[areaExciteID[area]].c_str());
}

void setAreaExcitationID(int id) {
	hyperDrumSynth->setAreaExcitationID(areaIndex, id);
	areaExciteID[areaIndex] = hyperDrumSynth->getAreaExcitationId(areaIndex);
	if(checkPercussive(areaIndex))
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

// this 'change' function permanently changes vol of current area and is designed to work via keyboard control
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

template <typename T>
void clampValue(T &val, T min, T max) {
	if(val <min)
		val = min;
	else if(val>max)
		val = max;
}

// taken from Bela
inline float map(float x, float in_min, float in_max, float out_min, float out_max)
{
	return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

// these two 'change' functions permanently change the params fo current area and are designed to work via keyboard control
void changeAreaDamp(float deltaD) {
	float nextD = areaDamp[areaIndex]+areaDeltaDamp[areaIndex]*deltaD;

	clampValue<float>(nextD, minDamp, maxDamp);

	setAreaDamp(areaIndex, nextD, true);

	printf("Area %d damping: %f\n", areaIndex, (nextD-minDamp)/(maxDamp-minDamp)); // print as normalized
}
void changeAreaProp(float deltaP) {
	float nextP = areaProp[areaIndex]+areaDeltaProp[areaIndex]*deltaP;

	clampValue<float>(nextP, minProp, maxProp);

	setAreaProp(areaIndex, nextP, true);
	printf("Area %d propagation factor: %f\n", areaIndex, (nextP-minProp)/(maxProp-minProp)); // print as normalized
}
// same for these, but working on press ranges for param control
void changeAreaPropPressRange(float delta){
	pressFinger_range[areaIndex][press_prop] += areaDeltaProp[areaIndex]*delta;
	printf("Area %d pressure-propagation factor modifier range: %f [current value: %f]\n", areaIndex, pressFinger_range[areaIndex][press_prop], areaProp[areaIndex]);
}
void changeAreaDampPressRange(float delta){
	pressFinger_range[areaIndex][press_dmp] += areaDeltaDamp[areaIndex]*delta;
	printf("Area %d pressure-damping modifier range: %f [current value: %f]\n", areaIndex, pressFinger_range[areaIndex][press_dmp], areaDamp[areaIndex]);
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


/*void changeAreaExcitationFixedFreq(int id) {
	areaExFreq[areaIndex] = oscFrequencies[id];
	deltaFreq[areaIndex] = areaExFreq[areaIndex]/100.0; // keep ratio constant
	//hyperDrumSynth->setAreaExcitationFixedFreq(areaIndex, id);

	printf("Area %d Excitation freq: %f [Hz]\n", areaIndex, areaExFreq[areaIndex]);
}*/

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
		printf("\tPropagation [pitch]: %f [press range: %f]\n", areaProp[areaIndex], pressFinger_range[areaIndex][press_prop]);
		printf("\tDamping [resonance]: %f [press range: %f]\n", areaDamp[areaIndex], pressFinger_range[areaIndex][press_dmp]);
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

	//printf("Boundary gain: %f [0:clamped, 1:free]\n", bgain);

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
		nextPreFingerMotion_dynamic_assigned = true; // only to trigger end function correctly
		endPreFingersMotionDynamic();
		//nextPreFingerMotion_dynamic_wait = false;
		//nextPreFingerMotion_dynamic = motion_none;
		printf("Next pre finger motion mode set dismissed\n");
		return;
	}

	nextPreFingerMotion_dynamic = mode;
	nextPreFingerMotion_dynamic_negated = negated;
	nextPreFingerMotion_dynamic_wait = true;
	nextPreFingerMotion_dynamic_assigned = false;
	nextPreFingerMotion_dynamic_endWait = false;
	if(preFingersPress_dynamic == press_none && preFingersPinch_dynamic == pinch_none)
		printf("Next pre finger motion mode set to %s\n", fingerMotion_names[mode].c_str());
	else
		printf("Add to next pre finger motion mode %s\n", fingerMotion_names[mode].c_str());
	if(nextPreFingerMotion_dynamic_negated)
		printf("[negated]\n");
}

void endPreFingersMotionDynamic(){
	if(nextPreFingerMotion_dynamic_assigned){
		nextPreFingerMotion_dynamic_wait = false;
	}else{
		nextPreFingerMotion_dynamic_endWait = true;
	}
}

void changePreFingersPressureDynamic(pressFinger_mode mode) {
	// dismiss if set already
	if(nextPreFingerPress_dynamic_wait && preFingersPress_dynamic == mode) {
		nextPreFingerPress_dynamic_assigned = true; // only to trigger end function correctly
		endPreFingersPressureDynamic();
		printf("Next pre finger pressure mode set dismissed\n");
		return;
	}
	// in case same pinch modifier was pending, dismiss it!
	else if( nextPreFingerPinch_dynamic_wait &&
			((preFingersPinch_dynamic==pinch_dmp && mode==press_dmp) || (preFingersPinch_dynamic==pinch_prop && mode==press_prop)) )
		endPreFingersPinchDynamic();

	preFingersPress_dynamic = mode;
	nextPreFingerPress_dynamic_wait = true;
	nextPreFingerPress_dynamic_endWait = false;
	nextPreFingerPress_dynamic_assigned = false;
	if(nextPreFingerMotion_dynamic == motion_none)
		printf("Next pre finger pressure mode set to %s\n", fingerPress_names[mode].c_str());
	else
		printf("Add to next pre finger pressure mode set to %s\n", fingerPress_names[mode].c_str());

}

void endPreFingersPressureDynamic(){
	if(nextPreFingerPress_dynamic_assigned){
		//printf("end, cos assigned\n");
		nextPreFingerPress_dynamic_wait = false;
	}else{
		//printf("one more, cos not assigned\n");
		nextPreFingerPress_dynamic_endWait = true;
	}
}


// VIC is this broken?
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


void deletePinchRefRecords(int slot) {
	pinchRef_pos[0][slot] = -1;
	pinchRef_pos[1][slot] = -1;

	int pinchedArea = boundTouch_areaPinchRef[slot];

	pinchRef_areaDamp[pinchedArea] = -1;
	pinchRef_areaProp[pinchedArea] = -1;
	pinchRef_areaDist[pinchedArea] = -1;

	boundAreaPinchRef_touch[pinchedArea] = -1;
	boundTouch_areaPinchRef[slot] = -1;

	pinchRef_pendingArea = -1;

	is_preFingerPinchRef_dynamic[slot] = false;
}

void deletePinchRecords(int slot) {
	int pinchedArea = boundTouch_areaPinch[slot];

	pinchRef_areaDist[pinchedArea] = -1;

	boundAreaPinch_touch[pinchedArea] = -1;
	boundTouch_areaPinch[slot] = -1;

	is_preFingerPinch_dynamic[slot] = false;

	preFingersTouch_pinch[slot] = pinch_none;
}

void unbindPinch(int slot, bool resetMod=false, int pinchedArea=-1) {
	//printf("@@----- touch %d is no more pinch for area %d\n", slot, boundTouch_areaPinch[slot]);

	// check if it was linked to any pinch modifier
	if(preFingersTouch_pinch[slot]==pinch_prop) {
		boundAreaProp_touch[boundTouch_areaProp[slot]] = -1;
		boundTouch_areaProp[slot] = -1;
		if(resetMod && pinchRef_areaProp[pinchedArea]!=-1) {
			// reset it to initial pinched ref value
			float nextP = pinchRef_areaProp[pinchedArea];
			setAreaProp(pinchedArea, nextP, true);
		}
	}
	else /*(preFingersTouch_pinch[currentSlot]==pinch_dmp)*/ {
		boundAreaDamp_touch[boundTouch_areaDamp[slot]] = -1;
		boundTouch_areaDamp[slot] = -1;
		if(resetMod && pinchRef_areaDamp[pinchedArea]!=-1) {
			// reset it to initial pinched ref value
			float nextD = pinchRef_areaDamp[pinchedArea];
			setAreaDamp(pinchedArea, nextD, true);
		}
	}
	// delete records
	deletePinchRecords(slot);
}


void changePreFingersPinchDynamic(pinchFinger_mode mode) {
	// dismiss if set already
	if(nextPreFingerPinch_dynamic_wait && preFingersPinch_dynamic==mode) {
		endPreFingersPinchDynamic();
		printf("Next pre finger pinch mode set dismissed\n");
		return;
	}
	// in case same press modifier was pending, dismiss it!
	else if( nextPreFingerPress_dynamic_wait &&
			((preFingersPress_dynamic==press_dmp && mode==pinch_dmp) || (preFingersPress_dynamic==press_prop && mode==pinch_prop)) ) {
		nextPreFingerPress_dynamic_assigned = true; // only to trigger end function correctly
		endPreFingersPressureDynamic();
	}


	preFingersPinch_dynamic = mode;
	nextPreFingerPinch_dynamic_wait = true;
	if(nextPreFingerMotion_dynamic == motion_none)
		printf("Next pre finger pinch mode set to %s\n", fingerPinch_names[mode].c_str());
	else
		printf("Add to next pre finger pinch mode set to %s\n", fingerPinch_names[mode].c_str());
}

void endPreFingersPinchDynamic() {
	// if there is a ref waiting for pinch touch
	if(pinchRef_pendingArea != -1) {
		// delete its records
		int pinchRefSlot = boundAreaPinchRef_touch[pinchRef_pendingArea];
		deletePinchRefRecords(pinchRefSlot);
	}

	nextPreFingerPinch_dynamic_wait = false;
	preFingersPinch_dynamic = pinch_none;
}




void savePreset() {
/*	int retval=hyperDrumSynth->saveFrame(AREA_NUM, areaDamp, areaProp, areaExcitationVol, areaExLowPassFreq, areaExFreq, areaExciteID, areaListenerCoord,
										 (int *)firstPreFingerMotion, firstPreFingerMotionNegated, (int *)firstPreFingerPress, (int *)postFingerMotion, postFingerMotionNegated, (int *)postFingerPress,
										 pressFinger_range, pressFinger_delta, press_cnt, "test.frm");
	if(retval==0)
		printf("Current frame saved\n");
	else if(retval==-1)
		printf("HyperDrumhead not inited, can't save frame file...\n");*/

	hyperDrumheadFrame frame(AREA_NUM);  // create with constructor since conveniently destroyed at the end of scope (:

		for(int i=0; i<AREA_NUM; i++) {
			frame.damp[i] = areaDamp[i];
			frame.prop[i] = areaProp[i];
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
	int retval = hyperDrumSynth->openFrame(AREA_NUM, filename, areaDamp, areaProp, areaExcitationVol, areaExLowPassFreq, areaExFreq, areaExciteID,
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
		// update local control params
		setAreaExcitationID(i, frame->exType[i]);
		areaListenerCoord[i][0]  = frame->listPos[0][i];
		areaListenerCoord[i][1]  = frame->listPos[1][i];
		areaDamp[i]        = frame->damp[i];
		areaProp[i] = frame->prop[i];
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

		if(areaExciteOnHold[i] && !checkPercussive(i)) {
			for(int touch : triggerArea_touches[i])
				triggerTouch_area[touch] = -1;
			triggerArea_touches[i].clear();

			hyperDrumSynth->setAreaMicCanDrag(i, 0); // update mic color

			areaExciteOnHold[i] = false; // hold released

			//TODO it seems that if area put on hold, then preset is loaded and are becomes precussive -> excitation screws up. check what happens!
		}
	}
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
//	changeAreaDamp(deltaD, false, area);
//	//printf("boundProx: %f, delta: %f\n------------------\n", prox, deltaD);
//}

inline float computeNextAreaParamPress(double press, int area, float range, float minVal, float maxVal) {
	clampValue<double>(press, minPress, maxPress);

	float delta/* = expMapping(expMin, expMax, expRange, expNorm, minPress, maxPress, rangePress, press, 0, pressFinger_range[area][mode])*/;
	delta = (press-minPress)/rangePress;
	delta *= range;

	float nextVal = areaProp[area]+delta;

	clampValue<float>(nextVal, minVal, maxVal);

	return nextVal;
}

inline void modulateAreaPropPress(double press, int area) {
	float nextP = computeNextAreaParamPress(press, area, pressFinger_range[area][press_prop], minProp, maxProp);
	setAreaProp(area, nextP);
	//printf("press %f - delta %f\n", press, delta);
}

inline void modulateAreaDampPress(double press, int area) {
	float nextD = computeNextAreaParamPress(press, area, pressFinger_range[area][press_dmp], minDamp, maxDamp);
	setAreaDamp(area, nextD);
	//printf("press %f - delta %f\n", press, delta);
}


inline float computeNextParamPinch(int dist, float initDist, float initVal, float minVal, float maxVal) {
	clampValue<int>(dist, minPinchDist, maxPinchDist);
	float d = map(dist, minPinchDist, maxPinchDist, 0, 1);

	// double linear mapping with helper map function
	float nextVal;
	// fingers getting closer -> decrease val
	if(d<initDist && initVal>minVal) { // also checks that value can be decreased
		float minVal_increased = initVal*0.01; // limit range, to avoid weird sounds
		if(minVal_increased < minVal)
			minVal_increased = minVal;
		nextVal = map(d, 0, initDist, minVal_increased, initVal);
		//printf("-------dist %d, d %f, init dist %f, min %f, max %f, next %f\n", dist, d, initDist, minVal_increased, maxVal, nextVal);
	}
	// fingers getting further away -> increase val
	else if(d>initDist && initVal<maxVal) { // also checks that value can be incrased
		float maxVal_decreased = initVal*10; // limit range, to avoid weird sounds, more than when decreasing, because more prone to weird sounds!
		clampValue<float>(maxVal_decreased, maxVal*0.01, maxVal); // we need to clamp here too, to assure max is not zero [cos initVal can be zero]
		nextVal = map(d, initDist, 1, initVal, maxVal_decreased);
		//printf("+++++++dist %d, d %f, init dist %f, min %f, max %f, next %f\n", dist, d, initDist, minVal, maxVal_decreased, nextVal);
	}
	else
		nextVal = initVal;
	return nextVal;
}

inline void changeAreaPropPinch(int dist, float initDist, float initProp, int area) {
	float nextP = computeNextParamPinch(dist, initDist, initProp, minProp, maxProp);
	setAreaProp(area, nextP, true);
	//printf("+++++++area %d, PINCH dist %d, init prop %f, prop %f\n", area, dist, initProp, nextP);

}

inline void changeAreaDampPinch(int dist, float initDist, float initDamp, int area) {
	float nextD = computeNextParamPinch(dist, initDist, initDamp, minDamp, maxDamp);
	setAreaDamp(area, nextD, true);
	//printf("+++++++area %d, PINCH dist %d, init damp %f, damp %f\n", area, dist, initDamp, nextD);

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
	if(checkPercussive(area))
		return;
	double press = maxPressure*in;
	modulateAreaVolPress(press, area);
}

// can't be inline cos used also in other files [and cannot put in controls.h cos its needs hyperDrumSynth]
//VIC needs exp!!!
void setAreaDampNorm(int area, float in) {
	float damp = map(in, 0, 1, minDamp, maxDamp);
	setAreaDamp(area, damp);
}
// can't be inline cos used also in other files [and cannot put in controls.h cos its needs hyperDrumSynth]
void setAreaPropNorm(int area, float in) {
	float prop = map(in, 0, 1, minProp, maxProp);
	setAreaProp(area, prop);
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
	for(int touch : triggerArea_touches[index])
		triggerTouch_area[touch] = -1;
	triggerArea_touches[index].clear();

	hyperDrumSynth->setAreaMicCanDrag(index, 0); // update mic color
}


void holdExcite(int index){
	areaExciteOnHold[index] = true;
	printf("Area %d excitation on hold!\n", index);

	if(!checkPercussive(index))
		hyperDrumSynth->setAreaMicCanDrag(index, 1); // update mic color
}

void holdPressure(int index) {
	// discard all prefinger pressure touches
	for(int touch : preFingersArea_touches[index]) {

		if(is_preFingerPress_dynamic[touch]) {
			is_preFingerPress_dynamic[touch] = false;


			if(preFingersTouch_press[touch]==press_prop) {
				printf("Area %d prop factor on hold!\n", index);

				areaPressOnHold[index][0] = true;
				// remove binding records
				boundTouch_areaProp[boundAreaProp_touch[index]] = -1;
				boundAreaProp_touch[index] = -1;
			}
			else if(preFingersTouch_press[touch]==press_dmp) {
				printf("Area %d damp on hold!\n", index);
				areaPressOnHold[index][1] = true;
				// remove binding records
				boundTouch_areaDamp[boundAreaDamp_touch[index]] = -1;
				boundAreaDamp_touch[index] = -1;
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
}

// this does the proper checks
void holdAreas() {
	for(int i=0; i<AREA_NUM; i++) {
		if( !areaExciteOnHold[i] && !checkPercussive(i) && !triggerArea_touches[i].empty() )
			holdExcite(i);

		if(!preFingersArea_touches[i].empty())
			holdPressure(i);
	}
}

inline int computeApproxDistance(int a_coordx, int a_coordy, int b_coordx, int b_coordy) {
	return ( abs(a_coordx-b_coordx) + abs(a_coordy-b_coordy) );
}

// squared dist
inline int computeAreaListDist(int area, int coordX, int coordY) {
	return abs((areaListenerCoord[area][0]-coordX)*(areaListenerCoord[area][0]-coordX) + (areaListenerCoord[area][1]-coordY)*(areaListenerCoord[area][1]-coordY) );
}

// squared dist
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

void getOldTouchCoords(int slot, int *coord_old) {
	float posX_old = touchPos_prev[0][slot]-windowPos[0];
	float posY_old = touchPos_prev[1][slot]-windowPos[1];
	coord_old[0] = (int)posX_old;
	coord_old[1] = (int)posY_old;
	getCellDomainCoords(coord_old);
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

//	if(preFingersTouch_press[slot]==press_prop || preFingersTouch_press[slot]==press_dmp) {
//		// no pressure modulation if finger is moving, to avoid wobbly effect
//		float posX_old = touchPos_prev[0][slot]-windowPos[0];
//		float posY_old = touchPos_prev[1][slot]-windowPos[1];
//		int coord_old[2] = {(int)posX_old, (int)posY_old};
//		getCellDomainCoords(coord_old);
//
//		if(coordX==coord_old[0] && coordY==coord_old[1]) {
//			// pressure
//			if(preFingersTouch_press[slot]==press_prop)
//				modulateAreaPropPress(press, area);
//			else if(preFingersTouch_press[slot]==press_dmp)
//				modulateAreaDampPress(press, area);
//		}
//	}
	if(preFingersTouch_press[slot]==press_prop) {
			int coord_old[2];
			getOldTouchCoords(slot, coord_old);
			// no pressure modulation if finger is moving, to avoid wobbly effect
			if(coordX==coord_old[0] && coordY==coord_old[1])
				modulateAreaPropPress(press, area);
	}
	else if(preFingersTouch_press[slot]==press_dmp) {
		int coord_old[2];
		getOldTouchCoords(slot, coord_old);
		// no pressure modulation if finger is moving, to avoid wobbly effect
		if(coordX==coord_old[0] && coordY==coord_old[1])
			modulateAreaDampPress(press, area);
	}
	else if(preFingersTouch_pinch[slot]==pinch_prop) {
		// get details of pinch ref touch
		int pinchedArea = boundTouch_areaPinch[slot];
		int refTouch = boundAreaPinchRef_touch[pinchedArea];

		// just in case we lost some info on the way...
		if(pinchRef_pos[0][refTouch]==-1 || pinchRef_pos[1][refTouch]==-1)
			return;

		int dist = computeApproxDistance(coordX, coordY, pinchRef_pos[0][refTouch], pinchRef_pos[1][refTouch]);
		changeAreaPropPinch(dist, pinchRef_areaDist[pinchedArea], pinchRef_areaProp[pinchedArea], pinchedArea);
	}
	else if(preFingersTouch_pinch[slot]==pinch_dmp) {

		// get details of pinch ref touch
		int pinchedArea = boundTouch_areaPinch[slot];
		int refTouch = boundAreaPinchRef_touch[pinchedArea];

		// just in case we lost some info on the way...
		if(pinchRef_pos[0][refTouch]==-1 || pinchRef_pos[1][refTouch]==-1)
			return;

		int dist = computeApproxDistance(coordX, coordY, pinchRef_pos[0][refTouch], pinchRef_pos[1][refTouch]);
		changeAreaDampPinch(dist, pinchRef_areaDist[pinchedArea], pinchRef_areaDamp[pinchedArea], pinchedArea);
	}
	else if(is_preFingerPinchRef_dynamic[slot]) {
		// update pinch ref position
		pinchRef_pos[0][slot] = coordX;
		pinchRef_pos[1][slot] = coordY;
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




bool assignPreFinger(int slot, int area, int *coord=NULL) {
	// if this touch is not pre finger for this area already AND it is not its post finger either
	if(preFingersTouch_area[slot] == -1 && postFingerTouch_area[slot]==-1) {

		// if any dynamic pre finger change pending, set temporary modifiers
		if(nextPreFingerMotion_dynamic_wait || nextPreFingerPress_dynamic_wait || nextPreFingerPinch_dynamic_wait) {

			int isMotionFinger = false;

			// if dynamic pre finger motion pending
			if( nextPreFingerMotion_dynamic_wait &&
			// AND either dynamic first pre finger is not listener OR it is listener but in this area no touch controls the listener
			( nextPreFingerMotion_dynamic!=motion_list || (nextPreFingerMotion_dynamic==motion_list && boundAreaListener_touch[area]==-1) ) ) {
				is_preFingerMotion_dynamic[slot] = true; // save id for when this finger is released

				// apply temp behavior
				preFingersTouch_motion[slot] = nextPreFingerMotion_dynamic;
				preFingersTouch_motionNegated[slot] = nextPreFingerMotion_dynamic_negated;

				// save records
				preFingersArea_touches[area].push_back(slot);
				preFingersTouch_area[slot] = area;

				nextPreFingerMotion_dynamic_assigned = true;
				if(nextPreFingerMotion_dynamic_endWait)
					nextPreFingerMotion_dynamic_wait = false; // done

				if(preFingersTouch_motion[slot]==motion_list /*&& boundTouch_areaListener[slot]==-1*/) {

					// hook to listener... either a super close one or the one from the area!
					bool hooked = false;
					for(int i=0; i<AREA_NUM; i++){
						if(computeAreaListDist(i, coord[0], coord[1])<MIN_DIST) {
							boundTouch_areaListener[slot] = i;
							boundAreaListener_touch[i] = slot;
							hooked = true;
							break;
						}
					}

					// in case we are far from any lister, we hook up the one from the area
					if(!hooked) {
						// save binding
						boundTouch_areaListener[slot] = area;
						boundAreaListener_touch[area] = slot;
					}
				}

				isMotionFinger = true;
			}

			// continue to check if it is a combined motion+ press OR pinch pre finger

			// if dynamic pre finger pressure pending
			// [may be both motion and pressure]
			if(nextPreFingerPress_dynamic_wait) {

				// if these modifiers are taken already on this area
				if( (preFingersPress_dynamic==press_prop && boundAreaProp_touch[area]!=-1) ||
					(preFingersPress_dynamic==press_dmp && boundAreaDamp_touch[area]!=-1)	)
					return false;

				is_preFingerPress_dynamic[slot] = true; // save id for when this finger is released

				// apply temp behavior
				preFingersTouch_press[slot] = preFingersPress_dynamic;

				if(preFingersPress_dynamic==press_prop) {
					boundTouch_areaProp[slot] = area;
					boundAreaProp_touch[area] = slot;
				} else if(preFingersPress_dynamic==press_dmp) {
					boundTouch_areaDamp[slot] = area;
					boundAreaDamp_touch[area] = slot;
				}

				// add only if not just added
				if(!nextPreFingerMotion_dynamic_wait) {
					preFingersArea_touches[area].push_back(slot);
					preFingersTouch_area[slot] = area;
				}

				nextPreFingerPress_dynamic_assigned = true;
				if(nextPreFingerPress_dynamic_endWait)
					nextPreFingerPress_dynamic_wait = false; // done
			}
			// if pinch is pending
			else if(nextPreFingerPinch_dynamic_wait) {

				//-------------------------------------------------------
				// pinch ref
				//-------------------------------------------------------
				// if this area still needs pinch ref and there is no pinch pending, assign it to this touch
				if(boundAreaPinchRef_touch[area]==-1 && pinchRef_pendingArea==-1) {

					// if these modifiers are taken already on this area
					//if( (preFingersPinch_dynamic==pinch_prop && boundAreaProp_touch[area]!=-1) ||
					//	(preFingersPinch_dynamic==pinch_dmp && boundAreaDamp_touch[area]!=-1)	)
					//	return false;
					if(preFingersPinch_dynamic==pinch_prop && boundAreaProp_touch[area]!=-1) {
						//VIC this is extra check in case something went wrong with swapTouches [DISPLAX broken lines]
						if(is_preFingerPinch_dynamic[boundAreaProp_touch[area]])
							return false;
						else {
							boundAreaProp_touch[area] = -1;
							boundTouch_areaProp[slot] = -1;
						}
					}
					if(preFingersPinch_dynamic==pinch_dmp && boundAreaDamp_touch[area]!=-1) {
						//VIC this is extra check in case something went wrong with swapTouches [DISPLAX broken lines]
						if(is_preFingerPinch_dynamic[boundAreaDamp_touch[area]])
							return false;
						else {
							boundAreaDamp_touch[area] = -1;
							boundTouch_areaDamp[slot] = -1;
						}
					}
					//TODO make sure press discards pinch and vice versa


					is_preFingerPinchRef_dynamic[slot] = true;
					boundAreaPinchRef_touch[area] = slot;
					boundTouch_areaPinchRef[slot] = area;
					pinchRef_pendingArea = area;

					//printf("@++++ touch %d is pinch ref for area %d\n", slot, area);

					// save initial pinch ref pos
					pinchRef_pos[0][slot] = coord[0];
					pinchRef_pos[1][slot] = coord[1];

					preFingersArea_touches[area].push_back(slot);
					preFingersTouch_area[slot] = area;

					return true; // the pinch ref behaves like a pre finger, but it only provides its position as ref!
				}


				//-------------------------------------------------------
				// coupled pinch touch
				//-------------------------------------------------------
				// if too far from ref, ignore!
				int refTouch = boundAreaPinchRef_touch[pinchRef_pendingArea];
				int dist = computeApproxDistance(pinchRef_pos[0][refTouch], pinchRef_pos[1][refTouch], coord[0], coord[1]);
				if(dist > 1.2*maxPinchDist)
					return false;

				// else, set pinch finger to this touch
				is_preFingerPinch_dynamic[slot] = true; // save id for when this finger is released
				// note that we bind this touch to the area of the ref touch, regardless of area that is touching
				//TODO allow for double pinch, damp and prop -> make two sets of pinch records
				boundAreaPinch_touch[pinchRef_pendingArea] = slot;
				boundTouch_areaPinch[slot] = pinchRef_pendingArea;

				// apply temp behavior
				preFingersTouch_pinch[slot] = preFingersPinch_dynamic;

				// update bindings and save initial value for dmp or prop
				if(preFingersPinch_dynamic==pinch_prop) {
					boundTouch_areaProp[slot] = pinchRef_pendingArea;
					boundAreaProp_touch[pinchRef_pendingArea] = slot;
					pinchRef_areaProp[pinchRef_pendingArea] = areaProp[pinchRef_pendingArea];
				} else /*if(preFingersPinch_dynamic==pinch_dmp)*/ {
					boundTouch_areaDamp[slot] = pinchRef_pendingArea;
					boundAreaDamp_touch[pinchRef_pendingArea] = slot;
					pinchRef_areaDamp[pinchRef_pendingArea] = areaDamp[pinchRef_pendingArea];
				}


				preFingersArea_touches[pinchRef_pendingArea].push_back(slot);
				preFingersTouch_area[slot] = pinchRef_pendingArea;

				// save initial dist from pinch ref, normalized!
				dist = computeApproxDistance(coord[0], coord[1], pinchRef_pos[0][refTouch], pinchRef_pos[1][refTouch]);
				clampValue<int>(dist, minPinchDist, maxPinchDist);
				pinchRef_areaDist[pinchRef_pendingArea] = map(dist, minPinchDist, maxPinchDist, 0, 1);

				//printf("@@++++ touch %d is pinch ref for area %d\n", slot, preFingersTouch_area[slot]);

				pinchRef_pendingArea = -1;
				nextPreFingerPinch_dynamic_wait = false; // done

				return true;
			}

			return isMotionFinger;
		}
		// otherwise, if there is a fixed first pre finger behavior set and the first pre finger has not yet arrived, let's set it
		else if( (firstPreFingerMotion[area]!=motion_none || firstPreFingerPress[area]!=press_none) && firstPreFingerArea_touch[area]==-1 &&
				// AND either the fixed first pre finger is not listener OR it is listener but in this area no touch controls the listener
				( firstPreFingerMotion[area]!=motion_list || (firstPreFingerMotion[area]==motion_list && boundAreaListener_touch[area]==-1) ) ) {
			firstPreFingerArea_touch[area] = slot;  // save id for when this finger is released

			// apply fixed behavior
			preFingersTouch_motion[slot] = firstPreFingerMotion[area];
			preFingersTouch_motionNegated[slot] = firstPreFingerMotionNegated[area];
			preFingersTouch_press[slot] = firstPreFingerPress[area];

			// save records
			preFingersArea_touches[area].push_back(slot);
			preFingersTouch_area[slot] = area;

			if(firstPreFingerMotion[area] == motion_list) {
				boundTouch_areaListener[slot] = area;
				boundAreaListener_touch[area] = slot;
			}

			return true;
		}
	}

	return false;
}



inline bool checkPreFinger(int slot, int area, int *coord=NULL) {

	return (preFingersTouch_area[slot] != -1);

	// if there are no pre fingers in this area AND first pre fingers are not fixed
	/*if( preFingersArea_touches[area].empty() &&
		(firstPreFingerMotion[area]==motion_none && firstPreFingerPress[area]==press_none) )
		return false; // no pre finger

	// otherwise simply check if this was pre touch already
	bool touchIsPreFinger = (std::find(preFingersArea_touches[area].begin(), preFingersArea_touches[area].end(), slot) != preFingersArea_touches[area].end());
	return touchIsPreFinger;*/
}


//VIC this should be split in assign() and check()
bool checkPostFinger(int slot, int area, bool noSet=false) {
	if(!noSet) {
		// if post finger is not linked to any modifier
		if( (postFingerMotion[area]==motion_none && postFingerPress[area]==press_none) ||
		// OR set to listener and another touch is controlling this area's listener [listener is controlled AND not controlled by this touch]
			( postFingerMotion[area]==motion_list && (boundAreaListener_touch[area]!=-1 && boundAreaListener_touch[area]!=slot) ) ) {
			return false;
		}

		// if we recognize it as new post touch, set it and return true
		if(!touchIsInhibited[slot] && postFingerArea_touch[area]==-1) { // touchIsInhibited serves to prevent twin touches from doing something stupid [displax bug!]
			postFingerArea_touch[area] = slot;
			postFingerTouch_area[slot] = area;

			if(postFingerMotion[slot] == motion_list) {
				boundTouch_areaListener[slot] = area;
				boundAreaListener_touch[area] = slot;
			}

			return true;
		}
	}

	// otherwise simply check if this was post touch already
	return (postFingerArea_touch[area]==slot);
}


void removeAreaExcitations(int area, int slot, bool reset=true) {
	if(reset) {
		// reset all excitations created by this touch [due to displax bugs...cos it should be just one goddammit!]
		for(pair<int, int> coord : triggerArea_touch_coords[area][slot])
			resetCell(coord.first, coord.second);
	}
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
		areaDamp[i]        = frame->damp[i];
		areaProp[i] = frame->prop[i];
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
		areaDeltaDamp[i] = areaDamp[i]*0.1;
		areaDeltaProp[i] = areaProp[i]*0.01;

		pressFinger_delta[i][press_prop] = areaDeltaProp[i]*0.1;
		pressFinger_delta[i][press_dmp]  = areaDeltaDamp[i]*0.1;
	}

	for(int i=frame->numOfAreas; i<AREA_NUM; i++) {
		areaListenerCoord[i][0]  = -2;
		areaListenerCoord[i][1]  = -2;
	}

	setMasterVolume(masterVolume); // just to re set it to all loaded areas

	// keep excitation in each area, but remove modifier bindings
	for(int i=0; i<AREA_NUM; i++) {

		// remove all modifier bindings! makes interaction cleaner
		for(int touch : preFingersArea_touches[i]) {
			preFingersTouch_area[touch] = -1;
			is_preFingerMotion_dynamic[touch] = false;
			is_preFingerPress_dynamic[touch] = false;
			is_preFingerPinch_dynamic[touch] = false;
			is_preFingerPinchRef_dynamic[touch] = false;
			boundTouch_areaListener[touch] = -1;
			boundTouch_areaDamp[touch] = -1;
			boundTouch_areaProp[touch] = -1;
			boundTouch_areaPinchRef[touch] = -1;
			boundTouch_areaPinch[touch] = -1;
			pinchRef_pos[0][touch] = -1;
			pinchRef_pos[1][touch] = -1;
			boundTouch_areaPinchRef[touch] = -1;
		}
		boundAreaListener_touch[i] = -1;

		boundAreaDamp_touch[i] = -1;
		boundAreaProp_touch[i] = -1;

		pinchRef_areaDamp[i] = -1;
		pinchRef_areaProp[i] = -1;
		pinchRef_areaDist[i] = -1;
		boundAreaPinchRef_touch[i] = -1;
		boundAreaPinch_touch[i] = -1;
		pinchRef_areaDist[i] = -1;
		pinchRef_pendingArea = -1;


		preFingersArea_touches[i].clear();
		firstPreFingerArea_touch[i] = -1;

		if(postFingerArea_touch[i] != -1) {
			int touch = postFingerArea_touch[i];
			postFingerTouch_area[touch] = -1;
			postFingerArea_touch[i] = -1;
			/*if(postFingerMotion[i] == motion_list)
				boundTouch_areaListener[i] = -1;*/
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

			if(areaExciteOnHold[i]) {
				if(newAreaID != i) {
					holdAreaPrepareRelease(i);
					areaExciteOnHold[i] = false;
				}
				holdExcite(newAreaID);
			}

			// however, if we are transitioning to or from a percussive area...
			if( checkPercussive(i, prevExciteID[i])/*(prevExciteID[i]==PERCUSSIVE)*/ || checkPercussive(newAreaID, newExciteID)/*(newExciteID==PERCUSSIVE)*/ ) {
				//...delete the triggers we just added and damp excitation
				resetCell(coord.first, coord.second);
				dampAreaExcitation(i);
				dampAreaExcitation(newAreaID);

				holdAreaPrepareRelease(i);
				areaExciteOnHold[i] = false;
				holdAreaPrepareRelease(newAreaID);
				areaExciteOnHold[newAreaID] = false;
			}
		}
	}

//VIC dist dist_scanBoundDist();
//VIC dist dist_scanExciteDist();
}

void loadNextPreset() {
	hyperDrumheadFrame *frame = presetManager.loadNextPreset(); // this method returns a dynamically allocated frame!
	resetSimulation();

	if(frame!=NULL)
		updateControlStatus(frame);

	hyperDrumSynth->retriggerStreams(); // retrigger all streams
	// check if click needs to be played
	int currentPreset = presetManager.getCurrentPresetIndex();
	if(currentPreset == presetClickIndex)
		clickTrack->retrigger();
	else
		clickTrack->stop();
	//TODO put click in each preset
}

void loadPrevPreset() {
	hyperDrumheadFrame *frame = presetManager.loadPrevPreset(); // this method returns a dynamically allocated frame!
	resetSimulation();

	if(frame!=NULL)
		updateControlStatus(frame);

	hyperDrumSynth->retriggerStreams(); // retrigger all streams
	// check if click needs to be played
	int currentPreset = presetManager.getCurrentPresetIndex();
	if(currentPreset == presetClickIndex)
		clickTrack->retrigger();
	else
		clickTrack->stop();
}

void reloadPreset() {
	hyperDrumheadFrame *frame = presetManager.reloadPreset(); // this method returns a dynamically allocated frame!
	resetSimulation();

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

	//then pre finger controls [there's no else cos if area is on hold touch can be both trigger and pre finger!]
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
			boundAreaListener_touch[boundTouch_areaListener[newTouchID]] = newTouchID;

			//printf("pass pre finger list on %d from %d to %d\n", boundTouch_areaListener[newTouchID], oldTouchID, newTouchID);
		}


		preFingersTouch_press[newTouchID] = preFingersTouch_press[oldTouchID];
		is_preFingerPress_dynamic[newTouchID] = is_preFingerPress_dynamic[oldTouchID];
		preFingersTouch_pinch[newTouchID] = preFingersTouch_pinch[oldTouchID];
		is_preFingerPinch_dynamic[newTouchID] = is_preFingerPinch_dynamic[oldTouchID];
		is_preFingerPinchRef_dynamic[newTouchID] = is_preFingerPinchRef_dynamic[oldTouchID];
		// Update pressure bindings
		if(is_preFingerPress_dynamic[newTouchID]) {
			if(preFingersTouch_press[newTouchID]==press_dmp) {
				boundTouch_areaDamp[newTouchID] = boundTouch_areaDamp[oldTouchID];
				boundAreaDamp_touch[boundTouch_areaDamp[newTouchID]] = newTouchID;
			} else if(preFingersTouch_press[newTouchID]==press_prop) {
				boundTouch_areaProp[newTouchID] = boundTouch_areaProp[oldTouchID];
				boundAreaProp_touch[boundTouch_areaProp[newTouchID]] = newTouchID;
			}
		}
		else if(is_preFingerPinch_dynamic[newTouchID]) {
			boundTouch_areaPinch[newTouchID] = boundTouch_areaPinch[oldTouchID];
			boundAreaPinch_touch[boundTouch_areaPinch[newTouchID]] = newTouchID;

			if(preFingersTouch_pinch[newTouchID]==pinch_dmp) {
				boundTouch_areaDamp[newTouchID] = boundTouch_areaDamp[oldTouchID];
				boundAreaDamp_touch[boundTouch_areaDamp[newTouchID]] = newTouchID;
			} else if(preFingersTouch_pinch[newTouchID]==pinch_prop) {
				boundTouch_areaProp[newTouchID] = boundTouch_areaProp[oldTouchID];
				boundAreaProp_touch[boundTouch_areaProp[newTouchID]] = newTouchID;
			}
		}
		else if(is_preFingerPinchRef_dynamic[newTouchID]) {
			boundTouch_areaPinchRef[newTouchID] = boundTouch_areaPinchRef[oldTouchID];
			boundAreaPinchRef_touch[boundTouch_areaPinchRef[newTouchID]] = newTouchID;
			pinchRef_pos[0][newTouchID] = pinchRef_pos[0][oldTouchID];
			pinchRef_pos[1][newTouchID] = pinchRef_pos[1][oldTouchID];
		}




		// wipe old
		preFingersTouch_area[oldTouchID] = -1;
		// swap the touch id in the areas array
		preFingersArea_touches[area].remove(oldTouchID);
		preFingersArea_touches[area].push_back(newTouchID);
		is_preFingerMotion_dynamic[oldTouchID] = false;
		preFingersTouch_motion[oldTouchID] = motion_none;
		preFingersTouch_motionNegated[oldTouchID] = false;
		is_preFingerPress_dynamic[oldTouchID] = false;
		preFingersTouch_press[oldTouchID] = press_none;
		is_preFingerPinch_dynamic[oldTouchID] = false;
		is_preFingerPinchRef_dynamic[oldTouchID] = false;
		preFingersTouch_pinch[oldTouchID] = pinch_none;
		boundTouch_areaDamp[oldTouchID] = -1;
		boundTouch_areaProp[oldTouchID] = -1;
		boundTouch_areaPinchRef[oldTouchID] = -1;
		boundTouch_areaPinch[oldTouchID] = -1;
		pinchRef_pos[0][oldTouchID] = -1;
		pinchRef_pos[1][oldTouchID] = -1;
	}
	// now post finger
	else if(postFingerTouch_area[oldTouchID]!=-1) {//if(is_preFingerPress_dynamic[oldTouchID]) {
		// it's post finger on area, so assign it the area and swap the touch id in the areas array
		int area = postFingerTouch_area[oldTouchID];
		postFingerTouch_area[newTouchID] = area;

		postFingerArea_touch[area] = newTouchID;

		// update also listener binding
		if(postFingerMotion[area]==motion_list) {
			boundTouch_areaListener[newTouchID] = boundTouch_areaListener[oldTouchID];
			boundTouch_areaListener[oldTouchID] = -1;
			boundAreaListener_touch[boundTouch_areaListener[newTouchID]] = newTouchID;
		}

		if(postFingerPress[area]==press_dmp) {
			boundTouch_areaDamp[newTouchID] = boundTouch_areaDamp[oldTouchID];
			boundTouch_areaDamp[oldTouchID] = -1;
			boundAreaDamp_touch[boundTouch_areaDamp[newTouchID]] = newTouchID;
		}
		else if(postFingerPress[area]==press_prop) {
			boundTouch_areaProp[newTouchID] = boundTouch_areaProp[oldTouchID];
			boundTouch_areaProp[oldTouchID] = -1;
			boundAreaProp_touch[boundTouch_areaProp[newTouchID]] = newTouchID;
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

	touch_coords[0][currentSlot] = coord[0];
	touch_coords[1][currentSlot] = coord[1];

	// assign pre finger modifier, if it's the case; newTouch helps distinguishing real new touches from same touch that travels across areas, as well as from fake touches originating from DISPLAX bug
	if( newTouch[currentSlot] && assignPreFinger(currentSlot, touchedArea, coord) ) {
		//printf("touch %d is new pre finger for area %d\n", currentSlot, preFingersTouch_area[currentSlot]);
	}

	// regular excitation
	if(checkPercussive(touchedArea)) {

		/*if( newTouch[currentSlot] && assignPreFinger(currentSlot, touchedArea, coord) )
				printf("touch %d is new pre finger for area %d\n", currentSlot, touchedArea);*/


		// check if this is first touch on this area and there are special modifiers linked to first touch [movement or pressure]
		if( checkPreFinger(currentSlot, touchedArea, coord/*, newTouch[currentSlot]*/) ) {
			modifierActionsPreFinger(currentSlot, touchedArea, coord[0], coord[1], (float)pressure[currentSlot]);
			if( preFingersTouch_press[currentSlot]==press_prop && areaPressOnHold[touchedArea][0])
				areaPressOnHold[touchedArea][0] = false;
			else if( preFingersTouch_press[currentSlot]==press_dmp && areaPressOnHold[touchedArea][1] )
				areaPressOnHold[touchedArea][1] = false;
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
		}
	}
	// continuous excitation, which needs extra care for areas on hold
	else {
		// first we handle hold case
		if(areaExciteOnHold[touchedArea]) {
			//printf("New touch %d on HOLD %d\n", currentSlot, touchedArea);

			// when on hold can select listener...under the hood we automatically set a listener motion pre finger
			if(computeAreaListDist(touchedArea, coord[0], coord[1])<MIN_DIST && postFingerArea_touch[touchedArea]==-1 && !touchIsInhibited[currentSlot]) {
				if(!nextPreFingerMotion_dynamic_wait || nextPreFingerMotion_dynamic != motion_list) { // to avoid deselection if already selected
					changePreFingersMotionDynamic(motion_list);
					endPreFingersMotionDynamic();
					if(assignPreFinger(currentSlot, touchedArea, coord)) {  //VIC here there is no need for controlling if this is new touch, because we get here only if it is a new touch [if area is on hold, HandleNewTouch() is not called by HandleTouchDrag()!]
						//printf("touch %d is new pre finger for area %d\n", currentSlot, touchedArea);
					}
					//printf("touch %d on mic %d\n", currentSlot, boundTouch_areaListener[currentSlot]);
				}
			}

			// we can also select excitation!
			else if(computeAreaExcitetDist(touchedArea, coord[0], coord[1])<MIN_DIST && (keepHoldArea_touch[touchedArea]==-1)) {
				keepHoldArea_touch[touchedArea] = currentSlot;
				keepHoldTouch_area[currentSlot] = touchedArea;
				//printf("touch %d keeps area %d\n", currentSlot, touchedArea);
				//printf("=========================keepHoldTouch_area[%d]: %d\n", currentSlot, keepHoldTouch_area[currentSlot]);

				int triggerTouch = triggerArea_touches[touchedArea].front();
				if(currentSlot!=triggerTouch) {
					list<int> triggers = triggerArea_touches[touchedArea];
					for(int touch : triggers) {
						removeAreaExcitations(touchedArea, touch);
						triggerArea_touches[touchedArea].remove(touch);
						triggerTouch_area[touch] = -1;
						//printf("---touch %d no more trigger of area %d\n", touch, touchedArea);
					}
				}

			}
			// otherwise we release all the excitations that were stuck there and GO ON with other checks [we actually release hold during those further checks]
			else if(!checkPreFinger(currentSlot, touchedArea, coord) && keepHoldArea_touch[touchedArea]==-1) {

				dampAreaExcitation(touchedArea); // stop excitation
				for(int touch : triggerArea_touches[touchedArea])
					removeAreaExcitations(touchedArea, touch);

				holdAreaPrepareRelease(touchedArea);

				//areaExciteOnHold[touchedArea] = false; // hold will be released further down, when the trigger is handled...
			}
		}

		// then we move on with the usual logic for continuous controllers
		if( checkPreFinger(currentSlot, touchedArea, coord) ) {
			modifierActionsPreFinger(currentSlot, touchedArea, coord[0], coord[1], (float)pressure[currentSlot]);
			// to release pressures controls that are on hold
			if( preFingersTouch_press[currentSlot]==press_prop && areaPressOnHold[touchedArea][0])
				areaPressOnHold[touchedArea][0] = false;
			else if( preFingersTouch_press[currentSlot]==press_dmp && areaPressOnHold[touchedArea][1] )
				areaPressOnHold[touchedArea][1] = false;
		}
		else if(triggerArea_touches[touchedArea].empty() || triggerArea_touches[touchedArea].front() >= maxTouches) { // second check in case area on hold

			// update local excitation status
			triggerArea_touches[touchedArea].push_back(currentSlot);
			triggerTouch_area[currentSlot] = touchedArea;
			//printf("******touch %d triggers area %d\n", currentSlot, touchedArea);

			// add moving excitation only if not on boundary
			if(!checkBoundaries(coord[0], coord[1])) {

				// if regular first touch
				if(!areaExciteOnHold[touchedArea])
					setExcitationCellDeck(touchedArea, coord[0], coord[1]); // add excitation and prepare crossfade
				// but if area was on hold
				else {
					// we kill held touch

					int heldTouch = maxTouches + touchedArea; // held touch index is fixed in every area

					// crossfade from previous excitation on hold!
					pair<int, int> coord_prev(-2, -2);
					//pair<int, int> coord_prev = triggerArea_touch_coords[touchedArea][heldTouch].back();
					pair<int, int> coord_next(coord[0], coord[1]);
					// adds next [we'll crossfade there from current] and deletes previous
					setExcitationCrossfade(touchedArea, coord_next.first, coord_next.second, coord_prev.first, coord_prev.second);

					// reset held touch
					triggerArea_touches[touchedArea].remove(heldTouch);
					triggerTouch_area[heldTouch] = -1;
					triggerArea_touch_coords[touchedArea][heldTouch].clear();
					removeAreaExcitations(touchedArea, heldTouch); // resets the excite cell at the hold trigger coord


					if(keepHoldArea_touch[touchedArea]==-1)
						areaExciteOnHold[touchedArea] = false; // and release hold
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
		//printf("old twin %d of %d let us in peace\n", currentSlot, touch_youngerTwin[currentSlot]);

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

		//-----------------------------------------------------------------------
		// remove this touch from list of triggering touches
		triggerArea_touches[area].remove(currentSlot);
		triggerTouch_area[currentSlot] = -1;

		if(!areaExciteOnHold[area])
			dampAreaExcitation(area); // and stop excitation
		else {
			// if area on hold, we will activate fake touch
			// compute new touch index [fake] as next available maxtouches+area index
			int heldTouch = maxTouches + area; // every area has an extra possible fake touch for when held
			// update trigger touch info for area
			triggerArea_touches[area].push_back(heldTouch);
			triggerTouch_area[heldTouch] = area;
			// save trigger coords
			for(pair<int, int> coord : triggerArea_touch_coords[area][currentSlot])
				triggerArea_touch_coords[area][heldTouch].push_back(coord);
		}

		removeAreaExcitations(area, currentSlot, !areaExciteOnHold[area] || checkPercussive(area)); // resets the excite cell at the trigger coord only if not on hold

		//printf("touch %d no more trigger of area %d\n", currentSlot, area);
		//-----------------------------------------------------------------------



		extraTouchDone[currentSlot] = false;

		if(!checkPercussive(area))
			excitationDeck[area] = 1-excitationDeck[area];

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

		// if was erasing finger, remove area eraser lines
		if( (preFingersTouch_motion[currentSlot] == motion_bound || preFingersTouch_motion[currentSlot] == motion_ex) && preFingersTouch_motionNegated[currentSlot] )
			hideAreaEraser(area);

		//bool wasFirstPreFinger = (firstPreFingerArea_touch[area] == currentSlot);

		// if this was a dynamic pre finger, reset temporary modifiers
		if(is_preFingerMotion_dynamic[currentSlot]) {
			if(preFingersTouch_motion[currentSlot]==motion_list) {
				//printf("which was listener\n");
				boundAreaListener_touch[boundTouch_areaListener[currentSlot]] = -1;
			}
			is_preFingerMotion_dynamic[currentSlot] = false;
			boundTouch_areaListener[currentSlot] = -1;

			preFingersTouch_motion[currentSlot] = motion_none;
			preFingersTouch_motionNegated[currentSlot] = false;
		}
		if(is_preFingerPress_dynamic[currentSlot]) {

			// check if it was linked to any pressure modifier
			if(preFingersTouch_press[currentSlot]==press_prop) {
				boundAreaProp_touch[boundTouch_areaProp[currentSlot]] = -1;
				setAreaProp(area, areaProp[area]); // in case, reset it
				boundTouch_areaProp[currentSlot] = -1;
			}
			else /*(preFingersTouch_press[currentSlot]==press_dmp)*/ {
				boundAreaDamp_touch[boundTouch_areaDamp[currentSlot]] = -1;
				setAreaDamp(area, areaDamp[area]); // in case, reset it
				boundTouch_areaDamp[currentSlot] = -1;
			}

			is_preFingerPress_dynamic[currentSlot] = false;
			preFingersTouch_press[currentSlot] = press_none;
		}
		else if(is_preFingerPinch_dynamic[currentSlot]) {
			int pinchedArea = boundTouch_areaPinch[currentSlot];

			unbindPinch(currentSlot);

			// we are now waiting for a new pinch touch to couple with ref touch
			pinchRef_pendingArea = pinchedArea;
			nextPreFingerPinch_dynamic_wait = true;


		}
		else if(is_preFingerPinchRef_dynamic[currentSlot]) {
			//printf("@----- touch %d is no more pinch ref for area %d\n", currentSlot, boundTouch_areaPinchRef[currentSlot]);
			// if there is a pre finger pinch touch in area where this is ref
			int touchedArea = boundTouch_areaPinchRef[currentSlot];
			int pinchSlot = boundAreaPinch_touch[touchedArea];
			if(pinchSlot != -1)
				unbindPinch(pinchSlot, true, touchedArea);

			// delete ref touch records
			deletePinchRefRecords(currentSlot);
			nextPreFingerPinch_dynamic_wait = false; // in case we are still waiting for coupled pinch touch
			preFingersPinch_dynamic = pinch_none;
		}


		if(firstPreFingerArea_touch[area] == currentSlot) {
			firstPreFingerArea_touch[area] = -1;
			boundTouch_areaListener[currentSlot] = -1;
			boundTouch_areaDamp[currentSlot] = -1;
			boundTouch_areaProp[currentSlot] = -1;

			// for sure remove bond with motion
			preFingersTouch_motion[currentSlot] = motion_none;
			preFingersTouch_motionNegated[currentSlot] = false;

			// check if it was linked to any pressure modifier
			if(firstPreFingerPress[area]==press_prop) {
				setAreaProp(area, areaProp[area]); // in case, reset it
			}
			else if(firstPreFingerPress[area]==press_dmp)
				setAreaDamp(area, areaDamp[area]); // in case, reset it
		}

		// wipe
		preFingersArea_touches[preFingersTouch_area[currentSlot]].remove(currentSlot);
		preFingersTouch_area[currentSlot] = -1;
	}
	else if(postFingerTouch_area[currentSlot]!=-1) {
		int area = postFingerTouch_area[currentSlot]; // they are where it was post finger

		// if was erasing finger, remove area eraser lines
		if( (postFingerMotion[area] == motion_bound || postFingerMotion[area] == motion_ex) && postFingerMotionNegated[area] )
			hideAreaEraser(area);
		else if(postFingerMotion[currentSlot]==motion_list)
			boundAreaListener_touch[boundTouch_areaListener[currentSlot]] = -1;

		// check if it was linked to any pressure modifier
		if(postFingerPress[area]==press_prop) {
			boundAreaProp_touch[boundTouch_areaProp[currentSlot]] = -1;
			setAreaProp(area, areaProp[area]); // in case, reset it
		}
		else if(postFingerPress[area]==press_dmp) {
			boundAreaDamp_touch[boundTouch_areaDamp[currentSlot]] = -1;
			setAreaDamp(area, areaDamp[area]); // in case, reset it
		}
		// all modifiers, motion and press, are kept, cos this is not dynamic assignment

		// finally wipe
		postFingerArea_touch[area] = -1;
		postFingerTouch_area[currentSlot] = -1;
	}

	touchIsInhibited[currentSlot] = false; // just in case
}

void handleTouchDrag(int currentSlot, int touchedArea, int coord[2]) {
	//printf("coord (%d,%d) \t\t prev_coord (%d,%d)\n", coord[0], coord[1], touch_coords[0][currentSlot], touch_coords[1][currentSlot]);
	//int prev_coord[2] = {touch_coords[0][currentSlot], touch_coords[1][currentSlot]};
	touch_coords[0][currentSlot] = coord[0];
	touch_coords[1][currentSlot] = coord[1];

//	if( prev_coord[0] == coord[0] && prev_coord[1] == coord[1]) {
//		printf("No motion\n");
//		return;
//	}

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
				if(!checkPercussive(touchedArea) && !areaExciteOnHold[touchedArea])
					handleNewTouch(currentSlot, touchedArea, coord);
			}
			else { //...in case we don't do anything, cos touch should be inactive
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

	if(checkPercussive(touchedArea)) {
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
	else if(!areaExciteOnHold[touchedArea] || keepHoldArea_touch[touchedArea]!=-1) {
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
			//changeAreaDamp(delta, false, touchedArea);
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
	else if( checkPostFinger(currentSlot, touchedArea, true) )
		modifierActionsPostFinger(currentSlot, touchedArea, coord[0], coord[1], (float)pressure[currentSlot]); // not new touch, not impulse, post finger [or more]
}



void mouseRightPressOrDrag(float xpos, float ypos) {
	resetCell(xpos, ypos);
}

//TODO does not work unless exciation cell is drawn already ----> check volume of excitation
void mouseLeftPress(float xpos, float ypos) {
	// turn to domain coords (:
	int coord[2] = {(int)xpos, (int)ypos};
	getCellDomainCoords(coord);

	int touchedArea = getAreaIndex(coord[0], coord[1]);

	if(!modifierActions(touchedArea, coord[0], coord[1])) {
		//printf("area index: %d\n", touchedArea);
		if(checkPercussive(touchedArea))
			triggerAreaExcitation(touchedArea);
		else {
			// mouse down excites area
			if(triggerArea_touches[touchedArea].empty()) {
				triggerAreaExcitation(touchedArea);
				triggerArea_touches[touchedArea].push_back(-2);
			} // mouse down damps already excited area
			else {
				bool mouseTriggered = (std::find(triggerArea_touches[touchedArea].begin(), triggerArea_touches[touchedArea].end(), -2) != triggerArea_touches[touchedArea].end());

				if(mouseTriggered) {
					dampAreaExcitation(touchedArea);
					triggerArea_touches[touchedArea].clear();
				}
			}
		}

	}
}

void mouseLeftDrag(float xpos, float ypos) {
	// turn to domain coords (:
	int coord[2] = {(int)xpos, (int)ypos};
	getCellDomainCoords(coord);
	modifierActions(getAreaIndex(coord[0], coord[1]), coord[0], coord[1]);
}

// mouse buttons actions
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
	double xpos;
	double ypos;

	// left mouse down
    if(button == GLFW_MOUSE_BUTTON_LEFT) {
    	if(action == GLFW_PRESS) {
			glfwGetCursorPos(window, &xpos, &ypos);
			//printf("Mouse position: (%f, %f)\n", xpos, ypos);

			if(!mouseLeftPressed)
				mouseLeftPress(xpos, ypos);
			else
				mouseLeftDrag(xpos, ypos);
			mouseLeftPressed = true;  // start dragging
			//printf("left mouse pressed\n");
    	} else if(action == GLFW_RELEASE) {
    		mouseLeftPressed = false; // stop dragging
    		//printf("left mouse released\n");
    	}
    }
	// right mouse down
    else  if(button == GLFW_MOUSE_BUTTON_RIGHT) {
    	if(action == GLFW_PRESS) {
			glfwGetCursorPos(window, &xpos, &ypos);
			mouseRightPressOrDrag(xpos, ypos);
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

	// left mouse dragged
	if(mouseLeftPressed)
		mouseLeftDrag(xpos, ypos);
	// right mouse dragged
	else if(mouseRightPressed)
		mouseRightPressOrDrag(xpos, ypos);
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
		areaModulatedVol[i] = 1;

		deltaLowPassFreq[areaIndex] = areaExLowPassFreq[i]/100.0;
		if(i>0)
			areaExFreq[i] = areaExFreq[0];
		deltaFreq[i] = areaExFreq[i]/100.0;

		areaDamp[i] = areaDamp[0];
		areaProp[i] = areaProp[0];

		areaDeltaDamp[i] = areaDamp[i]*0.1;
		areaDeltaProp[i] = areaProp[i]*0.01;

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

	if(!multitouch) {
		triggerTouch_area = new int[AREA_NUM];
		for(int i=0; i<AREA_NUM; i++)
			triggerTouch_area[i] = -1;
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
	maxPressure = 700; //VIC cos reading is wrong...cos it's not fucking pressure but area... ANOTHER DISPLAX BUG
	ioctl(touchScreenFileHandler, EVIOCGABS(ABS_MT_TOUCH_MAJOR), abs);
	maxArea = abs[2];

	// init touch vars
	touchOn     = new bool[maxTouches];
	touchPos[0] = new float[maxTouches];
	touchPos[1] = new float[maxTouches];
	touchPos_prev[0] = new float[maxTouches];
	touchPos_prev[1] = new float[maxTouches];
	newTouch    = new bool[maxTouches];
	posReceived[0] = new bool[maxTouches];
	posReceived[1] = new bool[maxTouches];
	touchOnBoundary[0] = new bool[maxTouches];
	touchOnBoundary[1] = new bool[maxTouches];
	prevTouchedArea = new int[maxTouches];
	boundTouch_areaListener = new int[maxTouches];
	boundTouch_areaDamp = new int[maxTouches];
	boundTouch_areaProp = new int[maxTouches];
	boundTouch_areaPinchRef = new int[maxTouches];
	boundTouch_areaPinch = new int[maxTouches];
	extraTouchDone = new bool[maxTouches];
	touchIsInhibited = new bool[maxTouches];
	pressure    = new int[maxTouches];
	triggerTouch_area   = new int[maxTouches+AREA_NUM]; // some extra fake touches for held areas, one per each area
	touchChangedAreaOnPreset = new bool[maxTouches];
	preFingersTouch_area   = new int[maxTouches];
	preFingersTouch_motion = new motionFinger_mode[maxTouches];
	preFingersTouch_motionNegated = new bool[maxTouches];
	preFingersTouch_press  = new pressFinger_mode[maxTouches];
	preFingersTouch_pinch  = new pinchFinger_mode[maxTouches];
	postFingerTouch_area   = new int[maxTouches];
	is_preFingerMotion_dynamic = new bool[maxTouches];
	is_preFingerPress_dynamic  = new bool[maxTouches];
	is_preFingerPinch_dynamic  = new bool[maxTouches];
	is_preFingerPinchRef_dynamic = new bool[maxTouches];
	keepHoldTouch_area = new int[maxTouches];
	touch_elderTwin   = new int[maxTouches];
	touch_youngerTwin = new int[maxTouches];
	touch_coords[0] = new int[maxTouches];
	touch_coords[1] = new int[maxTouches];
	pinchRef_pos[0] = new int[maxTouches];
	pinchRef_pos[1] = new int[maxTouches];

	for(int i=0; i<maxTouches; i++) {
		posReceived[0][i] = false;
		posReceived[1][i] = false;
		touchOnBoundary[0][i] = false;
		touchOnBoundary[1][i] = false;
		boundTouch_areaListener[i] = -1;
		boundTouch_areaDamp[i] = -1;
		boundTouch_areaProp[i] = -1;
		boundTouch_areaPinchRef[i] = -1;
		boundTouch_areaPinch[i] = -1;
		triggerTouch_area[i] = -1;
		touchChangedAreaOnPreset[i] = false;
		preFingersTouch_area[i] = -1;
		preFingersTouch_motion[i] = motion_none;
		preFingersTouch_motionNegated[i] = false;
		preFingersTouch_press[i] = press_none;
		preFingersTouch_pinch[i] = pinch_none;
		postFingerTouch_area[i] = -1;
		is_preFingerMotion_dynamic[i] = false;
		is_preFingerPress_dynamic[i] = false;
		is_preFingerPinch_dynamic[i] = false;
		is_preFingerPinchRef_dynamic[i] = false;
		//VIC all this shit to fix fucking bugs and faulty behaviors of displax
		extraTouchDone[i] = false;
		touchIsInhibited[i] = false;
		touchPos_prev[0][i] = -1;
		touchPos_prev[1][i] = -1;
		keepHoldTouch_area[i] = -1;
		touch_elderTwin[i]   = -1;
		touch_youngerTwin[i] = -1;
		touch_coords[0][i] = -1;
		touch_coords[1][i] = -1;
		pinchRef_pos[0][i] = -1;
		pinchRef_pos[1][i] = -1;
	}




	for(int i=0; i<AREA_NUM; i++) {
		triggerArea_touch_coords[i] = new list<pair<int, int>>[maxTouches+AREA_NUM]; // some extra fake touches for held areas, one per each area

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

		areaExciteOnHold[i] = false;
		areaPressOnHold[i][0] = false;
		areaPressOnHold[i][1] = false;

		pressFinger_delta[i][press_prop] = areaDeltaProp[i]*0.1;//0.01;
		pressFinger_delta[i][press_dmp]  = areaDeltaDamp[i]*0.1; //0.001;

		keepHoldArea_touch[i] = -1;

		boundAreaListener_touch[i] = -1;
		boundAreaDamp_touch[i] = -1;
		boundAreaProp_touch[i] = -1;
		boundAreaPinchRef_touch[i] = -1;
		boundAreaPinch_touch[i] = -1;

		pinchRef_areaDamp[i] = -1;
		pinchRef_areaProp[i] = -1;
		pinchRef_areaDist[i] = -1;
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
	// special values for master volume control
	expMaxMasterVol = 0;
	expMinMasterVol = -4.5;
	expRangeMasterVol = expMaxMasterVol-expMinMasterVol;
	expNormMasterVol = exp(expMaxMasterVol);

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
		char keyStroke = '.';
		cout << "Press q to quit." << endl;

		do {
			keyStroke =	getchar();

			while(getchar()!='\n'); // to read the pre stroke

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


	while(true) {
		readSize = read(touchScreenFileHandler, &ev, ev_size); // we could use select() if we were interested in non-blocking read...i am not, TROLOLOLOLOL

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
				}
				else if(ev[i].code == ABS_MT_PRESSURE) {
					//printf("Pressure[%d] %d\n", currentSlot, ev[i].value);
					pressure[currentSlot] = ev[i].value;

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
						if(newTouch[currentSlot]) { // when playing percussions we may handle the extra touch to fix the displax bug
							 handleNewTouch(currentSlot, touchedArea, coord);
						}
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
		// and above all did not introduce any advantages
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
		delete[] touchOnBoundary[0];
		delete[] touchOnBoundary[1];
		delete[] prevTouchedArea;
		delete[] boundTouch_areaListener;
		delete[] boundTouch_areaProp;
		delete[] boundTouch_areaDamp;
		delete[] boundTouch_areaPinchRef;
		delete[] boundTouch_areaPinch;
		delete[] extraTouchDone;
		delete[] touchPos_prev[0];
		delete[] touchPos_prev[1];
		delete[] touchIsInhibited;
		delete[] triggerTouch_area;
		delete[] touchChangedAreaOnPreset;
		delete[] preFingersTouch_area;
		delete[] preFingersTouch_motion;
		delete[] preFingersTouch_motionNegated;
		delete[] preFingersTouch_press;
		delete[] preFingersTouch_pinch;
		delete[] postFingerTouch_area;
		delete[] is_preFingerMotion_dynamic;
		delete[] is_preFingerPress_dynamic;
		delete[] is_preFingerPinch_dynamic;
		delete[] is_preFingerPinchRef_dynamic;
		delete[] touch_elderTwin;
		delete[] touch_youngerTwin;
		delete[] touch_coords[0];
		delete[] touch_coords[1];
		delete[] pinchRef_pos[0];
		delete[] pinchRef_pos[1];


/*
	for(int i=0; i<maxTouches; i++) {
		delete[] extraTouchPos[i];
	}
	delete[] extraTouchPos;
*/

		for(int i=0; i<AREA_NUM; i++)
			delete[] triggerArea_touch_coords[i];
	}
	//VIC dist dist_cleanup();

	stopOsc();


	if(touchScreenFileHandler!=-1) {
		// re-enable mouse control
		enableMouse(touchScreenName);

		close(touchScreenFileHandler);
		printf("Multitouch device nicely shut (:\n");
	}
}


