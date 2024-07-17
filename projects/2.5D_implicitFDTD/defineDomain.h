/*
 * defineDomain.h
 *
 *  Created on: 2016-06-06
 *      Author: vic
 */

#ifndef DEFINEDOMAIN_H_
#define DEFINEDOMAIN_H_

enum symmTest_tubes {symmTest_tube0, symmTest_tube1, symmTest_tube2, symmTest_tube3};
enum symmTest_type {symmTest_xyz, symmTest_yz, symmTest_z, symmTest_asymm};

// domainPixels always passed by reference since it is dynamically allocated in the functions
void openSpaceExcitation(float ***&domainPixels, float ***&widthMapPixels);
void story08Vowel(char vowel, float ***&domainPixels, float ***&widthMapPixels);
void twoTubesCentric(float ***&domainPixels, float ***&widthMapPixels, int micNum);
void symmTestTubes(int tubeID, float ***&domainPixels, float ***&widthMapPixels, bool rotated=false);
void symmTestStoryVowel(char vowel, int symmetryID, float ***&domainPixels, float ***&widthMapPixels, bool rotated=false);


#endif /* DEFINEDOMAIN_H_ */
