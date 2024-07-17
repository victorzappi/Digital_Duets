/*
 * controls.cpp
 *
 *  Created on: 2016-11-18
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

// applications stuff
#include "FDTDSynth.h"
#include "FDTDExcitation.h"

extern int verbose;

//----------------------------------------
// main variables
//----------------------------------------
// extern
extern AudioEngine audioEngine;
extern FDTDSynth *fdtdSynth;
//----------------------------------------

//----------------------------------------
// control variables
//---------------------------------------
extern bool wacomPen;
extern int listenerCoord[2];
extern float maxPressure;
extern float excitationVol;



int deltaPos           = 1;
float deltaPressure    = 0.05;
float deltaMaxPressure = 10;
float deltaDs          = 0.001;

float freq      = 440.0;
float deltaFreq = freq/100.0;
float oscFrequencies[FREQS_NUM] = {50, 100, 200, 400, 800, 1600, 3200, 6400};

float allWallBetas      = 0;
float deltaAllWallBetas = 0.05;

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

int xLock = -100;
int yLock = -100;
//----------------------------------------

void getCellDomainCoords(int *coords) {
	// y is uupside down
	coords[1] = winSize[1]-1 - coords[1] ;

	// we get window coordinates, but this filter works with domain ones
	fdtdSynth->fromWindowToDomain(coords);
}

void changeMousePos(int mouseX, int mouseY) {
	int coord[2] = {(int)mouseX, (int)mouseY};
	getCellDomainCoords(coord);

	fdtdSynth->setMousePos(coord[0], coord[1]);
}

void slowMotion(bool slowMo) {
	fdtdSynth->setSlowMotion(slowMo);
}

void setWallCell(float mouseX, float mouseY) {
	int coord[2] = {(int)mouseX, (int)mouseY};
	getCellDomainCoords(coord);

	fdtdSynth->setCell(FDTD::cell_wall, coord[0], coord[1]);
}

void setExcitationCell(float mouseX, float mouseY) {
	int coord[2] = {(int)mouseX, (int)mouseY};
	getCellDomainCoords(coord);

	fdtdSynth->setCell(FDTD::cell_excitation, coord[0], coord[1]);
}

void resetWallCell(float mouseX, float mouseY) {
	int coord[2] = {(int)mouseX, (int)mouseY};
	getCellDomainCoords(coord);

	fdtdSynth->resetCell(coord[0], coord[1]);
}

void clearDomain() {
	fdtdSynth->clearDomain();
}

void resetSimulation() {
	fdtdSynth->resetSimulation();
}

// from GLFW window coordinates
void moveListener(float mouseX, float mouseY) {
	int coord[2] = {(int)mouseX, (int)mouseY};
	getCellDomainCoords(coord);

	// finally move listener
	if(fdtdSynth->setListenerPosition(coord[0], coord[1]) == 0) {
		fdtdSynth->getListenerPosition(listenerCoord[0], listenerCoord[1]);
		printf("Listener position (in pixels): (%d, %d)\n", listenerCoord[0], listenerCoord[1]); //get pos
	}
}

void shiftListener(char dir, int delta) {
	if(dir == 'h') {
		listenerCoord[0] = fdtdSynth->shiftListenerH(delta);
		printf("Listener position (in pixels): (%d, %d)\n", listenerCoord[0], listenerCoord[1]);
	}else if(dir == 'v') {
		listenerCoord[1] = fdtdSynth->shiftListenerV(delta);
		printf("Listener position (in pixels): (%d, %d)\n", listenerCoord[0], listenerCoord[1]);
	}
}

void setPressure(float pressure) {
	excitationVol = pressure/maxPressure;
	fdtdSynth->setExcitationVolume(excitationVol);
	printf("Excitation volume: %.2f\t\tPressure: %.2f [Pa]  (Max pressure: %.1f [Pa])\n", excitationVol, excitationVol*maxPressure, maxPressure);
}

void changePressure(float delta) {
	excitationVol += delta;
	fdtdSynth->setExcitationVolume(excitationVol);
	printf("Excitation volume: %.2f\t\tPressure: %.2f [Pa]  (Max pressure: %.1f [Pa])\n", excitationVol, excitationVol*maxPressure, maxPressure);
}

void changeMaxPressure(float delta) {
	maxPressure += delta;
	fdtdSynth->setMaxPressure(maxPressure);
	printf("New Max Pressure: %f\n", maxPressure);
	printf("\tExcitation volume: %.2f\t\tPressure: %.2f [Pa]  (Max pressure: %.1f [Pa])\n", excitationVol, excitationVol*maxPressure, maxPressure);
}

void switchExcitationType(ex_type subGlotEx) {
	fdtdSynth->setExcitationType(subGlotEx);
	printf("Current excitation type: %s\n", FDTDExcitation::excitationNames[subGlotEx].c_str());
}

void changeExcitationFreq(float deltaF) {
	float nextF = freq+deltaF;
	if(nextF >=0) {
		fdtdSynth->setExcitationFreq(nextF);
		freq = nextF;
		deltaFreq = nextF/100.0; // keep ratio constant
		printf("Excitation freq: %f [Hz]\n", freq);
	}
}

void changeExcitationFixedFreq(int id) {
	fdtdSynth->setExcitationFixedFreq(id);
	freq = oscFrequencies[id];
	deltaFreq = freq/100.0; // keep ratio constant
	printf("Excitation freq: %f [Hz]\n", freq);
}

void changeAllWallBetas(float deltaB) {
	float nextB = allWallBetas+deltaB;
	if(nextB<0)
		nextB = 0;
	else if(nextB>1)
		nextB = 1;

	fdtdSynth->setAllWallBetas(nextB);
	allWallBetas = nextB;
	printf("Set betas (\"airness\") for all walls to: %f\n", allWallBetas);
}

void wacomLock(double &xpos, double &ypos) {
	if(!altIsPressed){
		xLock = -100;
	}
	else if(xLock == -100) {
		xLock = xpos;
	}

	if(!leftCtrlIsPressed){
		yLock = -100;
	}
	else if(yLock == -100) {
		yLock = ypos;
	}

	if(xLock != -100)
		xpos = xLock;
	if(yLock != -100)
		ypos = yLock;
}

void wacomUnlock() {
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
	resetWallCell(xpos, ypos);
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
			if(!wacomPen)
				 mouseLeftPressOrDrag(xpos, ypos);
			else
				wacomLeftPressOrDrag(xpos, ypos);
			mouseLeftPressed = true;  // start dragging
			//printf("left mouse pressed\n");
    	} else if(action == GLFW_RELEASE) {
    		if(wacomPen)
    			wacomUnlock();
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
					if(leftCtrlIsPressed || rightCtrlIsPressed)
						switchExcitationType(ex_sin);
					else
						changeExcitationFixedFreq(0);
					break;
				case GLFW_KEY_1:
				case GLFW_KEY_KP_1:
					if(leftCtrlIsPressed || rightCtrlIsPressed)
						switchExcitationType(ex_square);
					else
						changeExcitationFixedFreq(1);
					break;
				case GLFW_KEY_2:
				case GLFW_KEY_KP_2:
					if(leftCtrlIsPressed || rightCtrlIsPressed)
						switchExcitationType(ex_noise);
					else
						changeExcitationFixedFreq(2);
					break;
				case GLFW_KEY_3:
				case GLFW_KEY_KP_3:
					if(leftCtrlIsPressed || rightCtrlIsPressed)
						switchExcitationType(ex_impTrain);
					else
						changeExcitationFixedFreq(3);
					break;
				case GLFW_KEY_4:
				case GLFW_KEY_KP_4:
					if(leftCtrlIsPressed || rightCtrlIsPressed)
						switchExcitationType(ex_const);
					else
						changeExcitationFixedFreq(4);
					break;
				case GLFW_KEY_5:
				case GLFW_KEY_KP_5:
					if(leftCtrlIsPressed || rightCtrlIsPressed)
						switchExcitationType(ex_file);
					else
						changeExcitationFixedFreq(5);
					break;
				case GLFW_KEY_6:
				case GLFW_KEY_KP_6:
					if(leftCtrlIsPressed || rightCtrlIsPressed)
						switchExcitationType(ex_windowedImp);
					else
						changeExcitationFixedFreq(6);
					break;
				case GLFW_KEY_7:
				case GLFW_KEY_KP_7:
					if(leftCtrlIsPressed || rightCtrlIsPressed) {
						switchExcitationType(ex_oscBank);
					}
					else
						changeExcitationFixedFreq(7);
					break;
				case GLFW_KEY_8:
				case GLFW_KEY_KP_8:
					break;
				case GLFW_KEY_9:
				case GLFW_KEY_KP_9:
					break;

				case GLFW_KEY_L:
					break;

				case GLFW_KEY_BACKSPACE:
				case GLFW_KEY_DELETE:
					clearDomain();
					break;

				case GLFW_KEY_ENTER:
					resetSimulation();
					break;

				case GLFW_KEY_SPACE:
					slowMotion(true);
					break;

				default:
					break;
			}
		}

		switch(key) {
			case GLFW_KEY_A:
				if(!shiftIsPressed)
					shiftListener('h', -deltaPos);
				break;
			case GLFW_KEY_D:
				if(!shiftIsPressed)
					shiftListener('h', deltaPos);
				break;
			case GLFW_KEY_W:
				if(!shiftIsPressed)
					shiftListener('v', deltaPos);
				break;
			case GLFW_KEY_S:
				if(!shiftIsPressed && !leftCtrlIsPressed)
					shiftListener('v', -deltaPos);
				break;

			case GLFW_KEY_Z:
				changeMaxPressure(-deltaMaxPressure);
				break;
			case GLFW_KEY_X:
				changeMaxPressure(deltaMaxPressure);
				break;

			case GLFW_KEY_C:
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

			case GLFW_KEY_J:
				break;
			case GLFW_KEY_K:
				break;

			case GLFW_KEY_O:
				changeAllWallBetas(-deltaAllWallBetas);
				break;
			case GLFW_KEY_P:
				changeAllWallBetas(deltaAllWallBetas);
				break;

			case GLFW_KEY_B:
				break;
			case GLFW_KEY_H:
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
	window = fdtdSynth->getWindow(winSize); // try, but we all know it won't work |:
	while(window == NULL) {
		usleep(1000);			          // wait
		window = fdtdSynth->getWindow(winSize); // try again...patiently
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
		glfwSetInputMode(fdtdSynth->getWindow(winSize), GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
	}

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

	audioEngine.stopEngine();
	cout << "control thread ended" << endl;
	return (void *)0;
}

