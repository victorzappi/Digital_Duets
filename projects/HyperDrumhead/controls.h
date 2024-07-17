/*
 * controls.h
 *
 *  Created on: 2015-10-30
 *      Author: vic
 */

#ifndef CONTROLS_H_
#define CONTROLS_H_

enum motionFinger_mode {motion_none, motion_bound, motion_ex, motion_list, motion_cnt};
enum pressFinger_mode {press_none, press_prop, press_dmp, press_cnt};

void *runControlLoop(void *);
void modulateAreaVolPress(double press, int area=5);
void changePreFingersMotionDynamic(motionFinger_mode mode, bool negated=false);
void endPreFingersMotionDynamic();
void changePreFingersPressureDynamic(pressFinger_mode mode);
void endPreFingersPressureDynamic();
void modulateAreaVolNorm(int area, double in);
void setAreaExcitationVolume(int area, double v);
void setAreaExcitationID(int area, int id);
void setAreaExReleaseNorm(int area, double in);
void setAreaLowPassFilterFreqNorm(int area, double in);
void setAreaDampingFactorNorm(int area, float in);
void setAreaPropagationFactorNorm(int area, float in);
void setAreaListPosNormX(int area, float xPos);
void setAreaListPosNormY(int area, float yPos);
void triggerAreaExcitation(int index);
void setAreaVolNorm(int area, double in);
void dampAreaExcitation(int index);
void openFrame();
void cleanUpControlLoop();

void setMasterVolume(double v, bool exp=false);
void setBgain(float gain);
void holdAreas();
void loadNextPreset();
void loadPrevPreset();
void reloadPreset();

// only for keyboard
void setAreaExcitationID(int id);
void changeAreaIndex(int index);
void cycleFirstPreFingerPressure();
void cyclePreFingerPressureModeUnlock();
void changePostFingerMode(motionFinger_mode mode, bool negated=false);
void cyclePostFingerPressureMode();
void changeFirstPreFingerMotion(motionFinger_mode mode, bool negated=false);
void clearDomain();
void resetSimulation();
void slowMotion(bool slowMo);
void switchShowAreas();
void changeBgain(float deltaB);
void changeAreaDampFactorPressRange(float delta);
void changeAreaDampingFactor(float deltaD/*, bool update=true, int area=-1*/);
void changeAreaPropFactorPressRange(float delta);
void changeAreaPropagationFactor(float deltaP/*, bool update=true, int area=-1*/);
void changeAreaExcitationVolume(double delta);
void changeAreaLowPassFilterFreq(float deltaF);
void savePreset();


#endif /* CONTROLS_H_ */
