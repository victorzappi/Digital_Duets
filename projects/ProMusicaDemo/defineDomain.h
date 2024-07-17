/*
 * defineDomain.h
 *
 *  Created on: 2016-06-06
 *      Author: vic
 */

#ifndef DEFINEDOMAIN_H_
#define DEFINEDOMAIN_H_

// domainPixels always passed by reference since it is dynamically allocated in the functions
void openSpaceExcitation(float ***&domainPixels);
void emptyDomain(float ***&domainPixels);
void singleWall(float ***&domainPixels);
void doubleWall(float ***&domainPixels);
void shrinkingTube(float ***&domainPixels);
void customTube(float ***&domainPixels);
void fantVowel(char vowel, float ***&domainPixels, int asymmetric, bool nasal=false);
void storyVowel(char vowel, float ***&domainPixels, int asymmetric, bool nasal=false);
void countour(string countour_nameAndPath, float ***&domainPixels);

#endif /* DEFINEDOMAIN_H_ */
