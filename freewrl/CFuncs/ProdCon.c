/*******************************************************************
 Copyright (C) 2003 John Stewart, CRC Canada.
 DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
 See the GNU Library General Public License (file COPYING in the distribution)
 for conditions of use and redistribution.
*********************************************************************/

#include "EXTERN.h"
#include "perl.h"
#include "Bindable.h"

#include <stdio.h>
#include "Structs.h"
#include "headers.h"
#include <pthread.h>
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

/* thread synchronization issues */
pthread_mutex_t condition_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  condition_cond  = PTHREAD_COND_INITIALIZER;

#define DATA_LOCK       	pthread_mutex_lock(&condition_mutex);
#define DATA_LOCK_SIGNAL        pthread_cond_signal(&condition_cond);
#define DATA_UNLOCK     	pthread_mutex_unlock(&condition_mutex);
#define DATA_LOCK_WAIT          pthread_cond_wait(&condition_cond, &condition_mutex);

struct PSStruct {
	unsigned type;		/* what is this task? 			*/
	char *inp;		/* data for task (eg, vrml text)	*/
	unsigned ptr;		/* address (node) to put data		*/
	unsigned ofs;		/* offset in node for data		*/
	int bind;		/* should we issue a bind? 		*/
	char *path;		/* path of parent URL			*/
};



void addToNode (unsigned rc, unsigned newNode);
void _perlThread (void);
void loadInitialGroup(void); 
void openBrowser(void); 
void initializePerlThread(void);
unsigned int CreateVrml (char *tp, char *inputstring, unsigned int *retarr);
unsigned int getBindables (char *tp, unsigned int *retarr);
void getAllBindables(void);
int isPerlinitialized(void);
int perlParse(unsigned type, char *inp, int bind, int returnifbusy,
			unsigned ptr, unsigned ofs);

/* Bindables */
int *fognodes;
int *backgroundnodes;
int *navnodes;
int *viewpointnodes;
int totfognodes = 0;
int totbacknodes = 0;
int totnavnodes = 0;
int totviewpointnodes = 0;
int currboundvpno=0;

/* keep track of the producer thread made */
pthread_t PCthread;

/* is the Browser initialized? */
static int browserRunning=FALSE;

/* is the perlParse thread created? */
int PerlInitialized=FALSE;

/* is the parsing thread active? this is read-only, used as a "flag" by other tasks */
int PerlParsing=FALSE;

/* the actual perl interpreter */
PerlInterpreter *my_perl;

/* psp is the data structure that holds parameters for the parsing thread */
struct PSStruct psp;

void initializePerlThread() {
	int iret;

	/* create consumer thread and set the "read only" flag indicating this */
	iret = pthread_create (&PCthread, NULL, (void *)&_perlThread, NULL);
}

/* is Perl running? this is a function, because if we need to mutex lock, we
   can do all locking in this file */
int isPerlinitialized() {return PerlInitialized;}

/* statusbar uses this to tell user that we are still loading */
int isPerlParsing() {return(PerlParsing);}

/* does this file exist on the local file system, or via the HTML Browser? */
int fileExists(char *fname, char *firstBytes) {
	FILE *fp;
	int ok, tx;

	/* are we running under netscape? if so, ask the browser */

	/* we are not netscaped */
	fp= fopen (fname,"r");
	ok = (fp != NULL);

	/* try reading the first 4 bytes into the firstBytes array */
	if (ok) {
		if (fread(firstBytes,1,4,fp)!=4) ok = FALSE;
		fclose (fp);
	} 
	return (ok);
}

/* Inlines... Multi_URLs, load only when available, etc, etc */
void loadInline(struct VRML_Inline *node) {
	/* first, are we busy? */
	if (PerlParsing) return;

	node->__loadstatus = 1;
	perlParse(INLINE,(char *)node, FALSE, FALSE, 
		(unsigned *) node,
		offsetof (struct VRML_Inline, __children));
}

int perlParse(unsigned type, char *inp, int bind, int returnifbusy,
			unsigned ptr, unsigned ofs) {
	int iret;

	/* do we want to return if the parsing thread is busy, or do
	   we want to wait? */
	if (returnifbusy) {
		if (PerlParsing) return (FALSE);
	}
	DATA_LOCK
	/* copy the data over; malloc and copy input string */
	psp.type = type;
	psp.ptr = ptr;
	psp.ofs = ofs;
	psp.path = NULL;
	psp.bind = bind; /* should we issue a set_bind? */
	psp.inp = malloc (strlen(inp)+2);
	if (!(psp.inp)) {printf ("malloc failure in produceTask\n"); exit(1);}
	memcpy (psp.inp,inp,strlen(inp)+1);
	DATA_LOCK_SIGNAL
	DATA_UNLOCK
	return (TRUE);
}
	

void _perlThread() {
	int count;
	int retval;
	int retarr[1000];
	char *filename;
        char *commandline[] = {"", NULL};
	struct Multi_String *inurl;
	struct VRML_Inline *inl;
	int xx;
	char *thisurl;
	char *slashindex;
	char firstBytes[4];

	// is the browser started yet? 
	if (!browserRunning) {

		commandline[1] = "/usr/bin/fw2init.pl";

		/* initialize stuff for prel interpreter */
		my_perl = perl_alloc();
		perl_construct (my_perl);
		if (perl_parse(my_perl, xs_init, 2, commandline, NULL)) {
			printf ("freewrl can not find its initialization script %s\n",commandline[1]);
			exit (1);
		}

		//printf ("opening browser\n");
		openBrowser();

		//printf ("loading in initial Group{} \n");
		loadInitialGroup();
		browserRunning=TRUE;

		//Now, possibly this is the first VRML file to
		//add. Check to see if maybe we have a ptr of 0.
		if ((psp.ptr==0) && (psp.ofs==offsetof(
			struct VRML_Group, children))) {
			psp.ptr=rootNode;
		}
	}

	/* now, loop here forever, waiting for instructions and obeying them */
	for (;;) {
		DATA_LOCK
		PerlInitialized=TRUE; /* have to do this AFTER ensuring we are locked */
		DATA_LOCK_WAIT
		PerlParsing=TRUE;

		/* is this a INLINE? If it is, try to load one of the URLs. */
		if (psp.type == INLINE) {
			inl = (struct VRML_Inline *)psp.ptr;
			inurl = &(inl->url);
			filename = malloc(1000);

			/* lets make up the path and save it, and make it the global path */
			count = strlen(SvPV(inl->__parenturl,xx));
			psp.path = malloc (count+1);

			if ((!filename) || (!psp.path)) {
				printf ("perl thread can not malloc for filename\n");
				exit(1);
			}
			
			/* copy the parent path over */
			strcpy (psp.path,SvPV(inl->__parenturl,xx));

			/* and strip off the file name, leaving any path */
			slashindex = rindex(psp.path,'/');
			if (slashindex != NULL) { 
				slashindex ++; /* leave the slash there */
				*slashindex = 0;
			} else {psp.path[0] = 0;}

			/* try the first url, up to the last */
			count = 0;
			while (count < inurl->n) {
				thisurl = SvPV(inurl->p[count],xx);

				/* check to make sure we don't overflow */
				if ((strlen(thisurl)+strlen(psp.path)) > 900) break;

				/* does this name start off with a ftp, http, or a "/"? */
				if ((strncmp(thisurl,"ftp://", strlen("ftp://"))) &&
				   (strncmp(thisurl,"FTP://", strlen("FTP://"))) &&
				   (strncmp(thisurl,"http://", strlen("http://"))) &&
				   (strncmp(thisurl,"HTTP://", strlen("HTTP://"))) &&
				   (strncmp(thisurl,"/",strlen("/")))) {
					strcpy (filename,psp.path);
				} else {
					filename[0]=0;
				}
				strcat(filename,thisurl);

				if (fileExists(filename,firstBytes)) {
					break;
				}
				count ++;
			}
			psp.inp = filename; /* will be freed later */

			/* were we successful at locating one of these? if so,
			   make it into a FROMURL */
			if (count != inurl->n) {
				/* printf ("we were successful at locating %s\n",filename); */
				psp.type=FROMURL;
			} else {
				printf ("Could not locate url (last choice was %s)\n",filename);
			}
		}

			
		if ((psp.type == FROMSTRING) || (psp.type==FROMURL)) {
			if (psp.type==FROMSTRING) {
	        		retval = CreateVrml("String",psp.inp,retarr);
			} else {
				if (!fileExists(psp.inp,firstBytes)) { 
					retval=0;
					psp.bind=0;
					printf ("file problem: %s does not exist\n",psp.inp);
				} else {
					retval = CreateVrml("URL",psp.inp,retarr);
				}
			} 
	
	        	// now that we have the VRML/X3D file, load it into the scene.
	        	// retarr contains node number/memory location pairs; thus the count
	        	// by two.
	        	for (count =1; count < retval; count+=2) {
	        		addToNode(psp.ptr+psp.ofs, retarr[count]);
	        	}
	
			/* get the Bindables from this latest VRML/X3D file */
			if (retval > 0) getAllBindables();
	
			/* send a set_bind to any nodes that exist */
			if (psp.bind) {
				if (totfognodes != 0) send_bind_to (FOG,fognodes[0],1);
				if (totbacknodes != 0) send_bind_to (BACKGROUND,backgroundnodes[0],1);
				if (totnavnodes != 0) send_bind_to (NAVIGATIONINFO,navnodes[0],1);
				if (totviewpointnodes != 0) send_bind_to(VIEWPOINT,viewpointnodes[0],1);
			}
		} else if (psp.type==INLINE) {
			/* this should be changed to a FROMURL before here */
			printf ("Inline unsuccessful\n");
		} else {
			printf ("produceTask - invalid type!\n");
		}
		if (psp.inp) free (psp.inp);
		if (psp.path) free (psp.path);

		PerlParsing=FALSE;
		DATA_UNLOCK
	}
}

// add a node to the root group. ASSUMES ROOT IS A GROUP NODE! (it should be)
// this code is very similar to getMFNode in CFuncs/CRoutes.c, except that
// we do not pass in a string of nodes to assign. (and, do not remove, etc)
void addToNode (unsigned rc, unsigned newNode) {

	int oldlen, newlen;
	unsigned *newmal;
	unsigned *place;
	struct Multi_Node *par;
	unsigned *tmp;

	par = (struct Multi_Node *) rc;

	/* oldlen = what was there in the first place */
	oldlen = par->n;
	newlen=1;

	newmal = malloc ((oldlen+newlen)*sizeof(unsigned int));
	if (newmal == 0) {
		printf ("cant malloc memory for addChildren");
		return;
	}
	
	/* copy the old stuff over */
	if (oldlen > 0) memcpy (newmal,par->p,oldlen*sizeof(unsigned int));

	/* increment pointer to point to place for new addition */
	place = (void *) ((int) newmal + sizeof (unsigned int) * oldlen);

	/* and store the new child. */
	*place = newNode;

	/* set up the C structures for this new MFNode addition */
	tmp = par->p;
	par->p = (void *)newmal;
	par->n = oldlen+newlen;
	free (tmp);
}

/* get all of the bindables from the Perl side. */
void getAllBindables() {
	int retval;
	int aretarr[1000];
	int bretarr[1000];
	int cretarr[1000];
	int dretarr[1000];
	int count;

	/* first, free any previous nodes */
	if (fognodes) free (fognodes);
	if (backgroundnodes) free (backgroundnodes);
	if (navnodes) free (navnodes);
	if (viewpointnodes) free (viewpointnodes);

	/* now, get the values */
	totviewpointnodes = getBindables("Viewpoint",aretarr);
	totfognodes = getBindables("Fog",bretarr);
	totnavnodes = getBindables("NavigationInfo",cretarr);
	totbacknodes = getBindables("Background",dretarr);

	/* and, malloc the memory needed */
	viewpointnodes = malloc (sizeof(int)*totviewpointnodes);
	navnodes = malloc (sizeof(int)*totnavnodes);
	backgroundnodes = malloc (sizeof(int)*totbacknodes);
	fognodes = malloc (sizeof(int)*totfognodes);

	/* and, copy the results over */
	memcpy (fognodes,bretarr,(unsigned) totfognodes*sizeof(int));
	memcpy (backgroundnodes,dretarr,(unsigned) totbacknodes*sizeof(int));
	memcpy (navnodes,cretarr,(unsigned) totnavnodes*sizeof(int));
	memcpy (viewpointnodes,aretarr,(unsigned) totviewpointnodes*sizeof(int));
}

/* Create VRML/X3D, returning an array of nodes */
unsigned int CreateVrml (char *tp, char *inputstring, unsigned int *retarr) {
	int count;
	unsigned int noderef;
	int tmp;

	dSP;
	ENTER;
	SAVETMPS;
	PUSHMARK(SP);
	XPUSHs(sv_2mortal(newSVpv(inputstring, 0)));


	PUTBACK;
	if (strcmp(tp,"URL")==0)
		count = call_pv("VRML::Browser::EAI_CreateVrmlFromURL", G_ARRAY);
	else
		count = call_pv("VRML::Browser::EAI_CreateVrmlFromString", G_ARRAY);
	SPAGAIN ;

	//Perl is returning a series of BN/node# pairs, reorder to node#/BN.
	for (tmp = 1; tmp <= count; tmp++) {
		retarr[count-tmp] = POPi;
		/* printf ("popped off %d\n",retarr[count-tmp]); */
	}

	PUTBACK;
	FREETMPS;
	LEAVE;

	return (count);
}



unsigned int getBindables (char *tp, unsigned int *retarr) {
	int count;
	unsigned int noderef;
	int tmp, addr, ind;

	dSP;
	ENTER;
	SAVETMPS;
	PUSHMARK(SP);
	XPUSHs(sv_2mortal(newSVpv(tp, 0)));
	PUTBACK;
	count = call_pv("VRML::Browser::getBindables", G_ARRAY);
	SPAGAIN ;

	/* Perl is returning a series of Bindable node addresses */
	/* first comes the address, then the index. They might be out of order */
	count = count/2;
	for (tmp = 0; tmp < count; tmp++) {
		addr = POPi;
		ind = POPi;
		retarr[ind] = addr;
	}

	PUTBACK;
	FREETMPS;
	LEAVE;

	return (count);
}

void loadInitialGroup() {
	dSP;
	PUSHMARK(SP);
	call_pv("load_file_intro", G_ARRAY);
}


void openBrowser() {
	float eyedist = 10.0;	// have to really allow this to be passed in
	float screendist = 10.0;	// have to really allow this to be passed in
	int stereo = FALSE;
	
	set_viewer_type(1);

	set_eyehalf( eyedist/2.0,
		atan2(eyedist/2.0,screendist)*360.0/(2.0*3.1415926));

	if (stereo) 
		XEventStereo();

	
	dSP;
	ENTER;
	SAVETMPS;
	PUSHMARK(SP);
	XPUSHs(sv_2mortal(newSViv(1000))); // left in as an example
	XPUSHs(sv_2mortal(newSViv(2000)));
	PUTBACK;
	call_pv("open_browser", G_DISCARD);
	FREETMPS;
	LEAVE;
}
