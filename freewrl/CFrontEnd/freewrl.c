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

#ifdef AQUA

#include <gl.h>
#include <glu.h>
#include <glext.h>

#else

#include <GL/gl.h>
#include <GL/glx.h>
#include <GL/glu.h>
#include <GL/glext.h>

#endif

// display and win are opened here, then pointers passed to
// freewrl. We just use these as unsigned, because we just
// pass the pointers along; we do not care what type they are.
Display *Disp;
Window Win;
GLXContext globalContext;

/* threading variables for event loop */
static pthread_t *loopthread;
char *threadmsg = "eventloop";
pthread_t thread1;


/* function prototypes */
//extern void xs_init(void);
extern void openMainWindow(Display *dpy, Window *win,
		GLXContext *glocx);
extern unsigned rootNode;
void displayThread();
extern void glpOpenGLInitialize(void);
extern void new_tessellation(void);
extern char *BrowserURL;
extern produceTask(unsigned a,char *b,unsigned ptr, unsigned ofs);
extern PerlInterpreter *my_perl;
extern void setGeometry (char *optarg);

int main (int argc, char **argv) {
	int retval;
	int count;
	int c;
	int digit_optind = 0;

	/* parse command line arguments */
	while (1) {
		int this_option_optind = optind ? optind : 1;
		int option_index = 0;
		static struct option long_options[] = {
			{"version", 0, 0, 'x'},
			{"fullscreen", 0, 0, 'x'},
			{"plugin", 1, 0, 'x'},
			{"geometry", 1, 0, 'g'},
			{"parent", 1, 0, 'x'},
			{"seq", 0, 0, 'x'},
			{"seqb",1, 0, 'x'},
			{"snapb", 1, 0, 'x'},
			{"seqtmp", 1, 0, 'x'},
			{"shutter", 0, 0, 'x'},
			{"eyedist", 1, 0, 'x'},
			{"screendist", 1, 0, 'x'},
			{"gif", 0, 0, 'x'},
			{"maximg", 1, 0, 'x'},
			{"eai", 1, 0, 'x'},
			{"server", 1, 0, 'x'},
			{"sig", 1, 0, 'x'},
			{"ps", 1, 0, 'x'},
			{"help", 0, 0, 'h'},
			{0, 0, 0, 0}
		};

		c = getopt_long (argc, argv, "h", long_options, &option_index);
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

			case 'h':
				printf ("\nFreeWRL VRML/X3D browser from CRC Canada (http://www.crc.ca)\n");
				printf ("   type \"man freewrl\" to view man pages\n\n");
				break;

			case 'g':
				printf ("Geometry selected, with arg: %s\n",optarg);
				setGeometry(optarg);
				break;
			default:
				/* printf ("?? getopt returned character code 0%o ??\n", c); */
				break;
		}
	}
	if (optind < argc) {
		if (optind != (argc-1)) {
			printf ("freewrl:warning, expect only 1 file on command line; running file: %s\n", 
				argv[optind]);
		}

		/* save the url for later use, if required */
		if (BrowserURL != NULL) free (BrowserURL);
		BrowserURL = malloc (strlen(argv[optind])+1);
		strcpy (BrowserURL,argv[optind]);


	} else {
		printf ("freewrl:missing VRML/X3D file name\n");
		exit(1);
	}

	/* create the display thread. */
	pthread_create (&thread1, NULL, (void *)&displayThread, (void *)threadmsg);

	/* create the initial scene, from the file passed in
	and place it as a child of the rootNode. */

	produceTask(FROMURL, argv[optind],
		rootNode, offsetof (struct VRML_Group, children));

	/* now wait around until something kills this thread. */
	pthread_join(thread1, NULL);

	perl_destruct(my_perl);
	perl_free(my_perl);
}


/* handle all the displaying and event loop stuff. */
void displayThread() {
	int count;
	openMainWindow(Disp,&Win,&globalContext);
	glpOpenGLInitialize();
	new_tessellation();

	while (1==1) {
		EventLoop();
	}
}
