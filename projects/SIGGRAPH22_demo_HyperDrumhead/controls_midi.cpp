/*
 * midiControls.cpp
 *
 *  Created on: Oct 6, 2017
 *      Author: vic
 */

/*
 * digitech control 8
 * switch	cc
 *
 * 0		cc 21
 * 1		cc 22
 * 2		cc 23
 * 3		cc 24
 * 4		cc 25
 * up		pc up
 * down		pc down
 * expPedal cc 7
 *
 *  midi channel: 0
 *
 *
 * behringer fcb1010 [programmable pedal]
 * 1		cc 0 & note on 0
 * 2		cc 1 & note on 1
 * 3		cc 2 & note on 2
 * 4		cc 3
 * 5		cc 4
 * 6		cc 5 & note on 5
 * 7		cc 6 & note on 6
 * 8		cc 7 & note on 7
 * 9		cc 8
 * 10		cc 9
 * up		NO! changes pedal bank
 * down		NO! changes pedal bank
 * expPdlA  cc 27
 * expPdlB  cc 28
 *
 * midi channel: 0
 *
 */

#include "Midi.h"
#include "controls.h"
#include "HyperPresetManager.h"

Midi midi;
const char* gMidiPort0 = "/dev/midi1"; //TODO check on name using aplaymidi -l src

void midiMessageCallback(MidiChannelMessage message, void* arg){
	if(arg != NULL) {
		//printf("Message from midi port %s: ", (const char*) arg);
		//printf("channel %d, status byte %d, data byte0 %d, data byte1 %d\n", message.getChannel(), message.getType(), message.getDataByte(0), message.getDataByte(1));
		int chn = message.getChannel();
		int type = message.getType();
		int cc, val, note, vel;

		// midi footswitch mapping
		if(chn==0) {

			if(type==kmmNoteOn){
				note = message.getDataByte(0);
				vel = message.getDataByte(1);

				if(note==0) {
					if(vel == 100){
						changePreFingersPinchDynamic(pinch_prop);//changePreFingersPressureDynamic(press_prop);
					}else{
						//endPreFingersPressureDynamic();
					}
				}
				else if(note==1) {
					if(vel == 100){
						changePreFingersPinchDynamic(pinch_dmp);//changePreFingersPressureDynamic(press_dmp);
					}else{
						//endPreFingersPressureDynamic();
					}
				}
				else if(note==5) {
					if(vel == 100){
						changePreFingersMotionDynamic(motion_bound);
					}else{
						endPreFingersMotionDynamic();
					}
				}
				else if(note==6) {
					if(vel == 100){
						changePreFingersMotionDynamic(motion_bound, true);
					}else{
						endPreFingersMotionDynamic();
					}
				}
				else if(note==7) {
					if(vel == 100){
						changePreFingersMotionDynamic(motion_list);
					}else{
						endPreFingersMotionDynamic();
					}
				}
			}

			if(type==kmmControlChange) {
				cc = message.getDataByte(0);
			/*	if(cc==0) {
					changePreFingersPressureDynamic(press_prop);
				}
				else if(cc==1) {
					changePreFingersPressureDynamic(press_dmp);
				}
				else */if(cc==2) {
					holdAreas();
				}
				else if(cc==3) {
					reloadPreset();
				}
				else if(cc==4) {
					loadPrevPreset();
				}
				/*else if(cc==5) {
					changePreFingersMotionDynamic(motion_bound);
					//printf("Value: %d\n", val);
				}
				else if(cc==6) {
					changePreFingersMotionDynamic(motion_bound, true);
				}
				else if(cc==7) {
					changePreFingersMotionDynamic(motion_list);
				}*/
				else if(cc==8) {
					resetSimulation();
				}
				else if(cc==9) {
					loadNextPreset();
				}
				else if(cc==27) {
					setMasterVolume(message.getDataByte(1)/127.0, true); //VIC still crackly, it seems to jump midi values when pedal is moved slowly...WTF
				}
				else if(cc==28) {
					setBgain(message.getDataByte(1)/127.0);
				}
			}
		/*	else if(type==kmmProgramChange) {
				openFrame();
			}
*/
		}
		// remote control mapping
		else {
			int area = chn-1;

			if(type==kmmControlChange) {
				cc = message.getDataByte(0);
				val = message.getDataByte(1);
				if(cc==0) {
					modulateAreaVolNorm(area,  message.getDataByte(1)/127.0);
				}
				else if(cc==1) {
					setAreaExReleaseNorm(area,  message.getDataByte(1)/127.0);
				}
				else if(cc==2) {
					setAreaLowPassFilterFreqNorm(area, val/127.0);
				}
				else if(cc==3) {
					setAreaDampNorm(area, val/127.0);
				}
				else if(cc==4) {
					setAreaPropNorm(area, val/127.0);
				}
				else if(cc==5) {
					setAreaListPosNormX(area, val/127.0);
				}
				else if(cc==6) {
					setAreaListPosNormY(area, val/127.0);
				}
			}
			else if(type==kmmNoteOn) {
				note = message.getDataByte(0);
				vel = message.getDataByte(1);
				if(note==60) {
					if(vel > 0) {
						setAreaVolNorm(area, vel/127.0);
						triggerAreaExcitation(area);
					}
					else if(vel == 0)
						dampAreaExcitation(area);
				}
			}
			else if(type==kmmNoteOff) {
				dampAreaExcitation(area);
			}

		}
	}
	//message.prettyPrint();
}

void initMidiControls() {
	// set up midi to read and write from midi controller
	if(midi.readFrom(gMidiPort0, 5)==-1) {
		printf("Cannot open MIDI device %s\n", gMidiPort0);
		return;
	}

/*	if(midi.writeTo(gMidiPort0, 2)==-1) {
		printf("Cannot open MIDI device %s\n", gMidiPort0);
		return;
	}*/

	midi.enableParser(true);
	midi.setParserCallback(midiMessageCallback, (void*) gMidiPort0);
}
