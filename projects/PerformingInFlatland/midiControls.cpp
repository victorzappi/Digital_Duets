/*
 * midiControls.cpp
 *
 *  Created on: 2016-09-18
 *      Author: Victor Zappi
 */


// THE USED MIDI CONTROLLER IS A NOVATION LAUNCH CONTROL XL

#include "midiControls.h"
#include "FlatlandSynth.h"
#include "Midi.h"

extern FlatlandSynth *flatlandSynth;
extern int state;

bool stateChanged = false;

#define LAUNCH_CONTROL_CHANNEL_NUM 8

Midi midi;
extern const char* gMidiPort0;

int tracksMapping[TRACKS_NUM] = {73, 74, 75, 76, 89, 90, 91};       // for leds
bool tracksOn[TRACKS_NUM];
int trksPassThruMapping[TRACKS_NUM] = {41, 42, 43, 44, 57, 58, 59}; // for leds

int excitationsMapping[EXCITATIONS_NUM] = {13, 29, 45, 61, 77};     // for leds


inline void lightButton(int note, bool on) {
	int statusByte = 1<<7;
	int type = kmmNoteOn << 4;
	int channel = LAUNCH_CONTROL_CHANNEL_NUM;
	midi_byte_t bytes[3] = {(midi_byte_t)(statusByte+type+channel), (midi_byte_t)note, (midi_byte_t)(100*on)}; // 100 is green
	midi.writeOutput(bytes, 3);
}

inline void lightKnob(int cc, bool on) {
	int statusByte = 1<<7;
	int type = kmmNoteOn << 4;
	int channel = LAUNCH_CONTROL_CHANNEL_NUM;
	midi_byte_t bytes[3] = {(midi_byte_t)(statusByte+type+channel), (midi_byte_t)cc, (midi_byte_t)(10*on)}; // 10 is red
	midi.writeOutput(bytes, 3);
}

inline void buttonMapping0(int note, bool trigger, int trackN) {
	if(trigger) {
		tracksOn[trackN] = !tracksOn[trackN];
		flatlandSynth->toggleTrack(trackN, tracksOn[trackN]);
		lightButton(note, tracksOn[trackN]);

		// to avoid confusion in frame load
		if( !stateChanged && (note==76) ) {
			state = 1;
			stateChanged = true;
		}
	}
}

inline void buttonMapping1(int note, bool trigger, int trackN) {
	if(trigger) {
		flatlandSynth->setPassThruTrack(trackN);
		for(int i=0; i<TRACKS_NUM; i++) {
			if(i==trackN)
				lightButton(trksPassThruMapping[i], true);
			else
				lightButton(trksPassThruMapping[i], false);
		}
	}
}

void midiNotesMapping(int note, bool trigger) {
	int mapping = -1;
	int trackN  = -1;

	if( (note>=73) && (note<=76) ) {
		trackN = note-73;
		mapping = 0;
	}
	else if( (note>=89) && (note<=91) ) {
		trackN = note-85;
		mapping = 0;
	}
	else if( (note>=41) && (note<=44) ) {
		trackN = note-41;
		mapping = 1;
	}
	else if( (note>=57) && (note<=59) ) {
		trackN = note-53;
		mapping = 1;
	}
	else
		return;


	// we are using the trigger as a toggle
	switch(mapping) {
		case 0:
			buttonMapping0(note, trigger, trackN); // launch track
			break;
		case 1:
			buttonMapping1(note, trigger, trackN); // set track pass thru
			break;
		default:
			break;
	}
}

inline void knobsMapping(int exIndex, double vol) {
	flatlandSynth->setExcitationVol(exIndex, vol);
}

inline void slidersMapping(int trckIndex, double vol) {
	flatlandSynth->setTrackVol(trckIndex, vol);
}

inline void midiCCMapping(int cc, int value) {
	// first 5 knobs
	if( (cc>=13) && (cc<=17) )
		 knobsMapping(cc-13, (double(value))/127.0);
	// first 7 sliders
	else if( (cc>=77) && (cc<=83) ) {
		slidersMapping(cc-77, (double(value))/127.0);
	}
}


inline void midiMapping(MidiChannelMessage message) {
	if(message.getType() <= kmmNoteOn)
		midiNotesMapping(message.getDataByte(0), (message.getDataByte(1)==127));
	else if(message.getType() == kmmControlChange)
		midiCCMapping(message.getDataByte(0), message.getDataByte(1));
}

void midiMessageCallback(MidiChannelMessage message, void* arg){
	if(arg != NULL){
		//printf("Message from midi port %s ", (const char*) arg);
		//midi_byte_t bytes[3] = {152, 73, 127};
		//midi_byte_t bytes[3] = {(midi_byte_t)((1<<7)+(message.getType()<<4)+message.getChannel()), message.getDataByte(0), message.getDataByte(1)};
		//midi.writeOutput(bytes, 3);
		midiMapping(message);
	}
	//message.prettyPrint();
}

void initMidiControls() {

	// set up midi to read and write from midi controller
	if(midi.readFrom(gMidiPort0, 1)==-1) {
		printf("Cannot open MIDI device %s\n", gMidiPort0);
		return;
	}

	if(midi.writeTo(gMidiPort0, 2)==-1) {
		printf("Cannot open MIDI device %s\n", gMidiPort0);
		return;
	}
	midi.enableParser(true);
	midi.setParserCallback(midiMessageCallback, (void*) gMidiPort0);

	for(int i=0; i<150; i++)
		lightKnob(i, false);

	// initialize midi buttons on controller according to starting setup of synth
	int passThruTrack = flatlandSynth->getPassThruTrack();
	for(int i=0; i<TRACKS_NUM; i++) {
		// which track is on
		tracksOn[i] = flatlandSynth->getTrackStatus(i); // we have to save the status of each track to enable toggling behavior [if ON > OFF and if OFF > ON]
		lightButton(tracksMapping[i], tracksOn[i]);

		// which track is in passed thru
		if(i==passThruTrack)
			lightButton(trksPassThruMapping[i], true);
		else
			lightButton(trksPassThruMapping[i], false);
	}



	//13
	//29
	//45
	//61
	//77
	for(int i=0; i<EXCITATIONS_NUM; i++)
		lightKnob(excitationsMapping[i], true);
}

