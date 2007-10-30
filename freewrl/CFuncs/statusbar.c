/*
 * Copyright(C) 1998 Tuomas J. Lukka, 2001, 2002 John Stewart. CRC Canada.
 * NO WARRANTY. See the license (the file COPYING in the VRML::Browser
 * distribution) for details.
 */

#include <math.h>

#include "headers.h"
#include "Viewer.h"

/* DO NOT CHANGE THESE DEFINES WITHOUT CHECKING THE USE OF THE CODE, BELOW */
#define PROX "ProximitySensor { size 1000 1000 1000 }"
#define TEXT "Transform{translation 0 0 10 children[Collision{collide FALSE children [Transform{scale 0.4 0.8 0.8 translation 0 -0.1 -.2 children[Shape{geometry Text{fontStyle FontStyle{justify \"MIDDLE\" size 0.02}}}]}]}]}"



static int sb_initialized = FALSE;
static struct Uni_String *myline;
void render_init(void);
static void statusbar_init(void);
static struct X3D_ProximitySensor *proxNode = NULL;
static struct X3D_Transform *transNode =NULL;
static struct X3D_Text *textNode = NULL;

#define STATUS_LEN 2000

/* trigger a update */

void update_status(char* msg) {
	if (!sb_initialized) {
		if (rootNode == NULL) return; /* system not running yet?? */
		statusbar_init();
	}

	/* bounds check here - if the string is this long, it deserves to be ignored! */
	if (strlen(msg) > (STATUS_LEN-10)) return;

	sprintf(myline->strptr, "%s", msg);
	myline->len = strlen(msg)+1; /* length of message, plus the null terminator */
	#ifdef VERBOSE
	printf("myline-> strptr is %s, len is %d\n", myline->strptr, myline->len);
	#endif
	update_node((void*) textNode);
}

void clear_status() {
	if (!sb_initialized) return;

	sb_initialized = FALSE;

	myline->len = 0;
}


/* render the status bar. If it is required... */ 
static void statusbar_init() {
	int tmp;
	uintptr_t nodarr[200];
	int ra;
	struct X3D_Group * myn;
	struct X3D_Node *tempn;

	/* put a ProximitySensor and text string in there. */
	myn = createNewX3DNode(NODE_Group);
	inputParse(FROMSTRING, PROX, FALSE, FALSE, myn, offsetof(struct X3D_Group, children), &tmp, FALSE);

	/* get the ProximitySensor node from this parse. Note, errors are not checked. If this gives an errror,
	   then there are REALLY bad things happening somewhere else */

	/* remove this ProximitySensor node from the temporary variable, and reset the temp. variable */
	proxNode = myn->children.p[0];
	myn->children.n = 0;

	inputParse(FROMSTRING, TEXT, FALSE, FALSE, myn, offsetof(struct X3D_Group, children), &tmp, FALSE);
	transNode = myn->children.p[0];
	myn->children.n = 0;

	/* get the Text node, as a pointer. The TEXT definition, above, gives us the following:
		Transform(children)->Collision(children)->Transform(childen)->Shape(geometry)->Text */

	tempn = X3D_NODE(transNode->children.p[0]);
	/* printf ("step 1; we should be at a Collision node: %s\n",stringNodeType(tempn->_nodeType)); */

	tempn =  X3D_NODE(((struct X3D_Collision *)tempn)->children.p[0]);
	/* printf ("step 2; we should be at a Transform node: %s\n",stringNodeType(tempn->_nodeType)); */

	tempn =  X3D_NODE(((struct X3D_Transform *)tempn)->children.p[0]);
	/* printf ("step 3; we should be at a Shape node: %s\n",stringNodeType(tempn->_nodeType)); */

	textNode  =  (struct X3D_Text*) ((struct X3D_Shape *)tempn)->geometry;
	/* printf ("step 4; we should be at a text node: %s\n",stringNodeType(textNode->_nodeType)); */


	/* create a 1 UniString entry to the MFString */
	textNode->string.p = MALLOC(sizeof (struct Uni_String));
	textNode->string.p[0] = newASCIIString("");	/* first string is blank */
	textNode->string.n = 1; 				/* we have 1 string in this X3D_Text node */
	myline=(struct Uni_String *)textNode->string.p[0];

	/* create an "easy" handle for this string;
	myline = (struct Uni_String *) textNode->string.p[0];

	/* NOW - make the Uni_String large... in the first Unistring, make the string 2000 bytes long */
	myline->strptr  = MALLOC(STATUS_LEN);

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
