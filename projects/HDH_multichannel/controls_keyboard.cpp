/*
 * controls_keyboard.cpp
 *
 *  Created on: Feb 16, 2018
 *      Author: Victor Zappi
 */

#include <GL/glew.h>    // include GLEW and new version of GL on Windows [luv Windows]
#include <GLFW/glfw3.h> // GLFW helper library

#include "controls.h"


// externs from controls.cpp
extern bool controlsShouldStop;
// fixed modifiers
extern bool modNegated;
extern bool modIndex;
extern bool modBound;
extern bool modExcite;
extern bool modListener;
extern bool modArea;
extern bool lockMouseX;
extern bool lockMouseY;

bool shiftIsPressed     = false;
bool altIsPressed       = false;
bool leftCtrlIsPressed  = false;
bool rightCtrlIsPressed = false;
bool spaceIsPressed = false;
bool zIsPressed = false;
bool xIsPressed = false;
bool cIsPressed = false;
bool vIsPressed = false;


ControlAssignment nextAssignment;

void key_callback(GLFWwindow *, int key, int scancode, int action, int mods) {
	// not used, so we mute warnings
	(void)scancode;
	(void)mods;

	if(action == GLFW_PRESS || action == GLFW_REPEAT) {

		// one shot press
		if(action != GLFW_REPEAT) {
			switch(key) {
				case GLFW_KEY_ESCAPE:
					controlsShouldStop = true;
					break;
				case GLFW_KEY_LEFT_SHIFT:
				case GLFW_KEY_RIGHT_SHIFT:
					shiftIsPressed = true;
					modNegated = true;
					break;
				case GLFW_KEY_LEFT_ALT:
				case GLFW_KEY_RIGHT_ALT:
					altIsPressed = true;
					break;
				case GLFW_KEY_RIGHT_CONTROL:
					rightCtrlIsPressed = true;
					lockMouseY = true;
					break;
				case GLFW_KEY_LEFT_CONTROL:
					leftCtrlIsPressed = true;
					lockMouseX = true;
					break;

				case GLFW_KEY_Z:
					zIsPressed = true;
					modBound = true;
					break;
				case GLFW_KEY_X:
					xIsPressed = true;
					modExcite = true;
					break;
				case GLFW_KEY_C:
					cIsPressed = true;
					modListener = true;
					break;
				case GLFW_KEY_V:
					vIsPressed = true;
					modArea = true;
					break;

				case GLFW_KEY_0:
				case GLFW_KEY_KP_0:
					if(rightCtrlIsPressed)
						setChannelExcitationID(0+10*shiftIsPressed);
					else if(leftCtrlIsPressed)
						changeExChannel(0+10*shiftIsPressed);
					else
						changeAreaIndex(0);
					break;
				case GLFW_KEY_1:
				case GLFW_KEY_KP_1:
					if(rightCtrlIsPressed) 
				    	setChannelExcitationID(1+10*shiftIsPressed);
					else if(leftCtrlIsPressed)
						changeExChannel(1+10*shiftIsPressed);
					else
						changeAreaIndex(1);
					break;
				case GLFW_KEY_2:
				case GLFW_KEY_KP_2:
					if(rightCtrlIsPressed)
						setChannelExcitationID(2+10*shiftIsPressed);
					else if(leftCtrlIsPressed)
						changeExChannel(2+10*shiftIsPressed);
					else
						changeAreaIndex(2);
					break;
				case GLFW_KEY_3:
				case GLFW_KEY_KP_3:
					if(rightCtrlIsPressed)
						setChannelExcitationID(3+10*shiftIsPressed);
					else if(leftCtrlIsPressed)
						changeExChannel(3+10*shiftIsPressed);
					else
						changeAreaIndex(3);
					break;
				case GLFW_KEY_4:
				case GLFW_KEY_KP_4:
					if(rightCtrlIsPressed)
						setChannelExcitationID(4+10*shiftIsPressed);
					else if(leftCtrlIsPressed)
						changeExChannel(4+10*shiftIsPressed);
					else
						changeAreaIndex(4);
					break;
				case GLFW_KEY_5:
				case GLFW_KEY_KP_5:
					if(rightCtrlIsPressed)
						setChannelExcitationID(5+10*shiftIsPressed);
					else if(leftCtrlIsPressed)
						changeExChannel(5+10*shiftIsPressed);
					else
						changeAreaIndex(5);
					break;
				case GLFW_KEY_6:
				case GLFW_KEY_KP_6:
					if(rightCtrlIsPressed)
						setChannelExcitationID(6+10*shiftIsPressed);
					else if(leftCtrlIsPressed)
						changeExChannel(6+10*shiftIsPressed);
					else
						changeAreaIndex(6);
					break;
				case GLFW_KEY_7:
				case GLFW_KEY_KP_7:
					if(rightCtrlIsPressed)
						setChannelExcitationID(7+10*shiftIsPressed);
					else if(leftCtrlIsPressed)
						changeExChannel(7+10*shiftIsPressed);
					else
						changeAreaIndex(7);
					break;
				case GLFW_KEY_8:
				case GLFW_KEY_KP_8:
					if(rightCtrlIsPressed)
						setChannelExcitationID(8+10*shiftIsPressed);
					else if(leftCtrlIsPressed)
						changeExChannel(8+10*shiftIsPressed);
					else
						changeAreaIndex(8);
					break;
				case GLFW_KEY_9:
				case GLFW_KEY_KP_9:
					if(rightCtrlIsPressed)
						setChannelExcitationID(9+10*shiftIsPressed);
					else if(leftCtrlIsPressed)
						changeExChannel(9+10*shiftIsPressed);
					else
						changeAreaIndex(9);
					break;


				// pre finger interaction mode [for continuous control only]
				case GLFW_KEY_A:
					nextAssignment = {type_motion, control_motion_excite, shiftIsPressed};
				 	setNextControlAssignment(nextAssignment, !altIsPressed); // alt does not schedule stop of assignment
				 	break;
				case GLFW_KEY_S:
					if(leftCtrlIsPressed || rightCtrlIsPressed)
						savePreset();
					else {
						nextAssignment = {type_motion, control_motion_listener, shiftIsPressed};
				 		setNextControlAssignment(nextAssignment, !altIsPressed); // alt does not schedule stop of assignment
					}
					break;
				case GLFW_KEY_D:
					if(!altIsPressed)
						changeFirstPreFingerMotion(motion_list);
					else {
						changePreFingersMotionDynamic(motion_list);
						endPreFingersMotionDynamic(); // otherwise all successive touches will be pre fingers
					}
					break;
				case GLFW_KEY_F:
					if(!altIsPressed)
						cycleFirstPreFingerPressure();
					else
						cyclePreFingerPressureModeUnlock();
					break;


				// post finger interaction mode [for continuous control only]
				case GLFW_KEY_Q:
					changePostFingerMode(motion_bound, shiftIsPressed);
					break;
				case GLFW_KEY_W:
					changePostFingerMode(motion_ex, shiftIsPressed);
					break;
				case GLFW_KEY_E:
					changePostFingerMode(motion_list);
					break;
				case GLFW_KEY_R:
					cyclePostFingerPressureMode();
					break;



				// inhibit additional fingers' movement modifier
				case GLFW_KEY_BACKSPACE:
					if(!shiftIsPressed)
						changeFirstPreFingerMotion(motion_none);
					else
						changePostFingerMode(motion_none);
					break;




				case GLFW_KEY_DELETE:
					clearDomain();
					break;

				case GLFW_KEY_ENTER:
					resetSimulation();
					break;

				case GLFW_KEY_SPACE:
					if(leftCtrlIsPressed || rightCtrlIsPressed)
						slowMotion(true);
					else
						spaceIsPressed = true;

					modIndex = true;
					break;

				case GLFW_KEY_TAB:
					switchShowAreas();
					break;

				default:
					break;
			}
		}

		// pressed and kept pressed
		switch(key) {
			case GLFW_KEY_M:
				changeBgain(1/(1.0 + altIsPressed*9));
				break;
			case GLFW_KEY_N:
				changeBgain(-1/(1.0 + altIsPressed*9));
				break;

			case GLFW_KEY_L:
				if(shiftIsPressed)
					 changeAreaDampPressRange(1/(1.0 + altIsPressed*9)); // use delta for pressure-damping modifier
				else
					changeAreaDamp(1/(1.0 + altIsPressed*9));
				break;
			case GLFW_KEY_K:
				if(shiftIsPressed)
					 changeAreaDampPressRange(-1/(1.0 + altIsPressed*9)); // use delta for pressure-damping modifier
				else
					changeAreaDamp(-1/(1.0 + altIsPressed*9));
				break;

			case GLFW_KEY_P:
				if(shiftIsPressed)
					 changeAreaPropPressRange(1/(1.0 + altIsPressed*9)); // use delta for pressure-propagation modifier
				else
					changeAreaProp(1/(1.0 + altIsPressed*9));
				break;
			case GLFW_KEY_O:
				 if(leftCtrlIsPressed || rightCtrlIsPressed)
					 openFrame();
				 else if(shiftIsPressed)
					 changeAreaPropPressRange(-1/(1.0 + altIsPressed*9)); // use delta for pressure-propagation modifier
				 else
					 changeAreaProp(-1/(1.0 + altIsPressed*9));
				break;

			case GLFW_KEY_I:
				changeAreaExcitationVolume(1/(1.0 + altIsPressed*9));
				break;
			case GLFW_KEY_U:
				changeAreaExcitationVolume(-1/(1.0 + altIsPressed*9));
				break;

			case GLFW_KEY_Y:
				changeAreaLowPassFilterFreq(1/(1.0 + altIsPressed*9));
				break;
			case GLFW_KEY_T:
				changeAreaLowPassFilterFreq(-1/(1.0 + altIsPressed*9));
				break;

			case GLFW_KEY_J:
				loadNextPreset();
				break;
			case GLFW_KEY_H:
				loadPrevPreset();
				break;

			case GLFW_KEY_G:
				holdAreas();
				break;

			default:
				break;
		}
	} // release
	else if(action == GLFW_RELEASE) {
		switch(key) {
			case GLFW_KEY_LEFT_SHIFT:
			case GLFW_KEY_RIGHT_SHIFT:
				shiftIsPressed = false;
				modNegated = false;
				break;
			case GLFW_KEY_LEFT_ALT:
			case GLFW_KEY_RIGHT_ALT:
				altIsPressed = false;
				break;
			case GLFW_KEY_RIGHT_CONTROL:
				rightCtrlIsPressed = false;
				lockMouseY = false;
				break;
			case GLFW_KEY_LEFT_CONTROL:
				leftCtrlIsPressed = false;
				lockMouseX = false;
				break;

			// modifiers
			case GLFW_KEY_Z:
				zIsPressed = false;
				modBound = false;
				break;
			case GLFW_KEY_X:
				xIsPressed = false;
				modExcite = false;
				break;
			case GLFW_KEY_C:
				cIsPressed = false;
				modListener = false;
				break;
			case GLFW_KEY_V:
				vIsPressed = false;
				modArea = false;
				break;

			case GLFW_KEY_SPACE:
				slowMotion(false);
				spaceIsPressed = false;
				modIndex = false;
				break;

			default:
				break;
		}
	}
}



