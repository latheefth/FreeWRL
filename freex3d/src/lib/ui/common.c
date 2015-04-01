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

#if defined (_MSC_VER)
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
	void *colorScheme;
	int colorSchemeChanged;
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
		p->colorScheme = NULL;
		p->colorSchemeChanged = 0;
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

	snprintf(&p->messagebar[0], 10, " %8.2f ", p->myFps);
	snprintf(&p->messagebar[15], sizeof(p->myMenuStatus)-15, "%s", p->myMenuStatus);
}
char *getMessageBar()
{
	ppcommon p = (ppcommon)gglobal()->common.prv;
	return p->messagebar;
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
	int ic,ii;
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
#include <malloc.h>
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
void fwl_set_ui_colorscheme(char *colorschemename){
	colorScheme *cs;
	ppcommon p = (ppcommon)gglobal()->common.prv;
	cs = search_ui_colorscheme(colorschemename);
	if(cs) {
		p->colorScheme = cs;
		p->colorSchemeChanged;
	}
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
//want to compile-in the default color scheme? just define UI_COLORSCHEME_DEFAULT in your config.h
#ifndef UI_COLORSCHEME_DEFAULT
#define UI_COLORSCHEME_DEFAULT "neon:yellow" //"original" "favicon" "midnight" "aqua" "angry" "neon:cyan" "neon:yellow" "neon:lime" "neon:pink"
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