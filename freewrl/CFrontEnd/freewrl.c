/*******************************************************************
 Copyright (C) 2003 John Stewart, CRC Canada.
 DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
 See the GNU Library General Public License (file COPYING in the distribution)
 for conditions of use and redistribution.
*********************************************************************/

/* beginnings of FreeWRL in C */
#include "EXTERN.h"
#include "perl.h"
#include <Structs.h>
#include <pthread.h>
#include <headers.h>
#include <getopt.h>
#include <Snapshot.h>
#include <PluginSocket.h>

#ifndef AQUA

#include <GL/gl.h>
#include <GL/glx.h>
#include <GL/glu.h>

#endif

#ifdef LINUX
#include <GL/glext.h>
#endif

#include <signal.h>

// display and win are opened here, then pointers passed to
// freewrl. We just use these as unsigned, because we just
// pass the pointers along; we do not care what type they are.
#ifndef AQUA
Display *Disp;
Window Win;
GLXContext globalContext;
#endif

#ifndef AQUA
int wantEAI=FALSE;		/* enable EAI? */
#endif
extern int fullscreen;		/* fwopts.c - do fullscreen rendering? */

/* threading variables for event loop */
static pthread_t *loopthread;
char *threadmsg = "eventloop";
pthread_t thread1;

/* keypress sequence for startup - Robert Sim */
extern char *keypress_string;


/* for plugin running - these are read from the command line */
#ifndef AQUA
int _fw_pipe=0;
int _fw_FD=0;
unsigned  _fw_instance=0;
#endif

/* function prototypes */
void displayThread();
void catch_SIGQUIT();
void catch_SIGSEGV();
void catch_SIGALRM(int);

int main (int argc, char **argv) {
	int retval;
	int count;
	int c;
	int digit_optind = 0;
	int tmp;
	char *filename;
	char *pwd;

#ifndef IRIX
#ifndef AQUA
	/* first, get the FreeWRL shared lib, and verify the version. */
	if (strcmp(FWVER,getLibVersion())) {
		printf ("FreeWRL expected library version %s, got %s...\n",FWVER,getLibVersion());
	}
#endif
#endif

#ifndef AQUA
	/* set the screen width and height before getting into arguments */
	screenWidth = 450; screenHeight=300;
	fullscreen = 0;

#ifndef IRIX
	/* install the signal handler for SIGQUIT */
	signal (SIGQUIT, catch_SIGQUIT);
	signal (SIGSEGV, catch_SIGSEGV);
	signal (SIGALRM,catch_SIGALRM);

	/* parse command line arguments */
	/* JAS - for last parameter of long_options entries, choose
	 * ANY character that is not 'h', and that is not used */

	while (1) {
		int this_option_optind = optind ? optind : 1;
		int option_index = 0;
		static struct option long_options[] = {
			{"eai", 0, 0, 'e'},
			{"fast", 0, 0, 'f'},
			{"geometry", 1, 0, 'g'},
			{"help", 0, 0, 'h'},
			{"plugin", 1, 0, 'i'},
			{"fd", 1, 0, 'j'},
			{"instance", 1, 0, 'k'},
			{"version", 0, 0, 'v'},
			{"big",  0, 0, 'b'},		/* Alberto Dubuc */
			{"nostatus",0, 0, 's'},		/* Alberto Dubuc */
			{"keypress",1, 0, 'K'},		/* Robert Sim */

			{"seq", 0, 0, 'l'},
			{"seqb",1, 0, 'm'},
			{"snapb", 1, 0, 'n'},
			{"seqtmp", 1, 0, 'o'},
			{"gif", 0, 0, 'p'},
			{"maximg", 1, 0, 'q'},
			{"shutter", 0, 0, 'u'},
			{"eyedist", 1, 0, 'y'},
			{"fullscreen", 0, 0, 'c'},
			{"stereoparameter", 1, 0, 't'},
			{"screendist", 1, 0, 'r'},


			{"parent", 1, 0, 'x'},
			{"server", 1, 0, 'x'},
			{"sig", 1, 0, 'x'},
			{"ps", 1, 0, 'x'},
			{0, 0, 0, 0}
		};

#ifndef sparc
		c = getopt_long (argc, argv, "h", long_options, &option_index);
#else
		/*Sun version of getopt_long needs all the letters of parameters defined*/
		                c = getopt_long (argc, argv, "efghijkvlpqmnobsK", long_options, &option_index);
#endif

		if (c == -1)
			break;

		switch (c) {
			case 0:
				printf ("FreeWRL option --%s", long_options[option_index].name);
				if (optarg)
					printf (" with arg %s", optarg);
				printf ("\n");
				break;

			case 'x':
				printf ("option --%s not implemented yet, complain bitterly\n",
					long_options[option_index].name);
				break;

			case 'e':
#ifndef AQUA
				wantEAI=TRUE;
#endif
				break;

			case 'f':
				global_texSize = 256;
				break;

			case 'g':
				setGeometry(optarg);
				break;
			
			case 'c':
				fullscreen = 1;
#ifndef XF86V4
				printf("\nFullscreen mode is only available for XFree86 version 4.\n");
				printf("If you are running version 4, please add -DXF86V4 to your vrml.conf file\n");
				printf("in the FREEWRL_DEFINES section, and add -lXxf86vm to the FREEWRL_LIBS section.\n");
				fullscreen = 0;
#endif
				break;

			case 'h':
				printf ("\nFreeWRL VRML/X3D browser from CRC Canada (http://www.crc.ca)\n");
				printf ("   type \"man freewrl\" to view man pages\n\n");
				break;

			case 'i': sscanf (optarg,"pipe:%d",&_fw_pipe); break; 
			case 'j': sscanf (optarg,"%d",&_fw_FD); break;
			case 'k': sscanf (optarg,"%u",&_fw_instance); break;
			case 'v': printf ("FreeWRL version: %s\n",FWVER); exit(0);break;


			/* Snapshot stuff */
			case 'l': snapsequence = TRUE; break;
			case 'p': snapGif = TRUE; break;
			case 'q': sscanf (optarg,"%d",&maxSnapImages); 
				  if (maxSnapImages <=0) {
					printf ("FreeWRL: Commandline -maximg %s invalid\n",optarg);
					maxSnapImages = 100;
				  }
				  break;

			case 'm': 
				  count = strlen(argv[optind]);
				  if (count > 500) count = 500;
				  snapseqB = malloc (count+1);
                		  strcpy (snapseqB,argv[optind]);
				  break;
			case 'n':
				  count = strlen(argv[optind]);
				  if (count > 500) count = 500;
				  snapsnapB = malloc (count+1);
                		  strcpy (snapsnapB,argv[optind]);
				  break;
			case 'o':
				  count = strlen(argv[optind]);
				  if (count > 500) count = 500;
				  seqtmp = malloc (count+1);
                		  strcpy (seqtmp,argv[optind]);
				  break;

			case 'b': /* Alberto Dubuc - bigger window */
				setGeometry ("800x600");
				break;
			case 's': /* Alberto Dubuc - no status bar */
				display_status = 0;
				break;

				/* Shutter patches from Mufti @rus */
			case 'r':
				setScreenDist(optarg);
				break;
			case 't':
				setStereoParameter(optarg);
				break;
			case 'u':
				setShutter();
				XEventStereo();
				break;
			case 'y':
				setEyeDist(optarg);
				break;

				/* initial string of keypresses once main url is loaded */
			case 'K':	
				keypress_string=optarg;
				break;
			default:
				/* printf ("?? getopt returned character code 0%o ??\n", c); */
				break;
		}
	}

#endif
#endif
#ifndef AQUA
	if (optind < argc) {
		if (optind != (argc-1)) {
			printf ("freewrl:warning, expect only 1 file on command line; running file: %s\n", 
				argv[optind]);
		}

		/* save the url for later use, if required */
		count = strlen(argv[optind]);
		if (BrowserURL != NULL) free (BrowserURL);
		BrowserURL = malloc (count+1);
		strcpy (BrowserURL,argv[optind]);
	} else {
		printf ("freewrl:missing VRML/X3D file name\n");
		exit(1);
	}
#endif
	/* create the display thread. */
	pthread_create (&thread1, NULL, (void *)&displayThread, (void *)threadmsg);

#ifndef AQUA
	/* create the Perl parser thread */
	initializePerlThread(PERLPATH);
#endif
	while (!isPerlinitialized()) {usleep(50);}

	/* create the Texture parser thread */
	initializeTextureThread();
	while (!isTextureinitialized()) {usleep(50);}

	/* get the Netscape Browser name, if we are pluggind */
	NetscapeName[0] = NULL;
 	if (RUNNINGASPLUGIN) {
		if (read(_fw_FD, NetscapeName,MAXNETSCAPENAMELEN) < 0) {
		}
	}


	/* create the initial scene, from the file passed in
	and place it as a child of the rootNode. */

	filename = malloc(1000 * sizeof (char));
	pwd = malloc(1000 * sizeof (char));
	getcwd(pwd,1000);
	

#ifndef AQUA
	/* if this is a network file, leave the name as is. If it is
	 * a local file, prepend the path to it */
	if (checkNetworkFile(argv[optind])) {
		strcpy (filename,argv[optind]);
		BrowserFullPath = malloc ((strlen(argv[optind])+1) * sizeof(char));
		strcpy(BrowserFullPath,pwd);
				
	} else {

		makeAbsoluteFileName(filename, pwd, argv[optind]);
		BrowserFullPath = malloc ((strlen(filename)+1) * sizeof(char));
		strcpy (BrowserFullPath,filename);
	}
#endif
	// printf ("FrontEnd, filename %s\n",filename);
	perlParse(FROMURL, filename,TRUE,FALSE,
		rootNode, offsetof (struct VRML_Group, children),&tmp);

	free(filename); free(pwd);

	/* do we require EAI? */
#ifndef AQUA
	if (wantEAI) create_EAI();
#endif

	/* now wait around until something kills this thread. */
	pthread_join(thread1, NULL);

	perl_destruct(my_perl);
	perl_free(my_perl);
}


/* handle all the displaying and event loop stuff. */
void displayThread() {
	int count;
#ifndef AQUA
	openMainWindow(Disp,&Win,&globalContext);
#endif

	glpOpenGLInitialize();
	new_tessellation();

	while (1==1) {
		EventLoop();
	}
	if (fullscreen)
		resetGeometry();
}

/* SIGQUIT handler - plugin code sends a SIGQUIT... */
void catch_SIGQUIT() {
    doQuit();
}

void catch_SIGSEGV() {
	fprintf (stderr,"FreeWRL got a SIGSEGV - can you please mail the file(s) to\n");
	fprintf (stderr,"freewrl-04@rogers.com with a valid subject line. Thanks.\n");
    fflush(NULL);
}

void catch_SIGALRM(int sig)
{
    signal(SIGALRM, SIG_IGN);
    
    /* stuffs to do on alarm */
    //fprintf(stderr,"An alarm signal just arrived ...IT WAS IGNORED!\n");
    /* end of alarm actions */
    
    alarm(0);
    signal(SIGALRM, catch_SIGALRM);
}

/* funnel all print statements through here - this allows us to
 * eventually put errors/messages on a window
 * */

#ifndef AQUA
/* stop all of FreeWRL, terrible error! */
void freewrlDie (const char *format) {
	printf ("\nFreeWRL: Catastrophic error:\n");
	printf (format);
	printf ("\n");
	doQuit();
}
#endif
