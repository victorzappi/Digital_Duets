#ifndef TOUCH_CONTROL_MANAGER_H_
#define TOUCH_CONTROL_MANAGER_H_

#include <array>
#include <vector>
#include <list>
#include <cstdio> // print debug

using std::vector;
using std::pair;


enum control_type {type_none, type_motion, type_pressure, type_pinch};
enum control_motion {control_motion_none, control_motion_excite, control_motion_listener, control_motion_boundary};
enum control_pressure {control_pressure_none, control_pressure_propagation, control_pressure_damping};
enum control_pinch {control_pinch_none, control_pinch_propagation, control_pinch_damping};

struct ControlAssignment {
    control_type controlType;
    int control;
    bool negated;
};




class TouchControlManager {

public:
    TouchControlManager(int maxTouches, unsigned int exciteChns) : maxNumOfTouches(maxTouches), excitationChannels(exciteChns), 
    touchControlAssignments(maxNumOfTouches), channelExcitationPresent(excitationChannels), channelExcitationCoords(excitationChannels), touch_channelExcitation(excitationChannels), 
    channelExcitation_touch(maxNumOfTouches), touchOnBoundary(maxTouches) {
        resetControlAssignment(nextControlAssignment);

        for (int i=0; i<maxTouches; i++) {
            resetControlAssignment(touchControlAssignments[i]);
            channelExcitation_touch[i] = -1;
            touchOnBoundary[i] = false;
        }

        for (unsigned int i=0; i<excitationChannels; i++) {
            channelExcitationPresent[i] = false;
            channelExcitationCoords[i].first = -1;
            channelExcitationCoords[i].second = -1;
            touch_channelExcitation[i] = -1;
        }
    }    
    void setNextControlAssignment(ControlAssignment nextAssignment, bool scheduleStop=true);
    void getNextControlAssignment(ControlAssignment &nextAssignment);
    bool assignControl(int touch);
    void stopAssignmentWait();
    void scheduleStopAssignmentWait();
    bool getControl(int touch, ControlAssignment &assignment);
    bool removeControl(int touch);
    void storeExcitationCoords(int channel, int coords[]);
    pair<int, int> getExcitationCoords(int channel);
    int findExcitationCoords(pair<int, int> coords);
    bool removeExcitation(int channel);
    int isExcitationPresent(int channel);
    void bindTouchToExcitation(int touch, int channel);
    int getTouchExcitationBinding(int touch);
    int getExcitationTouchBinding(int channel);
    bool unbindTouchFromExcitation(int touch);
    void setTouchOnBoundary(int touch, bool onBoundary);
    bool isTouchOnBoundary(int touch);

private:
    void resetControlAssignment(ControlAssignment &assignment);
    bool hasControl(int touch);

    int maxNumOfTouches = -1;
    unsigned int excitationChannels = 0;
    bool waitingForAssignment = false;
    bool endWaitAfterAssignment = false;
    
    ControlAssignment nextControlAssignment;
    vector<ControlAssignment> touchControlAssignments;
    vector<bool> channelExcitationPresent;
    vector< pair<int, int> > channelExcitationCoords;   
    vector<int> touch_channelExcitation;
    vector<int> channelExcitation_touch;

    vector<bool> touchOnBoundary;
};



inline void TouchControlManager::setNextControlAssignment(ControlAssignment nextAssignment, bool scheduleStop) {
    nextControlAssignment = nextAssignment;
    endWaitAfterAssignment = scheduleStop;
    waitingForAssignment = true;
}

inline void TouchControlManager::getNextControlAssignment(ControlAssignment &nextAssignment) {
    nextAssignment.controlType = nextControlAssignment.controlType;
    nextAssignment.control = nextControlAssignment.control;
    nextAssignment.negated = nextControlAssignment.negated;
}


inline bool TouchControlManager::assignControl(int touch) {
    if(!waitingForAssignment)
        return false;
    
    touchControlAssignments[touch] = nextControlAssignment;

    if(endWaitAfterAssignment) {
        waitingForAssignment = false;
        resetControlAssignment(nextControlAssignment);
    }

    return true;
}

inline void TouchControlManager::stopAssignmentWait() {
    waitingForAssignment = false;
    endWaitAfterAssignment = false;
    resetControlAssignment(nextControlAssignment);
}

inline void TouchControlManager::scheduleStopAssignmentWait() {
    endWaitAfterAssignment = true;
}

inline bool TouchControlManager::getControl(int touch, ControlAssignment &assignment) {
    if(hasControl(touch)) {
        assignment = touchControlAssignments[touch];
        return true;
    }

    return false;
}

inline bool TouchControlManager::removeControl(int touch) {
    if(hasControl(touch)) {
        resetControlAssignment(touchControlAssignments[touch]);
        return true;
    }

    return false;
}

inline void TouchControlManager::storeExcitationCoords(int channel, int coords[]) {
    channelExcitationCoords[channel].first = coords[0];
    channelExcitationCoords[channel].second = coords[1];
    channelExcitationPresent[channel] = true;
}

inline pair<int, int> TouchControlManager::getExcitationCoords(int channel) {
    return channelExcitationCoords[channel];
}

inline int TouchControlManager::findExcitationCoords(pair<int, int> coords) {
    for(unsigned int i=0; i<channelExcitationCoords.size(); i++) {
        if(channelExcitationCoords[i].first == coords.first && channelExcitationCoords[i].second == coords.second)
            return i;
    }

    return -1; // not found
}

inline bool TouchControlManager::removeExcitation(int channel) {
    if(!channelExcitationPresent[channel])
        return false;
    
    channelExcitationCoords[channel].first = -1;
    channelExcitationCoords[channel].second = -1;
    channelExcitationPresent[channel] = false;
    return true;
}

inline int TouchControlManager::isExcitationPresent(int channel) {
    return channelExcitationPresent[channel];
}

inline void TouchControlManager::bindTouchToExcitation(int touch, int channel) {
    channelExcitation_touch[touch] = channel;
    touch_channelExcitation[channel] = touch;
}

inline int TouchControlManager::getTouchExcitationBinding(int touch) {
    return channelExcitation_touch[touch];
}

inline int TouchControlManager::getExcitationTouchBinding(int channel) {
    return touch_channelExcitation[channel];
}

inline bool TouchControlManager::unbindTouchFromExcitation(int touch) {
    if(channelExcitation_touch[touch] == -1)
        return false;

    int channel = channelExcitation_touch[touch];
    touch_channelExcitation[channel] = -1;
    channelExcitation_touch[touch] = -1;

    return true;
}


inline void TouchControlManager::setTouchOnBoundary(int touch, bool onBoundary) {
    touchOnBoundary[touch] = onBoundary;
}

inline bool TouchControlManager::isTouchOnBoundary(int touch) {
    return touchOnBoundary[touch];
}

//-------------------------------------------------------------------------------

inline void TouchControlManager::resetControlAssignment(ControlAssignment &assignment) {
    assignment = {type_none, 0, false};
}

inline bool TouchControlManager::hasControl(int touch) {
    return (touchControlAssignments[touch].controlType != type_none);
}



#endif /* TOUCH_CONTROL_MANAGER_H_ */
