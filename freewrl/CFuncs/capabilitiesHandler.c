/*******************************************************************
 Copyright (C) 2007 John Stewart, CRC Canada.
 DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
 See the GNU Library General Public License (file COPYING in the distribution)
 for conditions of use and redistribution.
*********************************************************************/

#include "headers.h"

void handleMetaDataStringString(struct Uni_String *val1, struct Uni_String *val2) {
	#ifdef CAPABILITIESVERBOSE
	printf ("handleMetaDataStringString, :%s:, :%s:\n",val1->strptr, val2->strptr);
	#endif
}

void handleProfile (int myProfile) {
	/* myProfile is a valid profile number - bounds checked before entry */
	#ifdef CAPABILITIESVERBOSE
	printf ("handleProfile, my profile is %s\n",PROFILES[myProfile]);
	#endif
}

void handleComponent (int myComponent, int myLevel) {
	/* myComponent is a valid component number - bounds checked before entry */
	#ifdef CAPABILITIESVERBOSE
	printf ("handleComponent: my Component is %s, level %d\n",COMPONENTS[myComponent], myLevel);
	#endif
}

void handleExport (char *node, char *as) {
	/* handle export statements. as will be either a string pointer, or NULL */
	
	#ifdef CAPABILITIESVERBOSE
	printf ("handleExport: node :%s: ",node);
	if (as != NULL) printf (" AS :%s: ",node);
	printf ("\n");
	#endif
}

void handleImport (char *nodeName,char *nodeImport, char *as) {
	/* handle Import statements. as will be either a string pointer, or NULL */
	
	#ifdef CAPABILITIESVERBOSE
	printf ("handleImport: inlineNodeName :%s: nodeToImport :%s:",nodeName, nodeImport);
	if (as != NULL) printf (" AS :%s: ",as);
	printf ("\n");
	#endif
}


