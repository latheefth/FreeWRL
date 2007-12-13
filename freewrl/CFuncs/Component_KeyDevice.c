/*******************************************************************
 Copyright (C) 2005, 2006 John Stewart, Ayla Khan, Sarah Dumoulin, CRC Canada.
 DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
 See the GNU Library General Public License (file COPYING in the distribution)
 for conditions of use and redistribution.
*********************************************************************/


/*******************************************************************

	X3D Key Device Component

I'll leave off comments about the validity of this part of the spec - it does
not seem well thought out. I'll leave off my comments on what I really think
about this part of the X3D Spec, because I'm nice, and I don't want to 
put other people's ideas down, especially in public, in source code, that
will outlive me. 

So, if there is a KeyDevice node present, DO NOT use keys for FreeWRL navigation
but instead, send any along that the Operating System GUI does not capture,
and hope that they are not too badly mangled by intervening layers.

Lets just hope that this part of the spec dies a convenient (and speedy)
death!

Anyway, with that, lets blindly forge along...

*********************************************************************/
#include "headers.h"

#define X3D_KEYSENSOR(node) ((struct X3D_KeySensor*)node)
#define X3D_STRINGSENSOR(node) ((struct X3D_StringSensor*)node)
static void sendToKS(int key, int upDown);
static void sendToSS(int key, int upDown);

/* mapped from my Apple OSX keyboard, canadian setup, so here goes... */
#define HOME_KEY 80
#define PGDN_KEY 86
#define END_KEY 87
#define UP_KEY 112
#define RIGHT_KEY 108
#define PGUP_KEY 85
#define PGDN_KEY 86
#define F1_KEY  0xFFBE
#define F2_KEY  0xFFBF
#define F3_KEY  0XFFC0
#define F4_KEY  0XFFC1
#define F5_KEY  0XFFC2
#define F6_KEY  0XFFC3
#define F7_KEY  0XFFC4
#define F8_KEY  0XFFC5
#define F9_KEY  0XFFC6
#define F10_KEY 0XFFC7
#define F11_KEY 0XFFC8
#define F12_KEY 0XFFC9
#define ALT_KEY	0XFFE7
#define CTL_KEY 0XFFE3
#define SFT_KEY 0XFFE1
#define DEL_KEY 0x08
#define RTN_KEY 13
#define KEYDOWN 2
#define KEYUP	3


/* only keep 1 keyDevice node around; we can make a list if that is eventually
required by the spec. From what I can see, the spec is silent on this regard */

static struct X3D_Node *keySink = NULL;

int KeySensorNodePresent() {
	/* no KeyDevice node present */
	if (keySink == NULL) return FALSE;

	/* hmmm, there is one, but is it enabled? */
	if (keySink->_nodeType == NODE_KeySensor) return X3D_KEYSENSOR(keySink)->enabled;
	if (keySink->_nodeType == NODE_StringSensor) return X3D_STRINGSENSOR(keySink)->enabled;

	/* should never get this far... */
	return FALSE;
}


void addNodeToKeySensorList(struct X3D_Node* node) {
	if ((node->_nodeType == NODE_KeySensor) || (node->_nodeType == NODE_StringSensor)) {
		if (keySink != NULL) {
			ConsoleMessage("More than 1 KeyDevice node; using last one defined");
		}
		keySink = node;
	}
}

void killKeySensorNodeList() {
	printf ("killKeySenoorNodeList\n");
	keySink = NULL;
}

void sendKeyToKeySensor(const char key, int upDown) {
	if (keySink == NULL) return;

	if (keySink->_nodeType == NODE_KeySensor) sendToKS((int)key&0xFFFF, upDown);
	if (keySink->_nodeType == NODE_StringSensor) sendToSS((int)key&0xFFFF, upDown);
}

/*******************************************************/

static void sendToKS(int key, int upDown) {
	#define MYN X3D_KEYSENSOR(keySink)
	/* printf ("sending key %x %u upDown %d to keySenors\n",key,key,upDown); */
	
	if (!MYN->enabled) return;

	/* is this an ACTION (tm) key  press or release? */
	switch (key) {
		case HOME_KEY:
		case PGDN_KEY:
		case END_KEY:
		case UP_KEY:
		case RIGHT_KEY:
		case PGUP_KEY:
		case F1_KEY:
		case F2_KEY:
		case F3_KEY:
		case F4_KEY:
		case F5_KEY:
		case F6_KEY:
		case F7_KEY:
		case F8_KEY:
		case F9_KEY:
		case F10_KEY:
		case F11_KEY:
		case F12_KEY:
			if (upDown == KEYDOWN)  {
				MYN->actionKeyPress = TRUE; 
				mark_event(keySink, offsetof (struct X3D_KeySensor, actionKeyPress));
			} else {
				MYN->actionKeyRelease = TRUE;
				mark_event(keySink, offsetof (struct X3D_KeySensor, actionKeyRelease));
			}
			break;
		default: {
			if ((MYN->keyPress->len != 2) || (MYN->keyRelease->len != 2)) {
				FREE_IF_NZ(MYN->keyPress->strptr);
				FREE_IF_NZ(MYN->keyRelease->strptr);
				MYN->keyPress = newASCIIString ("a");
				MYN->keyRelease = newASCIIString ("a");
			}
				
			if (upDown == KEYDOWN) {
				MYN->keyPress->strptr[0] = (char) (key&0xFF);
				mark_event(keySink, offsetof (struct X3D_KeySensor, keyPress));
			} else {
				MYN->keyRelease->strptr[0] = (char) (key&0xFF);
				mark_event(keySink, offsetof (struct X3D_KeySensor, keyRelease));
			}
		}
	}

	/* now, for some of the other keys, the ones that are modifiers, not ACTION (tm) keys. */
	MYN->altKey = key==ALT_KEY;
	mark_event(keySink, offsetof (struct X3D_KeySensor, altKey));
	MYN->controlKey = key==ALT_KEY;
	mark_event(keySink, offsetof (struct X3D_KeySensor, controlKey));
	MYN->shiftKey = key==ALT_KEY;
	mark_event(keySink, offsetof (struct X3D_KeySensor, shiftKey));

	/* now, presumably "isActive" means that the key is down... */
	MYN->isActive = upDown == KEYDOWN;
	mark_event(keySink, offsetof (struct X3D_KeySensor, isActive));
	#undef MYN
	
}
static void sendToSS(int key, int upDown) {
	#define MYN X3D_STRINGSENSOR(keySink)
	#define MAXSTRINGLEN 512

	printf ("sending key %x %u upDown %d to keySenors\n",key,key,upDown); 

	if (!MYN->enabled) return;
	if (upDown != KEYDOWN) return;

	/* is this initialized? */
	if (!MYN->_initialized) {
		FREE_IF_NZ(MYN->enteredText->strptr);
		FREE_IF_NZ(MYN->finalText->strptr);
		MYN->enteredText->strptr = MALLOC(MAXSTRINGLEN+1);
		MYN->finalText->strptr = MALLOC(MAXSTRINGLEN+1);
		MYN->enteredText->len=1;
		MYN->finalText->len=1;
		MYN->enteredText->strptr[0] = '\0';
		MYN->finalText->strptr[0] = '\0';
		MYN->_initialized = TRUE;
	}
	
	/* enteredText */
	if ((MYN->deletionAllowed) && (key==DEL_KEY)) {
		if (MYN->enteredText->len > 1) {
			MYN->enteredText->len--;
			MYN->enteredText->strptr[MYN->enteredText->len-1] = '\0';
			mark_event(keySink, offsetof (struct X3D_StringSensor, enteredText));
		}
	} else {
		if ((key != RTN_KEY) && (key != DEL_KEY) && (MYN->enteredText->len < MAXSTRINGLEN-1)) {
			MYN->enteredText->strptr[MYN->enteredText->len-1] = (char)key;
			MYN->enteredText->strptr[MYN->enteredText->len] = '\0';
			MYN->enteredText->len++;
			mark_event(keySink, offsetof (struct X3D_StringSensor, enteredText));

			if (!MYN->isActive) {
				MYN->isActive = TRUE;
				mark_event(keySink, offsetof (struct X3D_StringSensor, isActive));
			}
			
		}
	}

printf ("enteredText:%s:\n",MYN->enteredText->strptr);
	/* finalText */
	if (key==RTN_KEY) {
		strncpy (MYN->finalText->strptr, MYN->enteredText->strptr, MAXSTRINGLEN);
		MYN->finalText->len = MYN->enteredText->len;
		MYN->enteredText->len=1;
		MYN->enteredText->strptr[0] = '\0';
		mark_event(keySink, offsetof (struct X3D_StringSensor, finalText));

		MYN->isActive = FALSE;
		mark_event(keySink, offsetof (struct X3D_StringSensor, isActive));
printf ("finalText:%s:\n",MYN->finalText->strptr);
	}




}
