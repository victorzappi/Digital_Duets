/*
 * monomeControl.cpp
 *
 *  Created on: Sep 20, 2017
 *      Author: Victor Zappi
 */

#include "monomeControl.h"

#include "HyperDrumSynth.h"

#include "OSCClient.h"
#include "OSCServer.h"

#include "priority_utils.h"

#include "OscSliderV.h"
#include "OscPulseButton.h"
#include "OscButton.h"

#define NUM_OF_SLIDERS 4
#define NUM_OF_SAMPLEBTNS 12


// for client
int remotePort = 8001;
const char* remoteIp = "127.0.0.1";
// for server
int localPort = 8000;


OSCClient oscClient;
OSCServer oscServer;

pthread_t serverThread;

bool shouldStop = false;



extern HyperDrumSynth *hyperDrumSynth;


// sliders and buttons
OscSliderV slider[NUM_OF_SLIDERS];
OscPulseButton drawBtn[2];
OscButton sampleBtn[NUM_OF_SAMPLEBTNS];
OscButton optionBtn[3];

// we keep track of these 2 sliders, volume/scale control [where scale is propagation factor]
int volumeSlider = 5;
int scaleSlider  = 7;

int wait = 1000; // to avoid some packages to go lost when to many osc messages are sent in a row [would a bundle be better? who knows]

void resetMonome() {
	// reset sliders to initial values
	slider[0].setSlider(5);
	volumeSlider = 5;
	usleep(wait);
	slider[1].setSlider(7);
	usleep(wait);
	slider[2].setSlider(7);
	scaleSlider = 7;
	usleep(wait);
	slider[3].setSlider(7);
	usleep(wait);

	// reset on sine
	sampleBtn[0].setButton(true);
	usleep(wait);
	for(int i=1; i<NUM_OF_SAMPLEBTNS; i++) {
		sampleBtn[i].setButton(false);
		usleep(wait);
	}
}

void parseMessage(oscpkt::Message msg) {
	set_priority(50);

	// looks for any key up message
	int row, col, cmd;
	if(msg.match("/monome/grid/key").popInt32(row).popInt32(col).popInt32(cmd).isOkNoMoreArgs() ) {
		if(cmd == 1) { // button down
			// sliders, the 4 rightmost vertical lines
			if(col > 3) {
				slider[col-4].setSlider(row); // last four columns of monome

				// volume/scale control
				if(col-4 == 0)
					volumeSlider = row;
				else if(col-4 == 2) {
					scaleSlider = row;
					// if scale slider is lower than volume, we lower down the volume, to avoid sonic booms
					if(scaleSlider<volumeSlider) {
						usleep(wait); // again, to avoid some packages to go lost
						slider[0].setSlider(scaleSlider); // down
						volumeSlider = scaleSlider; // update
					}
				}

				return;
			}
			// editing buttons, 2 at bottom left
			else if(row==0 && col<2) {
				// exclusive activation
				drawBtn[col].setButton(true);
				drawBtn[1-col].setButton(false);
			}
			// option buttons, 3 in low left part
			else if(row==2 && col<3) {
				optionBtn[col].setButton(true);
				if(col==1)
					resetMonome(); // second option buttons resets monome too
			}
			// sample buttons, 4x3 block in top left
			else if(row>3 && col<3) {
				bool activeBtns[NUM_OF_SAMPLEBTNS]={false};
				int x = NUM_OF_SAMPLEBTNS/4 - (row-NUM_OF_SAMPLEBTNS/3); // remove shift and INVERT, cos button[0] is top left and rows grow downwards
				int y = col;
				activeBtns[x*3+y] = true;

				for(int i=0; i<NUM_OF_SAMPLEBTNS; i++)
					sampleBtn[i].setButton(activeBtns[i]); // init on sine
			}
		}
		else if(cmd == 0) { // button up
			// option buttons, 3 in low left part
			if(row==2 && col<3) {
				optionBtn[col].setButton(false);
			}
		}
	}
}

void *getMessageLoop(void *) {
	while(!shouldStop) {
		while(oscServer.messageWaiting())
			parseMessage(oscServer.popMessage());
		usleep(1000);
	}
	return (void *)0;
}

void initMonome(float &boundGain, bool &draw_wait, bool &delete_wait, int inputNum) {
	// init server to receive messages
	oscServer.setup(localPort, 50);
	pthread_create(&serverThread, NULL, getMessageLoop, NULL);

	// init client to send messages
	oscClient.setup(remotePort, remoteIp, 50);

	// lambdas that will be called by sliders [only 3 of them]
	auto f_volume      = [](float vol)  { for(int i=0; i<11; i++) hyperDrumSynth->setAreaExcitationVolume(i, vol); }; //VIC: [bad] 11 is num of touches + 1 [mouse] for USBest Technology SiS
	auto f_damping     = [](float damp) { hyperDrumSynth->setAreaDampingFactor(0, damp); };
	auto f_propagation = [](float prop) { hyperDrumSynth->setAreaPropagationFactor(0, prop); };

	// 4 sliders, in the last four columns of monome
	slider[0].init(4, volumeSlider, &oscClient, f_volume, 0.001, 1.0);
	slider[1].init(5, 7, &oscClient, f_damping, 0.00001, 0.03);
	slider[2].init(6, scaleSlider, &oscClient, f_propagation, 0.001, 0.5);
	slider[3].init(7, 7, &oscClient, &boundGain, 0, 1, false);


	// 2 editing buttons, bottom left of monome
	drawBtn[0].init(0, 0, &oscClient, &draw_wait);
	drawBtn[1].init(0, 1, &oscClient, &delete_wait);


	// lambdas that will be called by the sample buttons
	auto f_sin      = []() { for(int i=0; i<11; i++) hyperDrumSynth->setAreaExcitationType(i, drumEx_sin); }; //VIC: [bad] 11 is num of touches + 1 [mouse] for USBest Technology SiS -> can't capture it, cos it is not global var
	auto f_square   = []() { for(int i=0; i<11; i++) hyperDrumSynth->setAreaExcitationType(i, drumEx_square); }; //VIC: [bad] 11 is num of touches + 1 [mouse] for USBest Technology SiS
	auto f_noise    = []() { for(int i=0; i<11; i++) hyperDrumSynth->setAreaExcitationType(i, drumEx_noise); }; //VIC: [bad] 11 is num of touches + 1 [mouse] for USBest Technology SiS
	auto f_oscBank  = []() { for(int i=0; i<11; i++) hyperDrumSynth->setAreaExcitationType(i, drumEx_oscBank); }; //VIC: [bad] 11 is num of touches + 1 [mouse] for USBest Technology SiS
	auto f_oscBurst = []() { for(int i=0; i<11; i++) hyperDrumSynth->setAreaExcitationType(i, drumEx_burst); }; //VIC: [bad] 11 is num of touches + 1 [mouse] for USBest Technology SiS
	auto f_oscCello = []() { for(int i=0; i<11; i++) hyperDrumSynth->setAreaExcitationType(i, drumEx_cello); }; //VIC: [bad] 11 is num of touches + 1 [mouse] for USBest Technology SiS
	auto f_oscDrum  = []() { for(int i=0; i<11; i++) hyperDrumSynth->setAreaExcitationType(i, drumEx_drumloop); }; //VIC: [bad] 11 is num of touches + 1 [mouse] for USBest Technology SiS
	auto f_oscDrone = []() { for(int i=0; i<11; i++) hyperDrumSynth->setAreaExcitationType(i, drumEx_drone); }; //VIC: [bad] 11 is num of touches + 1 [mouse] for USBest Technology SiS
	auto f_oscFx    = []() { for(int i=0; i<11; i++) hyperDrumSynth->setAreaExcitationType(i, drumEx_fxloop); }; //VIC: [bad] 11 is num of touches + 1 [mouse] for USBest Technology SiS
	auto f_oscDisco = []() { for(int i=0; i<11; i++) hyperDrumSynth->setAreaExcitationType(i, drumEx_disco); }; //VIC: [bad] 11 is num of touches + 1 [mouse] for USBest Technology SiS
	auto f_oscJeeg  = []() { for(int i=0; i<11; i++) hyperDrumSynth->setAreaExcitationType(i, drumEx_jeeg); }; //VIC: [bad] 11 is num of touches + 1 [mouse] for USBest Technology SiS
	auto f_oscImp   = []() { for(int i=0; i<11; i++) hyperDrumSynth->setAreaExcitationType(i, drumEx_windowedImp); }; //VIC: [bad] 11 is num of touches + 1 [mouse] for USBest Technology SiS

	// buttons for sample selection, top left of monome, ordered going rightwards and then downwards
	sampleBtn[0].init(7, 0, &oscClient, f_sin);
	sampleBtn[1].init(7, 1, &oscClient, f_square);
	sampleBtn[2].init(7, 2, &oscClient, f_noise);
	sampleBtn[3].init(6, 0, &oscClient, f_oscBank);
	sampleBtn[4].init(6, 1, &oscClient, f_oscBurst);
	sampleBtn[5].init(6, 2, &oscClient, f_oscCello);
	sampleBtn[6].init(5, 0, &oscClient, f_oscDrum);
	sampleBtn[7].init(5, 1, &oscClient, f_oscDrone);
	sampleBtn[8].init(5, 2, &oscClient, f_oscFx);
	sampleBtn[9].init(4, 0, &oscClient, f_oscDisco);
	sampleBtn[10].init(4, 1, &oscClient, f_oscJeeg);
	sampleBtn[11].init(4, 2, &oscClient, f_oscImp);


	// lambdas that will be called by the option buttons
	auto f_clear = []() { hyperDrumSynth->clearDomain(); };
	auto f_reset = []() { hyperDrumSynth->resetSimulation(); };
	auto f_slow = [](bool slow) { hyperDrumSynth->setSlowMotion(slow); };

	// option buttons, 3 in low left part of monome
	optionBtn[0].init(2, 0, &oscClient, f_clear);
	optionBtn[1].init(2, 1, &oscClient, f_reset);
	optionBtn[2].init(2, 2, &oscClient, f_slow);

	resetMonome();
}

void stopMonomePulse(int btnIndex) {
	drawBtn[btnIndex].setButton(false);
}

void closeMonome() {
	shouldStop = true;
	// lights off
	oscClient.sendMessageNow(oscClient.newMessage.to("/monome/grid/led/all").add(0).end());
}
