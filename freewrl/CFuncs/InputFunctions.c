/*
 * Copyright(C) 2004 John Stewart. CRC Canada.
 * NO WARRANTY. See the license (the file COPYING in the VRML::Browser
 * distribution) for details.
 */

#include "headers.h"
#include "installdir.h"
#define READSIZE 2048


/* find a FreeWRL system file. If FreeWRL is installed, return that path, if not, return the local 
   build path, if not, return NULL */
char *findPathToFreeWRLFile(char *lfn) {
	FILE *tmpfile;
	char sys_fp[200];

	if ((strlen(INSTALLDIR) > (fp_name_len-50)) ||
		(strlen(BUILDDIR) > (fp_name_len-50))) {
		printf ("Internal problem; fp_name_len is not long enough\n");
		return NULL;
	}
	strcpy(sys_fp,INSTALLDIR);
	strcat(sys_fp,lfn);
	/* printf ("checking to see if directory %s exists\n",sys_fp);  */
	tmpfile = fopen(sys_fp,"r");
	if (!tmpfile) {
		/* printf ("FreeWRL not installed; trying build dir copy\n"); */
		strcpy(sys_fp,BUILDDIR);
		strcat(sys_fp,lfn);
		/* printf ("checking to see if directory %s exists\n",sys_fp); */
		tmpfile = fopen(sys_fp,"r");
		if (!tmpfile) {
			/* printf ("NO SYSTEM FONTS FOUND\n"); */
			return NULL;
		} else {
			return (BUILDDIR);
		}

	}
	return (INSTALLDIR);
}


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
	bufsize = 5 * READSIZE; /*  initial size*/
	buffer =(char *)MALLOC(bufsize * sizeof (char));

	/*printf ("start of readInputString, \n\tfile: %s\n\tparent: %s\n",
	 		fn,parent);
	printf ("\tBrowserFullPath: %s\n\n",BrowserFullPath); */

	/* verify (possibly, once again) the file name. This
	 * should have been done already, with the exception of
	 * EXTERNPROTOS; these "sneak" past the normal file reading
	 * channels. This does not matter; if this is a HTTP request,
	 * and is not an EXTERNPROTO, it will already be a local file
	 * */

	/* combine parent and fn to make mynewname */
	/* printf ("before mas, in Input, fn %s\n",fn);*/
	makeAbsoluteFileName(mynewname,parent,fn);

	/* check to see if this file exists */
	FREE_IF_NZ(cacheFileName);
	if (!fileExists(mynewname,firstbytes,TRUE)) {
		ConsoleMessage("problem reading file '%s' ('%s')",fn,mynewname);
		strcpy (buffer,"\n");
		return buffer;
	}

	/* was this a network file that got copied to the local cache? */
	/* printf ("readInputString, cache name %s stack name %s\n",cacheFileName, mynewname); */


	if (((unsigned char) firstbytes[0] == 0x1f) &&
			((unsigned char) firstbytes[1] == 0x8b)) {
		/* printf ("this is a gzipped file!\n");*/
		isTemp = TRUE;
		sprintf (tempname, "%s",tempnam("/tmp","freewrl_tmp"));
		/* first, move this to a .gz file */
		sprintf (sysline, "%s %s %s.gz",COPIER,cacheFileName,tempname);
		freewrlSystem (sysline);

		sprintf (sysline,"%s %s",UNZIP,tempname);
		freewrlSystem (sysline);
		infile = fopen(tempname,"r");
	} else {

		/* ok, now, really read this one. */
		infile = fopen(cacheFileName,"r");
	}

	if ((buffer == 0) || (infile == NULL)){
		ConsoleMessage("problem reading file '%s' (stdio:'%s')",fn,mynewname);
		strcpy (buffer,"\n");
		return buffer;
	}


	/* read in the file */
	do {
		justread = fread (&buffer[bufcount],1,READSIZE,infile);
		bufcount += justread;
		/* printf ("just read in %d bytes\n",justread);*/

		if ((bufsize - bufcount) < READSIZE) {
			/* printf ("HAVE TO REALLOC INPUT MEMORY\n");*/
			bufsize <<=1; 

			buffer =(char *) REALLOC (buffer, (unsigned int) bufsize);
		}
	} while (justread>0);

	buffer[bufcount] = '\0';
	/* printf ("finished read, buffcount %d\n string %s",bufcount,buffer);*/
	fclose (infile);

	if (isTemp) unlink(mynewname);

	return (buffer);
}

