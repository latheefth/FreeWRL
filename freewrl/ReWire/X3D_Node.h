#include <sys/types.h>
#include <stdint.h>
#include "EAIheaders.h"
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <strings.h>
#include <string.h>
#include <stdlib.h>

typedef struct { int type; int value; } _intX3D_SFBool;
typedef struct { int type; float value; } _intX3D_SFFloat;
typedef struct { int type; float value; } _intX3D_SFTime;
typedef struct { int type; int value; } _intX3D_SFInt32;
typedef struct { int type; uintptr_t *adr; char SFNodeType; } _intX3D_SFNode;
typedef struct { int type; float r[4]; } _intX3D_SFRotation;
typedef struct { int type; float c[2]; } _intX3D_SFVec2f;
typedef struct { int type; double c[2]; } _intX3D_SFVec2d;
typedef struct { int type; float c[3]; } _intX3D_SFColor;
typedef struct { int type; float c[3]; } _intX3D_SFVec3f;
typedef struct { int type; double c[3]; } _intX3D_SFVec3d;
typedef struct { int type; float r[4]; } _intX3D_SFColorRGBA;
typedef struct { int type; int len; char *strptr;} _intX3D_SFString;
typedef struct { int type; int len; char *strptr;} _intX3D_SFImage;

typedef struct { int type; int n; _intX3D_SFColor *p; } _intX3D_MFColor;
typedef struct { int type; int n; _intX3D_SFColorRGBA *p; } _intX3D_MFColorRGBA;
typedef struct { int type; int n; float  *p; } _intX3D_MFFloat;
typedef struct { int type; int n; double *p; } _intX3D_MFTime;
typedef struct { int type; int n; _intX3D_SFRotation *p; } _intX3D_MFRotation;
typedef struct { int type; int n; _intX3D_SFVec3d *p; } _intX3D_MFVec3d;
typedef struct { int type; int n; _intX3D_SFVec2d *p; } _intX3D_MFVec2d;
typedef struct { int type; int n; _intX3D_SFVec3f *p; } _intX3D_MFVec3f;
typedef struct { int type; int n; _intX3D_SFVec2f *p; } _intX3D_MFVec2f;
typedef struct { int type; int n; _intX3D_SFBool *p; } _intX3D_MFBool;
typedef struct { int type; int n; _intX3D_SFInt32 *p; } _intX3D_MFInt32;
typedef struct { int type; int n; _intX3D_SFNode *p; } _intX3D_MFNode;
typedef struct { int type; int n; _intX3D_SFString *p; } _intX3D_MFString;
typedef struct { int type; int n; _intX3D_SFImage *p; } _intX3D_MFImage;

typedef union _X3D_Node {
	int 	type;
	_intX3D_SFBool		X3D_SFBool;
	_intX3D_SFFloat		X3D_SFFloat;
	_intX3D_SFTime		X3D_SFTime;
	_intX3D_SFInt32		X3D_SFInt32;
	_intX3D_MFColor 	X3D_MFColor;
	_intX3D_MFColorRGBA	X3D_MFColorRGBA;
	_intX3D_SFString	X3D_SFString;
	_intX3D_SFNode		X3D_SFNode;
	_intX3D_SFRotation	X3D_SFRotation;
	_intX3D_SFVec2f		X3D_SFVec2f;
	_intX3D_SFVec2d		X3D_SFVec2d;
	_intX3D_SFColor		X3D_SFColor;
	_intX3D_SFColor		X3D_SFVec3f;
	_intX3D_SFVec3d		X3D_SFVec3d;
	_intX3D_SFColorRGBA	X3D_SFColorRGBA;
	_intX3D_MFFloat		X3D_MFFloat;
	_intX3D_MFTime		X3D_MFTime;
	_intX3D_MFInt32		X3D_MFInt32;
	_intX3D_MFString	X3D_MFString;
	_intX3D_MFNode		X3D_MFNode;
	_intX3D_MFRotation	X3D_MFRotation;
	_intX3D_MFVec2f		X3D_MFVec2f;
	_intX3D_MFVec3f		X3D_MFVec3f;
	_intX3D_MFImage		X3D_MFImage;

} X3D_Node;


#define TRUE 1==1
#define FALSE 1!=1

#define REMOVE_EOT {char *lp; lp=strstr(ptr,"RE_EOT"); if (lp!=NULL) {lp--; *lp='\0';}};
#define SKIP_IF_GT_SPACE        while (*ptr > ' ') ptr++;
#define SKIP_CONTROLCHARS       while ((*ptr != '\0') && (*ptr <= ' ')) ptr++;


/* structures */


struct _intX3D_EventIn {
	uintptr_t	nodeptr;
	int 		offset;
	char		datatype;
	int 		datasize;
	int		scripttype;
	char 		*field;
};

#define X3D_EventIn struct _intX3D_EventIn
#define X3D_EventOut struct _intX3D_EventIn
/* single value structures */


X3D_Node *X3D_getNode (char *name);
X3D_EventIn *X3D_getEventIn(X3D_Node *node, char *name);
X3D_EventOut *X3D_getEventOut(X3D_Node *node, char *name);
void X3D_setValue (X3D_EventIn *dest, X3D_Node *node);
void X3D_addRoute (X3D_EventOut *from, X3D_EventIn *to);
void X3D_deleteRoute (X3D_EventOut *from, X3D_EventIn *to);


/* initialize, shutdown public methods */
void X3D_initialize(char *);
void X3D_shutdown();
void freewrlReadThread(void);

/* float public methods */
float X3D_getCurrentSpeed();
float X3D_getCurrentFrameRate();

/* null public methods */
void X3D_firstViewpoint();
void X3D_lastViewpoint();
void X3D_nextViewpoint();
void X3D_previousViewpoint();
void X3D_setDescription(char *newDescription);

/* string return val public methods */
char *X3D_getDescription();
char *X3D_getName();
char *X3D_getVersion();
char *X3D_getWorldURL();

/* MFNode public methods */
X3D_Node *X3D_createVrmlFromString(char *str);
X3D_Node *X3D_newSFVec3f (float a, float b, float c);
X3D_Node *X3D_newSFColor (float a, float b, float c);
X3D_Node *X3D_newSFVec2f (float a, float b);
X3D_Node *X3D_newSFRotation (float a, float b,float c, float d);
X3D_Node *X3D_newSFColorRGBA (float a, float b,float c, float d);
X3D_Node *X3D_newSFVec3d (double a, double b,double c);
X3D_Node *X3D_newSFVec2d (double a, double b);
X3D_Node *X3D_newSFBool (int a);
X3D_Node *X3D_newSFFloat (float a);
X3D_Node *X3D_newSFTime (double a);
X3D_Node *X3D_newSFInt32 (int a);
X3D_Node *X3D_newSFString();
X3D_Node *X3D_newSFNode();
X3D_Node *X3D_newSFImage();
X3D_Node *X3D_newMFColor();
X3D_Node *X3D_newMFFloat();
X3D_Node *X3D_newMFTime();
X3D_Node *X3D_newMFInt32();
X3D_Node *X3D_newMFString();
X3D_Node *X3D_newMFNode();
X3D_Node *X3D_newMFRotation();
X3D_Node *X3D_newMFVec2f();
X3D_Node *X3D_newMFVec3f();
X3D_Node *X3D_newMFColorRGBA();
X3D_Node *X3D_newMFBool();
X3D_Node *X3D_newMFVec3d();
X3D_Node *X3D_newMFVec2d();

extern int _X3D_queryno;
extern int _X3D_FreeWRL_FD;
int _X3D_countWords(char *ptr);
char *_X3D_make1StringCommand (char command, char *name);
char *_X3D_make2StringCommand (char command, char *str1, char *str2);
char *_X3D_Browser_SendEventType(uintptr_t *adr,char *name, char *evtype);
char *_X3D_makeShortCommand (char command);
void _X3D_sendEvent (char command, char *string);
void _handleFreeWRLcallback(char *command);

void X3D_error(char *msg);
char *fieldTypeName(char type);

char * _RegisterListener (X3D_EventOut *node, int adin);
int X3DAdvise (X3D_EventOut *node, void *fn);
void _handleReWireCallback(char *buf);
