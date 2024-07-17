/*
 * controls.cpp
 *
 *  Created on: 2015-10-30
 *      Author: Victor Zappi
 */




#include "controls.h"

#include <signal.h> // to catch ctrl-c

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
#define EVENT_DEVICE    "/dev/input/event15"
#define MOUSE_INPUT_ID 10 // check with: xinput -list
#define MAX_NUM_EVENTS 64 // found here http://ozzmaker.com/programming-a-touchscreen-on-the-raspberry-pi/
#define MAX_X_POS 4095 // checked with evtest [from apt-get repo]
#define MAX_Y_POS 4095 // checked with evtest too
#define MAX_TOUCHES 10


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
extern bool multitouch;
extern int monitorIndex;
int touchScreenFileHandler = -1;
int windowPos[2] = {-1, -1};

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
bool zIsPressed = false;
bool xIsPressed = false;
bool cIsPressed = false;
bool vIsPressed = false;

int xLock = -100;
int yLock = -100;
//----------------------------------------








// this is called by ctrl-c
void my_handler(int s){
   printf("\nCaught signal %d\n",s);
   audioEngine.stopEngine();
}




//----------------------------------------

void getCellDomainCoords(int *coords) {
	// y is uupside down
	coords[1] = winSize[1] - coords[1] - 1;
	//printf("Mouse coord: %d %d\n", coords[0], coords[1]);
	// we get window coordinates, but this filter works with domain ones
	drumSynth->fromWindowToDomain(coords);
}

void changeMousePos(int mouseX, int mouseY) {
	int coord[2] = {(int)mouseX, (int)mouseY};
	getCellDomainCoords(coord);

	drumSynth->setMousePos(coord[0], coord[1]);
}


inline void slowMotion(bool slowMo) {
	drumSynth->setSlowMotion(slowMo);
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

inline void clearDomain() {
	drumSynth->clearDomain();
}

inline void resetSimulation() {
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
	printf("Current excitation type: %s\n", DrumExcitation::excitationNames[subGlotEx].c_str());
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
	printf("Clamping factor: %f\n", clampFactor);
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





bool modifierActions(float mouseX, float mouseY) {
	bool retval = false;
	if(zIsPressed) {
		if(!shiftIsPressed)
			setWallCell(mouseX, mouseY);
		else
			resetWallCell(mouseX, mouseY);
		retval = true;
	}
	else if(xIsPressed) {
		if(!shiftIsPressed)
			setExcitationCell(mouseX, mouseY);
		else
			resetWallCell(mouseX, mouseY);
		retval = true;
	}
	else if(cIsPressed) {
		moveListener(mouseX, mouseY);
		retval = true;
	}
	if(retval)
		changeMousePos(mouseX, mouseY);
	return retval;
}


void mouseLeftPress(double mouseX, double mouseY) {
	if(!modifierActions(mouseX, mouseY))
		drumSynth->triggerExcitation();
}

void mouseLeftDrag(double mouseX, double mouseY) {
	modifierActions(mouseX, mouseY);
}

void mouseRightPressOrDrag(double posX, double posY) {
	resetWallCell(posX, posY);
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
		mouseLeftDrag(xpos, ypos);
	}
	// right mouse dragged
	else if(mouseRightPressed) {
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
					if(leftCtrlIsPressed)
						switchExcitationType(drumEx_windowedImp);
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
						switchExcitationType(drumEx_oscBank);
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
				else if(!shiftIsPressed && leftCtrlIsPressed)
					saveFrame();
				else if(shiftIsPressed && leftCtrlIsPressed)
					saveFrameSequence();
				break;



			case GLFW_KEY_M:
				changeExcitationFreq(deltaFreq);
				break;
			case GLFW_KEY_N:
				changeExcitationFreq(-deltaFreq);
				break;

			case GLFW_KEY_L:
				changeDampingFactor(deltaDampFactor);
				break;
			case GLFW_KEY_K:
				changeDampingFactor(-deltaDampFactor);
				break;

			case GLFW_KEY_P:
				changePropagationFactor(deltaPropFactor);
				break;
			case GLFW_KEY_O:
				changePropagationFactor(-deltaPropFactor);
				break;

			case GLFW_KEY_I:
				changeVolume(deltaPressure);
				break;
			case GLFW_KEY_U:
				changeVolume(-deltaPressure);
				break;

			case GLFW_KEY_Y:
				changeLowPassFilterFreq(deltaLowPassFreq);
				break;
			case GLFW_KEY_T:
				changeLowPassFilterFreq(-deltaLowPassFreq);
				break;

			case GLFW_KEY_J:
				changeExcitationFreq(deltaFreq);
				break;
			case GLFW_KEY_H:
				changeExcitationFreq(-deltaFreq);
				break;

/*			case GLFW_KEY_U:
				changeAllWallBetas(-deltaAllWallBetas);
				break;
			case GLFW_KEY_I:
				changeAllWallBetas(deltaAllWallBetas);
				break;*/


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

	// we check again, at this point this setting should be finally fixed once for all
	if(!drumSynth->getUseGPU())
		return;

	// hide cursor if using wacom pen to control
/*	if(wacomPen) {
		int winSize[2];
		glfwSetInputMode(drumSynth->getWindow(winSize), GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
	}*/


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





	if(!multitouch) {
		char keyStroke = '.';
		cout << "Press q to quit." << endl;

		do {
			keyStroke =	getchar();

			while(getchar()!='\n'); // to read the first stroke

			switch (keyStroke) {
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
					drumSynth->triggerExcitation();
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



	//----------------------------------------------------------------------------------
	// multitouch controls
	//----------------------------------------------------------------------------------

	// disable mouse control
	disableMouse(MOUSE_INPUT_ID);


	struct input_event ev[MAX_NUM_EVENTS]; // where to save event blocks
	char name[256] = "Unknown"; // for the name of the device

	// open the device!
	touchScreenFileHandler = open(EVENT_DEVICE, O_RDONLY);
	if (touchScreenFileHandler == -1) {
		fprintf(stderr, "%s is not a vaild device\n", EVENT_DEVICE);
		return (void *)0;
	}

	// print it's name, useful (;
	ioctl(touchScreenFileHandler, EVIOCGNAME(sizeof(name)), name);
	printf("\nReading from:\n");
	printf("device file = %s\n", EVENT_DEVICE);
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

						modifierActions(posX, posY); // we perform a double update [both on X and Y events], cos X and Y events do not always happen together [sometime only X, sometimes only Y]
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

					if(!modifierActions(posX, posY) && newTouch[currentSlot]) {
						drumSynth->triggerExcitation();
						changeMousePos(-1, -1);
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
	enableMouse(MOUSE_INPUT_ID);

	close(touchScreenFileHandler);
	printf("Multitouch device nicely shut (:\n");
}

