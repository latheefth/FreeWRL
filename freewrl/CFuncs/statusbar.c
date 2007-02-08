/*
 * Copyright(C) 1998 Tuomas J. Lukka, 2001, 2002 John Stewart. CRC Canada.
 * NO WARRANTY. See the license (the file COPYING in the VRML::Browser
 * distribution) for details.
 */

#include <math.h>

#include "headers.h"
#include "Viewer.h"

#define MYSTUFF "DEF SARAHDUMOULINJOHNSTEWARTSENSOR ProximitySensor { size 1000 1000 1000 } DEF SARAHDUMOULINJOHNSTEWARTHUD Transform { translation 0 0 10 children [ Collision { collide FALSE children [ Transform { translation 0 -0.1 -.2 children [ Shape { geometry Text { fontStyle   FontStyle { justify   \"MIDDLE\" size 0.02 } } } ] } ] } ] } ROUTE SARAHDUMOULINJOHNSTEWARTSENSOR.orientation_changed TO SARAHDUMOULINJOHNSTEWARTHUD.set_rotation ROUTE SARAHDUMOULINJOHNSTEWARTSENSOR.position_changed TO SARAHDUMOULINJOHNSTEWARTHUD.set_translation"
int initialized = FALSE;
uintptr_t retarr[10];
int retsz;
int tmp;

struct X3D_Text *holder = NULL;
struct X3D_Text *lastTextNode = NULL;
struct Uni_String *myline;
/* trigger a update */

void update_status(char* msg) {
	printf("create statusbar if required... msg is now %s\n", msg); 
}

void clear_status() {
	printf ("destroy statusbar node\n");
}


/* render the status bar. If it is required... */ 
void render_status() {
	if (!initialized) {
		inputParse(FROMSTRING, MYSTUFF, FALSE, FALSE, rootNode, offsetof(struct X3D_Group, children), &tmp, FALSE);
		initialized = TRUE;

		/* record the last Text node created, because it is ours! This is easier than
		going through the nodes field by field in the VRML'd string */
		holder = lastTextNode; 


		/* mimic sending in a new string into update_status */
		holder->string.p = malloc (sizeof (struct Uni_String));

		holder->string.p[0] = newASCIIString ("this is my trial status");
		holder->string.n = 1;
		myline = holder->string.p[0];
	
	}
printf ("status bar says %s\n",myline->strptr);
}
 
