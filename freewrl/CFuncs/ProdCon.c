/*******************************************************************
 Copyright (C) 2003 John Stewart, CRC Canada.
 DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
 See the GNU Library General Public License (file COPYING in the distribution)
 for conditions of use and redistribution.
*********************************************************************/

#include "EXTERN.h"
#include "perl.h"

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


struct PSStruct {
	unsigned type;		// what is this task? 
	char *inp;		// data for task
	unsigned ptr;		// node to put data
	unsigned ofs;		// offset in node for data

	pthread_t waitfor;	// thread to wait for completion of
};



void addToNode (unsigned rc, unsigned newNode);
void consumeTask (struct PSStruct *psptr);
void loadInitialGroup(void); 
void openBrowser(void); 
unsigned int CreateVrml (char *tp, char *inputstring, unsigned int *retarr);

extern void xs_init(void);
extern unsigned rootNode;
extern Display *dpy;
extern Window win;
extern GLXContext cx;


// variables for gl context switching.
pthread_mutex_t _ContextLock 	= PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t condition_mutex	= PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t	condition_cond	= PTHREAD_COND_INITIALIZER;
int TextureWantsCX = FALSE;	
#define LOCK()     pthread_mutex_lock(&_ContextLock)
#define UNLOCK()   pthread_mutex_unlock(&_ContextLock)
#define CXSIG()	   pthread_cond_signal(&condition_cond)
#define CXWAIT()   pthread_cond_wait (&condition_cond, &condition_mutex)




// keep track of the last producer thread made
pthread_t lastPCthread = NULL;
static int browserRunning=FALSE;
PerlInterpreter *my_perl;


void produceTask(unsigned type, char *inp, unsigned ptr, unsigned ofs) {
	struct PSStruct *psptr;
	int iret;


	psptr = malloc (sizeof (struct PSStruct));
	if (!psptr) {printf ("malloc failure in produceTask\n"); exit(1);}

	psptr->type = type;
	psptr->ptr = ptr;
	psptr->ofs = ofs;
	psptr->inp = malloc (strlen(inp)+2);
	if (!(psptr->inp)) {printf ("malloc failure in produceTask\n"); exit(1);}
	memcpy (psptr->inp,inp,strlen(inp)+1);
	psptr->waitfor = lastPCthread;

	// create consumer thread
	iret = pthread_create (&lastPCthread, NULL, (void *)&consumeTask, (void *) psptr);
}
	

void consumeTask(struct PSStruct *psptr) {
	int count;
	int retval;
	int retarr[1000];
        char *commandline[] = {"", NULL};

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
		if ((psptr->ptr==0) && (psptr->ofs==offsetof(
			struct VRML_Group, children))) {
			psptr->ptr=rootNode;
		}
	}

	if ((psptr->type == FROMSTRING) || (psptr->type==FROMURL)) {
		if (psptr->type==FROMSTRING) {
        		retval = CreateVrml("String",psptr->inp,retarr);
		} else {
			retval = CreateVrml("URL",psptr->inp,retarr);
		} 

        	// now that we have the VRML/X3D file, load it into the scene.
        	// retarr contains node number/memory location pairs; thus the count
        	// by two.
        	for (count =1; count < retval; count+=2) {
        		addToNode(psptr->ptr+psptr->ofs, retarr[count]);
        	}
	} else {
		printf ("produceTask - invalid type!\n");
	}
	free (psptr->inp);
	free (psptr);
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
