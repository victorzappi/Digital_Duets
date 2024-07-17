/*
 * controls_multitouch.h
 *
 *  Created on: Jul 2, 2024
 *      Author: Victor Zappi
 */
#include <cstdio>
#include <string>
#include <list>
#include <unistd.h>
#include <fcntl.h>
#include <linux/input.h>
#include <GL/glew.h>    // include GLEW and new version of GL on Windows [luv Windows]
#include <GLFW/glfw3.h> // GLFW helper library


#include "controls.h"
#include "priority_utils.h"
#include "disableMouse.h" // temporarily disable mouse control when using touchscreen interface
#include "HyperDrumhead.h" // for AREA_NUM

#define MAX_NUM_EVENTS 64 // found here http://ozzmaker.com/programming-a-touchscreen-on-the-raspberry-pi/

using std::string;
using std::to_string;


pthread_t multitouchThread;
bool multitouchShouldStop = false;


int touchScreenFileHandler = -1;
int monitorPos[2] = {-1, -1};
int monitorReso[2];

int maxTouches;
int maxXpos;
int maxYpos;
int maxArea;
bool *posReceived[2];


// from main.cpp
extern int monitorIndex;
extern string touchScreenName;

// from control.cpp, for touch control
extern bool *touchOn;
extern int touchSlots;
extern int maxPressure;
extern int windowPos[2];
extern float *touchPos[2];
extern bool *newTouch;
extern bool *extraTouchDone;
extern float *touchPos_prev[2];
extern int *touch_elderTwin;
extern int *touch_youngerTwin;
extern bool *touchIsInhibited;
extern int *pressure;
extern int *prevTouchedArea;
extern bool *touchOnBoundary[2];
extern int *keepHoldTouch_area;
extern int keepHoldArea_touch[AREA_NUM];
extern list<int> triggerArea_touches[AREA_NUM];
extern list<pair<int, int>> *triggerArea_touch_coords[AREA_NUM];
extern int *triggerTouch_area;
extern motionFinger_mode *preFingersTouch_motion;
extern bool *preFingersTouch_motionNegated;
extern int firstPreFingerArea_touch[AREA_NUM];
extern list<int> preFingersArea_touches[AREA_NUM];
extern int *preFingersTouch_area;
extern bool *is_preFingerMotion_dynamic;
extern bool *is_preFingerPress_dynamic;
extern int *boundTouch_areaListener;
extern int *boundTouch_areaListener;
extern int *boundTouch_areaDamp;
extern int *boundTouch_areaProp;
extern int *boundTouch_areaPinchRef;
extern int *boundTouch_areaPinch;
extern int boundAreaListener_touch[AREA_NUM];
extern int boundAreaProp_touch[AREA_NUM];
extern int boundAreaDamp_touch[AREA_NUM];
extern int boundAreaPinchRef_touch[AREA_NUM];
extern int boundAreaPinch_touch[AREA_NUM];
extern int *pinchRef_pos[2];
extern pressFinger_mode *preFingersTouch_press;
extern pinchFinger_mode *preFingersTouch_pinch;
extern bool *is_preFingerPinch_dynamic;
extern bool *is_preFingerPinchRef_dynamic;
extern int postFingerArea_touch[AREA_NUM];
extern int *postFingerTouch_area;
extern motionFinger_mode postFingerMotion[AREA_NUM];
extern pressFinger_mode postFingerPress[AREA_NUM];


bool initMultitouchControls(GLFWwindow *window) {
	// let's search the requested touchscreen device
	string event_device_path = "/dev/input/event";
	int eventNum = 0; // to cycle all event devices' indices
	string event_device; // for info about of current device
	char name[256] = "Unknown"; // for the name current device
	bool touchscreenFound = true;

	do {
		event_device = event_device_path + to_string(eventNum); // build full path of current device

		touchScreenFileHandler = open(event_device.c_str(), O_RDONLY); // open it

		// reached end of files, requested device is not there
		if (touchScreenFileHandler == -1) {
			touchscreenFound = false; // can't use touchscreen ):
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


	if(!touchscreenFound) {
		printf("Can't find touchscreen device \"%s\"\n", touchScreenName.c_str());
		return false;
	}

	printf("\nReading from:\n");
	printf("device file = %s\n", event_device.c_str());
	printf("device name = %s\n\n", name);



	// disable mouse control
	disableMouse(touchScreenName);

	// hide cursor
	//	int winSize[2];
	//	glfwSetInputMode(hyperDrumSynth->getWindow(winSize), GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);



	// read touch surface specs
	int abs[6] = {0};
	ioctl(touchScreenFileHandler, EVIOCGABS(ABS_MT_SLOT), abs);
	maxTouches = abs[2];
	ioctl(touchScreenFileHandler, EVIOCGABS(ABS_MT_POSITION_X), abs);
	maxXpos = abs[2];
	ioctl(touchScreenFileHandler, EVIOCGABS(ABS_MT_POSITION_Y), abs);
	maxYpos = abs[2];
	ioctl(touchScreenFileHandler, EVIOCGABS(ABS_MT_PRESSURE), abs);
	//*maxPressure = abs[2];
	maxPressure = 700; //VIC cos reading is wrong...cos it's not fucking pressure but area... ANOTHER DISPLAX BUG
	ioctl(touchScreenFileHandler, EVIOCGABS(ABS_MT_TOUCH_MAJOR), abs);
	maxArea = abs[2]; // not used

	// init some working vars
	posReceived[0] = new bool[maxTouches];
	posReceived[1] = new bool[maxTouches];
	for(int i=0; i<maxTouches; i++) {
		touchOn[i] = false;
		posReceived[0][i] = false;
		posReceived[1][i] = false;
	}


	// handle position of touchscreen across (multi)monitor setup
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

	// we need to know how big is the window [resolution], to check where we are touching
	const GLFWvidmode* vmode = glfwGetVideoMode(mon);
	monitorReso[0] = vmode->width;
	monitorReso[1] = vmode->height;



	touchSlots += maxTouches; // this is initlized in controls.cpp, we simply add the slots for the multitouch here

	return true;
}






//VIC needed to deal with broken displax lines, otherwise harmless
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









void *readMultitouchLoop(void *) {
	set_priority(1);
	set_niceness(-20); // even if it should be inherited from calling process...but it's not

	// working vars
	const size_t ev_size = sizeof(struct input_event)*MAX_NUM_EVENTS;
	ssize_t readSize;
	int currentSlot = 0;
	struct input_event ev[MAX_NUM_EVENTS]; // where to save event blocks

	while(!multitouchShouldStop) {
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

					// translate into some kind of mouse pos, relative to window
					float posX = touchPos[0][currentSlot]-windowPos[0];
					float posY = touchPos[1][currentSlot]-windowPos[1];


					// if not touching window, fuck off
					if( !isOnWindow((int)posX, (int)posY) ) {
						newTouch[currentSlot] = false;
						continue;
					}
//					// if not touching window, fuck off
//					if( !( (touchPos[0][currentSlot] >= windowPos[0])&&(touchPos[0][currentSlot] < windowPos[0]+winSize[0]) ) ||
//						!( (touchPos[1][currentSlot] >= windowPos[1])&&(touchPos[1][currentSlot] < windowPos[1]+winSize[1]) ) ) {
//						newTouch[currentSlot] = false;
//						continue;
//					}

					if(!posReceived[0][currentSlot])
						touchPos_prev[0][currentSlot] = touchPos[0][currentSlot];
					if(!posReceived[1][currentSlot])
						touchPos_prev[1][currentSlot] = touchPos[1][currentSlot];

					posReceived[0][currentSlot] = false;
					posReceived[1][currentSlot] = false;


//					// translate into some kind of mouse pos, relative to window
//					float posX = touchPos[0][currentSlot]-windowPos[0];
//					float posY = touchPos[1][currentSlot]-windowPos[1];
					//printf("touch pos: %f %f\n", posX, posY);
					//printf("touch pos old: %f %f\n", posX_old, posY_old);

					// then to domain coords (:
					int coord[2] = {(int)posX, (int)posY};
					getCellDomainCoords(coord); // input is relative to window

					//printf("touch coord: %d %d\n", coord[0], coord[1]);
					//printf("touch coord old: %d %d\n\n", coord_old[0], coord_old[1]);

					int touchedArea = getAreaIndex(coord[0], coord[1]);

					//VIC this seems not needed, because always executed at the end of this scope
//					if(newTouch[currentSlot]) {
//						prevTouchedArea[currentSlot] = touchedArea;
//						//printf("%d is new touch and prev touched area is set to this area %d\n", currentSlot, touchedArea);
//					}

					if(!modifierActions(touchedArea, coord[0], coord[1])) {
						// if no generic modifiers are active
						if(newTouch[currentSlot]) { // when playing percussions we may handle the extra touch to fix the displax bug
							 handleNewTouch(currentSlot, touchedArea, coord);
						}
						// touch is not new
						else {
							handleTouchDrag(currentSlot, touchedArea, coord);
						}
						changeMousePos(-1, -1); // hide cross on touch location
					}
					else
						changeMousePos(touchPos[0][currentSlot], touchPos[1][currentSlot]); // with this we draw a cross on touch coord, useful when drawing/deleting cells via modifiers

					prevTouchedArea[currentSlot] = touchedArea;
					newTouch[currentSlot] = false;
				}
				/*else if(ev[i].code == ABS_MT_TOUCH_MAJOR) {
					printf("Area %d\n", ev[i].value);
				}*/
			}
		}

		usleep(1);
	}
	return (void *)0;
}


void startMultitouchControls() {
	if(touchScreenFileHandler == -1)
		return;

	// launch thread
	pthread_create(&multitouchThread, NULL, readMultitouchLoop, NULL);
}

void stopMultitouchControls() {
	if(touchScreenFileHandler == -1)
		return;

	multitouchShouldStop = true;
	pthread_join(multitouchThread, NULL); // wait for thread to finish

	delete[] posReceived[0];
	delete[] posReceived[1];

	// re-enable mouse control
	enableMouse(touchScreenName);

	close(touchScreenFileHandler);
	printf("Multitouch device nicely shut (:\n");
}
