/*
 * main.cpp
 *
 *  Created on: 2017-01-02
 *      Author: Victor Zappi
 */


#include <stdio.h>

// for input reading
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/input.h>

#include <string>
#include <signal.h> // for catching ctrl-c
#include <sys/time.h> // for time test


// disable mouse control
#include "disableMouse.h"




// check with:
// cat /proc/bus/input/devices
// OR simply evtest
#define EVENT_DEVICE "/dev/input/event15"
// then use:
// evtest /dev/input/event#WHATEVER# [from apt-get repo]
// to check the events

//here evtest output for Displax Skin Ultra:
/*Supported events:
  Event type 0 (EV_SYN)
  Event type 1 (EV_KEY)
    Event code 330 (BTN_TOUCH)
  Event type 3 (EV_ABS)
    Event code 0 (ABS_X)
      Value   4114
      Min        0
      Max     8200
      Resolution       9
    Event code 1 (ABS_Y)
      Value   3431
      Min        0
      Max     4650
      Resolution       9
    Event code 24 (ABS_PRESSURE)
      Value      0
      Min        0
      Max      512
    Event code 47 (ABS_MT_SLOT)
      Value      0
      Min        0
      Max       99
    Event code 48 (ABS_MT_TOUCH_MAJOR)
      Value      0
      Min        0
      Max      255
    Event code 49 (ABS_MT_TOUCH_MINOR)
      Value      0
      Min        0
      Max      255
    Event code 52 (ABS_MT_ORIENTATION)
      Value      0
      Min        0
      Max        1
    Event code 53 (ABS_MT_POSITION_X)
      Value      0
      Min        0
      Max     8200
      Resolution       9
    Event code 54 (ABS_MT_POSITION_Y)
      Value      0
      Min        0
      Max     4650
      Resolution       9
    Event code 57 (ABS_MT_TRACKING_ID)
      Value      0
      Min        0
      Max    65535
    Event code 58 (ABS_MT_PRESSURE)
      Value      0
      Min        0
      Max      512
Properties:
  Property type 1 (INPUT_PROP_DIRECT)
*/

// so far it seems that the EV_ABS events are all that we need

#define MAX_NUM_EVENTS 64 // found here http://ozzmaker.com/programming-a-touchscreen-on-the-raspberry-pi/

using namespace std;

string eventType[4] = {"EV_SYN", "EV_KEY", "NIHIL", "EV_ABS"};
string absEventCode[ABS_MT_PRESSURE+1]; // initialized in main for typing convenience (X

bool touchOn[100]  = {false}; // to keep track of new touches and touch removals

int fd = -1; // file handle

// time test
int elapsedTime = -1;
timeval start_;
timeval end_;

// disable mouse control, cos multitouch interfaces generally create another handler [other than eventX] to control mouse
// can look for double mouse entry with: ls /dev/input/ -l
#define MOUSE_INPUT_ID 10 // check with: xinput -list





// this is called by ctrl-c
void my_handler(int s){
   printf("\nCaught signal %d\n",s);
   if(fd != -1)
	   close(fd);

   // enable mouse control
   enableMouse(MOUSE_INPUT_ID);

   exit(1);
}

int main() {
	// let's catch ctrl-c to nicely stop the application
	struct sigaction sigIntHandler;

	sigIntHandler.sa_handler = my_handler;
	sigemptyset(&sigIntHandler.sa_mask);
	sigIntHandler.sa_flags = 0;

	sigaction(SIGINT, &sigIntHandler, NULL);



	// disable mouse control
	disableMouse(MOUSE_INPUT_ID);



	// init ABS events' codes
	absEventCode[ABS_MT_SLOT] = "ABS_MT_SLOT"; // index of tracked touch, from 0 to maximum number supported by device

	absEventCode[ABS_MT_TRACKING_ID] = "ABS_MT_TRACKING_ID"; // id of the tracked touch. -1 when touch is removed

	absEventCode[ABS_MT_TOUCH_MAJOR] = "ABS_MT_TOUCH_MAJOR"; // length of the major axis of the elliptical touch area

	absEventCode[ABS_MT_TOUCH_MINOR] = "ABS_MT_TOUCH_MINOR"; // length of the minor axis of the elliptical touch area. if the contact is circular, this event can be omitted
	// ABS_MT_TOUCH_MINOR not used, cos touch ellipse are always spheres

	absEventCode[ABS_MT_ORIENTATION] = "ABS_MT_ORIENTATION"; // orientation of the touching ellipse.
/*  the value describes a signed quarter of a revolution clockwise around the touch center
    the signed value range is arbitrary, but zero should be returned for an ellipse aligned with
	the Y axis of the surface, a negative value when the ellipse is turned to
	the left, and a positive value when the ellipse is turned to the right
	when completely aligned with the X axis, the range max should be
	returned.

	this even can be omitted if the touch area is circular */
	// ABS_MT_ORIENTATION not used, cos touch ellipse are always spheres

	absEventCode[ABS_MT_POSITION_X] = "ABS_MT_POSITION_X"; // x coordinate of the center of the touching ellipse

	absEventCode[ABS_MT_POSITION_Y] = "ABS_MT_POSITION_Y";  // y coordinate of the center of the touching ellipse

	absEventCode[ABS_MT_PRESSURE] = "ABS_MT_PRESSURE"; // touch pressure, in arbitrary unit

	// rest replicate data for first touch in slot
	// not used
	absEventCode[ABS_X] = "ABS_X";
	absEventCode[ABS_Y] = "ABS_Y";
	absEventCode[ABS_PRESSURE] = "ABS_PRESSURE";

	// general event order is explained here: https://www.kernel.org/doc/Documentation/input/multi-touch-protocol.txt



	struct input_event ev[MAX_NUM_EVENTS]; // where to save event blocks
	char name[256] = "Unknown"; // for the name of the device

	// initially only root has read permission on the file
	// so i did in terminal:
	// sudo chmod ugo+r /dev/input/event18
	// quite drastic lol and temporary
	// SO
	// i simply added my user to the group that owns the event file
/*	if ((getuid ()) != 0) {
		fprintf(stderr, "You are not root! This may not work...\n");
		return EXIT_SUCCESS;
	}*/

	// open the device!
	fd = open(EVENT_DEVICE, O_RDONLY);
	if (fd == -1) {
		fprintf(stderr, "%s is not a valid device\n", EVENT_DEVICE);
		printf("maybe %s does not have read access and you're not root?\n", EVENT_DEVICE);
		return EXIT_FAILURE;
	}

	// print it's name, useful (;
	ioctl(fd, EVIOCGNAME(sizeof(name)), name);
	printf("Reading from:\n");
	printf("device file = %s\n", EVENT_DEVICE);
	printf("device name = %s\n", name);

	const size_t ev_size = sizeof(struct input_event)*MAX_NUM_EVENTS;
	ssize_t size;

	int currentSlot = 0;
	// start endless loop to read events [polling]
	for (;;) {
		//gettimeofday(&start_, NULL);

		size = read(fd, &ev, ev_size); // we could use select() if we were interested in non-blocking read...i am not, TROLOLOLOLOL

		//gettimeofday(&end_, NULL);
		//elapsedTime = ( (end_.tv_sec*1000000+end_.tv_usec) - (start_.tv_sec*1000000+start_.tv_usec) );
		//printf("input time:%d\n", elapsedTime);

		// who cares about this...
/*		if (size < (ssize_t)ev_size) {
			fprintf(stderr, "Error size when reading\n");
			break;
		}*/

		for (unsigned int i = 0;  i <  (size / sizeof(struct input_event)); i++) {
			// let's consider only ABS events
			if(ev[i].type == EV_ABS) {
				// the first MT_SLOT determines what touch the following events will refer to
				if(ev[i].code == ABS_MT_SLOT) // this always arrives first! [except for the very first touch, which is assumed to be 0]
					currentSlot = ev[i].value;
				else if(ev[i].code == ABS_MT_TRACKING_ID) {
					if(ev[i].value != -1) {
						if(!touchOn[currentSlot])
							printf("touch %d is ON\n", currentSlot);
						touchOn[currentSlot] = true;
					}
					else {
						if(touchOn[currentSlot])
							printf("touch %d is OFF\n", currentSlot);
						touchOn[currentSlot] = false;
					}
				}
				else if(ev[i].code == ABS_MT_POSITION_X)
					printf("touch %d X pos: %d\n", currentSlot, ev[i].value);
				else if(ev[i].code == ABS_MT_POSITION_Y)
					printf("touch %d Y pos: %d \n", currentSlot, ev[i].value);
				else if(ev[i].code == ABS_MT_PRESSURE)
					printf("touch %d pressure: %d\n", currentSlot, ev[i].value);
				else if(ev[i].code == ABS_MT_TOUCH_MAJOR)
					printf("touch %d diameter: %d\n", currentSlot, ev[i].value);
				else if(ev[i].code == ABS_MT_TOUCH_MINOR)
					printf("touch %d minor surface axis: %d\n", currentSlot, ev[i].value); //VIC ABS_MT_TOUCH_MINOR is ALWAYS equal to ABS_MT_TOUCH_MAJOR
			}
		}

		//break;

		usleep(1); // could definitely be longer, but whatever...we use read blocking and we final application will run in multithreading
	}

    close(fd);

    printf("bella\n");
	return 0;
}

