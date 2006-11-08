/*
 * Copyright (C) 1998 Tuomas J. Lukka, 2002 John Stewart, Ayla Khan CRC Canada
 * DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
 * See the GNU Library General Public License
 * (file COPYING in the distribution) for conditions of use and
 * redistribution, EXCEPT on the files which belong under the
 * Mozilla public license.
 *
 * $Id$
 *
 */

#include "headers.h"
#include "jsVRMLBrowser.h"

/* we add/remove routes with this call */
void jsRegisterRoute(
	struct X3D_Node* from, int fromOfs,
	struct X3D_Node* to, int toOfs,
	int len, const char *adrem) {
 	char tonode_str[15];
 	snprintf(tonode_str, 15, "%lu:%d", to, toOfs);
	int ad;

	if (strncmp("addRoute",adrem,strlen("addRoute")) == 0) 
		ad = 1;
	else ad = 0;

 	CRoutes_Register(ad, from, fromOfs, 1, tonode_str, len, 
 		 returnInterpolatorPointer(stringNodeType(to->_nodeType)), 0, 0);
}
 

/* used in loadURL*/
void conCat (char *out, char *in) {

	while (strlen (in) > 0) {
		strcat (out," :loadURLStringBreak:");
		while (*out != '\0') out++;

		if (*in == '[') in++;
		while ((*in != '\0') && (*in == ' ')) in++;
		if (*in == '"') {
			in++;
			/* printf ("have the initial quote string here is %s\n",in); */
			while (*in != '"') { *out = *in; out++; in++; }
			*out = '\0';
			/* printf ("found string is :%s:\n",tfilename); */
		}

		/* skip along to the start of the next name */
		if (*in == '"') in++;
		if (*in == ',') in++;
		if (*in == ']') in++; /* this allows us to leave */
	}
}



void createLoadUrlString(char *out, int outLen, char *url, char *param) {
	int commacount1;
	int commacount2;
	char *tptr;
	char *orig;

	/* mimic the EAI loadURL, java code is:
        // send along sizes of the Strings
        SysString = "" + url.length + " " + parameter.length;
                
        for (count=0; count<url.length; count++) {
                SysString = SysString + " :loadURLStringBreak:" + url[count];
        }       

        for (count=0; count<parameter.length; count++) {
                SysString = SysString + " :loadURLStringBreak:" + parameter[count];
        }
	*/

	/* keep an original copy of the pointer */
	orig = out;
	
	/* find out how many elements there are */

	commacount1 = 0; commacount2 = 0;
	tptr = url; while (*tptr != '\0') { if (*tptr == '"') commacount1 ++; tptr++; }
	tptr = param; while (*tptr != '\0') { if (*tptr == '"') commacount2 ++; tptr++; }
	commacount1 = commacount1 / 2;
	commacount2 = commacount2 / 2;

	if ((	strlen(url) +
		strlen(param) +
		(commacount1 * strlen (" :loadURLStringBreak:")) +
		(commacount2 * strlen (" :loadURLStringBreak:"))) > (outLen - 20)) {
		printf ("createLoadUrlString, string too long\n");
		return;
	}

	sprintf (out,"%d %d",commacount1,commacount2);
	
	/* go to the end of this string */
	while (*out != '\0') out++;

	/* go through the elements and find which (if any) url exists */	
	conCat (out,url);
	while (*out != '\0') out++;
	conCat (out,param);
}



JSBool
VrmlBrowserInit(JSContext *context, JSObject *globalObj, BrowserNative *brow)
{
	JSObject *obj;

	#ifdef JSVERBOSE
		printf("VrmlBrowserInit\n");
	#endif

	obj = JS_DefineObject(context, globalObj, "Browser", &Browser, NULL, 
			JSPROP_ENUMERATE | JSPROP_PERMANENT);
	if (!JS_DefineFunctions(context, obj, BrowserFunctions)) {
		printf( "JS_DefineFunctions failed in VrmlBrowserInit.\n");
		return JS_FALSE;
	}

	if (!JS_SetPrivate(context, obj, brow)) {
		printf( "JS_SetPrivate failed in VrmlBrowserInit.\n");
		return JS_FALSE;
	}
	return JS_TRUE;
}


JSBool
VrmlBrowserGetName(JSContext *context, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	JSString *_str;

	UNUSED(obj);
	UNUSED(argc);
	UNUSED(argv);

	_str = JS_NewString(context,BrowserName,strlen(BrowserName)+1);
	*rval = STRING_TO_JSVAL(_str);
	return JS_TRUE;
}


/* get the string stored in global BrowserVersion into a jsObject */
JSBool
VrmlBrowserGetVersion(JSContext *context, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	JSString *_str;

	UNUSED(obj);
	UNUSED(argc);
	UNUSED(argv);

	_str = JS_NewString(context,BrowserVersion,strlen(BrowserVersion)+1);
	*rval = STRING_TO_JSVAL(_str);
	return JS_TRUE;
}


JSBool
VrmlBrowserGetCurrentSpeed(JSContext *context, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	JSString *_str;
	char string[1000];

	UNUSED(obj);
	UNUSED(argc);
	UNUSED(argv);

	/* get the variable updated */
	getCurrentSpeed();
	sprintf (string,"%f",BrowserSpeed);
	_str = JS_NewString(context,string,strlen(string)+1);
	*rval = STRING_TO_JSVAL(_str);
	return JS_TRUE;
}


JSBool
VrmlBrowserGetCurrentFrameRate(JSContext *context, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	JSString *_str;
	char FPSstring[1000];

	UNUSED(obj);
	UNUSED(argc);
	UNUSED(argv);

	sprintf (FPSstring,"%6.2f",BrowserFPS);
	_str = JS_NewString(context,FPSstring,strlen(FPSstring)+1);
	*rval = STRING_TO_JSVAL(_str);
	return JS_TRUE;
}


JSBool
VrmlBrowserGetWorldURL(JSContext *context, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	JSString *_str;

	UNUSED(obj);
	UNUSED(argc);
	UNUSED(argv);

	_str = JS_NewString(context,BrowserFullPath,strlen(BrowserFullPath)+1);
	*rval = STRING_TO_JSVAL(_str);
	return JS_TRUE;
}


JSBool
VrmlBrowserReplaceWorld(JSContext *context, JSObject *obj,
						uintN argc, jsval *argv, jsval *rval)
{
	JSObject *_obj;
	JSString *_str;
	JSClass *_cls;
	jsval _rval = INT_TO_JSVAL(0);
	char *_c_args = "MFNode nodes",
		*_costr,
		*_c_format = "o";
	char *tptr;

	/*FUNC_INIT*/

	if (JS_ConvertArguments(context, argc, argv, _c_format, &_obj)) {
		if ((_cls = JS_GetClass(_obj)) == NULL) {
			printf("JS_GetClass failed in VrmlBrowserReplaceWorld.\n");
			return JS_FALSE;
		}

		if (memcmp("MFNode", _cls->name, strlen(_cls->name)) != 0) {
			printf( "\nIncorrect argument in VrmlBrowserReplaceWorld.\n");
			return JS_FALSE;
		}
		_str = JS_ValueToString(context, argv[0]);
		_costr = JS_GetStringBytes(_str);

		/* sanitize string, for the EAI_RW call (see EAI_RW code) */
		tptr = _costr;
		while (*tptr != '\0') {
			if(*tptr == '[') *tptr = ' ';
			if(*tptr == ']') *tptr = ' ';
			if(*tptr == ',') *tptr = ' ';
			tptr++;
		}
		EAI_RW(_costr);

	} else {
		printf( "\nIncorrect argument format for replaceWorld(%s).\n", _c_args);
		return JS_FALSE;
	}
	*rval = _rval;

	return JS_TRUE;
}


JSBool
VrmlBrowserLoadURL(JSContext *context, JSObject *obj,
				   uintN argc, jsval *argv, jsval *rval)
{
	JSObject *_obj[2];
	JSString *_str[2];
	JSClass *_cls[2];
	char *_c_args = "MFString url, MFString parameter",
		*_costr[2],
		*_c_format = "o o";
	#define myBufSize 2000
	char myBuf[myBufSize];

	/*FUNC_INIT*/

	if (JS_ConvertArguments(context, argc, argv, _c_format, &(_obj[0]), &(_obj[1]))) {
		if ((_cls[0] = JS_GetClass(_obj[0])) == NULL) {
			printf( "JS_GetClass failed for arg 0 in VrmlBrowserLoadURL.\n");
			return JS_FALSE;
		}
		if ((_cls[1] = JS_GetClass(_obj[1])) == NULL) {
			printf( "JS_GetClass failed for arg 1 in VrmlBrowserLoadURL.\n");
			return JS_FALSE;
		}
		if (memcmp("MFString", (_cls[0])->name, strlen((_cls[0])->name)) != 0 &&
			memcmp("MFString", (_cls[1])->name, strlen((_cls[1])->name)) != 0) {
			printf( "\nIncorrect arguments in VrmlBrowserLoadURL.\n");
			return JS_FALSE;
		}
		_str[0] = JS_ValueToString(context, argv[0]);
		_costr[0] = JS_GetStringBytes(_str[0]);

		_str[1] = JS_ValueToString(context, argv[1]);
		_costr[1] = JS_GetStringBytes(_str[1]);

		/* we use the EAI code for this - so reformat this for the EAI format */
		extern struct X3D_Anchor EAI_AnchorNode;

		/* make up the URL from what we currently know */
		createLoadUrlString(myBuf,myBufSize,_costr[0], _costr[1]);
		createLoadURL(myBuf);

		/* now tell the EventLoop that BrowserAction is requested... */
		AnchorsAnchor = &EAI_AnchorNode;
		BrowserAction = TRUE;


	} else {
		printf( "\nIncorrect argument format for loadURL(%s).\n", _c_args);
		return JS_FALSE;
	}
	*rval = INT_TO_JSVAL(0);

	return JS_TRUE;
}


JSBool
VrmlBrowserSetDescription(JSContext *context, JSObject *obj,
						  uintN argc, jsval *argv, jsval *rval)
{
	char *_c, *_c_args = "SFString description", *_c_format = "s";

	/*FUNC_INIT*/

	if (argc == 1 &&
		JS_ConvertArguments(context, argc, argv, _c_format, &_c)) {

		/* we do not do anything with the description. If we ever wanted to, it is in _c */
		*rval = INT_TO_JSVAL(0);
	} else {
		printf( "\nIncorrect argument format for setDescription(%s).\n", _c_args);
		return JS_FALSE;
	}
	return JS_TRUE;
}


JSBool
VrmlBrowserCreateVrmlFromString(JSContext *context, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	char *_c, *_c_args = "SFString vrmlSyntax", *_c_format = "s";

	/* for the return of the nodes */
	uintptr_t nodarr[200];
	char *xstr; 
	char *tmpstr;
	int ra;
	int count;
	int wantedsize;
	int mallocdsize;
	

	/* make this a default value */
	*rval = INT_TO_JSVAL(0);

	/*FUNC_INIT*/

	if (argc == 1 &&
		JS_ConvertArguments(context, argc, argv, _c_format, &_c)) {
		#ifdef JSVERBOSE
			printf("VrmlBrowserCreateVrmlFromString: obj = %u, str = \"%s\"\n",
				   obj, _c);
		#endif

		/* do the call to make the VRML code */
		ra = EAI_CreateVrml("String",_c,nodarr,200);
		#ifdef JSVERBOSE
		printf ("EAI_CreateVrml returns %d nodes\n",ra);
		printf ("nodes %d %d\n",nodarr[0],nodarr[1]);
		#endif

		/* and, make a string that we can use to create the javascript object */
		mallocdsize = 200;
		xstr = malloc (mallocdsize);
		strcpy (xstr,"new MFNode(");
		for (count=0; count<ra; count += 2) {
			tmpstr = malloc(strlen(_c) + 100);
			sprintf (tmpstr,"new SFNode('%s','%d')",_c,nodarr[count*2+1]);
			wantedsize = strlen(tmpstr) + strlen(xstr);
			if (wantedsize > mallocdsize) {
				mallocdsize = wantedsize +200;
				xstr = realloc (xstr,mallocdsize);
			}
			
			
			strncat (xstr,tmpstr,strlen(tmpstr));
			free (tmpstr);
		}
		strcat (xstr,")");
		
		/* create this value NOTE: rval is set here. */
		jsrrunScript(context, obj, xstr, rval);
		free (xstr);

	} else {
		printf("\nIncorrect argument format for createVrmlFromString(%s).\n", _c_args);
		return JS_FALSE;
	}

	return JS_TRUE;
}

#define JSVERBOSE

JSBool
VrmlBrowserCreateVrmlFromURL(JSContext *context, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
	JSObject *_obj[2];
	JSString *_str[2];
	JSClass *_cls[2];
	jsval _v;
	char *_c,
		*_c_args = "MFString url, SFNode node, SFString event",
		*_costr0,
		*_costr1,
		*_c_format = "o o s";
	uintptr_t nodarr[200];
	uintptr_t myptr;
	int ra;
	#define myFileSizeLimit 4000
	char filename[myFileSizeLimit];
	char tfilename [myFileSizeLimit];
	char *tfptr; 
	char *coptr;
	char firstBytes[4];
	char *bfp;
	char *slashindex;
	int found;
	int count;

	/* rval is always zero, so lets just set it */
	*rval = INT_TO_JSVAL(0);

printf ("must fix Javascript VrmlBrowserCreateVrmlFromURL\n");
	/*FUNC_INIT*/

	if (JS_ConvertArguments(context, argc, argv, _c_format,
			&(_obj[0]), &(_obj[1]), &_c)) {
		if ((_cls[0] = JS_GetClass(_obj[0])) == NULL) {
			printf( "JS_GetClass failed for arg 0 in VrmlBrowserCreateVrmlFromURL.\n");
			return JS_FALSE;
		}
		if ((_cls[1] = JS_GetClass(_obj[1])) == NULL) {
			printf("JS_GetClass failed for arg 1 in VrmlBrowserCreateVrmlFromURL.\n");
			return JS_FALSE;
		}
		if (memcmp("MFString", (_cls[0])->name, strlen((_cls[0])->name)) != 0 &&
			memcmp("SFNode", (_cls[1])->name, strlen((_cls[1])->name)) != 0) {
			printf( "Incorrect arguments in VrmlBrowserCreateVrmlFromURL.\n");
			return JS_FALSE;
		}

		_str[0] = JS_ValueToString(context, argv[0]);
		_costr0 = JS_GetStringBytes(_str[0]);
printf ("costr0 %s\n",_costr0);

		if (!JS_GetProperty(context, _obj[1], "__handle", &_v)) {
			printf("JS_GetProperty failed for \"__handle\" in VrmlBrowserCreateVrmlFromURL.\n");
			return JS_FALSE;
		}
		_str[1] = JS_ValueToString(context, _v);
		_costr1 = JS_GetStringBytes(_str[1]);
printf ("costr1 %d from %d\n",_costr1,_v);
		ra = sscanf (_costr1,"%d",&myptr);
printf ("scanf returns %d\n",ra);
printf ("myptr %d\n",myptr);


		/* bounds checks */
		if (sizeof (_costr0) > (myFileSizeLimit-200)) {
			printf ("VrmlBrowserCreateVrmlFromURL, url too long...\n"); return;
		}

		/* ok - here we have:
			_costr0	: the url string array; eg: [ "vrml.wrl" ]
			_costr1	: the handle (memory pointer) of the node to send this to, eg 143203016
			_c	: the field to send this to, eg: addChildren
		*/

		/* find a file name that exists. If not, return JS_FALSE */
		bfp = strdup(BrowserFullPath);
		/* and strip off the file name, leaving any path */
		slashindex = (char *) rindex(bfp, ((int) '/'));
		if (slashindex != NULL) {
			slashindex ++; /* leave the slash there */
			*slashindex = 0;
		} else {bfp[0] = 0;}

		/* go through the elements and find which (if any) url exists */	
		found = FALSE;
		coptr = _costr0;

		while (!found) {
			tfptr = tfilename;
			/*printf ("start of loop, coptr :%s:\n",coptr); */
			if (*coptr == '[') coptr++;
			while ((*coptr != '\0') && (*coptr == ' ')) coptr++;
			if (*coptr == '\0') {
				ConsoleMessage ("javascript: could not find a valid url in %s",_costr0);
				return JS_FALSE;
			}

			if (*coptr == '"') {
				coptr++;
				/* printf ("have the initial quote string here is %s\n",coptr); */
				while (*coptr != '"') {
					*tfptr = *coptr;
					tfptr++; coptr++;
				}
				*tfptr = '\0';
				/* printf ("found string is :%s:\n",tfilename); */
			}


        	        /* we work in absolute filenames... */
                	makeAbsoluteFileName(filename,bfp,tfilename);

                	if (fileExists(filename,firstBytes,TRUE)) {
				/* printf ("file exists, break\n"); */
				found = TRUE;
        	        } 
				#ifdef JSVERBOSE
				else printf ("nope, file %s does not exist\n",filename);
				#endif

			/* skip along to the start of the next name */
			if (*coptr == '"') coptr++;
			if (*coptr == ',') coptr++;
			if (*coptr == ']') coptr++; /* this allows us to error out, above */

		}


		/* call the parser */
		ra = EAI_CreateVrml("URL",filename,nodarr,200);

		/* now, we make up a string of nodes, pass it to setField_method1 that
		takes this string apart. oh well... */

		filename[0] = '\0'; /* just reuse these variables */
		
		for (count = 1; count < ra; count +=2) {
			sprintf (tfilename, "%d,",nodarr[count]);	
			strcat (filename, tfilename);
		}

		#ifdef JSVERBOSE
		printf ("node string is %s\n",filename);
		#endif

{
struct X3D_Node *myNode;

myNode = (struct X3D_Node* )myptr;

printf ("myptr points to a %s\n",stringNodeType(myNode->_nodeType));
}
		/* remember the freewrl addChildren removeChildren stuff? */
		if ((strncmp (_c,"addChildren",strlen("addChildren")) == 0) ||
		(strncmp (_c,"removeChildren",strlen("removeChildren")) == 0)) {
			setField_method1 ((uintptr_t *)myptr, "children", filename);
		} else {
			setField_method1 ((uintptr_t *)myptr, _c, filename);
		}

	} else {
		printf( "Incorrect argument format for createVrmlFromURL(%s).\n", _c_args);
		return JS_FALSE;
	}
	return JS_TRUE;
}
#undef JSVERBOSE


JSBool
VrmlBrowserAddRoute(JSContext *context, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	jsval _rval = INT_TO_JSVAL(0);
	if (!doVRMLRoute(context, obj, argc, argv, "addRoute")) {
		printf( "doVRMLRoute failed in VrmlBrowserAddRoute.\n");
		return JS_FALSE;
	}
	*rval = _rval;
	return JS_TRUE;
}


JSBool
VrmlBrowserPrint(JSContext *context, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{	int count;
	JSString *_str;
	char *_id_c;
	jsval _rval = INT_TO_JSVAL(0);

	UNUSED (context); UNUSED(obj);
	/* printf ("FreeWRL:javascript: "); */
	for (count=0; count < argc; count++) {
		if (JSVAL_IS_STRING(argv[count])) {
			_str = JSVAL_TO_STRING(argv[count]);
			_id_c = JS_GetStringBytes(_str);
			#ifdef AQUA
			ConsoleMessage(_id_c);
			#else
				#ifdef HAVE_NOTOOLKIT 
					printf ("%s", _id_c);
				#else
					printf ("%s\n", _id_c);
					ConsoleMessage(_id_c);
				#endif
			#endif
		} else {
	/*		printf ("unknown arg type %d\n",count); */
		}
	}
	#ifdef AQUA
	ConsoleMessage("\n");
	#else
		#ifdef HAVE_NOTOOLKIT
			printf ("\n");
		#endif
	#endif
	*rval = _rval;
	return JS_TRUE;
}
JSBool
VrmlBrowserDeleteRoute(JSContext *context, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	jsval _rval = INT_TO_JSVAL(0);
	if (!doVRMLRoute(context, obj, argc, argv, "deleteRoute")) {
		printf( "doVRMLRoute failed in VrmlBrowserDeleteRoute.\n");
		return JS_FALSE;
	}
	*rval = _rval;
	return JS_TRUE;

	printf ("VrmlBrowserDeleteRoute\n");
printf ("must fix doVRMLRoute\n");
}


static JSBool
doVRMLRoute(JSContext *context, JSObject *obj, uintN argc, jsval *argv,
			const char *callingFunc)
{
	jsval _v[2];
	JSObject *_obj[2];
	JSClass *_cls[2];
	JSString *_str[2];
	char 
		*_cstr[2],
		*_costr[2],
		*_c_args =
		"SFNode fromNode, SFString fromEventOut, SFNode toNode, SFString toEventIn",
		*_c_format = "o s o s";
	struct X3D_Node *fromNode;
	struct X3D_Node *toNode;
	int fromOfs, toOfs, len;
	int fromtype, totype;
	int xxx;
	int myField;

	/*FUNC_INIT*/
printf ("must fix doVRMLRoute\n");

	if (JS_ConvertArguments(context,
				argc,
				argv,
				_c_format,
				&(_obj[0]), &(_cstr[0]), &(_obj[1]), &(_cstr[1]))) {
		if ((_cls[0] = JS_GetClass(_obj[0])) == NULL) {
			printf("JS_GetClass failed for arg 0 in doVRMLRoute called from %s.\n",
					callingFunc);
			return JS_FALSE;
		}
		if ((_cls[1] = JS_GetClass(_obj[1])) == NULL) {
			printf("JS_GetClass failed for arg 2 in doVRMLRoute called from %s.\n",
					callingFunc);
			return JS_FALSE;
		}
		if (memcmp("SFNode", (_cls[0])->name, strlen((_cls[0])->name)) != 0 &&
			memcmp("SFNode", (_cls[1])->name, strlen((_cls[1])->name)) != 0) {
			printf("\nArguments 0 and 2 must be SFNode in doVRMLRoute called from %s(%s): %s\n",
					callingFunc, _c_args, callingFunc);
			return JS_FALSE;
		}

		if (!JS_GetProperty(context, _obj[0], "__handle", &(_v[0]))) {
			printf("JS_GetProperty failed for arg 0 and \"__handle\" in doVRMLRoute called from %s.\n",
					callingFunc);
			return JS_FALSE;
		}
		_str[0] = JS_ValueToString(context, _v[0]);
		_costr[0] = JS_GetStringBytes(_str[0]);

		if (!JS_GetProperty(context, _obj[1], "__handle", &(_v[1]))) {
			printf("JS_GetProperty failed for arg 2 and \"__handle\" in doVRMLRoute called from %s.\n",
					callingFunc);
			return JS_FALSE;
		}
		_str[1] = JS_ValueToString(context, _v[1]);
		_costr[1] = JS_GetStringBytes(_str[1]);

		/* printf ("routing from %s %s to %s %s\n", _costr[0], _cstr[0], _costr[1], _cstr[1]); */

		/* convert the "handles" into memory pointers */
		if ((sscanf (_costr[0],"%d",&fromNode) == 0) ||
			(sscanf (_costr[1],"%d",&toNode) == 0)) {
			printf ("JS Routing, problem converting %s or %s to node pointer\n",_costr[0],_costr[1]);
			return JS_FALSE;
		}

		/*
		printf ("routing from a node of type %s to a node of type %s\n",
			stringNodeType(fromNode->_nodeType), 
			stringNodeType(toNode->_nodeType));
		*/

		/* From field */
		if ((strncmp (_cstr[0],"addChildren",strlen("addChildren")) == 0) || 
		(strncmp (_cstr[0],"removeChildren",strlen("removeChildren")) == 0)) {
			myField = findFieldInALLFIELDNAMES("children");
		} else {
			/* try finding it, maybe with a "set_" or "changed" removed */
			myField = findRoutedFieldInFIELDNAMES(_cstr[0],0);
			if (myField == -1) 
				myField = findRoutedFieldInFIELDNAMES(_cstr[0],1);
		}

		/* find offsets, etc */
       		findFieldInOFFSETS(NODE_OFFSETS[fromNode->_nodeType], myField, &fromOfs, &fromtype, &xxx);

		/* To field */
		if ((strncmp (_cstr[1],"addChildren",strlen("addChildren")) == 0) || 
		(strncmp (_cstr[1],"removeChildren",strlen("removeChildren")) == 0)) {
			myField = findFieldInALLFIELDNAMES("children");
		} else {
			/* try finding it, maybe with a "set_" or "changed" removed */
			myField = findRoutedFieldInFIELDNAMES(_cstr[1],0);
			if (myField == -1) 
				myField = findRoutedFieldInFIELDNAMES(_cstr[1],1);
		}

		/* find offsets, etc */
       		findFieldInOFFSETS(NODE_OFFSETS[toNode->_nodeType], myField, &toOfs, &totype, &xxx);

		/* do we have a mismatch here? */
		if (fromtype != totype) {
			printf ("Javascript routing problem - can not route from %s to %s\n",
				stringNodeType(fromNode->_nodeType), 
				stringNodeType(toNode->_nodeType));
			return JS_FALSE;
		}

		len = returnRoutingElementLength(totype);

		jsRegisterRoute(fromNode, fromOfs, toNode, toOfs, len,callingFunc);
	} else {
		printf( "\nIncorrect argument format for %s(%s).\n",
				callingFunc, _c_args);
		return JS_FALSE;
	}

	return JS_TRUE;
}
