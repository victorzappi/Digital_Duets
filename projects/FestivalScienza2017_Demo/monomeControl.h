/*
 * monomeControl.h
 *
 *  Created on: Sep 20, 2017
 *      Author: Victor Zappi
 */

#ifndef MONOMECONTROL_H_
#define MONOMECONTROL_H_


void initMonome(float &boundGain, bool &draw_wait, bool &delete_wait, int inputNum);
void stopMonomePulse(int btnIndex);
void closeMonome();


#endif /* MONOMECONTROL_H_ */
