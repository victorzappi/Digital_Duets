/*
 * boundaryDistances.h
 *
 *  Created on: Jun 23, 2017
 *      Author: Victor Zappi
 */

#ifndef DISTANCES_H_
#define DISTANCES_H_

// this shit is all we need to compute and keep track of all the minimum distances between each cell and the boundaries

void dist_init();

void dist_addBound(int x, int y);
void dist_removeBound(int x, int y);
float dist_getBoundDist(int x, int y);
void dist_scanBoundDist();
void dist_resetBoundDist();

void dist_addExcite(int x, int y);
void dist_removeExcite(int x, int y);
float dist_getExciteDist(int x, int y);
void dist_scanExciteDist();
void dist_resetExciteDist();

void dist_cleanup();
#endif /* DISTANCES_H_ */
