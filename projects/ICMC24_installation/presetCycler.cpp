/*
 * presetCycler.cpp
 *
 *  Created on: Jul 3, 2024
 *      Author: Victor Zappi
 */
#include <vector>
#include <random>
#include <cstdio>
#include <thread>
#include <chrono>

#include "controls.h"
#include "priority_utils.h"

using std::vector;
using std::discrete_distribution;

pthread_t presetCyclerThread;
bool presetCyclerShouldStop = false;

// random vars
std::mt19937 gen;
discrete_distribution<> dist;

int currentGroup;
int currentPreset;


// externs from main.cpp
extern int startPresetIdx;
extern int presetCyclerInteval_s;
extern vector<vector<int>> presetCyclerGroups;
extern vector<float> presetCyclerProbabilities;


bool initPresetCycler() {
	if( presetCyclerGroups.size() == 0 ) {
		printf("Warning! Preset cycler disabled because no preset groups have beend defined\n");
		return false;
	}
	if( presetCyclerProbabilities.size() != presetCyclerGroups.size() ) {
		printf("Warning! Preset cycler disabled because the number of probabilities does not match the number of preset groups\n");
		return false;
	}

	// initialize random generator
	std::random_device rd;
	gen.seed(rd());
	dist = discrete_distribution<>( presetCyclerProbabilities.begin(), presetCyclerProbabilities.end() );

	// look for current preset indices in groups
	bool presetFound = false;
	if(startPresetIdx != -1) {
		for(size_t group=0; group<presetCyclerGroups.size(); group++) {
			for(size_t preset=0; preset<presetCyclerGroups[group].size(); preset++) {
				if(presetCyclerGroups[group][preset] == startPresetIdx) {
					currentGroup = group;
					currentPreset = preset;
					presetFound = true;
					break;
				}
			}
			if(presetFound)
				break;
		}
	}
	else {
		// if a starting preset was not selected, choose the last preset in the groups
		// so that a new group and a new preset will be chosen by the cycler
		currentGroup = presetCyclerGroups.size()-1;
		currentPreset = presetCyclerGroups[currentGroup].size()-1;
	}

	return true;
}

void computeNextGroup() {
	int nextGroup;

	do {
		nextGroup = dist(gen);
	}
	while(nextGroup == currentGroup); // we never select the same group twice in a row

	currentGroup = nextGroup;
	currentPreset = 0; // start from the first preset in the group
}

void computeNextPreset() {
	currentPreset++;

	// if there are not more presets in this group, choose another group!
	if( currentPreset >= (int)presetCyclerGroups[currentGroup].size() )
		computeNextGroup();

	int currentPresetIdx = presetCyclerGroups[currentGroup][currentPreset];
	loadPreset(currentPresetIdx);
}

void *presetCyclerLoop(void *) {
	set_priority(50);

	auto interval_ms = std::chrono::milliseconds(presetCyclerInteval_s * 1000);
	auto last_wake_time = std::chrono::steady_clock::now();
	// if a starting preset was not selected, the cycler immediately sets a random one
	if(startPresetIdx == -1)
		last_wake_time -= interval_ms;

	while(!presetCyclerShouldStop) {

		auto now = std::chrono::steady_clock::now();
		if(now - last_wake_time >= interval_ms) {

			computeNextPreset();

			last_wake_time = std::chrono::steady_clock::now();
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(100)); // checks quite often to make thread closure responsive
	}
	return (void *)0;
}

void startPresetCycler() {
	pthread_create(&presetCyclerThread, NULL, presetCyclerLoop, NULL);
}

void stopPresetCycler() {
	presetCyclerShouldStop = true;
	pthread_join(presetCyclerThread, NULL); // wait for thread to finish, just in case
}


