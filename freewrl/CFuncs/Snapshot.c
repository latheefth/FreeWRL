/*******************************************************************
 Copyright (C) 2004 John Stewart, CRC Canada.
 DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
 See the GNU Library General Public License (file COPYING in the distribution)
 for conditions of use and redistribution.
*********************************************************************/

#include <unistd.h>
#include <stdio.h>
#include "Bindable.h"

#include "OpenGL_Utils.h"
#include "Viewer.h"
#include "Collision.h"
#include "SensInterps.h"

#include <dirent.h>

/* snapshot stuff */
int snapRawCount=0;
int snapGoodCount=0;
int snapsequence=FALSE;		/* --seq - snapshot sequence, not single click  */
int maxSnapImages=100; 		/* --maximg command line parameter 		*/
int snapGif = FALSE;		/* --gif save as an animated GIF, not mpg	*/
char *snapseqB = NULL;		/* --seqb - snap sequence base filename		*/
char *snapsnapB = NULL;		/* --snapb -single snapshot files		*/
char *seqtmp = NULL;		/* --seqtmp - directory for temp files		*/
int doSnapshot = FALSE;		/* are we doing a snapshot?			*/
void saveSnapSequence();


/* turn snapshotting on; if sequenced; possibly turn off an convert sequence */
void setSnapshot() {
	if (!doSnapshot) {
		doSnapshot = TRUE;
	} else {
		if (snapsequence) {
			doSnapshot = FALSE;
			saveSnapSequence();
		}
	}
}

/* convert a sequence of snaps into a movie */
void saveSnapSequence() {
	char *mytmp, *myseqb;
	char sysline[2000];
	char thisRawFile[2000];
	char thisGoodFile[2000];
	int xx;

	/* make up base names - these may be command line parameters */
	if (snapseqB == NULL)  myseqb  = "freewrl.seq";
	if (seqtmp == NULL)    mytmp   = "freewrl_tmp";

	snapGoodCount++;

	if (snapGif) {
		sprintf (thisGoodFile,"%s/%s.%04d.gif",mytmp,myseqb,snapGoodCount);
	} else {
		sprintf (thisGoodFile,"%s/%s.%04d.mpg",mytmp,myseqb,snapGoodCount);
	}
	sprintf(sysline,"convert -size %dx%d -depth 8 -flip %s/%s*rgb %s",
		screenWidth, screenHeight,mytmp,myseqb,thisGoodFile);

	/* printf ("convert line %s\n",sysline); */

	if (system (sysline) != 0) {
		printf ("Freewrl: error running convert line %s\n",sysline);
	}
	printf ("snapshot is :%s\n",thisGoodFile);
	/* remove temporary files */
	for (xx=1; xx <= snapRawCount; xx++) {
		sprintf (thisRawFile, "%s/%s.%04d.rgb",mytmp,myseqb,xx);
		unlink (thisRawFile);
	}
	snapRawCount=0;
}

/* get 1 frame; convert if we are doing 1 image at a time */
void Snapshot () {
	GLvoid *buffer;
	char sysline[2000];
	FILE * tmpfile;
	FILE * finalfile;
	DIR *mydir;
	char thisRawFile[2000];
	char thisGoodFile[2000];


	char *mytmp, *mysnapb;

	/* make up base names - these may be command line parameters */
	if (snapsequence) { if (snapseqB == NULL)  mysnapb  = "freewrl.seq";
	} else { 	    if (snapsnapB == NULL) mysnapb = "freewrl.snap"; }

	if (seqtmp == NULL)    mytmp   = "freewrl_tmp";

	/* does the directory exist? */
	if ((mydir = opendir(mytmp)) == NULL) {
		mkdir (mytmp,0755);
		if ((mydir = opendir(mytmp)) == NULL) {
			printf ("error opening Snapshot directory %s\n",mytmp);
			return;
		}
	}

	/* are we sequencing, or just single snapping? */
	if (!snapsequence) doSnapshot=FALSE;  	/* reset snapshot key */

	/* malloc 3 bytes per pixel */
	buffer = malloc (3*screenWidth*screenHeight*sizeof(char));
	if (!buffer) {printf ("malloc error in snapshot, exiting \n"); exit(1); }

	/* grab the data */
	glPixelStorei (GL_UNPACK_ALIGNMENT, 1);
	glPixelStorei (GL_PACK_ALIGNMENT, 1);
	glReadPixels (0,0,screenWidth,screenHeight,GL_RGB,GL_UNSIGNED_BYTE, buffer);
	
	/* save this snapshot */
	snapRawCount ++;
	if (snapRawCount > maxSnapImages) {
		free (buffer);
		return;
	}
	
	/* save the file */
	sprintf (thisRawFile,"%s/%s.%04d.rgb",mytmp,mysnapb,snapRawCount);
	tmpfile = fopen(thisRawFile,"w");
	if (tmpfile == NULL) {
		printf ("can not open temp file (%s) for writing\n",thisRawFile);
		free (buffer);
		return;
	}
	
	if (fwrite(buffer, 1, screenHeight*screenWidth*3, tmpfile) <= 0) {
		printf ("error writing snapshot to %s, aborting snapshot\n",thisRawFile);
		free (buffer);
		return;
	}
	fclose (tmpfile);
	
	//convert -size 450x300 -depth 8 -flip /tmp/snappedfile.rgb out.png works.

	free (buffer);

	/* now, if we are doing only 1, convert the raw into the good.... */
	if (!snapsequence) {
		snapGoodCount++;
		sprintf (thisGoodFile,"%s/%s.%04d.png",mytmp,mysnapb,snapGoodCount);
		sprintf(sysline,"convert -size %dx%d -depth 8 -flip %s %s",
		screenWidth, screenHeight,thisRawFile,thisGoodFile);

		if (system (sysline) != 0) {
			printf ("Freewrl: error running convert line %s\n",sysline);
		}
		printf ("snapshot is :%s\n",thisGoodFile);
		unlink (thisRawFile);
	}
}
