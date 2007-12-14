/*******************************************************************
 Copyright (C) 2006 John Stewart, CRC Canada
 DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
 See the GNU Library General Public License (file COPYING in the distribution)
 for conditions of use and redistribution.
*********************************************************************/

/*
 * 
 *
 */
#include "jsapi.h"
#include "jsUtils.h"
#include "headers.h"
#include "X3DParser.h"
#include "CParseGeneral.h"

static int currentProtoDeclare  = ID_UNDEFINED;
static int MAXProtos = 0;
static int curProDecStackInd = 0;
static int currentProtoInstance = ID_UNDEFINED;

/* for parsing fields in PROTO expansions */
/* FIELD_END is an ascii string that will pass the XML parser, but should not be found in a field value */
#define FIELD_END "\t \t\n" 
#define strIS "<IS>"
#define strNOTIS "</IS>"
#define strCONNECT  "connect"
#define strNODEFIELD "nodeField"
#define strPROTOFIELD "protoField"
#define MAX_ID_SIZE 1000
#define NODEFIELD_EQUALS "nodeField=\""
#define PROTOFIELD_EQUALS "protoField=\""

/* for parsing script initial fields */
#define MPFIELDS 4
#define MP_NAME 0
#define MP_ACCESSTYPE 1
#define MP_TYPE 2
#define MP_VALUE 3

/* ProtoInstance table This table is a dynamic table that is used for keeping track of ProtoInstance field values... */
static int curProtoInsStackInd = -1;

struct PROTOInstanceEntry {
	char *name[PROTOINSTANCE_MAX_PARAMS];
	char *value[PROTOINSTANCE_MAX_PARAMS];
	int container;
	int paircount;
};
struct PROTOInstanceEntry ProtoInstanceTable[PROTOINSTANCE_MAX_LEVELS];

/* PROTO table */
struct PROTOnameStruct {
	char *name;
	FILE *fileDescriptor;
	char *fileName;
	int charLen;
	int fileOpen;
};
struct PROTOnameStruct *PROTONames = NULL;


/* Script table - script parameter names, values, etc. */
struct ScriptFieldStruct {
	int scriptNumber;
	int fromScriptNotPROTO;
	struct Uni_String *fieldName;
	struct Uni_String *value;
	int type;
	int kind;
	int offs;
};

struct ScriptFieldStruct *ScriptFieldNames = NULL;
int ScriptFieldTableSize = ID_UNDEFINED;
int MAXScriptFieldParams = 0;



/****************************** PROTOS ***************************************************/

/* we are closing off a parse of an XML file. Lets go through and free/unlink/cleanup. */
void freeProtoMemory () {
	int i;

	
	#ifdef X3DPARSERVERBOSE
	printf ("freeProtoMemory, currentProtoDeclare is %d PROTONames = %d \n",currentProtoDeclare,PROTONames);
	#endif

	if (PROTONames != NULL) {
		for (i=0; i<= currentProtoDeclare; i++) {
			if (PROTONames[i].fileOpen) fclose(PROTONames[i].fileDescriptor); /* should never happen... */

			FREE_IF_NZ (PROTONames[i].name);
			if (PROTONames[i].fileName != NULL) unlink (PROTONames[i].fileName);
			free (PROTONames[i].fileName); /* can not FREE_IF_NZ this one as it's memory is not kept track of by MALLOC */

		}
		FREE_IF_NZ(PROTONames);
	}
	#ifdef X3DPARSERVERBOSE
	printf ("freeProtoMemory,ScriptFieldNames is %d ScriptFieldTableSize %d, MAXScriptFieldParams %d\n",ScriptFieldNames, ScriptFieldTableSize, MAXScriptFieldParams);
	#endif

	if (ScriptFieldNames != NULL) {
		for (i=0; i<=ScriptFieldTableSize; i++) {
			if (ScriptFieldNames[i].fieldName != NULL) {
				FREE_IF_NZ(ScriptFieldNames[i].fieldName->strptr);
				FREE_IF_NZ(ScriptFieldNames[i].fieldName);
			}
			if (ScriptFieldNames[i].value != NULL) {
				FREE_IF_NZ(ScriptFieldNames[i].value->strptr);
				FREE_IF_NZ(ScriptFieldNames[i].value);
			}
		}
	}
}


/* record each field of each script - the type, kind, name, and associated script */
static void registerProto(const char *name) {
	int i;
	#ifdef X3DPARSERVERBOSE
	TTY_SPACE
	printf ("registerProto for %s\n",name);
	#endif


	/* ok, we got a name and a type */
	if (currentProtoDeclare >= MAXProtos) {
		/* oooh! not enough room at the table */
		MAXProtos += 10; /* arbitrary number */
		PROTONames = (struct PROTOnameStruct*)REALLOC (PROTONames, sizeof(*PROTONames) * MAXProtos);
	}

	PROTONames[currentProtoDeclare].name = STRDUP((char *)name);
	PROTONames[currentProtoDeclare].fileName = tempnam("/tmp","freewrl_proto");
	PROTONames[currentProtoDeclare].fileDescriptor = fopen(PROTONames[currentProtoDeclare].fileName,"w");
#ifndef TRY
	PROTONames[currentProtoDeclare].charLen =  fprintf (PROTONames[currentProtoDeclare].fileDescriptor,
		"<?xml version=\"1.0\" encoding=\"utf-8\"?>\n<Scene>\n");
#else
	PROTONames[currentProtoDeclare].charLen = 0;
#endif
	PROTONames[currentProtoDeclare].fileOpen = TRUE;

	#ifdef X3DPARSERVERBOSE
	TTY_SPACE
	printf ("opened name %s for PROTO name %s id %d\n",PROTONames[currentProtoDeclare].fileName,
		PROTONames[currentProtoDeclare].name,currentProtoDeclare);
	#endif
}


void parseProtoInstanceFields(const char *name, const char **atts) {
	int count;
	int index;
	
	index = 0;
	ProtoInstanceTable[curProtoInsStackInd].name[index] = NULL;
	ProtoInstanceTable[curProtoInsStackInd].value[index] = NULL;

	#ifdef X3DPARSERVERBOSE
	printf ("parsing PRotoInstanceFields for %s at level %d\n",name,curProtoInsStackInd);
	#endif

	/* this should be a <fieldValue...> tag here */
	if (strcmp(name,"fieldValue") == 0) {
		for (count = 0; atts[count]; count += 2) {
			#ifdef X3DPARSERVERBOSE
			printf ("ProtoInstanceFields: %s=\"%s\"\n",atts[count], atts[count+1]);
			#endif

			/* add this to our instance tables */
			/* is this the name field? */
			if (strcmp("name",atts[count])==0) 
				ProtoInstanceTable[curProtoInsStackInd].name[index] = STRDUP(atts[count+1]);
			if (strcmp("value",atts[count])==0) 
				ProtoInstanceTable[curProtoInsStackInd].value[index] = STRDUP(atts[count+1]);

			/* did we get both a name and a value? */
			if ((ProtoInstanceTable[curProtoInsStackInd].name[index] != NULL) &&
			    (ProtoInstanceTable[curProtoInsStackInd].value[index] != NULL)) {
				index++;
				ProtoInstanceTable[curProtoInsStackInd].name[index] = NULL;
				ProtoInstanceTable[curProtoInsStackInd].value[index] = NULL;
			}

			if (index>=PROTOINSTANCE_MAX_PARAMS) {
				ConsoleMessage ("too many parameters for ProtoInstance, sorry...\n");
				index=0;
			}
		}

		ProtoInstanceTable[curProtoInsStackInd].paircount = index;

	} else if (strcmp(name,"ProtoInstance") != 0) {
		ConsoleMessage ("<ProtoInstance> expects <fieldValues> or </ProtoInstance>, got %s at line %d",name,LINE);
	}
}

void dumpProtoBody (const char *name, const char **atts) {
	int i;
	int count;

	#ifdef X3DPARSERVERBOSE
	TTY_SPACE
	printf ("dumping ProtoBody for %s\n",name);
	#endif

	if (PROTONames[currentProtoDeclare].fileOpen) {
		PROTONames[currentProtoDeclare].charLen += fprintf (PROTONames[currentProtoDeclare].fileDescriptor, "<%s",name);
		for (count = 0; atts[count]; count += 2) {
			PROTONames[currentProtoDeclare].charLen += 
				fprintf (PROTONames[currentProtoDeclare].fileDescriptor," %s=\"%s\" %s",atts[count],atts[count+1],FIELD_END);
		}
		PROTONames[currentProtoDeclare].charLen += fprintf (PROTONames[currentProtoDeclare].fileDescriptor,">\n");
	}
}

void dumpCDATAtoProtoBody (char *str) {
	if (PROTONames[currentProtoDeclare].fileOpen) {
		PROTONames[currentProtoDeclare].charLen += 
			fprintf (PROTONames[currentProtoDeclare].fileDescriptor,"<![CDATA[%s]]>",str);
	}
}

void endDumpProtoBody (const char *name) {
	int i;

	/* we are at the end of the ProtoBody, close our tempory file */
	if (PROTONames[currentProtoDeclare].fileOpen) {
		#ifndef try
		PROTONames[currentProtoDeclare].charLen += fprintf (PROTONames[currentProtoDeclare].fileDescriptor, "\n</Scene>\n");
		#endif

		fclose (PROTONames[currentProtoDeclare].fileDescriptor);
		PROTONames[currentProtoDeclare].fileOpen = FALSE;

		#ifdef X3DPARSERVERBOSE
			TTY_SPACE
			printf ("endDumpProtoBody, just closed %s, it is %d characters long\n",
				PROTONames[currentProtoDeclare].fileName, PROTONames[currentProtoDeclare].charLen);
		#endif
	}
}

/* find a value for the proto field on invocation. First look at the ProtoInstance, if not there, then look
   at the ProtoDeclare for the field. */
static char *getProtoValue(int ProtoInvoc, char *protofield) {
	char *start; char *end;
	char *retptr;
	char id[MAX_ID_SIZE];
	int i;

	start = strstr(protofield,PROTOFIELD_EQUALS);
	if (start == NULL) {
		ConsoleMessage ("could not find protoField in %s!\n",protofield);
		return "";
	}
	start += strlen(PROTOFIELD_EQUALS);

	end = strstr(start,FIELD_END);
	if (end == NULL) {
		ConsoleMessage ("could not find protoField END in %s!\n",protofield);
		return "";
	}

	if ((end - start) > (MAX_ID_SIZE-10)) { 
		ConsoleMessage ("Proto ID nodeField ID is way too long...\n"); 
		return; 
	}

	/* copy the proto Id over */
	memcpy (id, start, (end - start) -2); id[end-start-2]='\0';
	
	#ifdef X3DPARSERVERBOSE
	printf ("getProtoValue for proto %d, char :%s:\n",ProtoInvoc, id, curProtoInsStackInd);
	#endif

	/* get the start/end value pairs, and copy them into the id field. */
	if ((curProtoInsStackInd < 0) || (curProtoInsStackInd >= PROTOINSTANCE_MAX_LEVELS)) {
		return "";
	} else {
		/* is this to be matched in the ProtoInstance fields? */
		for (i=0; i<ProtoInstanceTable[curProtoInsStackInd].paircount; i++) {
			if (strcmp(id,ProtoInstanceTable[curProtoInsStackInd].name[i]) == 0) {
				/* printf ("getProtoValue, found name!\n"); */
				return ProtoInstanceTable[curProtoInsStackInd].value[i];
			}
		}

		/* no, maybe the field is in the ProtoInterface definitions? */
		#ifdef X3DPARSERVERBOSE
		printf ("have to look for id %s in ProtoInterface\n",id);
		#endif

		if (getFieldValueFromProtoInterface (id, ProtoInvoc, &retptr)) {
			return retptr;
		}
	}

	/* if we are here, we did not find that parameter */
	ConsoleMessage ("ProtoInstance <%s>, could not find parameter <%s>",
			PROTONames[ProtoInvoc].name, id);

	return "";
}


/* handle a <ProtoInstance> tag */
void parseProtoInstance (const char **atts) {
	int i,count;
	int foundName = FALSE;
	int nameIndex = ID_UNDEFINED;
	int containerIndex = ID_UNDEFINED;
	int containerField = ID_UNDEFINED;
	int protoTableIndex = 0;

	parserMode = PARSING_PROTOINSTANCE;
	curProtoInsStackInd++;

	#ifdef X3DPARSERVERBOSE
	printf ("parseProtoInstance, incremented curProtoInsStackInd to %d\n",curProtoInsStackInd);
	#endif

	currentProtoInstance = ID_UNDEFINED;
	

	for (count = 0; atts[count]; count += 2) {
		#ifdef X3DPARSERVERBOSE
		TTY_SPACE
		printf ("parseProtoInstance: field:%s=%s\n", atts[count], atts[count + 1]);
		#endif

		if (strcmp("name",atts[count]) == 0) {
			nameIndex=count+1;
		} else if (strcmp("containerField",atts[count]) == 0) {
			containerIndex = count+1;
		} else if (strcmp("class",atts[count]) == 0) {
			ConsoleMessage ("field \"class\" not currently used in a ProtoInstance parse... sorry");
		} else if (strcmp("DEF",atts[count]) == 0) {
			ConsoleMessage ("field \"DEF\" not currently used in a ProtoInstance parse... sorry");
		} else if (strcmp("USE",atts[count]) == 0) {
			ConsoleMessage ("field \"USE\" not currently used in a ProtoInstance parse... sorry");
		}
		
	}

	/* did we have a containerField? */
	if (containerIndex != ID_UNDEFINED) {
		containerField = findFieldInFIELDNAMES(atts[containerIndex]);
		printf ("parseProtoInstance, found a containerField of %s, id is %d\n",atts[containerIndex],containerField);

	printf ("so, parseProtoInstance, cpfs is %d\n", curProtoInsStackInd);
	}

	/* so, the container will either be -1, or will have a valid FIELDNAMES index */
	ProtoInstanceTable[curProtoInsStackInd].container = containerField;

	/* did we find the name? */
	if (nameIndex != ID_UNDEFINED) {
		#ifdef X3DPARSERVERBOSE
		printf ("parseProtoInstance, found name :%s:\n",atts[nameIndex]);
		#endif

		/* go through the PROTO table, and find the match, if it exists */
		for (protoTableIndex = 0; protoTableIndex <= currentProtoDeclare; protoTableIndex ++) {
			if (strcmp(atts[nameIndex],PROTONames[protoTableIndex].name) == 0) {
				currentProtoInstance = protoTableIndex;
				return;
			}
		}
	
	} else {
		ConsoleMessage ("\"ProtoInstance\" found, but field \"name\" not found!\n");
	}

	/* initialize this call level */
	if ((curProtoInsStackInd < 0) || (curProtoInsStackInd >= PROTOINSTANCE_MAX_LEVELS)) {
		ConsoleMessage ("too many levels of ProtoInstances, recompile with PROTOINSTANCE_MAX_LEVELS higher ");
		curProtoInsStackInd = 0;
	}

	ProtoInstanceTable[curProtoInsStackInd].paircount = 0;
}

/* have a </ProtoInstance> so should have valid name and fieldValues */
void expandProtoInstance(struct X3D_Group *myGroup) {
	int i;
	char *protoInString;
	char *origString;
	int psSize;
	int rs;
	char *IS = NULL;
	char *endIS = NULL;
	char *connect = NULL;
	char *shape = NULL;
	char *strptr;
	int pf;

	/* temps for string manipulation */
	char *connect_tmp;
	char *ptr_connect_nodefieldEquals;
	char *ptr_connect_protofieldEquals;
	char *ptr_connect_nodefieldEnd;
	char *ptr_connect_protofieldEnd;
	char *field_in_shape;
	char id[MAX_ID_SIZE];
	char *valueStr;

	#define OPEN_AND_READ_PROTO \
		PROTONames[currentProtoInstance].fileDescriptor = fopen (PROTONames[currentProtoInstance].fileName,"r"); \
		rs = fread(origString, 1, PROTONames[currentProtoInstance].charLen, PROTONames[currentProtoInstance].fileDescriptor); \
		origString[rs] = '\0'; /* ensure termination */ \
		fclose (PROTONames[currentProtoInstance].fileDescriptor); \
		/* printf ("OPEN AND READ %s returns:%si\n:\n",PROTONames[currentProtoInstance].fileName, origString); */ \
		if (rs != PROTONames[currentProtoInstance].charLen) { \
			ConsoleMessage ("protoInstance :%s:, expected to read %d, actually read %d\n",PROTONames[currentProtoInstance].name,  \
				PROTONames[currentProtoInstance].charLen,rs); \
		} 


	#define FIND_THE_CONNECT  \
		*endIS = '\0';  \
		connect = strstr(IS,strCONNECT);  \
		/* ok, we have a valid <IS> and </IS> lets process what is between them. */  \
		if (connect == NULL) {  \
			ConsoleMessage ("problem with connect in <IS> for proto %s\n",PROTONames[currentProtoInstance].name);  \
			FREE_IF_NZ(protoInString); FREE_IF_NZ(origString); \
			return;  \
		}

	#define FIND_THE_END_OF_IS \
		endIS = strstr(IS,strNOTIS); \
		/* printf ("endIS is :%s:\n",endIS); */ \
		if (endIS == NULL) { \
			ConsoleMessage ("did not find an </IS> for ProtoInstance %s\n",PROTONames[currentProtoInstance].name); \
			FREE_IF_NZ(protoInString); FREE_IF_NZ(origString); \
			return; \
		}

	#define FIND_THE_IS \
		 IS = strstr(strptr,strIS);

	#define FIND_ATTACHED_NODE \
		shape = strrchr(strptr,'<'); \
		if (shape == NULL) { \
			ConsoleMessage ("problem with finding a "<" for proto %s\n",PROTONames[currentProtoInstance].name); \
			FREE_IF_NZ(protoInString); FREE_IF_NZ(origString); \
			return; \
		}

	#define COPY_UP_TO_ATTACHED_NODE \
		*shape = '\0'; /* temporary... */ \
		strcat (protoInString,strptr); \
		*shape = '<'; /* to allow for copy to happen... */

	#define COPY_CONNECT_POINTER \
		connect_tmp = connect;

	#define FIND_NODEFIELD_EQUALS \
		ptr_connect_nodefieldEquals = strstr(connect_tmp, NODEFIELD_EQUALS); 


	#define INCREMENT_CONNECT_COPY \
		/* printf ("going to next > in :%s:\n",connect_tmp); */ \
		connect_tmp = strchr(connect_tmp, '>');  \
		if (connect_tmp != NULL) connect_tmp++; /* skip past the ">" char */
		


	#define VERIFY_ID_LENGTH \
		if ((ptr_connect_nodefieldEnd - ptr_connect_nodefieldEquals) > (MAX_ID_SIZE-10)) { \
			ConsoleMessage ("Proto ID nodeField ID is way too long...\n"); \
			FREE_IF_NZ(protoInString); FREE_IF_NZ(origString); \
			return; \
		}

	#define SKIP_PAST_NODEFIELD_EQUALS \
			ptr_connect_nodefieldEquals += strlen(NODEFIELD_EQUALS);

	#define FIND_NODEFIELD_END \
			ptr_connect_nodefieldEnd = strstr(ptr_connect_nodefieldEquals,FIELD_END);

	#define COPY_ID_WITH_EQUALS \
			memcpy(id,ptr_connect_nodefieldEquals,(ptr_connect_nodefieldEnd - ptr_connect_nodefieldEquals)-2);  \
			id[(ptr_connect_nodefieldEnd - ptr_connect_nodefieldEquals)-2] = '='; \
			id[(ptr_connect_nodefieldEnd - ptr_connect_nodefieldEquals)-1] = '\"'; \
			id[(ptr_connect_nodefieldEnd - ptr_connect_nodefieldEquals)-0] = '\0';

	#define COPY_ID_NO_EQUALS \
			memcpy(id,ptr_connect_nodefieldEquals,(ptr_connect_nodefieldEnd - ptr_connect_nodefieldEquals)-2);  \
			id[(ptr_connect_nodefieldEnd - ptr_connect_nodefieldEquals)-2] = '\0'; 

	#define REMOVE_FIELD_FROM_ATTACHED_NODE \
			/* printf ("REMOVE_FIELD_FROM_ATTACHED_NODE - id :%s: from string :%s:\n",id,shape); */ \
			field_in_shape = strstr(shape,id); \
			if (field_in_shape != NULL) { \
				memset (field_in_shape, ' ',(strstr(field_in_shape,FIELD_END)-field_in_shape)); \
			/* this can be ok; normal fields do not need to be specified \
			} else { \
				shape[7] = '\0'; \
				ConsoleMessage ("Proto Expansion, did not find field %s in shape :%s:",id,shape); \
			FREE_IF_NZ(protoInString); FREE_IF_NZ(origString); \
				return; \
			*/ \
			} 

	#define COPY_ATTACHED_NODE_WITH_UNALTERED_FIELDS \
		strcat (protoInString,shape);
	

	#define REMOVE_BRACKET_FROM_ATTACHED_NODE \
		if (strrchr(shape,'>') != NULL) *strrchr(shape,'>') = ' ';

	#define FIND_NODEFIELD_PROTOFIELD_PAIR \
		/* printf ("looking for field PAIRS in :%s:\n",connect_tmp); */ \
		ptr_connect_nodefieldEquals = strstr(connect_tmp, NODEFIELD_EQUALS); \
		ptr_connect_protofieldEquals = strstr(connect_tmp, PROTOFIELD_EQUALS); 


	/* first, do we actually have a valid proto here? */
	if (currentProtoInstance == ID_UNDEFINED) 
		return;

	#ifdef X3DPARSERVERBOSE
	printf ("ok, expandProtoInstance, have a valid protoInstance of %d\n",currentProtoInstance);
	#endif

	/* step 1. read in the PROTO text. */
	

	psSize = PROTONames[currentProtoInstance].charLen * 10;

	if (psSize < 0) {
		ConsoleMessage ("problem with psSize in expandProtoInstance");
		return;
	}

	protoInString = MALLOC (psSize);
	origString = MALLOC(PROTONames[currentProtoInstance].charLen+1);
	protoInString[0] = '\0';

	/* read in the PROTO */
	OPEN_AND_READ_PROTO
 
	#ifdef X3DPARSERVERBOSE
	printf ("now, we have in memory:\n%s\n", origString);
	#endif

	/* loop through, and replace any IS'd fields with our PROTO expansion stuff... */
	strptr = origString;
	FIND_THE_IS
	while (IS != NULL) {
		/* find the </IS> */
		FIND_THE_END_OF_IS

		/* find the connect... */
		FIND_THE_CONNECT
		
		/* put a string terminator at this <IS> position, for copying up until this point */
		*IS = '\0';

		/* and, lets find the entity to work on; the one we have to do the <IS> substitution on ... */
		FIND_ATTACHED_NODE

		/* copy up to here */
		COPY_UP_TO_ATTACHED_NODE

		/* now, we do some manipulating... */
		/* step 1. Go through the connect, and remove any nodeFields found from the "shape" string. This removes the
		   fields from the "shape" string */ 

		COPY_CONNECT_POINTER
		FIND_NODEFIELD_EQUALS
		while (ptr_connect_nodefieldEquals != NULL) {
			/* point to the ID at the end of the "nodeField="" string */
			SKIP_PAST_NODEFIELD_EQUALS
			FIND_NODEFIELD_END

			/* make sure ID is not toooo long. Only if the user makes a mistake... */
			VERIFY_ID_LENGTH

			/* copy this id over; make sure it has a =" on it */
			COPY_ID_WITH_EQUALS

			/* and remove this from the string. */
			REMOVE_FIELD_FROM_ATTACHED_NODE

			/* move along to the next one... */
			INCREMENT_CONNECT_COPY
			FIND_NODEFIELD_EQUALS
		}

		/* remove the ">" from the node */
		REMOVE_BRACKET_FROM_ATTACHED_NODE

		/* copy what we have to the output string, because that does not change... */
		COPY_ATTACHED_NODE_WITH_UNALTERED_FIELDS

		/* step 2. Go through the connect again, and do any nodeField substitutions with that in the PROTO
		   header/invocation parameters. */
		COPY_CONNECT_POINTER
		FIND_NODEFIELD_PROTOFIELD_PAIR

		while ((ptr_connect_nodefieldEquals != NULL) && (ptr_connect_protofieldEquals != NULL)) {
			/* point to the ID at the end of the "nodeField="" string */
			SKIP_PAST_NODEFIELD_EQUALS
			FIND_NODEFIELD_END

			/* make sure ID is not toooo long. Only if the user makes a mistake... */
			VERIFY_ID_LENGTH

			/* copy this id over; taking off the trailing "" " from it. */
			COPY_ID_NO_EQUALS

			/* do the substitution if we can. */
			valueStr = getProtoValue(currentProtoDeclare,ptr_connect_protofieldEquals);


			/* check sizes */
			if ((strlen(protoInString) + strlen (valueStr)) > (psSize/2)) {
				psSize *= ((strlen(protoInString) + strlen (valueStr)) *2);
				protoInString = REALLOC(protoInString,psSize);
			}


			if (valueStr != NULL) {
				strcat(protoInString,id);
				strcat (protoInString,"=\"");
				strcat (protoInString, valueStr);
				strcat (protoInString,"\"");
				strcat (protoInString,FIELD_END);
			}


			/* move along to the next one... */
			INCREMENT_CONNECT_COPY
			FIND_NODEFIELD_PROTOFIELD_PAIR
		}

		/* append a close brace for this shape */
		strcat (protoInString,">"); strcat (protoInString,FIELD_END);

		/* go past the IS, and continue */
		strptr = endIS + strlen(strNOTIS);
		IS = strstr(strptr,strIS);
	}

	/* printf ("and, parse this...\n:%s:\n",protoInString);
	   printf ("remainder :%s:\n",strptr); */

	/* put the remainder of this string on to the end of the protoInString */
	strcat (protoInString,strptr);

	#ifdef X3DPARSERVERBOSE
	printf ("PROTO EXPANSION IS:\n%s\n:\n",protoInString);
	#endif

	/* parse this string */

	if (X3DParse (myGroup,protoInString)) {
		#ifdef X3DPARSERVERBOSE
		printf ("PARSED OK\n");
		#endif

		if (ProtoInstanceTable[curProtoInsStackInd].container == ID_UNDEFINED) 
			pf = FIELDNAMES_children;
		else
			pf = ProtoInstanceTable[curProtoInsStackInd].container;
		myGroup->_defaultContainer = pf;
	
		#ifdef X3DPARSERVERBOSE
		printf ("expandProtoInstance cpsi %d, the proto's container is %s and as an id %d\n", curProtoInsStackInd, 
			FIELDNAMES[pf],myGroup->_defaultContainer);
		#endif
	} else {
		#ifdef X3DPARSERVERBOSE
		printf ("DID NOT PARSE THAT WELL:\n%s\n:\n",protoInString);
		#endif
	}

	/* remove the ProtoInstance calls from this stack */
	for (i=0; i<ProtoInstanceTable[curProtoInsStackInd].paircount; i++) {
		FREE_IF_NZ (ProtoInstanceTable[curProtoInsStackInd].name[i]);
		FREE_IF_NZ (ProtoInstanceTable[curProtoInsStackInd].value[i]);
	}
	ProtoInstanceTable[curProtoInsStackInd].paircount = 0;

	#ifdef X3DPARSERVERBOSE
	printf ("expandProtoInstance: decrementing curProtoInsStackInd from %d\n",curProtoInsStackInd);
	#endif

	linkNodeIn();
	DECREMENT_PARENTINDEX
	curProtoInsStackInd--;
	FREE_IF_NZ(protoInString);
        FREE_IF_NZ(origString);
}
#undef X3DPARSERVERBOSE

void parseProtoBody (const char **atts) {
	int i;

	#ifdef X3DPARSERVERBOSE
	TTY_SPACE
	printf ("start of parseProtoBody\n");
	#endif

	parserMode = PARSING_PROTOBODY;
}

void parseProtoDeclare (const char **atts) {
	int i,count;
	int foundName = FALSE;
	int nameIndex = ID_UNDEFINED;

	/* increment the currentProtoDeclare field. Check to see how many PROTOS we (bounds check) */
	currentProtoDeclare++;
	curProDecStackInd++;

	parserMode = PARSING_PROTODECLARE;
	

	for (count = 0; atts[count]; count += 2) {
		#ifdef X3DPARSERVERBOSE
		TTY_SPACE
		printf ("parseProtoDeclare: field:%s=%s\n", atts[count], atts[count + 1]);
		#endif

		if (strcmp("name",atts[count]) == 0) {nameIndex=count+1;}
		else if ((strcmp("appinfo", atts[count]) != 0)  ||
			(strcmp("documentation",atts[count]) != 0)) {
			ConsoleMessage ("found field :%s: in a ProtoDeclare",atts[count]);
		}
	}

	/* did we find the name? */
	if (nameIndex != ID_UNDEFINED) {
		/* found it, lets open a new PROTO for this one */
		registerProto(atts[nameIndex]);
	} else {
		ConsoleMessage ("\"ProtoDeclare\" found, but field \"name\" not found!\n");
	}
}

/* simple sanity check, and change mode */
void parseProtoInterface (const char **atts) {
	if (parserMode != PARSING_PROTODECLARE) {
		ConsoleMessage ("got a <ProtoInterface>, but not within a <ProtoDeclare>\n");
	}
	parserMode = PARSING_PROTOINTERFACE;
}


/* for initializing the script fields, make up a default value, in case the user has not specified one */
void parseScriptFieldDefaultValue(int type, union anyVrml *value) {
	switch (type) {
		case FIELDTYPE_SFFloat: value->sffloat = 0.0; break;
		case FIELDTYPE_MFFloat: value->mffloat.n=0; break;
		case FIELDTYPE_SFRotation: value->sfrotation.r[0] =0.0; value->sfrotation.r[1]=0.0; value->sfrotation.r[2] = 0.0; value->sfrotation.r[3] = 1.0; break;
		case FIELDTYPE_MFRotation: value->mfrotation.n=0; break;
		case FIELDTYPE_SFVec3f: value->sfvec3f.c[0] =0.0; value->sfvec3f.c[1]=0.0; value->sfvec3f.c[2] = 0.0; break;
		case FIELDTYPE_MFVec3f: value->mfvec3f.n=0; break;
		case FIELDTYPE_SFBool: value->sfbool=FALSE; break;
		case FIELDTYPE_MFBool: value->mfbool.n=0; break;
		case FIELDTYPE_SFInt32: value->sfint32 = 0; break;
		case FIELDTYPE_MFInt32: value->mfint32.n = 0; break;
		case FIELDTYPE_SFNode: value->sfnode = NULL; break;
		case FIELDTYPE_MFNode: value->mfnode.n = 0; break;
		case FIELDTYPE_SFColor: value->sfcolor.c[0] =0.0; value->sfcolor.c[1]=0.0; value->sfcolor.c[2] = 0.0; break;
		case FIELDTYPE_MFColor: value->mfcolor.n=0; break;
		case FIELDTYPE_SFColorRGBA: value->sfcolorrgba.r[0] =0.0; value->sfcolorrgba.r[1]=0.0; value->sfcolorrgba.r[2] = 0.0; value->sfcolorrgba.r[3] = 1.0; break;
		case FIELDTYPE_MFColorRGBA: value->mfcolorrgba.n = 0; break;
		case FIELDTYPE_SFTime: value->sftime = 0.0; break;
		case FIELDTYPE_MFTime: value->mftime.n=0; break;
		case FIELDTYPE_SFString: value->sfstring=newASCIIString(""); break;

		case FIELDTYPE_MFString: value->mfstring.n=0; break;
		case FIELDTYPE_SFVec2f: value->sfvec2f.c[0] =0.0; value->sfvec2f.c[1]=0.0; break;
		case FIELDTYPE_MFVec2f: value->mfvec2f.n=0; break;
		case FIELDTYPE_SFImage: value->sfimage.n=0;
		default: ConsoleMessage ("X3DProtoScript - can't parse default field value for script init");
	}
}


/* parse a script or proto field. Note that they are in essence the same, just used differently */
void parseScriptProtoField(const char **atts) {
	int i;
	uintptr_t myScriptNumber;
	int myparams[MPFIELDS];
	int strl;
	int which;
	int myFieldNumber;
	char *myValueString = NULL;
	union anyVrml value;
	int myAccessType;


	/* configure internal variables, and check sanity for top of stack This should be a Script node */
	if (parserMode == PARSING_SCRIPT) {
		if (parentStack[parentIndex]->_nodeType != NODE_Script) {
			ConsoleMessage ("X3DParser, line %d, expected the parent to be a Script node",LINE);
			printf ("X3DParser, parentIndex is %d\n",parentIndex);
			return;
		}
		myScriptNumber = ((struct X3D_Script *)parentStack[parentIndex])->_X3DScript;
	} else {
		myScriptNumber = currentProtoDeclare;

		#ifdef X3DPARSERVERBOSE
		TTY_SPACE
		printf ("parseScriptProtoField, working on proto idNumber %d\n",myScriptNumber);
		#endif
	}


	/* set up defaults for field parsing */
	for (i=0;i<MPFIELDS;i++) myparams[i] = ID_UNDEFINED;
	

	/* copy the fields over */
	/* have a "key" "value" pairing here. They can be in any order; put them into our order */
	for (i = 0; atts[i]; i += 2) {
		#ifdef X3DPARSERVERBOSE
		TTY_SPACE
		printf ("parseScriptProtoField, looking at %s=\"%s\"\n",atts[i],atts[i+1]);
		#endif

		/* skip any "appinfo" field here */
		if (strcmp(atts[i],"appinfo") != 0) {
			if (strcmp(atts[i],"name") == 0) { which = MP_NAME;
			} else if (strcmp(atts[i],"accessType") == 0) { which = MP_ACCESSTYPE;
			} else if (strcmp(atts[i],"type") == 0) { which = MP_TYPE;
			} else if (strcmp(atts[i],"value") == 0) { which = MP_VALUE;
			} else {
				ConsoleMessage ("X3D Proto/Script parsing line %d: unknown field type %s",LINE,atts[i]);
				return;
			}
			if (myparams[which] != ID_UNDEFINED) {
				ConsoleMessage ("X3DScriptParsing line %d, field %s already defined in this Script/Proto",
					LINE,atts[i],atts[i+1]);
			}
	
			/* record the index for the value of this field */
			myparams[which] = i+1;
		}
	}

	#ifdef X3DPARSERVERBOSE
	printf ("myparams:\n	%d\n	%d\n	%d	%d\n",myparams[MP_NAME],myparams[MP_ACCESSTYPE],myparams[MP_TYPE], myparams[MP_VALUE]);
	#endif

	/* ok now, we have a couple of checks to make here. Do we have all the parameters required? */
	if (myparams[MP_NAME] == ID_UNDEFINED) {
		ConsoleMessage("have a Script/PROTO at line %d with no parameter name",LINE);
		return;
	}
	if (myparams[MP_TYPE] == ID_UNDEFINED) {
		ConsoleMessage("have a Script/PROTO at line %d with no parameter type",LINE);
		return;
	}

	if (myparams[MP_ACCESSTYPE] == ID_UNDEFINED) {
		ConsoleMessage ("have a Script/PROTO at line %d with no paramater accessType ",LINE);
		return;
	}

	if (myparams[MP_VALUE] != ID_UNDEFINED) myValueString = atts[myparams[MP_VALUE]];

	/* register this field with the Javascript Field Indexer */
	myFieldNumber = JSparamIndex((char *)atts[myparams[MP_NAME]],(char *)atts[myparams[MP_TYPE]]);


	/* convert eventIn, eventOut, field, and exposedField to new names */
	myAccessType = findFieldInPROTOKEYWORDS(atts[myparams[MP_ACCESSTYPE]]);
	switch (myAccessType) {
		case PKW_eventIn: myAccessType = PKW_inputOnly;
		case PKW_eventOut: myAccessType = PKW_outputOnly;
		case PKW_exposedField: myAccessType = PKW_inputOutput;
		case PKW_field: myAccessType = PKW_initializeOnly;
		default: {}
	}
	
	registerX3DScriptField(myScriptNumber,
		findFieldInFIELDTYPES(atts[myparams[MP_TYPE]]),
		myAccessType,
		myFieldNumber,atts[myparams[MP_NAME]],myValueString);

	/* and initialize it if a Script */
	if (parserMode == PARSING_SCRIPT) {
		/* parse this string value into a anyVrml union representation */
		if (myValueString != NULL)
			Parser_scanStringValueToMem(X3D_NODE(&value), 0, findFieldInFIELDTYPES(atts[myparams[MP_TYPE]]), myValueString);
		else
			parseScriptFieldDefaultValue(findFieldInFIELDTYPES(atts[myparams[MP_TYPE]]), &value);
		
		/* send in the script field for initialization */
		InitScriptFieldC (myScriptNumber, myAccessType, findFieldInFIELDTYPES(atts[myparams[MP_TYPE]]),atts[myparams[MP_NAME]],value);
	}
}

/* we get script text from a number of sources; from the URL field, from CDATA, from a file
   pointed to by the URL field, etc... handle the data in one place */

void initScriptWithScript() {
	uintptr_t myScriptNumber;
	char *startingIndex;
	jsval rval;
	struct X3D_Script * me;
	char *myText = NULL;
	int i;
	struct Uni_String *myUni;
	char *mypath;
	char *thisurl;
	int count;
	char filename[1000];
	char firstBytes[4];
	int fromFile = FALSE;

	/* semantic checking... */
	me = (struct X3D_Script *)parentStack[parentIndex];

	if (me->_nodeType != NODE_Script) {
		ConsoleMessage ("initScriptWithScript - Expected to find a NODE_Script, got a %s\n",
		stringNodeType(me->_nodeType));
		return;
	}

	#ifdef X3DPARSERVERBOSE
	printf ("endElement: scriptText is %s\n",scriptText);
	#endif

	myScriptNumber = me->_X3DScript;

	/* did the script text come from a CDATA node?? */
	if (scriptText != NULL) if (scriptText[0] != '\0') myText = scriptText;

	/* do we still have nothing? Look in the url node for a file or a script. */
	if (myText == NULL) {
	        /* lets make up the path and save it, and make it the global path */
	        /* copy the parent path over */
	        mypath = STRDUP(me->__parenturl->strptr);
	        removeFilenameFromPath (mypath);

		/* try the first url, up to the last, until we find a valid one */
		count = 0;
		while (count < me->url.n) {
			thisurl = me->url.p[count]->strptr;

			/* leading whitespace removal */
			while ((*thisurl <= ' ') && (*thisurl != '\0')) thisurl++;

			/* is thisurl a vrml/ecma/javascript string?? */
			if ((strstr(thisurl,"ecmascript:")!= 0) ||
				(strstr(thisurl,"vrmlscript:")!=0) ||
				(strstr(thisurl,"javascript:")!=0)) {
				myText = thisurl;
				break;
			} else {
				/* check to make sure we don't overflow */
				if ((strlen(thisurl)+strlen(mypath)) > 900) { 
					ConsoleMessage ("url is waaaay too long for me.");
					return;
				}

				/* we work in absolute filenames... */
				makeAbsoluteFileName(filename,mypath,thisurl);

				if (fileExists(filename,firstBytes,TRUE)) {
					myText = readInputString(filename,"");
					fromFile = TRUE;
					break;
				}
			}
			count ++;
		}
/* error condition, if count >= me->url.n */
	}

	/* still have a problem here? */
	if (myText == NULL) {
		ConsoleMessage ("could not find Script text in url or CDATA");
		FREE_IF_NZ(scriptText);
		return;
	}

	/* peel off the ecmascript etc unless it was read in from a file */
	if (!fromFile) {
		startingIndex = strstr(myText,"ecmascript:");
		if (startingIndex != NULL) { startingIndex += strlen ("ecmascript:");
		} else if (startingIndex == NULL) {
			startingIndex = strstr(myText,"vrmlscript:");
			if (startingIndex != NULL) startingIndex += strlen ("vrmlscript:");
		} else if (startingIndex == NULL) {
			startingIndex = strstr(myText,"javascript:");
			if (startingIndex != NULL) startingIndex += strlen ("javacript:");
		} else {
			/* text is from a file in the URL field */
			startingIndex = myText;
		}
	} else {
		startingIndex = myText; /* from a file, no ecmascript: required */
	}

	if (startingIndex == NULL) {
		ConsoleMessage ("X3DParser, line %d have Script node, but no valid script",LINE);
		FREE_IF_NZ(scriptText);
		return;
	}

	if (!ACTUALRUNSCRIPT (myScriptNumber, startingIndex, &rval)) {
		ConsoleMessage ("X3DParser, script initialization error at line %d",LINE);
		FREE_IF_NZ(scriptText);
		return;
	}

	FREE_IF_NZ(scriptText);
	scriptTextMallocSize = 0;
	parserMode = PARSING_NODES;
	#ifdef X3DPARSERVERBOSE
	printf ("endElement: got END of script - script should be registered\n");
	#endif

}

void addToProtoCode(const char *name) {
        if (PROTONames[currentProtoDeclare].fileOpen) 
                PROTONames[currentProtoDeclare].charLen += fprintf (PROTONames[currentProtoDeclare].fileDescriptor,"</%s>\n",name);
}

void endProtoDeclare(void) {

		#ifdef X3DPARSERVERBOSE
		TTY_SPACE
		printf ("endElement, end of ProtoDeclare %d stack %d\n",currentProtoDeclare,curProDecStackInd);
		#endif

		/* decrement the protoDeclare stack count. If we are nested, get out of the nesting */
		curProDecStackInd--;

		/* now, a ProtoDeclare should be within normal nodes; make the expected mode PARSING_NODES, assuming
		   we don't have nested ProtoDeclares  */
		if (curProDecStackInd == 0) parserMode = PARSING_NODES;

		if (curProDecStackInd < 0) {
			ConsoleMessage ("X3D_Parser found too many end ProtoDeclares at line %d\n",LINE);
			curProDecStackInd = 0; /* reset it */
		}
}

/* we are doing a ProtoInstance, and we do not have a fieldValue in the Instance for a parameter. See if it is
   available in the ProtoInterface. */
int getFieldValueFromProtoInterface (char *fieldName, int protono, char **value) {
	int ctr;
	struct Uni_String *tmp;
	int len;

	#ifdef X3DPARSERVERBOSE
	printf ("getFieldValueFromProtoInterface, looking for %s\n",fieldName);
	#endif
	

	len = strlen(fieldName) +1; /* len in Uni_String has the '\0' on it */
	
        for (ctr=0; ctr<=ScriptFieldTableSize; ctr++) {
		if (protono == ScriptFieldNames[ctr].scriptNumber) {
                	tmp = ScriptFieldNames[ctr].fieldName;
                	if (strcmp(fieldName,tmp->strptr)==0) {
				*value = ScriptFieldNames[ctr].value->strptr;
				return TRUE;
			}
		}
	}

	/* did not find it. */
	*value = "";
	return FALSE;
}

/* record each field of each script - the type, kind, name, and associated script */
void registerX3DScriptField(int myScriptNumber,int type,int kind, int myFieldOffs, char *name, char *value) {

	/* semantic check */
	if ((parserMode != PARSING_SCRIPT) && (parserMode != PARSING_PROTOINTERFACE)) {
		ConsoleMessage ("registerX3DScriptField: wrong mode - got %d\n",parserMode);
	}

	ScriptFieldTableSize ++;

	#ifdef X3DPARSERVERBOSE
	printf ("registering script field %s script %d index %d\n",name,myScriptNumber,ScriptFieldTableSize);
	printf ("	type %d kind %d fieldOffs %d\n",type,kind,myFieldOffs);
	#endif


	/* ok, we got a name and a type */
	if (ScriptFieldTableSize >= MAXScriptFieldParams) {
		/* oooh! not enough room at the table */
		MAXScriptFieldParams += 100; /* arbitrary number */
		ScriptFieldNames = (struct ScriptFieldStruct*)REALLOC (ScriptFieldNames, sizeof(*ScriptFieldNames) * MAXScriptFieldParams);
	}

	ScriptFieldNames[ScriptFieldTableSize].scriptNumber = myScriptNumber;
	ScriptFieldNames[ScriptFieldTableSize].fieldName = newASCIIString(name);
	if (value == NULL) ScriptFieldNames[ScriptFieldTableSize].value = NULL;
	else ScriptFieldNames[ScriptFieldTableSize].value = newASCIIString(value);
	ScriptFieldNames[ScriptFieldTableSize].fromScriptNotPROTO = parserMode == PARSING_SCRIPT;
	ScriptFieldNames[ScriptFieldTableSize].type = type;
	ScriptFieldNames[ScriptFieldTableSize].kind = kind;
	ScriptFieldNames[ScriptFieldTableSize].offs = myFieldOffs;
}


/* look through the script fields for this field, and return the values. */
int getFieldFromScript (char *fieldName, int scriptno, int *offs, int *type) {
	int ctr;
	struct Uni_String *tmp;
	int len;

	#ifdef X3DPARSERVERBOSE
	printf ("getFieldFromScript, looking for %s\n",fieldName);
	#endif
	
	len = strlen(fieldName) +1; /* len in Uni_String has the '\0' on it */
	
        for (ctr=0; ctr<=ScriptFieldTableSize; ctr++) {
		if (scriptno == ScriptFieldNames[ctr].scriptNumber) {
                	tmp = ScriptFieldNames[ctr].fieldName;
                	if (strcmp(fieldName,tmp->strptr)==0) {
				*offs = ScriptFieldNames[ctr].offs;
				*type = ScriptFieldNames[ctr].type;
				#ifdef X3DPARSERVERBOSE
				printf ("getFieldFromScript - returning offset %d type %d (kind %d)\n",*offs,*type,
					ScriptFieldNames[ctr].kind);
				#endif
                	        return TRUE;
			}
                }
        }
        
	#ifdef X3DPARSERVERBOSE
	printf ("getFieldFromScript, did not find field %s in script %d\n",fieldName,scriptno);
	#endif
	
	/* did not find it */
	*offs = ID_UNDEFINED;  *type = 0;
	return FALSE;
}
