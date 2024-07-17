/*
 * controls.cpp
 *
 *  Created on: 2015-10-30
 *      Author: Victor Zappi
 */

//-------------------------------------------
// mouse mapping
//-------------------------------------------
// left click: move listener
// right click: move feedback
// left click + left control: write wall cells
// left click + shift: write excitation cells
// left click + left control OR shift: reset cells [to air or pml]
/* central click: acts as left control
   not yet lock x or y [straight lines]
 */


//-------------------------------------------
// keyboard mapping
//-------------------------------------------
// left control + num: excitation input
// right control + num: excitation model
// c, v: decrease, increase excitation input pressure [volume]
// z, x: decrease, increase simulation max pressure
// n, m: decrease, increase excitation input frequency
// num: excitation input discreet frequencies
// shift + num: excitation direction [with 5=all directions and zero is not mapped]
// j, k: decrease, increase excitation input low pass
// b, h: decrease, increase q in 2 mass models
//
// l: toggle boundary losses
// l + left control: toggle excitation model low pass
//
// o, p: decrease, increase simulation ds [super sick]
// backspace OR delete: wipe out all domain pixels
// return: reset simulation
// space bar: slow motion [sick]
//
// left control + s: save current frame
// left control + shift + s: save current frame sequence
// left control + o: open frame [file name is hardcoded for now]
// left control + shift + o: open frame sequence [file name is hardcoded for now]
// left control + shift + c: compute frame sequence [file name is hardcoded for now]
//
// left control + h: hide/show graphical UI
//
// a, d, s, v: move listener
// a, d, s, v + shift: move feedback
//-------------------------------------------


//-------------------------------------------
// wacom tablet+pen mapping
//-------------------------------------------
// touch tablet: move listener
// touch tablet + shift: move feedback
// touch tablet + lower button: write wall cells
// touch tablet + lower button + shift: write excitation cells
// touch tablet + upper button: reset cells [to air or pml]
// touch tablet + lower button + left control: write wall cells with lock x [straight horizontal line]
// touch tablet + lower button + alt: write wall cells with lock y [straight vertical line]
// touch tablet + lower button + shift + left control: write excitation cells with lock x [straight horizontal line]
// touch tablet + lower button + shift + alt: write excitation cells with lock y [straight vertical line]
/* left click = touch tablet with pen
   right click = upper pen button
   central click = lower pen button
*/
//-------------------------------------------



#include "controls.h"

//back end
#include "AudioEngine.h"
#include "priority_utils.h"
#include "ImplicitFDTD.h"

// applications stuff
#include "FlatlandSynth.h"
#include "FlatlandExcitation.h"

extern int verbose;

//----------------------------------------
// main variables
//----------------------------------------
// extern
extern AudioEngine audioEngine;
extern FlatlandSynth *flatlandSynth;
//----------------------------------------

//----------------------------------------
// control variables
//---------------------------------------
extern bool wacomPen;
extern int listenerCoord[2];
extern int feedbCoord[2];
extern float maxPressure;
extern float excitationVol;
//extern float excitationAngle;
extern float ds;
extern bool filterToggle;
extern bool glottalLowPassToggle;

int deltaPos           = 1;
float deltaPressure    = 0.05;
float deltaMaxPressure = 10;
float deltaDs          = 0.001;

float freq      = 440.0;
float deltaFreq = freq/100.0;
float oscFrequencies[FREQS_NUM] = {50, 100, 200, 400, 800, 1600, 3200, 6400};

float q      = 1;
float deltaQ = 0.05;

bool UIisHidden = true;

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
bool spaceBarIsPressed  = false;

int xLock = -100;
int yLock = -100;

bool uIinteraction = false;
float startingDs   = -1;

frame instrFrames[UI_AREA_BUTTONS_NUM*2];
string instrFrameNames[UI_AREA_BUTTONS_NUM*2];

int state=0;
//----------------------------------------

inline void updateVolume() {
	flatlandSynth->setExcitationVolume(excitationVol);
	printf("Excitation volume: %.2f\t\tPressure: %.2f [Pa]  (Max pressure: %.1f [Pa])\n", excitationVol, excitationVol*maxPressure, maxPressure);
}

inline void updateFrequency() {
	flatlandSynth->setExcitationFreq(freq);
	deltaFreq = freq/100.0; // keep ratio constant
	printf("Excitation freq: %f [Hz]\n", freq);
}

inline bool updateQ(){
	if(flatlandSynth->setQ(q) != 0)
		return false;
	printf("q parameter of 2 Mass Model set to: %f\n", q);
	return true;
}

inline void updateDs() {
	ds = flatlandSynth->setDS(ds);
	printf("Current ds: %f [m]\n", ds);
}

void getCellDomainCoords(int *coords) {
	// y is uupside down
	coords[1] = winSize[1] - coords[1] - 1;
	//printf("Mouse coord: %d %d\n", coords[0], coords[1]);
	// we get window coordinates, but this filter works with domain ones
	flatlandSynth->fromRawWindowToDomain(coords);
	//printf("Domain cell coord: %d %d\n", coords[0], coords[1]);
}

void getCellGlWindowCoords(float *coords) {
	// y is uupside down
	coords[1] = winSize[1] - coords[1] - 1;
	//printf("Mouse coord before: %d %d\n", coords[0], coords[1]);
	// we get window coordinates, but this filter works with domain ones
	flatlandSynth->fromRawWindowToGlWindow(coords);
	coords[1] += 1; //VIC otherwise there is a shift in UI
	//printf("GL Window coord: %d %d\n", coords[0], coords[1]);
}

void changeMousePos(int mouseX, int mouseY) {
	int coord[2] = {(int)mouseX, (int)mouseY};
	getCellDomainCoords(coord);
	flatlandSynth->setMousePos(coord[0], coord[1]);
}

void changeQ(float delta) {
	float oldQ = q;
	q += delta;
	if(!updateQ())
		q = oldQ;
}

void hideUI() {
	UIisHidden = !UIisHidden;
	flatlandSynth->setHideUI(UIisHidden);
}
void slowMotion(bool slowMo) {
	flatlandSynth->setSlowMotion(slowMo);
}

void setWallCell(float mouseX, float mouseY) {
	int coord[2] = {(int)mouseX, (int)mouseY};
	getCellDomainCoords(coord);

	// to prevent modifications within the spaces between UI components
	if(coord[0]<=21)
		return;

	flatlandSynth->setCell(ImplicitFDTD::cell_wall, coord[0], coord[1]);
}

void setExcitationCell(float mouseX, float mouseY) {
	int coord[2] = {(int)mouseX, (int)mouseY};
	getCellDomainCoords(coord);

	// to prevent modifications within the spaces between UI components
	if(coord[0]<=21)
		return;

	flatlandSynth->setCell(ImplicitFDTD::cell_excitation, coord[0], coord[1]);
}

void resetWallCell(float mouseX, float mouseY) {
	int coord[2] = {(int)mouseX, (int)mouseY};
	getCellDomainCoords(coord);

	// to prevent modifications within the spaces between UI components
	if(coord[0]<=21)
		return;

	flatlandSynth->resetCell(coord[0], coord[1]);
}

void clearDomain() {
	flatlandSynth->clearDomain();
}

void resetSimulation() {
	flatlandSynth->resetSimulation();
}

// from GLFW window coordinates
void moveListener(float mouseX, float mouseY) {
	int coord[2] = {(int)mouseX, (int)mouseY};
	getCellDomainCoords(coord);

	// to prevent listener to sneak within the spaces between UI components
	if(coord[0]<=21)
		return;

	// finally move listener
	if(flatlandSynth->setListenerPosition(coord[0], coord[1]) == 0) {
		flatlandSynth->getListenerPosition(listenerCoord[0], listenerCoord[1]);
		//printf("Listener position (in pixels): (%d, %d)\n", listenerCoord[0], listenerCoord[1]); //get pos
	}

}

void shiftListener(char dir, int delta) {
	if(dir == 'h') {
		listenerCoord[0] = flatlandSynth->shiftListenerH(delta);
		printf("Listener position (in pixels): (%d, %d)\n", listenerCoord[0], listenerCoord[1]);
	}else if(dir == 'v') {
		listenerCoord[1] = flatlandSynth->shiftListenerV(delta);
		printf("Listener position (in pixels): (%d, %d)\n", listenerCoord[0], listenerCoord[1]);
	}
}

// from GLFW window coordinates
void moveFeedback(float mouseX, float mouseY) {
	int coord[2] = {(int)mouseX, (int)mouseY};
	getCellDomainCoords(coord);

	// to prevent feedback to sneak within the spaces between UI components
	if(coord[0]<=21)
		return;

	// finally move listener
	if(flatlandSynth->setFeedbackPosition(coord[0], coord[1])==0) {
		flatlandSynth->getFeedbackPosition(feedbCoord[0], feedbCoord[1]);
		//printf("Feedback position (in pixels): (%d, %d)\n", coord[0], coord[1]);
	}
}

void shiftFeedback(char dir, int delta) {
	if(dir == 'h') {
		feedbCoord[0] = flatlandSynth->shiftFeedbackH(delta);
		printf("Feedback position (in pixels): (%d, %d)\n", feedbCoord[0], feedbCoord[1]);
	}else if(dir == 'v') {
		feedbCoord[1] = flatlandSynth->shiftFeedbackV(delta);
		printf("Feedback position (in pixels): (%d, %d)\n", feedbCoord[0], feedbCoord[1]);
	}
}

void setPressure(float pressure) {
	excitationVol = pressure/maxPressure;
	updateVolume();
}

void changePressure(float delta) {
	excitationVol += delta;
	updateVolume();
}

void changeMaxPressure(float delta) {
	maxPressure += delta;
	flatlandSynth->setMaxPressure(maxPressure);
	printf("New Max Pressure: %f\n", maxPressure);
	printf("\tExcitation volume: %.2f\t\tPressure: %.2f [Pa]  (Max pressure: %.1f [Pa])\n", excitationVol, excitationVol*maxPressure, maxPressure);
}

void changeDs(float delta) {
	ds += delta;
	updateDs();
}

void changeExcitationDir(float angle) {
	flatlandSynth->setExcitationDirection(angle);
	printf("Excitation direction: %f degrees\n", angle);
}

void switchExcitationModel(excitation_model index) {
	flatlandSynth->setExcitationModel(index);
	string exModelName = flatlandSynth->getExcitationModelName();

	printf("Excitation model: %s\n", exModelName.c_str());
}

void changeExcitationFreq(float deltaF) {
	float nextF = freq+deltaF;
	if(nextF >=0) {
		freq = nextF;
		updateFrequency();
	}
}

void toggleBoundaryLosses() {
	if(filterToggle) {
		filterToggle = false;
		printf("Boundary filter set to OFF\n");
	}
	else if(!filterToggle) {
		filterToggle = true;
		printf("Boundary filter set to ON\n");
	}
	flatlandSynth->setFilterToggle(filterToggle);
}

void toggleExcitationModelLowPass() {
	if(glottalLowPassToggle) {
		glottalLowPassToggle = false;
		printf("Glottal low pass set to OFF\n");
	}
	else if(!glottalLowPassToggle) {
		glottalLowPassToggle = true;
		printf("Glottal low pass set to ON\n");
	}
	flatlandSynth->setGlottalLowPassToggle(glottalLowPassToggle);
}

void saveFrame() {
	int retval=flatlandSynth->saveFrame();
	if(retval==0)
		printf("Current frame saved\n");
	else if(retval==-1)
		printf("Init Vocal Synthesizer before trying to save frame file!\n");
}

void saveFrameSequence() {
	int retval=flatlandSynth->saveFrameSequence();
	if(retval==0)
		printf("Current frame sequence saved \n");
	else if(retval==-1)
		printf("Init Vocal Synthesizer before trying to save frame sequence file!\n");
}

void computeFrameSequence() {
	string foldername = "data/seq_0_120x60";
	int retval = flatlandSynth->computeFrameSequence(foldername);
	if(retval==0)
		printf("Computed frame sequence from folder \"%s\"\n", foldername.c_str());
	else if(retval==-1)
		printf("Init Vocal Synthesizer before trying to compute frame sequence!\n");
}

void openFrame() {
	string filename = "frames/flute/flute0_note0.frm";
	int retval = flatlandSynth->openFrame(filename);
	if(retval==0)
		printf("Loaded frame from file \"%s\"\n", filename.c_str());
	else if(retval==-1)
		printf("Init Vocal Synthesizer before trying to open frame file!\n");
}

void openFrameSequence() {
	string filename = "data/seq_0_120x60.fsq";
	int retval = flatlandSynth->openFrameSequence(filename);
	if(retval==0)
		printf("Loaded frame from file \"%s\"\n", filename.c_str());
	else if(retval==-1)
		printf("Init Vocal Synthesizer before trying to open frame file!\n");
}

void wacomLock(double &xpos, double &ypos) {
	if(!spaceBarIsPressed)
		xLock = -100;
	else if(xLock == -100)
		xLock = xpos;

	if(!leftCtrlIsPressed)
		yLock = -100;
	else if(yLock == -100)
		yLock = ypos;

	if(xLock != -100)
		xpos = xLock;
	if(yLock != -100)
		ypos = yLock;
}

inline void wacomUnlock() {
	xLock = -100;
	yLock = -100;
}

void wacomLeftPressOrDrag(double xpos, double ypos) {
	if(mouseCenterPressed) {
		wacomLock(xpos, ypos);
		if(!shiftIsPressed)
			setWallCell(xpos, ypos);
		else
			setExcitationCell(xpos, ypos);
	}
	else if(mouseRightPressed) {
		wacomLock(xpos, ypos);
		resetWallCell(xpos, ypos);
	}
	else if(shiftIsPressed)
		moveFeedback(xpos, ypos);
	else
		moveListener(xpos, ypos);
}

void mouseLeftPressOrDrag(double xpos, double ypos) {
	if(shiftIsPressed)
		setExcitationCell(xpos, ypos);
	else if(leftCtrlIsPressed)
		setWallCell(xpos, ypos);
	else
		moveListener(xpos, ypos);
}

void mouseRightPressOrDrag(double xpos, double ypos) {
	if(!shiftIsPressed && !leftCtrlIsPressed)
		moveFeedback(xpos, ypos);
	else
		resetWallCell(xpos, ypos);
}

inline void sliderVolumeMapping(float value) {
	excitationVol = value*2;
	updateVolume();
}

const float freq_exp_max = (exp(9)-1);
inline void sliderFreqMapping(float value) {
	freq = 15000 * (exp(value*9)-1)/freq_exp_max;
	updateFrequency();
}

const float q_exp_max = (exp(2)-1);
inline void sliderQMapping(float value) {
	q = 4 * (exp(value*2)-1)/q_exp_max;
	updateQ();
}

const float q_exp_ds = (exp(3)-1);
inline void sliderDsMapping(float value) {
	value = (value<0.01) ? 0.00001 : value;
	ds = (exp(value*3)-1)/q_exp_ds + startingDs;
	updateDs();
}

inline void buttonsMapping(int index) {
	// 2 buttons are not used, for safety!
/*	if( (index!=4) && (index!=5) )
		flatlandSynth->loadFrame(&instrFrames[index]);*/
	if(leftCtrlIsPressed || rightCtrlIsPressed) {
		if(index==0)
			switchExcitationModel(exMod_noFeedback);
		else if(index==1)
			switchExcitationModel(exMod_clarinetReed);
		else if(index==2)
			switchExcitationModel(exMod_2massKees);
	}
	else if(shiftIsPressed) {
		if(index<3)
			flatlandSynth->loadFrame(&instrFrames[index]);
		else
			flatlandSynth->loadFrame(&instrFrames[index]+UI_AREA_BUTTONS_NUM);
	}
	else if(altIsPressed) {
		if(index==0) {
			// flute
			flatlandSynth->setExcitationVol(0, 1); // const on
			flatlandSynth->setExcitationVol(1, 0); // sin off
			flatlandSynth->setExcitationVol(2, 0); // square off
			flatlandSynth->setExcitationVol(3, 1); // noise on
			flatlandSynth->setExcitationVol(4, 0); // passthru off
		}
		else if(index==1) {
			// dijeridoo
			flatlandSynth->setExcitationVol(0, 1); // const on
			flatlandSynth->setExcitationVol(1, 1); // sin on
			flatlandSynth->setExcitationVol(2, 0); // square off
			flatlandSynth->setExcitationVol(3, 0); // noise on
			flatlandSynth->setExcitationVol(4, 0); // passthru off
		}
	}
	else {
		for(int i=0; i<5; i++) {
			flatlandSynth->setExcitationVol(i, index==i);
		}
		if(index>3) {
			flatlandSynth->setPassThruTrack(index-4);
			flatlandSynth->setExcitationVol(4, 1);
		}
	}
}

void slidersMapping(int index, float value, bool reset=false) {
	if(!reset) {
		switch(index) {
			case 0:
			case 4:
				sliderDsMapping(value);
				break;
			case 1:
			case 5:
				sliderQMapping(value);
				break;
			case 2:
			case 6:
				sliderFreqMapping(value);
				break;
			case 3:
			case 7:
				sliderVolumeMapping(value);
				break;
			default:
				break;
		}
	}
	else {
		switch(index) {
			case 0:
			case 4:
				sliderDsMapping(value);
				flatlandSynth->resetAreaSlider(0); // coupled with sticky 4
				break;
			case 1:
			case 5:
				sliderQMapping(value);
				flatlandSynth->resetAreaSlider(1); // coupled with sticky 5
				break;
			case 2:
			case 6:
				sliderFreqMapping(value);
				flatlandSynth->resetAreaSlider(2); // coupled with sticky 6
				break;
			case 3:
			case 7:
				sliderVolumeMapping(value);
				flatlandSynth->resetAreaSlider(3); // coupled with sticky 7
				break;
			default:
				break;
		}
	}
}

bool startUIinteraction(float xpos, float ypos) {
	if(UIisHidden)
		return false;

	float coords[2] = {xpos, ypos};
	getCellGlWindowCoords(coords);
	bool newTrigger = false;
	int triggered = flatlandSynth->checkAreaButtonsTriggered(coords[0], coords[1], newTrigger);
	if(triggered>-1) {
		buttonsMapping(triggered);
		return true;
	}

	float newVal = -1;
	int modified = flatlandSynth->checkAreaSlidersValues(coords[0], coords[1], newVal);
	if(modified>-1) {
		slidersMapping(modified, newVal);
		return true;
	}

	return false;
}

void dismissUIinteraction() {
	if(UIisHidden)
		return;

	flatlandSynth->untriggerAreaButtons();
	bool *resetlist = flatlandSynth->resetAreaSliders(true);
	for(int i=0; i<UI_AREA_SLIDERS_NUM; i++) {
		if(resetlist[i])
			slidersMapping(i, flatlandSynth->getAreaSliderVal(i), true);
	}
}

// mouse buttons actions
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	double xpos;
	double ypos;

	// left mouse down
    if(button == GLFW_MOUSE_BUTTON_LEFT) {
    	if(action == GLFW_PRESS) {
			glfwGetCursorPos (window, &xpos, &ypos);
			if(startUIinteraction(xpos, ypos)) {
				uIinteraction = true;
			}
			if(!uIinteraction) {
				if(!wacomPen)
					mouseLeftPressOrDrag(xpos, ypos);
				else
					wacomLeftPressOrDrag(xpos, ypos);
			}
			mouseLeftPressed = true;  // start dragging
			//printf("left mouse pressed\n");
    	} else if(action == GLFW_RELEASE) {
    		if(wacomPen)
    			wacomUnlock();
    		dismissUIinteraction();
    		mouseLeftPressed = false; // stop dragging
    		//printf("left mouse released\n");
    	}
    }
	// right mouse down
    else  if(button == GLFW_MOUSE_BUTTON_RIGHT) {
    	if(action == GLFW_PRESS) {
			glfwGetCursorPos (window, &xpos, &ypos);
			if(!wacomPen)
				mouseRightPressOrDrag(xpos, ypos);
			mouseRightPressed = true;  // start dragging
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
    		if(wacomPen)
    			wacomUnlock();
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
	if(mouseLeftPressed) {
		if(startUIinteraction(xpos, ypos)) {
			uIinteraction = true;
			return;
		}
		else
			uIinteraction = true;
		if(!wacomPen)
			mouseLeftPressOrDrag(xpos, ypos);
		else
			wacomLeftPressOrDrag(xpos, ypos);
	}
	// right mouse dragged
	else if(mouseRightPressed) {
		if(!wacomPen)
			mouseRightPressOrDrag(xpos, ypos);
	}
}

void cursor_enter_callback(GLFWwindow* window, int entered) {
    if (!entered){
        // The cursor exited the client area of the window
    	mousePos[0] = -1;
    	mousePos[1] = -1;
    	changeMousePos(mousePos[0], mousePos[1]);
    }
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

				case GLFW_KEY_0:
				case GLFW_KEY_KP_0:
					switchExcitationModel(exMod_noFeedback);
					break;
				case GLFW_KEY_1:
				case GLFW_KEY_KP_1:
					switchExcitationModel(exMod_clarinetReed);
					break;
				case GLFW_KEY_2:
				case GLFW_KEY_KP_2:
					if(leftCtrlIsPressed)
						switchExcitationModel(exMod_2massKees);
					break;
				case GLFW_KEY_3:
				case GLFW_KEY_KP_3:
					break;
				case GLFW_KEY_4:
				case GLFW_KEY_KP_4:
					break;
				case GLFW_KEY_5:
				case GLFW_KEY_KP_5:
					break;
				case GLFW_KEY_6:
				case GLFW_KEY_KP_6:
					break;
				case GLFW_KEY_7:
				case GLFW_KEY_KP_7:
				case GLFW_KEY_8:
				case GLFW_KEY_KP_8:
				case GLFW_KEY_9:
				case GLFW_KEY_KP_9:
					break;

				case GLFW_KEY_L:
					toggleBoundaryLosses();
					break;

				case GLFW_KEY_DELETE:
					clearDomain();
					break;

				case GLFW_KEY_ENTER:
					resetSimulation();
					break;

				case GLFW_KEY_SPACE:
					slowMotion(true);//spaceBarIsPressed = true;
					break;

				default:
					break;
			}
		}

		switch(key) {
			case GLFW_KEY_A:
				if(!shiftIsPressed)
					shiftListener('h', -deltaPos);
				else
					shiftFeedback('h', -deltaPos);
				break;
			case GLFW_KEY_D:
				if(!shiftIsPressed)
					shiftListener('h', deltaPos);
				else
					shiftFeedback('h', deltaPos);
				break;
			case GLFW_KEY_W:
				if(!shiftIsPressed)
					shiftListener('v', deltaPos);
				else
					shiftFeedback('v', deltaPos);
				break;
			case GLFW_KEY_S:
				if(!shiftIsPressed && !leftCtrlIsPressed)
					shiftListener('v', -deltaPos);
				else if(shiftIsPressed && !leftCtrlIsPressed)
					shiftFeedback('v', -deltaPos);
				else if(!shiftIsPressed && leftCtrlIsPressed)
					saveFrame();
				else if(shiftIsPressed && leftCtrlIsPressed)
					saveFrameSequence();
				break;

			case GLFW_KEY_Z:
				changeMaxPressure(-deltaMaxPressure);
				break;
			case GLFW_KEY_X:
				changeMaxPressure(deltaMaxPressure);
				break;

			case GLFW_KEY_C:
				if(shiftIsPressed && leftCtrlIsPressed)
					computeFrameSequence();
				else
					changePressure(-deltaPressure);
				break;
			case GLFW_KEY_V:
				changePressure(deltaPressure);
				break;

			case GLFW_KEY_N:
				changeExcitationFreq(-deltaFreq);
				break;
			case GLFW_KEY_M:
				changeExcitationFreq(deltaFreq);
				break;

			case GLFW_KEY_O:
				/*if(leftCtrlIsPressed && shiftIsPressed)
					openFrameSequence();
				else */if(leftCtrlIsPressed)
					flatlandSynth->loadFrame(&instrFrames[4]);//openFrame();
				else
					changeDs(-deltaDs);
				break;
			case GLFW_KEY_P:
				changeDs(deltaDs);
				break;

			case GLFW_KEY_B:
				changeQ(-deltaQ);
				break;
			case GLFW_KEY_H:
				if(leftCtrlIsPressed)
					hideUI();
				else
					changeQ(deltaQ);
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

			case GLFW_KEY_SPACE:
				slowMotion(false);//spaceBarIsPressed = false;
				break;

			default:
				break;
		}
	}
}

// to handle window interaction
void initCallbacks() {
	// grab window handle
	window = flatlandSynth->getWindow(winSize); // try, but we all know it won't work |:
	while(window == NULL) {
		usleep(1000); // wait
		window = flatlandSynth->getWindow(winSize); // try again...patiently
	}

	// callbacks
	glfwSetMouseButtonCallback(window, mouse_button_callback); // mouse buttons down
	glfwSetCursorPosCallback(window, cursor_position_callback);
	glfwSetCursorEnterCallback(window, cursor_enter_callback);
	glfwSetKeyCallback(window, key_callback);
}


void initControlThread() {
	initCallbacks();
	// hide cursor if using wacom pen to control
	if(wacomPen) {
		int winSize[2];
		glfwSetInputMode(flatlandSynth->getWindow(winSize), GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
	}

	startingDs = ds;

	// init frames for buttons

	//VIC these should be flutes
/*	instrFrameNames[0]  = "frames/flute/flute_0.frm";
	instrFrameNames[1]  = "frames/flute/flute_1.frm";
	instrFrameNames[2]  = "frames/flute/flute_2.frm";
	instrFrameNames[3]  = "frames/flute/flute_3.frm";
	instrFrameNames[4]  = "frames/flute/flute_init.frm";
	instrFrameNames[5]  = "frames/flute/flute_0.frm"; // not used

	instrFrameNames[6]  = "frames/didge/didge_0.frm";
	instrFrameNames[7]  = "frames/didge/didge_1.frm";
	instrFrameNames[8]  = "frames/didge/didge_2.frm";
	instrFrameNames[9]  = "frames/didge/didge_3.frm";
	instrFrameNames[10] = "frames/didge/didge_4.frm";
	instrFrameNames[11] = "frames/didge/didge_5.frm";*/


	instrFrameNames[0]  = "frames/flute/flute_1.frm";
	instrFrameNames[1]  = "frames/flute/flute_2.frm";
	instrFrameNames[2]  = "frames/flute/flute_3.frm";
	instrFrameNames[3]  = "frames/flute/flute_3.frm";
	instrFrameNames[4]  = "frames/flute/flute_3.frm";
	instrFrameNames[5]  = "frames/flute/flute_3.frm";

	instrFrameNames[6]  = "frames/didge/didge_0.frm";
	instrFrameNames[7]  = "frames/didge/didge_1.frm";
	instrFrameNames[8]  = "frames/didge/didge_2.frm";
	instrFrameNames[9]  = "frames/didge/didge_1.frm";
	instrFrameNames[10] = "frames/didge/didge_0.frm";
	instrFrameNames[11] = "frames/didge/didge_5.frm";

	for(int i=0; i<UI_AREA_BUTTONS_NUM*2; i++)
		flatlandSynth->computeFrame(instrFrameNames[i], instrFrames[i]);

	flatlandSynth->setExcitationVol(1, 1);
	sliderVolumeMapping(0);
}

void cleanUp() {
	for(int i=0; i<UI_AREA_BUTTONS_NUM*2; i++)
		delete instrFrames[i].pixels;
}

void *runControlLoop(void *) {
	if(verbose==1)
		cout << "__________set Control Thread priority" << endl;
	// set thread priority
	set_priority(1, verbose);

	if(verbose==1)
		cout << "_________________Control Thread!" << endl;

	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS,NULL); // now this thread can be cancelled from main()

	initControlThread();

	char keyStroke = '.';
	cout << "Press q to quit." << endl;

	do {
		keyStroke =	getchar();

		while(getchar()!='\n'); // to read the first stroke

		switch (keyStroke)
		{
			/*case 'a':
				SimSynth->gate(0);
				SimSynth->gate(1);
				cout << "attack" << endl;
				break;
			case 's':
				SimSynth->gate(0);
				cout << "release" << endl;
				break;
				break;*/
		case 'a':
			shiftListener('h', -deltaPos);
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
			changeMaxPressure(-deltaPressure);
			break;
		case 'x':
			changeMaxPressure(deltaPressure);
			break;

		case 'c':
			changePressure(-deltaPressure);
			break;
		case 'v':
			changePressure(deltaPressure);
			break;

			default:
				break;
		}
	}
	while (keyStroke != 'q');

	cleanUp();

	audioEngine.stopEngine();
	cout << "control thread ended" << endl;
	return (void *)0;
}

