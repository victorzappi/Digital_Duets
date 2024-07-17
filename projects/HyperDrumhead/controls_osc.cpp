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

bool shouldStop = false;

void parseMessage(oscpkt::Message msg) {
	set_priority(7);

	//printf("OSCmsg: %s\n", msg.addressPattern().c_str());

	// looks for any hdh messages
	int area, id;
	float val;
	if( msg.match("/hdh/vol").popInt32(area).popFloat(val).isOkNoMoreArgs() ) {
		//setAreaExcitationVolume(area, val);
		//modulateAreaVolNorm(area,  val);
		//printf("area %d vol: %f\n", area, val);
	} else if( msg.match("/hdh/rel").popInt32(area).popFloat(val).isOkNoMoreArgs() ) {
		setAreaExReleaseNorm(area,  val);
		//printf("rel: %f\n", val);
	} else if( msg.match("/hdh/filt").popInt32(area).popFloat(val).isOkNoMoreArgs() ) {
		setAreaLowPassFilterFreqNorm(area,  val);
		//printf("filt: %f\n", val);
	} else if( msg.match("/hdh/dampFact").popInt32(area).popFloat(val).isOkNoMoreArgs() ) {
		setAreaDampingFactorNorm(area,  val);
		//printf("damp: %f\n", val);
	} else if( msg.match("/hdh/propFact").popInt32(area).popFloat(val).isOkNoMoreArgs() ) {
		setAreaPropagationFactorNorm(area,  val);
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
	} else if( msg.match("/hdh/trigger").popInt32(area).popFloat(val).isOkNoMoreArgs() ) {
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
	} else if( msg.match("/hdh/excitationID").popInt32(area).popInt32(id).isOkNoMoreArgs() ) {
		setAreaExcitationID(area, id);
		//printf("OSC excitation ID %d\n", id);
	}
}

void *getMessageLoop(void *) {
	while(!shouldStop) {
		while(oscServer.messageWaiting())
			parseMessage(oscServer.popMessage());
		usleep(10);
	}
	return (void *)0;
}



void initOscControls() {
	// init server to receive messages
	oscServer.setup(localPort, 6);
	pthread_create(&serverThread, NULL, getMessageLoop, NULL);
}

void stopOsc() {
	shouldStop = true;
}
