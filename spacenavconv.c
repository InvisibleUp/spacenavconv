// SpaceNav-Conv
// Copyright (C) InvisibleUp 2016, 2019 <invisibleup@outlook.com>
// A utility for converting the output from a SpaceBall 2003 (or compatible) to something more usable by other programs.
// Licensed under the BSD 3-Clause license.
// Based on code from libspacenav from the spacenav project (spacenav.sf.net),
// Copyright (C) 2007-2009 John Tsiombikas <nuclear@member.fsf.org>

#define _POSIX_C_SOURCE 2

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include <signal.h>

#if defined(BUILD_X11)
#include <X11/Xlib.h>
#endif
#include <linux/input.h>
#include <linux/uinput.h>
#include <fcntl.h>
#include <unistd.h>
#include <spnav.h>

#define NUM_AXES 6
#define NUM_BTNS 9
#define UNMAPPED 0xFFFF

enum ControllerType {INVALID, JOYSTICK, RELATIVE, TABLET, MOUSE};
enum AxisMode {REL, ABS};
struct ControllerMappings {
	/* Device settings */
	uint16_t DevBusType;
	uint16_t DevVendor;
	uint16_t DevProduct;
	uint16_t DevVersion;

	/* Axis */
	uint16_t Axis[NUM_AXES];                                  /* Axis mapping */
	int8_t AxisDiv[NUM_AXES];                             /* Strength divider */
	enum AxisMode AxisMode;                         /* Axis mode (REL or ABS) */
	int16_t AxisMin[NUM_AXES];                /* Minimum value of source axis */
	int16_t AxisMax[NUM_AXES];                /* Maximum value of source axis */
	
	/* Button */
	uint16_t Button[NUM_BTNS];                             /* Button mappings */
};

/* X11 context */
#if defined(BUILD_X11)
Display *dpy;
Window win;
unsigned long bpix;
#endif
/* Device context */
int f_uinput = -1;

/* Configurations */
const struct ControllerMappings MAP_JOYSTICK_ABS = {
	.DevBusType = BUS_USB,
	.DevVendor = 0x0123,
	.DevProduct = 0,
	.DevVersion = 4,
	.Axis = {ABS_X, ABS_Y, ABS_Z, ABS_RX, ABS_RY, ABS_RZ},
	.AxisMode = ABS,
	.AxisMin = {-4096, -4096, -4096, -4096, -4096, -4096},
	.AxisMax = {4096, 4096, 4096, 4096, 4096, 4096},
	.AxisDiv = {-1, 1, 1, 1, 1, 1},
	.Button = {BTN_0, BTN_1, BTN_2, BTN_3, BTN_4, BTN_5, BTN_6, BTN_7, BTN_8},
};
const struct ControllerMappings MAP_JOYSTICK_REL = {
	.DevBusType = BUS_USB,
	.DevVendor = 0x0123,
	.DevProduct = 1,
	.DevVersion = 4,
	.Axis = {REL_X, REL_Y, REL_Z, REL_RX, REL_RY, REL_RZ},
	.AxisMode = REL,
	.AxisMin = {-4096, -4096, -4096, -4096, -4096, -4096},
	.AxisMax = {4096, 4096, 4096, 4096, 4096, 4096},
	.AxisDiv = {-1, 1, 1, 1, 1, 1},
	.Button = {BTN_0, BTN_1, BTN_2, BTN_3, BTN_4, BTN_5, BTN_6, BTN_7, BTN_8},
};
const struct ControllerMappings MAP_TABLET = {
	// Tablet isn't detected by anything unless on whitelist.
	// Using values from a Wacom Intuos5.
	.DevBusType = BUS_USB,
	.DevVendor = 0x056A,
	.DevProduct = 0x0027,
	.DevVersion = 0x0110,
	.Axis = {ABS_X, ABS_Y, ABS_PRESSURE, ABS_TILT_Y, ABS_TILT_X, ABS_RZ},
	.AxisMode = ABS,
	.AxisMin = {0, 0, 0, -4096, -4096, -4096},
	// TODO: First two should? be screen size.
	// XDisplayWidth(dpy, DefaultScreen(dpy));
	// XDisplayHeight(dpy, DefaultScreen(dpy));
	.AxisMax = {4096, 4096, 4096, 4096, 4096, 4096},
	.AxisDiv = {-1, 1, 1, 1, 1, 1},
	.Button = {
		BTN_TOUCH, BTN_TOOL_PEN, BTN_TOOL_RUBBER, BTN_TOOL_BRUSH, BTN_0, BTN_1,
		BTN_2, BTN_3, BTN_TOOL_MOUSE
	},
};
const struct ControllerMappings MAP_MOUSE = {
	.DevBusType = BUS_USB,
	.DevVendor = 0x0123,
	.DevProduct = 3,
	.DevVersion = 4,
	.Axis = {REL_X, REL_Y, UNMAPPED, REL_HWHEEL, REL_WHEEL, UNMAPPED},
	.AxisMode = REL,
	.AxisDiv = {-10, 10, 0, 10, 10, 0},
	.AxisMin = {-4096, -4096, 0, -4096, -4096, 0},
	.AxisMax = {4096, 4096, 0, 4096, 4096, 0},
	.Button = {
		BTN_LEFT, UNMAPPED, BTN_5, BTN_6, BTN_7, BTN_8, BTN_9, BTN_MIDDLE, BTN_RIGHT
	}
};

enum ControllerType type_from_string(char *option)
{
	if (strcmp(option, "joystick") == 0) {
		return JOYSTICK;
	} else if (strcmp(option, "relative") == 0) {
		return RELATIVE;
	} else if (strcmp(option, "tablet") == 0) {
		return TABLET;
	} else if (strcmp(option, "mouse") == 0) {
		return MOUSE;
	} else {
		return INVALID;
	}
}

bool set_mappings(enum ControllerType type, struct ControllerMappings *outmap)
{
	const struct ControllerMappings *src;
	
	switch (type) {
	case JOYSTICK: src = &MAP_JOYSTICK_ABS; break;
	case RELATIVE: src = &MAP_JOYSTICK_REL; break;
	case TABLET: src = &MAP_TABLET; break;
	case MOUSE: src = &MAP_MOUSE; break;
	default: return false;
	}
	
	memcpy(outmap, src, sizeof(struct ControllerMappings));
	return true;
}

bool apply_mappings(
	struct uinput_user_dev *uidev,
	struct ControllerMappings *inmap
) {
	uint8_t i;
	
	if (inmap == NULL || uidev == NULL) { return false; }
	
	/* Set event types */
	ioctl(f_uinput, UI_SET_EVBIT, EV_SYN);
	ioctl(f_uinput, UI_SET_EVBIT, EV_KEY); /* button */
	ioctl(f_uinput, UI_SET_EVBIT, (inmap->AxisMode == REL) ? EV_REL : EV_ABS);
	
	/* Set buttons */
	for (i = 0; i < NUM_BTNS; i++) {
		const uint16_t btn = inmap->Button[i];
		
		if (btn == UNMAPPED) {continue;}
		
		ioctl(f_uinput, UI_SET_KEYBIT, btn);
	}
	
	/* Set axes */
	const uint32_t axis_mode = (inmap->AxisMode == REL) ? UI_SET_RELBIT : UI_SET_ABSBIT;
	for (i = 0; i < NUM_AXES; i++) {
		const uint16_t axis = inmap->Axis[i];
		const uint16_t min = inmap->AxisMin[i];
		const uint16_t max = inmap->AxisMax[i];
		
		if (axis == UNMAPPED) {continue;}
		
		ioctl(f_uinput, axis_mode, axis);
		uidev->absmin[axis] = min;
		uidev->absmax[axis] = max;
	}
	
	/* Set fake USB device name */
	snprintf(uidev->name, UINPUT_MAX_NAME_SIZE, "Spaceball 2003");
	uidev->id.bustype = inmap->DevBusType;
	uidev->id.vendor = inmap->DevVendor;
	uidev->id.product = inmap->DevProduct;
	uidev->id.version = inmap->DevVersion;
	
	/* Finally, create the new device */
	write(f_uinput, &uidev, sizeof(uidev));
	ioctl(f_uinput, UI_DEV_CREATE);
	
	return true;
}

void print_help()
{
	puts("SpaceNav-Conv v4");
	puts("Options:");
	puts("t: Change controller type. Required.");
	puts("\tjoystick: 6 axis joystick with all buttons.");
	puts("\trelative: 6 axis joystick with raw relative output.");
	puts("\ttablet: Wacom-like drawing tablet w/ pressure and stroke direction.");
	puts("\tmouse: Standard 3 button mouse.");
	puts("v: Show version and exit.");
	puts("h: Show this screen and exit.");
}

void print_version()
{
	puts("SpaceNav-Conv v4.0");
	puts("Copyright (C) InvisibleUp 2016, 2019 <invisibleup@outlook.com>");
	puts("A utility for converting the output from a SpaceBall 2003 (or compatible) to something more usable by other programs.");
	puts("Licensed under the BSD 3-Clause license.");
	puts("Based on code from libspacenav from the spacenav project (spacenav.sf.net),");
	puts("Copyright (C) 2007-2009 John Tsiombikas <nuclear@member.fsf.org>");
}

/* ^C interrupt handler */
void sig(int s)
{
	if(f_uinput > 0) {
		ioctl(f_uinput, UI_DEV_DESTROY);
		close(f_uinput);
	}
	spnav_close();
	exit(EXIT_SUCCESS);
}

/* Function to connect to spacenavd daemon */
bool spnavd_connect()
{
#if defined(BUILD_X11)
	
	if(!(dpy = XOpenDisplay(0))) {
		fprintf(stderr, "failed to connect to the X server\n");
		return 1;
	}
	bpix = BlackPixel(dpy, DefaultScreen(dpy));
	win = XCreateSimpleWindow(dpy, DefaultRootWindow(dpy), 0, 0, 1, 1, 0, bpix, bpix);
	
	/* This actually registers our window with the driver for receiving *
	 * motion/button events through the 3dxsrv-compatible X11 protocol. */
	if(spnav_x11_open(dpy, win) == -1) {
		fprintf(stderr, "failed to connect to the space navigator daemon\n");
		return false;
	}
	
#elif defined(BUILD_AF_UNIX)
	if(spnav_open() == -1) {
		fprintf(stderr, "failed to connect to the space navigator daemon\n");
		return false;
	}
#else
#error Unknown build type!
#endif
	return true;
}

int main(int argc, char **argv)
{
	enum ControllerType TYPE = INVALID;
	struct ControllerMappings MAPPINGS;
	struct uinput_user_dev uidev;
	struct input_event ev;
	spnav_event sev;
	int c;
	
	memset(&uidev, 0, sizeof(uidev));
	memset(&MAPPINGS, 0, sizeof(MAPPINGS));
	
	/* Open connection to spacenavd */
	if(!spnavd_connect()) {return 1;}

	/* Register ^C interrupt handler */
	signal(SIGINT, sig);

	/* Open uinput device */
	f_uinput = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
	//f_uinput = open("/dev/null", O_WRONLY);
	if(f_uinput < 0){
		fprintf(stderr, "tmp file could not be opened for writing.\n");
		return EXIT_FAILURE;
	}

	/* Parse arguments */
	while((c = getopt(argc, argv, "vt:")) != -1){
		switch(c) {
		case 'v':
			print_version();
			return EXIT_SUCCESS;
		case 't':
			if (optarg == NULL) { break; }
			puts(optarg);
			TYPE = type_from_string(optarg);
			set_mappings(TYPE, &MAPPINGS);
			apply_mappings(&uidev, &MAPPINGS);
			break;
		}
	}
	
	/* Quit and print help if any errors occured */
	/* TODO: Actual error reporting */
	if (TYPE == INVALID) {
		print_help();
		return EXIT_FAILURE;
	}

	/* Main loop */
	while (spnav_wait_event(&sev)) {
		memset(&ev, 0, sizeof(ev));
		
		if (sev.type == SPNAV_EVENT_MOTION) {
			uint8_t axis;
			
			printf(
				"got motion event: t(%d, %d, %d) r(%d, %d, %d)\n",
				sev.motion.x, sev.motion.y, sev.motion.z,
				sev.motion.rx, sev.motion.ry, sev.motion.rz
			);
			
			for (axis = 0; axis < NUM_AXES; axis += 1) {
				uint16_t mapping = MAPPINGS.Axis[axis];
				enum AxisMode mode = MAPPINGS.AxisMode;
				int16_t div = MAPPINGS.AxisDiv[axis];
				int value;
				
				if (mapping == UNMAPPED){
					continue;
				} else {
					 ev.code = mapping;
				}
				
				ev.type = (mode == REL) ? EV_REL : EV_ABS;
				
				switch(axis){
				case 0: value = sev.motion.x; break;
				case 1: value = sev.motion.y; break;
				case 2: value = sev.motion.z; break;
				case 3: value = sev.motion.rx; break;
				case 4: value = sev.motion.ry; break;
				case 5: value = sev.motion.rz; break;
				}
				ev.value = value / div;

				printf("%d\t", ev.value);
				write(f_uinput, &ev, sizeof(ev));
			}


		} else {	/* SPNAV_EVENT_BUTTON */

			struct input_event ev;
			memset(&ev, 0, sizeof(ev));
			
			printf(
				"got button %s event b(%d)\n",
				sev.button.press ? "press" : "release", sev.button.bnum
			);
			
			ev.type = EV_KEY;
			ev.value = sev.button.press ? 1 : 0;

			switch(sev.button.bnum){
			case 0:
			case 1:
			case 2:
			case 3:
			case 4:
			case 5:
			case 6: // 1 - 7
				ev.code = MAPPINGS.Button[sev.button.bnum + 1]; 
				break;
			case 7: // ball
				ev.code = MAPPINGS.Button[0];
				break;
			case 14: // 8 (???)
				ev.code = MAPPINGS.Button[8];
				break;	
			default: // It's nothing! (This should never happen)
				ev.code = -1;
				break;
			}
			
			write(f_uinput, &ev, sizeof(ev));
		}
		
		/* Synchronize */
		memset(&ev, 0, sizeof(struct input_event));
		ev.type = EV_SYN;
		ev.value = SYN_REPORT;
		write(f_uinput, &ev, sizeof(struct input_event));
		spnav_remove_events(SPNAV_EVENT_ANY);
	}

	/* Close devices */
	sig(0);
}
