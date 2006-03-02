/*******************************************************************
 Copyright (C) 2004 John Stewart, CRC Canada.
 Based on methods produced and documented in the (now obsolete)
 FreeWRL file VRMLJava.pm:
 Copyright (C) 1998 Tuomas J. Lukka
               1999 John Stewart CRC Canada
               2002 Jochen Hoenicke

 DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
 See the GNU Library General Public License (file COPYING in the distribution)
 for conditions of use and redistribution.
*********************************************************************/

/* Open and communicate with a java .class file. We use as much EAI code
as we can.
*/

#include <math.h>

#include "headers.h"
#include "EAIheaders.h"
#include <stdlib.h>


#define MURLLEN 2000
#define CLASSVER "JavaClass version 1.0 - www.crc.ca"

/* commands sent back from the class */
#define FN "FINISHED"
#define GF "GETFIELD"
#define RF "READFIELD"
#define SE "JSENDEV"
#define GT "GETTYPE"
#define CV "CREATEVRML"

/* Function definitions for local functions */
int newClassConnection (int scriptno);
void makeJavaInvocation (char *commandline, int scriptno);
void send_int (int node, int fd);
void send_type (int node, int offset, int len, int fd);
void send_double (double dval, int scriptno);
void receive_command(int scriptno);

int JavaClassVerbose = 0;

/* input ClassBuffer */
int bufcount;
int bufsize;
char *ClassBuffer;
int eid = 0; /* event id */
int startEntry, endEntry;

void send_string (char *string, int fd) ;

extern void EAI_Convert_mem_to_ASCII (int id, char *reptype, int type, char *memptr, char *buf);

/* a new .class file has been called - lets load it */
int newJavaClass(int scriptInvocationNumber,char * nodeURLstr,char *nodeID) {
	char newURL [MURLLEN];
	char *ri;
	char *rv;

	if (JavaClassVerbose)
		printf ("newJavaClass, number %d url %s nodeID %s MaxScript %d\n",
				scriptInvocationNumber, nodeURLstr, nodeID,JSMaxScript);
	        /* more scripts than we can handle right now? */
        if (scriptInvocationNumber >= JSMaxScript)  {
                JSMaxAlloc();
        }



	/* register this script.... */
	CRoutes_js_new(scriptInvocationNumber, CLASSSCRIPT);

	/* make sure we tell the EAI socket code that this is not opened yet */
	ScriptControl[scriptInvocationNumber].listen_fd = -1;
	ScriptControl[scriptInvocationNumber].send_fd = -1;

	if (JavaClassVerbose)
		printf ("newJavaClass, past ScriptControl...\n");

	if (strlen(nodeID)>19) {
		printf ("warning, copy problem in newJavaClass\n");
		nodeID[18] = '\0';
	}
	strcpy(ScriptControl[scriptInvocationNumber].NodeID,nodeID);


	/* we have a url; have to make it into a real url for Java.  */
	/* **** NOTE: WE CAN USE A ProdCon.c ROUTINE FOR THIS CHECK */
	if ((strncmp("file:",nodeURLstr,5)!=0) &&
		(strncmp("FILE:",nodeURLstr,5)!=0)  &&
		(strncmp("HTTP:",nodeURLstr,5)!=0)  &&
		(strncmp("http:",nodeURLstr,5)!=0)) {

		/* start file off with a file: */
		strcpy (newURL,"file:");

		/* now, is the world relative to cwd, or not? */
		if (BrowserURL[0] == '/') {
			strncat (&newURL[5],BrowserURL,MURLLEN-10);
		} else {
			rv=getcwd (&newURL[5],MURLLEN-10);
			strcat (newURL,"/");
			strncat (newURL,BrowserURL,MURLLEN-100);
		}

		/* now, strip off the wrl filename, and place our name here */
		ri = (char *)rindex((char *)newURL,(int) '/');
		*ri = '\0';

		strcat (newURL,"/");
		strcat (newURL,nodeURLstr);
	} else {
		strncpy (newURL,nodeURLstr,MURLLEN-4);
		newURL[MURLLEN-1] = '\0';		/* always terminate */
	}
	if (JavaClassVerbose) printf ("JavaClass:class url is now %s\n",newURL);

	if (!(newClassConnection(scriptInvocationNumber))) return FALSE;

	send_string("NEWSCRIPT", scriptInvocationNumber);
	send_string (nodeID,scriptInvocationNumber);
	send_string (newURL, scriptInvocationNumber);
	send_int (eid, scriptInvocationNumber);
	return TRUE;
}

int initJavaClass(int scriptno) {

	ClassBuffer[0] = '\0';
	bufcount = 0;

	/* run initialize method */
        eid++;
        /* printf ("JavaClass:INITIALIZE script %d\n",scriptno);*/
        send_string("INITIALIZE",scriptno);
	send_string (ScriptControl[scriptno].NodeID,scriptno);
	send_int ((int)eid,scriptno);

        receive_command(scriptno);
}

void sendCLASSEvent(uintptr_t fn, int scriptno, char *fieldname, int type, int len) {
	char mystring [100];
	char mytype[10];

	/* printf ("sending ClassEvent from %d to script %d, node %s, field %d, type %d len %d\n",*/
	/* 		fn, scriptno, ScriptControl[scriptno].NodeID, fieldname, type, len);*/

	eid++;
	sprintf (mytype, "%d",type);
	EAI_Convert_mem_to_ASCII(eid,mytype,
			type+(int)EAI_SFUNKNOWN,(char *)fn,mystring);

	send_string("SENDEVENT", scriptno);
	send_string(ScriptControl[scriptno].NodeID,scriptno);
	send_string (fieldname, scriptno);
	send_string(mystring,scriptno);
	send_double (TickTime,scriptno);  /*  this is the time*/
        receive_command(scriptno);
}

void processClassEvents(int scriptno, int startent, int endent) {
	/* save routing table indexes for this script for  JSENDEV
	 * in receive_command */
	startEntry = startent;
	endEntry = endent;

        eid++;
        send_string("EVENTSPROCESSED",scriptno);
	send_string (ScriptControl[scriptno].NodeID,scriptno);
	send_int ((int)eid,scriptno);
        receive_command(scriptno);
}

/* recieve a string from the java class */
int receive_string (int scriptno) {
	int sleepcount;
	int lfd;

	/* printf ("JavaClass:start of receive_string\n");*/
	lfd = ScriptControl[scriptno].listen_fd;

	sleepcount = 1;
	while (bufcount == 0) {
		if (sleepcount>=3000) {
			printf ("FreeWRL Timeout: java class on socket - class problem?\n");
			return FALSE;
		}
		usleep (100000);
		ClassBuffer = read_EAI_socket(ClassBuffer,&bufcount, &bufsize, &lfd);
	}
	ClassBuffer[bufcount]='\0';
	/* printf ("JavaClass:readEAIsocket, :%s: bufcount %d\n",ClassBuffer,bufcount);*/
	if (JavaClassVerbose) printf ("FM JAVA:%s:\n",ClassBuffer);
	return TRUE;
}


/* send a constant string along... */
void send_string (char *string, int scriptno) {
	char mystring[100];

	if (strlen(string) > 99) {printf ("JavaClass:send_string, too long: %s\n",string); return;}
	strcpy (mystring, string);
	if (JavaClassVerbose) printf ("TO JAVA :%s:\n",string);
	EAI_send_string(mystring,ScriptControl[scriptno].listen_fd);
}

/* convert an integer to a string, and send it */
void send_int (int node, int scriptno) {
	char myintbuf[100];
	sprintf (myintbuf,"%d",node);
	if (JavaClassVerbose) printf ("TO JAVA :%s:\n",myintbuf);
	EAI_send_string(myintbuf,ScriptControl[scriptno].listen_fd);
}

/* convert an integer to a string, and send it */
void send_double (double dval, int scriptno) {
	char myintbuf[100];
	sprintf (myintbuf,"%lf",dval);
	if (JavaClassVerbose) printf ("TO JAVA :%s:\n",myintbuf);
	EAI_send_string(myintbuf,ScriptControl[scriptno].listen_fd);
}
/* convert an integer to a type, and send it */
void send_type (int node, int offset, int len, int scriptno) {
	char localchar[300];

	node = node - (int)'a'; /* no longer "ascii" based... */

	/* printf ("send_type, type now is %d\n",node);*/
	localchar[0] = '\0';
	/* sprintf (localchar, "%s %d",FIELD_TYPE_STRING(node), offset);*/
	sprintf (localchar, "%d %d %d",node, offset, len);

	if (JavaClassVerbose) printf ("TO JAVA :%s:\n",localchar);
	EAI_send_string(localchar,ScriptControl[scriptno].listen_fd);
}

/* new class - open file */
int newClassConnection (int scriptno) {
	char commandline[MURLLEN];
	int sleepcount;
	int fd, lfd;

	/* allocate memory for input ClassBuffer */
	bufcount = 0;
	bufsize = 2 * EAIREADSIZE; /*  initial size*/
	ClassBuffer = (char *)malloc(bufsize * sizeof (char));
	if (ClassBuffer == 0) {
		printf ("can not malloc memory for input ClassBuffer in create_EAI\n");
		return FALSE;
	}

	/* printf ("JavaClass:start of newClassConnection, scriptno is %d\n",scriptno);*/

	/* make the communications socket */
	fd = -1; lfd = -1;
	if (!conEAIorCLASS(scriptno+1, &fd, &lfd)) {
		printf ("could not open CLASS socket for script %d\n",scriptno);
		return FALSE;
	}
	if (JavaClassVerbose) printf ("JavaClass:socket %d lsocket %d\n",fd, lfd);

	/* make the commandline */
	makeJavaInvocation (commandline,scriptno+1);
	if (strlen(commandline) <= 0) return FALSE;

	/* invoke the java interpreter */
	if ((freewrlSystem (commandline)) < 0) {
		printf ("JavaClass:error calling %s\n",commandline);
		return FALSE;
	}


	sleepcount = 1;
	while (lfd<0) {
		if (sleepcount>=3000) {
			printf ("FreeWRL Timeout: java class on socket - class problem?\n");
			return FALSE;
		}
		usleep (100000);
		conEAIorCLASS(scriptno+1, &fd, &lfd);
		sleepcount++;
	}

	/* printf ("JavaClass:connection open, lfd %d\n",lfd);*/

	/* save the file descriptors */
	ScriptControl[scriptno].listen_fd = lfd;
	ScriptControl[scriptno].send_fd = fd;

	/* get the handshake version */
	if (!receive_string(scriptno)) return FALSE;

	/* do we have the correct version? */
	if (strncmp(CLASSVER, ClassBuffer, strlen(CLASSVER)) != 0) {
		printf ("FreeWRL - JavaClass version prob; expected :%s: got :%s:\n",
			CLASSVER, ClassBuffer);
		return FALSE;
	}

	/* throw away ClassBuffer contents */
	bufcount = 0;

	/* whew ! done. */
	/* printf ("JavaClass:newClassConnection finished\n");*/
	return TRUE;
}

/* make up the command line required to invoke the Java interpreter */
void makeJavaInvocation (char *commandline, int scriptno) {

 	char vrmlJar[MURLLEN];
 	char javaPolicy[MURLLEN];
 	char *libdir;
 	FILE *vJfile, *jPfile;
 	char *myenv;
 	int lenenv;
 	char myc[100];


 	if (JavaClassVerbose)
 		printf ("JavaClass:perlpath: %s, builddir %s\n",myPerlInstallDir, BUILDDIR);
 	commandline[0] = '\0';

	if (strncmp(JAVA,"JAVAISNOTDEFINED",strlen(JAVA)) != 0) {
		printf ("JavaClsss: java interpreter not found at build time\n");
		return;
	}

 	/* get the CLASSPATH, if one exists */
 	myenv = getenv ("CLASSPATH");
 	if (myenv == NULL) {
 		lenenv = 0; /* no classpath */
 	} else {
 		lenenv = strlen(myenv);
 	}

 	/* find the vrml.jar and the java.policy files */
 	/* look in the perl path first */
 	libdir = myPerlInstallDir; /* assume it is installed... */
 	strncpy (vrmlJar,myPerlInstallDir,MURLLEN-20);
 	strncpy (javaPolicy,myPerlInstallDir,MURLLEN-20);
 	strcat (vrmlJar,"/vrml.jar");
 	strcat (javaPolicy,"/java.policy");

 	vJfile = fopen (vrmlJar,"r");
 	jPfile = fopen (javaPolicy,"r");

 	/* did we find the vrml.jar file? */
 	if (!vJfile) {
 		strncpy (vrmlJar,BUILDDIR,MURLLEN-20);
 		strcat (vrmlJar, "/java/classes/vrml.jar");
 		vJfile = fopen (vrmlJar,"r");
 		if (!vJfile) {
 			printf ("JavaClass:FreeWRL can not find vrml.jar\n");
 			commandline[0] = '\0';
 			return;
 		}

 		libdir = BUILDDIR;
 	}
 	fclose (vJfile);

 	/* did we find the java.policy file? */
 	if (!jPfile) {
 		strncpy (javaPolicy,BUILDDIR,MURLLEN-20);
 		strcat (javaPolicy, "/java/classes/java.policy");
 		jPfile = fopen (javaPolicy,"r");
 		if (!jPfile) {
 			printf ("JavaClass:FreeWRL can not find java.policy\n");
 			commandline[0] = '\0';
 			return;
 		}
 	}
 	fclose (jPfile);

 	/* ok, we have found the jar and policy files... */
 	if (JavaClassVerbose)
 		printf ("JavaClass:found %s and %s\n",vrmlJar,javaPolicy);

 	/* expect to make a line like:
 		java -Dfreewrl.lib.dir=/usr/lib/perl5/site_perl/5.8.0/i586-linux-thread-multi/VRML
 		  -Djava.security.policy=/usr/lib/perl5/site_perl/5.8.0/i586-linux-thread-multi/VRML/java.policy
 		  -classpath /usr/lib/perl5/site_perl/5.8.0/i586-linux-thread-multi/VRML/vrml.jar
 		  vrml.FWJavaScript
 	*/

 	/* basic string checking - bounds checking of commandline length */
 	if ((lenenv + strlen(vrmlJar) + strlen(javaPolicy) + strlen (myPerlInstallDir)) > (MURLLEN - 100)) {

 		printf ("we have a memory problem with MURLLEN...\n");
 		commandline[0] = '\0';
 		return;
 	}

	strcat (commandline,JAVA);
 	strcat (commandline," -Dfreewrl.lib.dir=");
 	strcat (commandline,libdir);
 	strcat (commandline, " -Djava.security.policy=");
 	strcat (commandline, javaPolicy);

 	/* classpath, if one exists */
 	strcat (commandline, " -classpath ");
 	strcat (commandline, vrmlJar);
 	if (lenenv > 0) {
 		strcat (commandline,":");
 		strcat (commandline, myenv);
 	}

 	/* and, the command to run */
 	sprintf (myc, " vrml.FWJavaScript %d &",scriptno+EAIBASESOCKET);
 	strcat (commandline, myc);

 	if (JavaClassVerbose) printf ("JavaClass:command line %s\n",commandline);
}


/* parse commands from the java class until a finished command is received */
void receive_command(int scriptno) {
	char ctmp[1000], dtmp[1000];
	int uretval;
	uintptr_t nodeptr;
	int ra, rb, rc, rd;
	char *ptr;
	int scripttype;
	int tmp;
	char *retstr;
	char *EOT;
	int nodarr[200];
	int finished_found;
	int rv;

	finished_found = FALSE;
	ptr = ClassBuffer;
	/* printf ("JavaClass:start of receive_command, buffer len %d contents %s\n",strlen(ClassBuffer),ClassBuffer);*/

	while (1) {
		/* is the buffer empty? */
		if (strlen(ptr) == 0) {
			bufcount=0;	/* tell read to start from beginning */
			receive_string(scriptno);
			ptr = ClassBuffer;
		}


		/* printf ("JavaClass:RC while loop, bufcount %d, buf :%s:\n",bufcount,ptr);*/

		if (strncmp(ptr,FN,strlen(FN)) == 0) {
			/* printf ("JavaClass:receive_command, FINISHED\n");*/
			ptr += strlen(FN) +1;
			finished_found = TRUE;
		} else if (strncmp (ptr,GF,strlen(GF)) == 0) {
			/* printf ("JavaClass:receive_command, GETFIELD\n");*/

			ptr += strlen(GF) +1;

			/*  get the node id number - equiv of sscanf.*/
			uretval=0;
			while ((*ptr>='0') && (*ptr<='9')) {
				uretval = uretval*10 + (int) *ptr - (int) '0';
				ptr++;
			}
			/* printf ("JavaClass:node is NODE%d\n",uretval);*/

			/* get the field name */
			while (*ptr != ' ') {ptr++;} ptr++;
			tmp=0;
			while (*ptr!=' '){ctmp[tmp]=*ptr;tmp++;ptr++;}
			ctmp[tmp] = '\0';

			/* get the field type */
			tmp=0; ptr++;
			while (*ptr >' '){dtmp[tmp]=*ptr;tmp++;ptr++;}
			dtmp[tmp] = '\0';

			EAI_GetType (uretval,ctmp,dtmp,&ra,&rb,&rc,&rd,&scripttype);
			/* printf ("GT returns %d %d %d %d\n",ra, rb,rc,rd);*/
			/*  send the type offset, and length*/
			send_type(rd,rb,rc,scriptno);
			/* printf ("JavaClass:done GETFIELD\n");*/

		} else if (strncmp(ptr,RF,strlen(RF)) == 0) {
			/* printf ("JavaClass:receive_command, READFIELD\n");*/
			ptr += strlen(RF) +1;

			/*  get the node id number - equivalent of sscanf*/
			uretval=0;
			while ((*ptr>='0') && (*ptr<='9')) {
				uretval = uretval*10 + (int) *ptr - (int) '0';
				ptr++;
			}
			/* printf ("JavaClass:node is NODE%d\n",uretval);*/

			/* get the field name */
			while (*ptr >' ') {ptr++;} ptr++;
			tmp=0;
			/* printf ("getting field name for :%s:\n",ptr);*/
			while (*ptr>' '){ctmp[tmp]=*ptr; tmp++;ptr++;}
			/* printf ("assigning null to index  %d\n",tmp);*/
			ctmp[tmp] = '\0';

			/* printf ("JavaClass:readField, field name :%s:\n",ctmp);*/

			retstr = EAI_GetValue (uretval,ctmp,dtmp);
			/* printf ("JavaClass, retval %s\n",retstr);*/
			send_string(retstr,scriptno);
			free (retstr); /*  malloc'd in ProdCon*/

		} else if (strncmp(ptr,SE,strlen(SE)) == 0) {
			/* printf ("JavaClass:receive_command, JSENDEV\n");*/

			/* skip past the command */
			ptr += strlen(SE) + 1;

			/* this is the script name, or possibly another
			 * field that is USEd. If the second case, then
			 * we have a "direct out", and possibly this event
			 * is not routed */

			/* skip past the perl node number "xx:ddd" to the ptr */
			/* printf ("string here is %s\n",ptr);*/
			rv=sscanf(ptr,"%d:%d",&uretval,&nodeptr);
			while (*ptr >=' ') ptr++; /* now at the ptr */
			ptr++;   /* skip the carriage return */
			/* printf ("now string here is :%s\n",ptr);*/
			/* printf ("JSENDEV, node ptr is %d\n",nodeptr);*/

			/* process this event, then return */
			ptr = processThisClassEvent ((void*)nodeptr,startEntry,endEntry,ptr);
			/* printf ("after processThisClassEvent, string is :%s:\n",ptr);*/

		} else if (strncmp(ptr,GT,strlen(GT)) == 0) {
			/* printf ("JavaClass:receive_command, GETTYPENAME\n");*/
			ptr += strlen(GT) +1;

			/*  get the node id number - equivalent of sscanf*/
			uretval=0;
			while ((*ptr>='0') && (*ptr<='9')) {
				uretval = uretval*10 + (int) *ptr - (int) '0';
				ptr++;
			}
			retstr = EAI_GetTypeName(uretval);
			send_string (retstr ,scriptno);
			free (retstr);

		} else if (strncmp(ptr,CV,strlen(CV)) == 0) {
			/* printf ("JavaClass:receive_command, CREATEVRML\n");*/
			/* printf ("string here is %s\n",ptr);*/
			EOT = strstr(ptr,"\nEOT\n");
			/*  if we do not have a full string yet, we have to do this...*/
			while (EOT == NULL) {
				ClassBuffer = read_EAI_socket(ClassBuffer,&bufcount, &bufsize,
						&(ScriptControl[scriptno].listen_fd));
				EOT = strstr(ClassBuffer,"\nEOT\n");
			}

			*EOT = 0; /*  take off the EOT marker*/
			ptr = ClassBuffer; /*  in case it was realloc'd*/

			/*  do the conversion*/
			ptr += strlen(CV) + 1;
			ra = EAI_CreateVrml("String",ptr,(unsigned int*)nodarr,100);
			ptr = EOT;
			/* printf ("CreateVRML returned %d nodes\n",ra);*/
			if (ra < 0) ra =-1;
			send_int(ra/2,scriptno);
			/*  send frontend name "NODExx" and CNode backends, for each returned node.*/
			/*  the java class code will create a name of xx:CNodeaddr for this variable*/
			for (rb=0; rb<ra; rb++) {
				send_int(nodarr[rb],scriptno);
			}
		} else {
			printf ("JavaClass:receive_command, unknown command: %s\n",ptr);
		}

		/* skip to the end of the command, if possible */
		/* printf ("end of command loop strlen %d\n",strlen(ptr));*/

		while ((*ptr != '\0') && (*ptr != '\n')) ptr ++;
		if (*ptr == '\n') ptr++; /* skip newline */

		if (strlen(ptr) == 0) {
			ClassBuffer[0] = '\0';
			bufcount = 0;
		}
		/* printf ("now, end of command loop strlen %d\n",strlen(ptr));*/
		/* printf ("now, end, ubf if %s\n",ptr);*/
		if (finished_found) {
			/* printf ("we did find a finished; lets exit\n");*/
			/* printf ("JavaClass:END of receive_command, buffer len %d contents %s\n",strlen(ClassBuffer),ClassBuffer);*/

			return;
		}
	}
}

