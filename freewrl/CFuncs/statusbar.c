/*
 * Copyright(C) 1998 Tuomas J. Lukka, 2001, 2002 John Stewart. CRC Canada.
 * NO WARRANTY. See the license (the file COPYING in the VRML::Browser
 * distribution) for details.
 */

#ifdef OLDCODE

#include <math.h>

#include "headers.h"
#include "Viewer.h"

#define PROX "ProximitySensor { size 1000 1000 1000 }"
#define TEXT "Transform { translation 0 0 10 children [ Collision { collide FALSE children [ Transform { scale 0.6 1 1 translation 0 -0.1 -.2 children [ Shape { geometry Text { fontStyle   FontStyle { justify   \"MIDDLE\" size 0.02 } } } ] } ] } ] }"

static int sb_initialized = FALSE;

static struct X3D_Text *holder = NULL;
struct X3D_Text *lastTextNode = NULL;
static struct Uni_String *myline;
void render_init(void);
static uintptr_t proxNode = NULL;
static uintptr_t transNode = NULL;
/* trigger a update */

void update_status(char* msg) {
	if (!sb_initialized) {
		render_init();
	}
	sprintf(myline->strptr, "%s", msg);
	myline->len = strlen(msg)+1; /* length of message, plus the null terminator */
	#ifdef VERBOSE
	printf("myline-> strptr is %s, len is %d\n", myline->strptr, myline->len);
	#endif
	update_node((void*) holder);
}

void clear_status() {
	if (!sb_initialized) return;

	sb_initialized = FALSE;

	myline->len = 0;
	printf ("destroy statusbar node\n");
}


/* render the status bar. If it is required... */ 
void statusbar_init() {
	int tmp;
	uintptr_t nodarr[200];
	int ra;

              
        ra = EAI_CreateVrml("String",PROX,nodarr,200);
	if (ra != 2) { printf ("render_init, expected 2 here\n"); return; }
	proxNode = nodarr[1];
	/* printf ("prox node is %d\n",proxNode); */

        ra = EAI_CreateVrml("String",TEXT,nodarr,200);
	if (ra != 2) { printf ("render_init, expected 2 here\n"); return; }
	transNode = nodarr[1];
	/* printf ("trans %d\n",transNode); */
}

void render_init() {
	/* statusbar_init had to have been called... */
	if ((proxNode==NULL) || (transNode == NULL)) {
		printf ("hey, render_init for statusbar doomed to fail\n");
		return;
	}

	/* record the last Text node created, because it is ours! This is easier than
	going through the nodes field by field in the VRML'd string */
	holder = lastTextNode; 


	/* create a 1 UniString entry to the MFString */
	holder->string.p = MALLOC(sizeof (struct Uni_String));
	holder->string.p[0] = newASCIIString("");	/* first string is blank */
	holder->string.n = 1; 				/* we have 1 string in this X3D_Text node */
	myline=(struct Uni_String *)holder->string.p[0];

	/* create an "easy" handle for this string;
	myline = (struct Uni_String *) holder->string.p[0];

	/* NOW - make the Uni_String large... in the first Unistring, make the string 2000 bytes long */
	myline->strptr  = MALLOC(2000);

	/* set the Uni_String to zero length */
	myline->len = 0;

	addToNode (rootNode,offsetof (struct X3D_Group, children), (void *)proxNode);
	addToNode (rootNode,offsetof (struct X3D_Group, children), (void *)transNode);
	add_parent((void *)proxNode, rootNode);
	add_parent((void *)transNode, rootNode);
	
	CRoutes_RegisterSimple((void *)proxNode, offsetof (struct X3D_ProximitySensor, orientation_changed), 
		(void *)transNode, offsetof (struct X3D_Transform, rotation), sizeof (struct SFRotation), 0);
	CRoutes_RegisterSimple((void *)proxNode, offsetof (struct X3D_ProximitySensor, position_changed), 
		(void *)transNode, offsetof (struct X3D_Transform, translation), sizeof (struct SFColor), 0);
	
	sb_initialized = TRUE;
}
 
#endif
