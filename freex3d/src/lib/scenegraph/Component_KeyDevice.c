/*
=INSERT_TEMPLATE_HERE=

$Id$

X3D Key Device Component

*/


/****************************************************************************
    This file is part of the FreeWRL/FreeX3D Distribution.

    Copyright 2009 CRC Canada. (http://www.crc.gc.ca)

    FreeWRL/FreeX3D is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    FreeWRL/FreeX3D is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with FreeWRL/FreeX3D.  If not, see <http://www.gnu.org/licenses/>.
****************************************************************************/



#include <config.h>
#include <system.h>
#include <display.h>
#include <internal.h>

#include <libFreeWRL.h>

#include "../vrml_parser/Structs.h"
#include "../main/headers.h"
#include "../input/EAIHelpers.h"	/* for newASCIIString() */
#include "../vrml_parser/CRoutes.h"

#include "Component_KeyDevice.h"

/*
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

#ifndef AQUA
int shiftPressed = 0;
int ctrlPressed = 0;
#endif

/* mapped from my Apple OSX keyboard, canadian setup, so here goes... */
#if defined (_MSC_VER)
/* values from WinUser.h */
#define PHOME_KEY 0x24
#define PPGDN_KEY 0x22
#define PLEFT_KEY 0x25
#define PEND_KEY 0x23
#define PUP_KEY 0x26
#define PRIGHT_KEY 0x27
#define PPGUP_KEY 0x21
#define PDOWN_KEY 0x28
#define PF1_KEY  0x70
#define PF2_KEY  0x71
#define PF3_KEY  0x72
#define PF4_KEY  0x73
#define PF5_KEY  0x74
#define PF6_KEY  0x75
#define PF7_KEY  0x76
#define PF8_KEY  0x77
#define PF9_KEY  0x78
#define PF10_KEY 0x79
#define PF11_KEY 0x7a
#define PF12_KEY 0x7b
#define PALT_KEY 0x12
#define PCTL_KEY 0x11
#define PSFT_KEY 0x10
#define PDEL_KEY 0x2E  //2E is DELETE 0x08 is backspace
#define PRTN_KEY 13
#define KEYPRESS 1
#define KEYDOWN 2
#define KEYUP	3

#elif defined (AQUA) 

/* verified August 2010 for OSX - aluminium keyboard, "Canadian English" one. */

/* arrow keysi; between main keys and numeric keypad */

#define PLEFT_KEY 0x02
#define PRIGHT_KEY 0x03
#define PDOWN_KEY 0x01
#define PUP_KEY   0x00 
#define PPGUP_KEY 0x2c
#define PPGDN_KEY 0x2d
#define PHOME_KEY 0x29
#define PEND_KEY  0x2b

/* on my aluminum apple keyboard, there is a key labelled "fn" that must be pushed to get the
function mappings - like a "shift" key */
#define PF1_KEY  0x4
#define PF2_KEY  0x5
#define PF3_KEY  0X6
#define PF4_KEY  0X7
#define PF5_KEY  0X8
#define PF6_KEY  0X9
#define PF7_KEY  0X10
#define PF8_KEY  0X11
#define PF9_KEY  0X12	/* these did not work with the "fn" key modifier */
#define PF10_KEY 0X13	/* these did not work with the "fn" key modifier */
#define PF11_KEY 0X14	/* these did not work with the "fn" key modifier */
#define PF12_KEY 0X15	/* these did not work with the "fn" key modifier */

/* these keys are not found on the Apple keyboard (please correct if you disagree!) 
   so, we just make the keysyms a value not generated by the keyboard */
#define PALT_KEY 0XC0 /* JAS */ /* not available on OSX */
#define PCTL_KEY 0XC1 /* JAS */ /* not available on OSX */
#define PSFT_KEY 0XC2 /* JAS */ /* not available on OSX */

#define PDEL_KEY 0x28 /* JAS - key labelled "delete", just to the left of the "end" key */
#define PRTN_KEY 13
#define KEYPRESS 2
#define KEYDOWN 2
#define KEYUP	3

#else
#define PHOME_KEY 80
#define PPGDN_KEY 86
#define PLEFT_KEY 106
#define PEND_KEY 87
#define PUP_KEY 112
#define PRIGHT_KEY 108
#define PPGUP_KEY 85
#define PDOWN_KEY 59
#define PF1_KEY  0xFFBE
#define PF2_KEY  0xFFBF
#define PF3_KEY  0XFFC0
#define PF4_KEY  0XFFC1
#define PF5_KEY  0XFFC2
#define PF6_KEY  0XFFC3
#define PF7_KEY  0XFFC4
#define PF8_KEY  0XFFC5
#define PF9_KEY  0XFFC6
#define PF10_KEY 0XFFC7
#define PF11_KEY 0XFFC8
#define PF12_KEY 0XFFC9
#define PALT_KEY 0XFFE9 //left, and 0XFFEA   //0XFFE7
#define PCTL_KEY 0XFFE3 //left, and 0XFFE4 on right
#define PSFT_KEY 0XFFE1 //left, and 0XFFE2 on right
#define PDEL_KEY 0XFF9F //on numpad, and 0XFFFF near Insert //0x08  
#define PRTN_KEY 13
#define KEYPRESS 1
#define KEYDOWN 2
#define KEYUP	3

#endif
///* from http://www.web3d.org/x3d/specifications/ISO-IEC-19775-1.2-X3D-AbstractSpecification/index.html
//section 21.4.1 
//Key Value
//Home 13
//End 14
//PGUP 15
//PGDN 16
//UP 17
//DOWN 18
//LEFT 19
//RIGHT 20
//F1-F12  1 to 12
//ALT,CTRL,SHIFT true/false
//*/
//#define F1_KEY  1
//#define F2_KEY  2
//#define F3_KEY  3
//#define F4_KEY  4
//#define F5_KEY  5
//#define F6_KEY  6
//#define F7_KEY  7
//#define F8_KEY  8
//#define F9_KEY  9
//#define F10_KEY 10
//#define F11_KEY 11
//#define F12_KEY 12
//#define HOME_KEY 13
//#define END_KEY  14
//#define PGUP_KEY 15
//#define PGDN_KEY 16
//#define UP_KEY   17
//#define DOWN_KEY 18
//#define LEFT_KEY 19
//#define RIGHT_KEY 20
//#define ALT_KEY	30 /* not available on OSX */
//#define CTL_KEY 31 /* not available on OSX */
//#define SFT_KEY 32 /* not available on OSX */
//#define DEL_KEY 0XFFFF /* problem: I'm insterting this back into the translated char stream so 0XFFFF too high to clash with a latin? */
//#define RTN_KEY 13  //what about 10 newline?
//#define NUM0  40
//#define NUM1  41
//#define NUM2  42
//#define NUM3  43
//#define NUM4  44
//#define NUM5  45
//#define NUM6  46
//#define NUM7  47
//#define NUM8  48
//#define NUM9  49
//#define NUMDEC 50


int platform2web3dActionKey(int platformKey)
{
	int key;

	key = 0; //platformKey;
	if(platformKey >= PF1_KEY && platformKey <= PF12_KEY)
		key = platformKey - PF1_KEY + F1_KEY;
	else 
		switch(platformKey)
		{
		case PHOME_KEY:
			key = HOME_KEY; break;
		case PEND_KEY:
			key = END_KEY; break;
		case PPGDN_KEY:
			key = PGDN_KEY; break;
		case PPGUP_KEY:
			key = PGUP_KEY; break;
		case PUP_KEY:
			key = UP_KEY; break;
		case PDOWN_KEY:
			key = DOWN_KEY; break;
		case PLEFT_KEY:
			key = LEFT_KEY; break;
		case PRIGHT_KEY:
			key = RIGHT_KEY; break;
		case PDEL_KEY:  
			key = DEL_KEY; break;
		case PALT_KEY:
			key = ALT_KEY; break;
		case PCTL_KEY:
			key = CTL_KEY; break;
		case PSFT_KEY:
			key = SFT_KEY; break;
		default:
			key = 0;
		}
	return key;
}


/* only keep 1 keyDevice node around; we can make a list if that is eventually
required by the spec. From what I can see, the spec is silent on this regard */

//static struct X3D_Node **keySink = NULL;
//static int keySyncMallocLen = 0;
//static int keySinkCurMax = 0;

typedef struct pComponent_KeyDevice{
	struct X3D_Node **keySink;// = NULL;
	int keySyncMallocLen;// = 0;
	int keySinkCurMax;// = 0;
}* ppComponent_KeyDevice;
void *Component_KeyDevice_constructor(){
	void *v = malloc(sizeof(struct pComponent_KeyDevice));
	memset(v,0,sizeof(struct pComponent_KeyDevice));
	return v;
}
void Component_KeyDevice_init(struct tComponent_KeyDevice *t){
	//public
	//private
	t->prv = Component_KeyDevice_constructor();
	{
		ppComponent_KeyDevice p = (ppComponent_KeyDevice)t->prv;
		p->keySink = NULL;
		p->keySyncMallocLen = 0;
		p->keySinkCurMax = 0;

	}
}
//ppComponent_KeyDevice p = (ppComponent_KeyDevice)gglobal()->Component_KeyDevice.prv;

static void sendToSS(struct X3D_Node *wsk, int key, int upDown);
static void sendToKS(struct X3D_Node* wsk, int key, int upDown);

static void incrementKeySinkList() {
	ppComponent_KeyDevice p = (ppComponent_KeyDevice)gglobal()->Component_KeyDevice.prv;
	if (p->keySinkCurMax >= p->keySyncMallocLen) {
		p->keySyncMallocLen += 10; /* arbitrary number */
		p->keySink = REALLOC(p->keySink, sizeof (struct X3D_Node *) * p->keySyncMallocLen);
	}
}

int KeySensorNodePresent() {
	int count;
	ppComponent_KeyDevice p = (ppComponent_KeyDevice)gglobal()->Component_KeyDevice.prv;

	/* no KeyDevice node present */
	if (p->keySink == NULL) return FALSE;

	for (count=0; count < p->keySinkCurMax; count++) {
		/* hmmm, there is one, but is it enabled? */
		/* printf ("ks, checking %d\n",p->keySink[count]); */

		if (p->keySink[count]->_nodeType == NODE_KeySensor) 
			if (X3D_KEYSENSOR(p->keySink[count])->enabled) return TRUE;

		if (p->keySink[count]->_nodeType == NODE_StringSensor) 
			if (X3D_STRINGSENSOR(p->keySink[count])->enabled) return TRUE;
	}

	return FALSE;
}


void addNodeToKeySensorList(struct X3D_Node* node) {
	if ((node->_nodeType == NODE_KeySensor) || (node->_nodeType == NODE_StringSensor)) {
		ppComponent_KeyDevice p = (ppComponent_KeyDevice)gglobal()->Component_KeyDevice.prv;
		incrementKeySinkList();
		p->keySink[p->keySinkCurMax] = node;
		p->keySinkCurMax ++;
	}
}

void killKeySensorNodeList() {
	ppComponent_KeyDevice p = (ppComponent_KeyDevice)gglobal()->Component_KeyDevice.prv;
	FREE_IF_NZ(p->keySink);
	p->keySyncMallocLen = 0; 
	p->keySinkCurMax = 0;

	#ifndef AQUA
	shiftPressed = 0;
	ctrlPressed = 0;
	#endif
}

void sendKeyToKeySensor(const char key, int upDown) {
	int count;
	ppComponent_KeyDevice p = (ppComponent_KeyDevice)gglobal()->Component_KeyDevice.prv;
	if (p->keySink == NULL) return;

	for (count=0; count < p->keySinkCurMax; count++) {
		#ifdef VERBOSE
		printf ("sendKeyToKeySensor, sending key %d to %d of %d\n",key,count,p->keySinkCurMax);
		#endif

        /* make sure this has not been deleted  - we should really re-create list, but
         so few keySensor X3D nodes are in use, who cares? */
        if (checkNode(p->keySink[count],__FILE__,__LINE__)) {
			if(upDown%10 == KEYDOWN || upDown%10 == KEYUP) //2 down, or 3 up
				if (p->keySink[count]->_nodeType == NODE_KeySensor ) sendToKS(p->keySink[count], (int)key&0xFFFF, upDown);
			if(upDown == KEYPRESS) //LINUX,WIN32 PRESS=1, AQUA PRESS=2
				if (p->keySink[count]->_nodeType == NODE_StringSensor ) sendToSS(p->keySink[count], (int)key&0xFFFF, upDown);
		}
    }
}

/*******************************************************/

static void sendToKS(struct X3D_Node* wsk, int key, int upDown) {
	int actionKey;
	int isDown;
	int isActionKey;
	#define MYN X3D_KEYSENSOR(wsk)
	/* printf ("sending key %x %u upDown %d (down %d) to keySenors\n",key,key,upDown,KEYDOWN); */
	
	/* if not enabled, do nothing */
	if (!MYN) 
		return;
	if (MYN->__oldEnabled != MYN->enabled) {
		MYN->__oldEnabled = MYN->enabled;
		MARK_EVENT(X3D_NODE(MYN),offsetof (struct X3D_KeySensor, enabled));
	}
	if (!MYN->enabled) 
		return;

	/* is this an ACTION (tm) key  press or release? */
	isDown = upDown%10 == KeyPress;
	isActionKey = upDown / 10;
	if(isActionKey)
	{
	  actionKey = key;
	  switch (actionKey) {
		case HOME_KEY:
		case PGDN_KEY:
		case LEFT_KEY:
		case END_KEY:
		case UP_KEY:
		case RIGHT_KEY:
		case PGUP_KEY:
		case DOWN_KEY:
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
		/* no DEL key here*/
			if (isDown)  {
				MYN->actionKeyPress = actionKey; //TRUE; 
				MARK_EVENT(X3D_NODE(MYN), offsetof (struct X3D_KeySensor, actionKeyPress));
			} else {
				MYN->actionKeyRelease = actionKey; //TRUE;
				MARK_EVENT(X3D_NODE(MYN), offsetof (struct X3D_KeySensor, actionKeyRelease));
			}
			break;
		case ALT_KEY:
			/* now, for some of the other keys, the ones that are modifiers, not ACTION (tm) keys. */
			MYN->altKey = isDown;
			MARK_EVENT(X3D_NODE(MYN), offsetof (struct X3D_KeySensor, altKey));
			break;
		case CTL_KEY:
			MYN->controlKey = isDown;
			MARK_EVENT(X3D_NODE(MYN), offsetof (struct X3D_KeySensor, controlKey));
			break;
		case SFT_KEY:
			MYN->shiftKey = isDown;
			MARK_EVENT(X3D_NODE(MYN), offsetof (struct X3D_KeySensor, shiftKey));
			break;
		default:
			break;
	    }/*end switch */
	}  else { 
		/* regular key including RTN */
		if ((MYN->keyPress->len != 2) || (MYN->keyRelease->len != 2)) {
			FREE_IF_NZ(MYN->keyPress->strptr);
			FREE_IF_NZ(MYN->keyRelease->strptr);
			MYN->keyPress = newASCIIString ("a");
			MYN->keyRelease = newASCIIString ("a");
		}
			
		if (isDown) {
			MYN->keyPress->strptr[0] = (char) (key&0xFF);
			MARK_EVENT(X3D_NODE(MYN), offsetof (struct X3D_KeySensor, keyPress));
		} else {
			MYN->keyRelease->strptr[0] = (char) (key&0xFF);
			MARK_EVENT(X3D_NODE(MYN), offsetof (struct X3D_KeySensor, keyRelease));
		}
	}

	/* now, presumably "isActive" means that the key is down... */
	MYN->isActive = isDown;
	MARK_EVENT(X3D_NODE(MYN), offsetof (struct X3D_KeySensor, isActive));
	#undef MYN
	
}
static void sendToSS(struct X3D_Node *wsk, int key, int upDown) {
	//int actionKey;
	#define MYN X3D_STRINGSENSOR(wsk)
	#define MAXSTRINGLEN 512

	/* printf ("SS, %u enabled %d\n",wsk, MYN->enabled); */
	/* printf ("sendToSS, key %x, upDown %d\n",key,upDown); */

	/* if not enabled, do nothing */
	if (!MYN) return;
	if (MYN->__oldEnabled != MYN->enabled) {
		MYN->__oldEnabled = MYN->enabled;
		MARK_EVENT(X3D_NODE(MYN),offsetof (struct X3D_StringSensor, enabled));
	}
	if (!MYN->enabled) return;
	/* printf ("sending key %x %u upDown %d to keySenors\n",key,key,upDown); */

	//actionKey = platform2web3dActionKey(key);
	//translation moved to handle_XEvents
	//#if !defined(AQUA) && !defined(_MSC_VER)
	///* on Unix, we have to handle control/shift keys ourselves. OSX handles this
	//   by itself */
	//if (actionKey == SFT_KEY) {
	//	shiftPressed = (upDown == KEYDOWN);
	//	return;
	//}

	///* do the shift of the A-Z keys if shift pressed */
	//if ((key >= 'a') && (key<='z'))
	//	if (shiftPressed)
	//		key=key-'a'+'A';
	//#endif

	/* ignore the control key here. OSX will not event let one come this far... */
	//if (actionKey == CTL_KEY) return;

	/* we only care about key presses here */
	if (upDown != KEYPRESS) return;


	/* is this initialized? */
	if (!MYN->_initialized) {
		FREE_IF_NZ(MYN->enteredText->strptr);
		FREE_IF_NZ(MYN->finalText->strptr);
		MYN->enteredText->strptr = MALLOC(char *, MAXSTRINGLEN+1);
		MYN->finalText->strptr = MALLOC(char *, MAXSTRINGLEN+1);
		MYN->enteredText->len=1;
		MYN->finalText->len=1;
		MYN->enteredText->strptr[0] = '\0';
		MYN->finalText->strptr[0] = '\0';
		MYN->_initialized = TRUE;
		MYN->isActive = FALSE;
	}
	
	/* enteredText */
	if ((MYN->deletionAllowed) && (key==DEL_KEY)) {
		if (MYN->enteredText->len > 1) {
			MYN->enteredText->len--;
			MYN->enteredText->strptr[MYN->enteredText->len-1] = '\0';
			MARK_EVENT(X3D_NODE(MYN), offsetof (struct X3D_StringSensor, enteredText));
		}
	} else {
		if ((key != RTN_KEY) && (key != DEL_KEY) && (MYN->enteredText->len < MAXSTRINGLEN-1)) {
			MYN->enteredText->strptr[MYN->enteredText->len-1] = (char)key;
			MYN->enteredText->strptr[MYN->enteredText->len] = '\0';
			MYN->enteredText->len++;
			MARK_EVENT(X3D_NODE(MYN), offsetof (struct X3D_StringSensor, enteredText));

			if (!MYN->isActive) {
				MYN->isActive = TRUE;
				MARK_EVENT(X3D_NODE(MYN), offsetof (struct X3D_StringSensor, isActive));
			}
			
		}
	}


	/* finalText */
	if (key==RTN_KEY) {
		#ifdef VERBOSE
		printf ("found return!\n");
		printf ("current enteredText :%s:\n",MYN->enteredText->strptr);
		printf ("current finalText :%s:\n",MYN->finalText->strptr);
		#endif

		memcpy(MYN->finalText->strptr, MYN->enteredText->strptr, MAXSTRINGLEN);
		MYN->finalText->len = MYN->enteredText->len;
		MYN->enteredText->len=1;
		MYN->enteredText->strptr[0] = '\0';
		MARK_EVENT(X3D_NODE(MYN), offsetof (struct X3D_StringSensor, finalText));
		/* MARK_EVENT(X3D_NODE(MYN), offsetof (struct X3D_StringSensor, enteredText)); specs say don't gen an event here*/

		MYN->isActive = FALSE;
		MARK_EVENT(X3D_NODE(MYN), offsetof (struct X3D_StringSensor, isActive));

		#ifdef VERBOSE
		printf ("finalText:%s:\n",MYN->finalText->strptr); 
		#endif
	}
}
