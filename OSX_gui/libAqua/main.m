/* This is the default source file for new dynamic libraries. */

/* You can either fill in code here or remove this and create or add new files. */
#include <OpenGL/OpenGL.h>
#include <AGL/AGL.h>
#include <AppKit/AppKit.h>
#include "../frontend/FreeWRLView.h"

FreeWRLView* fwview;
NSOpenGLContext* fwcontext;
AGLContext aglcontext;
Boolean isPlugin;
NSCursor* crosscur;
NSCursor* arrowcur;

//int main() {

//	return 0;
//}

bool isOpen = FALSE;
FILE* theFile;
char debugs[256];

void debugPrint(char* theString)
{
	// if (!isOpen) {
	//	theFile = fopen("/Users/sarah/lib_log", "w");
	//	if (theFile == NULL)
	//		abort();
	//	isOpen = TRUE;
	// }
	// fprintf(theFile, "lib: %s\n", theString);
	// fflush(theFile);
}

void updateContext() {
	if (!isPlugin) {
		[fwcontext update];
		[fwcontext flushBuffer];
	} else {
			sprintf(debugs, "context is %p - updating not flushing\n", aglcontext);
			debugPrint(debugs);
			//aglSetCurrentContext(aglcontext);
			aglSwapBuffers(aglcontext);
			sprintf(debugs, "after swap buffer, error is %s\n", aglErrorString(aglGetError()));
			debugPrint(debugs);
			GLboolean check = aglUpdateContext(aglcontext);
			sprintf(debugs, "check is %d", check);
			debugPrint(debugs);
	}
}

void fwpassObjects(FreeWRLView* v, NSOpenGLContext* con, NSCursor* crcur, NSCursor* arrcur) {
	//debugPrint("got to passObjects");
	fwview = v;
	isPlugin = false;
	fwcontext = con;
	crosscur = crcur;
	arrowcur = arrcur;
	[crosscur retain];
	[arrowcur retain];
	[con makeCurrentContext];
	//debugPrint("getting out of passObjects");
}

void pluginPassObjects(AGLContext con) {
	aglcontext = con;
	sprintf(debugs, "ppo context is %p\n", con);
	debugPrint(debugs);
	aglSetCurrentContext(aglcontext);
	isPlugin = true;
}

void pluginPassObjects2(AGLContext* con) {
	aglcontext = *con;
	sprintf(debugs, "ppo context is %p (%p)\n", aglcontext, &aglcontext);
	debugPrint(debugs);
	aglSetCurrentContext(aglcontext);
	isPlugin = true;
}

float getWidth() {
	NSRect boundsRect = [fwview bounds];
	return boundsRect.size.width;
}

float getHeight() {
	NSRect boundsRect = [fwview bounds];
	return boundsRect.size.height;
}

void setAquaCursor(int ctype) {
	if (ctype == 0) 
	    [fwview setArrowCursor];
    else
		[fwview setCrossCursor];
}

void setMenuButton_headlight(int val) {
		[fwview setHeadlightButton: val];
}

void createAutoReleasePool() {
	[fwview createReleasePool];
}
void setMenuButton_collision(int val) {
	[fwview setCollisionButton: val];
}

void setMenuStatus(char* stat) {
	[fwview setStatusMess: stat];
}

void setMenuFps(float fps) {
	[fwview setStatusFps: fps];
}

void setConsoleMessage(char *str) {
	[fwview setConsoleMess: str];
}

void setMenuButton_navModes(int type) {
	[fwview setNavMode: type];
}

void setMenuButton_texSize(int size) {
	[fwview setTexSize: size];
}

void aquaSetConsoleMessage(char* msg) {
	printf(@"got message: %s\n", msg);
	[fwview setConsoleMess: msg];
}