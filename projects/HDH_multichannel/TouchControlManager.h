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
    touchControlAssignments(maxNumOfTouches), channelExcitationPresent(excitationChannels), channelExcitationCoords(excitationChannels), 
    touch_channelExcitation(excitationChannels), channelExcitation_touch(maxNumOfTouches), channelExcitatioAmp(excitationChannels), 
    channelListenerCoords(excitationChannels), touch_channelListener(excitationChannels), channelListener_touch(maxTouches), 
    boundaryTouches(maxTouches), boundaryTouchCoords(maxTouches), touchOnBoundary(maxTouches) {
        resetControlAssignment(nextControlAssignment);

        for (int i=0; i<maxTouches; i++) {
            resetControlAssignment(touchControlAssignments[i]);
            channelExcitation_touch[i] = -1;
            channelListener_touch[i] = -1;
            boundaryTouches[i] = false;
            boundaryTouchCoords[i].first = -1;
            boundaryTouchCoords[i].second = -1;
            touchOnBoundary[i] = false;
        }

        for (unsigned int i=0; i<excitationChannels; i++) {
            channelExcitationPresent[i] = false;
            channelExcitationCoords[i].first = -1;
            channelExcitationCoords[i].second = -1;
            touch_channelExcitation[i] = -1;
            channelExcitatioAmp[i] = 1;
            channelListenerCoords[i].first = -2;
            channelListenerCoords[i].second = -2;
            touch_channelListener[i] = -1;
        }
    }    
    void setNextControlAssignment(ControlAssignment nextAssignment, bool scheduleStop=true);
    void getNextControlAssignment(ControlAssignment &nextAssignment);
    bool assignControl(int touch);
    void stopAssignmentWait();
    void scheduleStopAssignmentWait();
    bool willAssignmentWaitStop();
    bool getControl(int touch, ControlAssignment &assignment);
    bool removeControl(int touch);
    void forceControlAssignment(int touch, ControlAssignment assignment);

    void storeExcitationCoords(int channel, int coords[]);
    pair<int, int> getExcitationCoords(int channel);
    vector< pair<int, int> > getExcitationCoords();
    bool removeExcitation(int channel);
    int isExcitationPresent(int channel);
    void setChannelExcitationAmp(int channel, double amp);
    double getChannelExcitationAmp(int channel);
    
    void bindTouchToExcitation(int touch, int channel);
    int getTouchExcitationBinding(int touch);
    int getExcitationTouchBinding(int channel);
    bool unbindTouchFromExcitation(int touch);

    void storeListenerCoords(int channel, int coords[]);
    pair<int, int> getListenerCoords(int channel);
    vector< pair<int, int> > getListenerCoords();
    void hideListener(int channel);

    void bindTouchToListener(int touch, int channel);
    int getTouchListenerBinding(int touch);
    int getListenerTouchBinding(int channel);
    bool unbindTouchFromListener(int touch);

    bool storeBoundaryTouchCoords(int touch, int coords[]);
    pair<int, int> getBoundaryTouchCoords(int touch);

    void bindTouchToBoundary(int touch);
    bool getTouchBoundaryBinding(int touch);
    bool unbindTouchFromBoundary(int touch);
    
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
    vector<double> channelExcitatioAmp; 

    vector< pair<int, int> > channelListenerCoords;   
    vector<int> touch_channelListener;
    vector<int> channelListener_touch;

    vector<bool> boundaryTouches;
    vector< pair<int, int> > boundaryTouchCoords;

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

inline bool TouchControlManager::willAssignmentWaitStop() {
    return endWaitAfterAssignment;
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

inline void TouchControlManager::forceControlAssignment(int touch, ControlAssignment assignment) {
    touchControlAssignments[touch] = assignment;
}

inline void TouchControlManager::storeExcitationCoords(int channel, int coords[]) {
    channelExcitationCoords[channel].first = coords[0];
    channelExcitationCoords[channel].second = coords[1];
    channelExcitationPresent[channel] = true;
}

inline pair<int, int> TouchControlManager::getExcitationCoords(int channel) {
    return channelExcitationCoords[channel];
}

inline vector< pair<int, int> > TouchControlManager::getExcitationCoords() {
    return channelExcitationCoords;
}

inline bool TouchControlManager::removeExcitation(int channel) {
    if(!channelExcitationPresent[channel])
        return false;

    // remove any outstanding biding with touch
    int boundTouch = touch_channelExcitation[channel];
    if(boundTouch != -1)
        unbindTouchFromExcitation(boundTouch);
    
    channelExcitationCoords[channel].first = -1;
    channelExcitationCoords[channel].second = -1;
    channelExcitationPresent[channel] = false;
    return true;
}

inline int TouchControlManager::isExcitationPresent(int channel) {
    return channelExcitationPresent[channel];
}

inline void TouchControlManager::setChannelExcitationAmp(int channel, double amp) {
    channelExcitatioAmp[channel] = amp;
}

inline double TouchControlManager::getChannelExcitationAmp(int channel) {
    return channelExcitatioAmp[channel];
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

inline void TouchControlManager::storeListenerCoords(int channel, int coords[]) {
    channelListenerCoords[channel].first = coords[0];
    channelListenerCoords[channel].second = coords[1];
}

inline pair<int, int> TouchControlManager::getListenerCoords(int channel) {
    return channelListenerCoords[channel];
}

inline vector< pair<int, int> > TouchControlManager::getListenerCoords() {
    return channelListenerCoords;
}

inline void TouchControlManager::hideListener(int channel) {
    channelListenerCoords[channel].first = -2;
    channelListenerCoords[channel].second = -2;
}


inline void TouchControlManager::bindTouchToListener(int touch, int channel) {
    channelListener_touch[touch] = channel;
    touch_channelListener[channel] = touch;
}

inline int TouchControlManager::getTouchListenerBinding(int touch) {
    return channelListener_touch[touch];
}

inline int TouchControlManager::getListenerTouchBinding(int channel) {
    return touch_channelListener[channel];
}

inline bool TouchControlManager::unbindTouchFromListener(int touch) {
    if(channelListener_touch[touch] == -1)
        return false;

    int channel = channelListener_touch[touch];
    touch_channelListener[channel] = -1;
    channelListener_touch[touch] = -1;

    return true;
}

inline bool TouchControlManager::storeBoundaryTouchCoords(int touch, int coords[]) {
    if(!boundaryTouches[touch])
        return false;

    boundaryTouchCoords[touch].first = coords[0];
    boundaryTouchCoords[touch].second = coords[1];
    return true;
}

inline pair<int, int> TouchControlManager::getBoundaryTouchCoords(int touch) {
    return boundaryTouchCoords[touch];
}

inline void TouchControlManager::bindTouchToBoundary(int touch) {
    boundaryTouches[touch] = true;
}

inline bool TouchControlManager::getTouchBoundaryBinding(int touch) {
    return boundaryTouches[touch];
}

inline bool TouchControlManager::unbindTouchFromBoundary(int touch) {
    if(!boundaryTouches[touch])
        return false;

    boundaryTouches[touch] = false;
    boundaryTouchCoords[touch].first = -1;
    boundaryTouchCoords[touch].second = -1;

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
