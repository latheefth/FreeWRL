/*******************************************************************
 *  Copyright (C) 2004 John Stewart, CRC Canada.
 *  DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
 *  See the GNU Library General Public License (file COPYING in the distribution)
 *  for conditions of use and redistribution.
 **********************************************************************/
                                                                                                                       
#include "headers.h"
#include "Bindable.h"

/* implement Anchor/Browser actions */

void doBrowserAction () {
	int count;
	int xx;

	int localNode;
	int tableIndex;

	char *filename;
	char *mypath;
	char *thisurl;
	char *slashindex;
	int flen;
	char firstBytes[4];
	char sysline[1000];

	
	struct Multi_String Anchor_url;
	//struct Multi_String { int n; SV * *p; };
	
	Anchor_url = AnchorsAnchor->url;
	
	printf ("FreeWRL::Anchor: going to \"%s\"\n",
			SvPV(AnchorsAnchor->description,xx));

	filename = malloc(1000);
	
	/* lets make up the path and save it, and make it the global path */
	count = strlen(SvPV(AnchorsAnchor->__parenturl,xx));
	mypath = malloc ((sizeof(char)* count)+1);
	
	if ((!filename) || (!mypath)) {
		printf ("Anchor can not malloc for filename\n");
		exit(1);
	}
	
	/* copy the parent path over */
	strcpy (mypath,SvPV(AnchorsAnchor->__parenturl,xx));
	
	/* and strip off the file name, leaving any path */
	slashindex = (char *)rindex(mypath,'/');
	if (slashindex != NULL) { 
		slashindex ++; /* leave the slash on */
		*slashindex = 0; 
	 } else {mypath[0] = 0;}

	//printf ("Anchor, url so far is %s\n",mypath);

	/* try the first url, up to the last */
	count = 0;
	while (count < Anchor_url.n) {
		thisurl = SvPV(Anchor_url.p[count],xx);

		// is this a local Viewpoint? 
		if (thisurl[0] == '#') {
			localNode = EAI_GetViewpoint(&thisurl[1]);
			tableIndex = -1; 

			for (flen=0; flen<totviewpointnodes;flen++) {
				if (localNode == viewpointnodes[flen]) {
					 tableIndex = flen;
					 break;
				}
			}
			// did we find a match with known Viewpoints?
			if (tableIndex>=0) {
				/* unbind current, and bind this one */
				send_bind_to(VIEWPOINT,
						(void *)viewpointnodes[currboundvpno],0);
				currboundvpno=tableIndex;
				send_bind_to(VIEWPOINT,
						(void *)viewpointnodes[currboundvpno],1);
			} else {
				printf ("failed to match local Viewpoint\n");
			}

			// lets get outa here - jobs done.
			free (filename);
			return;
		}
	
		/* check to make sure we don't overflow */
		if ((strlen(thisurl)+strlen(mypath)) > 900) break;

		/* put the path and the file name together */	
		makeAbsoluteFileName(filename,mypath,thisurl);

		if (fileExists(filename,firstBytes,FALSE)) { break; }
		count ++;
	}
	
	// did we locate that file?
	if (count == Anchor_url.n) {
		if (count > 0) {
			printf ("Could not locate url (last choice was %s)\n",filename);
		}
		free (filename);
		return;
	}
	printf ("we were successful at locating %s\n",filename);

	strcpy (sysline, "mozilla ");
	strcat (sysline, filename);
	strcat (sysline, " &");
	system (sysline);
	free (filename);
	
}
