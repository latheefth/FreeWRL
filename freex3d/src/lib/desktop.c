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

/*desktop.c theme: traditional desktop- and browser plugin-specific code
including former !FRONTEND_HANDLES_DISPLAY_THREAD and !FRONTEND_GETS_FILES code
(versus: sandbox configs like android, ios, winRT where the frontend has its own displaythread and io_http>download_url )
The functions in here emulate FEGF+FEHDT for desktop, so desktop works like sandbox apps
- desktop io_http > download_url is called from in here
- _displayThread loop is created and run in here
- desktop console-program startup
- desktop browser-plugin startup
- dequeue_get_enqueue called once per displaythread to dequeue URL from backend, 
	do URL2BLOB = URL2FILE+FILE2BLOB by spawning download, load tasks,
	and enqueuing the BLOB results to the backend

Tips for freewrl developers:
 don't call resource_fetch or download_url() -which are synchronous calls -from backend modules 
 (it took a lot of work to get them out of there) instead develop 'async' so the frontend can deliver
 results whenever downloads arrive.
 externProto: If you are trying to repair tests 17.wrl, 17.x3d, or externProto
	you should refactor externProto parsing and runtime code to allow asynchronous/delay loading
	of externProto definitions.

EMULATING FEGF in ML - more detail..
terminology
FE - frontend
BE - backend
ML - middle layer
FEGF (FRONTEND GETS FILES)
	- sandboxed Android, IOS and winRT download internet resources in frontend code, in asynchronous tasks so as not to block the UI thread.
	- (versus desktop MLGF - see code below and io_http.c)
FEHDT (FRONTEND_HANDLES_DISPLAY_THREAD)
	- android, winRT run their own displaythread, and own the swapbuffers and opengl/directx context
	- (versus desktop console and plugins that launch a displaythread which loops and does swapbuffers -see _displayThread below)
BLOB - binary large object - a text string blob for vrml/x3d
TEXBLOB - width,height,RGBA blob for textures
Scenery: vrml/x3d for main scene, inline, externProto, string
Imagery: image files .jpg, .ico, .png etc
Scripts: .js, shader

BE (BackEnd) worker threads:
Scenery and scripts: _inputParsingThread()
Imagery: _textureThread()

download2local - downloads an internet file to a local file
local2blob - reads a local disk file into BLOB or TEXBLOB
INITSCENE = {URL -> BE -> initScene -> enqueueURL}
-- killOldworld, new resItem with baseURL pushed on stack to populate _parentResource fields
-- URL enqueued for worker thread (which may in turn enqueue it for FE thread's URL2LOCAL)
FILE2BLOB = {localURL -> ML -> local2blob -> enqueueBlob -> BE}
W3DX - web3d stage .zip^^

Typical workflows for WinRT FEGF configuration:
1. URLBox -> INITSCENE -> FE -> URL? download2Local -> FILE2BLOB
2. Picker/OpenWith -> AccessCache -> INITSCENE -> FE -> cacheLookup -> found ? use : filePicker -> download/copy2Local -> FILE2BLOB
3. Picker/OpenWith -> W3DX? -> ML -> localStage -> manifest -> mainsceneURL -> INITSCENE -> ML-> manifestLookup -> FILE2BLOB

Desktop configurations would be refactored to move URL2LOCAL AND LOCAL2BLOB into the ML:
- resource push, pop at end of io_http.c would be moved to resources.c
- io_files.c and io_http.c would be moved to ML -out of the core library
- core library would work in URLs and BLOBS
- to avoid yet more threads to run the ML, the BE would enqueue a task with a function pointer for URL2LOCAL, for each URL it enqueues, if the function pointer is non-null it runs it, else does nothing. ML would populate the function pointer. After ML does URL2LOCAL, if successful, it enqueus another function task LOCAL2BLOB which the worker thread runs.

Android may be diskless for some types of files -no local file intermediary- and if so does URL2BLOB in one step in the FE.
Some different possible workflows for different configurations and scenarios:
1 ------------------------------------------(BLOB) ------------P-> SCENERY || JS || SHADER
2 --------------------------LOCAL FILE -L-> (BLOB || TEXBLOB) -P-> SCENERY || JS || SHADER || TEXTURE
3 URL --D-----------------> LOCAL FILE -L-> (BLOB || TEXBLOB) -P-> SCENERY || JS || SHADER || TEXTURE
4 URL --D-> LOCAL ZIP --U-> LOCAL FILE -L-> (BLOB || TEXBLOB) -P-> SCENERY || JS || SHADER || TEXTURE
5 ----------LOCAL ZIP --U-> LOCAL FILE -L-> (BLOB || TEXBLOB) -P-> SCENERY || JS || SHADER || TEXTURE
6 URL ---------D+L------------------------> (BLOB || TEXBLOB) -P-> SCENERY || JS || SHADER || TEXTURE

D-download, U-unzip, L-load, I-convert to Image w,h,rgba, P-Process: parse/texture
BLOB and TEXBLOB are the common/goal states
Two workflows for TEXBLOB:
LI-> (TEXBLOB) -P-> TEXTURE
- when your image libary loads from file
L-> (BLOB) I (TEXBLOB) -P-> TEXTURE
- when your image library loads from BLOB

I think I did .x3z + minizip in the wrong layer. It should be in a middle layer, not in the BE. You would never Anchor to a .x3z or .w3dx.

^^
Stage is a missing concept from web3d.org. A scene file, with all its .js files, its inline/extern Proto and anchor scenes aka scenery, 
all its imagery, as a self-contained set or 'stage'
- like .x3z minizip bundling of doc.x3d + images, except more general
- freewrl could be modified to aggregate files it downloads into a local cache of files, and add w3dx.manifest 
  that would list the original URL of the mainscene, and its stage relative path, and a lookup table of all
  the other resources original URLs and their stage relative path. Then zip it and rename it to .w3dx.
RULE: the BE must never know. ML unzips, gets the mainscene original URL and passes to the BE. The BE then requests
  the mainscene by it's original URL, and ML looks it up in the manifest table, gets the stage-relative path,
  and prepends the path of the stage.
w3dx.manifest:
<stage>
<file type="mainscene" url="" path=""/>
<file type="scene" url="" path=""/>
<file type="scenery" url="" path=""/>
<file type="imagery" url="" path=""/>
...
</stage>

*/

#include <config.h>
#include <system.h>
#include <system_threads.h>
#include <resources.h>
#include <internal.h>
#include <io_http.h>
#include "main/MainLoop.h"

/**
 *   resource_fetch: download remote url or check for local file access.

 */
//#define DEBUG_RES printf
bool resource_fetch(resource_item_t *res)
{
	//char* pound;
	DEBUG_RES("fetching resource: %s, %s resource %s\n", resourceTypeToString(res->type), resourceStatusToString(res->status) ,res->URLrequest);

	ASSERT(res);

	switch (res->type) {

	case rest_invalid:
		res->status = ress_invalid;
		ERROR_MSG("resource_fetch: can't fetch an invalid resource: %s\n", res->URLrequest);
		break;

	case rest_url:
		switch (res->status) {
		case ress_none:
		case ress_starts_good:
			DEBUG_RES ("resource_fetch, calling download_url\n");
			//pound = NULL;
			//pound = strchr(res->parsed_request, '#');
			//if (pound != NULL) {
			//	*pound = '\0';
			//	/* copy the name out, so that Anchors can go to correct Viewpoint */
			//	pound++;
			//	res->afterPoundCharacters = STRDUP(pound);
			//}

			download_url(res);
			break;
		default:
			/* error */
			break;
		}
		break;

	case rest_file:
		switch (res->status) {
		case ress_none:
		case ress_starts_good:
			/* SJD If this is a PROTO expansion, need to take of trailing part after # */
			//pound = NULL;
			//pound = strchr(res->parsed_request, '#');
			//if (pound != NULL) {
			//	*pound = '\0';
			//}
				
//#if defined(FRONTEND_GETS_FILES)
//ConsoleMessage ("ERROR, should not be here in rest_file");
//#else

			if (do_file_exists(res->parsed_request)) {
				if (do_file_readable(res->parsed_request)) {
					res->status = ress_downloaded;
					res->actual_file = STRDUP(res->parsed_request);
					//if (pound != NULL) {
					//	/* copy the name out, so that Anchors can go to correct Viewpoint */
					//	pound ++;
					//	res->afterPoundCharacters = STRDUP(pound);
					//}
				} else {
					res->status = ress_failed;
					ERROR_MSG("resource_fetch: wrong permission to read file: %s\n", res->parsed_request);
				}
			} else {
				res->status = ress_failed;
				ERROR_MSG("resource_fetch: can't find file: %s\n", res->parsed_request);
			}
//#endif //FRONTEND_GETS_FILES

			break;
		default:
			/* error */
			break;
		}
		break;

	case rest_multi:
	case rest_string:
		/* Nothing to do */
		break;
	}
	DEBUG_RES ("resource_fetch (end): network=%s type=%s status=%s"
		  " request=<%s> base=<%s> url=<%s> [parent %p, %s]\n", 
		  BOOL_STR(res->network), resourceTypeToString(res->type), 
		  resourceStatusToString(res->status), res->URLrequest, 
		  res->URLbase, res->parsed_request,
		  res->parent, (res->parent ? res->parent->URLbase : "N/A"));
	return (res->status == ress_downloaded);
}


void frontenditem_enqueue_tg(s_list_t *item, void *tg);
s_list_t *frontenditem_dequeue_tg(void *tg);
s_list_t *frontenditem_dequeue();

int checkReplaceWorldRequest();
int checkExitRequest();
enum {
	url2file_task_chain,
	url2file_task_spawn,
} url2file_task_tactic;

enum {
	file2blob_task_chain,
	file2blob_task_spawn,
	file2blob_task_enqueue,
} file2blob_task_tactic;


int file2blob(resource_item_t *res);
int url2file(resource_item_t *res){
	int retval = 0;
	int more_multi;
	resource_fetch(res); //URL2FILE
	//Multi_URL loop moved here (middle layer ML), 
	more_multi = (res->status == ress_failed) && (res->m_request != NULL);
	if(more_multi){
		//still some hope via multi_string url, perhaps next one
		res->status = ress_invalid; //downgrade ress_fail to ress_invalid
		res->type = rest_multi; //should already be flagged
		//must consult BE to convert relativeURL to absoluteURL via baseURL 
		//(or could we absolutize in a batch in resource_create_multi0()?)
		resource_identify(res->parent, res); //should increment multi pointer/iterator
		retval = 1;
	}else if(res->status == ress_downloaded){
		//queue for loading
		retval = 1;
	}
	return retval;
}

void file2blob_task(s_list_t *item);
extern int async_thread_count;
static void *thread_download_async (void *args){
	int downloaded; //, tactic;
	s_list_t *item = (s_list_t *)args;
	resource_item_t *res = (resource_item_t *)item->elem;
	async_thread_count++;
	printf("{%d}",async_thread_count);
	if(fwl_setCurrentHandle(res->tg, __FILE__, __LINE__));

	downloaded = url2file(res);

	//tactic = file2blob_task_chain;
	if(downloaded)
		file2blob_task(item); //ml_new(res));
	else
		resitem_enqueue(item); //for garbage collection
	async_thread_count--;
	return NULL;
}
void downloadAsync (s_list_t *item) {
	resource_item_t *res = (resource_item_t *)item->elem;
	if(!res->_loadThread) res->_loadThread = malloc(sizeof(pthread_t));
	pthread_create ((pthread_t*)res->_loadThread, NULL,&thread_download_async, (void *)item);
}


//this is for simulating frontend_gets_files in configs that run _displayThread: desktop and browser plugins
//but can be run from any thread as long as you know the freewrl instance/context/tg/gglobal* for the resitem and frontenditem queues, replaceWorldRequest etc
#define MAX_SPAWNED_PER_PASS 15  //in desktop I've had 57 spawned threads at once, with no problems. In case there's a problem this will limit spawned-per-pass, which will indirectly limit spawned-at-same-time
void frontend_dequeue_get_enqueue(void *tg){
	int count_this_pass;
	s_list_t *item = NULL;
	resource_item_t *res = NULL;
	fwl_setCurrentHandle(tg, __FILE__, __LINE__); //set the freewrl instance - will apply to all following calls into the backend. This allows you to call from any thread.
	count_this_pass = 0; //approximately == number of spawned threads running at one time when doing file2blob_task_spawn
	while( max(count_this_pass,async_thread_count) < MAX_SPAWNED_PER_PASS && !checkExitRequest() && !checkReplaceWorldRequest() && (item = frontenditem_dequeue()) != NULL ){
		count_this_pass++;
		//download_url((resource_item_t *) item->elem);
		res = item->elem;
		if(res->status != ress_downloaded){
			int tactic = url2file_task_spawn;//url2file_task_spawn;
			if(tactic == url2file_task_chain){
				int more_multi;
				resource_fetch(res); //URL2FILE
				//Multi_URL loop moved here (middle layer ML), 
				more_multi = (res->status == ress_failed) && (res->m_request != NULL);
				if(more_multi){
					//still some hope via multi_string url, perhaps next one
					res->status = ress_invalid; //downgrade ress_fail to ress_invalid
					res->type = rest_multi; //should already be flagged
					//must consult BE to convert relativeURL to absoluteURL via baseURL 
					//(or could we absolutize in a batch in resource_create_multi0()?)
					resource_identify(res->parent, res); //should increment multi pointer/iterator
					frontenditem_enqueue(item);
				}
			}else if(tactic == url2file_task_spawn){
				downloadAsync(item); //res already has res->tg with global context
			}
		}
		if(res->status == ress_downloaded){
			file2blob_task(item);
		}
	}
	//fwl_clearCurrentHandle(); don't unset, in case we are in a BE/ML thread ie _displayThread
}

#ifdef SSR_SERVER
void SSR_reply(void * tg);
void dequeue_SSR_request(void * tg);
#endif

void _displayThread(void *globalcontext)
{
	/* C CONTROLLER - used in configurations such as C main programs, and browser plugins with no loop of their own
	- usually the loop, create gl context, and swapbuffers stay together in the same layer
	MVC - Model-View-Controller design pattern: do the Controller activities here:
	1) tell Model (scenegraph, most of libfreewrl) to update itself
	2) tell View to poll-model-and-update-itself (View: GUI UI/statusbarHud/Console)
	-reason for MVC: no callbacks are needed from Model to UI(View), so easy to change View
	-here everything is in C so we don't absolutely need MVC style, but we are preparing MVC in C
	to harmonize with Android, IOS etc where the UI(View) and Controller are in Objective-C or Java and Model(state) in C

	Non-blocking UI thread - some frontends don't allow you to block the display thread. They will be in
	different code like objectiveC, java, C#, but here we try and honor the idea by allowing
	looping to continue while waiting for worker threads to flush and/or exit by polling the status of the workers
	rather than using mutex conditions.
	*/
	int more;
	fwl_setCurrentHandle(globalcontext, __FILE__, __LINE__);
	ENTER_THREAD("display");

	do{
		//if(frontendGetsFiles()==2) 
#ifdef SSR_SERVER
		SSR_reply(globalcontext);
		dequeue_SSR_request(globalcontext);
#endif
		frontend_dequeue_get_enqueue(globalcontext); //this is non-blocking (returns immediately) if queue empty
		more = fwl_draw();
		/* swap the rendering area */
		if(more)
			FW_GL_SWAPBUFFERS;
	} while (more);
	// moved to fwl_draw for disabler finalizeRenderSceneUpdateScene(); //Model end
	//printf("Ending display thread gracefully\n");
	return;
}
#ifdef _MSC_VER
void sync();
#endif
//#if !defined (FRONTEND_HANDLES_DISPLAY_THREAD)
void fwl_initializeDisplayThread()
{
	int ret;
	ttglobal tg = gglobal();
	/* Synchronize trace/error log... */
	fflush(stdout);
	fflush(stderr);
	sync();
	ASSERT(TEST_NULL_THREAD(gglobal()->threads.DispThrd));


	///* Initialize all mutex/condition variables ... */ //moved to resource and texture thread inits
	//pthread_mutex_init( &tg->threads.mutex_resource_tree, NULL );
	//pthread_mutex_init( &tg->threads.mutex_resource_list, NULL );
	//pthread_mutex_init( &tg->threads.mutex_texture_list, NULL );
	//pthread_cond_init( &tg->threads.resource_list_condition, NULL );
	//pthread_cond_init( &tg->threads.texture_list_condition, NULL );
	//pthread_mutex_init(&tg->threads.mutex_frontend_list,NULL);


	ret = pthread_create(&tg->threads.DispThrd, NULL, (void *) _displayThread, tg);
	switch (ret) {
	case 0: 
		break;
	case EAGAIN: 
		ERROR_MSG("initializeDisplayThread: not enough system resources to create a process for the new thread.");
		return;
	}


#if !defined(TARGET_AQUA) && !defined(_MSC_VER) 
	if (gglobal()->internalc.global_trace_threads) {
		TRACE_MSG("initializeDisplayThread: waiting for display to become initialized...\n");
		while (IS_DISPLAY_INITIALIZED == FALSE) {
			usleep(50);
		}
	}
#endif
}

//#endif /* FRONTEND_HANDLES_DISPLAY_THREAD */


//desktop plugin
void fwl_spawnRenderingThread(void *globalcontext){
	//if(!params->frontend_handles_display_thread){
		/* OK the display is now initialized,
		   create the display thread and wait for it
		   to complete initialization */
		fwl_initializeDisplayThread();

		//usleep(50);
		//set_thread2global(tg,tg->threads.DispThrd ,"display thread");
	//}

}

//desktop console
void fwl_startFreeWRL(const char *url)
{
	ttglobal tg = gglobal();
	//ConsoleMessage ("yes, really, FWL_STARTFREEWRL called is called\n");

	/* Give the main argument to the resource handler */
	if (url != NULL) {
		//char* suff = NULL;
		//char* local_name = NULL;
		//splitpath_local_suffix(url, &local_name, &suff);
		//if(url) tg->Mainloop.url = strdup(url);
		//tg->Mainloop.scene_name = local_name;
		//tg->Mainloop.scene_suff = suff;

		fwl_resource_push_single_request(url);
		DEBUG_MSG("request sent to parser thread, main thread joining display thread...\n");
	} else {
		DEBUG_MSG("no request for parser thread, main thread joining display thread...\n");
	}
	//this is for simulating frontend_gets_files for testing. Do not set FRONTEND_GETS_FILES. 
	//this tests an alternate method. 
	// you need to put an http:// file on the command line (this is hardwired for io_http gets only, not local
	//if(frontendGetsFiles()==1){
	//	for(;;){
	//		frontend_dequeue_get_enqueue(ttg); //this is non-blocking (returns immediately) if queue empty
	//		sleep(100);
	//		if(checkExitRequest()) break;
	//	}
	//	sleep(200); //wait for backend threads to wind down
	//}else{
		/* now wait around until something kills this thread. */
		//pthread_join(gglobal()->threads.DispThrd, NULL);
		_displayThread(tg);
	//}
}
