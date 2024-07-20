/*
 * disableMouse.cpp
 *
 *  Created on: May 4, 2017
 *      Author: vic
 */

#include <X11/Xlib.h>
#include <X11/extensions/XInput.h>

#include <cstdlib>  // atoi
#include <cstdio>   // fprintf, sprintf
#include <ctype.h>  // isdigit
#include <string.h> // strcmp, strlen
#include <cstdlib>  // getenv and putenv
#include <string> // strcmp, strlen

// taken from xinput.c, in xinput src
XDeviceInfo*
find_device_info(Display	*display,
		 char		*name,
		 Bool		only_extended)
{
    XDeviceInfo	*devices;
    XDeviceInfo *found = NULL;
    int		loop;
    int		num_devices;
    int		len = strlen(name);
    Bool	is_id = True;
    XID		id = (XID)-1;

    for(loop=0; loop<len; loop++) {
		if (!isdigit(name[loop])) {
			is_id = False;
			break;
		}
    }

    if (is_id) {
    	id = atoi(name);
    }

    devices = XListInputDevices(display, &num_devices);

    for(loop=0; loop<num_devices; loop++) {
		if ((!only_extended || (devices[loop].use >= IsXExtensionDevice)) &&
			((!is_id && strcmp(devices[loop].name, name) == 0) ||
			 (is_id && devices[loop].id == id))) {
			if (found) {
				fprintf(stderr,
						"Warning: There are multiple devices named '%s'.\n"
						"To ensure the correct one is selected, please use "
						"the device ID instead.\n\n", name);
				return NULL;
			} else {
				found = &devices[loop];
			}
		}
    }
    return found;
}


// taken from property.c, in xinput src
static Atom parse_atom(Display *dpy, char *name) {
    Bool is_atom = True;
    int i;

    for (i = 0; name[i] != '\0'; i++) {
        if (!isdigit(name[i])) {
            is_atom = False;
            break;
        }
    }

    if (is_atom)
        return atoi(name);
    else
        return XInternAtom(dpy, name, False);
}

// inspired by do_set_prop_xi1() in property.c, from xinput src
int toggleDevice(std::string inputDeviceName, int toggle) {
	Display *dpy = XOpenDisplay(NULL);
	if (dpy == NULL) {
		fprintf(stderr, "Unable to connect to X server\n");
		return 1;
	}

	// working vars
	XDeviceInfo  *info;
	XDevice      *dev;
	Atom          prop;
	Atom          type;
	int           format;
	unsigned long act_nitems, bytes_after;
	union {
		unsigned char *c;
		short *s;
		long *l;
		Atom *a;
	} data;



	// find device id from name
	char device_id[10]; // very big, excessively big
	int deviceNum = 0;  // to cycle all devices' indices
	bool deviceFound = false;

	do {
		sprintf(device_id, "%d", deviceNum);
		info = find_device_info(dpy, device_id, False);
		if(info) {
			//printf("%d) name: %s\n", deviceNum, info->name);

			// check if this device is the one we're looking for
			std::string deviceName(info->name);
			size_t found = deviceName.find(inputDeviceName);

			// if different
			if(found==std::string::npos) {
				deviceNum++; // try next one
				continue;
			}

			// otherwise it means we found it! exit loop
			deviceFound = true;
			break;
		}
		deviceNum++; // try next one
	}
	while(deviceNum<20);

	if(!deviceFound) {
		fprintf(stderr, "unable to find device '%s'\n", inputDeviceName.c_str());
		return 2;
	}


	// maybe this is pleonastic
	dev = XOpenDevice(dpy, info->id);
	if (!dev) {
		fprintf(stderr, "unable to open device '%s'\n", device_id);
		return 3;
	}

	// define property to change
	char property_name[] = "Device Enabled";
	prop = parse_atom(dpy, property_name);

	// some xinput magic
	if (XGetDeviceProperty(dpy, dev, prop, 0, 0, False, AnyPropertyType, &type,
						   &format, &act_nitems, &bytes_after, &data.c) != Success) {
		fprintf(stderr, "failed to get property type and format for '%s'\n", property_name);
		return 4;
	}
	XFree(data.c);

	// this will be the new value of the property [on (1) or off (0)]
	data.c = (unsigned char *)calloc(1, sizeof(long));
	data.c[0] = toggle;

	// apply new value to property of device, on the chosen display [current]
	XChangeDeviceProperty(dpy, dev, prop, type, format, PropModeReplace, data.c, 1);

	// clean up
	free(data.c);
	XCloseDevice(dpy, dev);

	// this is needed to the actual update of the property!
	XCloseDisplay(dpy);

	return 0;
}


// exposed functions
int enableMouse(std::string inputDeviceName) {
	return toggleDevice(inputDeviceName, 1); // same as terminal: xinput set-prop 9 "Device Enabled" 1
}
int disableMouse(std::string inputDeviceName) {
	return toggleDevice(inputDeviceName, 0); // same as terminal: xinput set-prop 9 "Device Enabled" 0
}

