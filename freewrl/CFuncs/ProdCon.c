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
#include "PluginSocket.h"
#include <pthread.h>
#ifdef AQUA

#include <gl.h>
#include <glu.h>
#include <glext.h>
#endif

#ifdef LINUX
#include <GL/gl.h>
#include <GL/glx.h>
#include <GL/glu.h>
#include <GL/glext.h>
#endif

#ifdef IRIX
#include <GL/gl.h>
#include <GL/glx.h>
#include <GL/glu.h>
#endif

#ifndef __jsUtils_h__
#include "jsUtils.h" /* misc helper C functions and globals */
#endif

#ifndef __jsVRMLBrowser_h__
#include "jsVRMLBrowser.h" /* VRML browser script interface implementation */
#endif

#include "jsVRMLClasses.h" /* VRML field type implementation */

#include "Viewer.h"

#define MAX_RUNTIME_BYTES 0x100000L
#define STACK_CHUNK_SIZE 0x2000L

/* thread synchronization issues */
pthread_mutex_t condition_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t psp_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  condition_cond  = PTHREAD_COND_INITIALIZER;

#ifdef AQUA
int _fw_FD = 0;
int _fw_pipe = 0;
unsigned _fw_instance;
#endif

/* for communicating with Netscape */
/* in headers.h extern int _fw_pipe, _fw_FD; */
extern unsigned _fw_instance;

#define DATA_LOCK       	pthread_mutex_lock(&condition_mutex);
#define DATA_LOCK_SIGNAL        pthread_cond_signal(&condition_cond);
#define DATA_LOCK_WAIT          pthread_cond_wait(&condition_cond, &condition_mutex);
#define DATA_UNLOCK     	pthread_mutex_unlock(&condition_mutex); 

//#define DATA_LOCK       	pthread_mutex_lock(&condition_mutex); \
					printf ("locked by %d\n",pthread_self());

//#define DATA_LOCK_SIGNAL        printf ("signalling by %d\n",pthread_self()); \
					pthread_cond_signal(&condition_cond); \
					printf ("signaled by %d\n",pthread_self());

//#define DATA_UNLOCK     	pthread_mutex_unlock(&condition_mutex); \
					printf ("unlocked by %d\n",pthread_self());
//#define DATA_LOCK_WAIT          printf ("waiting by %d\n",pthread_self()); \
					pthread_cond_wait(&condition_cond, &condition_mutex);


/* for debugging
#define PSP_LOCK		while(PerlParsing){printf("pp\n");usleep(10);}pthread_mutex_lock(&psp_mutex);
*/

#define PSP_LOCK		while(PerlParsing){usleep(10);}pthread_mutex_lock(&psp_mutex);
#define PSP_UNLOCK		pthread_mutex_unlock(&psp_mutex);


/*
#define PSP_LOCK
#define PSP_UNLOCK
*/
struct PSStruct {
	unsigned type;		/* what is this task? 			*/
	char *inp;		/* data for task (eg, vrml text)	*/
	unsigned ptr;		/* address (node) to put data		*/
	unsigned ofs;		/* offset in node for data		*/
	int bind;		/* should we issue a bind? 		*/
	char *path;		/* path of parent URL			*/
	int *comp;		/* pointer to complete flag		*/

	/* for javascript items, for Ayla's generic doPerlCallMethodVA call */
	/* warning; some fields shared by EAI */
	char *fieldname;	/* pointer to a static field name	*/
	unsigned Jptr[10];	/* array of x pointers    		*/
	char Jtype[10];		/* array of x pointer types (s or p)	*/
	int jparamcount;	/* number of parameters for this one	*/
	SV *sv;			/* the SV for javascript		*/

	/* for EAI */
	int *retarr;		/* the place to put nodes		*/
	int retarrsize;		/* size of array pointed to by retarr	*/
	unsigned Etype[10];	/* EAI return values			*/

	/* for class - return a string */
	char *retstr;
};



void _perlThread (void *perlpath);
void __pt_loadInitialGroup(void); 
void __pt_setPath(char *perlpath); 
void __pt_openBrowser(void); 
unsigned int _pt_CreateVrml (char *tp, char *inputstring, unsigned int *retarr);
unsigned int __pt_getBindables (char *tp, unsigned int *retarr);
void getAllBindables(void);
int isPerlinitialized(void);
int perlParse(unsigned type, char *inp, int bind, int returnifbusy,
			unsigned ptr, unsigned ofs, int *complete);
void __pt_doInline(void);
void __pt_doStringUrl (void);
void __pt_doPerlCallMethodVA(void);
void __pt_EAI_GetNode (void);
void __pt_EAI_GetType (void);
void __pt_EAI_GetTypeName (void);
void __pt_EAI_GetValue (void);
//JAS void __pt_EAI_replaceWorld (void);
void __pt_EAI_Route (void);
void EAI_readNewWorld(char *inputstring);

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

/* Initial URL loaded yet? - Robert Sim */
int URLLoaded=FALSE;

/* the actual perl interpreter */
PerlInterpreter *my_perl;

/* psp is the data structure that holds parameters for the parsing thread */
struct PSStruct psp;

char *myPerlInstallDir;

void initializePerlThread(char *perlpath) {
	int iret;

	myPerlInstallDir = malloc (strlen (perlpath) + 2);
	strcpy (myPerlInstallDir, perlpath);

	/* create consumer thread and set the "read only" flag indicating this */
	iret = pthread_create (&PCthread, NULL, (void *)&_perlThread, (void *) perlpath);
}

/* is Perl running? this is a function, because if we need to mutex lock, we
   can do all locking in this file */
int isPerlinitialized() {return PerlInitialized;}

/* statusbar uses this to tell user that we are still loading */
int isPerlParsing() {return(PerlParsing);}

/* is the initial URL loaded? Robert Sim */
int isURLLoaded() {return(URLLoaded&&!PerlParsing);}


/*
 * Check to see if the file name is a local file, or a network file.
 * return TRUE if it looks like a file from the network, false if it
 * is local to this machine
 */

int checkNetworkFile(char *fn) {
	if ((strncmp(fn,"ftp://", strlen("ftp://"))) &&
	   (strncmp(fn,"FTP://", strlen("FTP://"))) &&
	   (strncmp(fn,"http://", strlen("http://"))) &&
	   (strncmp(fn,"HTTP://", strlen("HTTP://"))) &&
	   (strncmp(fn,"urn://", strlen("urn://"))) &&
	   (strncmp(fn,"URN://", strlen("URN://")))) {
	   return FALSE;
	}
	return TRUE;
}


/* does this file exist on the local file system, or via the HTML Browser? */
/* WARNING! WARNING! the first parameter may be overwritten IF we are running
   within a Browser, so make sure it is large, like 1000 bytes. 	   */

int fileExists(char *fname, char *firstBytes) {
	FILE *fp;
	int ok;
	char *retName;

	char tempname[1000];
	char sysline[1000];

	/* are we running under netscape? if so, ask the browser, and 
	   save the name it returns (cache entry) */
	if (RUNNINGASPLUGIN && (strcmp(BrowserURL,fname)!=0)) {
		retName = requestUrlfromPlugin(_fw_FD,_fw_instance,fname);

		/* check for timeout; if not found, return false */
		if (!retName) return (FALSE);
		strcpy (fname,retName);
	}

	/* if not, do we need to invoke lwp to get the file, or 
	   is it just local? if we are running as a plugin, this should
	   be a local file by now 
	 */
	if (checkNetworkFile(fname)) {
		sprintf (tempname, "%s",tempnam("/tmp","freewrl_tmp"));
		sprintf (sysline,"wget %s -O %s\n",fname,tempname);
		printf ("\nFreeWRL will try to use wget to get %s\n",fname);
		system (sysline);
		strcpy (fname,tempname);
	}


	//printf ("fileExists, opening %s\n",fname);
	fp= fopen (fname,"r");
	ok = (fp != NULL);

	/* try reading the first 4 bytes into the firstBytes array */
	if (ok) {
		if (fread(firstBytes,1,4,fp)!=4) ok = FALSE;
		fclose (fp);
	} 
	return (ok);
}


/* filename is malloc'd, combine pspath and thisurl to make an
   absolute file name */
void makeAbsoluteFileName(char *filename, char *pspath,char *thisurl){

	/* lets try this - if we are running under a browser, let the
	   browser do the pathing stuff */
	if (RUNNINGASPLUGIN) {
		/* printf ("makeAbsolute, running under a browser, just copy\n"); */
		strcpy (filename,thisurl);
		return;
	}

	/* does this name start off with a ftp, http, or a "/"? */
	if ((!checkNetworkFile(thisurl)) && (strncmp(thisurl,"/",strlen("/"))!=0)) {
		//printf ("copying psppath over for %s\n",thisurl);
		strcpy (filename,pspath);
		/* do we actually have anything here? */
		if (strlen(pspath) > 0) strcat (filename,"/");

	} else {
		filename[0]=0;
	}
	strcat(filename,thisurl);

	/* and, return in the ptr filename, the filename created... */
	//printf ("makeAbsoluteFileName, just made :%s:\n",filename); 
}


/************************************************************************/
/*									*/
/* THE FOLLOWING ROUTINES INTERFACE TO THE PERL THREAD			*/
/*									*/
/************************************************************************/

/* Inlines... Multi_URLs, load only when available, etc, etc */
void loadInline(struct VRML_Inline *node) {
	/* first, are we busy? */
	if (PerlParsing) return;

	perlParse(INLINE,(char *)node, FALSE, FALSE, 
		(unsigned) node,
		offsetof (struct VRML_Inline, __children),
		&node->__loadstatus);
}

/* Javascript interface to the perl interpreter thread */
void doPerlCallMethodVA(SV *sv, const char *methodname, const char *format, ...) {
	va_list ap; /* will point to each unnamed argument in turn */
	char *c;
	void *v;
	size_t len = 0;
	const char *p = format;
	int complete;

	PSP_LOCK
	DATA_LOCK
	complete=0;
	/* copy the data over; malloc and copy input strings */
	psp.sv = sv;
	psp.comp = &complete;
	psp.type = CALLMETHOD;
	psp.ptr = (unsigned)NULL;
	psp.ofs = (unsigned)NULL;
	psp.path = NULL;
	psp.bind = FALSE; /* should we issue a set_bind? */
	psp.inp = NULL;
	psp.fieldname = (char *)methodname;

	psp.jparamcount = 0;
	va_start (ap,format);
	while (*p) {
		switch (*p++) {
		case 's':
			c = va_arg(ap, char *);
			len = strlen(c);
			c[len] = 0;
			psp.Jptr[psp.jparamcount]=(unsigned)c;
			psp.Jtype[psp.jparamcount]='s';
			break;
		case 'p':
			v = va_arg(ap, void *);
			psp.Jptr[psp.jparamcount]=(unsigned)v;
			psp.Jtype[psp.jparamcount]='p';
			break;
		default:
			fprintf(stderr, "doPerlCallMethodVA: argument type not supported!\n");
			break;
		}
		psp.jparamcount ++;
	}
	va_end(ap);

	DATA_LOCK_SIGNAL
	DATA_UNLOCK
	while (complete!=1) usleep(10);
	PSP_UNLOCK
}

/* interface for getting a node number via the EAI */
unsigned int EAI_GetNode(char *nname) {
	int complete;
	int retval;

	PSP_LOCK
	DATA_LOCK
	psp.comp = &complete;
	psp.type = EAIGETNODE;
	psp.ptr = (unsigned)NULL;
	psp.ofs = (unsigned)NULL;
	psp.path = NULL;
	psp.bind = FALSE; /* should we issue a set_bind? */
	psp.inp = NULL;
	psp.fieldname = nname;
	DATA_LOCK_SIGNAL
	DATA_UNLOCK
	while (complete!=1) usleep(10);
	retval = psp.jparamcount;
	PSP_UNLOCK
	return (retval);
}

void EAI_GetType (unsigned int uretval,
        char *ctmp, char *dtmp,
        int *ra, int *rb,
        int *rc, int *rd, int *re);

/* interface for getting node type parameters from EAI */
void EAI_GetType(unsigned int nodenum, char *fieldname, char *direction,
	int *nodeptr,
	int *dataoffset,
	int *datalen,
	int *nodetype,
	int *scripttype) {
	int complete;
	
	//printf ("EAI_GetType starting\n");
	PSP_LOCK
	DATA_LOCK
	psp.ptr = (unsigned)direction;
	psp.jparamcount=nodenum;
	psp.fieldname = fieldname;

	psp.comp = &complete;
	psp.type = EAIGETTYPE;
	psp.ofs = (unsigned)NULL;
	psp.path = NULL;
	psp.bind = FALSE; /* should we issue a set_bind? */
	psp.inp = NULL;
	DATA_LOCK_SIGNAL
	DATA_UNLOCK
	while (complete!=1) usleep(10);

	/* copy results out */
	*nodeptr = psp.Etype[0];
	*dataoffset = psp.Etype[1];
	*datalen = psp.Etype[2];
	*nodetype = psp.Etype[3];
	*scripttype = psp.Etype[4];
	//printf("EAI_GetType: %d %d %d %c %d\n",*nodeptr,*dataoffset,*datalen,*nodetype,*scripttype);
	PSP_UNLOCK
}

/* interface for getting node type parameters from EAI - mftype is for MF nodes.*/
char* EAI_GetValue(unsigned int nodenum, char *fieldname, char *nodename) {
	int complete;
	char *retstr;
	
	//printf ("EAI_GetValue starting node %d field %s\n",nodenum,fieldname);
	PSP_LOCK
	DATA_LOCK
	psp.ptr = (unsigned)nodename;
	psp.jparamcount=nodenum;
	psp.fieldname = fieldname;

	psp.comp = &complete;
	psp.type = EAIGETVALUE;
	psp.ofs = (unsigned)NULL;
	psp.path = NULL;
	psp.bind = FALSE; /* should we issue a set_bind? */
	psp.inp = NULL;
	DATA_LOCK_SIGNAL
	DATA_UNLOCK
	while (complete!=1) usleep(10);

	/* copy results out */
	retstr = psp.retstr;
	//printf ("EAI_GetValue finishing, retval = %s\n",retstr);
	PSP_UNLOCK
	return retstr;

}

/* interface for getting node type parameters from EAI */
char* EAI_GetTypeName(unsigned int nodenum) {
	int complete;
	char *retstr;
	
	//printf ("EAI_GetTypeName starting node %d \n",nodenum);
	PSP_LOCK
	DATA_LOCK
	psp.ptr = NULL;
	psp.jparamcount=nodenum;
	psp.fieldname = NULL;

	psp.comp = &complete;
	psp.type = EAIGETTYPENAME;
	psp.ofs = (unsigned)NULL;
	psp.path = NULL;
	psp.bind = FALSE; /* should we issue a set_bind? */
	psp.inp = NULL;
	DATA_LOCK_SIGNAL
	DATA_UNLOCK
	while (complete!=1) usleep(10);

	/* copy results out */
	retstr = psp.retstr;
	//printf ("EAI_GetTypeName finishing, retval = %s\n",retstr);
	PSP_UNLOCK
	return retstr;

}

/* interface for getting a node number via the EAI */
void EAI_Route(char cmnd, char *fn) {
	int complete;
	int retval;

	PSP_LOCK
	DATA_LOCK
	psp.comp = &complete;
	psp.type = EAIROUTE;
	psp.ptr = (unsigned) cmnd;
	psp.ofs = (unsigned)NULL;
	psp.path = NULL;
	psp.bind = FALSE; /* should we issue a set_bind? */
	psp.inp = NULL;
	psp.fieldname = fn;
	DATA_LOCK_SIGNAL
	DATA_UNLOCK
	while (complete!=1) usleep(10);
	retval = psp.jparamcount;
	PSP_UNLOCK
}

/* interface for creating VRML for EAI */
int EAI_CreateVrml(char *tp, char *inputstring, unsigned *retarr, int retarrsize) {
	int complete;
	int retval;
	UNUSED(tp);

	PSP_LOCK
	DATA_LOCK
	if (strncmp(tp,"URL",2) ==  0) {
			psp.type= FROMURL;
	} else {
		psp.type = FROMSTRING;
	}
				
	psp.comp = &complete;
	psp.ptr = (unsigned)NULL;
	psp.ofs = (unsigned)NULL;
	psp.path = NULL;
	psp.bind = FALSE; /* should we issue a set_bind? */
	psp.retarr = retarr;
	psp.retarrsize = retarrsize;
	/* copy over the command */
	psp.inp = malloc (strlen(inputstring)+2);
	if (!(psp.inp)) {printf ("malloc failure in produceTask\n"); exit(1);}
	memcpy (psp.inp,inputstring,strlen(inputstring)+1);
	DATA_LOCK_SIGNAL
	DATA_UNLOCK
	while (complete!=1) {usleep(10);}
	retval = psp.retarrsize;
	PSP_UNLOCK
	return (retval);
}

/* interface for replacing worlds from EAI */
void EAI_readNewWorld(char *inputstring) {
    int complete;
    
    PSP_LOCK
    DATA_LOCK
    psp.comp = &complete;
    psp.type = FROMURL;
    psp.ptr  = rootNode;
    psp.ofs  = offsetof(struct VRML_Group, children);
    psp.path = NULL;
    psp.bind = TRUE; /* should we issue a set_bind? */
    /* copy over the command */
    psp.inp  = malloc (strlen(inputstring)+2);
    if (!(psp.inp)) {printf ("malloc failure in produceTask\n"); exit(1);}
    memcpy (psp.inp,inputstring,strlen(inputstring)+1);
    DATA_LOCK_SIGNAL
    DATA_UNLOCK
    while (complete!=1) usleep(10);
    PSP_UNLOCK
}

/****************************************************************************/
int perlParse(unsigned type, char *inp, int bind, int returnifbusy,
			unsigned ptr, unsigned ofs,int *complete) {

	/* do we want to return if the parsing thread is busy, or do
	   we want to wait? */
	if (returnifbusy) {
		//printf ("perlParse, returnifbusy, PerlParsing %d\n",PerlParsing);
		if (PerlParsing) return (FALSE);
	}
	PSP_LOCK
	DATA_LOCK

	/* copy the data over; malloc and copy input string */
	psp.comp = complete;
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
	PSP_UNLOCK
	return (TRUE);
}
	

void _perlThread(void *perlpath) {
        char *commandline[] = {"", NULL};
	char *builddir;
	int xx;

	FILE *tempfp; /* for tring to locate the fw2init.pl file */

	/* is the browser started yet? */
	if (!browserRunning) {

		commandline[1] = FW2INITPL;

		/* find out where the fw2init.pl file is */
		if ((tempfp = fopen(commandline[1],"r")) != NULL) {
			/* printf ("opened %s %d\n",commandline[1],tempfp); */
			fclose(tempfp);
		} else {
			/* printf ("error opening %s\n",commandline[1]);  */
			xx = strlen (BUILDDIR) + strlen ("./CFrontEnd/fw2init.pl") + 10;
			builddir = malloc (sizeof(char) * xx);
			strcpy (builddir, BUILDDIR);
			strcat (builddir, "/CFrontEnd/fw2init.pl");
			commandline[1] = builddir;

			if ((tempfp = fopen(commandline[1],"r")) != NULL) {
	
				/* printf ("opened %s\n",commandline[1]);  */
				fclose(tempfp);
			} else {
				printf ("can not locate the fw2init.pl file, tried:\n");
				printf ("    %s\n    and\n    %s\nexiting...\n",
				FW2INITPL,builddir);
				exit(1);
			}
		}

		/* initialize stuff for prel interpreter */
		my_perl = perl_alloc();
		perl_construct (my_perl);
		if (perl_parse(my_perl, (XSINIT_t)xs_init, 2, commandline, NULL)) {
			printf ("freewrl can not parse initialization script %s, exiting...\n",
				commandline[1]);
			exit(1);
		}
		/* pass in the compiled perl path */
		/* printf ("sending in path %s\n",perlpath); */
		__pt_setPath(perlpath);

		/* pass in the source directory path in case make install not called */
		/* printf ("sending in path %s\n",BUILDDIR);  */
		__pt_setPath(BUILDDIR);


		/* printf ("opening browser\n"); */
		__pt_openBrowser();

		/* printf ("loading in initial Group{} \n"); */
		__pt_loadInitialGroup();
		browserRunning=TRUE;

		/* Now, possibly this is the first VRML file to
		   add. Check to see if maybe we have a ptr of 0. */
	}

	/* now, loop here forever, waiting for instructions and obeying them */
	for (;;) {
		DATA_LOCK
		PerlInitialized=TRUE; /* have to do this AFTER ensuring we are locked */
		DATA_LOCK_WAIT
		PerlParsing=TRUE;

		/* have to handle these types of commands: 
			FROMSTRING 	create vrml from string
			FROMURL		create vrml from url
			INLINE		convert an inline into code, and load it.
			CALLMETHOD	Javascript... 	
			EAIGETNODE      EAI getNode     
			EAIGETTYPE	EAI getType	
			EAIGETVALUE	EAI getValue - in a string.	
			EAIROUTE	EAI add/delete route
			EAIREPWORLD     EAI replace world */

		if (psp.type == INLINE) {
		/* is this a INLINE? If it is, try to load one of the URLs. */
			__pt_doInline();
		}

		switch (psp.type) {

		case FROMSTRING:
		case FROMURL:	{ 
			/* is this a Create from URL or string, or a successful INLINE? */	
			__pt_doStringUrl();
			break;
			}

		case CALLMETHOD: {
			/* Javascript command???? , do it */
			__pt_doPerlCallMethodVA();
			break;
			}

		case INLINE: {
			/* this should be changed to a FROMURL before here  - check */
			printf ("Inline unsuccessful\n");
			break;
			}

		case EAIGETNODE: {
			/* EAI wants info from a node */
			__pt_EAI_GetNode();
			break;
			}

		case EAIGETTYPE: {
			/* EAI wants type for a node */
			__pt_EAI_GetType();
			break;
			}
		case EAIGETVALUE: {
			/* EAI wants type for a node */
			__pt_EAI_GetValue();
			break;
			}
		case EAIGETTYPENAME: {
			/* EAI wants type for a node */
			__pt_EAI_GetTypeName();
			break;
			}
		case EAIROUTE: {
			/* EAI wants type for a node */
			__pt_EAI_Route();
			break;
			}

//JAS		case EAIREPWORLD: {
//JAS			/* EAI sending in a new world */
//JAS			__pt_EAI_replaceWorld();
//JAS			break;
//JAS			}

		default: {
			printf ("produceTask - invalid type!\n");
			}
		}

		/* finished this loop, free data */
		if (psp.inp) free (psp.inp);
		if (psp.path) free (psp.path);

		*psp.comp = 1;
		URLLoaded=TRUE;
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
	//printf ("addToNode, adding %d to %d\n",newNode,rc);

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
	tmp = (void *)par->p;
	par->p = (void *)newmal;
	par->n = oldlen+newlen;
	free (tmp);
}

/* get all of the bindables from the Perl side. */
void getAllBindables() {
	int aretarr[1000];
	int bretarr[1000];
	int cretarr[1000];
	int dretarr[1000];

	/* first, free any previous nodes */
	if (fognodes) free (fognodes);
	if (backgroundnodes) free (backgroundnodes);
	if (navnodes) free (navnodes);
	if (viewpointnodes) free (viewpointnodes);

	/* now, get the values */
	totviewpointnodes = __pt_getBindables("Viewpoint",aretarr);
	totfognodes = __pt_getBindables("Fog",bretarr);
	totnavnodes = __pt_getBindables("NavigationInfo",cretarr);
	totbacknodes = __pt_getBindables("Background",dretarr);

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

/*****************************************************************************
 *
 * Call Perl Routines. This has to happen from the "Perl" thread, otherwise
 * a segfault happens.
 *
 * See perldoc perlapi, perlcall, perlembed, perlguts for how this all
 * works.
 * 
 *****************************************************************************/

/****************************************************************************
 *
 * General load/create routines
 *
 ****************************************************************************/




/*************************NORMAL ROUTINES***************************/

/* Create VRML/X3D, returning an array of nodes */
unsigned int _pt_CreateVrml (char *tp, char *inputstring, unsigned int *retarr) {
	int count;
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



unsigned int __pt_getBindables (char *tp, unsigned int *retarr) {
	int count;
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
/* pass in the compiled path to the perl interpreter */
void __pt_setPath(char *perlpath) {
	dSP;
	ENTER;
	SAVETMPS;
	PUSHMARK(SP);
	XPUSHs(sv_2mortal(newSVpv((char *)perlpath, strlen ((char *)perlpath))));
	PUTBACK;
	call_pv("setINCPath", G_ARRAY);
	FREETMPS;
	LEAVE;
}

void __pt_loadInitialGroup() {
	dSP;
	PUSHMARK(SP);
	call_pv("load_file_intro", G_ARRAY);
}

/* Shutter glasses, stereo mode configure  Mufti@rus*/
float eyedist = 0.06;	
float screendist = 0.8;	

void setEyeDist (char *optArg) {
	sscanf(optArg,"%f",&eyedist);
}

void setScreenDist (char *optArg) {
	sscanf(optArg,"%f",&screendist);
}
/* end of Shutter glasses, stereo mode configure */


void __pt_openBrowser() {
	
	dSP;
	ENTER;
	SAVETMPS;

	set_viewer_type(1);

	set_eyehalf( eyedist/2.0,
		atan2(eyedist/2.0,screendist)*360.0/(2.0*3.1415926));

#ifndef AQUA
	if (shutter) 
		XEventStereo();
#endif

	
	PUSHMARK(SP);
	XPUSHs(sv_2mortal(newSViv(1000))); // left in as an example
	XPUSHs(sv_2mortal(newSViv(2000)));
	PUTBACK;
	call_pv("open_browser", G_DISCARD);
	FREETMPS;
	LEAVE;
}

/* handle an INLINE - should make it into a CreateVRMLfromURL type command */
void __pt_doInline() {
	int count;
	char *filename;
	struct Multi_String *inurl;
	struct VRML_Inline *inl;
	int xx;
	char *thisurl;
	char *slashindex;
	char firstBytes[4];
	inl = (struct VRML_Inline *)psp.ptr;
	inurl = &(inl->url);
	filename = malloc(1000);

	/* lets make up the path and save it, and make it the global path */
	count = strlen(SvPV(inl->__parenturl,xx));
	psp.path = malloc ((unsigned)(count+1));

	if ((!filename) || (!psp.path)) {
		printf ("perl thread can not malloc for filename\n");
		exit(1);
	}
	
	/* copy the parent path over */
	strcpy (psp.path,SvPV(inl->__parenturl,xx));

	/* and strip off the file name, leaving any path */
	slashindex = (char *) rindex(psp.path, ((int) '/'));
	if (slashindex != NULL) { 
		slashindex ++; /* leave the slash there */
		*slashindex = 0;
	} else {psp.path[0] = 0;}
	//printf ("doInLine, parenturl is %s\n",psp.path);

	/* try the first url, up to the last, until we find a valid one */
	count = 0;
	while (count < inurl->n) {
		thisurl = SvPV(inurl->p[count],xx);

		/* check to make sure we don't overflow */
		if ((strlen(thisurl)+strlen(psp.path)) > 900) break;

		/* we work in absolute filenames... */
		makeAbsoluteFileName(filename,psp.path,thisurl);

		if (fileExists(filename,firstBytes)) {
			break;
		}
		count ++;
	}
	psp.inp = filename; /* will be freed later */
	//printf ("doinline, psp.inp = %s\n",psp.inp);
	/* printf ("inlining %s\n",filename); */

	/* were we successful at locating one of these? if so,
	   make it into a FROMURL */
	if (count != inurl->n) {
		/* printf ("we were successful at locating %s\n",filename); */
		psp.type=FROMURL;
	} else {
		if (count > 0) printf ("Could not locate url (last choice was %s)\n",filename);
	}
}

/* this is a CreateVrmlFrom URL or STRING command */
void __pt_doStringUrl () {
	int count;
	int retval;
	int myretarr[2000];

	if (psp.type==FROMSTRING) {
       		retval = _pt_CreateVrml("String",psp.inp,myretarr);
		
	} else {
		retval = _pt_CreateVrml("URL",psp.inp,myretarr);
	} 

	/* copy the returned nodes to the caller */
	if (psp.retarr != NULL) {
		 /* printf ("returning to EAI caller, psp.retarr = %d, count %d\n",
			psp.retarr, retval);  */
		for (count = 0; count < retval; count ++) {
			/* printf ("	...saving %d in %d\n",myretarr[count],count); */
			psp.retarr[count] = myretarr[count];
		}
		psp.retarrsize = retval;	
	}	
	
	/* get the Bindables from this latest VRML/X3D file */
	if (retval > 0) getAllBindables();

	/* send a set_bind to any nodes that exist */
	if (psp.bind) {
		if (totfognodes != 0) send_bind_to (FOG,(void *)(fognodes[0]),1);
		if (totbacknodes != 0) send_bind_to (BACKGROUND,(void *)(backgroundnodes[0]),1);
		if (totnavnodes != 0) send_bind_to (NAVIGATIONINFO,(void *)(navnodes[0]),1);
		if (totviewpointnodes != 0) send_bind_to(VIEWPOINT,(void *)(viewpointnodes[0]),1);
	}

       	/* now that we have the VRML/X3D file, load it into the scene.
       	   myretarr contains node number/memory location pairs; thus the count
       	   by two. */
	if (psp.ptr != (unsigned int)NULL) {
		/* if we have a valid node to load this into, do it */
		/* note that EAI CreateVRML type commands will NOT give */
		/* a valid node */

	       	for (count =1; count < retval; count+=2) {
			/* add this child to the node */
       			addToNode(psp.ptr+psp.ofs, (unsigned)(myretarr[count]));

			/* tell the child that it has a new parent! */
			add_parent(myretarr[count],psp.ptr);
       		}

		/* tell the node that we have changed */
		update_node((void *)psp.ptr);
	}
}


/************************END OF NORMAL ROUTINES*********************/

/*************************JAVASCRIPT*********************************/
void
__pt_doPerlCallMethodVA() {
	int count = 0;

	dSP;
	ENTER;
	SAVETMPS;
	PUSHMARK(SP);
	XPUSHs(psp.sv);

	for (count = 0; count < psp.jparamcount; count++) {
        /* for javascript items, for Ayla's generic doPerlCallMethodVA call */
		switch (psp.Jtype[count]) {
		case 's':
			XPUSHs(sv_2mortal(newSVpv((char *)psp.Jptr[count], strlen((char *)psp.Jptr[count]))));
			break;
		case 'p':
			XPUSHs(sv_2mortal(newSViv((IV) (void *)psp.Jptr[count])));
			break;
		default:
			break;
		}
	}

	PUTBACK;
	count = call_method(psp.fieldname, G_SCALAR);

	SPAGAIN;
	

if (count > 1) {
	fprintf(stderr,
		"__pt_doPerlCallMethodgVA: call_method returned in list context - shouldnt happen here!\n");
	}

	PUTBACK;
	FREETMPS;
	LEAVE;
}

/*************************END OF JAVASCRIPT*********************************/


/****************************** EAI ****************************************/

/* get node info, send in a character string, get a node reference number */
void __pt_EAI_GetNode () {
	int count;

	dSP;
	ENTER;
	SAVETMPS;
	PUSHMARK(SP);
	//this is for integers XPUSHs(sv_2mortal(newSViv(nname)));
	XPUSHs(sv_2mortal(newSVpv(psp.fieldname, 0)));


	PUTBACK;
	count = call_pv("VRML::Browser::EAI_GetNode", G_SCALAR);
	SPAGAIN ;

	if (count != 1)
		printf ("EAI_getNode, node returns %d\n",count);

	/* return value in psp.jparamcount */
	psp.jparamcount = POPi;

	/* 
		printf ("The node is %x\n", noderef) ;
	*/

	PUTBACK;
	FREETMPS;
	LEAVE;
}


/* set/delete route */
void __pt_EAI_Route () {
	dSP;
	ENTER;
	SAVETMPS;
	PUSHMARK(SP);
	XPUSHs(sv_2mortal(newSViv(psp.ptr)));
	XPUSHs(sv_2mortal(newSVpv(psp.fieldname, 0)));
	PUTBACK;
	call_pv("VRML::Browser::EAI_Route", G_SCALAR);
	SPAGAIN ;
	PUTBACK;
	FREETMPS;
	LEAVE;
}

void __pt_EAI_GetType (){
	unsigned int 	count;

	dSP;
	ENTER;
	SAVETMPS;
	PUSHMARK(SP);

	/* push on the nodenum, fieldname and direction */
	XPUSHs(sv_2mortal(newSViv(psp.jparamcount)));
	XPUSHs(sv_2mortal(newSVpv(psp.fieldname, 0)));
	XPUSHs(sv_2mortal(newSVpv((const char *)psp.ptr, (STRLEN)0)));

	PUTBACK;
	count = call_pv("VRML::Browser::EAI_GetType",G_ARRAY);
	SPAGAIN;

	if (count != 5) {
		/* invalid return values; make *nodeptr = 97, the rest 0 */
		psp.Etype[0]=97;	/* SFUNKNOWN - check CFuncs/EAIServ.c */
		psp.Etype[4] = 0;
		psp.Etype[3] = 0;
		psp.Etype[2] = 0;
		psp.Etype[1] = 0;
	} else {
		/* pop values off stack in reverse of perl return order */
		psp.Etype[4] = POPi; psp.Etype[3] = POPi; psp.Etype[2] = POPi; 
		psp.Etype[1] = POPi; psp.Etype[0] = POPi;
	}

	PUTBACK;
	FREETMPS;
	LEAVE;
}

void __pt_EAI_GetValue (){
	unsigned int 	count;
	int len;
	char *ctmp;

	SV * retval;
	dSP;
	ENTER;
	SAVETMPS;
	PUSHMARK(SP);

	/* push on the nodenum, fieldname and direction */
	XPUSHs(sv_2mortal(newSViv(psp.jparamcount)));
	XPUSHs(sv_2mortal(newSVpv(psp.fieldname, 0)));
	// this was for pushing on an integer... XPUSHs(sv_2mortal(newSViv(psp.inp)));

	PUTBACK;
	count = call_pv("VRML::Browser::EAI_GetValue",G_EVAL|G_SCALAR);
	SPAGAIN;

	//printf ("GetValue return; count %d\n",count);
	if (count != 1) {
		psp.sv=NULL;	
	} else {
		/* pop values off stack in reverse of perl return order */
		retval = POPs;
	} 

	PUTBACK;
	//printf ("EAI_GetValue retval %d\n", retval) ;
                                                                                    
	//if (SvOK(retval)) {printf ("retval is an SV\n"); }
	//else {printf ("retval is NOT an SV\n"); return;}
	/* now, decode this SV */
	//printf ("SVtype is %x\n",SvTYPE(retval));
	//printf ("String is :%s: len %d \n",SvPV(retval,len),len);
	
	/* make a copy of the return string - caller has to free it after use */
	ctmp = SvPV(retval, len); // now, we have the length
	psp.retstr = malloc (sizeof (char) * (len+5));
	strcpy (psp.retstr,ctmp);
	//printf ("GetValue, retstr will be :%s:\n",psp.retstr);

	FREETMPS;
	LEAVE;
}


void __pt_EAI_GetTypeName (){
	unsigned int 	count;
	int len;

	SV * retval;
	char *ctmp;
	dSP;
	ENTER;
	SAVETMPS;
	PUSHMARK(SP);

	/* push on the nodenum */
	XPUSHs(sv_2mortal(newSViv(psp.jparamcount)));

	PUTBACK;
	count = call_pv("VRML::Browser::EAI_GetTypeName",G_EVAL|G_SCALAR);
	SPAGAIN;

	//printf ("GetTypeName return; count %d\n",count);
	if (count != 1) {
		psp.sv=NULL;	
	} else {
		/* pop values off stack in reverse of perl return order */
		retval = POPs;
	} 

	PUTBACK;
                                                                                    
	/* make a copy of the return string - caller has to free it after use */
	ctmp = SvPV(retval, len); // now, we have the length
	psp.retstr = malloc (sizeof (char) * (len+5));
	strcpy (psp.retstr,ctmp);
	//printf ("GetTypeName, retstr will be :%s:\n",psp.retstr);

	FREETMPS;
	LEAVE;
}

//JASvoid __pt_EAI_replaceWorld () {
//JAS	int count;
//JAS
//JAS	printf ("ProdCon, _pt_EAI_replaceWorld, fieldname %s\n",psp.fieldname);
//JAS//	dSP;
//JAS//	ENTER;
//JAS//	SAVETMPS;
//JAS//	PUSHMARK(SP);
//JAS//	XPUSHs(sv_2mortal(newSVpv(psp.fieldname, 0)));
//JAS//	PUTBACK;
//JAS//		count = call_pv("VRML::Browser::EAI_replaceWorld", G_ARRAY);
//JAS//	SPAGAIN ;
//JAS//	PUTBACK;
//JAS//	FREETMPS;
//JAS//	LEAVE;
//JAS}

/****************************** END OF EAI **********************************/
