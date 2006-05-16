/*******************************************************************
 Copyright (C) 2003 John Stewart, CRC Canada.
 DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
 See the GNU Library General Public License (file COPYING in the distribution)
 for conditions of use and redistribution.
*********************************************************************/
#include <headers.h>
#include <unistd.h>
#include <getopt.h>
#include <Snapshot.h>

extern int wantEAI;

/* for plugin running - these are read from the command line */
extern int _fw_pipe;
extern int _fw_browser_plugin;
extern uintptr_t _fw_instance;
/* keypress sequence for startup - Robert Sim */
extern char *keypress_string;

extern int xPos;
extern int yPos;

int parseCommandLine (int argc, char **argv) {
	int c;
	int tmp;

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
				/* set negative so that the texture thread will pick this up */
				setTexSize(-256);
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
				/* display_status = 0; */
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

	if (optind < argc) {
		if (optind != (argc-1)) {
			printf ("freewrl:warning, expect only 1 file on command line; running file: %s\n",
				argv[optind]);
		}

		/* save the url for later use, if required */
		setBrowserURL (argv[optind]);
	} else {
		/* printf ("no options  - just make BrowserURL point to nothing\n"); */
		setBrowserURL ("");
		return FALSE;
	}
	return TRUE;
}

