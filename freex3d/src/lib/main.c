/*
  $Id$

  FreeWRL support library.
  Resources handling: URL, files, ...

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
#include <system_threads.h>
#include <display.h>
#include <internal.h>
#include <signal.h>
#include <libFreeWRL.h>
#include <list.h>
#include <io_files.h>
#include <resources.h>
#include <threads.h>
#include "vrml_parser/Structs.h"
#include "main/ProdCon.h"
#include "input/InputFunctions.h"

#include "ui/common.h"

char consoleBuffer[200];
freewrl_params_t fwl_params;

/**
 * library initialization
 */
//#ifdef _MSC_VER
//void libFreeWRL_init(void)
//#else
//void __attribute__ ((constructor)) libFreeWRL_init(void)
//#endif
//{
//	memset(&fwl_params, 0, sizeof(fwl_params));
//}

/**
 * library exit routine
 */
//#ifdef _MSC_VER
//void libFreeWRL_fini(void)
//#else
//void __attribute__ ((destructor)) libFreeWRL_fini(void)
//#endif
//{
//}

/**
 * Explicit initialization
 */
 

#if defined (TARGET_AQUA) || defined(_ANDROID)

/* put some config stuff here, as that way the Objective-C Standalone OSX front end does not
need to worry about specific structures and calls */

void fwl_OSX_initializeParameters(const char* initialURL) {
    resource_item_t *res;
    freewrl_params_t myParams;

    ttglobal tg = gglobal();

    printf ("fwl_OSX_initializeParameters, sending in %s\n",initialURL);
    
    
    /* have we been through once already (eg, plugin loading new file)? */

	//ConsoleMessage("fwl_OSX_initializeParameters - loadThread %p,  pcThread %p", tg->threads.loadThread, tg->threads.PCthread);

    if ((tg->threads.loadThread == NULL) || (tg->threads.PCthread == NULL)) {
	//ConsoleMessage("fwl_OSX_initializeParameters, qParamsInit is FALSE");

	//JAS fwl_initParams(NULL);
#ifdef WEREWRWERW
        typedef struct freewrl_params {
            /* Put here global parameters, parsed in main program
             and needed to initialize libFreeWRL
             example: width, height, fullscreen, multithreading, eai...
             */
            int width;
            int height;
            int xpos;
            int ypos;
            long int winToEmbedInto;
            bool fullscreen;
            bool multithreading;
            bool eai;
            bool verbose;
            //int collision;	/* do collision detection? moved to x3d_viewer struct july 7, 2012*/
            
        } freewrl_params_t;
#endif 
        
        myParams.width = 600;
        myParams.height = 400;
        myParams.xpos = 0;
        myParams.ypos = 0;
        myParams.winToEmbedInto = 0;
        myParams.fullscreen = FALSE;
        myParams.multithreading = TRUE;
        myParams.eai = FALSE;
        myParams.verbose = FALSE;
        
    	/* Default values */
    	fwl_setp_width(600);
    	fwl_setp_height(400);
    	fwl_setp_eai(FALSE);
    	fwl_setp_fullscreen(FALSE);
    	/* removed by doug fwl_setp_collision(1); */

    /* start threads, parse initial scene, etc */

	//ConsoleMessage ("calling fwl_initFreeWRL from within fwl_OSX_initializeParameters");

	   if (!fwl_initFreeWRL(&myParams)) {
		ERROR_MSG("main: aborting during initialization.\n");
		exit(1);
	   }
    } 


    /* Give the main argument to the resource handler */
    res = resource_create_single(initialURL);

    res->new_root = TRUE;
    send_resource_to_parser(res,__FILE__,__LINE__);

    while ((!res->complete) && (res->status != ress_failed) && (res->status != ress_not_loaded)) {
            usleep(100);
    }


    /* did this load correctly? */
    if (res->status == ress_not_loaded) {
	sprintf(consoleBuffer , "FreeWRL: Problem loading file \"%s\"", res->request);
	fwl_StringConsoleMessage(consoleBuffer);
    }

    if (res->status == ress_failed) {
	printf("load failed %s\n", initialURL);
	sprintf(consoleBuffer , "FreeWRL: unknown data on command line: \"%s\"", res->request);
	fwl_StringConsoleMessage(consoleBuffer);
    } else {

    	/* tell the new world which viewpoint to go to */
    	if (res->afterPoundCharacters != NULL) {
		fwl_gotoViewpoint(res->afterPoundCharacters);
		/* Success! 
		printf("loaded %s\n", initialURL); */
	}

    }
}

#endif // iPhone

/* OSX plugin is telling us the id to refer to */
void setInstance (uintptr_t instance) {
        /* printf ("setInstance, setting to %u\n",instance); */
        _fw_instance = instance;
}

/* osx Safari plugin is telling us where the initial file is */
void setFullPath(const char* file) 
{
/* turn collision on? 
    if (!fwl_getp_collision()) {
        char ks = 'c';
        do_keyPress(ks, KeyPress);
    }
*/

    /* remove a FILE:// or file:// off of the front */
    file = stripLocalFileName ((char *)file);
    FREE_IF_NZ (BrowserFullPath);
    BrowserFullPath = STRDUP((char *) file);
    /*
	sprintf(consoleBuffer , "setBrowserFullPath is %s (%d)",BrowserFullPath,strlen(BrowserFullPath));
	fwl_StringConsoleMessage(consoleBuffer);
    */
}

char *strForeslash2back(char *str)
{
#ifdef _MSC_VER
	int jj;
	for( jj=0;jj<strlen(str);jj++)
		if(str[jj] == '/' ) str[jj] = '\\';
#endif
	return str;
}

//void fwl_initParams(freewrl_params_t *params)
//{
//	if (params) {
//		DEBUG_MSG("copying application supplied params...\n");
//		memcpy(&fwl_params, params, sizeof(freewrl_params_t));
//		memcpy(
//	} else {
//		memset(&fwl_params, 0, sizeof(freewrl_params_t));
//	}
//}

void fwl_setp_width		(int foo)	{ fwl_params.width = foo; }
void fwl_setp_height		(int foo)	{ fwl_params.height = foo; }
void fwl_setp_winToEmbedInto	(long int foo)	{ fwl_params.winToEmbedInto = foo; }
void fwl_setp_fullscreen	(bool foo)	{ fwl_params.fullscreen = foo; }
void fwl_setp_multithreading	(bool foo)	{ fwl_params.multithreading = foo; }
void fwl_setp_eai		(bool foo)	{ fwl_params.eai = foo; }
void fwl_setp_verbose		(bool foo)	{ fwl_params.verbose = foo; }
//void fwl_setp_collision		(int foo)	{ fwl_params.collision = foo; }

int	fwl_getp_width		(void)	{ return fwl_params.width; }
int	fwl_getp_height		(void)	{ return fwl_params.height; }
long int fwl_getp_winToEmbedInto (void)	{ return fwl_params.winToEmbedInto; }
bool	fwl_getp_fullscreen	(void)	{ return fwl_params.fullscreen; }
bool	fwl_getp_multithreading	(void)	{ return fwl_params.multithreading; }
bool	fwl_getp_eai		(void)	{ return fwl_params.eai; }
bool	fwl_getp_verbose	(void)	{ return fwl_params.verbose; }
//int	fwl_getp_collision	(void)	{ return fwl_params.collision; }

//static ttglobal fwl_instance_parameters = NULL;


void* fwl_init_instance()
{
	ttglobal tg; 
	//ConsoleMessage ("called fwl_init_instance");
	tg = iglobal_constructor();
    
    fwl_setCurrentHandle(tg,__FILE__,__LINE__);
	return (void *)tg;
}

bool fwl_initFreeWRL(freewrl_params_t *params){
	ttglobal tg;
	tg = (ttglobal)fwl_getCurrentHandle(__FILE__,__LINE__);
	//ConsoleMessage ("fwl_initFreeWRL, tg %p",tg);

	if(tg == NULL) tg = fwl_init_instance();
	TRACE_MSG("FreeWRL: initializing...\n");

	//ConsoleMessage ("fwl_initFreeWRL, mainThread %p",tg->threads.mainThread);

	tg->threads.mainThread = pthread_self();

	/* dug9 Aug 23, 2013 
		For the main UI thread that's shared between multiple
		libfreewrl instances within a single process
		-ie 2 ActiveX controls on a single web page or gui app, or
		a console program that pops up 2 separate freewrl instances-
		we use fwl_setCurrentHandle(ttglobal) from the calling application
		process to switch gglobals for this UI/main thread.
		For the worker threads, we lookup their ttglobal based on their
		threadID.
	*/
#if !defined(_ANDROID)
	/* Android does not have stdout nor stderr */
	/* Initialize console (log, error, ...) */
	setbuf(stdout,0);
        setbuf(stderr,0);
#endif
	/* Check parameters */
	if (params) {
		DEBUG_MSG("copying application supplied params...\n");
		//memcpy(&fwl_params, params, sizeof(freewrl_params_t));
		memcpy(&tg->display, params, sizeof(freewrl_params_t));
		//tg->display.win_height = params->height;// = 0; /* window */
		//tg->display.win_width = params->width;// = 0;
		//tg->display.winToEmbedInto = params->winToEmbedInto;// = -1;
		//tg->display.fullscreen = params->fullscreen;// = FALSE;
	}

#if !defined(EXCLUDE_EAI)
	/* do we require EAI? */
	if (params->eai){ //fwl_getp_eai()) {
		fwlio_RxTx_control(CHANNEL_EAI, RxTx_START);
		//	set_thread2global(tglobal* fwl, pthread_t any );

	}
#endif


	/* Initialize parser */
	fwl_initialize_parser();

	///* Initialize common UI variables */ - done in common.c
	//myMenuStatus[0] = '\0';

#ifndef FRONTEND_HANDLES_DISPLAY_THREAD
	/* OK the display is now initialized,
	   create the display thread and wait for it
	   to complete initialization */
	fwl_initializeDisplayThread();

	//usleep(50);
	//set_thread2global(tg,tg->threads.DispThrd ,"display thread");

#endif //FRONTEND_HANDLES_DISPLAY_THREAD

	fwl_initializeInputParseThread();
	//set_thread2global(tg, tg->threads.PCthread ,"parse thread");

	//while (!fwl_isInputThreadInitialized()) {
	//	usleep(50);
	//}

	fwl_initializeTextureThread();
	//set_thread2global(tg, tg->threads.loadThread ,"texture loading thread");
	//while (!fwl_isTextureinitialized()) {
	//	usleep(50);
	//}
	/* Hmm. display_initialize is really a frontend function. The frontend should call it before calling _displayThread */
	/* Initialize display */



	return TRUE;
}


/**
 *   startFreeWRL: we set up the main file / world
 *                 in the main() of the program, then
 *                 we call this routine after threads
 *                 initialization.
 */
void splitpath_local_suffix(const char *url, char **local_name, char **suff)
{
	//takes a http or file path, and gives back just the scene name and suffix
	//ie file://E:/tests/1.wrl -> local_name = "1" suff = "wrl"
	*local_name = NULL;
	*suff = NULL;
	if(url){
		int i,len;
		char *localname;
		len = (int) strlen(url);
		localname = NULL;
		for(i=len-1;i>=0;i--){
			if(url[i] == '/') break;
			localname = (char*)&url[i];
		}
		if(localname){
			*local_name = STRDUP(localname);
			localname = *local_name;
			len = (int) strlen(localname);
			*suff = NULL;
			for(i=len-1;i>=0;i--){
				if(localname[i] == '.') {
					localname[i] = '\0';
					*suff = &localname[i+1];
					break;
				}
			}
		}
	}
}
void fwl_startFreeWRL(const char *url)
{

	//ConsoleMessage ("yes, really, FWL_STARTFREEWRL called is called\n");

	/* Give the main argument to the resource handler */
	if (url != NULL) {
		//int i,len;
		//const char *localname;
		//len = strlen(url);
		//localname = NULL;
		//for(i=len-1;i>=0;i--){
		//	if(url[i] == '/') break;
		//	localname = &url[i];
		//}
		//if(localname){
		//	char* suff;
		//	char* local_name = STRDUP(localname);
		//	len = strlen(local_name);
		//	suff = NULL;
		//	for(i=len-1;i>=0;i--){
		//		if(local_name[i] == '.') {
		//			local_name[i] = '\0';
		//			suff = &local_name[i+1];
		//			break;
		//		}
		//	}
		//	gglobal()->Mainloop.url = strdup(url);
		//	gglobal()->Mainloop.scene_name = local_name;
		//	gglobal()->Mainloop.scene_suff = suff;
		//}
		char* suff = NULL;
		char* local_name = NULL;
		splitpath_local_suffix(url, &local_name, &suff);
		if(url) gglobal()->Mainloop.url = strdup(url);
		gglobal()->Mainloop.scene_name = local_name;
		gglobal()->Mainloop.scene_suff = suff;


		//file = stripLocalFileName ((char *)file);
		//FREE_IF_NZ (BrowserFullPath);
		//BrowserFullPath = STRDUP((char *) file);

		fwl_resource_push_single_request(url);
		DEBUG_MSG("request sent to parser thread, main thread joining display thread...\n");
	} else {
		DEBUG_MSG("no request for parser thread, main thread joining display thread...\n");
	}
#ifndef GLES2
	/* now wait around until something kills this thread. */
	pthread_join(gglobal()->threads.DispThrd, NULL);
#endif
}

/**
 * Explicit exit routine
 */
void closeFreeWRL()
{
}
