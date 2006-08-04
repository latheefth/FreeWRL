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

#define JSVERBOSE

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



void createLoadUrlString(char *out, char *url, char *param) {
	int commacount1;
	int commacount2;
	char *tptr;

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
	
	/* find out how many elements there are */

	commacount1 = 0; commacount2 = 0;
	tptr = url; while (*tptr != '\0') { if (*tptr == '"') commacount1 ++; tptr++; }
	tptr = param; while (*tptr != '\0') { if (*tptr == '"') commacount2 ++; tptr++; }
	commacount1 = commacount1 / 2;
	commacount2 = commacount2 / 2;

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
	char FPSstring[10];

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
	jsval _v, _rval = INT_TO_JSVAL(0);
	BrowserNative *brow;
	char *_c_args = "MFNode nodes",
		*_costr,
		*_c_format = "o";
	char *tptr;

	FUNC_INIT

	if (JS_ConvertArguments(context, argc, argv, _c_format, &_obj)) {
		if ((_cls = JS_GetClass(_obj)) == NULL) {
			printf("JS_GetClass failed in VrmlBrowserReplaceWorld.\n");
			return JS_FALSE;
		}

		if (memcmp("MFNode", _cls->name, strlen(_cls->name)) != 0) {
			printf( "\nIncorrect argument in VrmlBrowserReplaceWorld.\n");
			return JS_FALSE;
		}
		_str = JS_ValueToString(context, _obj);
		_costr = JS_GetStringBytes(_str);

printf ("DPCVA replaceworld str %s \n",_costr);/*		doPerlCallMethodVA(brow->sv_js, "jspBrowserReplaceWorld", "s", _costr);*/

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
	BrowserNative *brow;
	char *_c_args = "MFString url, MFString parameter",
		*_costr[2],
		*_c_format = "o o";
	char myBuf[2000];

	FUNC_INIT

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
		createLoadUrlString(myBuf,_costr[0], _costr[1]);
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
	jsval _rval = INT_TO_JSVAL(0);
	char *_c, *_c_args = "SFString description", *_c_format = "s";
	BrowserNative *brow;

	FUNC_INIT

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
	BrowserNative *brow;
	char *_c, *_c_args = "SFString vrmlSyntax", *_c_format = "s";
	JSString *_str;
	jsval _rval = INT_TO_JSVAL(0);

	/* for the return of the nodes */
	uintptr_t nodarr[200];
	char xstr[20000];
	char tmpstr[200];
	int ra;
	int count;
	

	/* make this a default value */
	*rval = INT_TO_JSVAL(0);

	FUNC_INIT

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
		strcpy (xstr,"Browser.__ret=new MFNode(");
		for (count=0; count<ra; count += 2) {
			sprintf (tmpstr,"new SFNode('bubba','%d')",nodarr[count*2+1]);
			strncat (xstr,tmpstr,strlen(tmpstr));
		}
		strcat (xstr,")");
		
printf ("going to call jsrunScript with %s\n",xstr);
		/* create this value NOTE: rval is set here. */
		jsrrunScript(context, obj, xstr, rval);

	} else {
		printf("\nIncorrect argument format for createVrmlFromString(%s).\n", _c_args);
		return JS_FALSE;
	}

	return JS_TRUE;
}


JSBool
VrmlBrowserCreateVrmlFromURL(JSContext *context, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
	JSObject *_obj[2];
	JSString *_str[2];
	JSClass *_cls[2];
	jsval _v, _rval = INT_TO_JSVAL(0);
	BrowserNative *brow;
	char *_c,
		*_c_args = "MFString url, SFNode node, SFString event",
		*_costr0,
		*_costr1,
		*_c_format = "o o s";
	uintptr_t nodarr[200];
	uintptr_t myptr;
	int ra;
	char filename[2000];
	char tfilename [2000];
	char *tfptr; 
	char *coptr;
	char firstBytes[4];
	char *bfp;
	char *slashindex;
	int found;
	int count;

	/* rval is always zero, so lets just set it */
	*rval = INT_TO_JSVAL(0);

	FUNC_INIT

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

		if (!JS_GetProperty(context, _obj[1], "__handle", &_v)) {
			printf("JS_GetProperty failed for \"__handle\" in VrmlBrowserCreateVrmlFromURL.\n");
			return JS_FALSE;
		}
		_str[1] = JS_ValueToString(context, _v);
		_costr1 = JS_GetStringBytes(_str[1]);
		ra = sscanf (_costr1,"%d",&myptr);

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

		/* now, we make up a string of nodes, pass it to c_set_field_be that
		takes this string apart. oh well... */

		filename[0] = '\0'; /* just reuse these variables */
		
		for (count = 1; count < ra; count +=2) {
			sprintf (tfilename, "%d,",nodarr[count]);	
			strcat (filename, tfilename);
		}

		#ifdef JSVERBOSE
		printf ("node string is %s\n",filename);
		#endif

		/* remember the freewrl addChildren removeChildren stuff? */
		if ((strncmp (_c,"addChildren",strlen("addChildren")) == 0) ||
		(strncmp (_c,"removeChildren",strlen("removeChildren")) == 0)) {
			c_set_field_be ((uintptr_t *)myptr, "children", filename);
		} else {
			c_set_field_be ((uintptr_t *)myptr, _c, filename);
		}

	} else {
		printf( "Incorrect argument format for createVrmlFromURL(%s).\n", _c_args);
		return JS_FALSE;
	}
	return JS_TRUE;
}


JSBool
VrmlBrowserAddRoute(JSContext *context, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	jsval _rval = INT_TO_JSVAL(0);
	if (!doVRMLRoute(context, obj, argc, argv,
					 "VrmlBrowserAddRoute", "jspBrowserAddRoute", "addRoute")) {
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
	if (!doVRMLRoute(context, obj, argc, argv,
					 "VrmlBrowserDeleteRoute", "jspBrowserDeleteRoute", "deleteRoute")) {
		printf( "doVRMLRoute failed in VrmlBrowserDeleteRoute.\n");
		return JS_FALSE;
	}
	*rval = _rval;
	return JS_TRUE;
}


static JSBool
doVRMLRoute(JSContext *context, JSObject *obj, uintN argc, jsval *argv,
			const char *callingFunc, const char *perlBrowserFunc, const char *browserFunc)
{
	jsval _v[2];
	BrowserNative *brow;
	JSObject *_obj[2];
	JSClass *_cls[2];
	JSString *_str[2];
	char *_route,
		*_cstr[2],
		*_costr[2],
		*_c_args =
		"SFNode fromNode, SFString fromEventOut, SFNode toNode, SFString toEventIn",
		*_c_format = "o s o s";
	size_t len;

	FUNC_INIT

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
					browserFunc, _c_args, callingFunc);
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

		len = strlen(_costr[0]) + strlen(_cstr[0]) +
			strlen(_costr[1]) + strlen(_cstr[1]) + 7;
		_route = (char *)JS_malloc(context, len * sizeof(char *));
		sprintf(_route, "%s %s %s %s",
				_costr[0], _cstr[0],
				_costr[1], _cstr[1]);

printf ("DPCVA route\n");/*		doPerlCallMethodVA(brow->sv_js, perlBrowserFunc, "s", _route);*/
		JS_free(context, _route);
	} else {
		printf( "\nIncorrect argument format for %s(%s).\n",
				callingFunc, _c_args);
		return JS_FALSE;
	}

	return JS_TRUE;
}
