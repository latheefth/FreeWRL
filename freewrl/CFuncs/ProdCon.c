/*******************************************************************
 Copyright (C) 2003 John Stewart, CRC Canada.
 DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
 See the GNU Library General Public License (file COPYING in the distribution)
 for conditions of use and redistribution.
*********************************************************************/

#include <stdio.h>
#include "headers.h"
#include "installdir.h"
#include "PluginSocket.h"
#include "Bindable.h"
#include "CParse.h"
#include <pthread.h>

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

int _fw_browser_plugin = 0;
int _fw_pipe = 0;
uintptr_t _fw_instance = 0;

/* do we use the experimental parser for our work? */
int useExperimentalParser = TRUE;

/* for keeping track of current url */
char *currentWorkingUrl = NULL;

int _P_LOCK_VAR;

/* thread synchronization issues */
#define PERL_LOCKING_INIT _P_LOCK_VAR = 0
#define SEND_TO_PERL if (_P_LOCK_VAR==0) _P_LOCK_VAR=1; else printf ("SEND_TO_PERL = flag wrong!\n");
#define PERL_FINISHING if (_P_LOCK_VAR==1) _P_LOCK_VAR=0; else printf ("PERL_FINISHING - flag wrong!\n");

#define UNLOCK pthread_cond_signal(&condition); pthread_mutex_unlock(&mutex);

#define WAIT_WHILE_PERL_BUSY  pthread_mutex_lock(&mutex); \
     while (_P_LOCK_VAR==1) { pthread_cond_wait(&condition, &mutex);}

#define WAIT_WHILE_NO_DATA pthread_mutex_lock(&mutex); \
     while (_P_LOCK_VAR==0) { pthread_cond_wait(&condition, &mutex);}

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condition = PTHREAD_COND_INITIALIZER;


struct PSStruct {
	unsigned type;		/* what is this task? 			*/
	char *inp;		/* data for task (eg, vrml text)	*/
	void *ptr;		/* address (node) to put data		*/
	unsigned ofs;		/* offset in node for data		*/
	int zeroBind;		/* should we dispose Bindables in Perl?	*/
	int bind;		/* should we issue a bind? 		*/
	char *path;		/* path of parent URL			*/
	int *comp;		/* pointer to complete flag		*/

	char *fieldname;	/* pointer to a static field name	*/
	int jparamcount;	/* number of parameters for this one	*/
	SV *sv;			/* the SV for javascript		*/

	/* for EAI */
	uintptr_t *retarr;		/* the place to put nodes		*/
	int retarrsize;		/* size of array pointed to by retarr	*/
	unsigned Etype[10];	/* EAI return values			*/

	/* for class - return a string */
	char *retstr;
};



void _inputParseThread (void *perlpath);
void __pt_setPath(char *perlpath);
void __pt_openBrowser(void);
void __pt_zeroDEFS(void);
unsigned int _pt_CreateVrml (char *tp, char *inputstring, uintptr_t *retarr);
int isInputThreadInitialized(void);
int inputParse(unsigned type, char *inp, int bind, int returnifbusy,
			void *ptr, unsigned ofs, int *complete,
			int zeroBind);
void __pt_doInline(void);
void __pt_doStringUrl (void);
void EAI_readNewWorld(char *inputstring);

/* Bindables */
void* *fognodes = NULL;
void* *backgroundnodes = NULL;
void* *navnodes = NULL;
void* *viewpointnodes = NULL;
int totfognodes = 0;
int totbacknodes = 0;
int totnavnodes = 0;
int totviewpointnodes = 0;
int currboundvpno=0;

/* keep track of the producer thread made */
pthread_t PCthread;

/* is the Browser initialized? */
static int browserRunning=FALSE;

/* is the inputParse thread created? */
int inputParseInitialized=FALSE;

/* is the parsing thread active? this is read-only, used as a "flag" by other tasks */
int inputThreadParsing=FALSE;

/* Initial URL loaded yet? - Robert Sim */
int URLLoaded=FALSE;

/* the actual perl interpreter */
PerlInterpreter *my_perl;

/* psp is the data structure that holds parameters for the parsing thread */
struct PSStruct psp;

char *myPerlInstallDir;

void initializeInputParseThread(const char *perlpath) {
	int iret;

	myPerlInstallDir = strdup(perlpath);

	/* create consumer thread and set the "read only" flag indicating this */
	iret = pthread_create(&PCthread, NULL, (void *(*)(void *))&_inputParseThread, (void *) perlpath);
}

/* is Perl running? this is a function, because if we need to mutex lock, we
   can do all locking in this file */
int isInputThreadInitialized() {return inputParseInitialized;}

/* statusbar uses this to tell user that we are still loading */
int isinputThreadParsing() {return(inputThreadParsing);}

/* is the initial URL loaded? Robert Sim */
int isURLLoaded() {return(URLLoaded&&!inputThreadParsing);}

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

/* parameter "GetIt" used as FALSE in Anchor */
int fileExists(char *fname, char *firstBytes, int GetIt) {
	FILE *fp;
	int ok;
	char *retName;

	char tempname[1000];
	char sysline[1000];

	/* printf ("checking for filename here %s\n",fname);  */

	/* are we running under netscape? if so, ask the browser, and
	   save the name it returns (cache entry) */
	if (RUNNINGASPLUGIN && (strcmp(BrowserFullPath,fname)!=0)) {
		retName = requestUrlfromPlugin(_fw_browser_plugin, _fw_instance, fname);

		/* check for timeout; if not found, return false */
		if (!retName) return (FALSE);
		strcpy (fname,retName);
	}

	/* if not, do we need to invoke lwp to get the file, or
	   is it just local? if we are running as a plugin, this should
	   be a local file by now
	 */
	if (checkNetworkFile(fname)) {
		/*  Is this an Anchor? if so, lets just assume we can*/
		/*  get it*/
		if (!GetIt) {
			/* printf ("Assuming Anchor mode, returning TRUE\n");*/
			return (TRUE);
		}

		sprintf (tempname, "%s",tempnam("/tmp","freewrl_tmp"));

		/* string length checking */
		if ((strlen(WGET)+strlen(fname)+strlen(tempname)) < (1000-10)) {
#ifdef AQUA
		    sprintf (sysline,"%s %s -o %s",WGET,fname,tempname);
#else
		    sprintf (sysline,"%s %s -O %s",WGET,fname,tempname);
#endif
		    /*printf ("\nFreeWRL will try to use wget to get %s in thread %d\n",fname,pthread_self());*/
		    printf ("\nFreeWRL will try to use wget to get %s\n",fname);
		    freewrlSystem (sysline);
		    strcpy (fname,tempname);
		} else {
		    printf ("Internal FreeWRL problem - strings too long for wget\n");
		    strcat (fname,"");
		}
	}

	/* printf ("opening file %s\n",fname); */


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
	/* printf ("makeAbs from:\n\t:%s:\n\t:%s:\n", pspath, thisurl); */

	/* if we are running under a browser, let it handle things */
	if (RUNNINGASPLUGIN) {
		strcpy (filename, thisurl);
		return;
	}

	/* does this name start off with a ftp, http, or a "/"? */
	if ((!checkNetworkFile(thisurl)) && (strncmp(thisurl,"/",strlen("/"))!=0)) {
		/* printf ("copying psppath over for %s\n",thisurl);*/
		strcpy (filename,pspath);
		/* do we actually have anything here? */
		if (strlen(pspath) > 0) {
			if (pspath[strlen(pspath)-1] != '/')
				strcat (filename,"/");
		}

		/* does this "thisurl" start with file:, as in "freewrl file:test.wrl" ? */
		if ((strncmp(thisurl,"file:",strlen("file:"))==0) || 
				(strncmp(thisurl,"FILE:",strlen("FILE:"))==0)) {
			/* printf ("stripping file off of start\n");  */
			thisurl += strlen ("file:");

			/* now, is this a relative or absolute filename? */
			if (strncmp(thisurl,"/",strlen("/")) !=0) {
				/* printf ("we have a leading slash after removing file: from name\n");
				printf ("makeAbsolute, going to copy %s to %s\n",thisurl, filename);  */
				strcat(filename,thisurl);
			
			} else {
				/* printf ("we have no leading slash after removing file: from name\n"); */
				strcpy (filename,thisurl);
			}	
			
		} else {
			/* printf ("makeAbsolute, going to copy %s to %s\n",thisurl, filename); */
			strcat(filename,thisurl);

		}


	} else {
		strcpy (filename,thisurl);
	}

	/* and, return in the ptr filename, the filename created... */
	 /* printf ("makeAbsoluteFileName, just made :%s:\n",filename); */
}


/************************************************************************/
/*									*/
/* keep track of the current url for parsing/textures			*/
/*									*/
/************************************************************************/

void pushInputURL(char *url) {

	FREE_IF_NZ(currentWorkingUrl);
	currentWorkingUrl = strdup(url);
	/* printf ("currenturl is %s\n",currentWorkingUrl); */
}

char *getInputURL() {
	return currentWorkingUrl;
}



/************************************************************************/
/*									*/
/* THE FOLLOWING ROUTINES INTERFACE TO THE PERL THREAD			*/
/*									*/
/************************************************************************/

/* Inlines... Multi_URLs, load only when available, etc, etc */
void loadInline(struct X3D_Inline *node) {
	/* first, are we busy? */
	if (inputThreadParsing) return;

	inputParse(INLINE,(char *)node, FALSE, FALSE,
		(void *) node,
		offsetof (struct X3D_Inline, __children),
		&node->__loadstatus,FALSE);
}

#ifdef USEEAIANDPERL
*unused*	/* interface for getting a node number via the EAI */
*unused*	char *EAI_GetNode(const char *nname) {
*unused*		int complete;
*unused*		char *retval;
*unused*		STRLEN len;
*unused*	
*unused*		WAIT_WHILE_PERL_BUSY;
*unused*		complete=0;
*unused*		psp.comp = &complete;
*unused*		psp.type = EAIGETNODE;
*unused*		psp.retarr = NULL;
*unused*		psp.ptr = (unsigned)NULL;
*unused*		psp.ofs = (unsigned)NULL;
*unused*		psp.path = NULL;
*unused*		psp.zeroBind = FALSE;
*unused*		psp.bind = FALSE; /* should we issue a set_bind? */
*unused*		psp.inp = NULL;
*unused*		psp.fieldname = strdup(nname);
*unused*		/* send data to Perl Interpreter */
*unused*		SEND_TO_PERL;
*unused*		UNLOCK;
*unused*	
*unused*		/* wait for data */
*unused*		WAIT_WHILE_PERL_BUSY;
*unused*		/* grab data */
*unused*		retval = psp.retstr;
*unused*	
*unused*		/* printf ("getNode is returning %s\n",retval); */
*unused*		UNLOCK;
*unused*		return (retval);
*unused*	}
*unused*	
*unused*	/* interface for getting a Viewpoint CNode */
*unused*	unsigned int EAI_GetViewpoint(const char *nname) {
*unused*		int complete;
*unused*		int retval;
*unused*	
*unused*		WAIT_WHILE_PERL_BUSY;
*unused*		complete=0;
*unused*		psp.comp = &complete;
*unused*		psp.type = EAIGETVIEWPOINT;
*unused*		psp.retarr = NULL;
*unused*		psp.ptr = (unsigned)NULL;
*unused*		psp.ofs = (unsigned)NULL;
*unused*		psp.path = NULL;
*unused*		psp.zeroBind = FALSE;
*unused*		psp.bind = FALSE; /* should we issue a set_bind? */
*unused*		psp.inp = NULL;
*unused*		psp.fieldname = strdup(nname);
*unused*	
*unused*		/* send data to Perl Interpreter */
*unused*		SEND_TO_PERL;
*unused*		UNLOCK;
*unused*	
*unused*		/* wait for data */
*unused*		WAIT_WHILE_PERL_BUSY;
*unused*		/* grab data */
*unused*		retval = psp.jparamcount;
*unused*		UNLOCK;
*unused*		return (retval);
*unused*	}
*unused*	
*unused*	/* interface for getting node type parameters from EAI */
*unused*	void EAI_GetType(unsigned int nodenum, const char *fieldname, const char *direction,
*unused*		int *nodeptr,
*unused*		int *dataoffset,
*unused*		int *datalen,
*unused*		int *nodetype,
*unused*		int *scripttype) {
*unused*		int complete;
*unused*	
*unused*		WAIT_WHILE_PERL_BUSY;
*unused*		complete=0;
*unused*		psp.ptr = strdup(direction);
*unused*		psp.jparamcount=nodenum;
*unused*		psp.fieldname = strdup(fieldname);
*unused*	
*unused*		psp.comp = &complete;
*unused*		psp.type = EAIGETTYPE;
*unused*		psp.retarr = NULL;
*unused*		psp.ofs = (unsigned)NULL;
*unused*		psp.path = NULL;
*unused*		psp.zeroBind = FALSE;
*unused*		psp.bind = FALSE; /* should we issue a set_bind? */
*unused*		psp.inp = NULL;
*unused*	
*unused*		/* send data to Perl Interpreter */
*unused*		SEND_TO_PERL;
*unused*		UNLOCK;
*unused*	
*unused*		/* wait for data */
*unused*		WAIT_WHILE_PERL_BUSY;
*unused*	
*unused*		/* grab data */
*unused*		/* copy results out */
*unused*		*nodeptr = psp.Etype[0];
*unused*		*dataoffset = psp.Etype[1];
*unused*		*datalen = psp.Etype[2];
*unused*		*nodetype = psp.Etype[3];
*unused*		*scripttype = psp.Etype[4];
*unused*		/* printf("EAI_GetType: %d %d %d %c %d\n",*nodeptr,*dataoffset,*datalen,*nodetype,*scripttype); */
*unused*		UNLOCK;
*unused*	}
*unused*	
*unused*	/* interface for getting node type parameters from EAI - mftype is for MF nodes.*/
*unused*	char* EAI_GetValue(unsigned int nodenum, const char *fieldname, const char *nodename) {
*unused*		int complete;
*unused*		char *retstr;
*unused*	
*unused*		/* printf ("EAI_GetValue starting node %d field %s\n",nodenum,fieldname); */
*unused*		WAIT_WHILE_PERL_BUSY;
*unused*		complete=0;
*unused*		psp.ptr = strdup(nodename);
*unused*		psp.jparamcount=nodenum;
*unused*		psp.fieldname = strdup(fieldname);
*unused*	
*unused*		psp.comp = &complete;
*unused*		psp.type = EAIGETVALUE;
*unused*		psp.retarr = NULL;
*unused*		psp.ofs = (unsigned)NULL;
*unused*		psp.path = NULL;
*unused*		psp.zeroBind = FALSE;
*unused*		psp.bind = FALSE; /* should we issue a set_bind? */
*unused*		psp.inp = NULL;
*unused*	
*unused*	
*unused*		/* send data to Perl Interpreter */
*unused*		SEND_TO_PERL;
*unused*		UNLOCK;
*unused*	
*unused*		/* wait for data */
*unused*		WAIT_WHILE_PERL_BUSY;
*unused*	
*unused*	
*unused*	
*unused*		/* grab data */
*unused*		/* copy results out */
*unused*		retstr = psp.retstr;
*unused*		/* printf ("EAI_GetValue finishing, retval = %s\n",retstr); */
*unused*		UNLOCK;
*unused*		return retstr;
*unused*	
*unused*	}
*unused*	
*unused*	/* interface for getting node type parameters from EAI */
*unused*	char* EAI_GetTypeName(unsigned int nodenum) {
*unused*		int complete;
*unused*		char *retstr;
*unused*	
*unused*		/* printf ("EAI_GetTypeName starting node %d \n",nodenum);*/
*unused*		WAIT_WHILE_PERL_BUSY;
*unused*		complete=0;
*unused*		psp.ptr = (unsigned int)NULL;
*unused*		psp.jparamcount=nodenum;
*unused*		psp.fieldname = NULL;
*unused*	
*unused*		psp.comp = &complete;
*unused*		psp.type = EAIGETTYPENAME;
*unused*		psp.retarr = NULL;
*unused*		psp.ofs = (unsigned)NULL;
*unused*		psp.path = NULL;
*unused*		psp.zeroBind = FALSE;
*unused*		psp.bind = FALSE; /* should we issue a set_bind? */
*unused*		psp.inp = NULL;
*unused*	
*unused*	
*unused*		/* send data to Perl Interpreter */
*unused*		SEND_TO_PERL;
*unused*		UNLOCK;
*unused*	
*unused*		/* wait for data */
*unused*		WAIT_WHILE_PERL_BUSY;
*unused*	
*unused*		/* grab data */
*unused*		/* copy results out */
*unused*		retstr = psp.retstr;
*unused*		/* printf ("EAI_GetTypeName finishing, retval = %s\n",retstr);*/
*unused*		UNLOCK;
*unused*		return retstr;
*unused*	
*unused*	}
*unused*	
*unused*	/* interface for sending/getting simple commands to the EAI/SAI. eg, updateNamedNode */
*unused*	int SAI_IntRetCommand (char cmnd, const char *fn) {
*unused*		int complete;
*unused*		int retval;
*unused*	
*unused*		WAIT_WHILE_PERL_BUSY;
*unused*		complete=0;
*unused*		psp.comp = &complete;
*unused*		psp.type = SAICOMMAND;
*unused*		psp.retarr = NULL;
*unused*		psp.ofs = (unsigned) cmnd;
*unused*		psp.ptr = NULL; /* null indicates that we want an integer returned here */
*unused*		psp.path = NULL;
*unused*		psp.zeroBind = FALSE;
*unused*		psp.bind = FALSE; /* should we issue a set_bind? */
*unused*		psp.inp = NULL;
*unused*		psp.fieldname = strdup(fn);
*unused*	
*unused*		/* send data to Perl Interpreter */
*unused*		SEND_TO_PERL;
*unused*		UNLOCK;
*unused*	
*unused*		/* wait for data */
*unused*		WAIT_WHILE_PERL_BUSY;
*unused*	
*unused*		/* grab data */
*unused*		retval = psp.jparamcount;
*unused*		UNLOCK;
*unused*		return retval;
*unused*	}
*unused*	
*unused*	
*unused*	/* interface for sending/getting simple commands to the EAI/SAI. eg, updateNamedNode */
*unused*	char *SAI_StrRetCommand (char cmnd, const char *fn) {
*unused*		int complete;
*unused*		char *retstr;
*unused*	
*unused*		WAIT_WHILE_PERL_BUSY;
*unused*		complete=0;
*unused*		psp.comp = &complete;
*unused*		psp.type = SAICOMMAND;
*unused*		psp.retarr = NULL;
*unused*		psp.ofs = (unsigned) cmnd;
*unused*		psp.ptr = (void *)1; 	/* 1 signifies that we want an SV returned here */
*unused*		psp.path = NULL;
*unused*		psp.zeroBind = FALSE;
*unused*		psp.bind = FALSE; /* should we issue a set_bind? */
*unused*		psp.inp = NULL;
*unused*		psp.fieldname = strdup(fn);
*unused*	
*unused*		/* send data to Perl Interpreter */
*unused*		SEND_TO_PERL;
*unused*		UNLOCK;
*unused*	
*unused*		/* wait for data */
*unused*		WAIT_WHILE_PERL_BUSY;
*unused*	
*unused*		/* grab data */
*unused*		retstr = psp.retstr;
*unused*		/* printf ("SAI_SVRetCommand, returning %s\n",retstr); */
*unused*		UNLOCK;
*unused*		return retstr;
*unused*	}
*unused*	
#endif

/* interface for telling the Perl side to forget about everything...  */
void EAI_killBindables (void) {
	int complete;

	WAIT_WHILE_PERL_BUSY;
	complete=0;
	psp.comp = &complete;
	psp.type = ZEROBINDABLES;
	psp.retarr = NULL;
	psp.ofs = 0;
	psp.ptr = NULL;
	psp.path = NULL;
	psp.zeroBind = FALSE;
	psp.bind = FALSE; /* should we issue a set_bind? */
	psp.inp = NULL;
	psp.fieldname = NULL;

	/* send data to Perl Interpreter */
	SEND_TO_PERL;
	UNLOCK;

	/* wait for data */
	WAIT_WHILE_PERL_BUSY;

	/* grab data */
	UNLOCK;

	/* and, reset our stack pointers */
	background_tos = -1;
	fog_tos = -1;
	navi_tos = -1;
	viewpoint_tos = -1;
}

/* interface for creating VRML for EAI */
int EAI_CreateVrml(const char *tp, const char *inputstring, uintptr_t *retarr, int retarrsize) {
	int complete;
	int retval;
	UNUSED(tp);

	/* tell the SAI that this is a VRML file, in case it cares later on (check SAI spec) */
	currentFileVersion = 3;

	WAIT_WHILE_PERL_BUSY;

	if (strncmp(tp,"URL",2) ==  0) {
			psp.type= FROMURL;
	} else if (strncmp(tp,"String",5) == 0) {
		psp.type = FROMSTRING;
	} else if (strncmp(tp,"CREATEPROTO",10) == 0) {
		psp.type = FROMCREATEPROTO;
	} else if (strncmp(tp,"CREATENODE",10) == 0) {
		psp.type = FROMCREATENODE;
	} else {
		printf ("EAI_CreateVrml - invalid input %s\n",tp);
		return 0;
	}
	 
	complete = 0; /* make sure we wait for completion */
	psp.comp = &complete;
	psp.ptr = (unsigned)NULL;
	psp.ofs = (unsigned)NULL;
	psp.path = NULL;
	psp.zeroBind = FALSE;
	psp.bind = FALSE; /* should we issue a set_bind? */
	psp.retarr = retarr;
	psp.retarrsize = retarrsize;
	/* copy over the command */
	psp.inp = (char *)malloc (strlen(inputstring)+2);
	if (!(psp.inp)) {outOfMemory ("malloc failure in produceTask\n");}
	memcpy (psp.inp,inputstring,strlen(inputstring)+1);

	/* send data to Perl Interpreter */
	SEND_TO_PERL;
	UNLOCK;

	/* wait for data */
	WAIT_WHILE_PERL_BUSY;

	/* grab data */
	retval = psp.retarrsize;
	UNLOCK;
	return (retval);
}

/* interface for replacing worlds from EAI */
void EAI_readNewWorld(char *inputstring) {
    int complete;

	WAIT_WHILE_PERL_BUSY;
    complete=0;
    psp.comp = &complete;
    psp.type = FROMURL;
	psp.retarr = NULL;
    psp.ptr  = rootNode;
    psp.ofs  = offsetof(struct X3D_Group, children);
    psp.path = NULL;
    psp.zeroBind = FALSE;
    psp.bind = TRUE; /* should we issue a set_bind? */
    /* copy over the command */
    psp.inp  = (char *)malloc (strlen(inputstring)+2);
    if (!(psp.inp)) {outOfMemory ("malloc failure in produceTask\n"); }
    memcpy (psp.inp,inputstring,strlen(inputstring)+1);

	/* send data to Perl Interpreter */
	SEND_TO_PERL;
	UNLOCK;

	/* wait for data */
	WAIT_WHILE_PERL_BUSY;
	/* grab data */
	UNLOCK;
}

/****************************************************************************/
int inputParse(unsigned type, char *inp, int bind, int returnifbusy,
			void *ptr, unsigned ofs,int *complete,
			int zeroBind) {

	/* do we want to return if the parsing thread is busy, or do
	   we want to wait? */
	/* printf ("start of PerlParse, thread %d\n",pthread_self()); */
	if (returnifbusy) {
		/* printf ("inputParse, returnifbusy, inputThreadParsing %d\n",inputThreadParsing);*/
		if (inputThreadParsing) return (FALSE);
	}

	WAIT_WHILE_PERL_BUSY;

	/* printf ("inputParse, past WAIT_WHILE_PERL_BUSY in %d\n",pthread_self()); */

	/* copy the data over; malloc and copy input string */
	psp.comp = complete;
	psp.type = type;
	psp.retarr = NULL;
	psp.ptr = ptr;
	psp.ofs = ofs;
	psp.path = NULL;
	psp.bind = bind; /* should we issue a set_bind? */
	psp.zeroBind = zeroBind; /* should we zero bindables? */

	psp.inp = (char *)malloc (strlen(inp)+2);

	if (!(psp.inp)) {outOfMemory ("malloc failure in produceTask\n");}
	memcpy (psp.inp,inp,strlen(inp)+1);

	/* send data to Perl Interpreter */
	SEND_TO_PERL;
	UNLOCK;

	/* printf ("inputParse, waiting for data \n"); */

	/* wait for data */
	WAIT_WHILE_PERL_BUSY;
	/* grab data */
	UNLOCK;

	return (TRUE);
}

/***********************************************************************************/

void _inputParseThread(void *perlpath) {
        char *commandline[] = {"", NULL};
	char *builddir;
	char *installdir;
	int xx;
	#define FW2A "/VRML/fw2init.pl"
	#define FW2B "/CFrontEnd/fw2init.pl"

	FILE *tempfp; /* for tring to locate the fw2init.pl file */

	/* printf ("inputParseThread is %d\n",pthread_self()); */

	PERL_LOCKING_INIT;

	if (!useExperimentalParser) {
		/* is the browser started yet? */
		if (!browserRunning) {
			/* find out if this FreeWRL is installed yet */
			xx = strlen(INSTALLDIR) + strlen (FW2A) + 10;
			installdir = (char *)malloc (sizeof(char) * xx);
			strcpy (installdir,INSTALLDIR);
			strcat (installdir,FW2A);
			commandline[1] = installdir;
	
			/* find out where the fw2init.pl file is */
			if ((tempfp = fopen(commandline[1],"r")) != NULL) {
				/* printf ("opened %s %d\n",commandline[1],tempfp); */
				fclose(tempfp);
			} else {
				/* printf ("error opening %s\n",commandline[1]); */
				xx = strlen (BUILDDIR) + strlen (FW2B) + 10;
				builddir = (char *)malloc (sizeof(char) * xx);
				strcpy (builddir, BUILDDIR);
				strcat (builddir, FW2B);
				commandline[1] = builddir;
	
				if ((tempfp = fopen(commandline[1],"r")) != NULL) {
	
					printf ("FreeWRL not installed; opened %s\n",commandline[1]); 
					fclose(tempfp);
				} else {
					ConsoleMessage ("can not locate the fw2init.pl file, tried: " \
					"    %s\n    and\n    %s\nexiting...\n",
					installdir,builddir);
					exit(1);
				}
			}
	
			/* initialize stuff for prel interpreter */
			my_perl = perl_alloc();
			perl_construct (my_perl);
			if (perl_parse(my_perl, (XSINIT_t)xs_init, 2, commandline, NULL)) {
				ConsoleMessage("freewrl can not parse initialization script %s, exiting...\n",
					commandline[1]);
				exit(1);
			}
			/* pass in the compiled perl path */
			/* printf ("sending in path %s\n",perlpath); */
			__pt_setPath((char *)perlpath);
	
			/* pass in the source directory path in case make install not called */
			/* printf ("sending in path %s\n",BUILDDIR); */
			__pt_setPath(BUILDDIR);
	
	
			/* printf ("opening browser\n"); */
			__pt_openBrowser();
	
			browserRunning=TRUE;
	
			/* Now, possibly this is the first VRML file to
			   add. Check to see if maybe we have a ptr of 0. */
	
			inputParseInitialized=TRUE;  /* have to do this AFTER ensuring we are locked */
		}
	} else {

		inputParseInitialized = TRUE;
	}
	viewer_default();

	/* now, loop here forever, waiting for instructions and obeying them */
	for (;;) {
		/* printf ("thread %d waiting for data\n",pthread_self()); */
		WAIT_WHILE_NO_DATA;

		inputThreadParsing=TRUE;

		/* have to handle these types of commands:
			FROMSTRING 	create vrml from string
			FROMURL		create vrml from url
			INLINE		convert an inline into code, and load it.
			CALLMETHOD	Javascript...
			EAIGETNODE      EAI getNode
			EAIGETVIEWPOINT get a Viewpoint CNode
			EAIGETTYPE	EAI getType
			EAIGETVALUE	EAI getValue - in a string.
			EAIROUTE	EAI add/delete route
			ZEROBINDABLES	tell the front end to just forget about DEFS, etc 
			SAICOMMAND	general command, with string param to perl returns an int */

		if (psp.type == INLINE) {
		/* is this a INLINE? If it is, try to load one of the URLs. */
			__pt_doInline();
		}

		switch (psp.type) {

		case FROMSTRING:
		case FROMCREATENODE:
		case FROMCREATEPROTO:
		case FROMURL:	{
			/* is this a Create from URL or string, or a successful INLINE? */
			__pt_doStringUrl();
			break;
			}

		case INLINE: {
			/* this should be changed to a FROMURL before here  - check */
			printf ("Inline unsuccessful\n");
			break;
			}

		case ZEROBINDABLES: 
			if (useExperimentalParser) {
				destroyCParserData();
			} else {
				__pt_zeroDEFS(); 
			}
			break;

		default: {
			printf ("produceTask - invalid type!\n");
			}
		}

		/* finished this loop, free data */
		if (psp.inp) free (psp.inp);
		if (psp.path) free (psp.path);

		*psp.comp = 1;
		URLLoaded=TRUE;
		inputThreadParsing=FALSE;
		PERL_FINISHING;
		UNLOCK;
	}
}

/*  add a node to the root group. ASSUMES ROOT IS A GROUP NODE! (it should be)*/
/*  this code is very similar to getMFNode in CFuncs/CRoutes.c, except that*/
/*  we do not pass in a string of nodes to assign. (and, do not remove, etc)*/
void addToNode (void *rc, int offs, void *newNode) {

	int oldlen, newlen;
	void **newmal;
	void **place;
	struct Multi_Node *par;
	void **tmp;

	char *tmpptr;

	tmpptr = (char *)rc;
	tmpptr = tmpptr + offs;

	par = (struct Multi_Node *) tmpptr;
	/* printf ("addToNode, adding %d to %d offset %d\n",newNode,rc,offs); */

	/* oldlen = what was there in the first place */
	oldlen = par->n;
	/* printf ("addToNode, ptr %d offs %d type %s, oldlen %d\n",rc, offs, stringNodeType(((struct X3D_Box*)rc)->_nodeType),oldlen);  */
	par->n = 0; /* temporary, in case render thread goes here */

	newlen=1;
	newmal = (void **)malloc ((oldlen+newlen)*sizeof(void **));
	if (newmal == 0) {
		printf ("cant malloc memory for addChildren");
		return;
	}

	/* copy the old stuff over */
	if (oldlen > 0) memcpy (newmal,par->p,oldlen*sizeof(void **));

	/* increment pointer to point to place for new addition */
	place = (void **) ((unsigned long int) newmal + sizeof (void **) * oldlen);

	/* and store the new child. */
	*place = newNode;

	/* set up the C structures for this new MFNode addition */
	tmp = par->p;
	par->p = newmal;
	par->n = oldlen+newlen;

	/* XXXX MEMORY LEAK XXXX */
	/* if tmp is freed, and if the caller to this is not the rendering thread,
	   and the freed memory is used somewhere else, it is possible to have the 
	   rendering thread use the values that the memory block is assigned - so
	   for now, don't free it. We could keep a list for later garbage collection,
	   but, generally, it is not too many bytes lost if we ignore it. 

	   oh, and, if you don't believe the above comment; run tests/33.wrl with
	   bounds checking, and see what happens with the free uncommented. */

	/* FREE_IF_NZ(tmp); */

	
	/* { int i;
		for (i=0; i<par->n; i++) {
		printf ("addToNode, child %d is %d\n",i,par->p[i]);
		}
	} */

}

/* on a ReplaceWorld call, tell the Browser.pm module to forget all about its past */
void kill_DEFS (void) {
	if (useExperimentalParser) destroyCParserData();
	else __pt_zeroDEFS();
}

/* for ReplaceWorld (or, just, on start up) forget about previous bindables */

void kill_bindables (void) {
	totfognodes=0;
	totbacknodes=0;
	totnavnodes=0;
	totviewpointnodes=0;
	currboundvpno=0;
	FREE_IF_NZ(fognodes);
	FREE_IF_NZ(backgroundnodes);
	FREE_IF_NZ(navnodes);
	FREE_IF_NZ(viewpointnodes);
}


void registerBindable (void *ptr) {
	struct X3D_Box *node;


	node = (struct X3D_Box *)ptr;
	/* printf ("registerBindable, on node %d %s\n",node,stringNodeType(node->_nodeType));  */
	switch (node->_nodeType) {
		case NODE_Viewpoint:
		case NODE_GeoViewpoint:
			viewpointnodes = realloc (viewpointnodes, (sizeof(void *)*(totviewpointnodes+1)));
			viewpointnodes[totviewpointnodes] = ptr;
			totviewpointnodes ++;
			break;
		case NODE_Background:
		case NODE_TextureBackground:
			backgroundnodes = realloc (backgroundnodes, (sizeof(void *)*(totbacknodes+1)));
			backgroundnodes[totbacknodes] = ptr;
			totbacknodes ++;
			break;
		case NODE_NavigationInfo:
			navnodes = realloc (navnodes, (sizeof(void *)*(totnavnodes+1)));
			navnodes[totnavnodes] = ptr;
			totnavnodes ++;
			break;
		case NODE_Fog:
			fognodes = realloc (fognodes, (sizeof(void *)*(totfognodes+1)));
			fognodes[totfognodes] = ptr;
			totfognodes ++;
			break;
		default: {
			/* do nothing with this node */
			/* printf ("got a registerBind on a node of type %s - ignoring\n",
					stringNodeType(node->_nodeType));
			*/
			return;
		}                                                

	}
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
unsigned int _pt_CreateVrml (char *tp, char *inputstring, uintptr_t *retarr) {
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
	else if (strcmp(tp,"String")==0)
		count = call_pv("VRML::Browser::EAI_CreateVrmlFromString", G_ARRAY);
	else if (strcmp(tp,"CREATEPROTO")==0)
		count = call_pv("VRML::Browser::EAI_CreateX3DNodeFromPROTO", G_ARRAY);
	else
		count = call_pv("VRML::Browser::EAI_CreateX3DNodeFromString", G_ARRAY);
	SPAGAIN ;

	/* Perl is returning a series of BN/node# pairs, reorder to node#/BN.*/
	for (tmp = 1; tmp <= count; tmp++) {
		retarr[count-tmp] = POPi;
		/* printf ("popped off %d\n",retarr[count-tmp]); */
	}

	PUTBACK;
	FREETMPS;
	LEAVE;

	return (count);
}


/* zero the bindables in Browser. */
void __pt_zeroDEFS() {
	dSP;
	PUSHMARK(SP);
	call_pv("VRML::Browser::zeroDEFS", G_ARRAY);
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

/* Shutter glasses, stereo mode configure  Mufti@rus*/
float eyedist = 0.06;
float screendist = 0.8;

void setEyeDist (const char *optArg) {
	int i;
	i= sscanf(optArg,"%f",&eyedist);
	if (i==0) printf ("warning, command line eyedist parameter incorrect - was %s\n",optArg);
}

void setScreenDist (const char *optArg) {
	int i;
	i= sscanf(optArg,"%f",&screendist);
	if (i==0) printf ("warning, command line screendist parameter incorrect - was %s\n",optArg);
}
/* end of Shutter glasses, stereo mode configure */


void __pt_openBrowser() {

	dSP;
	ENTER;
	SAVETMPS;

	PUSHMARK(SP);
	XPUSHs(sv_2mortal(newSViv(1000))); /*  left in as an example*/
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
	struct X3D_Inline *inl;
	STRLEN xx;
	char *thisurl;
	char *slashindex;
	char firstBytes[4];
	inl = (struct X3D_Inline *)psp.ptr;
	inurl = &(inl->url);
	filename = (char *)malloc(1000);

	/* lets make up the path and save it, and make it the global path */
	count = strlen(SvPV(inl->__parenturl,xx));
	psp.path = (char *)malloc ((unsigned)(count+1));

	if ((!filename) || (!psp.path)) {
		outOfMemory ("perl thread can not malloc for filename\n");
	}

	/* copy the parent path over */
	strcpy (psp.path,SvPV(inl->__parenturl,xx));

	/* and strip off the file name, leaving any path */
	slashindex = (char *) rindex(psp.path, ((int) '/'));
	if (slashindex != NULL) {
		slashindex ++; /* leave the slash there */
		*slashindex = 0;
	} else {psp.path[0] = 0;}
	/* printf ("doInLine, parenturl is %s\n",psp.path);*/

	/* try the first url, up to the last, until we find a valid one */
	count = 0;
	while (count < inurl->n) {
		thisurl = SvPV(inurl->p[count],xx);

		/* check to make sure we don't overflow */
		if ((strlen(thisurl)+strlen(psp.path)) > 900) break;

		/* we work in absolute filenames... */
		makeAbsoluteFileName(filename,psp.path,thisurl);

		if (fileExists(filename,firstBytes,TRUE)) {
			break;
		}
		count ++;
	}
	psp.inp = filename; /* will be freed later */
	/* printf ("doinline, psp.inp = %s\n",psp.inp);*/
	/* printf ("inlining %s\n",filename); */

	/* were we successful at locating one of these? if so,
	   make it into a FROMURL */
	if (count != inurl->n) {
		/* printf ("we were successful at locating %s\n",filename); */
		psp.type=FROMURL;
	} else {
		if (count > 0) printf ("Could Not Locate URL (last choice was %s)\n",filename);
	}
}

/* this is a CreateVrmlFrom URL or STRING command */
void __pt_doStringUrl () {
	int count;
	int retval;
	uintptr_t myretarr[2000];

	/* for cParser */
        char *buffer;
	struct X3D_Group *nRn;

	if (useExperimentalParser) {
		if (psp.zeroBind) {
			destroyCParserData();
			kill_bindables();
			psp.zeroBind = FALSE;
		}

		if (psp.type==FROMSTRING) {
ConsoleMessage ("cant FROMSTRING with cParser yet\n");

		} else if (psp.type==FROMURL) {
			pushInputURL (psp.inp);
	        	buffer = readInputString(psp.inp,"");
			nRn = (struct X3D_Group *) createNewX3DNode(NODE_Group);
			cParse (nRn,offsetof (struct X3D_Group, children), buffer);
			FREE_IF_NZ (buffer); 


		} else if (psp.type==FROMCREATENODE) {
ConsoleMessage ("cant  iFROMCREATENODE with cParser yet\n");
		} else 
ConsoleMessage ("cant FROMWHATEVER with cParser yet\n");


		/* set bindables, if required */
		if (psp.bind) {
			if (totfognodes != 0) send_bind_to (NODE_Fog,(fognodes[0]),1);
			if (totbacknodes != 0) send_bind_to (NODE_Background,(void *)(backgroundnodes[0]),1);
			if (totnavnodes != 0) send_bind_to (NODE_NavigationInfo,(void *)(navnodes[0]),1);
			if (totviewpointnodes != 0) send_bind_to(NODE_Viewpoint,(void *)(viewpointnodes[0]),1);
		}

		/* did the caller want these values returned? */
		if (psp.retarr != NULL) {
			for (count=0; count < nRn->children.n; count++) {
				psp.retarr[count*2] = 0; /* the "perl" node number */
				psp.retarr[count*2+1] = ((uintptr_t *) 
					nRn->children.p[count]); /* the Node Pointer */
			}
			psp.retarrsize = nRn->children.n * 2; /* remember, the old "perl node number" */
		}

	       	/* now that we have the VRML/X3D file, load it into the scene. */
		if (psp.ptr != NULL) {
			/* add the new nodes to wherever the caller wanted */
			for (count=0; count < nRn->children.n; count++) {
				/* add this child to the node */
				addToNode(psp.ptr,psp.ofs,nRn->children.p[count]);

				/* tell the child that it has a new parent! */
				add_parent(nRn->children.p[count],psp.ptr);
			}
			update_node(psp.ptr);
		}


		retval = 0;
		count = 0;
	} else {
	
		if (psp.zeroBind) {
			/* printf ("doStringUrl, have to zero Bindables in Perl\n"); */
			__pt_zeroDEFS();
			kill_bindables();
			psp.zeroBind=FALSE;
		}
	
		if (psp.type==FROMSTRING) {
	       		retval = _pt_CreateVrml("String",psp.inp,myretarr);
		} else if (psp.type==FROMURL) {
			pushInputURL (psp.inp);
			retval = _pt_CreateVrml("URL",psp.inp,myretarr);
		} else if (psp.type==FROMCREATENODE) {
			retval = _pt_CreateVrml("CREATENODE",psp.inp,myretarr);
		} else retval = _pt_CreateVrml("CREATEPROTO",psp.inp,myretarr);
	
		/* printf ("__pt_doStringUrl, retval %d; retarr %d\n",retval,psp.retarr);  */
	
	
		/* copy the returned nodes to the caller */
		if (psp.retarr != NULL) {
			/* printf ("returning to EAI caller, psp.retarr = %d, count %d\n", psp.retarr, retval); */
			for (count = 0; count < retval; count ++) {
				/* printf ("	...saving %d in %d\n",myretarr[count],count); */
				psp.retarr[count] = myretarr[count];
			}
			psp.retarrsize = retval;
		}
	
		/* send a set_bind to any nodes that exist */
		if (psp.bind) {
			if (totfognodes != 0) send_bind_to (NODE_Fog,(void *)(fognodes[0]),1);
			if (totbacknodes != 0) send_bind_to (NODE_Background,(void *)(backgroundnodes[0]),1);
			if (totnavnodes != 0) send_bind_to (NODE_NavigationInfo,(void *)(navnodes[0]),1);
			if (totviewpointnodes != 0) send_bind_to(NODE_Viewpoint,(void *)(viewpointnodes[0]),1);
		}
	
	       	/* now that we have the VRML/X3D file, load it into the scene.
	       	   myretarr contains node number/memory location pairs; thus the count
	       	   by two. */
		if (psp.ptr != NULL) {
			/* if we have a valid node to load this into, do it */
			/* note that EAI CreateVRML type commands will NOT give */
			/* a valid node */
	
		       	for (count =1; count < retval; count+=2) {
				/* printf ("__pt_doStringUrl, adding count %d %d\n", count,myretarr[count]); */
				/* add this child to the node */
	       			addToNode(psp.ptr, psp.ofs, (void *)(myretarr[count]));
	
				/* tell the child that it has a new parent! */
				add_parent((void *)myretarr[count],psp.ptr);
	       		}
			/* tell the node that we have changed */
			update_node(psp.ptr);
		}
	}
}


/************************END OF NORMAL ROUTINES*********************/

#ifdef USEEAIANDPERL
*unused*	/****************************** EAI ****************************************/
*unused*	
*unused*	/* get node info, send in a character string, get a node reference number */
*unused*	void __pt_EAI_GetNode () {
*unused*		int count;
*unused*		SV * myret;
*unused*		char *ctmp;
*unused*		STRLEN len;
*unused*	
*unused*		dSP;
*unused*		ENTER;
*unused*		SAVETMPS;
*unused*		PUSHMARK(SP);
*unused*		/* this is for integers XPUSHs(sv_2mortal(newSViv(nname)));*/
*unused*		XPUSHs(sv_2mortal(newSVpv(psp.fieldname, 0)));
*unused*	
*unused*	
*unused*		PUTBACK;
*unused*		count = call_pv("VRML::Browser::EAI_GetNode", G_SCALAR);
*unused*		SPAGAIN ;
*unused*	
*unused*		if (count != 1)
*unused*			printf ("EAI_getNode, node returns %d\n",count);
*unused*	
*unused*		/* return value in psp.retsv */
*unused*		myret = POPs;
*unused*	       	ctmp = SvPV(myret,len); /*  now, we have the length*/
*unused*	       	psp.retstr = (char *)malloc (sizeof (char) * (len+5));
*unused*	       	strcpy (psp.retstr,ctmp);
*unused*	
*unused*		PUTBACK;
*unused*		FREETMPS;
*unused*		LEAVE;
*unused*	}
*unused*	
*unused*	/* get Viewpoint CNode; send in a character string, get a memory ptr */
*unused*	void __pt_EAI_GetViewpoint () {
*unused*		int count;
*unused*	
*unused*		dSP;
*unused*		ENTER;
*unused*		SAVETMPS;
*unused*		PUSHMARK(SP);
*unused*		/* this is for integers XPUSHs(sv_2mortal(newSViv(nname)));*/
*unused*		XPUSHs(sv_2mortal(newSVpv(psp.fieldname, 0)));
*unused*	
*unused*	
*unused*		PUTBACK;
*unused*		count = call_pv("VRML::Browser::EAI_GetViewpoint", G_SCALAR);
*unused*		SPAGAIN ;
*unused*	
*unused*		if (count != 1)
*unused*			printf ("EAI_getViewpoint, node returns %d\n",count);
*unused*	
*unused*		/* return value in psp.jparamcount */
*unused*		psp.jparamcount = POPi;
*unused*	
*unused*		/* printf ("The node is %x\n", psp.jparamcount) ;*/
*unused*		PUTBACK;
*unused*		FREETMPS;
*unused*		LEAVE;
*unused*	}
*unused*	
*unused*	
*unused*	/* set/delete route */
*unused*	void __pt_SAI_Command () {
*unused*		int count;
*unused*		SV *svret;
*unused*		char *ctmp;
*unused*		STRLEN len;
*unused*	
*unused*		dSP;
*unused*		ENTER;
*unused*		SAVETMPS;
*unused*		PUSHMARK(SP);
*unused*		XPUSHs(sv_2mortal(newSViv(psp.ofs)));
*unused*		XPUSHs(sv_2mortal(newSVpv(psp.fieldname, 0)));
*unused*		PUTBACK;
*unused*		count = call_pv("VRML::Browser::EAI_Command", G_SCALAR);
*unused*		SPAGAIN ;
*unused*	
*unused*		if (count != 1) 
*unused*			printf ("EAI_Command returns %d\n",count);
*unused*	
*unused*		/* return value in psp.jparamcount */
*unused*	
*unused*		/* did we want an integer, or an SV? */
*unused*		if (psp.ptr != NULL) {
*unused*			svret = POPs;
*unused*			/* printf ("SAI_SCommand, svret %d, svType %d\n",svret, SvTYPE(svret)); */
*unused*	
*unused*	        	/* make a copy of the return string - caller has to free it after use */
*unused*	        	ctmp = SvPV(svret,len); /*  now, we have the length*/
*unused*	        	psp.retstr = (char *)malloc (sizeof (char) * (len+5));
*unused*	        	strcpy (psp.retstr,ctmp);
*unused*	        	/* printf ("GetValue, retstr will be :%s:\n",psp.retstr); */
*unused*	
*unused*			psp.sv = (SV *)svret;
*unused*		} else {
*unused*			psp.jparamcount = POPi;
*unused*			/* printf ("and, the return value of EAI_Command is %d\n",psp.jparamcount); */
*unused*		}
*unused*	
*unused*		PUTBACK;
*unused*		FREETMPS;
*unused*		LEAVE;
*unused*	}
*unused*	
*unused*	void __pt_EAI_GetType (){
*unused*		unsigned int 	count;
*unused*	
*unused*		dSP;
*unused*		ENTER;
*unused*		SAVETMPS;
*unused*		PUSHMARK(SP);
*unused*	
*unused*		/* push on the nodenum, fieldname and direction */
*unused*		XPUSHs(sv_2mortal(newSViv(psp.jparamcount)));
*unused*		XPUSHs(sv_2mortal(newSVpv(psp.fieldname, 0)));
*unused*		XPUSHs(sv_2mortal(newSVpv((const char *)psp.ptr, (STRLEN)0)));
*unused*	
*unused*		PUTBACK;
*unused*		count = call_pv("VRML::Browser::EAI_GetType",G_ARRAY);
*unused*		SPAGAIN;
*unused*	
*unused*		if (count != 5) {
*unused*			/* invalid return values; make *nodeptr = 97, the rest 0 */
*unused*			psp.Etype[0]=97;	/* SFUNKNOWN - check CFuncs/EAIServ.c */
*unused*			psp.Etype[4] = 0;
*unused*			psp.Etype[3] = 0;
*unused*			psp.Etype[2] = 0;
*unused*			psp.Etype[1] = 0;
*unused*		} else {
*unused*			/* pop values off stack in reverse of perl return order */
*unused*			psp.Etype[4] = POPi; psp.Etype[3] = POPi; psp.Etype[2] = POPi;
*unused*			psp.Etype[1] = POPi; psp.Etype[0] = POPi;
*unused*		}
*unused*	
*unused*		PUTBACK;
*unused*		FREETMPS;
*unused*		LEAVE;
*unused*	}
*unused*	
*unused*	void __pt_EAI_GetValue (){
*unused*		unsigned int 	count;
*unused*		STRLEN len;
*unused*		char *ctmp;
*unused*	
*unused*		SV * retval;
*unused*		dSP;
*unused*		ENTER;
*unused*		SAVETMPS;
*unused*		PUSHMARK(SP);
*unused*	
*unused*		/* push on the nodenum, fieldname and direction */
*unused*		XPUSHs(sv_2mortal(newSViv(psp.jparamcount)));
*unused*		XPUSHs(sv_2mortal(newSVpv(psp.fieldname, 0)));
*unused*		/*  this was for pushing on an integer... XPUSHs(sv_2mortal(newSViv(psp.inp)));*/
*unused*	
*unused*		PUTBACK;
*unused*		count = call_pv("VRML::Browser::EAI_GetValue",G_EVAL|G_SCALAR);
*unused*		SPAGAIN;
*unused*	
*unused*		/* printf ("GetValue return; count %d\n",count);*/
*unused*		if (count != 1) {
*unused*			psp.sv=NULL;
*unused*		} else {
*unused*			/* pop values off stack in reverse of perl return order */
*unused*			retval = POPs;
*unused*		}
*unused*	
*unused*		PUTBACK;
*unused*		/* printf ("EAI_GetValue retval %d\n", retval) ;*/
*unused*	
*unused*		/* if (SvOK(retval)) {printf ("retval is an SV\n"); }*/
*unused*		/* else {printf ("retval is NOT an SV\n"); return;}*/
*unused*		/* now, decode this SV */
*unused*		/* printf ("SVtype is %x\n",SvTYPE(retval));*/
*unused*		/* printf ("String is :%s: len %d \n",SvPV(retval,len),len);*/
*unused*	
*unused*		/* make a copy of the return string - caller has to free it after use */
*unused*		ctmp = SvPV(retval,len); /*  now, we have the length*/
*unused*		psp.retstr = (char *)malloc (sizeof (char) * (len+5));
*unused*		strcpy (psp.retstr,ctmp);
*unused*		/* printf ("GetValue, retstr will be :%s:\n",psp.retstr);*/
*unused*	
*unused*		FREETMPS;
*unused*		LEAVE;
*unused*	}
*unused*	
*unused*	
*unused*	void __pt_EAI_GetTypeName (){
*unused*		unsigned int 	count;
*unused*		STRLEN len;
*unused*	
*unused*		SV * retval;
*unused*		char *ctmp;
*unused*		dSP;
*unused*		ENTER;
*unused*		SAVETMPS;
*unused*		PUSHMARK(SP);
*unused*	
*unused*		/* push on the nodenum */
*unused*		XPUSHs(sv_2mortal(newSViv(psp.jparamcount)));
*unused*	
*unused*		PUTBACK;
*unused*		count = call_pv("VRML::Browser::EAI_GetTypeName",G_EVAL|G_SCALAR);
*unused*		SPAGAIN;
*unused*	
*unused*		/* printf ("GetTypeName return; count %d\n",count);*/
*unused*		if (count != 1) {
*unused*			psp.sv=NULL;
*unused*		} else {
*unused*			/* pop values off stack in reverse of perl return order */
*unused*			retval = POPs;
*unused*		}
*unused*	
*unused*		PUTBACK;
*unused*	
*unused*		/* make a copy of the return string - caller has to free it after use */
*unused*		ctmp = SvPV(retval, len); /*  now, we have the length*/
*unused*		psp.retstr =(char *) malloc (sizeof (char) * (len+5));
*unused*		strcpy (psp.retstr,ctmp);
*unused*		/* printf ("GetTypeName, retstr will be :%s:\n",psp.retstr);*/
*unused*	
*unused*		FREETMPS;
*unused*		LEAVE;
*unused*	}
*unused*	
#endif
