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

	#ifndef AQUA
	/* try the "FONTSDIR" */
	if ((strlen(FONTSDIR) > (fp_name_len-50)) ||
		(strlen(BUILDDIR) > (fp_name_len-50))) {
		printf ("Internal problem; fp_name_len is not long enough\n");
		return NULL;
	}
	strcpy(sys_fp,FONTSDIR);
	strcat(sys_fp,lfn);
	/* printf ("checking to see if directory %s exists\n",sys_fp);  */
	tmpfile = fopen(sys_fp,"r");
	if (tmpfile) {
		fclose(tmpfile);
		return (FONTSDIR);
	}
	#endif


	/* try the "INSTALLDIR" with "/fonts" appended */
	if ((strlen(INSTALLDIR) > (fp_name_len-50)) ||
		(strlen(BUILDDIR) > (fp_name_len-50))) {
		printf ("Internal problem; fp_name_len is not long enough\n");
		return NULL;
	}
	strcpy(sys_fp,INSTALLDIR);
	strcat (sys_fp, "/fonts");
	strcat(sys_fp,lfn);
	/* printf ("checking to see if directory %s exists\n",sys_fp); */
	tmpfile = fopen(sys_fp,"r");
	if (tmpfile) {
		fclose(tmpfile);
		return (INSTALLDIR);
	}

	/* try the "BUILDDIR" with "/fonts" appended */
	if ((strlen(BUILDDIR) > (fp_name_len-50)) ||
		(strlen(BUILDDIR) > (fp_name_len-50))) {
		printf ("Internal problem; fp_name_len is not long enough\n");
		return NULL;
	}
	strcpy(sys_fp,BUILDDIR);
	strcat (sys_fp, "/fonts");
	strcat(sys_fp,lfn);
	/* printf ("checking to see if directory %s exists\n",sys_fp); */
	tmpfile = fopen(sys_fp,"r");
	if (tmpfile) {
		fclose(tmpfile);
		return (INSTALLDIR);
	}

	/* printf - no system fonts found.... */
	return (NULL);
}

/* simulate a command line cp command */
/* we do this, rather than systeming cp, because sometimes we have filenames with spaces, etc, etc 
   and handling this with a system call is a pain */
static void localCopy(char *outFile, char *inFile) {
 FILE *in, *out;
  char ch;

  if((in=fopen(inFile, "rb")) == NULL) {
    ConsoleMessage ("FreeWRL copy: Cannot open input file.");
  }
  if((out=fopen(outFile, "wb")) == NULL) {
    ConsoleMessage ("FreeWRL copy: Cannot open output file.");
  }

  while(!feof(in)) {
    ch = getc(in);
    if(ferror(in)) {
      ConsoleMessage ("FreeWRL copy: input error.");
      clearerr(in);
      break;
    } else {
      if(!feof(in)) putc(ch, out);
      if(ferror(out)) {
        ConsoleMessage ("FreeWRL copy: output error.");
        clearerr(out);
        break;
      }
    }
  }
  fclose(in);
  fclose(out);

  return;
}
/* read a file, put it into memory. */
char * readInputString(char *fn) {
	char *buffer;
	int bufcount;
	int bufsize;
	FILE *infile;
	int justread;
	char mynewname[1000];
	char tempname[1000];
	char sysline[1000];
	char firstbytes[20];
	int isTemp = FALSE;
	int removeIt = FALSE;

	bufcount = 0;
	bufsize = 5 * READSIZE; /*  initial size*/
	buffer =(char *)MALLOC(bufsize * sizeof (char));

	/*printf ("start of readInputString, \n\tfile: %s\n",
	 		fn);
	printf ("\tBrowserFullPath: %s\n\n",BrowserFullPath); */

	/* verify (possibly, once again) the file name. This
	 * should have been done already, with the exception of
	 * EXTERNPROTOS; these "sneak" past the normal file reading
	 * channels. This does not matter; if this is a HTTP request,
	 * and is not an EXTERNPROTO, it will already be a local file
	 * */

	/* printf ("before mas, in Input, fn %s\n",fn); */
	strcpy (mynewname, fn);

	/* check to see if this file exists */
	if (!fileExists(mynewname,firstbytes,TRUE,&removeIt)) {
		ConsoleMessage("problem reading file '%s' ('%s')",fn,mynewname);
		strcpy (buffer,"\n");
		return buffer;
	}

	/* was this a network file that got copied to the local cache? */

	if (((unsigned char) firstbytes[0] == 0x1f) &&
			((unsigned char) firstbytes[1] == 0x8b)) {
		int len;

		/* printf ("this is a gzipped file!\n"); */
		isTemp = TRUE;
		sprintf (tempname, "%s.gz",tempnam("/tmp","freewrl_tmp"));

		/* first, move this to a .gz file */
		localCopy(tempname, mynewname);

		/* now, unzip it */
		/* remove the .gz from the file name - ".gz" is 3 characters */
		len = strlen(tempname); tempname[len-3] = '\0';
		sprintf (sysline,"%s %s",UNZIP,tempname);

		freewrlSystem (sysline);
		infile = fopen(tempname,"r");
	} else {

		/* ok, now, really read this one. */
		infile = fopen(mynewname,"r");
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

		if ((bufsize - bufcount-10) < READSIZE) {
			/* printf ("HAVE TO REALLOC INPUT MEMORY\n");*/
			bufsize <<=1; 

			buffer =(char *) REALLOC (buffer, (unsigned int) bufsize);
		}
	} while (justread>0);

	/* make sure we have a carrage return at the end - helps the parser find the end of line. */
	buffer[bufcount] = '\n'; bufcount++;
	buffer[bufcount] = '\0';
	/* printf ("finished read, buffcount %d\n string %s",bufcount,buffer); */
	fclose (infile);

	if (isTemp) UNLINK(tempname);
	if (removeIt) UNLINK (mynewname);

	return (buffer);
}

