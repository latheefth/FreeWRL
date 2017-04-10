/*

  FreeWRL support library.
  Plugin interaction.

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
#include "../threads.h"

#include <libFreeWRL.h>
#include <list.h>
#include <io_files.h>
#include <signal.h>

#include "../vrml_parser/Structs.h"
#include "../main/headers.h"
#include "../main/MainLoop.h" /* for fwl_replaceWorldNeededRes() */
#include "../input/EAIHeaders.h"	/* for implicit declarations */

#include "../x3d_parser/Bindable.h"
#include "../opengl/OpenGL_Utils.h"
#include "../scenegraph/RenderFuncs.h"
#include "../main/ProdCon.h"
#include "pluginUtils.h"
#include "PluginSocket.h"

#define UNUSED(v) ((void) v)

static int checkIfX3DVRMLFile(char *fn);
// OLDCODE static int checkIfHTMLFile(char *fn);

/* get all system commands, and pass them through here. What we do
 * is take parameters and execl them, in specific formats, to stop
 * people (or, to try to stop) from typing malicious code. */

/* keep a list of children; if one hangs, fwl_doQuit will hang, also. */
#ifndef _MSC_VER
#define MAXPROCESSLIST 128
pid_t childProcess[MAXPROCESSLIST];
int lastchildProcess = 0;
int childProcessListInit = FALSE;
#else
#include <process.h>
#endif

typedef struct ppluginUtils{
	/* we keep polling here, if we are loading a url...*/
	int waitingForURLtoLoad;// = FALSE;
	resource_item_t *plugin_res;// = NULL; 	/* If this res is valid, then we can replace root_res with it */
}* pppluginUtils;
void *pluginUtils_constructor(){
	void *v = MALLOCV(sizeof(struct ppluginUtils));
	memset(v,0,sizeof(struct ppluginUtils));
	return v;
}
void pluginUtils_init(struct tpluginUtils *t){
	//public
	//private
	t->prv = pluginUtils_constructor();
	{
		pppluginUtils p = (pppluginUtils)t->prv;
		/* we keep polling here, if we are loading a url...*/
		p->waitingForURLtoLoad = FALSE;
		p->plugin_res = NULL; 	/* If this res is valid, then we can replace root_res with it */
	}
}



void killErrantChildren(void) {
#ifndef _MSC_VER
	int count;
	
	for (count = 0; count < MAXPROCESSLIST; count++) {
		if (childProcess[count] != 0) {
			/* printf ("trying to kill %d\n",childProcess[count]); */
			/* http://www.opengroup.org/onlinepubs/000095399/functions/kill.html */
			kill (childProcess[count],SIGINT);
		}
	}
#endif
}

/* implement Anchor/Browser actions */

void goToViewpoint(char *vp) {
	struct X3D_Node *localNode;
	/* unused int tableIndex; */
	int flen;
	struct tProdCon *t = &gglobal()->ProdCon;

	/* see if we can get a node that matches this DEF name */
	localNode = EAI_GetViewpoint(vp);
	
	/*  did we find a match with known Viewpoints?*/
	if (localNode != NULL) {
		for (flen=0; flen<vectorSize(t->viewpointNodes);flen++) {
			if (localNode == vector_get(struct X3D_Node *,t->viewpointNodes,flen)) {
				struct X3D_Viewpoint *vp;
				/* unbind current, and bind this one */
				vp = (struct X3D_Viewpoint*)vector_get(struct X3D_Node *,t->viewpointNodes,t->currboundvpno);
				send_bind_to((struct X3D_Node*)vp,0);
				t->currboundvpno=flen;
				vp = (struct X3D_Viewpoint *)vector_get(struct X3D_Node *,t->viewpointNodes,t->currboundvpno);
				send_bind_to((struct X3D_Node*)vp,1);
				return;
			}
		}
	}
	/* printf ("goToViewpoint - failed to match local Viewpoint\n"); */
}

#if !defined(_MSC_VER) && !defined(ANDROIDNDK)
void startNewHTMLWindow(char *url) {
	const char *browser;
#define LINELEN 4000
#define ERRLINELEN 4200
	char sysline[LINELEN];
	int sysReturnCode;
	char syslineFailed[ERRLINELEN];
	int testlen;

	browser = NULL;

// OLD_IPHONE_AQUA #ifdef AQUA
// OLD_IPHONE_AQUA 	if (RUNNINGASPLUGIN) {
// OLD_IPHONE_AQUA 		/* printf ("Anchor, running as a plugin - load non-vrml file\n"); */
// OLD_IPHONE_AQUA 		requestNewWindowfromPlugin(_fw_browser_plugin, _fw_instance, url);
// OLD_IPHONE_AQUA 	} else {
// OLD_IPHONE_AQUA #endif

	browser = freewrl_get_browser_program();
	if (!browser) {
		ConsoleMessage ("Error: no Internet browser found.");
		return;
	}
		
	/* bounds check here */
	testlen = 0;
	if (browser) testlen = (int) strlen(browser);

	testlen += (int) strlen(url) + 10; 
	if (testlen > LINELEN) {
		ConsoleMessage ("Anchor: combination of browser name and file name too long.");
	} else {
			
		if (browser) strcpy (sysline, browser);
		else strcpy (sysline, browser);
		strcat (sysline, " ");
		strcat (sysline, url);
		strcat (sysline, " &");
		freewrlSystem (sysline);
	}
		

	if (browser) sprintf(sysline, "%s %s &", browser, url);
	else sprintf(sysline, "open %s &",  url);
	sysReturnCode = system (sysline);
	if (sysReturnCode < 0) {
		sprintf(syslineFailed ,"ERR %s %d system call failed, returned %d. Was: %s\n",__FILE__,__LINE__,sysReturnCode,sysline);
		ConsoleMessage (syslineFailed);
	}
// OLD_IPHONE_AQUA #ifdef AQUA
// OLD_IPHONE_AQUA 
// OLD_IPHONE_AQUA 	}
// OLD_IPHONE_AQUA #endif
}
#endif


///* we keep polling here, if we are loading a url...*/
//static int waitingForURLtoLoad = FALSE;
//static resource_item_t *plugin_res = NULL; 	/* If this res is valid, then we can replace root_res with it */

static int urlLoadingStatus() {
	pppluginUtils p = (pppluginUtils)gglobal()->pluginUtils.prv;

	/* printf ("urlLoadingStatus %s\n",resourceStatusToString(plugin_res->status)); */
	/* printf ("and we have %d children in root.\n",X3D_GROUP(rootNode)->children.n); */

	switch (p->plugin_res->status) {
		case ress_downloaded:
		case ress_parsed:
			EAI_Anchor_Response(TRUE);
			p->waitingForURLtoLoad = FALSE;
			break;
		case ress_failed:
			ConsoleMessage ("Failed to load URL\n");
			EAI_Anchor_Response(FALSE);
			p->waitingForURLtoLoad = FALSE;
			break;
		default: {}
	}

	return p->waitingForURLtoLoad;
}

/* returns FALSE if we are DONE the action, whether or not it was successful; 
   TRUE if we want to hit this next time through the event loop */

int doBrowserAction()
{
	int i;
	struct Multi_String Anchor_url;
	char *description;
	resource_item_t * parentPath;
	pppluginUtils p;
	ttglobal tg = gglobal();
	p = (pppluginUtils)tg->pluginUtils.prv;
	

	/* are we in the process of polling for a new X3D URL to load? */
	if (p->waitingForURLtoLoad) return urlLoadingStatus();

	UNUSED(description);  // compiler warning mitigation

	if (AnchorsAnchor() != NULL) {

		Anchor_url = AnchorsAnchor()->url;
		description = AnchorsAnchor()->description->strptr;

		TRACE_MSG("doBrowserAction: description: %s\n", description);

		/* are we going to load up a new VRML/X3D world, or are we going to just go and load up a new web page ? */
		if (Anchor_url.n <= 0) {
			/* printf ("have Anchor, empty URL\n"); */
			setAnchorsAnchor( NULL );
			return FALSE; /* done the action, the url is just not good */
		} 

		/*  June 2016 goal: make frontend handle .html anchors, so it can be done in a platform-specific way
			x we can't handle mixed anchors (see next comment)
			* so we still need to determine media_type here: 
				scene, external (ie html - for frontend to do external anchor), or viewpoint.
			
			[future: allow mixed anchors: Anchor url [ "missing.wrl" "bad.exe" "funny.html" "#viewpoint" ]
			and have resource.c iterate over SF till one loads. 
			Problem for future: it takes us several steps to unload current scene, and those steps conflict 
			with any attempt to load a new scene at the same time. So in a resource.c/ProdCon.c driven 
			mixed anchor, if we suddenly find a scene succeeds download, we can't properly unload 
			current scene, due to all the perl arrays and implicit 'bindings' ie one global route array,
			one global node array, one global texture array, navigation last drag locations in scene rather than mouse coords,
			and the resource and parsing queues that will think current scene parts are for the new scene.
			Ideally all those implicit bindings would be hot-swappable, resources would know their scene, so when 
			the are dequeued if their scene is gone they'll be disposed - 
			so a new scene can be parsed while old one running, and swapped in ProdCon after parse, or on next frame.]
		*/


		/* first test case - url is ONLY a viewpoint change */
		if (Anchor_url.p[0]->strptr[0] == '#') {
			setAnchorsAnchor( NULL );
			fwl_gotoViewpoint(&(Anchor_url.p[0]->strptr[1]));
			return TRUE;
		}
		/* We have a url, lets go and get the first one of them */
		parentPath = (resource_item_t *)AnchorsAnchor()->_parentResource;
		p->plugin_res = resource_create_multi0(&AnchorsAnchor()->url);
#define DO_THIS_BLOC 1
#ifdef DO_THIS_BLOC //EXPERIMENT_FAILED
		{
			/*Goal: avoid blocking UI thread
			  Method: schedule a scene change whereby the file part gets sent to the resource worker thread
			  while the UI thread keeps looping
			  - first, make sure it's a scene file (not .html, .img which are handled in the html browser in new window)
			*/
			BOOL isScene; //, isHTML; 

			// June 2016 change: avoid mixed anchors, must be uniform ie all SF are scene, or all SF are html
			// for it to be scene, all need to be scene (could split out non-scene to get scene-only MF / non-mixed anchor)
			isScene = TRUE; //was FALSE
			for (i = 0; i < Anchor_url.n; i++)
				isScene = isScene && checkIfX3DVRMLFile(Anchor_url.p[i]->strptr); // was ||
			if (isScene){
				resource_identify(parentPath, p->plugin_res);
				fwl_replaceWorldNeededRes(p->plugin_res);
				return FALSE;
			}
			// June 2016: if not scene or viewpoint, it might be html, plain images, audio
			// send to frontend for platform-specific anchoring to web browser
			p->plugin_res->actions = resa_download; //just to get it to the frontend
			p->plugin_res->media_type = resm_external;
			p->plugin_res->type = rest_multi;
			resource_identify(parentPath, p->plugin_res);
			resitem_enqueue(ml_new(p->plugin_res)); 
		}
#else //EXPERIMENT
	
#ifdef TEXVERBOSE
		PRINTF("url: ");
		Multi_String_print(&AnchorsAnchor()->url);
		PRINTF("parent resource: \n");
		resource_dump(parentPath);
		PRINTF("file resource: \n");
		resource_dump(p->plugin_res);
#endif
		{
		s_list_t *head_of_list;

		/* hold on to the top of the list so we can delete it later */
		head_of_list = p->plugin_res->m_request;

		/* go through the urls until we have a success, or total failure */
		do {
			/* Setup parent */
			resource_identify(parentPath, p->plugin_res);

			/* Setup media type */
			p->plugin_res->media_type = resm_image; /* quick hack */

			if (resource_fetch(p->plugin_res)) {
				/* printf ("really loading anchor from %s\n", plugin_res->actual_file); */

				/* we have the file; res->actual_file is the file on the local system; 
				   plugin_res->parsed_request is the file that might be remote */

				if (checkIfX3DVRMLFile(p->plugin_res->actual_file)) {
					resource_item_t *resToLoad;

					/* out with the old... */
					kill_oldWorld(TRUE,TRUE,__FILE__,__LINE__);

					/* tell the new world which viewpoint to go to */
					//fwl_gotoViewpoint (p->plugin_res->afterPoundCharacters);
					resToLoad = resource_create_single(p->plugin_res->actual_file);
					resToLoad->afterPoundCharacters = p->plugin_res->afterPoundCharacters;
					/* in with the new... */
					send_resource_to_parser(resToLoad);
					p->waitingForURLtoLoad = TRUE;
					return TRUE; /* keep the browser ticking along here */
				} else {
#ifdef _MSC_VER				
					resource_item_t *resToLoad;
					//we don't want to launch a new IE browser or IE tab, 
					//just load a new scene in freewrl
					//analogous to what happens when we have file://...AnchorA.x3d 
					//the following is the only way I know to do that right now, same as 
					//below several lines:
					kill_oldWorld(TRUE,TRUE,__FILE__,__LINE__);

					/* we want to clean out the old world AND load a new one in */

					resToLoad = resource_create_single(p->plugin_res->parsed_request);
					resToLoad->afterPoundCharacters = p->plugin_res->afterPoundCharacters;

					send_resource_to_parser(resToLoad);

					p->waitingForURLtoLoad = TRUE;
					return TRUE; /* keep the browser ticking along here */
#else
					p->plugin_res->complete = TRUE;
					startNewHTMLWindow(p->plugin_res->parsed_request);
#endif
				}
			} else {
				/* we had a problem with that URL, set this so we can try the next */
				p->plugin_res->type=rest_multi;
			}
		} while ((p->plugin_res->status != ress_downloaded) && (p->plugin_res->m_request != NULL));

		/* destroy the m_request, if it exists */
		if (head_of_list != NULL) {
			ml_delete_all(head_of_list);
		}

		/* were we successful?? */
		if (p->plugin_res->status != ress_loaded) {
			ERROR_MSG("Could not load new world: %s\n", p->plugin_res->actual_file);
			return FALSE;
		}
		}
#endif //EXPERIMENT

/*********************************************************/


	} else {
		/* printf ("\nwe have a single replacement here\n"); */
	}
	return FALSE; /* we are done the action */
}

/*
 * Check to see if the file name is a geometry file.
 * return TRUE if it looks like it is, false otherwise
 *
 * This should be kept in line with the plugin register code in
 * Plugin/netscape/source/npfreewrl.c
 */

static int checkIfX3DVRMLFile(char *fn) {
	if ((strstr(fn,".wrl") > 0) ||
		(strstr(fn,".WRL") > 0) ||
		(strstr(fn,".x3d") > 0) ||
		(strstr(fn,".x3z") > 0) ||
		(strstr(fn,".x3dv") > 0) ||
		(strstr(fn,".x3db") > 0) ||
		(strstr(fn,".X3DV") > 0) ||
		(strstr(fn,".X3DB") > 0) ||
		(strstr(fn,".X3D") > 0)) {
		return TRUE;
	}
	return FALSE;
}
// OLDCODE static int checkIfHTMLFile(char *fn) {
// OLDCODE 	if ((strstr(fn,".html") > 0) ||
// OLDCODE 		(strstr(fn,".HTML") > 0) 
// OLDCODE 		) {
// OLDCODE 		return TRUE;
// OLDCODE 	}
// OLDCODE 	return FALSE;
// OLDCODE }


void new_root();
/* we are an Anchor, and we are not running in a browser, and we are
 * trying to do an external VRML or X3D world.
 */
/* void Anchor_ReplaceWorld (char *name) */
bool Anchor_ReplaceWorld(const char *name)
{
	gglobal()->Mainloop.replaceWorldRequest = STRDUP(name);
	//resource_item_t *AR_res;

	////ConsoleMessage ("Anchor_ReplaceWorld called\n");

	//AR_res = resource_create_single(name);
	////AR_res->new_root = TRUE;
	//new_root();
	//send_resource_to_parser(AR_res);
	//resource_wait(AR_res);

	//if (AR_res->status != ress_loaded) {
	//	/* FIXME: destroy this new node tree */
	//	return FALSE;
	//}
	return TRUE;
}

/* send in a 0 to 15, return a char representation */
char tohex (int mychar) {
	if (mychar <10) return (mychar + '0');
	else return (mychar - 10 + 'A');
}

/* should this character be encoded? */
int URLmustEncode(int ch) {
	if (
		   (ch == 0x20) 
		|| (ch == 0x22) 
		|| (ch == 0x3c) 
		|| (ch == 0x3e) 
		|| (ch == 0x23) 
		|| (ch == 0x25)
		|| (ch == 0x7b) 
		|| (ch == 0x7d) 
		|| (ch == 0x7c) 
		|| (ch == 0x5c) 
		|| (ch == 0x5e) 
		|| (ch == 0x7e) 
		|| (ch == 0x5b) 
		|| (ch == 0x5d) 
		|| (ch == 0x60)) return TRUE;
	return FALSE;
} 


/***************/
/* static FILE * tty = NULL; */

/* prints to a log file if we are running as a plugin */
void URLprint (const char *m, const char *p) {
#undef URLPRINTDEBUG
#ifdef URLPRINTDEBUG
	if (tty == NULL) {
		tty = fopen("/home/luigi/logURLencod", "w");
		if (tty == NULL)
			abort();
		fprintf (tty, "\nplugin restarted\n");
	}

	fprintf (tty,"%f URLprint: ",TickTime());
	fprintf(tty, m,p);
	fflush(tty);
#endif
}

/* loop about waiting for the Browser to send us some stuff. */
/* Change a string to encode spaces for getting URLS. */
void URLencod (char *dest, const char *src, int maxlen) {
	int mylen;
	int sctr;
	int destctr;
	int curchar;

#ifdef URLPRINTDEBUG
	char *orig;

	orig = dest;

	/* get the length of the source and bounds check */
	URLprint ("going to start URLencod %s\n","on a string");
	URLprint ("start, src is %s\n",src);
	/* URLprint ("maxlen is %d\n",maxlen); */
#endif

	destctr = 0; /* ensure we dont go over dest length */
	mylen = (int) strlen(src);
	if (mylen == 0) {
		dest[0]= '\0';
		return;
	}
	
	/* encode the string */
	for (sctr = 0; sctr < mylen; sctr ++) {
		curchar = (int) *src; src++;	
		/* should we encode this one? */

                if (URLmustEncode(curchar)) { 
			*dest = '%'; dest ++;
			*dest = tohex ((curchar >> 4) & 0x0f); dest++;
			*dest = tohex (curchar & 0x0f); dest++;
			destctr +=3;
		} else {
			*dest = (char) curchar; destctr++; dest++;
		}
	


		/* bounds check */
		if (destctr > (maxlen - 5))  {
			*dest = '\0';
			return;
		}
	}
	*dest = '\0'; /* null terminate this one */
#ifdef URLPRINTDEBUG
	URLprint ("encoded string is %s\n",orig);
#endif

}

/* this is for Unix only */
// OLD_IPHONE_AQUA #if !defined(AQUA) && !defined(_MSC_VER) && !defined(_ANDROID) && !defined(ANDROIDNDK) && !defined(GLES2)
#if !defined(_MSC_VER) && !defined(_ANDROID) && !defined(ANDROIDNDK) && !defined(GLES2)

void sendXwinToPlugin()
{
	int writeSizeThrowAway ;
	UNUSED(writeSizeThrowAway); // compiler warning mitigation

	/* send the window id back to the plugin parent */
	DEBUG_MSG("Executing sendXwinToPlugin...\n");

#if KEEP_X11_INLIB
	XWindowAttributes mywin;

        XGetWindowAttributes(Xdpy,Xwin, &mywin);
        DEBUG_MSG("sendXwinToPlugin: sendXwin starting, mapped_state %d, IsUnmapped %d, isUnviewable %d isViewable %d\n",mywin.map_state, IsUnmapped, IsUnviewable, IsViewable);

	DEBUG_MSG("sendXwinToPlugin: sending Xwin ID back to plugin - %lu bytes\n",sizeof (Xwin));

	writeSizeThrowAway = write (_fw_pipe,&Xwin,sizeof(Xwin));
	close (_fw_pipe);

	/* wait for the plugin to change the map_state */
        XGetWindowAttributes(Xdpy,Xwin, &mywin);
	
	while (mywin.map_state == IsUnmapped) {
		usleep (100);
        	XGetWindowAttributes(Xdpy,Xwin, &mywin);
		#ifdef URLPRINTDEBUG
        	printf ("sendXwin in sleep loope, mapped_state %d, IsUnmapped %d, isUnviewable %d isViewable %d\n",mywin.map_state, IsUnmapped, IsUnviewable, IsViewable);
		#endif

	}

	#ifdef URLPRINTDEBUG
        XGetWindowAttributes(Xdpy,Xwin, &mywin);
        printf ("sendXwin at end,  mapped_state %d, IsUnmapped %d, isUnviewable %d isViewable %d\n",mywin.map_state, IsUnmapped, IsUnviewable, IsViewable);
        printf ("x %d y %d wid %d height %d\n",mywin.x,mywin.y,mywin.width,mywin.height);
	#endif
#endif /* KEEP_X11_INLIB */

}
#endif

