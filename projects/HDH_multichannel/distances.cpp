/*
 * boundaryDistances.cpp
 *
 *  Created on: Jun 23, 2017
 *      Author: Victor Zappi
 */

// this shit is all we need to compute and keep track of all the minimum distances between each cell and the boundaries

#include <unistd.h>  // usleep
#include <array>
#include <vector>
#include <algorithm> // to handle vectors' elements

#include "distances.h"
#include "HyperDrumSynth.h"

using namespace std;


extern int domainSize[2];
extern HyperDrumSynth *hyperDrumSynth;


// where to save all distances from closest boundary
float **boundDist;
float **startBoundDist; // snapshot of first configuration
// list and num of all boundaries
vector<array<int, 2> > allBoundaries;
vector<array<int, 2> > frameBoundaries; // only the dead cell in the frame
vector<array<int, 2> > startBoundaries; // useful copy of domain's initial boundaries [can overlap with frame boundaries]

float maxBoundDist;

pthread_t boundUptadeThread = 0;


// where to save all distances from closest excitation in current area
float **exciteDist;
// list and num of all excitations [organized in areas, cos excitations strictly belong to areas, while boundaries work as boundaries no matter what area they belong to]
vector<vector<array<int, 2> > > areaCells;       // all domain's cells, structured per area
vector<vector<array<int, 2> > > areaExcitations; // all domain's excitations, structured per area

float maxExciteDist;

pthread_t exciteUptadeThread = 0;
pthread_t exciteUptadeThreads[AREA_NUM] = {0};



// local functions' prototypes
void detectBoundaries(bool init=false);
void * updateBoundDist(void *arg);
void launchUpdateBoundDist(bool init=false);

void detectAreasAndExcitations();
void * updateExciteDist(void *arg);
void * updateAreaExciteDist(void *arg);
void launchUpdateExciteDist(int area=-1);


void dist_init() {
	//---------------------------------------------------------------------
	// boundaries

	// allocate data structures
	boundDist = new float*[domainSize[0]+1];
	for(int i=0; i<domainSize[0]; i++)
		boundDist[i] = new float[domainSize[1]];

	startBoundDist = new float*[domainSize[0]+1];
	for(int i=0; i<domainSize[0]; i++)
		startBoundDist[i] = new float[domainSize[1]];
	// inited in launchUpdateBoundDist()

	// add frame borders
	// interleaving bottom and top borders
	for(int i=-1; i<domainSize[0]+1; i++) {
		array<int, 2> boundTop = {i, -1};
		allBoundaries.push_back(boundTop);
		frameBoundaries.push_back(boundTop); // useful copy of frame boundaries only [to handle preset loading]
		startBoundaries.push_back(boundTop); // useful copy of initial state [to hadnle domain reset]

		array<int, 2> boundBottom = {i, domainSize[1]};
		allBoundaries.push_back(boundBottom);
		frameBoundaries.push_back(boundBottom);
		startBoundaries.push_back(boundBottom);
	}
	// interleaving left and right borders [no angles]
	for(int j=0; j<domainSize[1]; j++) {
		array<int, 2> boundLeft = {-1, j};
		allBoundaries.push_back(boundLeft);
		frameBoundaries.push_back(boundLeft);
		startBoundaries.push_back(boundLeft);

		array<int, 2> boundRight = {domainSize[0], j};
		allBoundaries.push_back(boundRight);
		frameBoundaries.push_back(boundRight);
		startBoundaries.push_back(boundRight);
	}

	// max of all possible min distances between a cell and a boundary
	// it is half of the smallest domain dimension
	maxBoundDist = 1+min(domainSize[0], domainSize[1])/2; // 1+ cos the boundary frame goes beyond the domain

	// finish
	detectBoundaries(true); // look for any other boundary
	launchUpdateBoundDist(true); // compute distances


	//---------------------------------------------------------------------
	// excitations

	// allocate data structure
	exciteDist = new float*[domainSize[0]+1];
	for(int i=0; i<domainSize[0]; i++)
		exciteDist[i] = new float[domainSize[1]];
	// inited in launchUpdateExciteDist()

	for(int i=0; i<AREA_NUM; i++) {
		vector<array<int, 2> > emptyAreaCells;
		areaCells.push_back(emptyAreaCells);

		vector<array<int, 2> > emptyAreaExcite;
		areaExcitations.push_back(emptyAreaExcite);
	}

	// max distance between a cell and the excitation in that area
	// it is the longest straight line in domain...i.e., domain's diagonal
	maxExciteDist = domainSize[0]+domainSize[1];

	// finish
	detectAreasAndExcitations(); // look for all excitations in areas
	launchUpdateExciteDist(); // compute distances
}




//------------------------
// boundary list handling
void dist_addBound(int x, int y) {
	array<int, 2> newBound = {x, y}; // boundary to add to list

	// if it's not there already...
	if(find(allBoundaries.begin(), allBoundaries.end(), newBound) == allBoundaries.end()) {
		allBoundaries.push_back(newBound); // add it
		launchUpdateBoundDist(); // update distances
	}
}
void dist_removeBound(int x, int y) {
	array<int, 2> bound = {x, y}; // boundary to remove from list
	vector<array<int, 2> >::iterator pos = find(allBoundaries.begin(), allBoundaries.end(), bound); // search for it

	// if it's there...
	if( pos != allBoundaries.end() ) {
		allBoundaries.erase(pos); // remove it
		launchUpdateBoundDist();  // update distances
	}
}

float dist_getBoundDist(int x, int y) {
	// handle frame separately
	if(x<0 || y<0 || x>=domainSize[0] || y>=domainSize[1])
		return 0;

	return boundDist[x][y];
}

// recomputes distances from scratch [after we open preset]
void dist_scanBoundDist() {
	// empty boundaries and copy the ones from the frame only [starting point]
	allBoundaries.clear();
	allBoundaries.insert(begin(allBoundaries), begin(frameBoundaries), end(frameBoundaries)); // put from beginning of allBoundaries whole content of startBoundaries

	detectBoundaries();      // look for any other boundaries
	launchUpdateBoundDist(); // finally compute all distances
}

// back to starting state [useful when we reset domain]
void dist_resetBoundDist() {
	// empty boundaries and copy the initial ones
	allBoundaries.clear();
	allBoundaries.insert(begin(allBoundaries), begin(startBoundaries), end(startBoundaries)); // put from beginning of allBoundaries whole content of startBoundaries

	// copy initial distances
	for(int j=0; j< domainSize[1]; j++) {
		for(int i=0; i<domainSize[0]; i++) {
			boundDist[i][j] = startBoundDist[i][j];
		}
	}
}


//------------------------
// excite list handling
void dist_addExcite(int x, int y) {
	array<int, 2> newExcite = {x, y}; // excitation to add to list
	int area = hyperDrumSynth->getCellArea(x, y);

	// do not consider frame cells
	if(area<0 || area>=AREA_NUM)
		return;

	// if it's not there already...
	if(find(areaExcitations[area].begin(), areaExcitations[area].end(), newExcite) == areaExcitations[area].end()) {
		areaExcitations[area].push_back(newExcite); // add it
		launchUpdateExciteDist(area); // update distances
	}
}
void dist_removeExcite(int x, int y) {
	array<int, 2> excite = {x, y}; // boundary to remove from list
	int area = hyperDrumSynth->getCellArea(x, y);

	// do not consider frame cells
	if(area<0 || area>=AREA_NUM)
		return;

	vector<array<int, 2> >::iterator pos = find( areaExcitations[area].begin(), areaExcitations[area].end(), excite); // search for it

	// if it's there...
	if( pos != allBoundaries.end() ) {
		areaExcitations[area].erase(pos); // remove it
		launchUpdateExciteDist(area); // update distances
	}
}

float dist_getExciteDist(int x, int y) {
	// to handle frame move cell to the closest in-domain cell
	if(x<0)
		x = 0;
	if(x>=domainSize[0])
		x = domainSize[0]-1;
	if(y<0)
		y = 0;
	if(y>=domainSize[1])
		y = domainSize[1]-1;

	int area = hyperDrumSynth->getCellArea(x, y);

	// do not consider frame cells
	if(area<0 || area>=AREA_NUM)
		return maxExciteDist;

	return exciteDist[x][y];
}

// recomputes distances from scratch [after we open preset]
void dist_scanExciteDist() {
	dist_resetExciteDist(); // same...cos we doi not have frame and initial snapshot is different from reset domain [no excitations, but areas]
}

// back to starting state [useful when we reset domain]
void dist_resetExciteDist() {
	for(int i=0; i<AREA_NUM; i++) {
		// empty cells
		areaCells[i].clear();

		// empty excitations
		areaExcitations[i].clear();
	}

	detectAreasAndExcitations();
	launchUpdateExciteDist();
}


void dist_cleanup() {
	for(int i=0; i<domainSize[0]; i++)
		delete[] boundDist[i];
	delete[] boundDist;

	for(int i=0; i<domainSize[0]; i++)
		delete[] startBoundDist[i];
	delete[] startBoundDist;

	for(int i=0; i<domainSize[0]; i++)
		delete[] exciteDist[i];
	delete[] exciteDist;
}





//-----------------------------------------------------------------------------------------------

//------------------------
// boundaries

// sweeps all domain
void detectBoundaries(bool init) {
	// for all domain cells...
	for(int j=0; j< domainSize[1]; j++) {
		for(int i=0; i<domainSize[0]; i++) {
			float type = hyperDrumSynth->getCellType(i, j);
			bool isBoundary = (type==HyperDrumhead::cell_boundary)||(type==HyperDrumhead::cell_dead);
			if(isBoundary) {
				array<int, 2> bound = {i, j}; // boundary to remove from list
				allBoundaries.push_back(bound);
				if(init)
					startBoundaries.push_back(bound);
			}
		}
	}
}

// creates a low prio thread
void launchUpdateBoundDist(bool init) {
	struct sched_param param;
	pthread_attr_t attr;
	int ret = pthread_attr_init(&attr);
	if (ret)
		printf("init pthread attributes failed\n");

	ret = pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
	if (ret)
		printf("pthread setschedpolicy failed\n");

	param.sched_priority = 10;
	ret = pthread_attr_setschedparam(&attr, &param);
	if (ret)
		printf("pthread setschedparam failed\n");

	/* Use scheduling parameters of attr */
	ret = pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);

	// kill thread if already running
	if(boundUptadeThread!=0)
		pthread_cancel(boundUptadeThread);

	if(ret)
		printf("pthread setinheritsched failed\n");

	// prepare generic argument...beautiful...
	bool *arg = new bool[1];
	arg[0] = init;

	pthread_create(&boundUptadeThread, &attr, updateBoundDist, arg);
}

// routine
void * updateBoundDist(void *arg) {
	float minDist;
	float dist;

/*	printf("\nbound vector: \n");
	for(unsigned int b=0; b<allBoundaries.size(); b++) {
		printf("[%d][%d]\t\t", allBoundaries[b][0], allBoundaries[b][1]);
	}
	printf("\n\n");*/

	// for all domain cells...
	for(int j=0; j< domainSize[1]; j++) {
		for(int i=0; i<domainSize[0]; i++) {

			minDist = maxBoundDist;
			// check distance with all boundaries
			for(unsigned int b=0; b<allBoundaries.size(); b++) {
				// if this cell is boundary...
				if(allBoundaries[b][0]==i && allBoundaries[b][1]==j) {
					minDist = 0; // distance is 0
					break; // next cell
				}

				// otherwise calculate distance
				dist = abs(i-allBoundaries[b][0]) + abs(j-allBoundaries[b][1]);
				if(dist<minDist)
					minDist = dist; // update minimum
			}
			boundDist[i][j] = minDist / maxBoundDist; // save normilized minimum
			//printf("%f\t\t", boundDist[i][j]);
		}
		//printf("\n");
		usleep(10); /// wait time to time, cos we are not in a rush
	}
	boundUptadeThread = 0;
	//printf("\n");

	// check if this is an init call
	bool init = *((bool *)arg);

	// if it is make a copy of the initial boundDist just computed
	if(init) {
		for(int j=0; j< domainSize[1]; j++) {
			for(int i=0; i<domainSize[0]; i++) {
				startBoundDist[i][j] = boundDist[i][j]; // useful copy of initial state
			}
		}
	}

	delete[] (bool *)arg;

	return NULL;
}




//------------------------
// excitations

// sweeps all domain
void detectAreasAndExcitations() {

	// for all domain cells...
	for(int j=0; j< domainSize[1]; j++) {
		for(int i=0; i<domainSize[0]; i++) {
			int area = hyperDrumSynth->getCellArea(i, j);
			array<int, 2> cell = {i, j}; // boundary to remove from list
			areaCells[area].push_back(cell);

			float type = hyperDrumSynth->getCellType(i, j);
			if(type == HyperDrumhead::cell_excitation)
				areaExcitations[area].push_back(cell);
		}
	}
}

// creates a low prio thread
void launchUpdateExciteDist(int area) {
	struct sched_param param;
	pthread_attr_t attr;
	int ret = pthread_attr_init(&attr);
	if (ret)
		printf("init pthread attributes failed\n");

	ret = pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
	if (ret)
		printf("pthread setschedpolicy failed\n");

	param.sched_priority = 10;
	ret = pthread_attr_setschedparam(&attr, &param);
	if (ret)
		printf("pthread setschedparam failed\n");

	/* Use scheduling parameters of attr */
	ret = pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
	if (ret)
		printf("pthread setinheritsched failed\n");

	// work on separate areas
	if(area>-1) {
		// kill thread for this area if already running
		if(exciteUptadeThreads[area]!=0) {
			pthread_cancel(exciteUptadeThreads[area]);
			//printf("cancelled %lu [area %d]\n", exciteUptadeThreads[area], area);
		}

		// prepare generic argument...beautiful...
		int *arg = new int[1];
		arg[0] = area;

		pthread_create(&exciteUptadeThreads[area], &attr, updateAreaExciteDist, arg);

		//printf("created %lu [area %d]\n", exciteUptadeThreads[area], area);
	}
	else { // work on all areas

		// kill all threads that are already running
		for(int i=0; i<AREA_NUM; i++) {
			if(exciteUptadeThreads[i]!=0)
				pthread_cancel(exciteUptadeThreads[i]);
		}
		if(exciteUptadeThread!=0)
			pthread_cancel(exciteUptadeThread);

		pthread_create(&exciteUptadeThread, &attr, updateExciteDist, NULL);
	}

}

// routine for single area
void * updateAreaExciteDist(void *arg) {
	float minDist;
	float dist;
	int i,j;
	int area = *((int *)arg);

	//VIC security check, cos if thread is killed beforehand it seems that area becomes rubbish
	if(area<0 || area>=AREA_NUM)
		return NULL;

/*	printf("\nexcite area %d vector: \n", area);
	for(unsigned int b=0; b<areaExcitations[area].size(); b++) {
		printf("[%d][%d]\t\t", areaExcitations[area][b][0], areaExcitations[area][b][1]);
	}
	printf("\n\n");*/


	// cycle only cells in this area
	for(unsigned int c=0; c<areaCells[area].size(); c++) {

		minDist = maxExciteDist;
		// coords
		i = areaCells[area][c][0];
		j = areaCells[area][c][1];

		// check distance with all excitations in area
		for(unsigned int b=0; b<areaExcitations[area].size(); b++) {
			if(areaExcitations[area][b][0]==i && areaExcitations[area][b][1]==j) {
				minDist = 0; // distance is 0
				break; // next cell
			}

			// otherwise calculate distance
			dist = abs(i-areaExcitations[area][b][0]) + abs(j-areaExcitations[area][b][1]);
			if(dist<minDist)
				minDist = dist; // update minimum
		}
		exciteDist[i][j] = minDist / maxExciteDist; // save normilized minimum
		//printf("%f\t\t", exciteDist[i][j]);
		usleep(2); /// wait time to time, cos we are not in a rush
	}
	exciteUptadeThreads[area] = 0;
	//printf("done are %d\n", area);

	delete[] (int *)arg;

	return NULL;
}

// routine for all areas
void * updateExciteDist(void *arg) {
	float minDist;
	float dist;
	int area;

/*	for(int a=0; a<AREA_NUM; a++) {
		printf("\nexcite area %d vector: \n", a);
		for(unsigned int b=0; b<areaExcitations[a].size(); b++) {
			printf("[%d][%d]\t\t", areaExcitations[a][b][0], areaExcitations[a][b][1]);
		}
		printf("\n\n");
	}*/

	// for all domain cells...
	for(int j=0; j< domainSize[1]; j++) {
		for(int i=0; i<domainSize[0]; i++) {

			area = hyperDrumSynth->getCellArea(i, j);
			minDist = maxExciteDist;

			// check distance with all excitations in area
			for(unsigned int b=0; b<areaExcitations[area].size(); b++) {
				// if this cell is excitation...
				if(areaExcitations[area][b][0]==i && areaExcitations[area][b][1]==j) {
					minDist = 0; // distance is 0
					break; // next cell
				}

				// otherwise calculate distance
				dist = abs(i-areaExcitations[area][b][0]) + abs(j-areaExcitations[area][b][1]);
				if(dist<minDist)
					minDist = dist; // update minimum

			}
			exciteDist[i][j] = minDist / maxExciteDist; // save normilized minimum
			//printf("%f\t\t", exciteDist[i][j]);
		}
		//printf("\n");
		usleep(10); /// wait time to time, cos we are not in a rush
	}
	exciteUptadeThread = 0;
	//printf("done all areas\n");

	return NULL;
}
