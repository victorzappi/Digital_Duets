/*
 * HyperPresetManager.h
 *
 *  Created on: Feb 16, 2018
 *      Author: Victor Zappi
 */

#ifndef HYPERPRESETMANAGER_H_
#define HYPERPRESETMANAGER_H_

#include "HyperDrumSynth.h"

//#include <string>
//using namespace std;

class HyperPresetManager {

public:
	HyperPresetManager();
	int init(string presetFolder, HyperDrumSynth *synth);
	hyperDrumheadFrame *loadPreset(int index);
	hyperDrumheadFrame *loadNextPreset();
	hyperDrumheadFrame *loadPrevPreset();
	hyperDrumheadFrame *reloadPreset();
	int savePreset(hyperDrumheadFrame *frm);

private:
	int listFilesInFolder(string dirname);
	bool loadPreset(string name, bool reload=false);
	bool incrementPreset(int &index, int dir=1);

	bool isInited;
	string folder;
	HyperDrumSynth *hyperDrumSynth;
	int numOfPresets;
	int currentPreset;
	hyperDrumheadFrame *currentFrame;
};

#endif /* HYPERPRESETMANAGER_H_ */
