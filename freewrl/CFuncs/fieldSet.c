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
void saveSFImage (struct X3D_PixelTexture *node, char *str);
void SetMemory (int type, void *destptr, void *srcptr, int len);

/*******************************************************************

A group of routines to SET a field in memory - in the FreeWRL
scene graph. 

Different methods are used, depending on the format of the call.

*********************************************************************/

/* used for reading in SVs */
SV *sv_global_tmp;


/* set a field; used in JavaScript, and in the Parser VRML parser 

	fields are:
		ptr:		pointer to a node (eg, X3D_Box)
		field:		string field name (eg, "size")
		value:		string of value (eg "2 2 2");

This is used mainly in parsing */

void setField_method1 (void *ptr, char *field, char *value) {
	int foffset;
	int coffset;
	int ctype;
	int ctmp;

	struct X3D_Node *node;
	struct X3D_Group *group;

	node = (struct X3D_Box *)ptr;

	#ifdef SETFIELDVERBOSE
	printf ("\nsetField_method1, node %d field %s value %s\n", node, field, value);
	#endif
	
	/* is this a valid field? */
	foffset = findFieldInALLFIELDNAMES(field);	
	if (foffset < 0) {
		printf ("setField_method1, field %s is not a valid field of a node %s\n",field,stringNodeType(node->_nodeType));
		/* return; */
	}

	/* get offsets for this field in this nodeType */
	#ifdef SETFIELDVERBOSE
	printf ("getting nodeOffsets for type %s field %s value %s\n",stringNodeType(node->_nodeType),field,value); 
	#endif
	findFieldInOFFSETS(NODE_OFFSETS[node->_nodeType], foffset, &coffset, &ctype, &ctmp);

	/* printf ("so, offset is %d, type %d value %s\n",coffset, ctype, value); */

	if (coffset <= 0) {
		printf ("setField_method1, trouble finding field %s in node %s\n",field,stringNodeType(node->_nodeType));
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
	unsigned char nodetype;
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
	nodetype = *ptr; ptr++;

	/* blank space */
	ptr++;

	/* nodeptr, offset */
	retint=sscanf (ptr, "%d %d %d",&nodeptr, &offset, &scripttype);
	while ((*ptr) > ' ') ptr++; 	/* node ptr */
	while ((*ptr) == ' ') ptr++;	/* inter number space(s) */
	while ((*ptr) > ' ') ptr++;	/* node offset */
	while ((*ptr) == ' ') ptr++;	/* inter number space(s) */
	while ((*ptr) > ' ') ptr++;	/* script type */

	#ifdef SETFIELDVERBOSE
		 printf ("EAI_SendEvent, type %c, nodeptr %x offset %x script type %d \n",
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
	if (strncmp("ONEVAL ",ptr,7) == 0) {
		ptr += 7;

		/* find out which element the user wants to set - that should be the next number */
		while (*ptr==' ')ptr++;
		retint=sscanf (ptr,"%d",&valIndex);
		while (*ptr>' ')ptr++; /* past the number */
		while (*ptr==' ')ptr++;

		/* lets do some bounds checking here. */
		tcol = (struct Multi_Color *) memptr;
		if (tcol->n <= valIndex) {
			printf ("Error, setting 1Value of %d, max in scenegraph is %d\n",valIndex,tcol->n);
			return FALSE;
		}


		/* if this is a struct Multi* node type, move the actual memory pointer to the data */
		memptr = Multi_Struct_memptr(nodetype-EAI_SFUNKNOWN, (void *) memptr);

		/* and index into that array; we have the index, and sizes to worry about 	*/
		memptr += valIndex * returnElementLength(nodetype-EAI_SFUNKNOWN) *  returnElementRowSize(nodetype-EAI_SFUNKNOWN);

		/* and change the nodetype to reflect this change */
		switch (nodetype) {
			case EAI_MFNODE: nodetype = EAI_SFNODE; break;
			case EAI_MFINT32: nodetype = EAI_SFINT32; break;
			case EAI_MFTIME: nodetype = EAI_SFTIME; break;
			case EAI_MFCOLOR: nodetype = EAI_SFCOLOR; break;
			case EAI_MFCOLORRGBA: nodetype = EAI_SFCOLORRGBA; break;
			case EAI_MFVEC3F: nodetype = EAI_SFVEC3F; break;
			case EAI_MFFLOAT: nodetype = EAI_SFFLOAT; break;
			case EAI_MFROTATION: nodetype = EAI_SFROTATION; break;
			case EAI_MFVEC2F: nodetype = EAI_SFVEC2F; break;
			default: {printf ("EAI input, ONEVAL set but type unknown %d\n",nodetype);
				  return FALSE;
				}
		}
	}

	/* This switch statement is almost identical to the one in the Javascript
	   code (check out CFuncs/CRoutes.c), except that explicit Javascript calls
	   are impossible here (this is not javascript!) */


	/* convert the ascii string into an internal representation */
	/* this will return '0' on failure */
	len = ScanValtoBuffer(&elemCount, nodetype - EAI_SFUNKNOWN, ptr , myBuffer, sizeof(myBuffer));


	/* an error in ascii to memory conversion happened */
	if (len == 0) {
		printf ("EAI_SendEvent, conversion failure\n");
		return( -1 );
	}


	MultiElement=FALSE;
	switch (nodetype) {
		case EAI_SFBOOL:
		case EAI_SFTIME:
		case EAI_SFNODE:
		case EAI_SFINT32:
		case EAI_SFFLOAT: {
			  MultiElement = FALSE;  /*Redundant, I hope the compiler will optimize */
			  break;
		} /* these are all ok, just continue on */

		case EAI_SFVEC2F:
	  	case EAI_SFVEC3F:
	  	case EAI_SFCOLOR:
	  	case EAI_SFCOLORRGBA:
		case EAI_SFROTATION: {
			MultiElement=TRUE;
			break;
		}
	        case EAI_MFROTATION:
	        case EAI_MFTIME    :
	        case EAI_MFINT32   :
	        case EAI_MFNODE    :
	        case EAI_MFVEC2F   :
	        case EAI_MFVEC3F   :
	        case EAI_MFCOLOR   :
	        case EAI_MFCOLORRGBA   :
	        case EAI_MFFLOAT   : {
		    MultiElement=TRUE;
		   break;
		}
		case EAI_MFSTRING: {
			/* myBuffer will have a full SV structure now, and len will*/
			/* be -1.*/
			break;
		}
		
		case EAI_SFIMAGE:
		case EAI_SFSTRING: {
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
		  case EAI_MFVEC3F:
		  case EAI_MFROTATION:
		  case EAI_MFCOLOR:
		  case EAI_MFCOLORRGBA:
		  case EAI_MFFLOAT: {
		      #ifdef SETFIELDVERBOSE
			printf("EAI_SendEvent, elem %i, count %i, nodeptr %i, off %i, ptr \"%s\".\n",len, elemCount, (int)nodeptr,(int)offset,ptr);
			#endif

		      set_EAI_MFElementtype ((int)nodeptr, (int)offset, (unsigned char *)myBuffer, len);
		      break;
		  }
		  case EAI_SFVEC2F   :
		  case EAI_SFVEC3F   :
		  case EAI_SFCOLOR   :
		  case EAI_SFCOLORRGBA   :
		  case EAI_SFROTATION: {
		      Set_one_MultiElementtype ((int)nodeptr, (int)offset,
						myBuffer,len);
		      break;
		  }
		}
	    }else {
		set_one_ECMAtype((int)nodeptr,(int)offset,
				 nodetype-EAI_SFUNKNOWN, myBuffer,len);
	    }
	    mark_script((int)nodeptr);
	} else {
		/* now, do the memory copy */
		/* if we have a positive len, then, do a straight copy */

		if (len > 0) {
			SetMemory(nodetype-EAI_SFUNKNOWN,(void *)memptr,(void *)myBuffer,len);
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
		case SFBOOL:	{	/* SFBool */
			/* printf ("we have a boolean, copy value over string is %s\n",strp); */
			if (strncmp(strp,"true",4)==0) {
				ival = 1;
			} else {
				/* printf ("ASSUMED TO BE FALSE\n"); */
				ival = 0;
			}
			memcpy ((void *)(tn+tptr), (void *)&ival,len);
			break;
		}

		case SFTIME: {
			if (!JS_ValueToNumber(scriptContext, global_return_val,&tval)) tval=0.0;

			/* printf ("SFTime conversion numbers %f from string %s\n",tval,strp); */
			/* printf ("copying to %#x offset %#x len %d\n",tn, tptr,len); */
			memcpy ((void *)(tn+tptr), (void *)&tval,len);
			break;
		}
		case SFNODE:
		case SFINT32: {
			rv=sscanf (strp,"%d",&ival);
			/* printf ("SFInt, SFNode conversion number %d\n",ival); */
			memcpy ((void *)((tn+tptr)), (void *)&ival,len);
			break;
		}
		case SFFLOAT: {
			rv=sscanf (strp,"%f",&fl[0]);
			memcpy ((void *)(tn+tptr), (void *)&fl,len);
			break;
		}

		case SFVEC2F: {	/* SFVec2f */
			rv=sscanf (strp,"%f %f",&fl[0],&fl[1]);
			/* printf ("conversion numbers %f %f\n",fl[0],fl[1]); */
			memcpy ((void *)(tn+tptr), (void *)fl,len);
			break;
		}
		case SFVEC3F:
		case SFCOLOR: {	/* SFColor */
			rv=sscanf (strp,"%f %f %f",&fl[0],&fl[1],&fl[2]);
			/* printf ("conversion numbers %f %f %f\n",fl[0],fl[1],fl[2]); */
			memcpy ((void *)(tn+tptr), (void *)fl,len);
			break;
		}

		case SFROTATION: {
int tmp;
tmp =
			rv=sscanf (strp,"%f %f %f %f",&fl[0],&fl[1],&fl[2],&fl[3]);
			/* printf ("conversion numbers %f %f %f %f\n",fl[0],fl[1],fl[2],fl[3]); */
			memcpy ((void *)(tn+tptr), (void *)fl,len);
			break;
		}
		case SFIMAGE: {
			saveSFImage ((struct X3D_PixelTexture*) tn, strp);
			break;
		}


			/* a series of Floats... */
		case MFVEC3F:
		case MFCOLOR: {getJSMultiNumType (scriptContext, (struct Multi_Vec3f *)(tn+tptr),3); break;}
		case MFFLOAT: {getJSMultiNumType (scriptContext, (struct Multi_Vec3f *)(tn+tptr),1); break;}
		case MFROTATION: {getJSMultiNumType (scriptContext, (struct Multi_Vec3f *)(tn+tptr),4); break;}
		case MFVEC2F: {getJSMultiNumType (scriptContext, (struct Multi_Vec3f *)(tn+tptr),2); break;}
		case MFNODE: {getMFNodetype (strp,(struct Multi_Node *)(tn+tptr),(struct X3D_Box *)tn,extraData); break;}
		case MFSTRING: {
			getMFStringtype (scriptContext, (jsval *)global_return_val,(struct Multi_String *)(tn+tptr));
			break;
		}

		case MFINT32: {getJSMultiNumType (scriptContext, (struct Multi_Vec3f *)(tn+tptr),0); break;}
		case MFTIME: {getJSMultiNumType (scriptContext, (struct Multi_Vec3f *)(tn+tptr),5); break;}

		default: {	printf("WARNING: unhandled from type %s\n", 
			FIELD_TYPE_STRING(fieldType));
		printf (" -- string from javascript is %s\n",strp);
		}
	}
}

/* find the ASCII string name of this field of this node */
char *findFIELDNAMESfromNodeOffset(uintptr_t node, int offset) {
	struct X3D_Box *no;
	int* np;
	int nodeType;
	char *ptr;

	if (node == 0) return "unknown";

	no = (struct X3D_Box *) node;
	nodeType = no->_nodeType;

	np = NODE_OFFSETS[nodeType];
	np++;  /* go to the offset field */

	while ((*np != -1) && (*np != offset)) np +=4;
	
	if (*np == -1) return "fieldNotFound";
	
	/* go back to the field name */
	np --;
	return FIELDNAMES[*np];
}


/* go through the generated table FIELDNAMES, and find the int of this string, returning it, or -1 on error 
	or if it is an "internal" field */
int findFieldInFIELDNAMES(char *field) {
	int x;
	int mystrlen;
	
	if (field[0] == '_') {
		printf ("findFieldInFIELDNAMES - internal field %s\n",field);
	}

	mystrlen = strlen(field);
	/* printf ("findFieldInFIELDNAMES, string :%s: is %d long\n",field,mystrlen);  */
	for (x=0; x<FIELDNAMES_COUNT; x++) {
		if (strlen(FIELDNAMES[x]) == mystrlen) {
			if (strcmp(field,FIELDNAMES[x])==0) return x;
		} 
	}
	return -1;
}

/* lets see if this node has a routed field  fromTo  = 0 = from node, anything else = to node */
int findRoutedFieldInFIELDNAMES (char *field, int fromTo) {
	int retval;
	char mychar[200];
	int tmp;

	retval = findFieldInFIELDNAMES(field);
	if (retval == -1) {
		/* try removing the "set_" or "_changed" */
		strncpy (mychar, field, 100);
		if (fromTo != 0) {
			if (strlen(field) > 4)
				retval = findFieldInFIELDNAMES(&mychar[4]);
		} else {
			if (strlen(field) > strlen("_changed")) {
				mychar[strlen(field) - strlen("_changed")] = '\0';
				retval = findFieldInFIELDNAMES(mychar);
			}
		}


	}

	return retval;
}

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

	x = nodeOffsetPtr;
	/* printf ("findFieldInOffsets, comparing %d to %d\n",*x, field); */
	while ((*x != field) && (*x != -1)) {
		x += 4;
	}
	if (*x == field) {
		/* printf ("found field!\n"); */
		x++; *coffset = *x; x++; *ctype = *x; x++; *ckind = *x; 
		return;
	}
	if (*x == -1) {
		printf ("did not find field %d in OFFSETS\n",field);
		*coffset = -1; *ctype = -1, *ckind = -1;
		return;
	}
}

int countCommas (char *instr) {
	int retval = 0;
	while (*instr != '\0') {
		if (*instr == ',') retval++;
		instr++;
	}
	return retval;
}

/* called effectively by VRMLCU.pm */
void Parser_scanStringValueToMem(void *ptr, int coffset, int ctype, char *value) {
	int datasize;
	int commaCount;

	char *nst;                      /* used for pointer maths */
	void *mdata;
	int *iptr;
	float *fptr;
	SV **svptr;
	SV *mysv;
	int tmp;
	int myStrLen;
	

	/* temporary for sscanfing */
	float fl[4];
	int in[4];
	uintptr_t inNode[4];
	double dv;
	char mytmpstr[20000];

	/* printf ("PST, for %s we have %s strlen %d\n",FIELD_TYPE_STRING(ctype), value, strlen(value)); */
	nst = (char *) ptr; /* should be 64 bit compatible */
	nst += coffset;

	datasize = returnElementLength(ctype);
	commaCount = countCommas(value);
	
	switch (ctype) {
		case SFFLOAT: {sscanf (value,"%f",fl); memcpy (nst,fl,datasize); break;} 

		case SFBOOL:
		case SFINT32:
			{ sscanf (value,"%d",in); 
				memcpy(nst,in,datasize); 
				/* SFNODES need to have the parent field linked in */
				if (ctype == SFNODE) {
					add_parent((void *)in[0], ptr); 
				}
				
			break;}
		case FREEWRLPTR:
		case SFNODE: { 
				sscanf (value,"%d",inNode); 
				/* if (inNode[0] != 0) {
					printf (" andof type %s\n",stringNodeType(((struct X3D_Box *)inNode[0])->_nodeType));
				} */
				memcpy(nst,inNode,datasize); 
				/* SFNODES need to have the parent field linked in */
				if (ctype == SFNODE) {
					add_parent((void *)inNode[0], ptr); 
				}
				
			break;}
		case SFVEC2F:
			{sscanf (value,"%f,%f",&fl[0],&fl[1]); memcpy (nst,fl,datasize*2); break;}
		case SFROTATION:
		case SFCOLORRGBA:
			{sscanf (value,"%f,%f,%f,%f",&fl[0],&fl[1],&fl[2],&fl[3]); memcpy (nst,fl,datasize*4); break;}
		case SFVEC3F:
		case SFCOLOR:
			{ sscanf (value,"%f,%f,%f",&fl[0],&fl[1],&fl[2]); memcpy (nst,fl,datasize*3); break;}
		case MFBOOL:
		case MFINT32: {
			mdata = malloc ((commaCount+1) * datasize);
			iptr = (int *)mdata;
			for (tmp = 0; tmp < (commaCount+1); tmp++) {
				sscanf(value, "%d",iptr);
				iptr ++;
				/* skip past the number and trailing comma, if there is one */
				if (*value == '-') value++;
				while (*value>='0') value++;
				if ((*value == ' ') || (*value == ',')) value++;
			}
			((struct Multi_Node *)nst)->p=mdata;
			((struct Multi_Node *)nst)->n = commaCount+1;
			break;
			}

		case MFNODE: {
			for (tmp = 0; tmp < (commaCount+1); tmp++) {
				sscanf(value, "%d",inNode);
				addToNode(ptr,coffset,(void *)inNode[0]); 
				/* printf ("MFNODE, have to add child %d to parent %d\n",inNode[0],ptr);  */
				add_parent((void *)inNode[0], ptr); 
				/* skip past the number and trailing comma, if there is one */
				if (*value == '-') value++;
				while (*value>='0') value++;
				if ((*value == ' ') || (*value == ',')) value++;
			}
			break;
			}
		case SFTIME: { sscanf (value, "%lf", &dv); 
				/* printf ("SFtime, for value %s has %lf datasize %d\n",value,dv,datasize); */
				memcpy (nst,&dv,datasize);
			break; }

		case MFROTATION:
		case MFCOLOR:
		case MFFLOAT:
		case MFTIME:
		case MFVEC2F:
		case MFVEC3F:
		case MFCOLORRGBA: {
			/* printf ("data size is %d elerow %d commaCount %d\n",datasize, returnElementRowSize(ctype),commaCount+1); */
			mdata = malloc ((commaCount+1) * datasize);
			fptr = (float *)mdata;
			for (tmp = 0; tmp < (commaCount+1); tmp++) {
				sscanf(value, "%f",fptr);
				fptr ++;
				/* skip past the number; checking to see if it has a decimal point or not */
				while ((*value != '\0') && (*value != ' ') && (*value != ',')) value++;

				/* skip to the beginning of the next number */
				if ((*value == ' ') || (*value == ',')) value++;
			}
			((struct Multi_Node *)nst)->p=mdata;
			((struct Multi_Node *)nst)->n = (commaCount+1)/returnElementRowSize(ctype);
			break;
			}

		case SFSTRING: 
		case SFIMAGE: {
			mysv  = EAI_newSVpv(value); 
			memcpy (nst,&mysv,sizeof(SV *));
			break; }
			
		case MFSTRING: {
			/* string comes in from VRMLCU.pm as:
				1:8:MODULATE,3:ADD 
			   where 1: is the last element (ie, 2 elements)
				 8: is the length of the first string,
				 3: is the length of the next string */

			/* get a new count of elements */
			sscanf (value,"%d",&commaCount);
			while ((*value >=0) && (*value<='9')) value++;
			if (*value == ':') value++;
		
			mdata = malloc ((commaCount+1) * datasize);
			svptr = (SV **)mdata;

			for (tmp = 0; tmp < (commaCount+1); tmp++) {
				sscanf(value, "%d",&myStrLen);

				if (myStrLen>20000) myStrLen = 19000;

				while (*value<='9') value++;
				if (*value == ':') value++;

				strncpy (mytmpstr,value,myStrLen);
				mytmpstr[myStrLen] = '\0';
				value += myStrLen;
				*svptr = EAI_newSVpv(mytmpstr);

				svptr ++;
				if (*value == ',') value++;
			}
			((struct Multi_Node *)nst)->p=mdata;
			((struct Multi_Node *)nst)->n = commaCount+1;
			break;
			}

		default: {
printf ("Unhandled PST, %s: value %s, ptrnode %s nst %d offset %d numelements %d\n",
	FIELD_TYPE_STRING(ctype),value,stringNodeType(((struct X3D_Box *)ptr)->_nodeType),nst,coffset,commaCount+1);
			break;
			};
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
/* 	0: MFINT32						*/
/* 	1: MFFLOAT						*/
/* 	2: MFVEC2F						*/
/* 	3: MFCOLOR						*/
/* 	4: MFROTATION						*/
/*	5: MFTIME						*/
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


	/* get size of each element, used for mallocing memory */
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
	printf ("getmuiltie length of grv is %d old len is %d\n",len,tn->n);

	/* do we have to realloc memory? */
	if (len != tn->n) {
		tn->n = 0;
		/* yep... */
			/* printf ("old pointer %d\n",tn->p); */
		if (tn->p != NULL) free (tn->p);
		#ifdef CRVERBOSE 
		printf ("mallocing memory for elesize %d len %d\n",elesize,len);
		#endif
		tn->p = (struct SFColor *)malloc ((unsigned)(elesize*len));
		if (tn->p == NULL) {
			printf ("can not malloc memory in getJSMultiNumType\n");
			return;
		}
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
printf ("getJSMultiNumType - got %f %f %f\n",(float *) tn->p,f2,f3);

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
	SV **svptr;
	SV **newp, **oldp;
	int myv;
	int count;
	struct xpv *mypv;

	JSString *strval; /* strings */

	/* oldlen = what was there in the first place */
	verifySVtype(to);

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
		to->p = (SV**)malloc(newlen * sizeof(to->p));
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
			*newp = (SV*)malloc (sizeof (struct STRUCT_SV));
			(*newp)->sv_flags = SVt_PV | SVf_POK;
			(*newp)->sv_refcnt=1;
			mypv = (struct xpv *)malloc(sizeof (struct xpv));
			(*newp)->sv_any = mypv;

			/* now, make it point to a blank string */
			(*mypv).xpv_pv = (char *)malloc (2);
			strcpy((*mypv).xpv_pv ,"");
			(*mypv).xpv_cur = 0;
			(*mypv).xpv_len = 1;
			newp ++;
		}
		free (svptr);
		svptr = to->p;
	}
	/* printf ("verifying structure here\n");
	for (i=0; i<(to->n); i++) {
		printf ("indx %d flag %x string :%s: len1 %d len2 %d\n",i,
				(svptr[i])->sv_flags,
				 SvPVX(svptr[i]), SvCUR(svptr[i]), SvLEN(svptr[i]));
	}
	printf ("done\n");
	*/


	for (i = 0; i < newlen; i++) {
		/* get the old string pointer */
		OldvalStr = SvPVX(svptr[i]);
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
		if (strncmp(valStr,OldvalStr,strlen(valStr)) != 0) {
			/* now Perl core dumps since this is the wrong thread, so lets do this
			 ourselves: sv_setpv(svptr[i],valStr); */

			/* get a pointer to the xpv to modify */
			mypv = (struct xpv *)SvANY(svptr[i]);

			/* free the old string */
			free (mypv->xpv_pv);

			/* malloc a new string, of correct len for terminator */
			mypv->xpv_pv =(char *) malloc (strlen(valStr)+2);

			/* copy string over */
			strcpy (mypv->xpv_pv, valStr);

			/* and tell us that it is now longer */
			mypv->xpv_len = strlen(valStr)+1;
			mypv->xpv_cur = strlen(valStr)+0;
		}
	}
	/* printf ("\n new structure: %d %d\n",svptr,newlen);
	for (i=0; i<newlen; i++) {
		printf ("indx %d string :%s: len1 %d len2 %d\n",i,
				mypv->xpv_pv, mypv->xpv_cur,mypv->xpv_len);
				 SvPVX(svptr[i]), SvCUR(svptr[i]), SvLEN(svptr[i]));
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

	#ifdef CRVERBOSE 
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

	/* create the list to send to the AddRemoveChildren function */
	newmal = malloc (newlen*sizeof(void *));
	tmpptr = (uintptr_t*)newmal;

	if (newmal == 0) {
		printf ("cant malloc memory for addChildren");
		return;
	}


	/* scan through the string again, and get the node numbers. */
	while (sscanf (cptr,"%d", (int *)tmpptr) == 1) {
		/* skip past this number */
		while (isdigit(*cptr) || (*cptr == ',') || (*cptr == '-')) cptr++;
		while (*cptr == ' ') cptr++; /* skip spaces */
		tmpptr = (uintptr_t*) (tmpptr + sizeof (void *));
	}

	/* now, perform the add/remove */
	AddRemoveChildren (parent, tn, newmal, newlen, ar);
}


/******************************************************************

saveSFImage - a PixelTexture is being sent back from a script, save it!
	It comes back as a string; have to put it in as a SV.

*********************************************************************/
void saveSFImage (struct X3D_PixelTexture *node, char *str) {
	char *strptr;
	STRLEN xx;
	int thissize;
	struct xpv *mypv;
	SV *newSV;
	SV *oldSV;

	thissize = strlen(str);

	/* make the new SV */
	newSV = (SV*)malloc (sizeof (struct STRUCT_SV));
	(newSV)->sv_flags = SVt_PV | SVf_POK;
	(newSV)->sv_refcnt=1;
	mypv = (struct xpv *)malloc(sizeof (struct xpv));
	/* printf ("just mallocd for mypv, it is %d and size %d\n", mypv, sizeof (struct xpv)); */
	(newSV)->sv_any = mypv;

	/* fill in the SV values...copy the string over... */
	(*mypv).xpv_pv = (char *)malloc (thissize+2);
	strncpy((*mypv).xpv_pv ,str,thissize+1);
	(*mypv).xpv_cur = thissize-1;    /* size without term */
	(*mypv).xpv_len = thissize;      /* size with termination */

	/* switcheroo, image now is this new SV */
	oldSV = node->image;
	node->image = newSV;

	/* remove the old one... */
	if ((SvFLAGS(oldSV) & SVf_POK) == 0) {
		/*printf ("saveSFImage this one is going to fail\n"); */
	} else {
		/* printf ("saveSFImage passed test, lets decode it\n"); */
		/* ok - we have a valid Perl pointer, go for it. */
		strptr = (char *)SvPV(node->image,xx);
		/* printf ("saveSFImage PixelTexture string was %s\n",strptr); */

		/* free the old "parts" of this... */
		/* have to REALLY look at this - might be a memory leak */
		/*free( oldSV->sv_any); */
		/*free (oldSV); */
	}
}


/* change a memory loction - do we need to do a simple copy, or
do we need to change a Structure type? */

void SetMemory (int type, void *destptr, void *srcptr, int len) {
	void *newptr;
	struct Multi_Vec3f *mp;
	SV *svptr;


	/* is this a structure? If Multi_Struct_memptr returns a different
	   pointer, than it IS a struct {int n void *p} structure type. */

	/* printf ("start of SetMemory, len %d type %d\n",len,type);  */

	newptr = (void *)Multi_Struct_memptr(type, destptr);
	if (newptr != destptr) {
		mp = (struct Multi_Vec3f*) destptr;
		/* printf ("SetMemory was %d ",mp->n); */
		mp->n=0;
		FREE_IF_NZ(mp->p);
		mp->p = malloc(len);
		memcpy (mp->p,srcptr,len);
		mp->n = len /(returnElementLength(type)*returnElementRowSize(type));
		/* printf (" is %d\n ",mp->n); */
	} else {

		/* is this a straight copy, or a setting of an SV * ? */
		if ((type == SFSTRING)  || (type == SFIMAGE)) {
			/* printf ("SetMemory - this is an SV *\n"); */
                        svptr = (SV *)destptr;
			/* printf ("dest was a SV of type %x at %x\n",SvTYPE((SV *)svptr->sv_any),svptr->sv_any); */
			svptr->sv_any = sv_global_tmp;
			/* printf ("dest now is a SV of type %xat %x\n",SvTYPE((SV *)svptr->sv_any),svptr->sv_any); */
			
		} else {
			/* this is a straight copy */
			memcpy (destptr, srcptr, len);
		}
	}
}



/****************************************************************/
/* a EAI client is returning a MFString type; add this to the C	*/
/* children field						*/
/****************************************************************/
void getEAI_MFStringtype (struct Multi_String *from, struct Multi_String *to) {
	int oldlen, newlen;
	jsval _v;
	int i;
	char *valStr, *OldvalStr;
	SV **oldsvptr;
	SV **newsvptr;
	SV **newp, **oldp;
	int myv;
	int count;
	struct xpv *mypv;

	/* oldlen = what was there in the first place */
	/*  should be ok verifySVtype(from); */
	verifySVtype(to);

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
		to->p =(SV **) malloc(newlen * sizeof(to->p));
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
			*newp = (SV *)malloc (sizeof (struct STRUCT_SV));
			(*newp)->sv_flags = SVt_PV | SVf_POK;
			(*newp)->sv_refcnt=1;
			mypv = (struct xpv *)malloc(sizeof (struct xpv));
			(*newp)->sv_any = mypv;

			/* now, make it point to a blank string */
			(*mypv).xpv_pv =(char *) malloc (2);
			strcpy((*mypv).xpv_pv ,"");
			(*mypv).xpv_cur = 0;
			(*mypv).xpv_len = 1;
			newp++;
		}
		free (oldsvptr);
		oldsvptr = to->p;
	}
	/*
	printf ("verifying structure here\n");
	for (i=0; i<(to->n); i++) {
		printf ("indx %d flag %x string :%s: len1 %d len2 %d\n",i,
				(oldsvptr[i])->sv_flags,
				 SvPVX(oldsvptr[i]), SvCUR(oldsvptr[i]), SvLEN(oldsvptr[i]));
	}
	for (i=0; i<(from->n); i++) {
		printf ("NEW indx %d flag %x string :%s: len1 %d len2 %d\n",i,
				(newsvptr[i])->sv_flags,
				 SvPVX(newsvptr[i]), SvCUR(newsvptr[i]), SvLEN(newsvptr[i]));
	}
	printf ("done\n");
	*/


	for (i = 0; i < newlen; i++) {
		/*  get the old string pointer */
		OldvalStr = SvPVX(oldsvptr[i]);
		/* printf ("old string at %d is %s len %d\n",i,OldvalStr,strlen(OldvalStr)); */

		valStr = SvPVX(newsvptr[i]);

		/* printf ("new string %d is %s len %d\n",i,valStr,strlen(valStr)); */

		/* if the strings are different... */
		if (strncmp(valStr,OldvalStr,strlen(valStr)) != 0) {
			/* now Perl core dumps since this is the wrong thread, so lets do this
			 ourselves: sv_setpv(oldsvptr[i],valStr); */

			/* get a pointer to the xpv to modify */
			mypv = (struct xpv *)SvANY(oldsvptr[i]);

			/* free the old string */
			free (mypv->xpv_pv);

			/* malloc a new string, of correct len for terminator */
			mypv->xpv_pv = (char *)malloc (strlen(valStr)+2);

			/* copy string over */
			strcpy (mypv->xpv_pv, valStr);

			/* and tell us that it is now longer */
			mypv->xpv_len = strlen(valStr)+1;
			mypv->xpv_cur = strlen(valStr)+0;
		}
	}
	/*
	printf ("\n new structure: %d %d\n",oldsvptr,newlen);
	for (i=0; i<newlen; i++) {
		printf ("indx %d string :%s: len1 %d len2 %d\n",i,
				mypv->xpv_pv, mypv->xpv_cur,mypv->xpv_len);
				 SvPVX(oldsvptr[i]), SvCUR(oldsvptr[i]), SvLEN(oldsvptr[i]));
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

	#ifdef EAIVERBOSE
	printf("ScanValtoBuffer - buffer %s\n",buf);
	#endif

	if (bufsz < 10) {
		printf ("cant perform conversion with small buffer\n");
		return (0);
	}

	switch (type) {
	    case SFBOOL:	{	/* SFBool */
	    	if (strncasecmp(buf,"true",4)==0) {
		    *(int *)memptr = 1;
	    	} else {
		    *(int *)memptr = 0;
	    	}
		len = sizeof(int);
	    	break;
	    }

	    case SFNODE:
	    case SFINT32: {
	    	retint=sscanf (buf,"%d",(int *)memptr);
		len = sizeof (int);
	    	break;
	    }
	    case SFFLOAT: {
	    	retint=sscanf (buf,"%f",(float *)memptr);
		len = sizeof (float);
	    	break;
	    }

	    case SFVEC2F: {	/* SFVec2f */
		flmem = (float *)memptr;
	    	retint=sscanf (buf,"%f %f",&flmem[0], &flmem[1]);
		len = sizeof(float) * 2;
	    	break;
	    }

	    case SFVEC3F:
	    case SFCOLOR: {	/* SFColor */
		flmem = (float *)memptr;
	    	retint=sscanf (buf,"%f %f %f",&flmem[0],&flmem[1],&flmem[2]);
		len = sizeof(float) * 3;
	    	break;
	    }

	    case SFCOLORRGBA:
	    case SFROTATION: {
		flmem = (float *)memptr;
	    	retint=sscanf (buf,"%f %f %f %f",&flmem[0],&flmem[1],&flmem[2],&flmem[3]);
		len = sizeof(float) * 4;
	    	break;
	    }

	    case SFTIME: {
		retint=sscanf (buf, "%lf", (double *)memptr);
		len = sizeof(double);
		break;
	    }

	    case MFNODE:
	    case MFINT32:
	    case MFTIME:
	    case MFCOLOR:
	    case MFCOLORRGBA:
	    case MFVEC3F:
	    case MFFLOAT:
	    case MFROTATION:
	    case MFVEC2F: {
		/* first number is the "number of numbers". */

		/* scan to start of element count, and read it in.*/
		while (*buf==' ') buf++; 
		retint=sscanf (buf,"%d",quant); while (*buf>' ') buf++;

		/* how many elements per row of this type? */
		*quant = *quant * returnElementRowSize(type);

		/* so, right now, quant tells us how many numbers we will read in. */



		/* how long is each element in bytes of this type? */
		len = *quant * returnElementLength(type);

		  #ifdef EAIVERBOSE
			printf ("bufsz is %d, len = %d quant = %d str: %s\n",bufsz, len, *quant,buf);
		  #endif
		  if (len > bufsz) {
			  printf ("Warning, MultiFloat too large, truncating to %d \n",bufsz);
			  len = bufsz;
		  }

		tmpbuf = (float *) malloc (len);
		fp = (float *) tmpbuf;
		ip = (int *) tmpbuf;
		for (count = 0; count < *quant; count++) {
			/* scan to start of number */
			while (*buf<=' ') buf++;

			/* scan in number */
			if ((type==MFINT32) || (type==MFNODE)) {
				retint=sscanf (buf,"%d",ip);
			} else { 
				retint=sscanf (buf,"%f",fp);
			}

			/* go to next number */
			while (*buf >' ') buf++;
			fp ++; ip++;
		}
			
		  /* now, copy over the data to the memory pointer passed in */
		  if(NULL != tmpbuf) memcpy (memptr,tmpbuf,len);
		  else perror("ScanValtoBuffer: tmpbuf NULL!");

		  /* free the memory malloc'd in Alberto's code */
		  free (tmpbuf);
		  break;
	    }

	    case MFSTRING: {
		int count;
		SV ** newp;
		struct xpv *mypv;
		struct Multi_String *strptr;
		int thisele, thissize, maxele;	/* used for reading in MFStrings*/


		/* return a Multi_String.						*/
		/*  struct Multi_String { int n; SV * *p; };				*/
		/*  buf will look like:							*/
		/*  	2  0;9:wordg.png  1;12:My"wordg.png				*/
		/*  	where 2 = max elements; 0;9 is element 0, 9 chars long...	*/
		/* OR, if a set1Value:							*/
		/* 	  -1 2:10:xxxxxxxxx						*/
		/* 	  where the 2 is the index.					*/

		strptr = (struct Multi_String *)memptr;

		/* scan to start of element count, and read it in.*/
		while (*buf==' ') buf++;
		retint=sscanf (buf,"%d",&maxele);
		while (*buf!=' ') buf++;

		/* is this a set1Value, or a setValue */
		if (maxele == -1) {
			/* is the range ok? for set1Value, we only replace, do not expand. */
			while (*buf==' ') buf++;
			retint=sscanf (buf,"%d;%d",&thisele,&thissize);
			/* printf ("this element %d has len %d MFStr size %d \n",thisele,thissize, strptr->n); */

			if (maxele < strptr->n) {
				/* scan to start of string*/
				while (*buf!=':') buf++; buf++;

				/* replace the space at stringln with a 0 */
				buf += thissize; *buf = 0; buf-=thissize;
				strptr->p[thisele] = EAI_newSVpv(buf);

				/* go to end of string */
				buf += thissize+1;
			} else {
				printf ("EAI - warning, MFString set1Value, set %d out of range for array (0-%d)\n",
					maxele,strptr->n);
			}
		} else {	
			/* make (and initialize) this MFString internal representation.*/
			strptr->n = maxele;
			FREE_IF_NZ (strptr->p);

			strptr->p = (SV**)malloc (maxele * sizeof(strptr->p));
			newp = strptr->p;
	
			/* scan through EAI string, extract strings, etc, etc.*/
			do {
				/* scan to start of element number*/
				/* make the new SV */
	
				while (*buf==' ') buf++;
				retint=sscanf (buf,"%d;%d",&thisele,&thissize);
				/*printf ("this element %d has size %d\n",thisele,thissize);*/
	
				/* scan to start of string*/
				while (*buf!=':') buf++; buf++;
	
				/* replace the space at stringln with a 0 */
				buf += thissize; *buf = 0; buf-=thissize;
				strptr->p[thisele] = EAI_newSVpv(buf);

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

	case SFIMAGE:
	case SFSTRING: {
		int thissize;

		/* save this stuff to a global SV, rather than worrying about memory pointers */
		#ifdef EAIVERBOSE
		printf ("ScanValtoBuffer: SFSTRING, string is %s, ptr %x %d\n",buf,memptr,memptr);
		#endif

		/* strings in the format "25:2 2 1 0xff 0x80 0x80 0xff" where 25 is the length */
		while (*buf == ' ') buf++;

		retint=sscanf (buf,"%d",&thissize);
		/* printf ("this SFStr size %d \n",thissize);  */

		/* scan to start of string*/
		while (*buf!=':') buf++; buf++;

		/* replace the space at stringln with a 0 */
		buf += thissize; 
		*buf=0;
		buf -= thissize;
                sv_global_tmp = EAI_newSVpv(buf);

		/* return char to a space */
		buf += thissize;
		*buf=' ';

		/* printf ("ScanValtoBuffer, svptr is now of type %x\n",SvTYPE(sv_global_tmp)); */
		/*len = maxele*sizeof(struct Multi_String);*/
		len = sizeof (void *);
		break;
	}
		
	  default: {
		printf("WARNING: unhandled CLASS from type %s\n", FIELD_TYPE_STRING(type));
		printf ("complain to the FreeWRL team.\n");
		printf ("(string is :%s:)\n",buf);
		return (0);
	    }
	}
	return (len);
}
