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

#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

#include <math.h>

#include "headers.h"
#include "EAIheaders.h"
#include <stdlib.h>


#define MURLLEN 2000
#define CLASSVER "JavaClass version 1.0 - www.crc.ca"

/* Function definitions for local functions */
int newClassConnection (int scriptno);
void makeJavaInvocation (char *commandline, int scriptno);
void send_int (int node, int fd);

int JavaClassVerbose = 1;

int fd, lfd;	/* socket descriptors */

/* input buffer */
int bufcount; 
int bufsize;
char *buffer;
int eid = 0; /* event id */

int ClassVerbose = 1;

void send_string (char *string, int fd) ;


/* a new .class file has been called - lets load it */
int newJavaClass(int scriptInvocationNumber,char * nodestr,int *node) {
	char newURL [MURLLEN];
	char *ri;


	/* register this script.... */
	CRoutes_js_new(scriptInvocationNumber, CLASSSCRIPT, 0, 0, 0);

	/* make sure we tell the EAI socket code that this is not opened yet */
	fd = -1;
	lfd = -1;

	/* we have a url; have to make it into a real url for Java.  */
	if ((strncmp("file:",nodestr,5)!=0) &&
		(strncmp("FILE:",nodestr,5)!=0)  &&
		(strncmp("HTTP:",nodestr,5)!=0)  &&
		(strncmp("http:",nodestr,5)!=0)) {

		/* start file off with a file: */
		strcpy (newURL,"file:");

		/* now, is the world relative to cwd, or not? */
		if (BrowserURL[0] == '/') {
			strncat (&newURL[5],BrowserURL,MURLLEN-10);
		} else {
			getcwd (&newURL[5],MURLLEN-10);
			strcat (newURL,"/");
			strncat (newURL,BrowserURL,MURLLEN-100);
		}

		/* now, strip off the wrl filename, and place our name here */
		ri = (char *)rindex((char *)newURL,(int) '/'); 
		*ri = '\0';

		strcat (newURL,"/");
		strcat (newURL,nodestr);
	} else {
		strncpy (newURL,nodestr,MURLLEN-4);
		newURL[MURLLEN-1] = '\0';		/* always terminate */
	}
	if (JavaClassVerbose) printf ("class url is now %s\n",newURL);

	if (!(newClassConnection(scriptInvocationNumber))) return FALSE;

	send_string("NEWSCRIPT", lfd);
	send_string ("SFNode", lfd);
	send_int ((int)node,(int)lfd);
	printf ("newURL :%s:\n",newURL);
	send_string (newURL, lfd);
		
	/* run initialize method */
        eid++;
        send_string("INITIALIZE",lfd);
	send_string ("SFNode", lfd);
	send_int ((int)node,(int)lfd);
	send_int ((int)eid,(int)lfd);
        receive_string();

	/* is this the eventid, or a command? */
	printf ("recieved string is %s\n",buffer);
printf ("newJavaClass returning TRUE\n");
	return TRUE;
}

/* recieve a string from the java class */
int receive_string () {
	int sleepcount;

	printf ("start of receive_string, lfd %d fd %d\n",lfd,fd);

	sleepcount = 1;
	while (bufcount == 0) {
		if (sleepcount>=3000) {
			printf ("FreeWRL Timeout: java class on socket - class problem?\n");
			return FALSE;
		}
		usleep (100000);
		read_EAI_socket(buffer,&bufcount, &bufsize, &lfd);
		printf ("readEAIsocket loop, bufcount %d\n",bufcount);
	}
	return TRUE;
}


/* send a constant string along... */
void send_string (char *string, int fd) {
	strcpy (buffer, string);
	printf ("sending to fd %d, string :%s:\n",lfd,string);
	EAI_send_string(buffer,lfd);
}

/* convert an integer to a string, and send it */
void send_int (int node, int fd) {
	char myintbuf[100];
	sprintf (myintbuf,"%d",node);
	EAI_send_string(myintbuf,fd);
}

/* new class - open file */
int newClassConnection (int scriptno) {
	char commandline[MURLLEN];
	int sleepcount;

	/* allocate memory for input buffer */
	bufcount = 0;
	bufsize = 2 * EAIREADSIZE; // initial size
	buffer = malloc(bufsize * sizeof (char));
	if (buffer == 0) {
		printf ("can not malloc memory for input buffer in create_EAI\n");
		return FALSE;
	}

	printf ("start of newClassConnection, scriptno is %d\n",scriptno);

	/* make the communications socket */
	if (!conEAIorCLASS(scriptno+1, &fd, &lfd)) {
		printf ("could not open CLASS socket for script %d\n",scriptno);
		return FALSE;
	}
	if (JavaClassVerbose) printf ("socket %d lsocket %d\n",fd, lfd);

	/* make the commandline */
	makeJavaInvocation (commandline,scriptno+1);

	/* invoke the java interpreter */
	if (strlen(commandline) <= 0) return FALSE;
	//JAS strcpy (commandline,"/usr/local/bin/java -classpath /usr/local/src/freewrl/FreeWRL-1.07-pre1/java/classes/vrml.jar vrml.FWJavaScript 9878 &");
	if ((system (commandline)) < 0) {
		printf ("error calling %s\n",commandline);
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

	printf ("connection open\n");

	/* get the handshake version */
	if (!receive_string()) return FALSE;

	/* do we have the correct version? */
	if (strncmp(CLASSVER, buffer, strlen(CLASSVER)) != 0) {
		printf ("FreeWRL - JavaClass version prob; expected :%s: got :%s:\n",
			CLASSVER, buffer);
		return FALSE;
	}

	/* throw away buffer contents */
	bufcount = 0;

	/* whew ! done. */
printf ("newClassConnection finished\n");
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
 		printf ("perlpath: %s, builddir %s\n",myPerlInstallDir, BUILDDIR);
 	commandline[0] = '\0';
 
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
 			printf ("FreeWRL can not find vrml.jar\n");
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
 			printf ("FreeWRL can not find java.policy\n");
 			commandline[0] = '\0';
 			return;
 		}
 	}
 	fclose (jPfile);
 
 	/* ok, we have found the jar and policy files... */
 	if (JavaClassVerbose)
 		printf ("found %s and %s\n",vrmlJar,javaPolicy);
 
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
 
 	strcat (commandline,"java -Dfreewrl.lib.dir=");
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
 	sprintf (myc, " vrml.FWJavaScript %d &\n",scriptno+EAIBASESOCKET);
 	strcat (commandline, myc);
 	
 	if (JavaClassVerbose) printf ("command line %s\n",commandline);
}
