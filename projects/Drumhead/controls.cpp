/*
 * controls.cpp
 *
 *  Created on: 2015-10-30
 *      Author: Victor Zappi
 */




#include "controls.h"

//back end
#include "Engine.h"
#include "priority_utils.h"

// applications stuff
#include "DrumSynth.h"
#include "DrumExcitation.h"

extern int verbose;

//----------------------------------------
// main variables
//----------------------------------------
// extern
extern Engine audioEngine;
extern DrumSynth *drumSynth;
//----------------------------------------

//----------------------------------------
// control variables
//---------------------------------------
extern bool wacomPen;
extern int listenerCoord[2];
extern float excitationVol;

int deltaPos           = 1;
float deltaPressure    = 0.05;

float freq      = 440.0;
float deltaFreq = freq/100.0;
float oscFrequencies[FREQS_NUM] = {50, 100, 200, 400, 800, 1600, 3200, 6400};

extern float lowPassFreq;
float deltaLowPassFreq;

float allWallBetas      = 0;
float deltaAllWallBetas = 0.005;

extern float dampFactor;
float deltaDampFactor;

extern float propagationFactor;
float deltaPropFactor;

extern float clampFactor;
float deltaClampFactor = 0.05;

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
	coords[1] = winSize[1] - coords[1] - 1;

	// we get window coordinates, but this filter works with domain ones
	drumSynth->fromWindowToDomain(coords);
}

void changeMousePos(int mouseX, int mouseY) {
	int coord[2] = {(int)mouseX, (int)mouseY};
	getCellDomainCoords(coord);

	drumSynth->setMousePos(coord[0], coord[1]);
}


void slowMotion(bool slowMo) {
	//drumSynth->setSlowMotion(slowMo);
}

void setWallCell(float mouseX, float mouseY) {
	int coord[2] = {(int)mouseX, (int)mouseY};
	getCellDomainCoords(coord);

	drumSynth->setCell(DrumheadGPU::cell_wall, coord[0], coord[1]);
}

void setExcitationCell(float mouseX, float mouseY) {
	int coord[2] = {(int)mouseX, (int)mouseY};
	getCellDomainCoords(coord);

	drumSynth->setCell(DrumheadGPU::cell_excitation, coord[0], coord[1]);
}

void resetWallCell(float mouseX, float mouseY) {
	int coord[2] = {(int)mouseX, (int)mouseY};
	getCellDomainCoords(coord);

	drumSynth->resetCell(coord[0], coord[1]);
}

void clearDomain() {
	drumSynth->clearDomain();
}

void resetSimulation() {
	drumSynth->resetSimulation();
}

// from GLFW window coordinates
void moveListener(float mouseX, float mouseY) {
	int coord[2] = {(int)mouseX, (int)mouseY};
	getCellDomainCoords(coord);

	// finally move listener
	if(drumSynth->setListenerPosition(coord[0], coord[1]) == 0) {
		drumSynth->getListenerPosition(listenerCoord[0], listenerCoord[1]);
		printf("Listener position (in pixels): (%d, %d)\n", listenerCoord[0], listenerCoord[1]); //get pos
	}
}

void shiftListener(char dir, int delta) {
	if(dir == 'h') {
		listenerCoord[0] = drumSynth->shiftListenerH(delta);
		printf("Listener position (in pixels): (%d, %d)\n", listenerCoord[0], listenerCoord[1]);
	}else if(dir == 'v') {
		listenerCoord[1] = drumSynth->shiftListenerV(delta);
		printf("Listener position (in pixels): (%d, %d)\n", listenerCoord[0], listenerCoord[1]);
	}
}

// from GLFW window coordinates
void moveFeedback(float mouseX, float mouseY) {
	int coord[2] = {(int)mouseX, (int)mouseY};
	getCellDomainCoords(coord);

	// finally move listener
	/*if(//drumSynth->setFeedbackPosition(coord[0], coord[1])==0) {
		//drumSynth->getFeedbackPosition(feedbCoord[0], feedbCoord[1]);
		printf("Feedback position (in pixels): (%d, %d)\n", coord[0], coord[1]);
	}*/
}

void changeVolume(float delta) {
	excitationVol += delta;
	drumSynth->setExcitationVolume(excitationVol);
	printf("Excitation volume: %.2f\n", excitationVol);
}

void switchExcitationType(drumEx_type subGlotEx) {
	drumSynth->setSubGlotExType(subGlotEx);
	//printf("Current excitation type: %s\n", DrumExcitation::excitationNames[subGlotEx].c_str());
}

void changeDampingFactor(float deltaDF) {
	float nextD = dampFactor+deltaDF;
	if(nextD >=0) {
		drumSynth->setDampingFactor(nextD);
		dampFactor = nextD;
		deltaDampFactor = nextD/100.0; // keep ratio constant
		printf("Damping factor: %f\n", dampFactor);
	}
}

void changePropagationFactor(float deltaP) {
	float nextP = propagationFactor+deltaP;
	if(nextP<0)
		nextP = 0.0001;
	else if(nextP>0.5)
		nextP = 0.5;

	drumSynth->setPropagationFactor(nextP);
	propagationFactor = nextP;
	printf("Propagation factor: %f\n", propagationFactor);
}

void changeClampFactor(float deltaC) {
	float nextC = clampFactor+deltaC;
	if(nextC<0)
		nextC = 0.0;
	else if(nextC>1)
		nextC = 1;

	drumSynth->setClampingFactor(nextC);
	clampFactor = nextC;
	printf("Calmping factor: %f\n", clampFactor);
}


void changeExcitationFreq(float deltaF) {
	float nextF = freq+deltaF;
	if(nextF >=0) {
		drumSynth->setExcitationFreq(nextF);
		freq = nextF;
		deltaFreq = nextF/100.0; // keep ratio constant
		printf("Excitation freq: %f [Hz]\n", freq);
	}
}

void changeExcitationFixedFreq(int id) {
	drumSynth->setExcitationFixedFreq(id);
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

	drumSynth->setAllWallBetas(nextB); //VIC this has strange behavior...check it
	allWallBetas = nextB;
	printf("Set betas (\"airness\") for all walls to: %f\n", allWallBetas);
}

void changeLowPassFilterFreq(float deltaF) {
	float nextF = lowPassFreq+deltaF;
	if(nextF >=0 && nextF<= 22050) {
		drumSynth->setExcitationLowPassFreq(nextF);
		lowPassFreq = nextF;
		deltaLowPassFreq = nextF/100.0;
		printf("Excitation low pass freq: %f [Hz]\n", lowPassFreq);
	}
	else
		printf("Requested excitation low pass freq %f [Hz] is out range [0, 22050 Hz]\n", nextF);
}

void saveFrame() {
	/*int retval=//drumSynth->saveFrame();
	if(retval==0)
		printf("Current frame saved\n");
	else if(retval==-1)
		printf("Init Vocal Synthesizer before trying to save frame file!\n");*/
}

void saveFrameSequence() {
	/*int retval=//drumSynth->saveFrameSequence();
	if(retval==0)
		printf("Current frame sequence saved \n");
	else if(retval==-1)
		printf("Init Vocal Synthesizer before trying to save frame sequence file!\n");*/
}

void computeFrameSequence() {
	/*string foldername = "data/seq_0_120x60";
	int retval = //drumSynth->computeFrameSequence(foldername);
	if(retval==0)
		printf("Computed frame sequence from folder \"%s\"\n", foldername.c_str());
	else if(retval==-1)
		printf("Init Vocal Synthesizer before trying to compute frame sequence!\n");*/
}

void openFrame() {
	/*string filename = "data/seq_0_120x60/framedump_0.frm";
	int retval = //drumSynth->openFrame(filename);
	//string filename = "data/seq_0.fsq";
	//int retval = //drumSynth->openFrameSequence(filename);
	if(retval==0)
		printf("Loaded frame from file \"%s\"\n", filename.c_str());
	else if(retval==-1)
		printf("Init Vocal Synthesizer before trying to open frame file!\n");*/
}

void openFrameSequence() {
	/*string filename = "data/seq_0_120x60.fsq";
	int retval = //drumSynth->openFrameSequence(filename);
	if(retval==0)
		printf("Loaded frame from file \"%s\"\n", filename.c_str());
	else if(retval==-1)
		printf("Init Vocal Synthesizer before trying to open frame file!\n");*/
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
					if(leftCtrlIsPressed)
						switchExcitationType(drumEx_sandbox);
					else
						changeExcitationFixedFreq(0);
					break;
				case GLFW_KEY_1:
				case GLFW_KEY_KP_1:
				    if(leftCtrlIsPressed)
				    	switchExcitationType(drumEx_const);
					else
						changeExcitationFixedFreq(1);
					break;
				case GLFW_KEY_2:
				case GLFW_KEY_KP_2:
					if(leftCtrlIsPressed)
						switchExcitationType(drumEx_sin);
					else
						changeExcitationFixedFreq(2);
					break;
				case GLFW_KEY_3:
				case GLFW_KEY_KP_3:
					if(leftCtrlIsPressed)
						switchExcitationType(drumEx_square);
					else
						changeExcitationFixedFreq(3);
					break;
				case GLFW_KEY_4:
				case GLFW_KEY_KP_4:
					if(leftCtrlIsPressed)
						switchExcitationType(drumEx_noise);
					else
						changeExcitationFixedFreq(4);
					break;
				case GLFW_KEY_5:
				case GLFW_KEY_KP_5:
					if(leftCtrlIsPressed)
						switchExcitationType(drumEx_impTrain);
					else
						changeExcitationFixedFreq(5);
					break;
				case GLFW_KEY_6:
				case GLFW_KEY_KP_6:
					if(leftCtrlIsPressed)
						switchExcitationType(drumEx_file);
					else
						changeExcitationFixedFreq(6);
					break;
				case GLFW_KEY_7:
				case GLFW_KEY_KP_7:
					if(leftCtrlIsPressed)
						switchExcitationType(drumEx_windowedImp);
					else
						changeExcitationFixedFreq(7);
					break;
				case GLFW_KEY_8:
				case GLFW_KEY_KP_8:
					if(leftCtrlIsPressed)
						switchExcitationType(drumEx_fileImp);
					break;
				case GLFW_KEY_9:
				case GLFW_KEY_KP_9:
					if(leftCtrlIsPressed)
						switchExcitationType(drumEx_oscBank);
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
				else if(!shiftIsPressed && leftCtrlIsPressed)
					saveFrame();
				else if(shiftIsPressed && leftCtrlIsPressed)
					saveFrameSequence();
				break;

			case GLFW_KEY_Z:
				changeDampingFactor(-deltaDampFactor);
				break;
			case GLFW_KEY_X:
				changeDampingFactor(deltaDampFactor);
				break;

			case GLFW_KEY_K:
				changePropagationFactor(-deltaPropFactor);
				break;
			case GLFW_KEY_L:
				changePropagationFactor(deltaPropFactor);
				break;

			case GLFW_KEY_C:
				if(shiftIsPressed && leftCtrlIsPressed)
					computeFrameSequence();
				else
					changeVolume(-deltaPressure);
				break;
			case GLFW_KEY_V:
				changeVolume(deltaPressure);
				break;

			case GLFW_KEY_N:
				changeExcitationFreq(-deltaFreq);
				break;
			case GLFW_KEY_M:
				changeExcitationFreq(deltaFreq);
				break;

			case GLFW_KEY_H:
				changeLowPassFilterFreq(-deltaLowPassFreq);
				break;
			case GLFW_KEY_J:
				changeLowPassFilterFreq(deltaLowPassFreq);
				break;

			case GLFW_KEY_U:
				changeAllWallBetas(-deltaAllWallBetas);
				break;
			case GLFW_KEY_I:
				changeAllWallBetas(deltaAllWallBetas);
				break;

			case GLFW_KEY_O:
				changeClampFactor(-deltaClampFactor);
				break;
			case GLFW_KEY_P:
				changeClampFactor(deltaClampFactor);
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
	window = drumSynth->getWindow(winSize); // try, but we all know it won't work |:
	while(window == NULL) {
		usleep(1000);			          // wait
		window = drumSynth->getWindow(winSize); // try again...patiently
		if(!drumSynth->getUseGPU()) // we check continuously, because this setting might be changed when the audio thread is parsing command arguments
			return;
	}

	// callbacks
	glfwSetMouseButtonCallback(window, mouse_button_callback); // mouse buttons down
	glfwSetCursorPosCallback(window, cursor_position_callback);
	glfwSetCursorEnterCallback(window, cursor_enter_callback);
	glfwSetKeyCallback(window, key_callback);
}


void initControlThread() {
	initCallbacks();

	// we check again, at this point this setting should be finally fixed once for all
	if(!drumSynth->getUseGPU())
		return;

	// hide cursor if using wacom pen to control
	if(wacomPen) {
		int winSize[2];
		glfwSetInputMode(drumSynth->getWindow(winSize), GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
	}


	deltaLowPassFreq = lowPassFreq/100.0;
	deltaDampFactor  = dampFactor/100.0;
	deltaPropFactor  = propagationFactor/100.0;
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
			switchExcitationType(drumEx_windowedImp);
			break;
		/*case 'd':
			shiftListener('h', deltaPos);
			break;
		case 'w':
			shiftListener('v', deltaPos);
			break;
		case 's':
			shiftListener('v', -deltaPos);
			break;
*/
		case 'z':
			break;
		case 'x':
			break;

		case 'c':
			changeVolume(-deltaPressure);
			break;
		case 'v':
			changeVolume(deltaPressure);
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

