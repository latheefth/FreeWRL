/* 
 * Copyright(C) 2004 John Stewart. CRC Canada.
 * NO WARRANTY. See the license (the file COPYING in the VRML::Browser
 * distribution) for details.
 */

#include "Structs.h"
#include "headers.h"
#define READSIZE 2048
#define VRML2HEADER "#VRML V2.0 utf8"
#define X3DHEADER  "<\?xml version"
#define VRMLFILE 1
#define X3DFILE 2
#define UNKNOWNFILE 0

void VRMLPreParse(char *buffer);

/* read a file, put it into memory. */
char * readInputString(char *fn, char *parent) {
	char *buffer;
	int bufcount;
	int bufsize;
	FILE *infile;
	int justread;
	char mynewname[1000];
	char tempname[1000];
	char sysline[1000];
	char firstbytes[20];
	int isTemp;

	isTemp = FALSE;
	bufcount = 0;
	bufsize = 5 * READSIZE; // initial size
	buffer = malloc(bufsize * sizeof (char));

	//printf ("start of readInputString, \n\tfile: %s\n\tparent: %s\n",
	//		fn,parent);
	//printf ("\tBrowserURL: %s\n\n",BrowserURL);

	/* verify (possibly, once again) the file name. This
	 * should have been done already, with the exception of
	 * EXTERNPROTOS; these "sneak" past the normal file reading
	 * channels. This does not matter; if this is a HTTP request,
	 * and is not an EXTERNPROTO, it will already be a local file
	 * */

	/* combine parent and fn to make mynewname */
	//printf ("before mas, in Input, fn %s\n",fn);
	makeAbsoluteFileName(mynewname,parent,fn);

	/* check to see if this file exists */
	if (!fileExists(mynewname,firstbytes,TRUE)) {
		printf ("problem reading file step1 %s\n",fn);
		strcpy (buffer,"\n");
		return buffer;
	} 

	if (((unsigned char) firstbytes[0] == 0x1f) &&
			((unsigned char) firstbytes[1] == 0x8b)) {
		//printf ("this is a gzipped file!\n");
		isTemp = TRUE;
		sprintf (tempname, "%s",tempnam("/tmp","freewrl_tmp"));
		sprintf (sysline,"%s <%s >%s",UNZIP,mynewname,tempname);
		system (sysline);
		strcpy (mynewname,tempname);
	}

	/* ok, now, really read this one. */
	infile = fopen(mynewname,"r");

	if ((buffer == 0) || (infile == NULL)){
		printf ("problem reading file step2 %s\n",fn);
		strcpy (buffer,"\n");
		return buffer;
	}


	/* save the file name, in case something in perl wants it */
	if (lastReadFile != NULL) free(lastReadFile);
	lastReadFile = malloc(strlen(mynewname)+2);
	strncpy(lastReadFile,mynewname,strlen(mynewname)+1);



	do {
		justread = fread (&buffer[bufcount],1,READSIZE,infile);
		bufcount += justread;
		//printf ("just read in %d bytes\n",justread);
	
		if ((bufsize - bufcount) < READSIZE) {
			//printf ("HAVE TO REALLOC INPUT MEMORY\n");
			bufsize += READSIZE;
			buffer = realloc (buffer, (unsigned int) bufsize);
		}
	} while (justread>0);

	buffer[bufcount] = '\0';
	//printf ("finished read, buffcount %d\n string %s",bufcount,buffer);
	fclose (infile);

	if (isTemp) unlink(mynewname);

	return (buffer);
}


/* kind of make sure string is ok... */
char *sanitizeInputString (char *buffer) {
	int inputtype;


	/* now, what kind of file is this? */
	inputtype = UNKNOWNFILE;
	if (strncmp (buffer,VRML2HEADER,sizeof(VRML2HEADER)-1) == 0) {
		//printf ("this is a VRML V2 file\n");
		inputtype = VRMLFILE;
	} else if (strncmp (buffer, X3DHEADER, sizeof(X3DHEADER)-1) == 0) {
		//printf ("this is an X3D file\n");
		inputtype = X3DFILE;
	}

	/* hmmm - we could not tell by the header; lets see if we can find
	 * anything else of interest */
	if (inputtype == UNKNOWNFILE) {
		if (strstr (buffer,"<Scene>") != NULL) {
			//printf ("Scene found, its x3d\n");
			inputtype = X3DFILE;
		} else if (strstr (buffer,"<X3D") != NULL) {
			//printf ("Scene found, its x3d\n");
			inputtype = X3DFILE;
		}

		/* oh well, assume its a VRML file */
		else {inputtype = VRMLFILE;}
	}

	/* pre-parse this; maybe remove comments, etc, etc */
	if (inputtype == VRMLFILE) {
		VRMLPreParse(buffer);
	}

	return (buffer);
}
		

void VRMLPreParse(char *buffer) {
	int cptr;
	int maxptr;
	int inquotes;

	//cptr = strlen(VRML2HEADER); /* leave the header intact, for now */
	cptr = 0;
	inquotes = FALSE;
	maxptr = strlen(buffer);
	

	/* go through the memory */
	while (cptr < maxptr) {
		/* determine whether this is within quotes or not */
		if (buffer[cptr] == '"') {
			//printf ("found a quote start at %d\n",cptr);
			if (buffer[cptr-1] != '\\') {
				inquotes = !inquotes;
			}
			//printf ("so, inquotes = %d\n",inquotes);
		}

		if ((!inquotes) & (buffer[cptr]=='#')) {
			//printf ("comment starts at %d\n",cptr);
			while (((buffer[cptr]&0xff) >= ' ') || (buffer[cptr] == '\t')) {
				buffer[cptr] = ' ';
				cptr ++;
				//printf ("char is %x\n",(buffer[cptr]&0xff));
			}
		}

		cptr ++;
	}
}
