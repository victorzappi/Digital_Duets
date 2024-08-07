/*
 * oscControls.cpp
 *
 *  Created on: Dec 12, 2017
 *      Author: Victor Zappi
 */


#include "OSCClient.h"
#include "OSCServer.h"

#include "controls.h"

#include "priority_utils.h"



// for server [receive]
int localPort = 8000;

OSCServer oscServer;

pthread_t serverThread;

bool oscShouldStop = false;

int touchSlotOffset;

// from control.cpp, for remote touch control
extern bool *touchOn;
extern float *touchPos[2];
extern bool *newTouch;
extern int *prevTouchedArea;
extern float *touchPos_prev[2];

//extern int maxTouches;
extern int *pressure;
extern int maxPressure;
//extern const int remoteTouchesOsc;
//extern const int remoteTouchesMidi;

extern int windowPos[2];
extern int winSize[2];


void updateTouchRecordsOsc(int touchSlot, float &xpos, float &ypos, bool isNew=false) {
	// denormalize touch poisition, now relative to window size
	xpos *= winSize[0]-1;
	ypos *= winSize[1]-1;

	// update touch records
	if(!isNew) {
		touchPos_prev[0][touchSlot] = touchPos[0][touchSlot];
		touchPos_prev[1][touchSlot] = touchPos[1][touchSlot];
	}
	else {
		touchPos_prev[0][touchSlot] = xpos + windowPos[0]; // make it absolute position and not relative to window
		touchPos_prev[1][touchSlot] = ypos + windowPos[1];
	}
	touchPos[0][touchSlot] = xpos + windowPos[0];
	touchPos[1][touchSlot] = ypos + windowPos[1];
	//TODO make touchPos relative to window EVERYWHERE
}


void newOscTouch(int touchSlot, float xpos, float ypos) {
	// update touch records
	updateTouchRecordsOsc(touchSlot, xpos, ypos, true);

	// turn to domain coords (:
	int coord[2] = {(int)xpos, (int)ypos};
	getCellDomainCoords(coord); // input is relative to window

	newTouch[touchSlot] = true;  // for handleNewTouch()
	int touchedArea = getAreaIndex(coord[0], coord[1]);

	if(!modifierActions(touchedArea, coord[0], coord[1])) {
		pressure[touchSlot] = maxPressure; //VIC this has to change
		handleNewTouch(touchSlot, touchedArea, coord);
	}

	prevTouchedArea[touchSlot] = touchedArea;
	newTouch[touchSlot] = false; // dealt with
}


void oscTouchDrag(int touchSlot, float xpos, float ypos) {
	// update touch records
	updateTouchRecordsOsc(touchSlot, xpos, ypos);

	// turn to domain coords (:
	int coord[2] = {(int)xpos, (int)ypos};
	getCellDomainCoords(coord); // input is relative to window

	int touchedArea = getAreaIndex(coord[0], coord[1]);

	if(!modifierActions(touchedArea, coord[0], coord[1])) {
		pressure[touchSlot] = maxPressure; //VIC this has to change
		handleTouchDrag(touchSlot, touchedArea, coord);
	}

	prevTouchedArea[touchSlot] = touchedArea;
}

void oscTouchRelease(int touchSlot) {
	handleTouchRelease(touchSlot);
}

void parseMessage(oscpkt::Message msg) {

//	printf("OSCmsg: %s\n", msg.addressPattern().c_str());

	// looks for any hdh messages
	int area, id, touchSlot;
	float val, xpos, ypos;

	// remote touch control
	if( msg.match("/hdh/touch").popInt32(touchSlot).popFloat(xpos).popFloat(ypos).isOkNoMoreArgs() ) {
//		printf("touch:%d %f %f\n", touchSlot, xpos, ypos);
		touchSlot += touchSlotOffset;

		if(!touchOn[touchSlot]) {
//			printf("newOscTouch: %d %f, %f\n", touchSlot, xpos, ypos);
			newOscTouch(touchSlot, xpos, ypos);
			touchOn[touchSlot] = true;
		}
		else {
			oscTouchDrag(touchSlot, xpos, ypos);
//			printf("oscTouchDrag: %d %f, %f\n", touchSlot, xpos, ypos);

		}
	}
	else if( msg.match("/hdh/touchRelease").popInt32(touchSlot).isOkNoMoreArgs() ) {
//		printf("newOscRelease: %d\n", touchSlot);

		touchSlot += touchSlotOffset;

		touchOn[touchSlot] = false;
		oscTouchRelease(touchSlot);
	}
	// remote parameters control
	else if( msg.match("/hdh/vol").popInt32(area).popFloat(val).isOkNoMoreArgs() ) {
		//setAreaExcitationVolume(area, val);
		modulateAreaVolNorm(area,  val);
		//printf("area %d vol: %f\n", area, val);
	} else if( msg.match("/hdh/rel").popInt32(area).popFloat(val).isOkNoMoreArgs() ) {
		setAreaExReleaseNorm(area,  val);
		//printf("rel: %f\n", val);
	} else if( msg.match("/hdh/filt").popInt32(area).popFloat(val).isOkNoMoreArgs() ) {
		setAreaLowPassFilterFreqNorm(area,  val);
		//printf("filt: %f\n", val);
	} else if( msg.match("/hdh/dampFact").popInt32(area).popFloat(val).isOkNoMoreArgs() ) {
		setAreaDampNorm(area,  val);
		//printf("damp: %f\n", val);
	} else if( msg.match("/hdh/propFact").popInt32(area).popFloat(val).isOkNoMoreArgs() ) {
		setAreaPropNorm(area,  val);
		//printf("prop: %f\n", val);
	} else if( msg.match("/hdh/posX").popInt32(area).popFloat(val).isOkNoMoreArgs() ) {
		setAreaListPosNormX(area,  val);
		//printf("pos x: %f\n", val);
	} else if( msg.match("/hdh/posY").popInt32(area).popFloat(val).isOkNoMoreArgs() ) {
		setAreaListPosNormY(area,  val);
		//printf("pos Y: %f\n", val);
	} else if( msg.match("/hdh/posY").popInt32(area).popFloat(val).isOkNoMoreArgs() ) {
		setAreaListPosNormY(area,  val);
		//printf("pos Y: %f\n", val);
	} /*else if( msg.match("/hdh/trigger").popInt32(area).popFloat(val).isOkNoMoreArgs() ) {
		if(val > 0) {
			setAreaVolNorm(area, val);
			triggerAreaExcitation(area);
			//printf("trigger area %d: %f\n", area, val);
		}
		else if(val == 0) {
			dampAreaExcitation(area);
			//printf("damp area %d\n", area);
		}
	} else if( msg.match("/hdh/damp").popInt32(area).isOkNoMoreArgs() ) {
		dampAreaExcitation(area);
		//printf("damp area %d\n", area);
	}*/ else if( msg.match("/hdh/excitationID").popInt32(area).popInt32(id).isOkNoMoreArgs() ) {
		setChannelExcitationID(area, id);
		//printf("OSC excitation ID %d\n", id);
	}
}

void *getMessageLoop(void *) {
	set_priority(7);

	while(!oscShouldStop) {
		while(oscServer.messageWaiting())
			parseMessage(oscServer.popMessage());
		usleep(10);
	}
	return (void *)0;
}

void initOscControls(int touchslotoffset) {
	// init server to receive messages
	oscServer.setup(localPort, 6);

	touchSlotOffset = touchslotoffset;
}

void startOscControls() {
	// launch thread
	pthread_create(&serverThread, NULL, getMessageLoop, NULL);
}

void stopOscControls() {
	oscShouldStop = true;
	pthread_join(serverThread, NULL); // wait for thread to finish, just in case
}
