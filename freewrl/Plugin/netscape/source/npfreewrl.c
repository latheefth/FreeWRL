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
 * John Stewart, Alya Khan, Sarah Dumoulin - CRC Canada 2002.
 *
 ******************************************************************************/

#include <sys/socket.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ctype.h>
#include <unistd.h>

#include "npapi.h"
#include "pluginUtils.h"

#include <stdio.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>

//JAS #include "Simple.h"

#include <X11/Xlib.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>


#define PLUGIN_NAME			"FreeWRL VRML Netscape - Mozilla Plugin 2.0"
#define PLUGIN_DESCRIPTION	"Implements VRML plugin functionality using FreeWRL. \nPlease see http://www.crc.ca/FreeWRL for more information"

char *paramline[20]; /* parameter line */

/*******************************************************************************
 * Instance state information about the plugin.
 ******************************************************************************/

typedef struct _FW_PluginInstance
{
	uint16			fMode;

	int			fd[2];
	Display 		*display;
	uint32 			x, y;
	uint32 			width, height;
	Window 			mozwindow;
	Window 			fwwindow;
	Widget			mozillaWidget;
	int			embedded;
	pid_t 			childPID;
	char 			*fName;

	int			precv[2];		/* pipe plugin FROM freewrl	*/
} FW_PluginInstance;



typedef void (* Sigfunc) (int);



static FILE * tty = NULL;
static int np_fd;

// Socket file descriptors
#define FWRL 0
#define NP   1



//JAS void DisplayJavaMessage(NPP instance, char* msg, int len);
static void signalHandler (int);
static int freewrlRecieve (int);
static int init_socket (int, Boolean);
Sigfunc signal (int, Sigfunc func);

/*******************************************************************************
 ******************************************************************************/

char debs[256];
static int freewrl_running = 0;

// Debugging routine
static void print_here (char * xx)
{
// uncomment this stuff to enable log printing.
//    if (tty == NULL) {
//	tty = fopen("/tmp/log", "w");
//
//	if (tty == NULL)
//	    abort();
//	fprintf (tty, "\nplugin restarted\n");
//	}
//
//    fprintf(tty, "plug-in: %s\n", xx);
//    fflush(tty);
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
	sprintf(debs, "Signal %d caught from signalHandler!\n", signo);
	print_here(debs);

	if (signo == SIGIO) {
		freewrlReceive(np_fd);

	} else {
		/* Should handle all except the uncatchable ones. */
		print_here("\nClosing plugin log.\n");
		fclose(tty);
		//JASexit(1);
	}
}

int freewrlReceive(int fd) {
	sigset_t newmask, oldmask;

	urlRequest request;
	size_t request_size = 0;
	int rv = 0;

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
	if (read(fd, (urlRequest *) &request, request_size) < 0) {
		if (errno != EINTR && errno != EAGAIN) {
			print_here("Call to read failed");
		}
		return(NPERR_GENERIC_ERROR);
	} else {
		if ((rv = NPN_GetURL(request.instance, request.url, NULL)) 
			!= NPERR_NO_ERROR) {
			sprintf(debs, "Call to NPN_GetURL failed with error %d.\n", rv);
			print_here(debs);
		}
	}

	/* Restore old signal set, which unblocks SIGIO. */
	if (sigprocmask(SIG_SETMASK, &oldmask, NULL) < 0) {
		print_here("Call to sigprocmask failed");
		return(NPERR_GENERIC_ERROR);
	}

	return(NPERR_NO_ERROR);
}

int init_socket(int fd, Boolean nonblock) {
	int io_flags;

	if (fcntl(fd, F_SETOWN, getpid()) < 0) {
		print_here("Call to fcntl with command F_SETOWN failed");
		return(NPERR_GENERIC_ERROR);
	}

	if ( (io_flags = fcntl(fd, F_GETFL, 0)) < 0 ) {
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

	if ( (io_flags = fcntl(fd, F_SETFL, io_flags)) < 0 ) {
		print_here("Call to fcntl with command F_SETFL failed");
		return(NPERR_GENERIC_ERROR);
	}
	return(NPERR_NO_ERROR);
}

// actually run FreeWRL and swallow it, if enough information has been found
void Run (NPP instance) {

	FW_PluginInstance* FW_Plugin;

	char	pipetome[25];
	char	childFd[25];
	char	instanceStr[25];

	print_here ("start of Run");
	FW_Plugin = (FW_PluginInstance*) instance->pdata;

	// Return if we do not have all of the required parameters.
	if (FW_Plugin->mozwindow == 0) return;

	if (FW_Plugin->fName == NULL) return;

	if (FW_Plugin->display == 0) return;

	sprintf (debs,"Run, can run; disp win %x %x fname %s",
			FW_Plugin->mozwindow, FW_Plugin->display,
			FW_Plugin->fName);
	print_here (debs);


	// start FreeWRL, if it is not running already.
	if (!freewrl_running) {
		freewrl_running = 1;
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
				// Nice FreeWRL to a lower priority 
				paramline[0] = "nice";
				paramline[1] = "freewrl";

				// We have the file name, so include it
				paramline[2] = FW_Plugin->fName;

				// Pass in the pipe number so FreeWRL can return the 
				// window id
				paramline[3] = "-plugin";
				paramline[4] = pipetome;

				// EAI connection
				paramline [5] = "-eai";
				paramline [6] = "localhost:2000";

				// File descriptor and instance  - allows FreeWRL to
				// request files from browser's cache

				paramline[7] = "-fd";
				paramline[8] = childFd;
				paramline[9] = "-instance";
				paramline[10] = instanceStr;

				paramline[11] = NULL;


				// create pipe string
			    	sprintf(pipetome,"pipe:%d",FW_Plugin->precv[NP]);

				// child file descriptor - to send requests back here
				sprintf (childFd, "%d",FW_Plugin->fd[FWRL]);

				// Instance, so that FreeWRL knows its us...
				sprintf (instanceStr, "%u",(uint) instance);

				sprintf (debs,"exec param line is %s %s %s %s %s %s %s %s %s %s %s",
						paramline[0],paramline[1],paramline[2],paramline[3],
						paramline[4],paramline[5],paramline[6],paramline[7],
						paramline[8],paramline[9],paramline[10]);

				print_here (debs);	
			    	execvp(paramline[0], (char* const *) paramline);
			}

			print_here("\tFreeWRL Plugin: Couldn\'t run FreeWRL.\n");

		} else {
		    	// return error
		}
	}		

	print_here ("after freewrl_running call - waiting on pipe");

	read(FW_Plugin->precv[FWRL],&FW_Plugin->fwwindow,4);

	sprintf (debs,"After exec, and after read from pipe, FW window is %x\n",FW_Plugin->fwwindow);
	print_here(debs);

	sprintf (debs,"disp mozwindow height width %x %x %d %d\n",FW_Plugin->display,
			FW_Plugin->mozwindow, FW_Plugin->width,FW_Plugin->height);
	print_here (debs);


	//reparent the window
	XFlush(FW_Plugin->display);
	XSync (FW_Plugin->display, FALSE);
	XReparentWindow(FW_Plugin->display, 
			FW_Plugin->fwwindow,
			FW_Plugin->mozwindow,
			0,0);
	print_here ("after reparent/n");
		
	XResizeWindow(FW_Plugin->display, FW_Plugin->fwwindow,
			FW_Plugin->width, FW_Plugin->height);
		
	print_here ("after resize/n");
		
	XMapWindow(FW_Plugin->display,FW_Plugin->fwwindow);
	print_here ("after mapwindow/n");
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
NPError
NPP_Initialize(void)
{
	print_here ("NPP_Initialize");


    return NPERR_NO_ERROR;
}





/*
** We'll keep a global execution environment around to make our life
** simpler.
*/
//JAS JRIEnv* env;

/*
** NPP_GetJavaClass is called during initialization to ask your plugin
** what its associated Java class is. If you don't have one, just return
** NULL. Otherwise, use the javah-generated "use_" function to both
** initialize your class and return it. If you can't find your class, an
** error will be signalled by "use_" and will cause the Navigator to
** complain to the user.
*/
//JAS jref
//JAS NPP_GetJavaClass(void)
//JAS {
//JAS 	struct java_lang_Class* myClass;
//JAS 	env = NPN_GetJavaEnv();
//JAS 	if (env == NULL)
//JAS 		return NULL;		/* Java disabled */

//JAS     myClass = use_Simple(env);

//JAS 	if (myClass == NULL) {
//JAS 		/*
//JAS 		** If our class doesn't exist (the user hasn't installed it) then
//JAS 		** don't allow any of the Java stuff to happen.
//JAS 		*/
//JAS 		env = NULL;
//JAS 	}
//JAS 	return myClass;
//JAS }

/*
** NPP_Shutdown is called when your DLL is being unloaded to do any
** DLL-specific shut-down. You should be a good citizen and declare that
** you're not using your java class any more. FW_Plugin allows java to unload
** it, freeing up memory.
*/
void
NPP_Shutdown(void)
{
	print_here ("NPP_Shutdown");
//JAS	if (env)
//JAS		unuse_Simple(env);
}

//JAS /*
//JAS ** FW_Plugin function is a utility routine that calls back into Java to print
//JAS ** messages to the Java Console and to stdout (via the native method,
//JAS ** native_Simple_printToStdout, defined below).  Sure, it's not a very
//JAS ** interesting use of Java, but it gets the point across.
//JAS */
//JAS void
//JAS DisplayJavaMessage(NPP instance, char* msg, int len)
//JAS {
//JAS 	jref str, javaPeer;
//JAS 
//JAS 	if (!env) {
//JAS 		/* Java failed to initialize, so do nothing. */
//JAS 		return;
//JAS 	}
//JAS 
//JAS 	if (len == -1) 
//JAS 		len = strlen(msg);
//JAS 
//JAS 	/*
//JAS     ** Use the JRI (see jri.h) to create a Java string from the input
//JAS     ** message:
//JAS     */
//JAS 	str = JRI_NewStringUTF(env, msg, len);
//JAS 
//JAS 	/*
//JAS     ** Use the NPN_GetJavaPeer operation to get the Java instance that
//JAS     ** corresponds to our plug-in (an instance of the Simple class):
//JAS     */
//JAS 	javaPeer = NPN_GetJavaPeer(instance);
//JAS 	
//JAS 	/*
//JAS     ** Finally, call our plug-in's big "feature" -- the 'doit' method,
//JAS     ** passing the execution environment, the object, and the java
//JAS     ** string:
//JAS     */
//JAS 	Simple_doit(env, javaPeer, str);
//JAS }

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


	//sprintf (debs,"NPP_New, argc %d argn %s  argv %s",argc,argn[0],argv[0]);
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

	/* mode is NP_EMBED, NP_FULL, or NP_BACKGROUND (see npapi.h) */
	FW_Plugin->fMode = mode;
	FW_Plugin->display = NULL;
	FW_Plugin->x = 0;
	FW_Plugin->y=0;
	FW_Plugin->width = 0;
	FW_Plugin->height = 0;
	FW_Plugin->mozwindow = 0;
	FW_Plugin->fwwindow = 0;
	FW_Plugin->embedded = 0;
	FW_Plugin->childPID=0;
	FW_Plugin->mozillaWidget = 0;
	FW_Plugin->fName = NULL;
	pipe(FW_Plugin->precv);


	// Assume plugin and FreeWRL child process run on the same machine,
	// then we can use UDP and have incredibly close to 100.00% reliability
	if (socketpair(AF_LOCAL, SOCK_DGRAM, 0, FW_Plugin->fd) < 0) {
		print_here ("Call to socketpair failed");
		return (NPERR_GENERIC_ERROR);
	}

	np_fd = FW_Plugin->fd[NP];

	if (signal(SIGIO, signalHandler) == SIG_ERR) return (NPERR_GENERIC_ERROR);
	if (signal(SIGBUS, signalHandler) == SIG_ERR) return (NPERR_GENERIC_ERROR);

	// prepare communication sockets
	if ((err=init_socket(FW_Plugin->fd[FWRL], FALSE))!=NPERR_NO_ERROR) return (err);	
	if ((err=init_socket(FW_Plugin->fd[NP], TRUE))!=NPERR_NO_ERROR) return (err);	

	return (err);
}


NPError 
NPP_Destroy(NPP instance, NPSavedData** save)
{
	FW_PluginInstance* FW_Plugin;

	print_here ("NPP_Destroy kill FreeWRL if it is running still");
	if (instance == NULL)
		return NPERR_INVALID_INSTANCE_ERROR;

	FW_Plugin = (FW_PluginInstance*) instance->pdata;

	if (FW_Plugin != NULL) {

		if (FW_Plugin->precv[FWRL] > 0) {
			close (FW_Plugin->precv[NP]);
			close (FW_Plugin->precv[FWRL]);
		}


		if (FW_Plugin->fName != NULL) {
			NPN_MemFree(FW_Plugin->fName);
		}


		if (FW_Plugin->childPID >0) {
			kill(FW_Plugin->childPID*-1, SIGQUIT);
		}
	
		// Close file descriptors
		//if (FW_Plugin->fd[NP]) {
		//	close (FW_Plugin->fd[NP]);
		//}
	
		// debugging code
		//if (tty) fclose(tty);


		NPN_MemFree(instance->pdata);
		instance->pdata = NULL;
	}
	freewrl_running = 0;

//JAS	DisplayJavaMessage(instance, "Calling NPP_Destroy.", -1);

	return NPERR_NO_ERROR;
}

NPError 
NPP_SetWindow(NPP instance, NPWindow *browser_window)
{
	NPError result = NPERR_NO_ERROR;
	FW_PluginInstance* FW_Plugin;
	int X_err;
	int count;

//JAS	DisplayJavaMessage(instance, "Calling NPP_SetWindow.", -1); 

	if (instance == NULL)
		return NPERR_INVALID_INSTANCE_ERROR;

	FW_Plugin = (FW_PluginInstance*) instance->pdata;


	// Now, see if this is the first time
	if (!FW_Plugin->display) {
			FW_Plugin->display = ((NPSetWindowCallbackStruct *)
					browser_window->ws_info)->display;
			sprintf (debs,"NPP_SetWindow, plugin display now is %x", FW_Plugin->display);
			print_here(debs);
	}




sprintf (debs, "NPP_SetWindow, moz window is %x childPID is %d",browser_window->window,FW_Plugin->childPID);
print_here (debs);


	FW_Plugin->width = browser_window->width;
	FW_Plugin->height = browser_window->height;


	if (FW_Plugin->mozwindow != (Window) browser_window->window) {
		FW_Plugin->mozwindow = (Window) browser_window->window;

		// run FreeWRL, if it is not already running. It might not be...
		if (!freewrl_running) {
				Run(instance);
		}
	}

	// Handle the FreeWRL window
	if (FW_Plugin->fwwindow) {
		sprintf (debs,"xresize x %d y %d  wid %d hei %d",
			FW_Plugin->x, FW_Plugin->y,
			FW_Plugin->width, FW_Plugin->height);
		print_here (debs);
		// lets unmap before resizing
		
		XResizeWindow(FW_Plugin->display, FW_Plugin->fwwindow,
			FW_Plugin->width, FW_Plugin->height);

		XSync (FW_Plugin->display,FALSE);
	}

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

	
	sprintf (debs,"NPP_NewStream, instance %d",instance);
	print_here(debs);
	if (instance == NULL)
		return NPERR_INVALID_INSTANCE_ERROR;

	FW_Plugin = (FW_PluginInstance*) instance->pdata;

//JAS	DisplayJavaMessage(instance, "Calling NPP_NewStream.", -1); 
	
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
	FW_PluginInstance* FW_Plugin;
	if (instance != NULL)
		FW_Plugin = (FW_PluginInstance*) instance->pdata;
print_here("NPP_WriteReady");

	//JAS DisplayJavaMessage(instance, "Calling NPP_WriteReady.", -1); 

	/* Number of bytes ready to accept in NPP_Write() */
	return STREAMBUFSIZE;
}


int32 
NPP_Write(NPP instance, NPStream *stream, int32 offset, int32 len, void *buffer)
{
	if (instance != NULL)
	{
		FW_PluginInstance* FW_Plugin = (FW_PluginInstance*) instance->pdata;
	}
print_here("NPP_Write");

	//JAS DisplayJavaMessage(instance, (char*)buffer, len); 

	return len;		/* The number of bytes accepted */
}


NPError 
NPP_DestroyStream(NPP instance, NPStream *stream, NPError reason)
{
	FW_PluginInstance* FW_Plugin;

	if (instance == NULL)
		return NPERR_INVALID_INSTANCE_ERROR;
	FW_Plugin = (FW_PluginInstance*) instance->pdata;

	//JAS DisplayJavaMessage(instance, "Calling NPP_DestroyStream.", -1); 

	return NPERR_NO_ERROR;
}


void 
NPP_StreamAsFile(NPP instance, NPStream *stream, const char* fname)
{
	int bytes;
	
	FW_PluginInstance* FW_Plugin;
	if (instance != NULL) {
		FW_Plugin = (FW_PluginInstance*) instance->pdata;

		// Get the base file name for FreeWRL to run
		FW_Plugin->fName = (char *) NPN_MemAlloc((strlen(fname) +1) *sizeof(char *));
		strcpy(FW_Plugin->fName,fname);
		sprintf (debs,"NPP_StreamAsFile, name is %s",FW_Plugin->fName);
		print_here(debs);

		if (!freewrl_running) {

			// if we are not running yet, see if we have enough to start.
			Run (instance);

		} else {
			if (fname == NULL) {
				print_here ("NPP_StreamAsFile has a NULL file");

				// Try sending an empty string
				if (write(FW_Plugin->fd[NP], "", 1) < 0) {
					print_here ("Call to write failed");
				}
			} else {
				bytes = (strlen(fname)+1)*sizeof(const char *);
				sprintf (debs,"writing %s (%u bytes) to socket %d",
						fname, bytes,FW_Plugin->fd[NP]);

				if (write(FW_Plugin->fd[NP], fname, bytes) < 0) {
					print_here ("Call to write failed");
				}

			}
		}
	}

	//JAS DisplayJavaMessage(instance, "Calling NPP_StreamAsFile.", -1); 
}


void 
NPP_Print(NPP instance, NPPrint* printInfo)
{
	//JAS DisplayJavaMessage(instance, "Calling NPP_Print.", -1); 

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
		    /*
		     * PLUGIN DEVELOPERS:
		     *	If your plugin is embedded, or is full-screen
		     *	but you returned false in pluginPrinted above, NPP_Print
		     *	will be called with mode == NP_EMBED.  The NPWindow
		     *	in the printInfo gives the location and dimensions of
		     *	the embedded plugin on the printed page.  On the
		     *	Macintosh, platformPrint is the printer port; on
		     *	Windows, platformPrint is the handle to the printing
		     *	device context.
		     */

			NPWindow* printWindow =
				&(printInfo->print.embedPrint.window);
			void* platformPrint =
				printInfo->print.embedPrint.platformPrint;
		}
	}
}

/*******************************************************************************
 * Define the Java native methods
 ******************************************************************************/

//JAS /* public native printToStdout(Ljava/lang/String;)V */
//JAS JRI_PUBLIC_API(void)
//JAS native_Simple_printToStdout(JRIEnv* env, struct Simple* self,
//JAS 							struct java_lang_String * s)
//JAS {
//JAS     const char* chars = JRI_GetStringUTFChars(env, s);
//JAS     printf(chars);		/* cross-platform UI! */
//JAS }

/*******************************************************************************
// NPP_URLNotify:
// Notifies the instance of the completion of a URL request. 
// 
// NPP_URLNotify is called when Netscape completes a NPN_GetURLNotify or
// NPN_PostURLNotify request, to inform the plug-in that the request,
// identified by url, has completed for the reason specified by reason. The most
// common reason code is NPRES_DONE, indicating simply that the request
// completed normally. Other possible reason codes are NPRES_USER_BREAK,
// indicating that the request was halted due to a user action (for example,
// clicking the "Stop" button), and NPRES_NETWORK_ERR, indicating that the
// request could not be completed (for example, because the URL could not be
// found). The complete list of reason codes is found in npapi.h. 
// 
// The parameter notifyData is the same plug-in-private value passed as an
// argument to the corresponding NPN_GetURLNotify or NPN_PostURLNotify
// call, and can be used by your plug-in to uniquely identify the request. 
 ******************************************************************************/

void
NPP_URLNotify(NPP instance, const char* url, NPReason reason, void* notifyData)
{
}

