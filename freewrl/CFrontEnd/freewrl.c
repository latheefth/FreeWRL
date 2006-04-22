/*******************************************************************
 Copyright (C) 2003 John Stewart, CRC Canada.
 DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
 See the GNU Library General Public License (file COPYING in the distribution)
 for conditions of use and redistribution.
*********************************************************************/

/* beginnings of FreeWRL in C */
#include <headers.h>
#include <vrmlconf.h>
#include <Structs.h>
#include "OpenGL_Utils.h"
#include <pthread.h>
#include <PluginSocket.h>
#include <stdlib.h>
#include <unistd.h>
#include <dlfcn.h>

#include <GL/gl.h>
#include <GL/glx.h>
#include <GL/glu.h>

#ifdef LINUX
#include <GL/glext.h>
#endif

#include <signal.h>


#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Xaw/AsciiText.h>
#include <X11/Xaw/Box.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Dialog.h>
#include <X11/Xaw/MenuButton.h>
#include <X11/Xaw/Paned.h>
#include <X11/Xaw/SimpleMenu.h>
#include <X11/Xaw/SmeBSB.h>
#include <X11/Xaw/SmeLine.h>
#include <X11/Xaw/Sme.h>


extern XtAppContext freewrlXtAppContext;

extern pthread_t DispThrd;
int wantEAI;		/* enable EAI? */
extern int fullscreen;		/* fwopts.c - do fullscreen rendering? */
extern char *initialFilename;	/* file to start FreeWRL with */
extern void   XEventStereo(void);

extern void openMainWindow (void);
extern void glpOpenGLInitialize(void);
extern void EventLoop(void);
extern void resetGeometry(void);

/* function prototypes */
void catch_SIGQUIT();
void catch_SIGSEGV();
void catch_SIGALRM(int);
void initFreewrl(void);
extern int parseCommandLine(int, char **);

int main (int argc, char **argv) {
	int retval;
	int count;
	int digit_optind = 0;
	char *pwd;

	/* first, get the FreeWRL shared lib, and verify the version. */
	if(strcmp(FWVER,getLibVersion())){
  	  ConsoleMessage ("FreeWRL expected library version %s, got %s...\n",FWVER,getLibVersion());
	}

	/* set the screen width and height before getting into arguments */
	screenWidth = 600; screenHeight=400;
	fullscreen = 0;
	wantEAI = 0;

	/* install the signal handler for SIGQUIT */
	signal (SIGQUIT, (void(*)(int))catch_SIGQUIT);
	signal (SIGSEGV,(void(*)(int))catch_SIGSEGV);
	signal (SIGALRM,(void(*)(int))catch_SIGALRM);

	/* parse command line arguments */
	/* JAS - for last parameter of long_options entries, choose
	 * ANY character that is not 'h', and that is not used */
	if (parseCommandLine (argc, argv)) {
        	/* create the initial scene, from the file passed in
        	  and place it as a child of the rootNode. */

        	initialFilename = (char *)malloc(1000 * sizeof (char));
        	pwd = (char *)malloc(1000 * sizeof (char));
        	getcwd(pwd,1000);
        	/* if this is a network file, leave the name as is. If it is
        	   a local file, prepend the path to it */


	        if (checkNetworkFile(argv[optind])) {
			setFullPath(argv[optind]);
	                strcpy (initialFilename,argv[optind]); 
	                BrowserFullPath = (char *)malloc ((strlen(argv[optind])+1) * sizeof(char));
	                strcpy(BrowserFullPath,pwd); 
	
	        } else {
	                makeAbsoluteFileName(initialFilename, pwd, argv[optind]);
	                BrowserFullPath = (char *)malloc ((strlen(initialFilename)+1) * sizeof(char));
	                strcpy (BrowserFullPath,initialFilename);
	        }
	} else {
		BrowserFullPath = NULL;
		initialFilename = NULL;
	}


	/* start threads, parse initial scene, etc */
	initFreewrl();

	/* free(initialFilename); free(pwd); */

	/* do we require EAI? */
	if (wantEAI) create_EAI();

	/* now wait around until something kills this thread. */
	pthread_join(DispThrd, NULL);
	perl_destruct(my_perl);
	perl_free(my_perl);
}


static int CaughtSEGV = FALSE;
/* SIGQUIT handler - plugin code sends a SIGQUIT... */
void catch_SIGQUIT() {
	/*
		ConsoleMessage ("FreeWRL got a sigquit signal");
	 shut up any SIGSEGVs we might get now.
	*/
	CaughtSEGV = TRUE;
    	doQuit();
}

void catch_SIGSEGV() {
	if (!CaughtSEGV) {
		ConsoleMessage ("FreeWRL got a SIGSEGV - can you please mail the file(s) to\n freewrl-04@rogers.com with a valid subject line. Thanks.\n");
		CaughtSEGV = TRUE;
	}
    exit(1);
}

void catch_SIGALRM(int sig)
{
    signal(SIGALRM, SIG_IGN);

    /* stuffs to do on alarm */
    /* fprintf(stderr,"An alarm signal just arrived ...IT WAS IGNORED!\n"); */
    /* end of alarm actions */

    alarm(0);
    signal(SIGALRM, catch_SIGALRM);
}
