/* function protos */

#include "Eai_C.h"

/* get a node pointer */
X3D_Node *X3D_getNode (char *name) {
	char *ptr;
	uintptr_t adr;
	int oldPerl;
	X3D_Node *retval;

	retval = malloc (sizeof(X3D_Node));
	retval->type = FIELDTYPE_SFNode;
	retval->X3D_SFNode.SFNodeType = '\0';

	retval->X3D_SFNode.adr = 0;

	/* get the node address. ignore the old perl pointer, save the address. */
	ptr = _X3D_make1StringCommand(GETNODE,name);
	
	if (sscanf (ptr,"%d %lu",&oldPerl, &adr) != 2) {
		printf ("error getting %s\n",name);
	} else {
		#ifdef VERBOSE
		printf ("X3D_getNode, ptr %s\n",ptr);
		printf ("oldPerl %d adr %p\n",oldPerl, adr);
		#endif

		if (adr == 0) {
			printf ("node %s does not exist\n",name);
		}

		
		retval->X3D_SFNode.adr = (uintptr_t *)adr;
	}
	REMOVE_EOT
	return retval;
}

/* get an eventIn */

X3D_EventIn *_X3D_getEvent(X3D_Node *node, char *name, int into) {
	char *ptr;
	uintptr_t origPtr;
	int offset;
	int nds;
	X3D_EventIn *retval;
	uintptr_t *adr;

        retval = malloc (sizeof (struct _intX3D_EventIn));
	retval->offset = 0;
	retval->nodeptr = 0;
	retval->datasize = 0;
	retval->field = NULL;
	retval->scripttype = 0;

	if((node->type != FIELDTYPE_SFNode) && (node->type != FIELDTYPE_MFNode)) {
		printf ("X3D_getEvent, expected a node, got a %s\n",FIELDTYPES[node->type]);
		exit (1);
	}

	if (node->type == FIELDTYPE_SFNode) {
		adr = node->X3D_SFNode.adr;
	} else {
		if (node->X3D_MFNode.n != 1) {
			printf ("warning - will only get event for first node = have %d nodes\n",node->X3D_MFNode.n);
		}
		/* get the pointer to the memory for stored SFNode addresses... */
		adr = ((uintptr_t *)node->X3D_MFNode.p);
		/* get the first entry in this list */
		adr = (uintptr_t *) *adr;
	}

	/* printf ("getting eventin for address %d, field %s\n",adr, name); */
	if (into) ptr = _X3D_Browser_SendEventType(adr, name, "eventIn");
	else ptr = _X3D_Browser_SendEventType(adr, name, "eventOut");

	/* ptr should point to , for example: 161412616 116 0 q 0 eventIn
	   where we have 
			newnodepoiner,
			 field offset, 
			node datasize,
			field type (as an ascii char - eg 'q' for X3D_MFNODE
			ScriptType,
			and whether this is an eventIn or not.
	printf ("getEvent: ptr is %s\n",ptr);
	*/

	/* eg: ptr is 161412616 116 0 q 0 eventIn */
	if (sscanf(ptr,"%ld %d %d", &origPtr, &offset, &nds) != 3) {
		printf ("error in getEventIn\n");
		return retval;
	}

	/* do the pointers match? they should.. */
	if (origPtr !=  (uintptr_t )adr) {
		printf ("error in getEventIn, origptr and node ptr do not match\n");
	}

	/* save the first 3 fields */
	retval->nodeptr = origPtr;
	retval->offset = offset;
	retval->datasize = nds;

	/* go to the data type. will be a character, eg 'q' */
	SKIP_CONTROLCHARS	/* should now be at the copy of the memory pointer */
	SKIP_IF_GT_SPACE	/* should now be  at end of memory pointer */
	SKIP_CONTROLCHARS	/* should now be at field offset */
	SKIP_IF_GT_SPACE	/* should now be at end of field offset */
	SKIP_CONTROLCHARS	/* should now be at the nds parameter */
	SKIP_IF_GT_SPACE	/* should now be at ehd of the nds parameter */
	SKIP_CONTROLCHARS	/* should now be at start of the type */


	retval->datatype = mapEAItypeToFieldType(*ptr);
	SKIP_IF_GT_SPACE
	SKIP_CONTROLCHARS

	/* should be sitting at field type */
	if (sscanf (ptr,"%d",&(retval->scripttype)) != 1) {
		printf ("error reading scriptttype in getEventIn\n");
		return retval;
	}

	SKIP_IF_GT_SPACE
	SKIP_CONTROLCHARS

	if (into) {
		if (strncmp(ptr,"eventIn",strlen("eventIn")) != 0) 
			if (strncmp(ptr,"exposedField",strlen("exposedField")) != 0) 
				printf ("WARNING: expected for field %s: eventIn or exposedField, got %s\n",name,ptr);
	} else {
		if (strncmp(ptr,"eventOut",strlen("eventOut")) != 0) 
			if (strncmp(ptr,"exposedField",strlen("exposedField")) != 0) 
				printf ("WARNING: expected for field %seventOut or exposedField, got %s\n",name,ptr);
	}


	retval->field = strdup(name);
	
	REMOVE_EOT
    return retval;
}


X3D_EventIn *X3D_getEventIn(X3D_Node *node, char *name) {
	X3D_EventIn *retval;
	retval = _X3D_getEvent(node, name,TRUE);
	return retval;
}

X3D_EventOut *X3D_getEventOut(X3D_Node *node, char *name) {
	X3D_EventOut *retval;
	retval = _X3D_getEvent(node, name,FALSE);
	return retval;

}

void X3D_addRoute (X3D_EventOut *from, X3D_EventIn *to) {
	char myline[200];
	char *ptr;
	sprintf (myline,"%ld %s %ld %s",from->nodeptr,from->field,to->nodeptr,to->field);
	ptr = _X3D_make1StringCommand(ADDROUTE,myline);
}

void X3D_deleteRoute (X3D_EventOut *from, X3D_EventIn *to) {
	char myline[200];
	char *ptr;
	sprintf (myline,"%ld %s %ld %s",from->nodeptr,from->field,to->nodeptr,to->field);
	ptr = _X3D_make1StringCommand(DELETEROUTE,myline);
}


void X3D_setValue (X3D_EventIn *dest, X3D_Node *node) {
	char myline[2048];
	int count;
	uintptr_t *ptr;
	
	/* sanity check */
	if (dest->datatype != node->type) {
		printf ("X3D_setValue mismatch: event type %s, value type %s\n", 
				FIELDTYPES[(int)dest->datatype], FIELDTYPES[node->type]);
		return;
	}

	switch (dest->datatype) {
		default:
		printf ("XXX - setValue, not implemented yet for type '%s'\n",FIELDTYPES[dest->datatype]);
		return;

		case FIELDTYPE_SFVec3f:
		case FIELDTYPE_SFColor:
			sprintf (myline, "%c %ld %d %d %f %f %f\n",
				mapFieldTypeToEAItype(dest->datatype),
				dest->nodeptr, dest->offset, dest->scripttype,
				node->X3D_SFVec3f.c[0],
				node->X3D_SFVec3f.c[1],
				node->X3D_SFVec3f.c[2]);
			_X3D_sendEvent (SENDEVENT,myline);
		break;

			
		case FIELDTYPE_MFNode:
			#ifdef VERBOSE
			printf ("sending in %d nodes\n",node->X3D_MFNode.n);
			#endif

			ptr = ((uintptr_t *)node->X3D_MFNode.p);
			for (count = 0; count < node->X3D_MFNode.n; count ++) {
				/* printf ("adding in ptr %d, adr %d\n",*ptr,*ptr); */

				sprintf (myline,"%ld %d %s %ld\n",
					dest->nodeptr,
					dest->offset,
					dest->field,
					*ptr);

				_X3D_sendEvent (SENDCHILD,myline);
				ptr++;
			}

		break;

	}

}


/*****************************************************************************/

char *descrip = NULL;

void X3D_setDescription(char *newDesc) {
	/* description is held locally. */
	if (descrip != NULL) 
		free (descrip);
	descrip  = strdup(newDesc);
}

char *X3D_getDescription() {

	/* description is held locally. */
	if (descrip == NULL) {
		descrip = strdup("in X3D");
	}
	return descrip;
}

char *X3D_getName() {
	char *ptr;
	ptr = strdup(_X3D_makeShortCommand(GETNAME));
	REMOVE_EOT
	return ptr;
}
char *X3D_getVersion() {
	char *ptr;
	ptr = strdup(_X3D_makeShortCommand(GETVERSION));
	REMOVE_EOT
	return ptr;
}
char *X3D_getWorldURL() {
	char *ptr;
	ptr = strdup(_X3D_makeShortCommand(GETURL));
	REMOVE_EOT
	return ptr;
}



float X3D_getCurrentSpeed() {
	char *ptr;
	float curspeed;
	ptr = _X3D_makeShortCommand(GETCURSPEED);
	if (sscanf(ptr,"%f",&curspeed) == 0) {
		printf ("client, error - problem reading float from %s\n",ptr);
		exit(0);
	}
	return curspeed;
}


float X3D_getCurrentFrameRate() {
	char *ptr;
	float curframe;
	ptr = _X3D_makeShortCommand(GETFRAMERATE);
	if (sscanf(ptr,"%f",&curframe) == 0) {
		printf ("client, error - problem reading float from %s\n",ptr);
		exit(0);
	}
	return curframe;
}

X3D_Node *X3D_createVrmlFromString(char *str) {
	X3D_Node *retval;
	char *ptr;
	int retvals;
	int count;
	uintptr_t *mytmp;
	
        retval = malloc (sizeof(X3D_Node));
	retval->type = FIELDTYPE_MFNode;
	retval->X3D_MFNode.n = 0;

	#ifdef VERBOSE
	printf ("X3D_createVrmlFromString  - string %s\n",str);
	#endif

	ptr = _X3D_make2StringCommand(CREATEVS,str,"\nEOT\n");
	
	#ifdef VERBOSE
	printf ("return pointer is %s\n",ptr);
	#endif

	/* now, how many numbers did it return? ignore the (obsolete) perl pointers */
	retvals = _X3D_countWords(ptr);
	retval->X3D_MFNode.p = malloc (retvals * sizeof (uintptr_t*));
	mytmp = ((uintptr_t *)retval->X3D_MFNode.p); /* for putting values in */
	retval->X3D_MFNode.n = retvals/2;

	for (count = 0; count < (retvals/2); count++) {
		/* skip to perlpointer */
		SKIP_CONTROLCHARS

		/* skip over perlpointer */
		SKIP_IF_GT_SPACE

		/* skip to the memory pointer */
		SKIP_CONTROLCHARS

		/* read in the memory pointer */
		sscanf (ptr,"%lu",mytmp); /* changed for 1.18.15 JAS */
		mytmp++;

		/* skip past this number now */
		SKIP_IF_GT_SPACE
	}
	#ifdef VERBOSE
	printf ("X3D_createVrmlFromString, found %d pointers, they are:\n",retval->X3D_MFNode.n);
	mytmp = retval->X3D_MFNode.p;
	for (count=0; count<retval->X3D_MFNode.n; count++) {
		printf ("	(ptr %d) %d\n",mytmp,*mytmp);
		mytmp++;
	}
	printf ("(end oflist)\n");
	#endif
	return retval;	
}

