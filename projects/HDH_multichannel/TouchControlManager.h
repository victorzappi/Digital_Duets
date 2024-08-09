#ifndef TOUCH_CONTROL_MANAGER_H_
#define TOUCH_CONTROL_MANAGER_H_


#include <vector>
#include <cstdio> // print debug

using std::vector;

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
    TouchControlManager(int maxTouches) : maxNumOfTouches(maxTouches), touchControls(maxNumOfTouches) {
        for (int i=0; i<maxTouches; i++) 
            resetControlAssignment(touchControls[i]);
        resetControlAssignment(nextControlAssignment);
    }    
    void setNextControlAssignment(ControlAssignment nextAssignment, bool scheduleStop=true);
    bool assignControl(int touch);
    void stopAssignmentWait();
    void scheduleStopAssignmentWait();
    bool getControl(int touch, ControlAssignment &assignment);
    bool removeControl(int touch);

private:
    void resetControlAssignment(ControlAssignment &assignment);
    bool hasControl(int touch);

    int maxNumOfTouches = -1;
    bool waitingForAssignment = false;
    bool endWaitAfterAssignment = false;
    
    ControlAssignment nextControlAssignment;

    vector<ControlAssignment> touchControls;
};



inline void TouchControlManager::setNextControlAssignment(ControlAssignment nextAssignment, bool scheduleStop) {
    nextControlAssignment = nextAssignment;
    endWaitAfterAssignment = !scheduleStop;
    waitingForAssignment = true;
}

inline bool TouchControlManager::assignControl(int touch) {
    if(!waitingForAssignment)
        return false;
    
    touchControls[touch] = nextControlAssignment;

    resetControlAssignment(nextControlAssignment);

    if(endWaitAfterAssignment)
        waitingForAssignment = false;

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
        assignment = touchControls[touch];
        return true;
    }

    return false;
}

inline bool TouchControlManager::removeControl(int touch) {
    if(hasControl(touch)) {
        resetControlAssignment(touchControls[touch]);
        return true;
    }

    return false;
}

//-------------------------------------------------------------------------------

inline void TouchControlManager::resetControlAssignment(ControlAssignment &assignment) {
    assignment = {type_none, 0, false};
}

inline bool TouchControlManager::hasControl(int touch) {
    return (touchControls[touch].controlType != type_none);
}



#endif /* TOUCH_CONTROL_MANAGER_H_ */
