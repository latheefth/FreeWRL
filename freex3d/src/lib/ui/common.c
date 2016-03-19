/*

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
#include <scenegraph/Vector.h>

#if defined (_MSC_VER) || defined (AQUA)
#include "../../buildversion.h"
#endif


// Linux builds, thanks to our very own Ian, creates this function for us.
// on other platforms, we have to have this defined, as we don't have Ian's
// talents to help us out.

#if defined (AQUA) || defined (_MSC_VER) || defined(QNX)
const char *libFreeWRL_get_version(void) {return FW_BUILD_VERSION_STR;}
#endif //OSX


#define MAXSTAT 200

#define MAXTITLE 200

typedef struct keyval {
	char *key;
	char *val;
} keyval;

/* textual status messages */
typedef struct pcommon{
	float myFps; // = (float) 0.0;
	int target_frames_per_second;
	char myMenuStatus[MAXSTAT];
	char messagebar[MAXSTAT];
	char fpsbar[16];
	char window_title[MAXTITLE];
	int cursorStyle;
	int promptForURL;
	int promptForFile;
	int sb_hasString;// = FALSE;
	char buffer[200];
	int showConsoleText;
	void *colorScheme;
	int colorSchemeChanged;
	int pin_statusbar;
	int pin_menubar;
	int want_menubar;
	int want_statusbar;
	struct Vector *keyvals;
}*ppcommon;
void *common_constructor(){
	void *v = MALLOCV(sizeof(struct pcommon));
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
		p->colorScheme = NULL;
		p->colorSchemeChanged = 0;
		p->pin_statusbar = 1;
		p->pin_menubar = 0;
		p->want_menubar = 1;
		p->want_statusbar = 1;
		p->keyvals = NULL;
		p->showConsoleText = 0;  //in the UI, if a callback is registered with ConsoleMessage. Won't affect old fashioned console, 
		p->target_frames_per_second = 120;  //is 120 FPS a good target FPS?
	}
}
void common_clear(struct tcommon *t){
	//public
	//private
	{
		ppcommon p = (ppcommon)t->prv;
		if(p->keyvals){
			int i;
			for(i=0;i<vectorSize(p->keyvals);i++){
				keyval k_v = vector_get(keyval,p->keyvals,i);
				FREE_IF_NZ(k_v.key);
				FREE_IF_NZ(k_v.val);
			}
			deleteVector(keyval,p->keyvals);
		}
	}
}

//ppcommon p = (ppcommon)gglobal()->common.prv;

/* Status update functions (generic = all platform) */
void setFpsBar();
void setMenuFps(float fps)
{
	ppcommon p = (ppcommon)gglobal()->common.prv;

	p->myFps = fps;
	setFpsBar();
}
/* make sure that on a re-load that we re-init */
void kill_status(void) {
	/* hopefully, by this time, rendering has been stopped */
	ppcommon p = (ppcommon)gglobal()->common.prv;

	p->sb_hasString = FALSE;
	p->buffer[0] = '\0';
}

void showConsoleText(int on){
	ppcommon p = (ppcommon)gglobal()->common.prv;
	p->showConsoleText = on;
}
int getShowConsoleText(){
	ppcommon p = (ppcommon)gglobal()->common.prv;
	return p->showConsoleText;
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
void setMenuStatus3(char* status3)
{
	char *pp;
	ppcommon p = (ppcommon)gglobal()->common.prv;

	pp = status3;
	if (!pp) pp = "";
	snprintf(p->myMenuStatus, MAXSTAT-1, "%s", pp);
}
void setMenuStatus(char *stattext)
{
	setMenuStatus3(stattext);
}
void setMenuStatusVP(char *stattext)
{
	setMenuStatus3(stattext);
}
char *getMenuStatus()
{
	return ((ppcommon)gglobal()->common.prv)->myMenuStatus;
}
//#if !defined (_ANDROID)


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
//#endif //ANDROID

void setMessageBar()
{
	ppcommon p = (ppcommon)gglobal()->common.prv;

	snprintf(p->messagebar, MAXSTAT-1, "%s", p->myMenuStatus);
}
char *getMessageBar()
{
	ppcommon p = (ppcommon)gglobal()->common.prv;
	return p->messagebar;
}
char *getFpsBar(){
	ppcommon p = (ppcommon)gglobal()->common.prv;
	return p->fpsbar;
}
void setFpsBar(){
	ppcommon p = (ppcommon)gglobal()->common.prv;
	//snprintf(p->fpsbar, 10, "%7.2f", p->myFps);
	snprintf(p->fpsbar, 10, "%4d", (int)(p->myFps + .49999f));
}
static int frontend_using_cursor = 0;
void fwl_set_frontend_using_cursor(int on)
{
	//used by statusbarHud to shut off cursor settings coming from sensitive nodes
	//while the mouse is over the statusbar or menu buttons.
	frontend_using_cursor = on;
}

void setArrowCursor()
{
	ppcommon p = (ppcommon)gglobal()->common.prv;
	p->cursorStyle = ACURSE;
}
void setLookatCursor()
{
	ppcommon p = (ppcommon)gglobal()->common.prv;
	p->cursorStyle = SCURSE;  // need a special cursor just for lookat
}

void setSensorCursor()
{
	ppcommon p = (ppcommon)gglobal()->common.prv;
	p->cursorStyle = SCURSE;
}

int getCursorStyle()
{
	ppcommon p = (ppcommon)gglobal()->common.prv;
	if (!frontend_using_cursor)
		return p->cursorStyle;
	else
		return ACURSE;
}

int fwl_set_sbh_pin_option(char *optarg){
	if(optarg && strlen(optarg) > 1){
		ppcommon p = (ppcommon)gglobal()->common.prv;
		p->pin_statusbar = (optarg[0] == 'T' || optarg[0] == 't') ? 1 : 0;
		p->pin_menubar = (optarg[1] == 'T' || optarg[1] == 't') ? 1 : 0;
	}
	return 1;
}
int fwl_set_sbh_want_option(char *optarg){
	if(optarg && strlen(optarg) > 1){
		ppcommon p = (ppcommon)gglobal()->common.prv;
		p->want_statusbar = (optarg[0] == 'T' || optarg[0] == 't') ? 1 : 0;
		p->want_menubar = (optarg[1] == 'T' || optarg[1] == 't') ? 1 : 0;
	}
	return 1;
}

void fwl_set_sbh_pin(int sb, int mb){
	ppcommon p = (ppcommon)gglobal()->common.prv;
	p->pin_statusbar = sb;
	p->pin_menubar = mb;
}
void fwl_get_sbh_pin(int *sb, int *mb){
	ppcommon p = (ppcommon)gglobal()->common.prv;
	*sb = p->pin_statusbar;
	*mb = p->pin_menubar;
}
void fwl_set_sbh_wantMenubar(int want){
	ppcommon p = (ppcommon)gglobal()->common.prv;
	p->want_menubar = want ? 1 : 0;
}
int fwl_get_sbh_wantMenubar(){
	ppcommon p = (ppcommon)gglobal()->common.prv;
	return p->want_menubar;
}
void fwl_set_sbh_wantStatusbar(int want){
	ppcommon p = (ppcommon)gglobal()->common.prv;
	p->want_statusbar = want ? 1 : 0;
}
int fwl_get_sbh_wantStatusbar(){
	ppcommon p = (ppcommon)gglobal()->common.prv;
	return p->want_statusbar;
}

void fwl_set_target_fps(int target_fps){
	ppcommon p = (ppcommon)gglobal()->common.prv;
	p->target_frames_per_second = max(1,target_fps);
}
int fwl_get_target_fps(){
	ppcommon p = (ppcommon)gglobal()->common.prv;
	return p->target_frames_per_second;
}
// start ui color scheme >>>>>>>>>>>

// StatusbarHud color schemes:

typedef struct colorScheme {
	char *name;
	char *panel;
	char *menuIcon;
	char *statusText;
	char *messageText;
} colorScheme;
static colorScheme colorSchemes [] = {
{ 
"original",
"#EBE8D7", //{.922f,.91f,.844f,1.0f}; 235 232 215  //offwhite
"#5E5EE6", //{0.37f,0.37f,0.9f,1.0f};  94 94 230//medium blue
"#333333", //{.2f, .2f, .2f, 1.0f};	51 //very dark grey
"#FFFFFF", //{1.0f, 1.0f, 1.0f, 1.0f}; 255 //white
},
{
"midnight",
"#000000",
"#FFFFFF",
"#FFFFFF",
"#FFFFFF",
},
{
"angry",
"#003333", //= {0.0f,0.2f,0.2f,1.0f};  //slightly blue-green black
"#FF0000", // {1.0f, 0.0f, 0.0f, 1.0f}; //red
"#FF0000", //{1.0f, 0.0f, 0.0f, 1.0f}; //red
"#FF0000", // {1.0f, 0.0f, 0.0f, 1.0f}; //red
},
{
"favicon", 
"#004073", // {0.0f,0.25f,0.45f,1.0f}; 0 64 115//indigo
"#91CCF0", // {.57f, 0.8f, 0.94f, 1.0f}; 145 204 240//light aqua
"#FF7800", // {1.0f, 0.47f, 0.0f, 1.0f}; 255 120 0//orange
"#FF7800", // {1.0f, 0.47f, 0.0f, 1.0f}; 255 120 0//orange
},
{
"aqua",
"#BFD4BD", // {0.75f,0.83f,0.74f,1.0f}; 191 212 189//clamshell
"#007085", //{.0f, 0.44f, 0.52f, 1.0f};  0 112 133//dark aqua/indigo
"#52736E", // {.32f, 0.45f, 0.43f, 1.0f};  82 115 110//dark clamshell
"#0FB0CC", // {.06f, 0.69f, 0.8f, 1.0f}; 15 176 204//aqua
},
{
"neon:lime",
"#3D4557", //= {0.24f,0.27f,0.34f,1.0f};  61 69 87//steely grey
"#CCFF00", //LIME {.8f,1.0f,0.0f,1.0f} 204 255 0
"#CCFF00", //LIME {.8f,1.0f,0.0f,1.0f} 204 255 0
"#CCFF00", //LIME {.8f,1.0f,0.0f,1.0f} 204 255 0
},
{
"neon:yellow",
"#3D4557", //= {0.24f,0.27f,0.34f,1.0f};  61 69 87//steely grey
"#FFFF33", //YELLOW {1.0f,1.0f,.2f,1.0f} 255 255 51
"#FFFF33", //YELLOW {1.0f,1.0f,.2f,1.0f} 255 255 51
"#FFFF33", //YELLOW {1.0f,1.0f,.2f,1.0f} 255 255 51
},
{
"neon:cyan",
"#3D4557", //= {0.24f,0.27f,0.34f,1.0f};  61 69 87//steely grey
"#00FFFF", //CYAN {0.0f,1.0f,1.0f,1.0f} 0 255 255
"#00FFFF", //CYAN {0.0f,1.0f,1.0f,1.0f} 0 255 255
"#00FFFF", //CYAN {0.0f,1.0f,1.0f,1.0f} 0 255 255
},
{
"neon:pink",
"#3D4557", //= {0.24f,0.27f,0.34f,1.0f};  61 69 87//steely grey
"#FF78FF", //PINK {1.0f,.47f,1.0f,1.0f} 255 120  255
"#FF78FF", //PINK {1.0f,.47f,1.0f,1.0f} 255 120  255
"#FF78FF", //PINK {1.0f,.47f,1.0f,1.0f} 255 120  255
},
{
"custom",
NULL,
NULL,
NULL,
NULL,
},
{NULL,NULL,NULL,NULL},
};

void color_html2rgb(char *html, float *rgb){
	//converts one html color in "#FFFFFF" or "FFFFFF" format
	//int float[3] rgb colors in range 0.0f-1.0f suitable for  use in opengl
	int ir, ig, ib;
	long ic;
	char *shex;
	shex = html;
	if(shex[0] == '#') shex = &shex[1];
	ic = strtol(shex,NULL,16);
	ib = (ic & 0xFF);
	ig = (ic & 0xFF00) >> 8;
	ir = (ic & 0xFF0000) >> 16;
	rgb[0] = (float)ir/255.0f;
	rgb[1] = (float)ig/255.0f;
	rgb[2] = (float)ib/255.0f;
}
char *hexpermitted = " #0123456789ABCDEFabcdef";
#ifdef AQUA
#include <malloc/malloc.h>
#else
#include <malloc.h>
#endif

#include <string.h>
int colorsoption2colorscheme(const char *optionstring, colorScheme *cs){
	//converts html colors given for freewrl command line option:
	// --ui_colors "#FFFFFF,#FFFFFF,#FFFFFF,#FFFFFF" (for panel, menuicon, statusText, messageText)
	//into 4 float [0-1] rgb colors suitable for use in opengl calls
	//returns number of successfully parsed numbers
	int len,i,count;
	char *str, *html, *stok; //4 colors per color scheme
	len = strlen(optionstring);
	str = alloca(len+1); //alloca on stack so its freed automatically at end of function, _msc can't do str[len]
	strcpy(str,optionstring);
	//clean string
	for(i=0;i<len;i++){
		if(!strchr(hexpermitted,str[i])){
			str[i] = ' ';
		}
	}
	//find color substrings ie strtok
	count = 0;
	stok = str;
	for(i=0;i<4;i++){
		html = strtok(stok," ");
		if(!html) {
			if(cs->menuIcon) html = cs->menuIcon;
			else html = "#FFFFFF";
		}
		switch(i){
			case 0: cs->panel = strdup(html); break;
			case 1: cs->menuIcon = strdup(html); break;
			case 2: cs->statusText = strdup(html); break;
			case 3: cs->messageText = strdup(html); break;
			default:
				break;
		}
		count++;
		stok = NULL;
	}
	return count;
}

colorScheme *search_ui_colorscheme(char *colorschemename){
	int i;
	colorScheme *cs = NULL;
	i = 0;
	do{
		if(!strcmp(colorSchemes[i].name,colorschemename)){
			cs = &colorSchemes[i];
			break;
		}
		i++;
	}while(colorSchemes[i].name);
	return cs;
}
int fwl_set_ui_colorscheme(char *colorschemename){
	colorScheme *cs;
	ppcommon p = (ppcommon)gglobal()->common.prv;
	cs = search_ui_colorscheme(colorschemename);
	if(cs) {
		p->colorScheme = cs;
		p->colorSchemeChanged++;
	}
	return 1;
}
// set here from commandline options
// --ui_colorscheme "angry"
// --ui_colors "#FFFFFF,#FFFFFF,#FFFFFF,#FFFFFF"  panel, menuIcon, statusText, messsageText

void fwl_set_ui_colors(char *fourhtmlcolors){
	colorScheme *cs;
	ppcommon p = (ppcommon)gglobal()->common.prv;
	cs = search_ui_colorscheme("custom");
	colorsoption2colorscheme(fourhtmlcolors, cs);
	p->colorScheme = (void *)cs;
	p->colorSchemeChanged++;
}
char *fwl_get_ui_colorschemename(){
	colorScheme *cs;
	ppcommon p = (ppcommon)gglobal()->common.prv;
	cs = (colorScheme*)p->colorScheme;
	return cs->name;
}
void fwl_next_ui_colorscheme(){
	int i;
	colorScheme *cs;
	char *colorschemename;
	//ppcommon p = (ppcommon)gglobal()->common.prv;

	colorschemename = fwl_get_ui_colorschemename();
	i = 0;
	do{
		if(!strcmp(colorSchemes[i].name,colorschemename)){
			cs = &colorSchemes[i+1];
			if(!cs->name){
				cs = &colorSchemes[0]; //start over
			}
			if(!strcmp(cs->name,"custom")){
				cs = &colorSchemes[0]; //skip custom and start over
			}
			fwl_set_ui_colorscheme(cs->name);
			break;
		}
		i++;
	}while(colorSchemes[i].name);

}

//want to compile-in the default color scheme? just define UI_COLORSCHEME_DEFAULT in your config.h
#ifndef UI_COLORSCHEME_DEFAULT
#define UI_COLORSCHEME_DEFAULT "neon:lime" //"original" "favicon" "midnight" "aqua" "angry" "neon:cyan" "neon:yellow" "neon:lime" "neon:pink"
#endif
void fwl_get_ui_color(char *use, float *rgb){
	colorScheme *cs;
	ppcommon p = (ppcommon)gglobal()->common.prv;
	if(!p->colorScheme){
		p->colorScheme = search_ui_colorscheme(UI_COLORSCHEME_DEFAULT); //"original");
		p->colorSchemeChanged++;
	}
	cs = p->colorScheme;
	if(!strcmp(use,"panel")){
		color_html2rgb(cs->panel, rgb);
	}else if(!strcmp(use,"menuIcon")){
		color_html2rgb(cs->menuIcon, rgb);
	}else if(!strcmp(use,"statusText")){
		color_html2rgb(cs->statusText, rgb);
	}else if(!strcmp(use,"messageText")){
		color_html2rgb(cs->messageText, rgb);
	}
}
int fwl_get_ui_color_changed(){
	ppcommon p = (ppcommon)gglobal()->common.prv;
	return p->colorSchemeChanged;
}
// end ui colors <<<<<<<<<<<<<<<


// fwl_command() >>>>>>>>>>
/* generally: I'm tired of writing fwl_setThisOrThat() functions. I want a simpler interface.
	one idea is to have a set_keyval(key,val) function and a set_command(key) function.
	another idea is to have a set_commandline(commandline) function and it would split 
	on a separator like ',' or ' ' and decide if it has a parameter or not.
	Now from your front end you can call:
		fwl_commandline("pin,FF");
	Or if calling through the dllfreewrl api wrapper:
		dllFreeWRL_commandline(fwctx,"pin,FF");

	PS. A benefit of storing View settings/preferences like colorscheme and pinning
	in the Model part of MVC is that several Views can be setting and/or polling 
	the settings on each frame.
	For example commandline options can set, then javascript Browser.keyvalue can set or get,
	then statusbarHud can poll or set, then Motif or .net Gui can poll or set. And they have
	a common place to set, and poll. If there's no statusbarHud, and no GUI, nothing 
	breaks: commandline options still has a place to put the values. Even though they
	aren't used in the backend/Model.
*/
#include <scenegraph/Viewer.h>

int fwl_setDragChord(char *chordname);
int fwl_setKeyChord(char *chordname);
int print_help();
int fwl_keyval(char *key, char *val);

int searchkeyvals(char *key){
	int i, iret;
	ppcommon p = (ppcommon)gglobal()->common.prv;
	if(!p->keyvals)
		p->keyvals = newVector(keyval,4);
	iret = -1;
	for(i=0;i<vectorSize(p->keyvals);i++){
		keyval k_v = vector_get(keyval,p->keyvals,i);
		if(!strcmp(k_v.key,key)){
			iret = i;
			break;
		}
	}
	return iret;
}
int set_key_val(char *key, char *val){
	int index;
	keyval k_v;
	ppcommon p = (ppcommon)gglobal()->common.prv;

	index = searchkeyvals(key);
	if(index < 0){
		if(!p->keyvals)
			p->keyvals = newVector(keyval,4);
		k_v.key = STRDUP(key);
		k_v.val = STRDUP(val);
		vector_pushBack(keyval,p->keyvals,k_v);
	}else{
		k_v = vector_get(keyval,p->keyvals,index);
		FREE_IF_NZ(k_v.val);
		k_v.val = STRDUP(val);
		vector_set(keyval,p->keyvals,index,k_v);
	}
	return 1;
}
int set_keyval(char *keyval){
	//save arbitrary char* keyval = "key,val" pairs, 
	// for later retrieval with print_keyval or get_key_val
	int i, iret;
	char kv[100];
	ppcommon p = (ppcommon)gglobal()->common.prv;
	if(!p->keyvals)
		p->keyvals = newVector(keyval,4);
	i = strlen(keyval);
	iret = 0;
	if(i > 100) 
		iret = -1;
	else
	{
		char *sep;
		strcpy(kv,keyval);
		sep = strchr(kv,' ');
		if(!sep) sep = strchr(kv,',');
		if(sep){
			char *key, *val;
			val = &sep[1];
			(*sep) = '\0';
			key = kv;
			set_key_val(key,val);
			iret = 1;
		}
	}
	return iret;
}
char *get_key_val(char *key){
	int index;
	keyval k_v;
	char *ret = NULL;
	ppcommon p = (ppcommon)gglobal()->common.prv;

	index = searchkeyvals(key);
	if(index < 0) return NULL;
	k_v = vector_get(keyval,p->keyvals,index);
	return k_v.val; //warning not strduped here, caller doesn't own, just looking
}
int print_keyval(char *key){
	int index;
	ppcommon p = (ppcommon)gglobal()->common.prv;
	index = searchkeyvals(key);
	if(index < 0)
		ConsoleMessage("\n key %s not found\n",key);
	else{
		keyval k_v;
		k_v = vector_get(keyval,p->keyvals,index);
		ConsoleMessage("\n key=%s val=%s\n",key,k_v.val);
	}
	return 1;
}
void fwl_hyper_option(char *val);
int ssr_test(char *keyval);
struct command {
	char *key;
	int (*cmdfunc)();
	int (*valfunc)(char *val);
	char *helpstring;
} commands [] = {
	{"dragchord",NULL,fwl_setDragChord,"[yawz,yawpitch,roll,xy]"},
	{"keychord", NULL,fwl_setKeyChord,"[yawz,yawpitch,roll,xy]"},
	{"navmode",NULL,fwl_setNavMode,"[walk,fly,examine,explore,spherical,turntable,lookat]"},
	{"help",print_help,NULL,NULL},
	{"pin",NULL,fwl_set_sbh_pin_option,"[tf,tt,ft,ff]"},
	{"colorscheme",NULL,fwl_set_ui_colorscheme,"[original,midnight,angry,favicon,aqua,neon:lime,neon:yellow,neon:cyan,neon:pink]"},
	{"set_keyval",NULL,set_keyval,"key,val"},
	{"print_keyval",NULL,print_keyval,"key"},
	{"hyper_option",NULL,fwl_hyper_option,"[0 - 10]"},
#ifdef SSR_SERVER
	{"ssrtest",NULL,ssr_test,"nav,val"},
#endif
	{"",print_help,NULL,NULL}, //bootstrap user knowhow by spacebarEnter lucky sequence
	{NULL,NULL,NULL},
};
int print_help(){
	int i, ret = 1;
	ConsoleMessage("\n%s\n","spacebar commands: spacebar:key[,val]Enter");
	i = 0;
	while(commands[i].key){
		if(commands[i].helpstring) 
			ConsoleMessage(" %s,%s\n",commands[i].key,commands[i].helpstring);
		else
			ConsoleMessage(" %s\n",commands[i].key);
		i++;
	}
	return ret;
}
struct command *getCommand(char *key){
	struct command *ret;
	int i, ok = 0;
	i = 0;
	ret = NULL;
	while(commands[i].key){
		if(!strcmp(key,commands[i].key)){
			ret = &commands[i];
			break;
		}
		i++;
	}
	return ret;
}
int fwl_keyval(char *key, char *val){
	struct command *cmd;
	int ok = 0;
	cmd = getCommand(key);
	if(cmd){
		if(cmd->valfunc)
			ok = cmd->valfunc(val);
	}
	return ok;
}
int fwl_command(char *key){
	struct command *cmd;
	int ok = 0;
	cmd = getCommand(key);
	if(cmd){
		if(cmd->cmdfunc)
			ok = cmd->cmdfunc();
	}
	return ok;
}
int fwl_commandline(char *cmdline){
	char *sep = strchr(cmdline,' ');
	if(!sep) sep = strchr(cmdline,',');
	if(sep){
		int keylen;
		char *key, *val;
		val = strdup(&sep[1]);
		keylen = (int)(sep - cmdline);
		//(*sep) = '\0';
		key = strndup(cmdline,keylen +1);
		key[keylen] = '\0';
		printf("key=[%s] val=[%s]\n",key,val);
		fwl_keyval(key,val);
		free(key);
		free(val);
	}else{
		//not key,val, just command
		fwl_command(cmdline);
	}
	return 1;
}

// fwl_command() <<<<<<<<<<