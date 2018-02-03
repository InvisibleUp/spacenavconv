SpaceNav-Conv v3
----------------

This was a quick and dirty program I wrote a while back so I could use my Spaceball 2003 6DOF input device that I got for $1 from a Goodwill with software on my Linux computer. It should be compatible with other SpaceNav devices, but I haven't tested them. I was going to make this all generic using config files and such, but I never got around to it so I'm just releasing what I have here. It wouldn't be too terribly hard to add support for that, though, as it's set up to be rather flexible.

Note that I made some cleanup edits to the program in this last commit that I can't test right now, so if it doesn't work just try the last commit.

# Supported modes

## Joystick
The SpaceBall's inputs are directly mapped to a joystick, usable by any program which can read a joystick. Button 0 corresponds with the ball and 1-8 are the labeled numbers 1-8. Side note, it's REALLY fun playing Kirby Air Ride like this.

## Relative
Same as Joystick, but the 6 axises are specified as "relative" instead of "absolute". This makes the SpaceBall work more like a mouse, if that's needed.

## Tablet
This emulates a Wacom-like drawing tablet, with pressure sensitivity and 3 degrees of tilt. This, unfortunately, doesn't turn out too terribly well as the SpaceBall isn't very percise for drawing stuff. But it works, in theory.

## Mouse
Emulates a 3-button mouse. Button 0 (on the ball) left clicks, 7 and 8 middle and right click respectively, and 1 toggles scrolling by turning the ball.

# How to run

SpaceNav-Conv requires a Linux system. It also requires that you have spacenavd installed and running. Libspacenav is the only build dependency, so you can simply compile this and run it. You will most likely need to run this as root, as it creats an input device through /dev/uinput, which requires root for writing.
