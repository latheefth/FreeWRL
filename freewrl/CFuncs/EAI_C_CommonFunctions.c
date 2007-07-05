/*******************************************************************
Copyright (C) 2007 John Stewart (CRC Canada)
DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
See the GNU Library General Public License (file COPYING in the distribution)
for conditions of use and redistribution.
*********************************************************************/

/* Common functions between the FreeWRL code and the ReWire C code */

#ifdef  REWIRE
	#include "Eai_C.h"
	#define add_parent(a,b)
	#define addToNode(a,b,c)
	#define MALLOC(a) malloc(a)
	int eaiverbose = FALSE;
#else
	#include "headers.h"

#endif


/* mimic making newSVpv, but *not* using Perl, as this is a different thread */
/* see Extending and Embedding Perl, Jenness, Cozens pg 75-77 */
struct Uni_String *newASCIIString(char *str) {
	struct Uni_String *retval;
	int len;

	if (eaiverbose) {
	printf ("newASCIIString for :%s:\n",str);
	}

	/* the returning Uni_String is here. Make blank struct */
	retval = MALLOC (sizeof (struct Uni_String));
	len = strlen(str);

	retval->strptr  = MALLOC (sizeof(char) * len+1);
	strncpy(retval->strptr,str,len+1);
	retval->len = len+1;
	retval->touched = 1; /* make it 1, to signal that this is a NEW string. */

	return retval;
}

/* do these strings differ?? If so, copy the new string over the old, and 
touch the touched flag */
void verify_Uni_String(struct  Uni_String *unis, char *str) {
	char *ns;
	char *os;
	int len;

	/* bounds checking */
	if (unis == NULL) {
		printf ("Warning, verify_Uni_String, comparing to NULL Uni_String, %s\n",str);
		return;
	}

	/* are they different? */
	if (strcmp(str,unis->strptr)!= 0) {
		os = unis->strptr;
		len = strlen(str);
		ns = MALLOC (len+1);
		strncpy(ns,str,len+1);
		unis->strptr = ns;
		FREE_IF_NZ (os);
		unis->touched++;
	}
}
		



/* get how many bytes in the type */
int returnElementLength(int type) {
	  switch (type) {
		case FIELDTYPE_SFTime :
    		case FIELDTYPE_MFTime : return sizeof(double); break;
    		case FIELDTYPE_MFInt32: return sizeof(int)   ; break;
		case FIELDTYPE_FreeWRLPTR:
    		case FIELDTYPE_SFNode :
    		case FIELDTYPE_MFNode : return sizeof(void *); break;
	  	default     : {}
	}
	return sizeof(float) ; /* turn into byte count */
}

/* for passing into CRoutes/CRoutes_Register */
/* the numbers match the "sub clength" in VRMLFields.pm, and the getClen in VRMLC.pm */
int returnRoutingElementLength(int type) {
	  switch (type) {
		case FIELDTYPE_SFTime:	return sizeof(double); break;
		case FIELDTYPE_SFBool:
		case FIELDTYPE_SFString:
		case FIELDTYPE_SFInt32:	return sizeof(int); break;
		case FIELDTYPE_SFFloat:	return sizeof (float); break;
		case FIELDTYPE_SFVec2f:	return sizeof (struct SFVec2f); break;
		case FIELDTYPE_SFVec3f:
		case FIELDTYPE_SFColor: 	return sizeof (struct SFColor); break;
		case FIELDTYPE_SFColorRGBA:
		case FIELDTYPE_SFRotation:return sizeof (struct SFRotation); break;
		case FIELDTYPE_SFNode:	return sizeof (uintptr_t); break;
		case FIELDTYPE_MFNode:	return -10; break;
		case FIELDTYPE_SFImage:	return -12; break;
		case FIELDTYPE_MFString: 	return -13; break;
		case FIELDTYPE_MFFloat:	return -14; break;
		case FIELDTYPE_MFColorRGBA:
		case FIELDTYPE_MFRotation: return -15; break;
		case FIELDTYPE_MFBool:
		case FIELDTYPE_MFInt32:	return -16; break;
		case FIELDTYPE_MFColor:	return -17; break;
		case FIELDTYPE_MFVec2f:	return -18; break;
		case FIELDTYPE_MFVec3f:	return -19; break;

                default:       return type;
	}
} 



/* how many numbers/etc in an array entry? eg, SFVec3f = 3 - 3 floats */
/*		"" ""			eg, MFVec3f = 3 - 3 floats, too! */
int returnElementRowSize (int type) {
	switch (type) {
		case FIELDTYPE_SFVec2f:
		case FIELDTYPE_MFVec2f:
			return 2;
		case FIELDTYPE_SFColor:
		case FIELDTYPE_MFColor:
		case FIELDTYPE_SFVec3f:
		case FIELDTYPE_MFVec3f:
		case FIELDTYPE_SFImage: /* initialization - we can have a "0,0,0" for no texture */
			return 3;
		case FIELDTYPE_SFRotation:
		case FIELDTYPE_MFRotation:
		case FIELDTYPE_SFColorRGBA:
		case FIELDTYPE_MFColorRGBA:
			return 4;
	}
	return 1;

}


int countFloatElements (char *instr) {
	int count = 0;
	SCANTONUMBER(instr);
	while (*instr != '\0') {
		SCANPASTFLOATNUMBER(instr);
		SCANTONUMBER(instr);
		count ++;
		/* printf ("string now is :%s:, count %d\n",instr,count); */
	}
	return count;
}
	
int countIntElements (char *instr) {
	int count = 0;
	SCANTONUMBER(instr);
	while (*instr != '\0') {
		SCANPASTINTNUMBER(instr);
		SCANTONUMBER(instr);
		count ++;
		/* printf ("countIntElements:string now is :%s:, count %d\n",instr,count); */
	}
	return count;
}

int countStringElements (char *instr) {
	int count = 0;
	char *ptr;

	char startsWithQuote;
	SCANTOSTRING(instr);

	/* is this a complex string like: "images/256x256.jpg" or just MODULATE4X. */
	/* if it is just a word, return that we have 1 string */
	if ((*instr == '"') || (*instr == '\'')) startsWithQuote = *instr;
	else return 1;

	count = 0;
	ptr = instr;
	while (ptr != NULL) { ptr++; ptr = strchr(ptr,startsWithQuote); count++; }
	return count /2;
}

int countBoolElements (char *instr) {
printf ("CAN NOT COUNT BOOL ELEMENTS YET\n");
printf ("string %s\n",instr);
return 0;
}
	

int countElements (int ctype, char *instr) {
	int elementCount;
	
	switch (ctype) {
		case FIELDTYPE_SFVec2f:	elementCount = 2; break;
		case FIELDTYPE_SFRotation:
		case FIELDTYPE_SFColorRGBA: elementCount = 4; break;
		case FIELDTYPE_SFVec3f:
		case FIELDTYPE_SFColor: elementCount = 3; break;
		case FIELDTYPE_MFRotation:
		case FIELDTYPE_MFColor:
		case FIELDTYPE_MFFloat:
		case FIELDTYPE_MFTime:
		case FIELDTYPE_MFVec2f:
		case FIELDTYPE_MFVec3f:
		case FIELDTYPE_MFColorRGBA: 
		case FIELDTYPE_MFNode: elementCount = countFloatElements(instr); break;
		case FIELDTYPE_MFBool: elementCount = countBoolElements(instr); break;
		case FIELDTYPE_MFString: elementCount = countStringElements(instr); break;
		case FIELDTYPE_SFImage:
		case FIELDTYPE_MFInt32: elementCount = countIntElements(instr); break;
		default: elementCount = 1;
	}
	
	return elementCount;
}

/* called effectively by VRMLCU.pm */
void Parser_scanStringValueToMem(void *ptr, int coffset, int ctype, char *value) {
	int datasize;
	int rowsize;
	int elementCount;

	char *nst;                      /* used for pointer maths */
	void *mdata;
	int *iptr;
	float *fptr;
	struct Uni_String **svptr;
	struct Uni_String *mysv;
	int tmp;
	char *Cptr;
	char startsWithQuote;
	

	/* temporary for sscanfing */
	float fl[4];
	int in[4];
	uintptr_t inNode[4];
	double dv;

	#ifdef SETFIELDVERBOSE
	printf ("PST, for %s we have %s strlen %d\n",FIELDTYPES[ctype], value, strlen(value));
	#endif

	nst = (char *) ptr; /* should be 64 bit compatible */
	nst += coffset;

	/* go to the start of the string - javascript will return this with open/close brackets */
	while (*value == ' ') value ++;
	if (*value == '[') {
		value ++;
		Cptr = strrchr(value,']');
		if (Cptr!=NULL) *Cptr = '\0';
		#ifdef SETFIELDVERBOSE
		printf ("PST, string was from Javascript, now is %s\n",value);
		#endif
	}

	datasize = returnElementLength(ctype);
	elementCount = countElements(ctype,value);

	switch (ctype) {

		case FIELDTYPE_SFBool: {
				if (strstr(value,"true") != NULL) *in = TRUE;
				else if (strstr (value,"TRUE") != NULL) *in = TRUE;
				else *in = FALSE;
				memcpy(nst,in,datasize); 
				break;
			}
		case FIELDTYPE_SFInt32:
			{ sscanf (value,"%d",in); 
				memcpy(nst,in,datasize); 
				/* FIELDTYPE_SFNodeS need to have the parent field linked in */
				if (ctype == FIELDTYPE_SFNode) {
					add_parent((void *)in[0], ptr); 
				}
				
			break;}
		case FIELDTYPE_FreeWRLPTR:
		case FIELDTYPE_SFNode: { 
				/* JAS - changed %d to %ld for 1.18.15 */
				sscanf (value,"%ld",inNode); 
				/*
				if (inNode[0] != 0) {
					printf (" andof type %s\n",stringNodeType(((struct X3D_Box *)inNode[0])->_nodeType));
				} */
				memcpy(nst,inNode,datasize); 
				/* FIELDTYPE_SFNodeS need to have the parent field linked in */
				if (ctype == FIELDTYPE_SFNode) {
					add_parent((void *)inNode[0], ptr); 
				}
				
			break;}

		
		case FIELDTYPE_SFFloat:
		case FIELDTYPE_SFVec2f:
		case FIELDTYPE_SFRotation:
		case FIELDTYPE_SFColorRGBA:
		case FIELDTYPE_SFVec3f:
		case FIELDTYPE_SFColor: {
			for (tmp = 0; tmp < elementCount; tmp++) {
				SCANTONUMBER(value);
				sscanf (value, "%f",&fl[tmp]);
				SCANPASTFLOATNUMBER(value);
			}
			memcpy (nst,fl,datasize*elementCount); break;}
		case FIELDTYPE_MFBool:
		case FIELDTYPE_SFImage: 
		case FIELDTYPE_MFInt32: {
			mdata = MALLOC (elementCount * datasize);
			iptr = (int *)mdata;
			for (tmp = 0; tmp < elementCount; tmp++) {
				SCANTONUMBER(value);
				/* is this a HEX number? the %i should handle it */
				sscanf(value, "%i",iptr);
				iptr ++;
				SCANPASTINTNUMBER(value);
			}
			((struct Multi_Int32 *)nst)->p=mdata;
			((struct Multi_Int32 *)nst)->n = elementCount;
			break;
			}

		case FIELDTYPE_MFNode: {
			for (tmp = 0; tmp < elementCount; tmp++) {
				/* JAS changed %d to %ld for 1.18.15 */
				sscanf(value, "%ld",inNode);
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
		case FIELDTYPE_SFTime: { sscanf (value, "%lf", &dv); 
				/* printf ("SFtime, for value %s has %lf datasize %d\n",value,dv,datasize); */
				memcpy (nst,&dv,datasize);
			break; }

		case FIELDTYPE_MFRotation:
		case FIELDTYPE_MFColor:
		case FIELDTYPE_MFFloat:
		case FIELDTYPE_MFTime:
		case FIELDTYPE_MFVec2f:
		case FIELDTYPE_MFVec3f:
		case FIELDTYPE_MFColorRGBA: {
			/* skip past any brackets, etc, that might come via Javascript.
			   see tests/8.wrl for one of these */

			/* get the row size */
			rowsize = returnElementRowSize(ctype);

			#ifdef SETFIELDVERBOSE
			printf ("MF* data size is %d elerow %d elementCount %d str %s\n",datasize, returnElementRowSize(ctype),elementCount,value);
			#endif

			mdata = MALLOC (elementCount * datasize);
			fptr = (float *)mdata;
			for (tmp = 0; tmp < elementCount; tmp++) {
				SCANTONUMBER(value);
				sscanf(value, "%f",fptr);
				fptr ++;
				SCANPASTFLOATNUMBER(value);
			}
			((struct Multi_Node *)nst)->p=mdata;
			((struct Multi_Node *)nst)->n = elementCount/rowsize;
			break;
			}

		case FIELDTYPE_SFString: 
			{
			/* first, can we destroy the old value?? */
			memcpy (&mysv,nst,datasize);
			mysv->len=0;
			FREE_IF_NZ(mysv->strptr);

			/* create new value, and copy its pointer over. */
			mysv  = newASCIIString(value); 
			memcpy (nst, &mysv, datasize);
			break; }
			
		case FIELDTYPE_MFString: {
			/* printf ("start of FIELDTYPE_MFString :%s:\n",value); */
			mdata = MALLOC (elementCount * datasize);
			svptr = (struct Uni_String **)mdata;

			SCANTOSTRING(value);
			if ((*value == '"') || (*value == '\'')) startsWithQuote = *value;
			else startsWithQuote = '\0';

			for (tmp = 0; tmp < elementCount; tmp++) {
				if (startsWithQuote != '\0') value++;
				Cptr = strchr (value,startsWithQuote);
				*Cptr = '\0';

				/* scan in the new ascii string */
				/* printf ("MFSTRING, sitting at string :%s: for %d of %d\n",value,tmp,elementCount); */
				*svptr = newASCIIString(value);
				svptr ++;

				/* replace that character, and continue on */
				*Cptr = startsWithQuote;
				value = Cptr;
				if (startsWithQuote != '\0') value++;
				SCANTOSTRING(value);
				/* printf ("MFSTRING string now is :%s:\n",value); */
				
			}
			((struct Multi_Node *)nst)->p=mdata;
			((struct Multi_Node *)nst)->n = elementCount;
			break;
			}

		default: {
			#ifdef REWIRE 
			printf ("unhandled PST type, check code in EAI_C_CommonFunctions\n");
			#else
			printf ("Unhandled PST, %s: value %s, ptrnode %s nst %d offset %d numelements %d\n",
			FIELDTYPES[ctype],value,stringNodeType(((struct X3D_Box *)ptr)->_nodeType),nst,coffset,elementCount+1);
			#endif
			break;
			};
	}
}

