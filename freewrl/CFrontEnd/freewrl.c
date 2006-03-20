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
#include <pthread.h>
#include <getopt.h>
#include <Snapshot.h>
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

extern pthread_t DispThrd;
static int wantEAI;		/* enable EAI? */
extern int fullscreen;		/* fwopts.c - do fullscreen rendering? */
extern char *initialFilename;	/* file to start FreeWRL with */
extern void   XEventStereo(void);

extern void openMainWindow (void);
extern void glpOpenGLInitialize(void);
extern void EventLoop(void);
extern void resetGeometry(void);

/* keypress sequence for startup - Robert Sim */
extern char *keypress_string;


/* for plugin running - these are read from the command line */
extern int _fw_pipe;
extern int _fw_browser_plugin;
uintptr_t _fw_instance;

/* function prototypes */
void catch_SIGQUIT();
void catch_SIGSEGV();
void catch_SIGALRM(int);
void initFreewrl(void);

int main (int argc, char **argv) {
	int retval;
	int count;
	int c;
	int digit_optind = 0;
	int tmp;
	char *pwd;

#ifndef IRIX
	/* first, get the FreeWRL shared lib, and verify the version. */
	if(strcmp(FWVER,getLibVersion())){
  	  ConsoleMessage ("FreeWRL expected library version %s, got %s...\n",FWVER,getLibVersion());
	}
#endif

	/* set the screen width and height before getting into arguments */
	screenWidth = 600; screenHeight=400;
	fullscreen = 0;
	wantEAI = 0;

#ifndef IRIX
	/* install the signal handler for SIGQUIT */
	signal (SIGQUIT, (void(*)(int))catch_SIGQUIT);
	signal (SIGSEGV,(void(*)(int))catch_SIGSEGV);
	signal (SIGALRM,(void(*)(int))catch_SIGALRM);

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
			{"nocollision",0, 0, 'Q'},		/* Alberto Dubuc */
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
			{"linewidth", 1, 0, 'W'},  /* Petr Mikulik */

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
		                c = getopt_long (argc, argv, "efghijkvlpqmnobsQWKX", long_options, &option_index);
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
				wantEAI=TRUE;
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
			case 'j': sscanf (optarg,"%d",&_fw_browser_plugin);  break;
			case 'k': sscanf (optarg,"%u",&_fw_instance); break;
			case 'v': printf ("FreeWRL version: %s\n",FWVER); exit(0);break;
			/* Petr Mikiluk - ILS line width */
			case 'W': sscanf (optarg,"%g",&tmp); setLineWidth(tmp); break;

			case 'Q': be_collision = FALSE; break;


			/* Snapshot stuff */
			case 'l': setSnapSeq(); break;
			case 'p': snapGif = TRUE; break;
			case 'q': sscanf (optarg,"%d",&maxSnapImages);
				  setMaxImages(maxSnapImages);
				  break;

			case 'm':
				  setSeqFile(argv[optind]);
				  break;
			case 'n':
				  setSnapFile(argv[optind]);
				  break;
			case 'o':
				  setSeqTemp(argv[optind]);
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
	if (optind < argc) {
		if (optind != (argc-1)) {
			printf ("freewrl:warning, expect only 1 file on command line; running file: %s\n",
				argv[optind]);
		}

		/* save the url for later use, if required */
		setBrowserURL (argv[optind]);
	} else {
		ConsoleMessage ("freewrl:missing VRML/X3D file name\n");
		exit(1);
	}

        /* create the initial scene, from the file passed in
        and place it as a child of the rootNode. */

        initialFilename = (char *)malloc(1000 * sizeof (char));
        pwd = (char *)malloc(1000 * sizeof (char));
        getcwd(pwd,1000);
        /* if this is a network file, leave the name as is. If it is
         * a local file, prepend the path to it */

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

	/* start threads, parse initial scene, etc */
	initFreewrl();

	free(initialFilename); free(pwd);

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
	//ConsoleMessage ("FreeWRL got a sigquit signal");
	// shut up any SIGSEGVs we might get now.
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
    //fprintf(stderr,"An alarm signal just arrived ...IT WAS IGNORED!\n");
    /* end of alarm actions */

    alarm(0);
    signal(SIGALRM, catch_SIGALRM);
}
