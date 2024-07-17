/*
 * HyperPresetManager.cpp
 *
 *  Created on: Feb 16, 2018
 *      Author: Victor Zappi
 */

#include "HyperPresetManager.h"

#include <dirent.h>  // to handle files in dirs


HyperPresetManager::HyperPresetManager() {
	folder = "";
	hyperDrumSynth = NULL;
	numOfPresets = -1;
	currentPreset = -1;
	isInited = false;
	currentFrame = NULL;
}

int HyperPresetManager::init(string presetFolder, HyperDrumSynth *synth) {

	folder = presetFolder;
	hyperDrumSynth = synth;

	if(listFilesInFolder(presetFolder)!=0)
		return 1;

	currentPreset = -1; // remains unset

	isInited = true;

	return 0;
}

hyperDrumheadFrame *HyperPresetManager::loadPreset(int index) {
	if(!isInited) {
		printf("Preset Manager needs to be inited before usage...so init it in the code!\n");
		return NULL;
	}

	if(index>=numOfPresets) {
		printf("Cannot load preset %d, cos only %d [from 0 to %d] were detected in folder %s\n", index, numOfPresets, numOfPresets-1, folder.c_str());
		return NULL;
	}

	if(index<0) {
		printf("Preset Manager cannot load preset %d [it's negative...]\n", index);
		return NULL;
	}

	if(currentFrame!=NULL)
		delete currentFrame;

	string presetName = folder+"/preset"+to_string(index)+".frm";

	if(!loadPreset(presetName))
		return NULL;

	currentPreset = index;

	return currentFrame;
}

hyperDrumheadFrame *HyperPresetManager::loadNextPreset() {
	if(!isInited) {
		printf("Preset Manager needs to be inited before usage...so init it in the code!\n");
		return NULL;
	}

	if(numOfPresets<1) {
		printf("No presets found in preset folder %s\n", folder.c_str());
		return NULL;
	}

	int index = -1;

	if(!incrementPreset(index))
		return NULL;

	currentPreset = index;

	return currentFrame;
}

hyperDrumheadFrame *HyperPresetManager::loadPrevPreset() {
	if(!isInited) {
		printf("Preset Manager needs to be inited before usage...so init it in the code!\n");
		return NULL;
	}

	if(numOfPresets<1) {
		printf("No presets found in preset folder %s\n", folder.c_str());
		return NULL;
	}

	int index = -1;

	if(!incrementPreset(index, -1))
		return NULL;

	currentPreset = index;

	return currentFrame;
}

hyperDrumheadFrame *HyperPresetManager::reloadPreset() {
	if(!isInited) {
		printf("Preset Manager needs to be inited before usage...so init it in the code!\n");
		return NULL;
	}

	if(currentPreset<0) {
		printf("No preset loaded yet\n");
		return NULL;
	}

	string presetName = folder+"/preset"+to_string(currentPreset)+".frm";

	if(!loadPreset(presetName, true))
		return NULL;

	return currentFrame;
}

int HyperPresetManager::savePreset(hyperDrumheadFrame *frm) {
	int index = numOfPresets;

	string presetName = folder+"/preset"+to_string(index)+".frm";

	if(hyperDrumSynth->saveFrame(frm, presetName)!=0)
		return 1;

	numOfPresets = index+1;

	printf("---> Preset %s successfully saved!\n", presetName.c_str());

	return 0;
}


//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
// private methods
//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------
int HyperPresetManager::listFilesInFolder(string dirname) {
		DIR *dir;
		struct dirent *ent;
		string ending = ".frm";


		numOfPresets = 0;

		// Adapted from http://stackoverflow.com/questions/612097/how-can-i-get-a-list-of-files-in-a-directory-using-c-or-c
		if ((dir = opendir (dirname.c_str())) != NULL) {
			/* print all the files and directories within directory */
			while ((ent = readdir (dir)) != NULL) {
				// Ignore dotfiles and . and .. paths
				if(!strncmp(ent->d_name, ".", 1))
					continue;

				//printf("%s\n", ent->d_name);
				string name = string(ent->d_name);

				// count only .frm files
				if(equal(ending.rbegin(), ending.rend(), name.rbegin())) {
					numOfPresets++;
					//printf("--->%s\n", ent->d_name);
				}
			}
			closedir(dir);
		} else {
			/* could not open directory */
			printf("Could not open directory %s!\n", dirname.c_str());
			return -1;
		}

		return 0;
}

bool HyperPresetManager::loadPreset(string name, bool reload) {
	if(hyperDrumSynth->openFrame(name, currentFrame, reload) != 0)
		return false;

	printf("---> Preset %s successfully loaded!\n", name.c_str());

	return true;
}

bool HyperPresetManager::incrementPreset(int &index, int dir) {
	// check boundaries
	index = currentPreset+dir;
	index = (index == -1) ? 0 : index;
	index = (index > numOfPresets-1) ? numOfPresets-1 : index;

	// load only if different from before and positive index [to cope with decrement from -1 starting case]
	if(index == currentPreset || index<0)
		return false;

	string presetName = folder+"/preset"+to_string(index)+".frm";

	if(!loadPreset(presetName))
		return false;

	return true;
}
