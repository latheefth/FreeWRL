/*******************************************************************
 *  Copyright (C) 2004 John Stewart, CRC Canada.
 *  DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
 *  See the GNU Library General Public License (file COPYING in the distribution)
 *  for conditions of use and redistribution.
 **********************************************************************/
                                                                                                                       
#include "headers.h"

/* implement Anchor/Browser actions */

void doBrowserAction () {
//	printf ("doBrowserAction, working on :%s:\n",BrowserActionString);

	/* nothing to do; too little information */
//	if (strlen (BrowserActionString) <=1) return;

	/* load the new url, if we can */

//	if (BrowserActionString[0] == '#') {
//		printf ("local viewpoint found\n");
//	}
//
//		/* go through the URL field, sanitize them, and return the string
//		   to let Browser.pm parse/load/etc the files */
//		for (counter = 0; counter <node->url.n; counter++) {
//			urlptr = SvPV((node->url.p[counter]),urllen);
//			
//			/* not a blank entry? */
//			if (urllen > 0) {
//				printf ("do_Anchor, have to check if file exists\n");
//
//				/* remove whitespace of front */
//				while (*urlptr <= ' ') urlptr++;
//			
//				/* can we put this on the return string? */
//				if ((strlen(urlptr)+strlen(AnchorString)) < ASLEN-2) {
//					strcat (AnchorString,urlptr);
//				}
//	
//				BrowserAction = TRUE;
//			}
//		}
//
//
}
