/* -*- Mode: C; tab-width: 4; -*- */
/*******************************************************************************
 * Simple LiveConnect Sample Plugin
 * Copyright (c) 1996 Netscape Communications. All rights reserved.
 *
 * Modified by John Stewart  - CRC Canada to provide for plugin capabilities
 * for FreeWRL - an open source VRML and X3D browser.
 *
 * Operation:
 *
 * In the NPP_Initialize routine, a pipe is created and sent as the window
 * title to FreeWRL. FreeWRL (OpenGL/OpenGL.xs) looks at this "wintitle",
 * and if it starts with "pipe:", then treats the number following as
 * a pipe id to send the window id back through. The pipe is then closed.
 *
 * The Plugin uses this window id to rehost the window.
 *
 * John Stewart, Alya Khan, Sarah Dumoulin - CRC Canada 2002 - 2006.
 *
 ******************************************************************************/
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "npapi.h"
#include "pluginUtils.h"
#include <X11/Xlib.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>


#define PLUGIN_NAME			"FreeWRL X3D/VRML"
#define PLUGIN_DESCRIPTION	"V4.3 VRML/X3D with FreeWRL. from http://www.crc.ca/FreeWRL"

#define RUNNINGONAMD64 (sizeof(void *) == 8)

#define ERRORMSG "FILE DOES NOT EXIST"

char *paramline[15]; /* parameter line */

static int PluginVerbose = 0;  /* CHECK LOG FILE PATH BEFORE SETTING THIS TO 1 */

/*******************************************************************************
 * Instance state information about the plugin.
 ******************************************************************************/

typedef struct _FW_PluginInstance
{
	int			interfaceFile[2];
	Display 		*display;
	uint32 			x, y;
	uint32 			width, height;
	Window 			mozwindow;
	Window 			fwwindow;
	pid_t 			childPID;
	char 			*fName;
	int			freewrl_running;
	int			interfacePipe[2];		/* pipe plugin FROM freewrl	*/
} FW_PluginInstance;



typedef void (* Sigfunc) (int);



static int np_fileDescriptor;

/* Socket file descriptors */
#define SOCKET_2 0
#define SOCKET_1   1

#define PIPE_PLUGINSIDE 0
#define PIPE_FREEWRLSIDE   1



static void signalHandler (int);
static int freewrlRecieve (int);
static int init_socket (int, Boolean);
Sigfunc signal (int, Sigfunc func);

/*******************************************************************************
 ******************************************************************************/

char debs[256];
static FILE * tty = NULL;

struct timeval mytime;
struct timezone tz; /* unused see man gettimeofday */
double TickTime;
NPStream *currentStream = NULL;


/* Debugging routine */
static void print_here (char * xx) {
	if (!PluginVerbose) return;

	if (tty == NULL) {
		tty = fopen("/home/luigi/log", "w");
		if (tty == NULL)
			PluginVerbose = FALSE;
		fprintf (tty, "\nplugin restarted\n");
	}

        /* Set the timestamp */
        gettimeofday (&mytime,&tz);
        TickTime = (double) mytime.tv_sec + (double)mytime.tv_usec/1000000.0;

	fprintf (tty,"%f: ",TickTime);
	fprintf(tty, "plug-in: %s\n", xx);
	fflush(tty);

	/* printf ("plugin: %s\n",xx); */
}


Sigfunc signal(int signo, Sigfunc func) {
	struct sigaction action, old_action;

	action.sa_handler = func;
	/*
 	 * Initialize action's signal set as empty set
	 * (see man page sigsetops(3)).
	*/
	sigemptyset(&action.sa_mask);

	action.sa_flags = 0; /* Is this a good idea??? */

	/* Add option flags for handling signal: */
	action.sa_flags |= SA_NOCLDSTOP;
	#ifdef SA_NOCLDWAIT
	action.sa_flags |= SA_NOCLDWAIT;
	#endif

	if (sigaction(signo, &action, &old_action) < 0) {
		print_here("Call to sigaction failed");
		return(SIG_ERR);
	}
	/* Return the old action for the signal or SIG_ERR. */
	return(old_action.sa_handler);
}

void signalHandler(int signo) {
	print_here("\n");
	sprintf(debs, "ACTION on our port - Signal %d caught from signalHandler.", signo);
	print_here(debs);

	if (signo == SIGIO) {
		freewrlReceive(np_fileDescriptor);

	} else {
		/* Should handle all except the uncatchable ones. */
		print_here("\nClosing plugin log.\n");
	}
}

int freewrlReceive(int fileDescriptor) {
	sigset_t newmask, oldmask;

	urlRequest request;
	size_t request_size = 0;
	int rv = 0;
	int retval;

	retval = NPERR_NO_ERROR;

	sprintf(debs, "Call to freewrlReceive fileDescriptor %d.", fileDescriptor);
	print_here (debs);

	bzero(request.url, FILENAME_MAX);
	request.instance = 0;
	request.notifyCode = 0; /* not currently used */

	request_size = sizeof(request);

	/*
	 * The signal handling code is based on the work of
	 * W. Richard Stevens from Unix Network Programming,
	 * Networking APIs: Sockets and XTI.
	*/

	/* Init. the signal sets as empty sets. */
	if (sigemptyset(&newmask) < 0) {
		print_here("Call to sigemptyset with arg newmask failed");
		return(NPERR_GENERIC_ERROR);
	}

	if (sigemptyset(&oldmask) < 0) {
		print_here("Call to sigemptyset with arg oldmask failed");
		return(NPERR_GENERIC_ERROR);
	}

	if (sigaddset(&newmask, SIGIO) < 0) {
		print_here("Call to sigaddset failed");
		return(NPERR_GENERIC_ERROR);
	}

	/* Code to block SIGIO while saving the old signal set. */
	if (sigprocmask(SIG_BLOCK, &newmask, &oldmask) < 0) {
		print_here("Call to sigprocmask failed");
		return(NPERR_GENERIC_ERROR);
	}

	/* If blocked or interrupted, be silent. */
	if (read(fileDescriptor, (urlRequest *) &request, request_size) < 0) {
		if (errno != EINTR && errno != EAGAIN) {
			print_here("Call to read failed");
		}
		/* FreeWRL has died, or THIS IS US WRITING CREATING THAT SIG. */
		print_here ("freewrlReceive, quick return; either this is us writing or freewrl croaked");
		return(NPERR_GENERIC_ERROR);
	} else {
		sprintf (debs, "notifyCode = %d url = %s", request.notifyCode, request.url);
		print_here(debs);

		/* is this a getUrl, or a "open new window for url" */
		if (request.notifyCode == 0) {
			/* get Url and return it to FreeWRL */
			if ((rv = NPN_GetURLNotify(request.instance, request.url, NULL,(void *)request.url))
				!= NPERR_NO_ERROR) {
				sprintf(debs, "Call to NPN_GetURLNotify failed with error %d.", rv);
				print_here(debs);
				retval = NPERR_GENERIC_ERROR;
			}
			sprintf (debs, "step 2a, request.url %s",request.url);
			print_here(debs);

		} else if (request.notifyCode == -99) {
			/* Firefox, etc took too long. we have timed out. */
			sprintf (debs,"notifyCode = -99, we have timed out for %s",request.url);
			print_here(debs);
			if (currentStream != NULL) {
				NPN_DestroyStream(request.instance, currentStream, NPRES_USER_BREAK);
				sprintf (debs, "FreeWRL can not find: %s",request.url);
				NPN_Status (request.instance, debs);
				currentStream = NULL;
			}

		} else {
			/* request.notifyCode must be 1 */
			sprintf (debs,"NPN_GetStream...");
			print_here(debs);

			NPStream* stream;
    			NPError err = NPERR_NO_ERROR;
			char* myData = "<HTML><B>This is a message from my plug-in!</b></html>";
			int32 myLength = strlen(myData) + 1;
			err = NPN_NewStream(request.instance,
					"text/html",
					"_AnchorFailsinFreeWRL",
					&stream);
			print_here ("NewStream made");

			err = NPN_Write(request.instance,
					stream,
					myLength,
					myData);
			print_here ("NPN_Write made");
		}

		/* now, put a status line on bottom of browser */
		sprintf (debs, "FreeWRL loading: %s\n",request.url);
		NPN_Status (request.instance, debs);
	}

	/* Restore old signal set, which unblocks SIGIO. */
	if (sigprocmask(SIG_SETMASK, &oldmask, NULL) < 0) {
		print_here("Call to sigprocmask failed");
		return(NPERR_GENERIC_ERROR);
	}

	return(retval);
	print_here("returning from freewrl_receive\n");
}

int init_socket(int fileDescriptor, Boolean nonblock) {
	int io_flags;

	if (fcntl(fileDescriptor, F_SETOWN, getpid()) < 0) {
		print_here("Call to fcntl with command F_SETOWN failed");
		return(NPERR_GENERIC_ERROR);
	}

	if ( (io_flags = fcntl(fileDescriptor, F_GETFL, 0)) < 0 ) {
		print_here("Call to fcntl with command F_GETFL failed");
		return(NPERR_GENERIC_ERROR);
	}

	/*
	 * O_ASYNC is specific to BSD and Linux.
	 * Use ioctl with FIOASYNC for others.
	*/
	#ifndef __sgi
	io_flags |= O_ASYNC;
	#endif

	if (nonblock) { io_flags |= O_NONBLOCK; }

	if ( (io_flags = fcntl(fileDescriptor, F_SETFL, io_flags)) < 0 ) {
		print_here("Call to fcntl with command F_SETFL failed");
		return(NPERR_GENERIC_ERROR);
	}
	return(NPERR_NO_ERROR);
}

/* actually run FreeWRL and swallow it, if enough information has been found */
void Run (NPP instance, char *origUrl) {

	FW_PluginInstance* FW_Plugin;

	char	pipetome[25];
	char	childFd[25];
	char	instanceStr[25];

	print_here ("start of Run");
	FW_Plugin = (FW_PluginInstance*) instance->pdata;

	/* Return if we do not have all of the required parameters. */
	if (FW_Plugin->mozwindow == 0) return;

	if (FW_Plugin->fName == NULL) return;

	if (FW_Plugin->display == 0) return;

	sprintf (debs,"Run, can run; disp win %x %x fname %s",
			FW_Plugin->mozwindow, FW_Plugin->display,
			FW_Plugin->fName);
	print_here (debs);


	/* start FreeWRL, if it is not running already. */
	if (!FW_Plugin->freewrl_running) {
		FW_Plugin->freewrl_running = 1;
		sprintf (debs,"STARTING testrun program, disp and win %x %x\n",FW_Plugin->display, FW_Plugin->mozwindow);
		print_here (debs);

		FW_Plugin->childPID = fork ();
		if (FW_Plugin->childPID == -1) {
			sprintf (debs, "\tFreeWRL: Fork for plugin failed: ");
			print_here (debs);
			FW_Plugin->childPID = 0;
		} else if (FW_Plugin->childPID == 0) {
			pid_t mine = getpid();
			if (setpgid(mine,mine) < 0) {
				sprintf (debs,"\tFreeWRL child group set failed");
				print_here (debs);
			} else {
				/* Nice FreeWRL to a lower priority */
				paramline[0] = "nice";
				paramline[1] = "freewrl";

				/* We have the file name, so include it */
				paramline[2] = FW_Plugin->fName;

				/* Pass in the pipe number so FreeWRL can return the
				window id */
				paramline[3] = "--plugin";
				paramline[4] = pipetome;

				/* EAI connection */
				paramline [5] = "--eai";

				/* File descriptor and instance  - allows FreeWRL to
				 request files from browser's cache */

				paramline[6] = "--fd";
				paramline[7] = childFd;
				paramline[8] = "--instance";
				paramline[9] = instanceStr;
				
				paramline[10] = "--originalFirefoxUrl";
				paramline[11] = origUrl;

				paramline[12] = NULL;


				/* create pipe string */
			    	sprintf(pipetome,"pipe:%d",FW_Plugin->interfacePipe[PIPE_FREEWRLSIDE]);

				/* child file descriptor - to send requests back here */
				sprintf (childFd, "%d", FW_Plugin->interfaceFile[SOCKET_2]);

				/* Instance, so that FreeWRL knows its us... */
				sprintf (instanceStr, "%u",(uintptr_t) instance);

				sprintf (debs,"exec param line is %s %s %s %s %s %s %s %s %s %s %s %s %s",
						paramline[0],paramline[1],paramline[2],paramline[3],
						paramline[4],paramline[5],paramline[6],paramline[7],
						paramline[8],paramline[9],paramline[10],paramline[11],
						paramline[12]);

				print_here (debs);
			    	execvp(paramline[0], (char* const *) paramline);
			}

			print_here("\tFreeWRL Plugin: Couldn\'t run FreeWRL.\n");

		} else {
		    	/* return error */
		}
	}

	print_here ("after FW_Plugin->freewrl_running call - waiting on pipe");

	read(FW_Plugin->interfacePipe[PIPE_PLUGINSIDE],&FW_Plugin->fwwindow,sizeof(Window));

	/*
	sprintf (debs,"After exec, and after read from pipe, FW window is %p\n",FW_Plugin->fwwindow);
	print_here(debs);

	sprintf (debs,"disp mozwindow height width %x %x %d %d\n",FW_Plugin->display,
			FW_Plugin->mozwindow, FW_Plugin->width,FW_Plugin->height);
	print_here (debs);
	*/


	/*reparent the window */
	if (!RUNNINGONAMD64) {
		/* print_here ("going to XFlush"); */

		XFlush(FW_Plugin->display);
		
		/* print_here ("going to XSync"); */

		XSync (FW_Plugin->display, FALSE);

		/* print_here ("going to reparent"); */
		XReparentWindow(FW_Plugin->display,
			FW_Plugin->fwwindow,
			FW_Plugin->mozwindow,
			0,0);

		XResizeWindow(FW_Plugin->display, FW_Plugin->fwwindow,
				FW_Plugin->width, FW_Plugin->height);


		XMapWindow(FW_Plugin->display,FW_Plugin->fwwindow);
		print_here ("after mapwindow");
	}

	print_here ("Run function finished\n");
}


/*******************************************************************************
 ******************************************************************************/
char*
NPP_GetMIMEDescription(void)
{
	print_here ("NPP_GetMIMEDescription");
        return("x-world/x-vrml:wrl:FreeWRL VRML Browser;model/vrml:wrl:FreeWRL VRML Browser;model/x3d+vrml:x3dv:FreeWRL VRML Browser;model/x3d+xml:x3d:FreeWRL X3D Browser;model/x3d+vrml:x3dv:FreeWRL X3D Browser;model/x3d+binary:x3db:FreeWRL X3D Browser");
}

NPError
NPP_GetValue(void *future, NPPVariable variable, void *value)
{
    NPError err = NPERR_NO_ERROR;
	print_here ("NPP_GetValue");
    if (variable == NPPVpluginNameString)
		*((char **)value) = PLUGIN_NAME;
    else if (variable == NPPVpluginDescriptionString)
		*((char **)value) = PLUGIN_DESCRIPTION;
    else
		err = NPERR_GENERIC_ERROR;

    return err;
}

/*******************************************************************************
 * General Plug-in Calls
 ******************************************************************************/

/*
** NPP_Initialize is called when your DLL is being loaded to do any
** DLL-specific initialization.
*/
NPError NPP_Initialize(void) {
	print_here ("NPP_Initialize");
    	return NPERR_NO_ERROR;
}


/*
** NPP_Shutdown is called when your DLL is being unloaded to do any
** DLL-specific shut-down. You should be a good citizen and declare that
** you're not using your java class any more. FW_Plugin allows java to unload
** it, freeing up memory.
*/
void NPP_Shutdown(void) { 
	print_here ("NPP_Shutdown");
}

/*
** NPP_New is called when your plugin is instantiated (i.e. when an EMBED
** tag appears on a page).
*/
NPError
NPP_New(NPMIMEType pluginType,
		NPP instance,
		uint16 mode,
		int16 argc,
		char* argn[],
		char* argv[],
		NPSavedData* saved) {

	NPError result = NPERR_NO_ERROR;
	FW_PluginInstance* FW_Plugin;
	char factString[60];
	unsigned int err;


	/* sprintf (debs,"NPP_New, argc %d argn %s  argv %s",argc,argn[0],argv[0]); */

	/* mode is NP_EMBED, NP_FULL, or NP_BACKGROUND (see npapi.h) */
	sprintf (debs,"NPP_New, argc %d" ,argc);
	if (mode == NP_EMBED) {
		strcat (debs, "NP_EMBED");
	} else if (mode == NP_FULL) {
		strcat (debs, "NP_FULL");
	} else strcat (debs, "UNKNOWN MODE");
	print_here (debs);

	if (instance == NULL)
		return NPERR_INVALID_INSTANCE_ERROR;

	instance->pdata = NPN_MemAlloc(sizeof(FW_PluginInstance));

	FW_Plugin = (FW_PluginInstance*) instance->pdata;

	print_here ("after memalloc");

	if (FW_Plugin == NULL)
	    return NPERR_OUT_OF_MEMORY_ERROR;

	FW_Plugin->display = NULL;
	FW_Plugin->x = 0;
	FW_Plugin->y=0;
	FW_Plugin->width = 0;
	FW_Plugin->height = 0;
	FW_Plugin->mozwindow = 0;
	FW_Plugin->fwwindow = 0;
	FW_Plugin->childPID=0;
	FW_Plugin->fName = NULL;
	FW_Plugin->freewrl_running = 0;
	pipe(FW_Plugin->interfacePipe);

	sprintf (debs, "Pipe created, PIPE_FREEWRLSIDE %d PIPE_PLUGINSIDE %d",
		FW_Plugin->interfacePipe[PIPE_FREEWRLSIDE], 
		FW_Plugin->interfacePipe[PIPE_PLUGINSIDE]);
	print_here (debs);


	/* Assume plugin and FreeWRL child process run on the same machine, 
	 then we can use UDP and have incredibly close to 100.00% reliability */
	
	if (socketpair(AF_LOCAL, SOCK_DGRAM, 0, FW_Plugin->interfaceFile) < 0) {
		print_here ("Call to socketpair failed");
		return (NPERR_GENERIC_ERROR);
	}
	sprintf (debs, "file pair created, SOCKET_1 %d SOCKET_2 %d",
		FW_Plugin->interfaceFile[SOCKET_1], 
		FW_Plugin->interfaceFile[SOCKET_2]);
	print_here (debs);


	np_fileDescriptor = FW_Plugin->interfaceFile[SOCKET_1];

	if (signal(SIGIO, signalHandler) == SIG_ERR) return (NPERR_GENERIC_ERROR);
	if (signal(SIGBUS, signalHandler) == SIG_ERR) return (NPERR_GENERIC_ERROR);

	/* prepare communication sockets */
	if ((err=init_socket(FW_Plugin->interfaceFile[SOCKET_2], FALSE))!=NPERR_NO_ERROR) return (err);
	if ((err=init_socket(FW_Plugin->interfaceFile[SOCKET_1], TRUE))!=NPERR_NO_ERROR) return (err);
	sprintf (debs, "NPP_New returning %d",err); 
	print_here (debs);

	return (err);
}


NPError
NPP_Destroy(NPP instance, NPSavedData** save)
{
	FW_PluginInstance* FW_Plugin;
	int status;

	print_here ("NPP_Destroy kill FreeWRL if it is running still");
	if (instance == NULL)
		return NPERR_INVALID_INSTANCE_ERROR;

	FW_Plugin = (FW_PluginInstance*) instance->pdata;

	if (FW_Plugin != NULL) {

		if (FW_Plugin->interfacePipe[PIPE_PLUGINSIDE] > 0) {
			close (FW_Plugin->interfacePipe[PIPE_FREEWRLSIDE]);
			close (FW_Plugin->interfacePipe[PIPE_PLUGINSIDE]);
		}


		if (FW_Plugin->fName != NULL) {
			NPN_MemFree(FW_Plugin->fName);
		}


		if (FW_Plugin->childPID >0) {

			sprintf (debs,"killing command kill %d",FW_Plugin->childPID);
			print_here(debs);
			/* kill(FW_Plugin->childPID, SIGQUIT);
			// really kill this - sometimes the SIGQUIT would just hang around */
			kill(FW_Plugin->childPID, SIGKILL);
			waitpid(FW_Plugin->childPID, &status, 0);
		}

		NPN_MemFree(instance->pdata);
		instance->pdata = NULL;
	}
	FW_Plugin->freewrl_running = 0;

	return NPERR_NO_ERROR;
}

void 
NPP_URLNotify (NPP instance, const char *url, NPReason reason, void* notifyData) {

	#define returnBadURL "this file is not to be found on the internet"
	FW_PluginInstance* FW_Plugin;

	FW_Plugin = (FW_PluginInstance*) instance->pdata;

	sprintf (debs,"NPP_URLNotify, url %s reason %d",url,reason);
	print_here (debs);

	if (reason == NPRES_DONE) {
		print_here ("NPP_UrlNotify - NPRES_DONE");
		return;
	} else if (reason == NPRES_USER_BREAK) {
		print_here ("NPP_UrlNotify - NPRES_USER_BREAK");
	} else if (reason == NPRES_NETWORK_ERR) {
		print_here ("NPP_UrlNotify - NPRES_NETWORK_ERR");
	} else {
		print_here ("NPP_UrlNotify - unknown");
	}


	sprintf (debs,"NPP_UrlNotify - writing %s (%u bytes) to socket %d",
		returnBadURL, strlen (returnBadURL) ,FW_Plugin->interfaceFile[SOCKET_1]);
	print_here(debs);

	if (write(FW_Plugin->interfaceFile[SOCKET_1], returnBadURL, strlen (returnBadURL)) < 0) {
		print_here ("Call to write failed");
	}
}


NPError
NPP_SetWindow(NPP instance, NPWindow *browser_window)
{
	NPError result = NPERR_NO_ERROR;
	FW_PluginInstance* FW_Plugin;
	int X_err;
	int count;

	print_here ("start of NPP_SetWindow");

	if (instance == NULL)
		return NPERR_INVALID_INSTANCE_ERROR;

	FW_Plugin = (FW_PluginInstance*) instance->pdata;


	/* set the display, if we know it yet */ 
	if (!FW_Plugin->display) { 
		if ((NPSetWindowCallbackStruct *)(browser_window->ws_info) != NULL) { 
			FW_Plugin->display = ((NPSetWindowCallbackStruct *) 
				browser_window->ws_info)->display; 
 
			sprintf (debs,"NPP_SetWindow, plugin display now is %p", FW_Plugin->display); 
			print_here (debs); 
		} 
	} 

	/* verify that the display has not changed */
	if ((NPSetWindowCallbackStruct *)(browser_window->ws_info) != NULL) { 
		if ((FW_Plugin->display) != ((NPSetWindowCallbackStruct *)
                                browser_window->ws_info)->display) {

					print_here ("HMMM - display has changed");
					FW_Plugin->display = ((NPSetWindowCallbackStruct *)
                        		        browser_window->ws_info)->display;
		}
	}

	sprintf (debs, "NPP_SetWindow, moz window is %x childPID is %d",browser_window->window,FW_Plugin->childPID);
	print_here (debs);


	FW_Plugin->width = browser_window->width;
	FW_Plugin->height = browser_window->height;


	if (FW_Plugin->mozwindow != (Window) browser_window->window) {
		FW_Plugin->mozwindow = (Window) browser_window->window;

		/* run FreeWRL, if it is not already running. It might not be... */
		if (!FW_Plugin->freewrl_running) {
			print_here ("NPP_SetWindow, running FreeWRL here!");
				Run(instance,FW_Plugin->fName);
		}
	}

	/* Handle the FreeWRL window */
	if (FW_Plugin->fwwindow) {
		sprintf (debs,"xresize x %d y %d  wid %d hei %d",
			FW_Plugin->x, FW_Plugin->y,
			FW_Plugin->width, FW_Plugin->height);
		print_here (debs);

		XResizeWindow(FW_Plugin->display, FW_Plugin->fwwindow,
			FW_Plugin->width, FW_Plugin->height);

		XSync (FW_Plugin->display,FALSE);
	}
	print_here ("exiting NPP_SetWindow");
	return result;
}


NPError
NPP_NewStream(NPP instance,
	      NPMIMEType type,
	      NPStream *stream,
	      NPBool seekable,
	      uint16 *stype)
{
	FW_PluginInstance* FW_Plugin;

	if (currentStream == NULL) {
		currentStream = stream;
	} else {
		print_here("NPP_NewStream, currentstream NOT NULL");
	}

	sprintf (debs,"NPP_NewStream, instance %d, type %d, stream %d, seekable %d stype %d",instance,
			stream, seekable,*stype);
	print_here(debs);
	if (instance == NULL)
		return NPERR_INVALID_INSTANCE_ERROR;

	FW_Plugin = (FW_PluginInstance*) instance->pdata;

	if (stream->url == NULL) {
		return(NPERR_NO_DATA);
	}

	/* Lets tell netscape to save this to a file. */
	*stype = NP_ASFILEONLY;
	seekable = FALSE;

	print_here ("NPP_NewStream returning noerror");
	return NPERR_NO_ERROR;
}


/* PLUGIN DEVELOPERS:
 *	These next 2 functions are directly relevant in a plug-in which
 *	handles the data in a streaming manner. If you want zero bytes
 *	because no buffer space is YET available, return 0. As long as
 *	the stream has not been written to the plugin, Navigator will
 *	continue trying to send bytes.  If the plugin doesn't want them,
 *	just return some large number from NPP_WriteReady(), and
 *	ignore them in NPP_Write().  For a NP_ASFILE stream, they are
 *	still called but can safely be ignored using this strategy.
 */

int32 STREAMBUFSIZE = 0X0FFFFFFF; /* If we are reading from a file in NPAsFile
								   * mode so we can take any size stream in our
								   * write call (since we ignore it) */

int32
NPP_WriteReady(NPP instance, NPStream *stream)
{
	print_here("NPP_WriteReady");

	/* Number of bytes ready to accept in NPP_Write() */
	return STREAMBUFSIZE;
}


int32 NPP_Write(NPP instance, NPStream *stream, int32 offset, int32 len, void *buffer) {
	print_here("NPP_Write");
	return 0;
	return len;		/* The number of bytes accepted */
}


NPError
NPP_DestroyStream(NPP instance, NPStream *stream, NPError reason)
{
	sprintf (debs,"NPP_DestroyStream, instance %d stream %d",instance,stream);
	print_here(debs);
	if (reason == NPRES_DONE) print_here("reason: NPRES_DONE");
	if (reason == NPRES_USER_BREAK) print_here("reason: NPRES_USER_BREAK");
	if (reason == NPRES_NETWORK_ERR) print_here("reason: NPRES_NETWORK_ERR");

	if (stream == currentStream) {
		currentStream = NULL;
	} else {
		print_here("NPP_DestroyStream, STREAMS DO NOT MATCH!\n");
	}

	if (instance == NULL)
		return NPERR_INVALID_INSTANCE_ERROR;
	return NPERR_NO_ERROR;
}


void
NPP_StreamAsFile(NPP instance, NPStream *stream, const char* fname)
{
	int bytes;

	FW_PluginInstance* FW_Plugin;
	if (instance != NULL) {
		FW_Plugin = (FW_PluginInstance*) instance->pdata;

		/* Get the base file name for FreeWRL to run */
		FW_Plugin->fName = (char *) NPN_MemAlloc((strlen(fname) +1) *sizeof(char *));
		strcpy(FW_Plugin->fName,fname);
		sprintf (debs,"NPP_StreamAsFile, name is %s",FW_Plugin->fName);
		print_here(debs);
		sprintf (debs,"NPP_StreamAsFile, stream url is %s",stream->url);
		print_here(debs);

		if (!FW_Plugin->freewrl_running) {
			/* if we are not running yet, see if we have enough to start. */
			Run (instance,stream->url);

		} else {
			if (fname == NULL) {
				print_here ("NPP_StreamAsFile has a NULL file");

				/* Try sending an empty string */
				if (write(FW_Plugin->interfaceFile[SOCKET_1], "", 1) < 0) {
					print_here ("Call to write failed");
				}
			} else {
				bytes = (strlen(fname)+1)*sizeof(const char *);
				sprintf (debs,"writing %s (%u bytes) to socket %d",
						fname, bytes,FW_Plugin->interfaceFile[SOCKET_1]);
				print_here(debs);

				if (write(FW_Plugin->interfaceFile[SOCKET_1], fname, bytes) < 0) {
					print_here ("Call to write failed");
				}

				/* send a "done" message to status bar */
				sprintf (debs, "FreeWRL: Done\n");
				NPN_Status(instance,debs);

			}
		}
	}
}


void
NPP_Print(NPP instance, NPPrint* printInfo)
{
	if(printInfo == NULL)
		return;

	if (instance != NULL) {
		FW_PluginInstance* FW_Plugin = (FW_PluginInstance*) instance->pdata;

		if (printInfo->mode == NP_FULL) {
		    /*
		     * PLUGIN DEVELOPERS:
		     *	If your plugin would like to take over
		     *	printing completely when it is in full-screen mode,
		     *	set printInfo->pluginPrinted to TRUE and print your
		     *	plugin as you see fit.  If your plugin wants Netscape
		     *	to handle printing in this case, set
		     *	printInfo->pluginPrinted to FALSE (the default) and
		     *	do nothing.  If you do want to handle printing
		     *	yourself, printOne is true if the print button
		     *	(as opposed to the print menu) was clicked.
		     *	On the Macintosh, platformPrint is a THPrint; on
		     *	Windows, platformPrint is a structure
		     *	(defined in npapi.h) containing the printer name, port,
		     *	etc.
		     */

			void* platformPrint =
				printInfo->print.fullPrint.platformPrint;
			NPBool printOne =
				printInfo->print.fullPrint.printOne;

			/* Do the default*/
			printInfo->print.fullPrint.pluginPrinted = FALSE;
		}
		else {	/* If not fullscreen, we must be embedded */
			NPWindow* printWindow =
				&(printInfo->print.embedPrint.window);
			void* platformPrint =
				printInfo->print.embedPrint.platformPrint;
		}
	}
}


