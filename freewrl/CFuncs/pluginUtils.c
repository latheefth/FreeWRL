/*******************************************************************
 *  Copyright (C) 2004 John Stewart, CRC Canada.
 *  DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
 *  See the GNU Library General Public License (file COPYING in the distribution)
 *  for conditions of use and redistribution.
 **********************************************************************/
                                                                                                                       
#include "headers.h"
#include "Bindable.h"

/* implement Anchor/Browser actions */

int checkIfX3DVRMLFile(char *fn);
void Anchor_ReplaceWorld (char *fn);

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
	
	if (!RUNNINGASPLUGIN) 
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
	printf ("we were successful at locating :%s:\n",filename);

	//which browser are we running under? if we are running as a 
	//plugin, we'll have some of this information already.
	
	if (RUNNINGASPLUGIN) {
		printf ("Anchor, running as a plugin...\n");
	} else {
		if (checkIfX3DVRMLFile(filename)) {
			Anchor_ReplaceWorld (filename);

		} else {
			//printf ("IS NOT a vrml/x3d file\n");
			//printf ("Anchor: -DBROWSER is :%s:\n",BROWSER);
			strcpy (sysline, BROWSER);
			strcat (sysline, " ");
			strcat (sysline, filename);
			strcat (sysline, " &");
			system (sysline);
		}
	}
	free (filename);
}



/*
 * Check to see if the file name is a geometry file.
 * return TRUE if it looks like it is, false otherwise
 *
 * We cant use the firstbytes of fileExists, because, we may be
 * trying to get a whole web-page for reload, and lets let the
 * browser adequately resolve that one.
 *
 * This should be kept in line with the plugin register code in
 * Plugin/netscape/source/npfreewrl.c
 */

int checkIfX3DVRMLFile(char *fn) {
	if ((strstr(fn,".wrl") > 0) ||
		(strstr(fn,".WRL") > 0) ||
		(strstr(fn,".x3d") > 0) ||
		(strstr(fn,".x3dv") > 0) ||
		(strstr(fn,".x3db") > 0) ||
		(strstr(fn,".X3DV") > 0) ||
		(strstr(fn,".X3DB") > 0) ||
		(strstr(fn,".X3D") > 0)) {
		return TRUE;
	}
	return FALSE;
}

/* we are an Anchor, and we are not running in a browser, and we are
 * trying to do an external VRML or X3D world.
 */

void Anchor_ReplaceWorld (char *filename) {
	int tmp;
	struct VRML_Group *rn;
	struct Multi_Node *par;

	rn = (struct VRML_Group *) rootNode;
	par = &(rn->children);

	/* make the old root have ZERO nodes  -well, leave the initial Group {}*/
	par->n = 1;

	/* tell statusbar that we have none */
	viewer_default();
	viewpoint_name_status("NONE");

	/* reset OpenGL positioning */

	/* set the initial viewpoint to 0,0,0... */

	perlParse(FROMURL, filename,TRUE,FALSE,
		rootNode, offsetof (struct VRML_Group, children),&tmp,
		TRUE);
}
