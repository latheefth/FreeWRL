/*
 *  Fake a joystick OSC stream.
 *  Describe a sine wave that goes from -0 .... -32767 .... 0 .... 32768 .... 0
 *  On a Speedlink BlackWidow throttle controller this is equivalent to:
 *  Start at neutral,
 *  Push all the way forward on the throttle controller,
 *  Pull all the way back
 *  Push back to neutral.
 *
 *  Compile with gcc -lm -llo joystickOSC.c 
 *  Start freewrl freewrl/tests/18-OSC-jZoom-4.wrl , and then in another window run ./aout
 *
 *  Fragments based on http://liblo.sourceforge.net/examples/example_client.c, which is Copyright (C) 2004 Steve Harris, Uwe Koloska
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>

#include "lo/lo.h"

#define	TWOPI 4.0*asin(1.0)
double theta , smallIncr ;
int jsNum = 1;
int jsAxis = 2 ;
int jsVal ;
char *path  = "/joystick/proportional/axis" ;
char *types = "iii" ;

int main(int argc, char *argv[])
{
	lo_address t = lo_address_new(NULL, "7770");
	sleep(5);
	smallIncr = (TWOPI)/360.0;
	for (theta = TWOPI ; theta >= 0.0 ; theta = theta-smallIncr) {
		int jsVal = (int) round(sin(theta)*32767.0) ;
		if (lo_send(t, path, types, jsNum , jsAxis , jsVal) == -1) {
			printf("OSC error %d: %s\n", lo_address_errno(t), lo_address_errstr(t));
		}
		usleep(40000);
		printf("path=%s , types=%s argslist = %d %d %d\n",path , types , jsNum , jsAxis , jsVal) ;
	}
}
