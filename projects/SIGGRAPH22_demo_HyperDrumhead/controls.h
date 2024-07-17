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
void setAreaExcitationID(int area, int id);
void setAreaExReleaseNorm(int area, double in);
void setAreaLowPassFilterFreqNorm(int area, double in);
void setAreaDampNorm(int area, float in);
void setAreaPropNorm(int area, float in);
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
void loadPreset(int index);
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
void changeAreaDampPressRange(float delta);
void changeAreaDamp(float deltaD/*, bool update=true, int area=-1*/);
void changeAreaPropPressRange(float delta);
void changeAreaProp(float deltaP/*, bool update=true, int area=-1*/);
void changeAreaExcitationVolume(double delta);
void changeAreaLowPassFilterFreq(float deltaF);
void savePreset();


#endif /* CONTROLS_H_ */
