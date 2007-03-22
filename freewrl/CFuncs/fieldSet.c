/*******************************************************************
 Copyright (C) 2006 John Stewart, CRC Canada.
 DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
 See the GNU Library General Public License (file COPYING in the distribution)
 for conditions of use and redistribution.
*********************************************************************/

#include "headers.h"
#include "EAIheaders.h"
#include "jsUtils.h"
#include "jsNative.h"


void getJSMultiNumType (JSContext *cx, struct Multi_Vec3f *tn, int eletype);
void getMFStringtype (JSContext *cx, jsval *from, struct Multi_String *to);
void SetMemory (int type, void *destptr, void *srcptr, int len);

/*******************************************************************

A group of routines to SET a field in memory - in the FreeWRL
scene graph. 

Different methods are used, depending on the format of the call.

*********************************************************************/

/* set a field; used in JavaScript, and in the Parser VRML parser 

	fields are:
		ptr:		pointer to a node (eg, X3D_Box)
		field:		string field name (eg, "size")
		value:		string of value (eg "2 2 2");

This is used mainly in parsing */

void setField_fromJavascript (uintptr_t *ptr, char *field, char *value) {
	int foffset;
	int coffset;
	int ctype;
	int ctmp;

	struct X3D_Node *node;
	struct X3D_Group *group;

	node = (struct X3D_Node *)ptr;
	#ifdef SETFIELDVERBOSE
	printf ("\nsetField_fromJavascript, node %d field %s value %s\n", node, field, value);
	#endif
	
	/* is this a valid field? */
	foffset = findRoutedFieldInFIELDNAMES(node,field,1);	
	if (foffset < 0) {
		printf ("setField_fromJavascript, field %s is not a valid field of a node %s\n",field,stringNodeType(node->_nodeType));
		/* return; */
	}

	/* get offsets for this field in this nodeType */
	#ifdef SETFIELDVERBOSE
	printf ("getting nodeOffsets for type %s field %s value %s\n",stringNodeType(node->_nodeType),field,value); 
	#endif
	findFieldInOFFSETS(NODE_OFFSETS[node->_nodeType], foffset, &coffset, &ctype, &ctmp);

	/* printf ("so, offset is %d, type %d value %s\n",coffset, ctype, value); */

	if (coffset <= 0) {
		printf ("setField_fromJavascript, trouble finding field %s in node %s\n",field,stringNodeType(node->_nodeType));
		printf ("is this maybe a PROTO?? if so, it will be a Group node with __protoDef set to the pointer\n");
		if (node->_nodeType == NODE_Group) {
			group = (struct X3D_Group *)node;
			printf ("it IS a group...\n");
			if (group->__protoDef) {
				printf ("and, this is a PROTO...have to go through PROTO defs to get to it\n");
			}
		}
	}

	if (strlen(value)>0) 
		Parser_scanStringValueToMem(ptr, coffset, ctype, value);
}


/* an incoming EAI/CLASS event has come in, convert the ASCII characters
 * to an internal representation, and act upon it */

unsigned int setField_method2 (char *ptr) {
	unsigned char nt;
	int nodetype;
	uintptr_t nodeptr;
	uintptr_t offset;
	unsigned int scripttype;

	uintptr_t memptr;

	int valIndex;
	struct Multi_Color *tcol;

	int len, elemCount;
	int MultiElement;
	int retint; 			/* used to get return value of sscanf */
	char myBuffer[6000];

	#ifdef SETFIELDVERBOSE
	printf ("\nsetField_method2, string :%s:\n",ptr);
	#endif
	
	/* we have an event, get the data properly scanned in from the ASCII string, and then
		friggin do it! ;-) */

	/* node type */
	while (*ptr==' ')ptr++;
	nt = *ptr; ptr++;
	nodetype = mapEAItypeToFieldType(nt);

	/* blank space */
	ptr++;

	/* nodeptr, offset */
	retint=sscanf (ptr, "%d %d %d",&nodeptr, &offset, &scripttype);
	if (retint != 3) ConsoleMessage ("setField_method2: error reading 3 numbers from the string :%s:\n",ptr);

	while ((*ptr) > ' ') ptr++; 	/* node ptr */
	while ((*ptr) == ' ') ptr++;	/* inter number space(s) */
	while ((*ptr) > ' ') ptr++;	/* node offset */
	while ((*ptr) == ' ') ptr++;	/* inter number space(s) */
	while ((*ptr) > ' ') ptr++;	/* script type */

	#ifdef SETFIELDVERBOSE
		 printf ("EAI_SendEvent, type %d, nodeptr %x offset %x script type %d \n",
				 nodetype,nodeptr,offset, scripttype);
	#endif

	/* We have either a event to a memory location, or to a script. */
	/* the field scripttype tells us whether this is true or not.   */

	memptr = nodeptr+offset;	/* actual pointer to start of destination data in memory */

	/* now, we are at start of data. */


	/* lets go to the first non-blank character in the string */
	while (*ptr == ' ') ptr++;
	#ifdef SETFIELDVERBOSE 
	printf ("EAI_SendEvent, event string now is :%s:\n",ptr);
	#endif

	/* is this a MF node, that has floats or ints, and the set1Value method is called? 	*/
	/* check out the java external/field/MF*java files for the string "ONEVAL "		*/
	if (strcmp("ONEVAL ",ptr) == 0) {
		ptr += 7;

		/* find out which element the user wants to set - that should be the next number */
		while (*ptr==' ')ptr++;
		retint=sscanf (ptr,"%d",&valIndex);
		if (retint != 1) ConsoleMessage ("setField_method2: error reading 1 numbers from the string :%s:\n",ptr);
		while (*ptr>' ')ptr++; /* past the number */
		while (*ptr==' ')ptr++;

		/* lets do some bounds checking here. */
		tcol = (struct Multi_Color *) memptr;
		if (tcol->n <= valIndex) {
			printf ("Error, setting 1Value of %d, max in scenegraph is %d\n",valIndex,tcol->n);
			return FALSE;
		}


		/* if this is a struct Multi* node type, move the actual memory pointer to the data */
		memptr = Multi_Struct_memptr(nodetype, (void *) memptr);

		/* and index into that array; we have the index, and sizes to worry about 	*/
		memptr += valIndex * returnElementLength(nodetype) *  returnElementRowSize(nodetype);

		/* and change the nodetype to reflect this change */
		nodetype = convertToSFType(nodetype);
	}

	/* This switch statement is almost identical to the one in the Javascript
	   code (check out CFuncs/CRoutes.c), except that explicit Javascript calls
	   are impossible here (this is not javascript!) */


	/* convert the ascii string into an internal representation */
	/* this will return '0' on failure */
	len = ScanValtoBuffer(&elemCount, nodetype, ptr , myBuffer, sizeof(myBuffer));


	/* an error in ascii to memory conversion happened */
	if (len == 0) {
		printf ("EAI_SendEvent, conversion failure\n");
		return( -1 );
	}


	MultiElement=FALSE;
	switch (nodetype) {
		case FIELDTYPE_SFBool:
		case FIELDTYPE_SFTime:
		case FIELDTYPE_SFNode:
		case FIELDTYPE_SFInt32:
		case FIELDTYPE_SFFloat:
			  MultiElement = FALSE;  /*Redundant, I hope the compiler will optimize */
			  break;

		case FIELDTYPE_SFVec2f:
		case FIELDTYPE_SFVec3f:
		case FIELDTYPE_SFColor:
		case FIELDTYPE_SFColorRGBA:
		case FIELDTYPE_SFRotation:
			MultiElement=TRUE;
			break;
	        case FIELDTYPE_MFRotation:
	        case FIELDTYPE_MFTime    :
	        case FIELDTYPE_MFInt32   :
	        case FIELDTYPE_MFNode    :
	        case FIELDTYPE_MFVec2f   :
	        case FIELDTYPE_MFVec3f   :
	        case FIELDTYPE_MFColor   :
	        case FIELDTYPE_MFColorRGBA   :
	        case FIELDTYPE_MFFloat   : {
		    MultiElement=TRUE;
		   break;
		}
		case FIELDTYPE_MFString: {
			/* myBuffer will have a full SV structure now, and len will*/
			/* be -1.*/
			break;
		}
		
		case FIELDTYPE_SFImage:
		case FIELDTYPE_SFString: {
			break;
		}
		default: {
                        printf ("unhandled Event :%c: - get code in here\n",nodetype);
			return FALSE;
		}
	}

	if (scripttype) {
	    /* this is a Javascript route, so... */
	    if (MultiElement) {
		switch (nodetype)
		{
		  case FIELDTYPE_MFVec3f:
		  case FIELDTYPE_MFRotation:
		  case FIELDTYPE_MFColor:
		  case FIELDTYPE_MFColorRGBA:
		  case FIELDTYPE_MFFloat: {
		      #ifdef SETFIELDVERBOSE
			printf("EAI_SendEvent, elem %i, count %i, nodeptr %i, off %i, ptr \"%s\".\n",len, elemCount, (int)nodeptr,(int)offset,ptr);
			#endif

		      set_EAI_MFElementtype ((int)nodeptr, (int)offset, (unsigned char *)myBuffer, len);
		      break;
		  }
		  case FIELDTYPE_SFVec2f   :
		  case FIELDTYPE_SFVec3f   :
		  case FIELDTYPE_SFColor   :
		  case FIELDTYPE_SFColorRGBA   :
		  case FIELDTYPE_SFRotation: {
		      Set_one_MultiElementtype ((int)nodeptr, (int)offset,
						myBuffer,len);
		      break;
		  }
		}
	    }else {
		set_one_ECMAtype((int)nodeptr,(int)offset,
				 nodetype, myBuffer,len);
	    }
	    mark_script((int)nodeptr);
	} else {
		/* now, do the memory copy */
		/* if we have a positive len, then, do a straight copy */

		if (len > 0) {
			SetMemory(nodetype,(void *)memptr,(void *)myBuffer,len);
		} else {
			/* if len < 0, it is "wierd". See ScanValtoBuffer
			 * for accurate return values. */
			if (len == -1) {
				/*printf ("EAI_MFSTRING copy over \n");*/
				getEAI_MFStringtype ((struct Multi_String *)myBuffer,
							(struct Multi_String *)memptr);
			}
		}


		/* if this is a geometry, make it re-render.
		   Some nodes (PROTO interface params w/o IS's)
		   will have an offset of zero, and are thus not
		   "real" nodes, only memory locations
		*/

		if (offset > 0) update_node ((void *)nodeptr);

		/* if anything uses this for routing, tell it that it has changed */
		mark_event ((void *)nodeptr,offset);
	}
	return TRUE;
}

void setField_method3(void *tn,unsigned int tptr, char *strp, int fieldType, unsigned len, int extraData, uintptr_t mycx) {
        int ival;
        double tval;
        float fl[4];
        int rv;
	JSContext *scriptContext;

/*
        int route;
        unsigned int fptr;
        void * fn;
*/

	/* not all files know what a JSContext is, so we just pass it around as a uintptr_t type */
	scriptContext = (JSContext *) mycx;

	switch (fieldType) {
		case FIELDTYPE_SFBool:	{	/* SFBool */
			/* printf ("we have a boolean, copy value over string is %s\n",strp); */
			if (strcmp(strp,"true")==0) {
				ival = 1;
			} else {
				/* printf ("ASSUMED TO BE FALSE\n"); */
				ival = 0;
			}
			memcpy ((void *)(tn+tptr), (void *)&ival,len);
			break;
		}

		case FIELDTYPE_SFTime: {
			if (!JS_ValueToNumber(scriptContext, global_return_val,&tval)) tval=0.0;

			/* printf ("SFTime conversion numbers %f from string %s\n",tval,strp); */
			/* printf ("copying to %#x offset %#x len %d\n",tn, tptr,len); */
			memcpy ((void *)(tn+tptr), (void *)&tval,len);
			break;
		}
		case FIELDTYPE_SFNode:
		case FIELDTYPE_SFInt32: {
			rv=sscanf (strp,"%d",&ival);
			if (rv != 1) ConsoleMessage ("setField_method2: error reading 1 numbers from the string :%s:\n",strp);
			/* printf ("SFInt, SFNode conversion number %d\n",ival); */
			memcpy ((void *)((tn+tptr)), (void *)&ival,len);
			break;
		}
		case FIELDTYPE_SFFloat: {
			rv=sscanf (strp,"%f",&fl[0]);
			if (rv != 1) ConsoleMessage ("setField_method2: error reading 1 numbers from the string :%s:\n",strp);
			memcpy ((void *)(tn+tptr), (void *)&fl,len);
			break;
		}

		case FIELDTYPE_SFVec2f: {	/* SFVec2f */
			rv=sscanf (strp,"%f %f",&fl[0],&fl[1]);
			if (rv != 2) ConsoleMessage ("setField_method2: error reading 2 numbers from the string :%s:\n",strp);
			/* printf ("conversion numbers %f %f\n",fl[0],fl[1]); */
			memcpy ((void *)(tn+tptr), (void *)fl,len);
			break;
		}
		case FIELDTYPE_SFVec3f:
		case FIELDTYPE_SFColor: {	/* SFColor */
			rv=sscanf (strp,"%f %f %f",&fl[0],&fl[1],&fl[2]);
			if (rv != 3) ConsoleMessage ("setField_method3: error reading 3 numbers from the string :%s:\n",strp);
			/* printf ("conversion numbers %f %f %f\n",fl[0],fl[1],fl[2]); */
			memcpy ((void *)(tn+tptr), (void *)fl,len);
			break;
		}

		case FIELDTYPE_SFRotation: {
			rv=sscanf (strp,"%f %f %f %f",&fl[0],&fl[1],&fl[2],&fl[3]);
			if (rv != 4) ConsoleMessage ("setField_method3: error reading 3 numbers from the string :%s:\n",strp);
			/* printf ("conversion numbers %f %f %f %f\n",fl[0],fl[1],fl[2],fl[3]); */
			memcpy ((void *)(tn+tptr), (void *)fl,len);
			break;
		}
		case FIELDTYPE_SFImage: {
			/* the string should be saved as an SFImage */
			Parser_scanStringValueToMem(tn, tptr, FIELDTYPE_SFImage, strp);
			break;
		}

		case FIELDTYPE_SFString: {
			struct Uni_String *ms;
			uintptr_t *newptr;

			/* copy the string over, delete the old one, if need be */
			/* printf ("fieldSet SFString, tn %d tptr %d offset from struct %d\n",
				tn, tptr, offsetof (struct X3D_TextureCoordinateGenerator, mode)); */
			newptr = (uintptr_t *)(tn+tptr);
			ms = (struct Uni_String*) *newptr;
			verify_Uni_String (ms,strp);
			break;
		}


			/* a series of Floats... */
		case FIELDTYPE_MFVec3f:
		case FIELDTYPE_MFColor: {getJSMultiNumType (scriptContext, (struct Multi_Vec3f *)(tn+tptr),3); break;}
		case FIELDTYPE_MFFloat: {getJSMultiNumType (scriptContext, (struct Multi_Vec3f *)(tn+tptr),1); break;}
		case FIELDTYPE_MFRotation: {getJSMultiNumType (scriptContext, (struct Multi_Vec3f *)(tn+tptr),4); break;}
		case FIELDTYPE_MFVec2f: {getJSMultiNumType (scriptContext, (struct Multi_Vec3f *)(tn+tptr),2); break;}
		case FIELDTYPE_MFNode: {getMFNodetype (strp,(struct Multi_Node *)(tn+tptr),(struct X3D_Box *)tn,extraData); break;}
		case FIELDTYPE_MFString: {
			getMFStringtype (scriptContext, (jsval *)global_return_val,(struct Multi_String *)(tn+tptr));
			break;
		}

		case FIELDTYPE_MFInt32: {getJSMultiNumType (scriptContext, (struct Multi_Vec3f *)(tn+tptr),0); break;}
		case FIELDTYPE_MFTime: {getJSMultiNumType (scriptContext, (struct Multi_Vec3f *)(tn+tptr),5); break;}

		default: {	printf("WARNING: unhandled from type %s\n", FIELDTYPES[fieldType]);
		printf (" -- string from javascript is %s\n",strp);
		}
	}
}

/* find the ASCII string name of this field of this node */
char *findFIELDNAMESfromNodeOffset(uintptr_t node, int offset) {
	struct X3D_Box *no;
	int* np;
	int nodeType;

	if (node == 0) return "unknown";

	no = (struct X3D_Box *) node;
	nodeType = no->_nodeType;

	
	np = (int *) NODE_OFFSETS[nodeType];  /* it is a const int* type */
	np++;  /* go to the offset field */

	while ((*np != -1) && (*np != offset)) np +=4;
	
	if (*np == -1) return "fieldNotFound";
	
	/* go back to the field name */
	np --;
	return ((char *) FIELDNAMES[*np]);
}




/* go through the generated table X3DACCESSORS, and find the int of this string, returning it, or -1 on error 
	or if it is an "internal" field */
/* XXX:  Do we really need the strlen-call to go next to strcmp?! */
int findFieldInX3DACCESSORS(char *field) {
	int x;
	int mystrlen;
	
	if (field[0] == '_') {
		printf ("findFieldInX3DACCESSORS - internal field %s\n",field);
	}

	mystrlen = strlen(field);
	/* printf ("findFieldInX3DACCESSORS, string :%s: is %d long\n",field,mystrlen);  */
	for (x=0; x<X3DACCESSORS_COUNT; x++) {
		if (strlen(X3DACCESSORS[x]) == mystrlen) {
			if (strcmp(field,X3DACCESSORS[x])==0) return x;
		} 
	}
	return -1;
}

/* go through the generated table FIELDTYPES, and find the int of this string, returning it, or -1 on error 
	or if it is an "internal" field */
int findFieldInFIELDTYPES(char *field) {
	int x;
	int mystrlen;
	
	if (field[0] == '_') {
		printf ("findFieldInFIELDTYPES - internal field %s\n",field);
	}

	mystrlen = strlen(field);
	/* printf ("findFieldInFIELDTYPES, string :%s: is %d long\n",field,mystrlen);  */
	for (x=0; x<FIELDTYPES_COUNT; x++) {
		if (strlen(FIELDTYPES[x]) == mystrlen) {
			if (strcmp(field,FIELDTYPES[x])==0) return x;
		} 
	}
	return -1;
}

/* go through the generated table FIELDTYPES, and find the int of this string, returning it, or -1 on error 
	or if it is an "internal" field */
int findFieldInARR(char* field, const char** arr, size_t cnt)
{
	int x;
	int mystrlen;
	
	if (field[0] == '_') {
		printf ("findFieldInFIELDNAMES - internal field %s\n",field);
	}

	mystrlen = strlen(field);
	/* printf ("findFieldInFIELDNAMES, string :%s: is %d long\n",field,mystrlen);  */
	for (x=0; x!=cnt; ++x) {
		if (strlen(arr[x]) == mystrlen) {
			if (strcmp(field, arr[x])==0) return x;
		} 
	}
	return -1;

}
#define DEF_FINDFIELD(arr) \
 int findFieldIn##arr(char* field) \
 { \
  return findFieldInARR(field, arr, arr##_COUNT); \
 }
DEF_FINDFIELD(FIELDNAMES)
DEF_FINDFIELD(FIELD)
DEF_FINDFIELD(EXPOSED_FIELD)
DEF_FINDFIELD(EVENT_IN)
DEF_FINDFIELD(EVENT_OUT)

/* lets see if this node has a routed field  fromTo  = 0 = from node, anything else = to node */
/* returns the FIELDNAMES index. */
int findRoutedFieldInARR (struct X3D_Node * node, char *field, int fromTo,
 const char** arr, size_t cnt) {
	int retval;
	char mychar[200];
	int a,b,c;

#define FIELDCHECK(fld) \
	if (retval >=0) { \
		findFieldInOFFSETS (NODE_OFFSETS[node->_nodeType], retval,&a,&b,&c); \
		/* did this return any of the ints as != -1? */ \
		/* printf ("     findRoutedField for field %s, nodetype %s is %d\n",fld,stringNodeType(node->_nodeType),a); */ \
		if (a != -1) return retval;  /* found it! */ \
	} 

	/* step 1. try the field as is. */
	retval = findFieldInARR(field, arr, cnt);
	FIELDCHECK (field)

	/* try removing the "set_" or "_changed" */
	/* XXX: Not checking if substring is really "set_" or "_changed"! */
	strncpy (mychar, field, 100);
	if (fromTo != 0) {
		if (strlen(field) > 4)
			retval = findFieldInARR(mychar+4, arr, cnt);
	} else {
		if (strlen(field) > strlen("_changed")) {
			mychar[strlen(field) - strlen("_changed")] = '\0';
			retval = findFieldInARR(mychar, arr, cnt);
		}
	}
	FIELDCHECK (mychar)

	return retval;
}
#define DEF_FINDROUTEDFIELD(arr) \
 int findRoutedFieldIn##arr(struct X3D_Node* node, char* field, int fromTo) \
 { \
  return findRoutedFieldInARR(node, field, fromTo, arr, arr##_COUNT); \
 }
DEF_FINDROUTEDFIELD(FIELDNAMES)
DEF_FINDROUTEDFIELD(EXPOSED_FIELD)
DEF_FINDROUTEDFIELD(EVENT_IN)
DEF_FINDROUTEDFIELD(EVENT_OUT)

/* go through the generated table NODENAMES, and find the int of this string, returning it, or -1 on error */
int findNodeInNODES(char *node) {
	int x;
	int mystrlen;
	
	mystrlen = strlen(node);
	/* printf ("findNodeInNODENAMES, string :%s: is %d long\n",node,mystrlen); */
	for (x=0; x<NODES_COUNT; x++) {
		if (strlen(NODES[x]) == mystrlen) {
			if (strcmp(node,NODES[x])==0) return x;
		} 
	}
	return -1;
}
/* go through the generated table KEYWORDS, and find the int of this string, returning it, or -1 on error */
int findFieldInKEYWORDS(char *field) {
	int x;
	int mystrlen;
	
	mystrlen = strlen(field);
	/* printf ("findFieldInKEYWORDS, string :%s: is %d long\n",field,mystrlen); */
	for (x=0; x<KEYWORDS_COUNT; x++) {
		if (strlen(KEYWORDS[x]) == mystrlen) {
			if (strcmp(field,KEYWORDS[x])==0) return x;
		} 
	}
	return -1;
}

/* go through the generated table FIELDNAMES, and find the int of this string, returning it, or -1 on error */
int findFieldInALLFIELDNAMES(char *field) {
	int x;
	int mystrlen;
	
	mystrlen = strlen(field);
	/* printf ("findFieldInFIELDNAMES, string :%s: is %d long\n",field,mystrlen); */
	for (x=0; x<FIELDNAMES_COUNT; x++) {
		if (strlen(FIELDNAMES[x]) == mystrlen) {
			if (strcmp(field,FIELDNAMES[x])==0) return x;
		} 
	}
	return -1;
}

/* go through the OFFSETS for this node, looking for field, and return offset, type, and kind */
void findFieldInOFFSETS(const int *nodeOffsetPtr, const int field, int *coffset, int *ctype, int *ckind) {
	int *x;

	x = (int *) nodeOffsetPtr;

	#ifdef SETFIELDVERBOSE
	printf ("findFieldInOffsets, comparing %d to %d\n",*x, field); 
	#endif

	while ((*x != field) && (*x != -1)) {
		x += 4;
	}
	if (*x == field) {
		x++; *coffset = *x; x++; *ctype = *x; x++; *ckind = *x; 

		#ifdef SETFIELDVERBOSE
		printf ("found field, coffset %d ctype %d ckind %d\n",*coffset, *ctype, *ckind); 
		#endif

		return;
	}
	if (*x == -1) {
		#ifdef  SETFIELDVERBOSE
		printf ("did not find field %d in OFFSETS\n",field);
		#endif

		*coffset = -1; *ctype = -1, *ckind = -1;
		return;
	}

}



/****************************************************************/
/* a Jscript is returning a Multi-number type; copy this from 	*/
/* the Jscript return string to the data structure within the	*/
/* freewrl C side of things.					*/
/*								*/
/* note - this cheats in that the code assumes that it is 	*/
/* a series of Multi_Vec3f's while in reality the structure	*/
/* of the multi structures is the same - so we "fudge" things	*/
/* to make this multi-purpose.					*/
/* eletype switches depending on:				*/
/* 	0: FIELDTYPE_MFInt32						*/
/* 	1: FIELDTYPE_MFFloat						*/
/* 	2: FIELDTYPE_MFVec2f						*/
/* 	3: FIELDTYPE_MFColor						*/
/* 	4: FIELDTYPE_MFRotation						*/
/*	5: FIELDTYPE_MFTime						*/
/****************************************************************/

void getJSMultiNumType (JSContext *cx, struct Multi_Vec3f *tn, int eletype) {
	float *fl;
	int *il;
	double *dl;

	float f2, f3, f4;
	jsval mainElement;
	int len;
	int i;
	JSString *_tmpStr;
	char *strp;
	int elesize;
	int rv; /* temp for sscanf return vals */


	/* get size of each element, used for MALLOCing memory */
	if (eletype == 0) elesize = sizeof (int);		/* integer */
	else if (eletype == 5) elesize = sizeof (double);	/* doubles. */
	else elesize = sizeof (float)*eletype;			/* 1, 2, 3 or 4 floats per element. */

	/* rough check of return value */
	if (!JSVAL_IS_OBJECT(global_return_val)) {
		printf ("getJSMultiNumType - did not get an object\n");
		return;
	}

	/* printf ("getmultielementtypestart, tn %d %#x dest has  %d size %d\n",tn,tn,eletype, elesize); */

	if (!JS_GetProperty(cx, (JSObject *)global_return_val, "length", &mainElement)) {
		printf ("JS_GetProperty failed for \"length\" in getJSMultiNumType\n");
		return;
	}
	len = JSVAL_TO_INT(mainElement);
	#ifdef SETFIELDVERBOSE
	printf ("getmuiltie length of grv is %d old len is %d\n",len,tn->n);
	#endif

	/* do we have to realloc memory? */
	if (len != tn->n) {
		tn->n = 0;
		/* yep... */
			/* printf ("old pointer %d\n",tn->p); */
		FREE_IF_NZ (tn->p);
		#ifdef SETFIELDVERBOSE 
		printf ("MALLOCing memory for elesize %d len %d\n",elesize,len);
		#endif
		tn->p = (struct SFColor *)MALLOC ((unsigned)(elesize*len));
	}

	/* set these three up, but we only use one of them */
	fl = (float *) tn->p;
	il = (int *) tn->p;
	dl = (double *) tn->p;

	/* go through each element of the main array. */
	for (i = 0; i < len; i++) {
		if (!JS_GetElement(cx, (JSObject *)global_return_val, i, &mainElement)) {
			printf ("JS_GetElement failed for %d in getJSMultiNumType\n",i);
			return;
		}

                _tmpStr = JS_ValueToString(cx, mainElement);
		strp = JS_GetStringBytes(_tmpStr);
                /* printf ("sub element %d is %s as a string\n",i,strp);  */

		switch (eletype) {
		case 0: { rv=sscanf(strp,"%d",il); il++; break;}
		case 1: { rv=sscanf(strp,"%f",fl); fl++; break;}
		case 2: { rv=sscanf (strp,"%f %f",fl,&f2);
			fl++; *fl=f2; fl++; break;}
		case 3: { rv=sscanf (strp,"%f %f %f",fl,&f2,&f3);
			fl++; *fl=f2; fl++; *fl=f3; fl++; break;}
		case 4: { rv=sscanf (strp,"%f %f %f %f",fl,&f2,&f3,&f4);
			fl++; *fl=f2; fl++; *fl=f3; fl++; *fl=f4; fl++; break;}
		case 5: {rv=sscanf (strp,"%lf",dl); dl++; break;}

		default : {printf ("getJSMultiNumType unhandled eletype: %d\n",
				eletype);
			   return;
			}
		}
		#ifdef SETFIELDVERBOSE
		printf ("getJSMultiNumType - got %f %f %f\n",(float *) tn->p,f2,f3);
		#endif

	}
	tn->n = len;
}


/****************************************************************/
/* a script is returning a MFString type; add this to the C	*/
/* children field						*/
/****************************************************************/
void getMFStringtype (JSContext *cx, jsval *from, struct Multi_String *to) {
	int oldlen, newlen;
	jsval _v;
	JSObject *obj;
	int i;
	char *valStr, *OldvalStr;
	struct Uni_String **svptr;
	struct Uni_String **newp, **oldp;
	int count;

	JSString *strval; /* strings */

	oldlen = to->n;
	svptr = to->p;
	newlen=0;

	if (!JS_ValueToObject(cx, (jsval) from, &obj))
		printf ("JS_ValueToObject failed in getMFStringtype\n");

	if (!JS_GetProperty(cx, obj, "length", &_v)) {
		printf ("JS_GetProperty failed for \"length\" in getMFStringtype.\n");
        }

	newlen = JSVAL_TO_INT(_v);

	/*  if we have to expand size of SV... */
	if (newlen > oldlen) {
		oldp = to->p; /* same as svptr, assigned above */
		to->n = newlen;
		to->p = (struct Uni_String**)MALLOC(newlen * sizeof(to->p));
		newp = to->p;

		/* copy old values over */
		for (count = 0; count <oldlen; count ++) {
			/*printf ("copying over element %d\n",count); */
			*newp = *oldp;
			newp++;
			oldp++;
		}

		/* zero new entries */
		for (count = oldlen; count < newlen; count ++) {
			/* make the new SV */
			*newp = (struct Uni_String *)MALLOC (sizeof (struct Uni_String));
			

			/* now, make it point to a blank string */
			*newp = newASCIIString("");
			newp ++;
		}
		FREE_IF_NZ (svptr);
		svptr = to->p;
	}
	/* printf ("verifying structure here\n");
	for (i=0; i<(to->n); i++) {
		printf ("indx %d flag %x string :%s: len1 %d len2 %d\n",i,
				(svptr[i])->sv_flags,
	}
	printf ("done\n");
	*/


	for (i = 0; i < newlen; i++) {
		/* get the old string pointer */
		OldvalStr = svptr[i]->strptr;
		/* printf ("old string at %d is %s len %d\n",i,OldvalStr,strlen(OldvalStr)); */

		/* get the new string pointer */
		if (!JS_GetElement(cx, obj, i, &_v)) {
			fprintf(stderr,
				"JS_GetElement failed for %d in getMFStringtype\n",i);
			return;
		}
		strval = JS_ValueToString(cx, _v);
		valStr = JS_GetStringBytes(strval);

		/* printf ("new string %d is %s\n",i,valStr); */

		/*  if the strings are different... */
		if (strcmp(valStr,OldvalStr) != 0) {
			/* MALLOC a new string, of correct len for terminator */
			svptr[i] =  newASCIIString(valStr);
		}
	}
	/* printf ("\n new structure: %d %d\n",svptr,newlen);
	for (i=0; i<newlen; i++) {
		printf ("indx %d string :%s: len1 %d len2 %d\n",i,
				mypv->xpv_pv, mypv->xpv_cur,mypv->xpv_len);
	}
	*/

	/* JAS 
	myv = INT_TO_JSVAL(1);
	printf ("b setting touched_flag for %d %d\n",cx,obj);

	if (!JS_SetProperty(cx, obj, "__touched_flag", (jsval *)&myv)) {
		fprintf(stderr,
			"JS_SetProperty failed for \"__touched_flag\" in doMFAddProperty.\n");
	}
	*/
}


/************************************************************************/
/* a script is returning a MFNode type; add or remove this to the C	*/
/* children field							*/
/* note params - tn is the address of the actual field, parent is parent*/
/* structure								*/
/************************************************************************/

void getMFNodetype (char *strp, struct Multi_Node *tn, struct X3D_Box *parent, int ar) {
	uintptr_t newptr;
	int newlen;
	char *cptr;
	void *newmal;
	uintptr_t *tmpptr;

	/* is this 64 bit compatible? - unsure right now. */
	if (sizeof(void *) != sizeof (unsigned int))
		printf ("getMFNodetype - unverified that this works on 64 bit machines\n");

	#ifdef SETFIELDVERBOSE 
		printf ("getMFNodetype, %s ar %d\n",strp,ar);
		printf ("getMFNodetype, parent %d has %d nodes currently\n",tn,tn->n);
	#endif

	newlen=0;

	/* this string will be in the form "[ CNode addr CNode addr....]" */
	/* count the numbers to add  or remove */
	if (*strp == '[') { strp++; }
	while (*strp == ' ') strp++; /* skip spaces */
	cptr = strp;

	while (sscanf (cptr,"%d",&newptr) == 1) {
		newlen++;
		/* skip past this number */
		while (isdigit(*cptr) || (*cptr == ',') || (*cptr == '-')) cptr++;
		while (*cptr == ' ') cptr++; /* skip spaces */
	}
	cptr = strp; /* reset this pointer to the first number */

	/* printf ("newlen HERE is %d\n",newlen); */

	/* create the list to send to the AddRemoveChildren function */
	newmal = MALLOC (newlen*sizeof(void *));
	tmpptr = (uintptr_t*)newmal;

	/* scan through the string again, and get the node numbers. */
	while (sscanf (cptr,"%d", (uintptr_t *)tmpptr) == 1) {
		/* printf ("just scanned in %d, which is a %s\n",*tmpptr, 
			stringNodeType(((struct X3D_Node*) (*tmpptr))->_nodeType)); */

		/* skip past this number */
		while (isdigit(*cptr) || (*cptr == ',') || (*cptr == '-')) cptr++;
		while (*cptr == ' ') cptr++; /* skip spaces */
		tmpptr = (uintptr_t*) (tmpptr + sizeof (void *));
	}

	/* now, perform the add/remove */
	AddRemoveChildren (parent, tn, newmal, newlen, ar);
}


/* change a memory loction - do we need to do a simple copy, or
do we need to change a Structure type? */

void SetMemory (int type, void *destptr, void *srcptr, int len) {
	void *newptr;
	struct Multi_Vec3f *mp;


	/* is this a structure? If Multi_Struct_memptr returns a different
	   pointer, than it IS a struct {int n void *p} structure type. */

	/* printf ("start of SetMemory, len %d type %d\n",len,type);  */

	newptr = (void *)Multi_Struct_memptr(type, destptr);
	if (newptr != destptr) {
		mp = (struct Multi_Vec3f*) destptr;
		/* printf ("SetMemory was %d ",mp->n); */
		mp->n=0;
		FREE_IF_NZ(mp->p);
		mp->p = MALLOC(len);
		memcpy (mp->p,srcptr,len);
		mp->n = len /(returnElementLength(type)*returnElementRowSize(type));
		/* printf (" is %d\n ",mp->n); */
	} else {
		memcpy (destptr, srcptr, len);
	}
}



/****************************************************************/
/* a EAI client is returning a MFString type; add this to the C	*/
/* children field						*/
/****************************************************************/
void getEAI_MFStringtype (struct Multi_String *from, struct Multi_String *to) {
	int oldlen, newlen;
	int i;
	char *valStr, *OldvalStr;
	struct Uni_String **oldsvptr;
	struct Uni_String **newsvptr;
	struct Uni_String **newp, **oldp;
	int count;

	/* oldlen = what was there in the first place */
	/*  should be ok verifySVtype(from); */
	#ifdef OLDCODE
	verifySVtype(to);
	#endif

	oldlen = to->n;
	oldsvptr = to->p;
	newlen= from->n;
	newsvptr = from->p;

	/* printf ("old len %d new len %d\n",oldlen, newlen); */

	/* if we have to expand size of SV... */
	if (newlen > oldlen) {

		/* printf ("have to expand...\n"); */
		oldp = to->p; /* same as oldsvptr, assigned above */
		to->n = newlen;
		to->p =(struct Uni_String **) MALLOC(newlen * sizeof(to->p));
		newp = to->p;
		/* printf ("newp is %d, size %d\n",newp, newlen * sizeof(to->p)); */

		/*  copy old values over */
		for (count = 0; count <oldlen; count ++) {
			*newp = *oldp;
			newp++;
			oldp++;
		}

		/*  zero new entries */
		for (count = oldlen; count < newlen; count ++) {
			/* printf ("zeroing %d\n",count); */
			/* make the new SV */
			*newp = (struct Uni_String *)MALLOC (sizeof (struct Uni_String));

			/* now, make it point to a blank string */
			*newp = newASCIIString("");
			newp++;
		}
		FREE_IF_NZ (oldsvptr);
		oldsvptr = to->p;
	}
	/*
	printf ("verifying structure here\n");
	for (i=0; i<(to->n); i++) {
		printf ("indx %d flag %x string :%s: len1 %d len2 %d\n",i,
				(oldsvptr[i])->sv_flags,
	}
	for (i=0; i<(from->n); i++) {
		printf ("NEW indx %d flag %x string :%s: len1 %d len2 %d\n",i,
				(newsvptr[i])->sv_flags,
	}
	printf ("done\n");
	*/


	for (i = 0; i < newlen; i++) {
		/* did the string change ?? */
		verify_Uni_String(oldsvptr[i],newsvptr[i]->strptr);
	}
	/*
	printf ("\n new structure: %d %d\n",oldsvptr,newlen);
	for (i=0; i<newlen; i++) {
	}
	*/
}




/* take an ASCII string from the EAI or CLASS, and convert it into
   a memory block 

   This is not the whole command, just a convert this string to a 
   bunch of floats, or equiv. */

int ScanValtoBuffer(int *quant, int type, char *buf, void *memptr, int bufsz) {
	float *fp;
	int *ip;
	void *tmpbuf;
	int count;
	int len;
	int retint;	/* used for getting sscanf return val */
	
	/* pointers to cast memptr to*/
	float *flmem;

	/* pass in string in buf; memory block is memptr, size in bytes, bufsz */

	#ifdef SETFIELDVERBOSE
	printf("ScanValtoBuffer - memptr %d, buffer %s\n",memptr, buf);
	#endif

	if (bufsz < 10) {
		printf ("cant perform conversion with small buffer\n");
		return (0);
	}

	switch (type) {
	    case FIELDTYPE_SFBool:	{	/* SFBool */
	    	if (strncasecmp(buf,"true",4)==0) {
		    *(int *)memptr = 1;
	    	} else {
		    *(int *)memptr = 0;
	    	}
		len = sizeof(int);
	    	break;
	    }

	    case FIELDTYPE_SFNode:
	    case FIELDTYPE_SFInt32: {
	    	retint=sscanf (buf,"%d",(int *)memptr);
		if (retint != 1) ConsoleMessage ("ScanValtoBuffer: can not read 1 floats from :%s:",buf);
		len = sizeof (int);
	    	break;
	    }
	    case FIELDTYPE_SFFloat: {
	    	retint=sscanf (buf,"%f",(float *)memptr);
		if (retint != 1) ConsoleMessage ("ScanValtoBuffer: can not read 1 floats from :%s:",buf);
		len = sizeof (float);
	    	break;
	    }

	    case FIELDTYPE_SFVec2f: {	/* SFVec2f */
		flmem = (float *)memptr;
	    	retint=sscanf (buf,"%f %f",&flmem[0], &flmem[1]);
		if (retint != 2) ConsoleMessage ("ScanValtoBuffer: can not read 2 floats from :%s:",buf);
		len = sizeof(float) * 2;
	    	break;
	    }

	    case FIELDTYPE_SFVec3f:
	    case FIELDTYPE_SFColor: {	/* SFColor */
		flmem = (float *)memptr;
	    	retint=sscanf (buf,"%f %f %f",&flmem[0],&flmem[1],&flmem[2]);
		len = sizeof(float) * 3;
		if (retint != 3) ConsoleMessage ("ScanValtoBuffer: can not read 3 floats from :%s:",buf);
	    	break;
	    }

	    case FIELDTYPE_SFColorRGBA:
	    case FIELDTYPE_SFRotation: {
		flmem = (float *)memptr;
	    	retint=sscanf (buf,"%f %f %f %f",&flmem[0],&flmem[1],&flmem[2],&flmem[3]);
		if (retint != 4) ConsoleMessage ("ScanValtoBuffer: can not read 4 floats from :%s:",buf);
		len = sizeof(float) * 4;
	    	break;
	    }

	    case FIELDTYPE_SFTime: {
		retint=sscanf (buf, "%lf", (double *)memptr);
		if (retint != 1) ConsoleMessage ("ScanValtoBuffer: can not read 1 floats from :%s:",buf);
		len = sizeof(double);
		break;
	    }

	    case FIELDTYPE_MFNode:
	    case FIELDTYPE_MFInt32:
	    case FIELDTYPE_MFTime:
	    case FIELDTYPE_MFColor:
	    case FIELDTYPE_MFColorRGBA:
	    case FIELDTYPE_MFVec3f:
	    case FIELDTYPE_MFFloat:
	    case FIELDTYPE_MFRotation:
	    case FIELDTYPE_MFVec2f: {
		/* first number is the "number of numbers". */

		/* scan to start of element count, and read it in.*/
		while (*buf==' ') buf++; 
		retint=sscanf (buf,"%d",quant); while (*buf>' ') buf++;
		if (retint != 1) ConsoleMessage ("ScanValtoBuffer: can not read 1 ints from :%s:",buf);

		/* how many elements per row of this type? */
		*quant = *quant * returnElementRowSize(type);

		/* so, right now, quant tells us how many numbers we will read in. */



		/* how long is each element in bytes of this type? */
		len = *quant * returnElementLength(type);

		  #ifdef SETFIELDVERBOSE
			printf ("bufsz is %d, len = %d quant = %d str: %s\n",bufsz, len, *quant,buf);
		  #endif
		  if (len > bufsz) {
			  printf ("Warning, MultiFloat too large, truncating to %d \n",bufsz);
			  len = bufsz;
		  }

		tmpbuf = (float *) MALLOC (len);
		fp = (float *) tmpbuf;
		ip = (int *) tmpbuf;
		for (count = 0; count < *quant; count++) {
			/* scan to start of number */
			while (*buf<=' ') buf++;

			/* scan in number */
			if ((type==FIELDTYPE_MFInt32) || (type==FIELDTYPE_MFNode)) {
				retint=sscanf (buf,"%d",ip);
			} else { 
				retint=sscanf (buf,"%f",fp);
			}
			if (retint != 1) ConsoleMessage ("ScanValtoBuffer: can not read 1 number from :%s:",buf);

			/* go to next number */
			while (*buf >' ') buf++;
			fp ++; ip++;
		}
			
		  /* now, copy over the data to the memory pointer passed in */
		  if(NULL != tmpbuf) memcpy (memptr,tmpbuf,len);
		  else perror("ScanValtoBuffer: tmpbuf NULL!");

		  /* free the memory MALLOC'd in Alberto's code */
		  FREE_IF_NZ (tmpbuf);
		  break;
	    }

	    case FIELDTYPE_MFString: {
		struct Uni_String ** newp;
		struct Multi_String *MFStringptr;
		int thisele, thissize, maxele;	/* used for reading in MFStrings*/


		/* return a Multi_String.						*/
		/*  struct Multi_String { int n; SV * *p; };				*/
		/*  buf will look like:							*/
		/*  	2  0;9:wordg.png  1;12:My"wordg.png				*/
		/*  	where 2 = max elements; 0;9 is element 0, 9 chars long...	*/
		/* OR, if a set1Value:							*/
		/* 	  -1 2:10:xxxxxxxxx						*/
		/* 	  where the 2 is the index.					*/

		MFStringptr = (struct Multi_String *)memptr;

		/* scan to start of element count, and read it in.*/
		while (*buf==' ') buf++;
		retint=sscanf (buf,"%d",&maxele);
		if (retint != 1) ConsoleMessage ("ScanValtoBuffer: can not read 1 number from :%s:",buf);
		while (*buf!=' ') buf++;

		/* is this a set1Value, or a setValue */
		if (maxele == -1) {
			/* is the range ok? for set1Value, we only replace, do not expand. */
			while (*buf==' ') buf++;
			retint=sscanf (buf,"%d;%d",&thisele,&thissize);
			if (retint != 2) ConsoleMessage ("ScanValtoBuffer: can not read 2 number from :%s:",buf);
			/* printf ("this element %d has len %d MFStr size %d \n",thisele,thissize, MFStringptr->n); */

			if (maxele < MFStringptr->n) {
				/* scan to start of string*/
				while (*buf!=':') buf++; buf++;

				/* replace the space at stringln with a 0 */
				buf += thissize; *buf = 0; buf-=thissize;
				MFStringptr->p[thisele] = newASCIIString(buf);

				/* go to end of string */
				buf += thissize+1;
			} else {
				printf ("EAI - warning, MFString set1Value, set %d out of range for array (0-%d)\n",
					maxele,MFStringptr->n);
			}
		} else {	
			/* make (and initialize) this MFString internal representation.*/
			MFStringptr->n = maxele;
			FREE_IF_NZ (MFStringptr->p);

			MFStringptr->p = (struct Uni_String **)MALLOC (maxele * sizeof(MFStringptr->p));
			newp = MFStringptr->p;
	
			/* scan through EAI string, extract strings, etc, etc.*/
			do {
				/* scan to start of element number*/
				/* make the new SV */
	
				while (*buf==' ') buf++;
				retint=sscanf (buf,"%d;%d",&thisele,&thissize);
				if (retint != 2) ConsoleMessage ("ScanValtoBuffer: can not read 2 number from :%s:",buf);
				/*printf ("this element %d has size %d\n",thisele,thissize);*/
	
				/* scan to start of string*/
				while (*buf!=':') buf++; buf++;
	
				/* replace the space at stringln with a 0 */
				buf += thissize; *buf = 0; buf-=thissize;
				MFStringptr->p[thisele] = newASCIIString(buf);

				/* go to end of string */
				buf += thissize+1;
	
				/* scan to next start of string, or end of line*/
				while (*buf==' ') buf++;
	
				/* point to next SV to fill*/
				newp++;
			} while (((int)*buf)>=32);
		}
		/*len = maxele*sizeof(struct Multi_String);*/
		/* return -1 to indicate that this is "wierd".*/
		len = -1;

		break;
	   }

	case FIELDTYPE_SFImage:
	case FIELDTYPE_SFString: {
		int thissize;

		/* save this stuff to a global SV, rather than worrying about memory pointers */
		#ifdef SETFIELDVERBOSE
		printf ("ScanValtoBuffer: FIELDTYPE_SFString, string is %s, ptr %x %d\n",buf,memptr,memptr);
		#endif

		/* strings in the format "25:2 2 1 0xff 0x80 0x80 0xff" where 25 is the length */
		while (*buf == ' ') buf++;

		retint=sscanf (buf,"%d",&thissize);
		if (retint != 1) ConsoleMessage ("ScanValtoBuffer: can not read 1 number from :%s:",buf);
		/* printf ("this SFStr size %d \n",thissize);  */

		/* scan to start of string*/
		while (*buf!=':') buf++; buf++;

		/* replace the space at stringln with a 0 */
		buf += thissize; 
		*buf=0;
		buf -= thissize;
                printf ("do not know where to save this: %s\n",buf); 
		/* newASCIIString(buf); */

		/* return char to a space */
		buf += thissize;
		*buf=' ';

		/*len = maxele*sizeof(struct Multi_String);*/
		len = sizeof (void *);
		break;
	}
		
	  default: {
		printf("WARNING: unhandled CLASS from type %s\n", FIELDTYPES[type]);
		printf ("complain to the FreeWRL team.\n");
		printf ("(string is :%s:)\n",buf);
		return (0);
	    }
	}
	return (len);
}
