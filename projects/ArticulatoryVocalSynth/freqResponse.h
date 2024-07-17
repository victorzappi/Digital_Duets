/*
 * freqResponse.h
 *
 *  Created on: 2016-06-25
 *      Author: Victor Zappi
 */

#ifndef FREQRESPONSE_H_
#define FREQRESPONSE_H_

#include <cstddef> // NULL

void calculateFreqResponse(int numOfFFTbins, int periodSize, float refFromants[]=NULL, int refFormantsNum=0);

#endif /* FREQRESPONSE_H_ */
