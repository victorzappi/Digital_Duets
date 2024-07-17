/*
 * areaFunctions.h
 *
 *  Created on: 2016-06-03
 *      Author: Victor Zappi
 */

#ifndef AREAFUNCTIONS_H_
#define AREAFUNCTIONS_H_

int createStoryVowelTube(float *vowelAreaFunc, float vowelAreaFunc_len, float vowel_spacing, float ds, int domainSize[], float ***domainPixels, int asymm=0, bool nasal=false);
int createFantVowelTube(float *vowelAreaFunc, float vowelAreaFunc_len, float vowel_spacing, float ds, int domainSize[], float ***domainPixels, int asymm=0, bool nasal=false);
int createStraightTube(float length, float width, float ds, int domainSize[], float ***domainPixels, int &widthPixel, bool open=false);



#endif /* AREAFUNCTIONS_H_ */
