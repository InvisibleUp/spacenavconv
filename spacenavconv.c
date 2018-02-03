// SpaceNav-Conv v3
// Copyright (C) InvisibleUp 2016 <invisibleup@outlook.com>
// A utility for converting the output from a SpaceBall 2003 (or compatible) to something more usable by other programs.
// Licensed under the BSD 3-Clause license.
// Based on code from libspacenav from the spacenav project (spacenav.sf.net),
// Copyright (C) 2007-2009 John Tsiombikas <nuclear@member.fsf.org>

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <ctype.h>
#include <X11/Xlib.h>
#include <spnav.h>

#include <linux/input.h>
#include <linux/uinput.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

enum ControllerType {INVALID, JOYSTICK, RELATIVE, TABLET, MOUSE};
enum AxisMode_t {REL, ABS};
enum ButtonMode_t {BTN, TOGGLE};
struct ControllerMappings{
	int Axis[6];
	double AxisMul[6];
	enum AxisMode_t AxisMode[6];
	int AXIS_COUNT;
	
	int Button[9];
	enum ButtonMode_t ButtonMode[9];
	int BTN_COUNT;
};

void sig(int s)
{
	spnav_close();
	exit(0);
}

int main(int argc, char **argv)
{

	double MULT = 1;
	enum ControllerType TYPE = INVALID;
	struct ControllerMappings MAPPINGS;
	struct uinput_user_dev uidev;
	int MODETOGGLE = 0; // Controller-specific
	int f_uinput;
	
	memset(&uidev, 0, sizeof(uidev));
	memset(&MAPPINGS, 0, sizeof(MAPPINGS));

	/* Open uinput device */
	f_uinput = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
	if(f_uinput < 0){
		fprintf(stderr, "/dev/uinput could not be opened for writing.\n");
		return 1;
	}
	
	#if defined(BUILD_X11)
	Display *dpy;
	Window win;
	unsigned long bpix;
	#endif
	
	spnav_event sev;
	signal(SIGINT, sig);
	
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
		return 1;
	}
	
	#elif defined(BUILD_AF_UNIX)
	if(spnav_open()==-1) {
		fprintf(stderr, "failed to connect to the space navigator daemon\n");
		return 1;
	}
	#else
	#error Unknown build type!
	#endif

	snprintf(uidev.name, UINPUT_MAX_NAME_SIZE, "Spaceball 2003");
	uidev.id.bustype = BUS_USB;
	uidev.id.vendor = 0x0123;
	uidev.id.product = 0xCDEF;
	uidev.id.version = 3;
	
	ioctl(f_uinput, UI_SET_EVBIT, EV_SYN);

	if(argc > 1){
		int c;
		while((c = getopt(argc, argv, "tv")) != -1){
			switch(c){

			case 'v':
				puts("SpaceNav-Conv v3");
				puts("Copyright (C) InvisibleUp 2016 <invisibleup@outlook.com>");
				puts("A utility for converting the output from a SpaceBall 2003 (or compatible) to something more usable by other programs.");
				puts("Licensed under the BSD 3-Clause license.");
				puts("Based on code from libspacenav from the spacenav project (spacenav.sf.net),");
				puts("Copyright (C) 2007-2009 John Tsiombikas <nuclear@member.fsf.org>");
			return EXIT_SUCCESS;

			case 't':
				puts(optarg);

				// Init controller
				if(strcmp(optarg, "joystick") == 0){
					TYPE = JOYSTICK;
					ioctl(f_uinput, UI_SET_EVBIT, EV_KEY);
					ioctl(f_uinput, UI_SET_EVBIT, EV_ABS);

					ioctl(f_uinput, UI_SET_KEYBIT, BTN_0);
					ioctl(f_uinput, UI_SET_KEYBIT, BTN_1);
					ioctl(f_uinput, UI_SET_KEYBIT, BTN_2);
					ioctl(f_uinput, UI_SET_KEYBIT, BTN_3);
					ioctl(f_uinput, UI_SET_KEYBIT, BTN_4);
					ioctl(f_uinput, UI_SET_KEYBIT, BTN_5);
					ioctl(f_uinput, UI_SET_KEYBIT, BTN_6);
					ioctl(f_uinput, UI_SET_KEYBIT, BTN_7);
					ioctl(f_uinput, UI_SET_KEYBIT, BTN_8);

					ioctl(f_uinput, UI_SET_ABSBIT, ABS_X);
					ioctl(f_uinput, UI_SET_ABSBIT, ABS_Y);
					ioctl(f_uinput, UI_SET_ABSBIT, ABS_Z);
					ioctl(f_uinput, UI_SET_ABSBIT, ABS_RX);
					ioctl(f_uinput, UI_SET_ABSBIT, ABS_RY);
					ioctl(f_uinput, UI_SET_ABSBIT, ABS_RZ);
					
					MAPPINGS.Axis[0] = ABS_X;
					MAPPINGS.Axis[1] = ABS_Y;
					MAPPINGS.Axis[2] = ABS_Z;
					MAPPINGS.Axis[3] = ABS_RX;
					MAPPINGS.Axis[4] = ABS_RY;
					MAPPINGS.Axis[5] = ABS_RZ;
					MAPPINGS.AXIS_COUNT = 6;
					
					MAPPINGS.AxisMode[0] = ABS;
					MAPPINGS.AxisMode[1] = ABS;
					MAPPINGS.AxisMode[2] = ABS;
					MAPPINGS.AxisMode[3] = ABS;
					MAPPINGS.AxisMode[4] = ABS;
					MAPPINGS.AxisMode[5] = ABS;
					
					MAPPINGS.AxisMul[0] = -1;
					MAPPINGS.AxisMul[1] = 1;
					MAPPINGS.AxisMul[2] = 1;
					MAPPINGS.AxisMul[3] = 1;
					MAPPINGS.AxisMul[4] = 1;
					MAPPINGS.AxisMul[5] = 1;
					
					MAPPINGS.Button[0] = BTN_0;
					MAPPINGS.Button[1] = BTN_1;
					MAPPINGS.Button[2] = BTN_2;
					MAPPINGS.Button[3] = BTN_3;
					MAPPINGS.Button[4] = BTN_4;
					MAPPINGS.Button[5] = BTN_5;
					MAPPINGS.Button[6] = BTN_6;
					MAPPINGS.Button[7] = BTN_7;
					MAPPINGS.Button[8] = BTN_8;
					MAPPINGS.BTN_COUNT = 9;
					
					MAPPINGS.ButtonMode[0] = BTN;
					MAPPINGS.ButtonMode[1] = BTN;
					MAPPINGS.ButtonMode[2] = BTN;
					MAPPINGS.ButtonMode[3] = BTN;
					MAPPINGS.ButtonMode[4] = BTN;
					MAPPINGS.ButtonMode[5] = BTN;
					MAPPINGS.ButtonMode[6] = BTN;
					MAPPINGS.ButtonMode[7] = BTN;
					MAPPINGS.ButtonMode[8] = BTN;
					
					uidev.absmin[ABS_X] = -4096;
					uidev.absmax[ABS_X] = 4096;
					uidev.absmin[ABS_Y] = -4096;
					uidev.absmax[ABS_Y] = 4096;
					uidev.absmin[ABS_Z] = -4096;
					uidev.absmax[ABS_Z] = 4096;
					uidev.absmin[ABS_RX] = -4096;
					uidev.absmax[ABS_RX] = 4096;
					uidev.absmin[ABS_RY] = -4096;
					uidev.absmax[ABS_RY] = 4096;
					uidev.absmin[ABS_RZ] = -4096;
					uidev.absmax[ABS_RZ] = 4096;

				} else if (strcmp(optarg, "relative") == 0){
					TYPE = RELATIVE;
					ioctl(f_uinput, UI_SET_EVBIT, EV_KEY);
					ioctl(f_uinput, UI_SET_EVBIT, EV_REL);
					
					ioctl(f_uinput, UI_SET_KEYBIT, BTN_0); //Ball
					ioctl(f_uinput, UI_SET_KEYBIT, BTN_1);
					ioctl(f_uinput, UI_SET_KEYBIT, BTN_2);
					ioctl(f_uinput, UI_SET_KEYBIT, BTN_3);
					ioctl(f_uinput, UI_SET_KEYBIT, BTN_4);
					ioctl(f_uinput, UI_SET_KEYBIT, BTN_5);
					ioctl(f_uinput, UI_SET_KEYBIT, BTN_6);
					ioctl(f_uinput, UI_SET_KEYBIT, BTN_7);
					ioctl(f_uinput, UI_SET_KEYBIT, BTN_8);
					
					ioctl(f_uinput, UI_SET_RELBIT, REL_X);
					ioctl(f_uinput, UI_SET_RELBIT, REL_Y);
					ioctl(f_uinput, UI_SET_RELBIT, REL_Z);
					ioctl(f_uinput, UI_SET_RELBIT, REL_RX);
					ioctl(f_uinput, UI_SET_RELBIT, REL_RY);
					ioctl(f_uinput, UI_SET_RELBIT, REL_RZ);
					
					MAPPINGS.Axis[0] = REL_X;
					MAPPINGS.Axis[1] = REL_Y;
					MAPPINGS.Axis[2] = REL_Z;
					MAPPINGS.Axis[3] = REL_RX;
					MAPPINGS.Axis[4] = REL_RY;
					MAPPINGS.Axis[5] = REL_RZ;
					MAPPINGS.AXIS_COUNT = 6;
					
					MAPPINGS.AxisMul[0] = -1;
					MAPPINGS.AxisMul[1] = 1;
					MAPPINGS.AxisMul[2] = 1;
					MAPPINGS.AxisMul[3] = 1;
					MAPPINGS.AxisMul[4] = 1;
					MAPPINGS.AxisMul[5] = 1;
					
					MAPPINGS.Button[0] = BTN_0;
					MAPPINGS.Button[1] = BTN_1;
					MAPPINGS.Button[2] = BTN_2;
					MAPPINGS.Button[3] = BTN_3;
					MAPPINGS.Button[4] = BTN_4;
					MAPPINGS.Button[5] = BTN_5;
					MAPPINGS.Button[6] = BTN_6;
					MAPPINGS.Button[7] = BTN_7;
					MAPPINGS.Button[8] = BTN_8;
					MAPPINGS.BTN_COUNT = 9;
					
					MAPPINGS.ButtonMode[0] = BTN;
					MAPPINGS.ButtonMode[1] = BTN;
					MAPPINGS.ButtonMode[2] = BTN;
					MAPPINGS.ButtonMode[3] = BTN;
					MAPPINGS.ButtonMode[4] = BTN;
					MAPPINGS.ButtonMode[5] = BTN;
					MAPPINGS.ButtonMode[6] = BTN;
					MAPPINGS.ButtonMode[7] = BTN;
					MAPPINGS.ButtonMode[8] = BTN;
					
					uidev.absmin[ABS_X] = -4096;
					uidev.absmax[ABS_X] = 4096;
					uidev.absmin[ABS_Y] = -4096;
					uidev.absmax[ABS_Y] = 4096;
					uidev.absmin[ABS_Z] = -4096;
					uidev.absmax[ABS_Z] = 4096;
					uidev.absmin[ABS_RX] = -4096;
					uidev.absmax[ABS_RX] = 4096;
					uidev.absmin[ABS_RY] = -4096;
					uidev.absmax[ABS_RY] = 4096;
					uidev.absmin[ABS_RZ] = -4096;
					uidev.absmax[ABS_RZ] = 4096;
					
				} else if (strcmp(optarg, "tablet") == 0){
					TYPE = TABLET;

					// Tablet isn't detected by anything unless on whitelist.
					// Using values from a Wacom Intuos5.
					snprintf(uidev.name, UINPUT_MAX_NAME_SIZE, "Wacom Spaceball 2003");
					uidev.id.bustype = BUS_USB;
					uidev.id.vendor = 0x056a;
					uidev.id.product = 0x0027;
					uidev.id.version = 0x110;

					ioctl(f_uinput, UI_SET_EVBIT, EV_KEY);
					ioctl(f_uinput, UI_SET_EVBIT, EV_ABS);
					ioctl(f_uinput, UI_SET_EVBIT, EV_REL);
					
					ioctl(f_uinput, UI_SET_KEYBIT, BTN_TOUCH); //Ball (or Auto)
					ioctl(f_uinput, UI_SET_KEYBIT, BTN_TOOL_PEN); //1
					ioctl(f_uinput, UI_SET_KEYBIT, BTN_TOOL_RUBBER); //2
					ioctl(f_uinput, UI_SET_KEYBIT, BTN_TOOL_BRUSH); //3
					ioctl(f_uinput, UI_SET_KEYBIT, BTN_0); //4
					ioctl(f_uinput, UI_SET_KEYBIT, BTN_1); //5
					ioctl(f_uinput, UI_SET_KEYBIT, BTN_2); //6
					ioctl(f_uinput, UI_SET_KEYBIT, BTN_3); //7
					ioctl(f_uinput, UI_SET_KEYBIT, BTN_TOOL_MOUSE); //8
					
					ioctl(f_uinput, UI_SET_ABSBIT, ABS_X);
					ioctl(f_uinput, UI_SET_ABSBIT, ABS_Y);
					ioctl(f_uinput, UI_SET_ABSBIT, ABS_PRESSURE);
					ioctl(f_uinput, UI_SET_ABSBIT, ABS_TILT_X);
					ioctl(f_uinput, UI_SET_ABSBIT, ABS_TILT_Y);
					ioctl(f_uinput, UI_SET_ABSBIT, ABS_RZ);
					
					MAPPINGS.Axis[0] = ABS_X;
					MAPPINGS.Axis[1] = ABS_Y;
					MAPPINGS.Axis[2] = ABS_PRESSURE;
					MAPPINGS.Axis[3] = ABS_TILT_Y;
					MAPPINGS.Axis[4] = ABS_TILT_X;
					MAPPINGS.Axis[5] = ABS_RZ;
					MAPPINGS.AXIS_COUNT = 6;
					
					MAPPINGS.AxisMode[0] = ABS;
					MAPPINGS.AxisMode[1] = ABS;
					MAPPINGS.AxisMode[2] = ABS;
					MAPPINGS.AxisMode[3] = ABS;
					MAPPINGS.AxisMode[4] = ABS;
					MAPPINGS.AxisMode[5] = ABS;
					
					MAPPINGS.AxisMul[0] = -0.1;
					MAPPINGS.AxisMul[1] = 0.1;
					MAPPINGS.AxisMul[2] = 1;
					MAPPINGS.AxisMul[3] = 1;
					MAPPINGS.AxisMul[4] = 1;
					MAPPINGS.AxisMul[5] = 1;
					
					uidev.absmin[ABS_X] = 0;
					uidev.absmax[ABS_X] = XDisplayWidth(dpy, DefaultScreen(dpy));
					uidev.absmin[ABS_Y] = 0;
					uidev.absmax[ABS_Y] = XDisplayHeight(dpy, DefaultScreen(dpy));
					uidev.absmin[ABS_PRESSURE] = -0;
					uidev.absmax[ABS_PRESSURE] = 4096;
					uidev.absmin[ABS_TILT_X] = -4096;
					uidev.absmax[ABS_TILT_X] = 4096;
					uidev.absmin[ABS_TILT_Y] = -4096;
					uidev.absmax[ABS_TILT_Y] = 4096;
					uidev.absmin[ABS_RZ] = -4096;
					uidev.absmax[ABS_RZ] = 4096;
					
					MAPPINGS.Button[0] = BTN_TOUCH;
					MAPPINGS.Button[1] = BTN_TOOL_PEN;
					MAPPINGS.Button[2] = BTN_TOOL_RUBBER;
					MAPPINGS.Button[3] = BTN_TOOL_BRUSH;
					MAPPINGS.Button[4] = BTN_0;
					MAPPINGS.Button[5] = BTN_1;
					MAPPINGS.Button[6] = BTN_2;
					MAPPINGS.Button[7] = BTN_3;
					MAPPINGS.Button[8] = BTN_TOOL_MOUSE;
					MAPPINGS.BTN_COUNT = 9;
					
					MAPPINGS.ButtonMode[0] = BTN;
					MAPPINGS.ButtonMode[1] = BTN;
					MAPPINGS.ButtonMode[2] = BTN;
					MAPPINGS.ButtonMode[3] = BTN;
					MAPPINGS.ButtonMode[4] = BTN;
					MAPPINGS.ButtonMode[5] = BTN;
					MAPPINGS.ButtonMode[6] = BTN;
					MAPPINGS.ButtonMode[7] = BTN;
					MAPPINGS.ButtonMode[8] = BTN;
					
				} else if (strcmp(optarg, "mouse") == 0){
					TYPE = MOUSE;
					
					ioctl(f_uinput, UI_SET_EVBIT, EV_KEY);
					ioctl(f_uinput, UI_SET_EVBIT, EV_REL);
					
					ioctl(f_uinput, UI_SET_KEYBIT, BTN_LEFT);
					ioctl(f_uinput, UI_SET_KEYBIT, BTN_MIDDLE);
					ioctl(f_uinput, UI_SET_KEYBIT, BTN_RIGHT);
					ioctl(f_uinput, UI_SET_KEYBIT, BTN_4);
					ioctl(f_uinput, UI_SET_KEYBIT, BTN_5);
					ioctl(f_uinput, UI_SET_KEYBIT, BTN_6);
					ioctl(f_uinput, UI_SET_KEYBIT, BTN_7);
					ioctl(f_uinput, UI_SET_KEYBIT, BTN_8);
					
					ioctl(f_uinput, UI_SET_RELBIT, REL_X);
					ioctl(f_uinput, UI_SET_RELBIT, REL_Y);
					ioctl(f_uinput, UI_SET_RELBIT, REL_WHEEL);
					ioctl(f_uinput, UI_SET_RELBIT, REL_HWHEEL);
					
					MAPPINGS.Button[0] = BTN_LEFT;
					MAPPINGS.Button[1] = -1;
					MAPPINGS.Button[2] = BTN_5;
					MAPPINGS.Button[3] = BTN_6;
					MAPPINGS.Button[4] = BTN_7;
					MAPPINGS.Button[5] = BTN_8;
					MAPPINGS.Button[6] = BTN_9;
					MAPPINGS.Button[7] = BTN_MIDDLE;
					MAPPINGS.Button[8] = BTN_RIGHT;
					
					MAPPINGS.ButtonMode[0] = BTN; //Ball
					MAPPINGS.ButtonMode[1] = TOGGLE; //1
					MAPPINGS.ButtonMode[2] = BTN;
					MAPPINGS.ButtonMode[3] = BTN;
					MAPPINGS.ButtonMode[4] = BTN;
					MAPPINGS.ButtonMode[5] = BTN;
					MAPPINGS.ButtonMode[6] = BTN;
					MAPPINGS.ButtonMode[7] = BTN;
					MAPPINGS.ButtonMode[8] = BTN;
					
					MAPPINGS.Axis[0] = REL_X;
					MAPPINGS.Axis[1] = REL_Y;
					MAPPINGS.Axis[2] = -1;
					MAPPINGS.Axis[3] = REL_HWHEEL;
					MAPPINGS.Axis[4] = REL_WHEEL;
					MAPPINGS.Axis[5] = -1;
					MAPPINGS.AXIS_COUNT = 4;
					
					MAPPINGS.AxisMul[0] = -0.1;
					MAPPINGS.AxisMul[1] = 0.1;
					MAPPINGS.AxisMul[3] = 0.1;
					MAPPINGS.AxisMul[4] = 0.1;
					
					uidev.absmin[ABS_X] = 0;
					uidev.absmax[ABS_X] = XDisplayWidth(dpy, DefaultScreen(dpy));
					uidev.absmin[ABS_Y] = 0;
					uidev.absmax[ABS_Y] = XDisplayHeight(dpy, DefaultScreen(dpy));
					
					MAPPINGS.AxisMode[0] = REL;
					MAPPINGS.AxisMode[1] = REL;
					MAPPINGS.AxisMode[3] = REL;
					MAPPINGS.AxisMode[4] = REL;
					
					//It's a *really nice* mouse.
				} else {
					fprintf(stderr, "Unknown controller type %s", optarg);
					return EXIT_FAILURE;
				}
			break;
			}
		}
	} else {
		puts("SpaceNav-Conv v3");
		puts("Options:");
		puts("t: Change controller type. Required.");
		puts("\tjoystick: 6 axis joystick with all buttons.");
		puts("\trelative: 6 axis joystick with raw relative output.");
		puts("\ttablet: Wacom-like drawing tablet w/ pressure and stroke direction.");
		puts("\tmouse: Standard 3 button mouse.");
		puts("v: Show version and exit.");
		puts("h: Show this screen and exit.");
		return EXIT_SUCCESS;
	}

	//Finally write out device info
	write(f_uinput, &uidev, sizeof(uidev));
	ioctl(f_uinput, UI_DEV_CREATE);

	/* spnav_wait_event() and spnav_poll_event(), will silently ignore any non-spnav X11 events.
	 *
	 * If you need to handle other X11 events you will have to use a regular XNextEvent() loop,
	 * and pass any ClientMessage events to spnav_x11_event, which will return the event type or
	 * zero if it's not an spnav event (see spnav.h).
	 */
	int oldX, oldY;
	//Center origin on screen
	oldX = XDisplayWidth(dpy, DefaultScreen(dpy));
	oldY = XDisplayHeight(dpy, DefaultScreen(dpy));

	while(spnav_wait_event(&sev)) {

		if(sev.type == SPNAV_EVENT_MOTION) {
			printf("got motion event: t(%d, %d, %d) ", sev.motion.x, sev.motion.y, sev.motion.z);
			printf("r(%d, %d, %d)\n", sev.motion.rx, sev.motion.ry, sev.motion.rz);

			struct input_event ev[MAPPINGS.AXIS_COUNT];
			memset(ev, 0, sizeof(ev));
			
			int AxisNo = 0, a = 0;
			while(a < MAPPINGS.AXIS_COUNT){
				//Axis
				if(MAPPINGS.Axis[AxisNo] == -1){
					AxisNo++;
					continue;
				} else {
					 ev[a].code = MAPPINGS.Axis[AxisNo];
				}
				
				//Axis mode
				if(MAPPINGS.AxisMode[AxisNo] == ABS){
					ev[a].type = EV_ABS;
				} else {
					ev[a].type = EV_REL;
				}
				
				//Axis value
				switch(AxisNo){
				case 0:
					ev[a].value = sev.motion.x;
				break;
				case 1:
					ev[a].value = sev.motion.y;
				break;
				case 2:
					ev[a].value = sev.motion.z;
				break;
				case 3:
					ev[a].value = sev.motion.rx;
				break;
				case 4:
					ev[a].value = sev.motion.ry;
				break;
				case 5:
					ev[a].value = sev.motion.rz;
				break;
				}
				ev[a].value *= MULT * MAPPINGS.AxisMul[AxisNo];

				if(TYPE == TABLET){
					if(AxisNo == 0){
						ev[a].value += oldX;
						oldX = ev[a].value;
					} else if (AxisNo == 1){
						ev[a].value += oldY;
						oldY = ev[a].value;
					} 
					if(AxisNo <= 2 && ev[a].value < 0){
						ev[a].value = 0;
					}
				}
				printf("%d\t", ev[a].value);

				a++;
				AxisNo++;
			}

			write(f_uinput, ev, sizeof(ev));

		} else {	/* SPNAV_EVENT_BUTTON */
			printf("got button %s event b(%d)\n", sev.button.press ? "press" : "release", sev.button.bnum);

			struct input_event ev;
			memset(&ev, 0, sizeof(struct input_event));
			
			//Check for toggle type (eh, later)
			ev.type = EV_KEY;
			ev.value = sev.button.press ? 1 : 0;
			if(ev.value == 0 && sev.button.bnum == 0 && TYPE == TABLET){
				ev.value = 1;
			}			

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
			
			// Button is not toggle
			if(ev.code != -1){
				write(f_uinput, &ev, sizeof(struct input_event));
			}
			
		}
		
		// Synchronize
		{
			struct input_event ev;
			memset(&ev, 0, sizeof(struct input_event));
			ev.type = EV_SYN;
			ev.value = SYN_REPORT;
			write(f_uinput, &ev, sizeof(struct input_event));
		}
		spnav_remove_events(SPNAV_EVENT_ANY);
	}

	ioctl(f_uinput, UI_DEV_DESTROY);
	close(f_uinput);
	spnav_close();
	return 0;
}
