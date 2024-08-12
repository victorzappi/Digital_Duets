/*
 * controls.h
 *
 *  Created on: 2015-10-30
 *      Author: vic
 */

#ifndef CONTROLS_H_
#define CONTROLS_H_

#include "TouchControlManager.h"


enum motionFinger_mode {motion_none, motion_bound, motion_ex, motion_list, motion_cnt};
enum pressFinger_mode {press_none, press_prop, press_dmp, press_cnt};
enum pinchFinger_mode {pinch_none, pinch_prop, pinch_dmp, pinch_cnt};


void *runControlLoop(void *);
void modulateAreaVolPress(double press, int area=5);
void changePreFingersMotionDynamic(motionFinger_mode mode, bool negated=false);
void endPreFingersMotionDynamic();
void changePreFingersPressureDynamic(pressFinger_mode mode);
void endPreFingersPressureDynamic();
void changePreFingersPinchDynamic(pinchFinger_mode mode);
void endPreFingersPinchDynamic();
void modulateAreaVolNorm(int area, double in);
void setAreaExcitationVolume(int area, double v);
void setChannelExcitationID(int area, int id);
void setAreaExReleaseNorm(int area, double in);
void setAreaLowPassFilterFreqNorm(int area, double in);
void setAreaDampNorm(int area, float in);
void setAreaPropNorm(int area, float in);
void setAreaListPosNormX(int area, float xPos);
void setAreaListPosNormY(int area, float yPos);
//void triggerAreaExcitation(int index);
void setAreaVolNorm(int area, double in);
//void dampAreaExcitation(int index);
void openFrame();
void cleanUpControlLoop();

void setMasterVolume(double v, bool exp=false);
void setBgain(float gain);
void holdAreas();
void loadPreset(int index);
void loadNextPreset();
void loadPrevPreset();
void reloadPreset();

void changeMousePos(int mouseX, int mouseY);



void setNextControlAssignment(ControlAssignment nextAssignment, bool scheduleStop=true);

// only for keyboard
void setChannelExcitationID(int id);
void changeAreaIndex(int index);
void changeExChannel(int channel);
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
void changeAreaDampPressRange(float delta);
void changeAreaDamp(float deltaD/*, bool update=true, int area=-1*/);
void changeAreaPropPressRange(float delta);
void changeAreaProp(float deltaP/*, bool update=true, int area=-1*/);
void changeAreaExcitationVolume(double delta);
void changeAreaLowPassFilterFreq(float deltaF);
void savePreset();

// only for multitouch
bool isOnWindow(int posx_rel, int posy_rel);

// for remote touch control
void getCellDomainCoords(int *coords);
int getAreaIndex(int coordX, int coordY);
bool modifierActions(int area, int coordX, int coordY);
void handleNewTouch(int currentSlot, int touchedArea, int coord[2]);
void handleTouchDrag(int currentSlot, int touchedArea, int coord[2]);
void handleTouchRelease(int currentSlot);

void handleNewTouch2(int currentSlot, int coords[2]);

#endif /* CONTROLS_H_ */
