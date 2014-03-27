/*
  $Id$

  FreeWRL support library.

  See common.h.

*/

/*
	MVC - the setter functions here in common.c are called from the Model (libfreewrl),
	- the getter functions you call from your View (UI), typically once per frame, in a style called 'polling'
	- once per Controller loop (_DisplayThread) you can have the controller notify your View that 
	  it's time to poll the Model for updates
	- benefit of MVC: the Model never calls back into the View, so isn't dependent on it, so
		a) the View is easier to change (for different platforms and windowing technology), and
		b) the Model is UI-technology-agnostic, so it's easier to maintain across platforms.
	- Polling vs callbacks: the reason we poll the model, instead of registering callbacks:
		the Controller is usually in the same language/technology as the UI, which often isn't C, 
		and calling into C is usually much easier then calling back from C into python, ObjectiveC, Java, C#
		or whatever other technology/language your View/UI and Controller is in
*/

#include <config.h>
#include <system.h>
#include <internal.h>
#include <libFreeWRL.h>
#include <iglobal.h>
#include "../ui/common.h"

#include "../../buildversion.h"

// Linux builds, thanks to our very own Ian, creates this function for us.
// on other platforms, we have to have this defined, as we don't have Ian's
// talents to help us out.

#if defined (AQUA) || defined (_MSC_VER) || defined(QNX)
const char *libFreeWRL_get_version(void) {return FW_BUILD_VERSION_STR;}
#endif //OSX


#define MAXSTAT 200

#define MAXTITLE 200

/* textual status messages */
typedef struct pcommon{
	float myFps; // = (float) 0.0;
	char myMenuStatus[MAXSTAT];
	char messagebar[MAXSTAT];
	char window_title[MAXTITLE];
	int cursorStyle;
	int promptForURL;
	int promptForFile;
	int sb_hasString;// = FALSE;
	char buffer[200];
}*ppcommon;
void *common_constructor(){
	void *v = malloc(sizeof(struct pcommon));
	memset(v,0,sizeof(struct pcommon));
	return v;
}
void common_init(struct tcommon *t){
	//public
	//private
	t->prv = common_constructor();
	{
		ppcommon p = (ppcommon)t->prv;
		p->myFps = (float) 0.0;
		p->cursorStyle = ACURSE;
		p->sb_hasString = FALSE;
	}
}
//ppcommon p = (ppcommon)gglobal()->common.prv;

/* Status update functions (generic = all platform) */

void setMenuFps(float fps)
{
	ppcommon p = (ppcommon)gglobal()->common.prv;

	p->myFps = fps;
	setMessageBar();
}
/* make sure that on a re-load that we re-init */
void kill_status(void) {
	/* hopefully, by this time, rendering has been stopped */
	ppcommon p = (ppcommon)gglobal()->common.prv;

	p->sb_hasString = FALSE;
	p->buffer[0] = '\0';
}


/* trigger a update */
void update_status(char* msg) {
	ppcommon p = (ppcommon)gglobal()->common.prv;

	if (msg == NULL){
		p->sb_hasString = FALSE;
		p->buffer[0] = '\0';
	}
	else {
		p->sb_hasString = TRUE;
		strcpy(p->buffer, msg);
	}
}
char *get_status(){
	ppcommon p = (ppcommon)gglobal()->common.prv;
	return p->buffer;
}
void setMenuStatus2(char* prefix, char *suffix)
{
	//int loading = FALSE;
	char *pp, *ss;
	ppcommon p = (ppcommon)gglobal()->common.prv;

 //       if (fwl_isinputThreadParsing() || 
	//    fwl_isTextureParsing() || 
	//    (!fwl_isInputThreadInitialized())) loading = TRUE;

	//if (loading) {
	//	snprintf(p->myMenuStatus, sizeof(p->myMenuStatus),
	//		 "(Loading...)");
	//} else {
	pp = prefix;
	ss = suffix;
	if (!pp) pp = "";
	if (!ss) ss = "";
		snprintf(p->myMenuStatus, sizeof(p->myMenuStatus), "%s %s", pp,ss);
	//}
}
void setMenuStatus(char *stattext)
{
	setMenuStatus2(stattext, NULL);
}
void setMenuStatusVP(char *stattext)
{
	setMenuStatus2("Viewpoint:",stattext);

}
char *getMenuStatus()
{
	return ((ppcommon)gglobal()->common.prv)->myMenuStatus;
}
#if !defined (_ANDROID)


void setWindowTitle0()
{
	ppcommon p = (ppcommon)gglobal()->common.prv;

	snprintf(p->window_title, sizeof(p->window_title), "FreeWRL");
	//setWindowTitle(); //dug9 Mar2014: it will be up to your UI/View to poll for getWindowTitle and set any windowing title in your UI.
}
char *getWindowTitle()
{
	ppcommon p = (ppcommon)gglobal()->common.prv;
	return p->window_title;
}
#endif //ANDROID

void setMessageBar()
{
	ppcommon p = (ppcommon)gglobal()->common.prv;

	snprintf(&p->messagebar[0], 10, " %8.2f ", p->myFps);
	snprintf(&p->messagebar[15], sizeof(p->myMenuStatus)-15, "%s", p->myMenuStatus);
}
char *getMessageBar()
{
	ppcommon p = (ppcommon)gglobal()->common.prv;
	return p->messagebar;
}


void setArrowCursor()
{
	ppcommon p = (ppcommon)gglobal()->common.prv;
	p->cursorStyle = ACURSE;
}
void setSensorCursor()
{
	ppcommon p = (ppcommon)gglobal()->common.prv;
	p->cursorStyle = SCURSE;
}
int getCursorStyle()
{
	ppcommon p = (ppcommon)gglobal()->common.prv;
	return p->cursorStyle;
}

