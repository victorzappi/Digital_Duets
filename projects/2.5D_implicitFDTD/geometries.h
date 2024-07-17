/*
 * geometries.h
 *
 *  Created on: 2016-06-03
 *      Author: Victor Zappi
 */

#ifndef GEOMETRIES_H_
#define GEOMETRIES_H_

int createStoryVowelTube(float *vowelAreaFunc, float vowelAreaFunc_len, float vowel_spacing, float ds,
						 int domainSize[], float ***domainPixels, float ***widthMapPixels, float &openSpaceWidth, bool openSpace=false);
int createTwoTubesCentric(float ds, int domainSize[], float ***domainPixels, float ***widthMapPixels, bool openSpace=false);
int createSymmTestTube(float ds, int domainSize[], double tubeHeight[], double widthDs[], double *widthProfile[2], float ***domainPixels, float ***widthMapPixels, bool openSpace=false);
int createYzSymmStoryVowelTube(float *vowelAreaFunc, float vowelAreaFunc_len, float vowel_spacing, float ds,
						         int domainSize[], float ***domainPixels, float ***widthMapPixels, bool openEnd, bool rotated=false);

int createStoryVowelTube_symmTest(float *vowelAreaFunc, float vowelAreaFunc_len, float vowel_spacing, float ds, int domainSize[],
								  double refArea, double refheight, double refWidthDs, double *refWidth,
								  float ***domainPixels, float ***widthMapPixels, bool openEnd);

#endif /* GEOMETRIES_H_ */

