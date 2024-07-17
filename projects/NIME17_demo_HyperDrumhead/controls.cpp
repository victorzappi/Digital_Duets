/*
 * controls.cpp
 *
 *  Created on: 2015-10-30
 *      Author: Victor Zappi
 */




#include "controls.h"

#include <signal.h> // to catch ctrl-c
#include <string>

// for input reading
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/input.h>

// temporarily disable mouse control via multitouch interface
#include "disableMouse.h"

// check with:
// cat /proc/bus/input/devices
// OR simply evtest
std::string event_device_path = "/dev/input/event";
//#define EVENT_DEVICE    "/dev/input/event10"
//#define MOUSE_INPUT_ID 10 // check with: xinput -list
#define MAX_NUM_EVENTS 64 // found here http://ozzmaker.com/programming-a-touchscreen-on-the-raspberry-pi/
#define MAX_X_POS 4095 // checked with evtest [from apt-get repo]
#define MAX_Y_POS 4095 // checked with evtest too
#define MAX_TOUCHES 10


//back end
#include "Engine.h"
#include "priority_utils.h"

// applications stuff
#include "HyperDrumSynth.h"
#include "DrumExcitation.h"



extern int verbose;

//----------------------------------------
// main variables
//----------------------------------------
// extern
extern AudioEngine audioEngine;
extern HyperDrumSynth *hyperDrumSynth;
//----------------------------------------

//----------------------------------------
// control variables
//---------------------------------------
extern bool multitouch;
extern int monitorIndex;
extern int touchScreenEvent;
extern int touchScreenMouseID;
int touchScreenFileHandler = -1;
int windowPos[2] = {-1, -1};

extern int listenerCoord[2];
extern int areaListenerCoord[AREA_NUM][2];
extern float excitationVol;
extern float areaExcitationVol[AREA_NUM];
extern drumEx_type drumExType;

int deltaPos        = 1;
float deltaVolume = 0.05;

float freq      = 440.0;
float deltaFreq = freq/100.0;  //TODO each should have a different one
float oscFrequencies[FREQS_NUM] = {50, 100, 200, 400, 800, 1600, 3200, 6400};

extern float areaExLowPassFreq[FREQS_NUM];
float deltaLowPassFreq; //TODO each should have a different one

extern float areaDampFactor[AREA_NUM];
float deltaDampFactor; //TODO each should have a different one

extern float areaPropagationFactor[AREA_NUM];
float deltaPropFactor; //TODO each should have a different one

drumEx_type areaExciteType[AREA_NUM];
int areaTouch[AREA_NUM];
int touchArea[MAX_TOUCHES];

enum mode2ndFinger {m2nd_bound, m2nd_ex, m2ndList};
mode2ndFinger secondFingerMode = m2ndList;
bool modeNegated = false; // to erase with first two modes


GLFWwindow *window = NULL;
int winSize[2] = {-1, -1};

double mousePos[2]      = {-1, -1};
bool mouseLeftPressed   = false;
bool mouseRightPressed  = false;
bool mouseCenterPressed = false;
bool shiftIsPressed     = false;
bool altIsPressed       = false;
bool leftCtrlIsPressed  = false;
bool rightCtrlIsPressed = false;
bool zIsPressed = false;
bool xIsPressed = false;
bool cIsPressed = false;
bool vIsPressed = false;
int stickyZ = -1;
int stickyX = -1;
int stickyC = -1;
bool stickyNegated = false; // to erase with stickyZ or stickyX

int areaIndex = 0;

float bgain = 0;
float deltaBgain = 0.05;

bool showAreas = false;
//----------------------------------------







// this is called by ctrl-c
void my_handler(int s){
   printf("\nCaught signal %d\n",s);
   audioEngine.stopEngine();
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

void setBoundaryCell(float posX, float posY) {
	int coord[2] = {(int)posX, (int)posY};
	getCellDomainCoords(coord);

	hyperDrumSynth->setCell(coord[0], coord[1], areaIndex, HyperDrumhead::cell_boundary, bgain);
}

void setBoundaryCell(int index, float posX, float posY) {
	int coord[2] = {(int)posX, (int)posY};
	getCellDomainCoords(coord);

	hyperDrumSynth->setCell(coord[0], coord[1], index, HyperDrumhead::cell_boundary, bgain);
}

void setExcitationCell(float posX, float posY) {
	int coord[2] = {(int)posX, (int)posY};
	getCellDomainCoords(coord);

	hyperDrumSynth->setCellType(coord[0], coord[1], HyperDrumhead::cell_excitation); // leave area as it is [safer?]
	//hyperDrumSynth->setCell(coord[0], coord[1], areaIndex, HyperDrumhead::cell_excitation); // changes also area [more prone to human errors?]
}

void setCellArea(float mouseX, float mouseY) {
	int coord[2] = {(int)mouseX, (int)mouseY};
	getCellDomainCoords(coord);

	hyperDrumSynth->setCellArea(coord[0], coord[1], areaIndex);
}

void resetCell(float mouseX, float mouseY) {
	int coord[2] = {(int)mouseX, (int)mouseY};
	getCellDomainCoords(coord);

	hyperDrumSynth->resetCellType(coord[0], coord[1]);
}

void clearDomain() {
	hyperDrumSynth->clearDomain();
}

void resetSimulation() {
	hyperDrumSynth->resetSimulation();
}

void switchShowAreas() {
	showAreas = !showAreas;
	hyperDrumSynth->setShowAreas(showAreas);
}

int getAreaIndex(float posX, float posY) {
	int coord[2] = {(int)posX, (int)posY};
	getCellDomainCoords(coord);

	return hyperDrumSynth->getCellArea(coord[0], coord[1]);
}

void triggerAreaExcitation(float mouseX, float mouseY) {
	int index = getAreaIndex(mouseX, mouseY);
	hyperDrumSynth->triggerAreaExcitation(index);
	//printf("excite area %d!\n", index);
}

inline void triggerAreaExcitation(int index) {
	hyperDrumSynth->triggerAreaExcitation(index);
	//printf("excite area %d!\n", index);
}

inline void dampAreaExcitation(int index) {
	hyperDrumSynth->dampAreaExcitation(index);
	//printf("excite area %d!\n", index);
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

void switchExcitationType(drumEx_type exciteType) {
	hyperDrumSynth->setExcitationType(exciteType);
	printf("Current excitation type: %s\n", DrumExcitation::excitationNames[exciteType].c_str());
}

void switchAreaExcitationType(drumEx_type exciteType) {
	hyperDrumSynth->setAreaExcitationType(areaIndex, exciteType);
	areaExciteType[areaIndex] = exciteType;
	printf("Area %d excitation type: %s\n", areaIndex, DrumExcitation::excitationNames[exciteType].c_str());

	int currentSlot = areaTouch[areaIndex];
	areaTouch[areaIndex] = -1; // reset area
	if(currentSlot>-1) // to cope with mouse pointer case
		touchArea[currentSlot] = -1; // reset touch
}

// from GLFW window coordinates
void moveListener(float mouseX, float mouseY) {
	int coord[2] = {(int)mouseX, (int)mouseY};
	getCellDomainCoords(coord);

	// finally move listener
	if(hyperDrumSynth->setListenerPosition(coord[0], coord[1]) == 0) {
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

// from GLFW window coordinates
void moveAreaListener(float posX, float posY) {
	int coord[2] = {(int)posX, (int)posY};
	getCellDomainCoords(coord);

	// finally move listener
	if(hyperDrumSynth->setAreaListenerPosition(areaIndex, coord[0], coord[1]) == 0) {
		hyperDrumSynth->getAreaListenerPosition(areaIndex, areaListenerCoord[areaIndex][0], areaListenerCoord[areaIndex][1]);
		//printf("Area %d Listener position (in pixels): (%d, %d)\n", areaIndex, areaListenerCoord[areaIndex][0], areaListenerCoord[areaIndex][1]); //get pos
	}
}

void moveAreaListener(int index, float posX, float posY) {
	int coord[2] = {(int)posX, (int)posY};
	getCellDomainCoords(coord);

	// finally move listener
	if(hyperDrumSynth->setAreaListenerPosition(index, coord[0], coord[1]) == 0) {
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


void changeExcitationVolume(float delta) {
	excitationVol += delta;
	hyperDrumSynth->setExcitationVolume(excitationVol);
	printf("Excitation volume: %.2f\n", excitationVol);
}

void changeAreaExcitationVolume(float delta) {
	areaExcitationVol[areaIndex] += delta;
	hyperDrumSynth->setAreaExcitationVolume(areaIndex, areaExcitationVol[areaIndex]);
	printf("Area %d excitation volume: %.2f\n", areaIndex, areaExcitationVol[areaIndex]);
}

void changeAreaDampingFactor(float deltaDF) {
	float nextD = areaDampFactor[areaIndex]+deltaDF;
	if(nextD >=0) {
		hyperDrumSynth->setAreaDampingFactor(areaIndex, nextD);
		areaDampFactor[areaIndex] = nextD;
		deltaDampFactor = nextD/100.0; // keep ratio constant
		printf("Area %d damping factor: %f\n", areaIndex, areaDampFactor[areaIndex]);
	}
}

void changeAreaPropagationFactor(float deltaP) {
	float nextP = areaPropagationFactor[areaIndex]+deltaP;
	if(nextP<0)
		nextP = 0.0001;
	else if(nextP>0.5)
		nextP = 0.5;

	hyperDrumSynth->setAreaPropagationFactor(areaIndex, nextP);
	areaPropagationFactor[areaIndex] = nextP;
	printf("Area %d propagation factor: %f\n", areaIndex, areaPropagationFactor[areaIndex]);
}

void changeExcitationFreq(float deltaF) {
	float nextF = freq+deltaF;
	if(nextF >=0) {
		hyperDrumSynth->setExcitationFreq(nextF);
		freq = nextF;
		deltaFreq = nextF/100.0; // keep ratio constant
		printf("Excitation freq: %f [Hz]\n", freq);
	}
}

void changeAreaExcitationFreq(float deltaF) {
	float nextF = freq+deltaF;
	if(nextF >=0) {
		hyperDrumSynth->setAreaExcitationFreq(areaIndex, nextF);
		freq = nextF;
		deltaFreq = nextF/100.0; // keep ratio constant
		printf("Area %d Excitation freq: %f [Hz]\n", areaIndex, freq);
	}
}

void changeExcitationFixedFreq(int id) {
	hyperDrumSynth->setExcitationFixedFreq(id);
	freq = oscFrequencies[id];
	deltaFreq = freq/100.0; // keep ratio constant
	printf("Excitation freq: %f [Hz]\n", freq);
}

void changeAreaExcitationFixedFreq(int id) {
	hyperDrumSynth->setAreaExcitationFixedFreq(areaIndex, id);
	freq = oscFrequencies[id];
	deltaFreq = freq/100.0; // keep ratio constant

	printf("Area %d Excitation freq: %f [Hz]\n", areaIndex, freq);
}

void changeAreaLowPassFilterFreq(float deltaF) {
	float nextF = areaExLowPassFreq[areaIndex]+deltaF;
	if(nextF >=0 && nextF<= 22050) {
		hyperDrumSynth->setAreaExLowPassFreq(areaIndex, nextF);
		areaExLowPassFreq[areaIndex] = nextF;
		deltaLowPassFreq = nextF/100.0;
		printf("Area %d excitation low pass freq: %f [Hz]\n", areaIndex, areaExLowPassFreq[areaIndex]);
	}
	else
		printf("Requested excitation low pass freq %f [Hz] is out range [0, 22050 Hz]\n", nextF);
}

void changeAreaIndex(int index) {
	if(index<AREA_NUM) {
		areaIndex = index;
		printf("Current Area index: %d\n", areaIndex);
	}
	else
		printf("Cannot switch to Area index %d cos avalable areas are 0 to %d /:\n", index, AREA_NUM-1);
}

void changeBgain(float deltaB) {
	bgain += deltaB;
	if(bgain <0)
		bgain = 0;
	else if(bgain>1)
		bgain = 1;
	printf("Current bgain: %f\n", bgain);

}





void saveFrame() {
	/*int retval=//hyperDrumSynth->saveFrame();
	if(retval==0)
		printf("Current frame saved\n");
	else if(retval==-1)
		printf("Init Vocal Synthesizer before trying to save frame file!\n");*/
}

void openFrame() {
	/*string filename = "data/seq_0_120x60/framedump_0.frm";
	int retval = //hyperDrumSynth->openFrame(filename);
	//string filename = "data/seq_0.fsq";
	//int retval = //hyperDrumSynth->openFrameSequence(filename);
	if(retval==0)
		printf("Loaded frame from file \"%s\"\n", filename.c_str());
	else if(retval==-1)
		printf("Init Vocal Synthesizer before trying to open frame file!\n");*/
}









bool modifierActions(double xpos, double ypos) {
	bool retval = false;
	if(zIsPressed) {
		if(!shiftIsPressed)
			setBoundaryCell(xpos, ypos);
		else
			resetCell(xpos, ypos);
		retval = true;
	}
	else if(xIsPressed) {
		if(!shiftIsPressed)
			setExcitationCell(xpos, ypos);
		else
			resetCell(xpos, ypos);
		retval = true;
	}
	else if(cIsPressed){
		if(!shiftIsPressed) {
				moveAreaListener(xpos, ypos);
		}
		else
			hideAreaListener();
		retval = true;
	}
	else if(vIsPressed) {
		setCellArea(xpos, ypos);
		retval = true;
	}
	if(retval)
		changeMousePos(xpos, ypos);
	return retval;
}

bool modifierActions(int touch, double xpos, double ypos) {
	bool retval = false;
	if(zIsPressed || stickyZ==touch) {
		if( !(shiftIsPressed ^ stickyNegated) )
			setBoundaryCell(xpos, ypos);
		else
			resetCell(xpos, ypos);
		retval = true;
	}
	else if(xIsPressed || stickyX==touch) {
		if( !(shiftIsPressed ^ stickyNegated) )
			setExcitationCell(xpos, ypos);
		else
			resetCell(xpos, ypos);
		retval = true;
	}
	else if(cIsPressed || stickyC==touch){
		if(!shiftIsPressed)
			moveAreaListener(xpos, ypos);
		else
			hideAreaListener();
		retval = true;
	}
	else if(vIsPressed) {
		setCellArea(xpos, ypos);
		retval = true;
	}
	if(retval)
		changeMousePos(xpos, ypos);
	return retval;
}


void modifierActions2ndFinger(int area, double xpos, double ypos) {
	if(secondFingerMode==m2nd_bound) {
		if(!shiftIsPressed && !modeNegated)
			setBoundaryCell(area, xpos, ypos);
		else
			resetCell(xpos, ypos);
	}
	else if(secondFingerMode==m2nd_ex) {
		if(!shiftIsPressed && !modeNegated)
			setExcitationCell(xpos, ypos);
		else
			resetCell(xpos, ypos);
	}
	else
		moveAreaListener(area, xpos, ypos);
}








void mouseRightPressOrDrag(double xpos, double ypos) {
	resetCell(xpos, ypos);
}

void mouseLeftPress(double xpos, double ypos) {
	if(!modifierActions(xpos, ypos)) {
		//triggerAreaExcitation(xpos, ypos);

		int currentArea = getAreaIndex(xpos, ypos);
		//printf("area index: %d\n", currentArea);
		if(areaExciteType[currentArea] == drumEx_windowedImp)
			triggerAreaExcitation(currentArea);
		else {
			if(areaTouch[currentArea]==-1) {
				triggerAreaExcitation(currentArea);
				areaTouch[currentArea] = -2;
			}
			else if(areaTouch[currentArea]==-2) {
				dampAreaExcitation(currentArea);
				areaTouch[currentArea] = -1;
			}
		}

	}
}

void mouseLeftDrag(double xpos, double ypos) {
	modifierActions(xpos, ypos);
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
			glfwGetCursorPos (window, &xpos, &ypos);
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

void key_callback(GLFWwindow *, int key, int scancode, int action, int mods) {
	if(action == GLFW_PRESS || action == GLFW_REPEAT) {
		// modifiers
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
					zIsPressed = true;
					break;
				case GLFW_KEY_X:
					xIsPressed = true;
					break;
				case GLFW_KEY_C:
					cIsPressed = true;
					break;
				case GLFW_KEY_V:
					vIsPressed = true;
					break;

				case GLFW_KEY_0:
				case GLFW_KEY_KP_0:
					if(leftCtrlIsPressed) {
						if(!shiftIsPressed)
							switchAreaExcitationType(drumEx_windowedImp);
						else
							switchAreaExcitationType(drumEx_drone);
					}
					else  if(rightCtrlIsPressed)
						changeAreaExcitationFixedFreq(0);
					else
						changeAreaIndex(0);
					break;
				case GLFW_KEY_1:
				case GLFW_KEY_KP_1:
				    if(leftCtrlIsPressed) {
				    	if(!shiftIsPressed)
				    		switchAreaExcitationType(drumEx_sin);
				    	else
				    		switchAreaExcitationType(drumEx_pacific);
				    }
					else if(rightCtrlIsPressed)
						changeAreaExcitationFixedFreq(1);
					else
						changeAreaIndex(1);
					break;
				case GLFW_KEY_2:
				case GLFW_KEY_KP_2:
					if(leftCtrlIsPressed)
						switchAreaExcitationType(drumEx_square);
					else if(rightCtrlIsPressed)
						changeAreaExcitationFixedFreq(2);
					else
						changeAreaIndex(2);
					break;
				case GLFW_KEY_3:
				case GLFW_KEY_KP_3:
					if(leftCtrlIsPressed)
						switchAreaExcitationType(drumEx_noise);
					else if(rightCtrlIsPressed)
						changeAreaExcitationFixedFreq(3);
					else
						changeAreaIndex(3);
					break;
				case GLFW_KEY_4:
				case GLFW_KEY_KP_4:
					if(leftCtrlIsPressed)
						switchAreaExcitationType(drumEx_impTrain);
					else if(rightCtrlIsPressed)
						changeAreaExcitationFixedFreq(4);
					else
						changeAreaIndex(4);
					break;
				case GLFW_KEY_5:
				case GLFW_KEY_KP_5:
					if(leftCtrlIsPressed)
						switchAreaExcitationType(drumEx_oscBank);
					else if(rightCtrlIsPressed)
						changeAreaExcitationFixedFreq(5);
					else
						changeAreaIndex(5);
					break;
				case GLFW_KEY_6:
				case GLFW_KEY_KP_6:
					if(leftCtrlIsPressed)
						switchAreaExcitationType(drumEx_burst);
					else if(rightCtrlIsPressed)
						changeAreaExcitationFixedFreq(6);
					else
						changeAreaIndex(6);
					break;
				case GLFW_KEY_7:
				case GLFW_KEY_KP_7:
					if(leftCtrlIsPressed)
						switchAreaExcitationType(drumEx_cello);
					else if(rightCtrlIsPressed)
						changeAreaExcitationFixedFreq(7);
					else
						changeAreaIndex(7);
					break;
				case GLFW_KEY_8:
				case GLFW_KEY_KP_8:
					if(leftCtrlIsPressed)
						switchAreaExcitationType(drumEx_drumloop);
					else
						changeAreaIndex(8);
					break;
				case GLFW_KEY_9:
				case GLFW_KEY_KP_9:
					if(leftCtrlIsPressed)
						switchAreaExcitationType(drumEx_fxloop);
					else
						changeAreaIndex(9);
					break;


				case GLFW_KEY_BACKSPACE:
				case GLFW_KEY_DELETE:
					clearDomain();
					break;

				case GLFW_KEY_ENTER:
					resetSimulation();
					break;

				case GLFW_KEY_SPACE:
					if(leftCtrlIsPressed || rightCtrlIsPressed)
						excite(true);
					else
						slowMotion(true);
					break;

				case GLFW_KEY_TAB:
					switchShowAreas();
					break;

				default:
					break;
			}
		}

		switch(key) {
/*			case GLFW_KEY_A:
				if(!shiftIsPressed)
					shiftAreaListener('h', -deltaPos);
				break;
			case GLFW_KEY_D:
				if(!shiftIsPressed)
					shiftAreaListener('h', deltaPos);
				break;
			case GLFW_KEY_W:
				if(!shiftIsPressed)
					shiftAreaListener('v', deltaPos);
				break;
			case GLFW_KEY_S:
				if(!shiftIsPressed && !leftCtrlIsPressed)
					shiftAreaListener('v', -deltaPos);
				else if(!shiftIsPressed && leftCtrlIsPressed)
					saveFrame();
				break;*/


			case GLFW_KEY_M:
				changeBgain(deltaBgain);
				break;
			case GLFW_KEY_N:
				changeBgain(-deltaBgain);
				break;

			case GLFW_KEY_L:
				changeAreaDampingFactor(deltaDampFactor);
				break;
			case GLFW_KEY_K:
				changeAreaDampingFactor(-deltaDampFactor);
				break;

			case GLFW_KEY_P:
				changeAreaPropagationFactor(deltaPropFactor);
				break;
			case GLFW_KEY_O:
				 if(leftCtrlIsPressed)
					openFrame();
				 else
					 changeAreaPropagationFactor(-deltaPropFactor);
				break;

			case GLFW_KEY_I:
				changeAreaExcitationVolume(deltaVolume);
				break;
			case GLFW_KEY_U:
				changeAreaExcitationVolume(-deltaVolume);
				break;

			case GLFW_KEY_Y:
				changeAreaLowPassFilterFreq(deltaLowPassFreq);
				break;
			case GLFW_KEY_T:
				changeAreaLowPassFilterFreq(-deltaLowPassFreq);
				break;

			case GLFW_KEY_J:
				changeAreaExcitationFreq(deltaFreq);
				break;
			case GLFW_KEY_H:
				changeAreaExcitationFreq(-deltaFreq);
				break;

			default:
				break;
		}
	} // release modifiers
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
			case GLFW_KEY_Z:
				zIsPressed = false;
				break;
			case GLFW_KEY_X:
				xIsPressed = false;
				break;
			case GLFW_KEY_C:
				cIsPressed = false;
				break;
			case GLFW_KEY_V:
				vIsPressed = false;
				break;

				// sticky modifiers
			case GLFW_KEY_A:
				stickyZ = -2; // waiting
				printf("ready for sticky boundaries\n");
				if(shiftIsPressed) {
					stickyNegated = true;
					printf("[negated]\n");
				}
				break;
			case GLFW_KEY_S:
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
				break;

				// second finger interaction mode [for continuous control only]
			case GLFW_KEY_Q:
				secondFingerMode = m2nd_bound;
				printf("Second finger mode set to boundaries\n");
				if(shiftIsPressed) {
					modeNegated = true;
					printf("[negated]\n");
				}
				break;
			case GLFW_KEY_W:
				secondFingerMode = m2nd_ex;
				printf("Second finger mode set to excitation\n");
				if(shiftIsPressed) {
					modeNegated = true;
					printf("[negated]\n");
				}
				break;
			case GLFW_KEY_E:
				secondFingerMode = m2ndList ;
				printf("Second finger mode set to listener\n");
				break;


			case GLFW_KEY_SPACE:
				slowMotion(false);
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
	sigIntHandler.sa_handler = my_handler;
	sigemptyset(&sigIntHandler.sa_mask);
	sigIntHandler.sa_flags = 0;
	sigaction(SIGINT, &sigIntHandler, NULL);
}


void initControlThread() {
	initCallbacks();

	// hide cursor if using wacom pen to control
/*	if(wacomPen) {
		int winSize[2];
		glfwSetInputMode(hyperDrumSynth->getWindow(winSize), GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
	}*/

	//TODO each should have a different one
	deltaLowPassFreq = areaExLowPassFreq[0]/100.0;
	deltaDampFactor  = areaDampFactor[0]/100.0;
	deltaPropFactor  = areaPropagationFactor[0]/100.0;

	for(int i=0; i<AREA_NUM; i++) {
		areaExciteType[i] = drumExType;
		areaTouch[i] = -1;
		touchArea[i] = -1;
	}
}

void *runControlLoop(void *) {
	// first this here
	initControlThread(); // cos it syncs with the audio thread and all its settings

	if(verbose==1)
		cout << "__________set Control Thread priority" << endl;

	// set thread niceness
	 set_niceness(-20, verbose); // even if it should be inherited from calling process...but it's not

	 // set thread priority
	set_priority(1, verbose);

	if(verbose==1)
		cout << "_________________Control Thread!" << endl;

	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS,NULL); // now this thread can be cancelled from main()


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
				case 'c':
					changeExcitationVolume(-deltaVolume);
					break;
				case 'v':
					changeExcitationVolume(deltaVolume);
					break;

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

	// disable mouse control
	disableMouse(touchScreenMouseID);


	struct input_event ev[MAX_NUM_EVENTS]; // where to save event blocks
	char name[256] = "Unknown"; // for the name of the device

	std::string event_device = event_device_path + to_string(touchScreenEvent);

	// open the device!
	touchScreenFileHandler = open(event_device.c_str(), O_RDONLY);
	if (touchScreenFileHandler == -1) {
		fprintf(stderr, "%s is not a vaild device\n", event_device.c_str());
		return (void *)0;
	}

	// print it's name, useful (;
	ioctl(touchScreenFileHandler, EVIOCGNAME(sizeof(name)), name);
	printf("\nReading from:\n");
	printf("device file = %s\n", event_device.c_str());
	printf("device name = %s\n\n", name);


	// working vars
	const size_t ev_size = sizeof(struct input_event)*MAX_NUM_EVENTS;
	ssize_t size;
	bool touchOn[MAX_TOUCHES]  = {false}; // to keep track of new touches and touch removals
	float touchPos[MAX_TOUCHES][2];
	bool newTouch[MAX_TOUCHES] = {false};
	int currentSlot = 0;

	// we need to know how big is the window [resolution], to check where we are touching
	GLFWmonitor* mon;
	if(monitorIndex==-1)
		mon = glfwGetPrimaryMonitor ();
	else {
		int count = -1;
		mon = glfwGetMonitors(&count)[monitorIndex];
	}
	const GLFWvidmode* vmode = glfwGetVideoMode (mon);
	int monitorReso[] = {vmode->width, vmode->height};

	while(true) {
		size = read(touchScreenFileHandler, &ev, ev_size); // we could use select() if we were interested in non-blocking read...i am not, TROLOLOLOLOL

		for (unsigned int i = 0;  i <  (size / sizeof(struct input_event)); i++) {
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
						}
						touchOn[currentSlot] = true;
					}
					else {
						//printf("touch %d is OFF\n", currentSlot);
						touchOn[currentSlot] = false;
						// sticky
						if(stickyZ == currentSlot) {
							stickyZ = -1; // reset
							stickyNegated = false;
							printf("sticky boundaries released\n");
						}
						else if(stickyX == currentSlot) {
							stickyX = -1; // reset
							stickyNegated = false;
							printf("sticky excitation released\n");
						}
						else if(stickyC == currentSlot) {
							stickyC = -1; // reset
							printf("sticky listener released\n");
						}
						// continuous excitation
						else if(touchArea[currentSlot] != -1) {
							int area = touchArea[currentSlot];
							areaTouch[area]        = -1;
							touchArea[currentSlot] = -1;
							dampAreaExcitation(area);
						}
					}
				}
				else if(ev[i].code == ABS_MT_POSITION_X) {
					touchPos[currentSlot][0] = (float(ev[i].value)/MAX_X_POS) * monitorReso[0];
					//printf("touch %d pos X: %f\n", currentSlot, touchPos[currentSlot][0]);

					if(!newTouch[currentSlot]) {
						// if not touching window, fuck off
						if( !( (touchPos[currentSlot][0] >= windowPos[0])&&(touchPos[currentSlot][0] < windowPos[0]+winSize[0]) ) ||
							!( (touchPos[currentSlot][1] >= windowPos[1])&&(touchPos[currentSlot][1] < windowPos[1]+winSize[1]) ) ) {
							continue;
						}

						float posX = touchPos[currentSlot][0]-windowPos[0];
						float posY = (touchPos[currentSlot][1]-windowPos[1]);

						//printf("touch pos: %f %f\n", posX, posY);

						if(!modifierActions(currentSlot, posX, posY)) { // we perform a double update [both on X and Y events], cos X and Y events do not always happen together [sometime only X, sometimes only Y]
							int currentArea = getAreaIndex(posX, posY);
							if(areaTouch[currentArea]!=-1 && areaTouch[currentArea]!=currentSlot)
								modifierActions2ndFinger(currentArea, posX, posY);
						}
					}
				}
				else if(ev[i].code == ABS_MT_POSITION_Y) {
					touchPos[currentSlot][1] = (float(ev[i].value)/MAX_Y_POS) * monitorReso[1];
					//printf("touch %d pos Y: %f\n", currentSlot, touchPos[currentSlot][1]);

					//printf("win pos: %d %d\n", windowPos[0], windowPos[1]);
					//printf("win size: %d %d\n", winSize[0], winSize[1]);

					// if not touching window, fuck off
					if( !( (touchPos[currentSlot][0] >= windowPos[0])&&(touchPos[currentSlot][0] < windowPos[0]+winSize[0]) ) ||
						!( (touchPos[currentSlot][1] >= windowPos[1])&&(touchPos[currentSlot][1] < windowPos[1]+winSize[1]) ) ) {
						newTouch[currentSlot] = false;
						continue;
					}

					// translate into some kind of mouse pos, relative to window
					float posX = touchPos[currentSlot][0]-windowPos[0];
					float posY = touchPos[currentSlot][1]-windowPos[1];

					//printf("touch pos: %f %f\n", posX, posY);

					if(!modifierActions(currentSlot, posX, posY)) {
						int currentArea = getAreaIndex(posX, posY);
						if(newTouch[currentSlot]) {
							// stickies to be assigned
							if(stickyZ == -2) {
								stickyZ = currentSlot;
								printf("sticky boundaries taken!\n");
							}
							else if(stickyX == -2) {
								stickyX = currentSlot;
								printf("sticky excitation taken!\n");
							}
							else if(stickyC == -2) {
								stickyC = currentSlot;
								printf("sticky listener taken!\n");
							}
							// regular excitation
							else if(areaExciteType[currentArea] == drumEx_windowedImp)
								triggerAreaExcitation(currentArea);
							else {
								// continuous excitation
								if(areaTouch[currentArea]==-1) {
									triggerAreaExcitation(currentArea);
									areaTouch[currentArea] = currentSlot;
									touchArea[currentSlot] = currentArea;
								}
								else  // new touch, not impulse, second finger [or more]
									modifierActions2ndFinger(currentArea, posX, posY);
							}
						}
						else if(areaTouch[currentArea]!=-1 && areaTouch[currentArea]!=currentSlot)
							modifierActions2ndFinger(currentArea, posX, posY); // not new touch, not impulse, second finger [or more]

						changeMousePos(-1, -1); // always
					}

					newTouch[currentSlot] = false;
				}
			}
		}
		usleep(1);
	}




	return (void *)0;
}


void cleanUpControlLoop() {
	// re-enable mouse control
	enableMouse(touchScreenMouseID);

	close(touchScreenFileHandler);
	printf("Multitouch device nicely shut (:\n");
}


