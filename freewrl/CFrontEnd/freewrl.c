/*******************************************************************
 Copyright (C) 2003 John Stewart, CRC Canada.
 DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
 See the GNU Library General Public License (file COPYING in the distribution)
 for conditions of use and redistribution.
*********************************************************************/

/* beginnings of FreeWRL in C */
#include "EXTERN.h"
#include <perl.h>
#include <Structs.h>
#include <pthread.h>
#include <headers.h>
#include <getopt.h>
#include <Snapshot.h>
#include <PluginSocket.h>
#include <stdlib.h>
#include <unistd.h>
#include <dlfcn.h>
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
extern float global_linewidth;	/* in CFrontEnd/fwopts.c - ILS line width */
extern void   XEventStereo();

extern int perlParse(unsigned type, char *inp, int bind, int returnifbusy,
                        unsigned ptr, unsigned ofs, int *complete,
                        int zeroBind);


extern void openMainWindow (Display *Disp, unsigned *Win, GLXContext *Cont);
extern void glpOpenGLInitialize();
extern void EventLoop();
extern void resetGeometry();

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

//These variables exsist for the device input libraries. Linux ONLY.
//Added Nov 18/04 by M. Ward
#ifdef ALLDEV

  #ifdef do_open
    #undef do_open
  #endif
  #ifdef do_close
    #undef do_close
  #endif

  #include "libmio.h"
  #include "libmread.h"
  #include "interface.h"
  #include "dev_core.h"

  void* cyberlib; //this is for the cyberglove library(unused at this time Nov 18/04 )
  create_t* create_cyber; //this is for creating a 'copy' the library
  destroy_t* destroy_cyber; //this will destroy our copy of the library
  void* flocklib;  //this is for the ascension flock library
  create_t* create_flock;
  destroy_t* destroy_flock;
  void* joylib;	 //this is for the joystick library
  create_t* create_joy;
  destroy_t* destroy_joy;
  void* spacelib;  //this is for the spaceball library
  create_t* create_space;
  destroy_t* destroy_space;
  void* pollib;  //this is for the polhemus fastrak library
  create_t* create_polhemus;
  destroy_t* destroy_polhemus;
  //these are for the 'copies' of the libraries to go into
  device *glove;
  device *fastrak;
  device *nest;
  device *joy;
  device *space;
  device *global_dev;
  //this is an instance of the device manager class
  managedev Manager;

  //these are here to track the presence of the library
  int have_cyberlib = -1;
  int have_flocklib = -1;
  int have_spacelib = -1;
  int have_joylib = -1;
  int have_polhemuslib = -1;
  int connected_device = -1;
  int use_external_input = 1;
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
	if(strcmp(FWVER,getLibVersion())){
  	  ConsoleMessage ("FreeWRL expected library version %s, got %s...\n",FWVER,getLibVersion());
	}
#endif
#endif

#ifndef AQUA
	/* set the screen width and height before getting into arguments */
	screenWidth = 450; screenHeight=300;
	fullscreen = 0;

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
			/* Petr Mikiluk - ILS line width */
			case 'W': sscanf (optarg,"%g",&global_linewidth); break;

			case 'Q': be_collision = FALSE; break;


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
				  snapseqB = (char *)malloc (count+1);
                		  strcpy (snapseqB,argv[optind]);
				  break;
			case 'n':
				  count = strlen(argv[optind]);
				  if (count > 500) count = 500;
				  snapsnapB = (char *)malloc (count+1);
                		  strcpy (snapsnapB,argv[optind]);
				  break;
			case 'o':
				  count = strlen(argv[optind]);
				  if (count > 500) count = 500;
				  seqtmp = (char*)malloc (count+1);
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
		BrowserURL = (char *)malloc (count+1);
		strcpy (BrowserURL,argv[optind]);
	} else {
		ConsoleMessage ("freewrl:missing VRML/X3D file name\n");
		exit(1);
	}
#endif

//Added Nov 18/04 M. Ward
#ifdef ALLDEV
      //first a quick check did you want to use an external device for input control?
      if( use_external_input == 1 ) {
  	//now before we start forking threads and I get confused, lets check the status
	//of the libraries. Are they present or not?
	cyberlib = dlopen("libcyberglove.so.1", RTLD_LAZY);
	if( !cyberlib ) {
   	  printf("Cannot load library: %s .\n\r",dlerror() );
	} else {
	  //try and get handles to the create and destroy routines
	  create_cyber = (create_t*) dlsym(cyberlib, "createdev");
  	  destroy_cyber = (destroy_t*) dlsym(cyberlib, "destroydev");
	  //if we can;t get the access routines, we need to show the error
          if (!create_cyber || !destroy_cyber) {
            printf("Cannot load symbols: %s .\n\r",dlerror() );
  	  } else {
	    //As this library is unused these are commented out, add them back in if you
	    //want to enable the cyberglove
	    //glove = create_cyber();
	    //if( glove != NULL ) {	
  	      //have_cyberlib = 1;
	      //clear the interal data structures of the library
	      //memset( &glove->info, 0, sizeof( space->info ) );
            //}
	    //remove this to prevent the library from closing before you use it
	    dlclose( cyberlib );
	  }
	}
        flocklib =  dlopen("libascension.so.1", RTLD_LAZY);
        if( !flocklib ) {
          printf("Cannot load library: %s .\n\r",dlerror() );
        } else {
          //try and get handles to the create and destroy routines
          create_flock = (create_t*) dlsym(flocklib, "createdev");
          destroy_flock = (destroy_t*) dlsym(flocklib, "destroydev");
          //if we can;t get the access routines, we need to show the error
          if (!create_flock || !destroy_flock) {
            printf("Cannot load symbols: %s .\n\r",dlerror() );
          } else {
            nest = create_flock();
	    if( nest != NULL ) {
              have_flocklib = 1;
  	      memset( &nest->info, 0, sizeof( nest->info ) );
	    }
	  }
        }
        pollib =  dlopen("libpolhemus.so.1", RTLD_LAZY);
        if( !pollib ) {
          printf("Cannot load library: %s .\n\r",dlerror() );
        } else {
          //try and get handles to the create and destroy routines
          create_polhemus = (create_t*) dlsym(pollib, "createdev");
          destroy_polhemus = (destroy_t*) dlsym(pollib, "destroydev");
          //if we can;t get the access routines, we need to show the error
          if (!create_polhemus || !destroy_polhemus) {
            printf("Cannot load symbols: %s .\n\r",dlerror() );
          } else {
            fastrak = create_polhemus();
	    if( fastrak != NULL ) {
              have_polhemuslib = 1;
	      memset( &fastrak->info, 0, sizeof( fastrak->info ) );
	    }
	  }
        }
	joylib = dlopen("libjoystick.so.1", RTLD_LAZY);
        if( !joylib ) {
          printf("Cannot load library: %s .\n\r",dlerror() );
        } else {
          //try and get handles to the create and destroy routines
          create_joy = (create_t*) dlsym(joylib, "createdev");
          destroy_joy = (destroy_t*) dlsym(joylib, "destroydev");
          //if we can;t get the access routines, we need to show the error
          if (!create_joy || !destroy_joy) {
            printf("Cannot load symbols: %s .\n\r",dlerror() );
          } else {
            joy = create_joy();
	    if( joy != NULL ) {
              have_joylib = 1;
	      memset( &joy->info, 0, sizeof( joy->info ) );
	    }
	  }
        }
        spacelib = dlopen("libspaceball.so.1", RTLD_LAZY);
        if( !spacelib ) {
          printf("Cannot load library: %s .\n\r",dlerror() );
        } else {
          //try and get handles to the create and destroy routines
          create_space = (create_t*) dlsym(spacelib, "createdev");
          destroy_space = (destroy_t*) dlsym(spacelib, "destroydev");
          //if we can;t get the access routines, we need to show the error
          if (!create_space || !destroy_space) {
            printf("Cannot load symbols: %s .\n\r",dlerror() );
          } else {
            space = create_space();
	    if( space != NULL ) {
              have_spacelib = 1;
	      memset( &space->info, 0, sizeof( space->info ) );
	    }
	  }
        }

	//this section actually tries to connect to the input device, and verify if it's 
	//active and able to provide data
	//Arbitrarily I've decided that the order of prefrence for the devices is:
	// The Ascension, followed by the polhemus, then the joystick, then the spaceball
	//and lastly the cyberglove. If any of the devices is found, then no other devices
	//will be connected(at this time)
	/*if( ( have_flocklib ) && ( connected_device == -1 ) ) {
	  printf("Looking for Ascension..\n\r");
	  //have the library auto-search for the device
	  //if a value other than zero was returned we found something
	  if( (nest->info.filep = nest->FindDev( &Manager )) != 0 ) {
            //check that the device is connected
  	    if( ( nest->VerifyDev( nest->info.filep ) ) != 0 ){
	      printf("Verified the flock!\n\r");
	      nest->info.NumberOfSensors = nest->GetNumberOfSensors(nest->info);
	      connected_device = 1;
	      nest->info.Type = FLOCKDEV;
	      global_dev = nest;
	    }else {
	      #ifdef DEBUG
	        printf("An error has occured while attempting to verify the connection to the Ascension. \n\p");
	      #endif
	    }
	  }
	}
	if( ( have_polhemuslib ) && ( connected_device == -1 ) ) {
	  printf("Looking for Polhemus..\n\r");
	  //have the library auto-search for the device
	  //if a value other than zero was returned we found something
	  if ( (fastrak->info.filep = fastrak->FindDev( &Manager )) != 0 ) {
            //check that the device is connected
  	    if( ( fastrak->VerifyDev( fastrak->info.filep ) ) != 0 ){
	      printf("Verified the Polhemus\n\r");
	      fastrak->info.NumberOfSensors = fastrak->GetNumberOfSensors(fastrak->info);
  	      connected_device = 1;
	      fastrak->info.Type = FASTRAK;
	      global_dev = fastrak;
  	    }else {
	      #ifdef DEBUG
	        printf("An error has occured while attempting to verify the connection to the Fastrak. \n\p");
	      #endif
	    }
	  }
	}
	if( ( have_joylib ) && ( connected_device == -1 ) ) {
	  printf("Looking for Joystick..\n\r");	
	  //have the library auto-search for the device
	  //if a value other than zero was returned we found something
	  if( (joy->info.filep = joy->FindDev( &Manager )) != 0 ) {
            //check that the device is connected
  	    if( ( joy->VerifyDev( joy->info.filep ) ) != 0 ){
	      printf("Verified the Joystick\n\r");
              joy->info.NumberOfSensors = joy->GetNumberOfSensors( joy->info );
 connected_device = 1;
              joy->info.Type = JOYDEV;
              global_dev = joy;
            }else {
              #ifdef DEBUG
                printf("An error has occured while attempting to verify the connection to the Joystick. \n\p");
              #endif
            }
          }
        }*/
        if( ( have_spacelib ) && ( connected_device == -1 ) ) {
	  printf("Looking for Spaceball..\n\r");
          //have the library auto-search for the device
          //if a value other than zero was returned we found something
          if( (space->info.filep = space->FindDev( &Manager )) != 0 ) {
            //check that the device is connected
            if( ( space->VerifyDev( space->info.filep ) ) != 0 ){
              printf("Verified the Spaceball\n\r");
              space->info.NumberOfSensors = space->GetNumberOfSensors( space->info );
 connected_device = 1;
              space->info.Type = SPACEDEV;
              global_dev = space;
            }else {
              #ifdef DEBUG
                printf("An error has occured while attempting to verify the connection to the Spaceball. \n\p");
              #endif
            }
          }
        }
      }
#endif
   /* create the display thread. */
        pthread_create (&thread1, NULL, (void *(*)(void *))&displayThread, (void *)threadmsg);
                                                                                                          
#ifndef AQUA
        /* create the Perl parser thread */
        initializePerlThread(PERLPATH);
#endif
        while (!isPerlinitialized()) {usleep(50);}
                                                                                                          
        /* create the Texture parser thread */
        initializeTextureThread();
        while (!isTextureinitialized()) {usleep(50);}
                                                                                                          
        /* get the Netscape Browser name, if we are pluggind */
        NetscapeName[0] = (char)NULL;
        if (RUNNINGASPLUGIN) {
                if (read(_fw_FD, NetscapeName,MAXNETSCAPENAMELEN) < 0) {
                }
        }
                                                                                                          
                                                                                                          
        /* create the initial scene, from the file passed in
        and place it as a child of the rootNode. */
                                                                                                          
        filename = (char *)malloc(1000 * sizeof (char));
        pwd = (char *)malloc(1000 * sizeof (char));
        getcwd(pwd,1000);
#ifndef AQUA
        /* if this is a network file, leave the name as is. If it is
         * a local file, prepend the path to it */
        if (checkNetworkFile(argv[optind])) {
                strcpy (filename,argv[optind]);
                BrowserFullPath = (char *)malloc ((strlen(argv[optind])+1) * sizeof(char));
                strcpy(BrowserFullPath,pwd);
                                                                                                          
        } else {
                                                                                                          
                makeAbsoluteFileName(filename, pwd, argv[optind]);
                BrowserFullPath = (char *)malloc ((strlen(filename)+1) * sizeof(char));
                strcpy (BrowserFullPath,filename);
        }
#endif

        // printf ("FrontEnd, filename %s\n",filename);
	perlParse(FROMURL, filename,TRUE,FALSE,
		rootNode, offsetof (struct VRML_Group, children),&tmp,TRUE);

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
	openMainWindow(Disp,(unsigned int *)&Win,&globalContext);
#endif

	glpOpenGLInitialize();
	new_tessellation();

	while (1==1) {
		EventLoop();
	}
	if (fullscreen)
		resetGeometry();
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

/* funnel all print statements through here - this allows us to
 * eventually put errors/messages on a window
 * */

#ifndef AQUA
/* stop all of FreeWRL, terrible error! */
void freewrlDie (const char *format) {
	ConsoleMessage ("Catastrophic error: %s\n",format);
	doQuit();
}
#endif
