/*


Javascript C language binding.

*/

/****************************************************************************
    This file is part of the FreeWRL/FreeX3D Distribution.

    Copyright 2009 CRC Canada. (http://www.crc.gc.ca)

    FreeWRL/FreeX3D is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    FreeWRL/FreeX3D is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with FreeWRL/FreeX3D.  If not, see <http://www.gnu.org/licenses/>.
****************************************************************************/


#include <config.h>
#if defined(JAVASCRIPT_DUK)
#include <system.h>
#include <display.h>
#include <internal.h>

#include <libFreeWRL.h>
#include <list.h>

#include "../vrml_parser/Structs.h"
#include "../vrml_parser/CRoutes.h"
#include "../opengl/OpenGL_Utils.h"
#include "../main/headers.h"
#include "../scenegraph/RenderFuncs.h"
#include "../vrml_parser/CParseGeneral.h"
#include "../scenegraph/Vector.h"
#include "../vrml_parser/CFieldDecls.h"
#include "../vrml_parser/CParseParser.h"
#include "../vrml_parser/CParseLexer.h"
#include "../vrml_parser/CProto.h"
#include "../vrml_parser/CParse.h"
#include "../main/Snapshot.h"
#include "../scenegraph/Collision.h"
#include "../scenegraph/quaternion.h"
#include "../scenegraph/Viewer.h"
#include "../x3d_parser/Bindable.h"
#include "../input/EAIHeaders.h"	/* for implicit declarations */


#include "JScript.h"
#include "CScripts.h"
#include "fieldSet.h"
#include "FWTYPE.h"



/* The Browser's supportedComponents and supportedProfiles are statically defined 
   in 'bits and pieces' in generatedCode.c and capabilitiesHandler.c and Structs.h.
   The Scene/ExecutionContext Profile and Components should be created during parsing
   (as of Aug 3, 2013 the parser calls handleProfile() or handleComponent() which
    just complains with printfs if freewrl can't handle the scene, and doesn't save them)

	For the browser's supportedComponents and supportedProfiles, we'll have
	indexable arrays, and on getting an index, we'll construct a throwaway JS object.
*/





/*
ComonentInfo{
String name;
Numeric level;
String Title;
String providerUrl;
}

ComponentInfoArray{
numeric length;
ComponentInfo [integer index];
}


ProfileInfo{
String name;
Numeric level;
String Title;
String providerUrl;
ComonentInfoArray components;
}
ProfileInfoArray{
numeric length;
ProfileInfo [integer index];
}

X3DFieldDefinition{
//properties
String name;
numeric accessType;  //e.g.. inputOnly
numeric dataType; //e.g. SFBool
}

FieldDefinitionArray{
numeric length;
X3DFieldDefinition [integer index];
}


ProtoDeclaration{
//properties
String name;
FieldDefinitionArray fields;
Boolean isExternProto;
//functions
SFNode newInstance();
}

ExternProtoDeclaration : ProtoDeclaration {
//properties
MFString urls;
numeric loadState;
//functions
void loadNow();
}

ProtoDeclarationArray{
numeric length;
X3DProtoDeclaration [integer index];
}

ExternProtoDeclarationArray{
numeric length;
X3DExternProtoDeclaration [integer index];
}

Route{
}
RouteArray{
numeric length;
Route [integer index];
}


ExecutionContext{
//properties
	String specificationVersion;
	String encoding;
	ProfileInfo profile;
	ComponentInfoArray components;
	String worldURL;
	MFNode rootNodes; //R + writable except in protoInstance
	ProtoDeclarationArray protos; //RW
	ExternProtoDeclarationArray externprotos; //RW
	RouteArray routes;
//functions
	X3DRoute addRoute(SFNode fromNode, String fromReadableField, SFNode toNode, String toWriteableField);
	void deleteRoute(X3DRoute route);
	SFNode createNode(String x3dsyntax);
	SFNode createProto(String x3dsyntax);
	SFNode getImportedNode(String defname, String);
	void updateImportedNode(String defname, String);
	void removeImportedNode(String defname);
	SFNode getNamedNode(String defname):
	void updateNamedNode(String defname, SFNode);
	void removeNamedNode(String defname);
}

Scene : ExecutionContext{
//properties
	String specificationVersion;
//functions
	void setMetaData(String name, String value);
	String getMetaData(String name);
	SFNode getExportedNode(String defname);
	void updateExportedNode(String defname, SFNode node);
	void removeExportedNode(String defname);
}

//just createX3DFromString, createX3DFromURL and replaceWorld differ in signature between VRML and X3D browser classes
X3DBrowser{
//properties
	String name;
	String version;
	numeric currentSpeed;
	numeric currentFrameRate;
	String description; //R/W
	CompnentInfoArray supportedComponents;
	ProfileInfoArray supportedProfiles;
	X3DScene currentScene;  //since X3DScene : X3DExecutionContext, use Scene w/flag
//functions
	void replaceWorld(X3DScene);
	X3DScene createX3DFromString(String x3dsyntax);
	X3DScene createX3DFromURL(MFString url, String callbackFunctionName, Object cbContextObject);
	void loadURL(MFString url, MFString parameter);
	X3DScene importDocument(DOMNode domNodeObject);
	void getRenderingProperty(String propertyName);
	void print(Object or String);
	void println(Object or String);
}
*/

int VrmlBrowserGetName(FWType fwtype, void * fwn, int argc, FWval fwpars, FWval fwretval)
{
	fwretval->_string = BrowserName;
	fwretval->itype = 'S';
	return 1;
}
int VrmlBrowserGetVersion(FWType fwtype, void * fwn, int argc, FWval fwpars, FWval fwretval)
{
	fwretval->_string = libFreeWRL_get_version();
	fwretval->itype = 'S';
	return 1;
}
int VrmlBrowserGetCurrentSpeed(FWType fwtype, void * fwn, int argc, FWval fwpars, FWval fwretval)
{
	char string[1000];
	sprintf (string,"%f",gglobal()->Mainloop.BrowserSpeed);
	fwretval->_string = strdup(string);
	fwretval->itype = 'S';
	return 1;
}

int VrmlBrowserGetCurrentFrameRate(FWType fwtype, void * fwn, int argc, FWval fwpars, FWval fwretval)
{
	char string[1000];
	sprintf (string,"%6.2f",gglobal()->Mainloop.BrowserFPS);
	fwretval->_string = strdup(string);
	fwretval->itype = 'S';
	return 1;
}
int VrmlBrowserGetWorldURL(FWType fwtype, void * fwn, int argc, FWval fwpars, FWval fwretval)
{
	fwretval->_string = BrowserFullPath;
	fwretval->itype = 'S';
	return 1;
}
const char *flexiString(FWval fwpars, char *buffer)
{
	//allow MFString[0], SFString or ecma String
	//if buffer is given: MF is converted to '["MF[0]"] ["MF[1]"] [...' format
	//if buffer is NULL, MF[0] is returned
	char *tptr;
	const char *_costr;
	int lenbuf = 1000;
	
	_costr = NULL;
	if(fwpars[0].itype == 'S')
		_costr = fwpars[0]._string;
	else if(fwpars[0].itype == 'W'){
		switch(fwpars[0]._web3dval.fieldType){
		case FIELDTYPE_SFString:
			{
				//Q. shoulD we ever get in here? SFString is supposed to be represented by an ECMA type in javascript
				struct Uni_String *sfs = (struct Uni_String*)fwpars[0]._web3dval.native;
				_costr = sfs->strptr;
			}
			break;
		case FIELDTYPE_MFString:
			{
				struct Multi_String *mfs = (struct Multi_String*)fwpars[0]._web3dval.native;
				if(buffer){
					int i, l1, l2, l3, lt;
					char *start = "[\"";
					char *end = "\"] ";
					l1 = strlen(start);
					l2 = strlen(end);
					buffer[0] = '\0';
					lt = 1;
					for(i=0;i<mfs->n;i++){
						l3 = strlen(mfs->p[i]->strptr);
						if(lt + l1 + l2 + l3 > lenbuf) break;
						strcat(buffer,"[\"");
						strcat(buffer,mfs->p[i]->strptr);
						strcat(buffer,"\"] ");
						lt += l1 + l2 + l3;
					}
					_costr = buffer;
				}else{
				_costr = mfs->p[0]->strptr;
				}
			}
			break;
		}
	}
	return _costr;
}

int VrmlBrowserReplaceWorld(FWType fwtype, void * fwn, int argc, FWval fwpars, FWval fwretval)
{
	char *tptr;
	char*_costr;
	
	_costr = strdup(flexiString(&fwpars[0],NULL));
	EAI_RW(_costr);
	return 0;
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
struct X3D_Anchor* get_EAIEventsIn_AnchorNode();
int VrmlBrowserLoadURL(FWType fwtype, void * fwn, int argc, FWval fwpars, FWval fwretval)
{
	char *url, *parameter;
	char bufferUrl[1000];
	char bufferParam[1000];
	char myBuf[1000];

	url = strdup(flexiString(&fwpars[0],bufferUrl));
	parameter = strdup(flexiString(&fwpars[1],bufferParam));
		/* we use the EAI code for this - so reformat this for the EAI format */
		{
			/* make up the URL from what we currently know */
			createLoadUrlString(myBuf,1000,url, parameter);
			createLoadURL(myBuf);

			/* now tell the fwl_RenderSceneUpdateScene that BrowserAction is requested... */
			setAnchorsAnchor( get_EAIEventsIn_AnchorNode()); //&gglobal().EAIEventsIn.EAI_AnchorNode;
		}
		gglobal()->RenderFuncs.BrowserAction = TRUE;
	return 0;
}
int VrmlBrowserSetDescription(FWType fwtype, void * fwn, int argc, FWval fwpars, FWval fwretval)
{
	const char *_costr = NULL;
	if(fwpars[0].itype == 'S')
		gglobal()->Mainloop.BrowserDescription = fwpars[0]._string;
	return 0;
}
int VrmlBrowserCreateX3DFromString(FWType fwtype, void * fwn, int argc, FWval fwpars, FWval fwretval)
{
	/* for the return of the nodes */
	struct X3D_Group *retGroup;
	char *xstr; 
	char *tmpstr;
	char *separator;
	int ra;
	int count;
	int wantedsize;
	int MallocdSize;
	ttglobal tg = gglobal();
	struct VRMLParser *globalParser = (struct VRMLParser *)tg->CParse.globalParser;
	const char *_c = fwpars[0]._string;

	/* do the call to make the VRML code  - create a new browser just for this string */
	gglobal()->ProdCon.savedParser = (void *)globalParser; globalParser = NULL;
	retGroup = createNewX3DNode(NODE_Group);
	ra = EAI_CreateX3d("String",_c,retGroup);
	globalParser = (struct VRMLParser*)gglobal()->ProdCon.savedParser; /* restore it */

	//fwretval->_web3dval.native = (void *)retGroup;
	if(retGroup->children.n < 1) return 0;
	if(0){
	fwretval->_web3dval.native = &retGroup->children.p[0];
	fwretval->_web3dval.fieldType = FIELDTYPE_SFNode; //Group
	}else{
	fwretval->_web3dval.native = &retGroup->children;
	fwretval->_web3dval.fieldType = FIELDTYPE_MFNode; //Group
	}
	fwretval->itype = 'W';
	return 1;
}
//int jsrrunScript(duk_context *ctx, char *script, FWval retval);
int VrmlBrowserCreateVrmlFromString(FWType fwtype, void * fwn, int argc, FWval fwpars, FWval fwretval)
{
	/* for the return of the nodes */
	struct X3D_Group *retGroup;
	char *xstr; 
	char *tmpstr;
	char *separator;
	int ra;
	int count;
	int wantedsize;
	int MallocdSize;
	ttglobal tg = gglobal();
	struct VRMLParser *globalParser = (struct VRMLParser *)tg->CParse.globalParser;
	const char *_c = fwpars[0]._string;

	/* do the call to make the VRML code  - create a new browser just for this string */
	gglobal()->ProdCon.savedParser = (void *)globalParser; globalParser = NULL;
	retGroup = createNewX3DNode(NODE_Group);
	ra = EAI_CreateVrml("String",_c,retGroup);
	globalParser = (struct VRMLParser*)gglobal()->ProdCon.savedParser; /* restore it */
	if(retGroup->children.n < 1) return 0;
	if(0){
	fwretval->_web3dval.native = &retGroup->children.p[0];
	fwretval->_web3dval.fieldType = FIELDTYPE_SFNode; //Group
	}else{
	fwretval->_web3dval.native = &retGroup->children;
	fwretval->_web3dval.fieldType = FIELDTYPE_MFNode; //Group
	}
	fwretval->itype = 'W';
	return 1;

}
int VrmlBrowserCreateVrmlFromURL(FWType fwtype, void * fwn, int argc, FWval fwpars, FWval fwretval)
{
	//from x3dnode, from char*field, to x3dnode, to char*field
	return 0;
}
void getFieldFromNodeAndName(struct X3D_Node* node,const char *fieldname, int *type, int *kind, int *iifield, union anyVrml **value);
/* we add/remove routes with this call */
void jsRegisterRoute(
	struct X3D_Node* from, int fromOfs,
	struct X3D_Node* to, int toOfs,
	int len, const char *adrem) {
	int ad;

	if (strcmp("addRoute",adrem) == 0) 
		ad = 1;
	else ad = 0;

 	CRoutes_Register(ad, from, fromOfs, to, toOfs , len, 
 		 returnInterpolatorPointer(stringNodeType(to->_nodeType)), 0, 0);
}
typedef struct X3DRouteStruct {
	struct X3D_Node* fromNode;
	struct X3D_Node* toNode;
	const char *fromField;
	const char *toField;
} X3DRoute;
int addDeleteRoute(char* callingFunc, int argc, FWval fwpars, FWval fwretval){
	struct X3D_Node *fromNode;
	struct X3D_Node *toNode;
	const char *fromFieldString, *toFieldString;
	int fromType,toType,fromKind,toKind,fromField,toField;
	union anyVrml *fromValue, *toValue;
	int myField;
	int fromOfs, toOfs, len;

	fromNode = X3D_NODE(fwpars[0]._web3dval.native);
	toNode   = X3D_NODE(fwpars[2]._web3dval.native);
	fromFieldString = fwpars[1]._string;
	toFieldString = fwpars[3]._string;
	getFieldFromNodeAndName(fromNode,fromFieldString,&fromType,&fromKind,&fromField,&fromValue);
	getFieldFromNodeAndName(toNode,toFieldString,&toType,&toKind,&toField,&toValue);

	/* do we have a mismatch here? */
	if (fromType != toType) {
		printf ("Javascript routing problem - can not route from %s to %s\n",
			stringNodeType(fromNode->_nodeType), 
			stringNodeType(toNode->_nodeType));
		return 0;
	}

	len = returnRoutingElementLength(toType);
	fromOfs = fromField > 999? fromField -1000 : fromField*5; // * sizeof(function list item)
	toOfs = toField > 999? toField -1000 : toField*5;
	jsRegisterRoute(fromNode, fromOfs, toNode, toOfs, len,callingFunc);
	//unfortunately there's no stable X3DRoute table pointer.
	//We just added to a queue, and the queue entry is deep copied into the final route array.
	//we should be returning an X3DRoute type wrapping a table entry
	//but instead we'll return a tuple, so the route can be looked up later 
	X3DRoute *route = malloc(sizeof(X3DRoute)); //leak
	route->fromNode = fromNode;
	route->toNode = toNode;
	route->fromField = fromFieldString;
	route->toField = toFieldString;
	fwretval->_pointer.native = (void *)route;
	fwretval->_pointer.fieldType = AUXTYPE_X3DRoute;
	fwretval->itype = 'P';
	return 1;
}
int VrmlBrowserAddRoute(FWType fwtype, void * fwn, int argc, FWval fwpars, FWval fwretval){
	int iret = addDeleteRoute("addRoute",argc,fwpars,fwretval);
	return iret;
}
int VrmlBrowserDeleteRoute(FWType fwtype, void * fwn, int argc, FWval fwpars, FWval fwretval){
	int iret = addDeleteRoute("deleteRoute",argc,fwpars,fwretval);
	return iret;
}
int VrmlBrowserPrint(FWType fwtype, void * fwn, int argc, FWval fwpars, FWval fwretval)
{
	const char *_costr = NULL;
	if(fwpars[0].itype == 'S'){
		_costr = fwpars[0]._string;
		ConsoleMessage("%s",_costr);
	}
	return 0;
}
int VrmlBrowserPrintln(FWType fwtype, void * fwn, int argc, FWval fwpars, FWval fwretval)
{
	const char *_costr = NULL;
	if(fwpars[0].itype == 'S'){
		_costr = fwpars[0]._string;
		ConsoleMessage("%s\n",_costr);
	}
	return 0;
}

FWFunctionSpec (BrowserFunctions)[] = {
	{"getName",	VrmlBrowserGetName, 'S',{0,0,0,NULL}},
	{"getVersion", VrmlBrowserGetVersion, 'S',{0,0,0,NULL}},
	{"getCurrentSpeed", VrmlBrowserGetCurrentSpeed, 'S',{0,0,0,NULL}},
	{"getCurrentFrameRate", VrmlBrowserGetCurrentFrameRate, 'S',{0,0,0,NULL}},
	{"getWorldURL", VrmlBrowserGetWorldURL, 'S',{0,0,0,NULL}},
	{"replaceWorld", VrmlBrowserReplaceWorld, '0',{1,-1,'F',"F"}},
	{"loadURL", VrmlBrowserLoadURL, '0',{2,1,'T',"FF"}},
	{"setDescription", VrmlBrowserSetDescription, '0',{1,-1,'F',"S"}},
	{"createVrmlFromString", VrmlBrowserCreateVrmlFromString, 'W',{1,-1,'F',"S"}},
	{"createVrmlFromURL", VrmlBrowserCreateVrmlFromURL,'W',{3,2,'F',"WSO"}},
	{"createX3DFromString", VrmlBrowserCreateX3DFromString, 'W',{1,-1,'F',"S"}},
	{"createX3DFromURL", VrmlBrowserCreateVrmlFromURL, 'W',{3,2,'F',"WSO"}},
	{"addRoute", VrmlBrowserAddRoute, 'P',{4,-1,'F',"WSWS"}},
	{"deleteRoute", VrmlBrowserDeleteRoute, '0',{4,-1,'F',"WSWS"}},
	{"print", VrmlBrowserPrint, '0',{1,-1,'F',"S"}},
	{"println", VrmlBrowserPrintln, '0',{1,-1,'F',"S"}},

	//{"replaceWorld", X3dBrowserReplaceWorld, 0},  //conflicts - X3DScene vs MFNode parameter - could detect?
	//{"createX3DFromString", X3dBrowserCreateX3DFromString, 0}, //conflicts but above verion shouldn't be above, or could detect?
	//{"createX3DFromURL", X3dBrowserCreateVrmlFromURL, 0}, //conflicts but above version shouldn't be above, or could detect?
	//{importDocument, X3dBrowserImportDocument, 0), //not sure we need/want this, what does it do?
	//{getRenderingProperty, X3dGetRenderingProperty, 0},
	//{addBrowserListener, X3dAddBrowserListener, 0},
	//{removeBrowserListener, X3dRemoveBrowserListener, 0},

	{0}
};


//typedef struct FWPropertySpec {
//    const char	*name; //NULL means index int: SFVec3f[0], MF[i]
//    char		index; //stable property index for switch/casing instead of strcmp on name
//    char		type; //0 = null, N=numeric I=Integer B=Boolean S=String, W=Object-web3d O-js Object P=ptr
//	char		isReadOnly; //T/F
//} FWPropertySpec;


FWPropertySpec (BrowserProperties)[] = {
	{"name", 0, 'S', 'T'},
	{"version", 1, 'S', 'T'},
	{"currentSpeed", 2, 'N', 'T'},
	{"currentFameRate", 3, 'N', 'T'},
	{"description", 4, 'S', 'F'},
	{"supportedComponents", 5, 'P', 'T'},
	{"supportedProfiles", 6, 'P', 'T'},
	{"currentScene", 7, 'P', 'T'},
	{NULL,0,0,0},
};
struct proftablestruct {
	int profileName;
	const int *profileTable;
	int level; //dug9
};
struct proftablestruct *getProfTable();
int * getCapabilitiesTable();
int BrowserGetter(int index, void * fwn, FWval fwretval){
	int nr = 1;
	//fwretval->itype = 'S'; //0 = null, N=numeric I=Integer B=Boolean S=String, W=Object-web3d O-js Object P=ptr F=flexiString(SFString,MFString[0] or ecmaString)
	switch (index) {
		case 0: //name
			fwretval->_string = BrowserName;
			fwretval->itype = 'S';
			break;
		case 1: //version
			fwretval->_string = libFreeWRL_get_version();
			fwretval->itype = 'S';
			break;
		case 2: //currentSpeed
			fwretval->_numeric = gglobal()->Mainloop.BrowserSpeed;
			fwretval->itype = 'N';
			break;
		case 3: //currentFrameRate
			fwretval->_numeric = gglobal()->Mainloop.BrowserFPS;
			fwretval->itype = 'N';
			break;
		case 4: //description
			fwretval->_string = gglobal()->Mainloop.BrowserDescription; //this is settable
			fwretval->itype = 'S';
			break;
		case 5: //supportedComponents
			fwretval->_pointer.fieldType = AUXTYPE_ComponentInfoArray;
			fwretval->_pointer.native = (void*)getCapabilitiesTable(); //TO-DO something about component info array
			fwretval->itype = 'P';
			break;
		case 6: //supportedProfiles
			fwretval->_pointer.fieldType = AUXTYPE_ProfileInfoArray;
			fwretval->_pointer.native = (void*)getProfTable(); //TO-DO something about profile info array
			fwretval->itype = 'P';
			break;
		case 7: //currentScene
			fwretval->_web3dval.fieldType = AUXTYPE_X3DScene;
			fwretval->_web3dval.native = (void *)(struct X3D_Node*)rootNode(); //X3DScene || X3DExecutionContext
				//change this to (Script)._executionContext when brotos working fully
			fwretval->itype = 'P';
			break;
		default:
			nr = 0;
		}
	return nr;
}
int BrowserSetter(int index, void * fwn, FWval fwval){
	switch (index) {
		case 4: //description is settable
			gglobal()->Mainloop.BrowserDescription = fwval->_string;
			break;
		default:
			break;
	}
	return TRUE;
}


FWTYPE BrowserType = {
	AUXTYPE_X3DBrowser,
	"X3DBrowser",
	0, //sizeof(struct X3DBrowser), 
	NULL, //no constructor for Browser
	NULL, //no constructor args
	BrowserProperties,
	NULL, //no special has
	BrowserGetter,
	BrowserSetter,
	0, 0, //takes int index in prop
	BrowserFunctions,
};




/* ProfileInfo, ProfileInfoArray, ComponentInfo, ComponentInfoArray
   I decided to do these as thin getter wrappers on the bits and pieces defined
   in Structs.h, GeneratedCode.c and capabilitiesHandler.c
   The Array types return the info type wrapper with private native member == index into
   the native array.
*/
//ComonentInfo{
//String name;
//Numeric level;
//String Title;
//String providerUrl;
//}

int capabilitiesHandler_getTableLength(int* table);
int capabilitiesHandler_getComponentLevel(int *table, int comp);
int capabilitiesHandler_getProfileLevel(int prof);
const int *capabilitiesHandler_getProfileComponent(int prof);
const int *capabilitiesHandler_getCapabilitiesTable();
/*
typedef struct intTableIndex{
	int* table;
	int index;
} *IntTableIndex;
*/


//ComponentInfoArray{
//numeric length;
//ComponentInfo [integer index];
//}

int ComponentInfoArrayGetter(int index, void * fwn, FWval fwretval){
	int *_table;
	int nr = 1;
	_table = (int *)fwn;
	//fwretval->itype = 'S'; //0 = null, N=numeric I=Integer B=Boolean S=String, W=Object-web3d O-js Object P=ptr F=flexiString(SFString,MFString[0] or ecmaString)
	if(index == -1){
//extern const char *COMPONENTS[];
//extern const int COMPONENTS_COUNT;
		int _length = capabilitiesHandler_getTableLength(_table); //COMPONENTS_COUNT;
		fwretval->_integer = _length;
		fwretval->itype = 'I';
	}else if(index > -1 && index < COMPONENTS_COUNT ){
		fwretval->_pointer.native = &_table[2*index];
		fwretval->_pointer.fieldType = AUXTYPE_ComponentInfo;
		fwretval->itype = 'P';
	}
	return nr;
}


FWPropertySpec (ComponentInfoArrayProperties)[] = {
	{"length", -1, 'I', 'T'},
	{NULL,0,0,0},
};

FWTYPE ComponentInfoArrayType = {
	AUXTYPE_ComponentInfoArray,
	"ComponentInfoArray",
	0, //sizeof(struct X3DBrowser), 
	NULL, //no constructor for Browser
	NULL, //no constructor args
	ComponentInfoArrayProperties,
	NULL, //no special has
	ComponentInfoArrayGetter,
	NULL,
	'P', 'T', //takes int index in prop of this type 
	NULL,
};

FWPropertySpec (ComponentInfoProperties)[] = {
	{"name", 0, 'S', 'T'},
	{"Title", 1, 'S', 'T'},
	{"level", 2, 'N', 'T'},
	{"providerUrl", 3, 'S', 'T'},
	{NULL,0,0,0},
};


int ComponentInfoGetter(int index, void * fwn, FWval fwretval){
	int nr, *tableEntry, nameIndex;
	tableEntry = (int *)fwn;
	nr = 1;
	//fwretval->itype = 'S'; //0 = null, N=numeric I=Integer B=Boolean S=String, W=Object-web3d O-js Object P=ptr F=flexiString(SFString,MFString[0] or ecmaString)
	switch (index) {
		case 0://name
		case 1://Title
			nameIndex = tableEntry[0];
			fwretval->_string = COMPONENTS[nameIndex]; 
			fwretval->itype = 'S';
			break;
		case 2://level
			fwretval->_integer = tableEntry[1];
			fwretval->itype = 'I';
			break;
		case 3://providerUrl
			fwretval->_string = "freewrl.sourceforge.net";
			fwretval->itype = 'S';
			break;
		default:
			nr = 0;
			break;
	}
	return nr;
}

FWTYPE ComponentInfoType = {
	AUXTYPE_ComponentInfo,
	"ComponentInfo",
	0, //sizeof(struct X3DBrowser), 
	NULL, //no constructor for Browser
	NULL, //no constructor args
	ComponentInfoProperties,
	NULL, //no special has
	ComponentInfoGetter,
	NULL,
	0,0, //takes int index in prop
	NULL,
};



//ProfileInfoArray{
//numeric length;
//ProfileInfo [integer index];
//}


int ProfileInfoArrayGetter(int index, void * fwn, FWval fwretval){
	struct proftablestruct *_table;
	int nr = 1;
	_table = (struct proftablestruct *)fwn;
	//fwretval->itype = 'S'; //0 = null, N=numeric I=Integer B=Boolean S=String, W=Object-web3d O-js Object P=ptr F=flexiString(SFString,MFString[0] or ecmaString)
	if(index == -1){
//extern const char *COMPONENTS[];
//extern const int COMPONENTS_COUNT;
		int _length = PROFILES_COUNT;
		fwretval->_integer = _length;
		fwretval->itype = 'I';
	}else if(index > -1 && index < PROFILES_COUNT ){
		fwretval->_pointer.native = &_table[index];
		fwretval->_pointer.fieldType = AUXTYPE_ProfileInfo;
		fwretval->itype = 'P';
	}
	return nr;
}


FWPropertySpec (ProfileInfoArrayProperties)[] = {
	{"length", -1, 'I', 'T'},
	{NULL,0,0,0},
};

FWTYPE ProfileInfoArrayType = {
	AUXTYPE_ProfileInfoArray,
	"ProfileInfoArray",
	0, //sizeof(struct X3DBrowser), 
	NULL, //no constructor for Browser
	NULL, //no constructor args
	ProfileInfoArrayProperties,
	NULL, //no special has
	ProfileInfoArrayGetter,
	NULL,
	'P', 'T',//takes int index in prop, readOnly
	NULL,
};


//ProfileInfo{
//String name;
//Numeric level;
//String Title;
//String providerUrl;
//ComonentInfoArray components;
//}

FWPropertySpec (ProfileInfoProperties)[] = {
	{"name", 0, 'S', 'T'},
	{"level", 1, 'N', 'T'},
	{"Title", 2, 'S', 'T'},
	{"providerUrl", 3, 'S', 'T'},
	{"components", 4, 'P', 'T'}, //writable? if so then I need a constructor for ComponentInfo?
	{NULL,0,0,0},
};


int ProfileInfoGetter(int index, void * fwn, FWval fwretval){
	int nr, nameIndex;
	struct proftablestruct *tableEntry;
	tableEntry = (struct proftablestruct *)fwn;
	nr = 1;
	//fwretval->itype = 'S'; //0 = null, N=numeric I=Integer B=Boolean S=String, W=Object-web3d O-js Object P=ptr F=flexiString(SFString,MFString[0] or ecmaString)
	switch (index) {
		case 0://name
		case 1://Title
			nameIndex = tableEntry->profileName;
			fwretval->_string = stringProfileType(nameIndex);
			fwretval->itype = 'S';
			break;
		case 2://level
			fwretval->_integer = tableEntry->level;
			fwretval->itype = 'I';
			break;
		case 3://providerUrl
			fwretval->_string = "freewrl.sourceforge.net";
			fwretval->itype = 'S';
			break;
		case 4://components
			fwretval->_pointer.native = (void *)tableEntry->profileTable;
			fwretval->_pointer.fieldType = AUXTYPE_ProfileInfo;
			fwretval->itype = 'P';
			break;
		default:
			nr = 0;
			break;
	}
	return nr;
}

FWTYPE ProfileInfoType = {
	AUXTYPE_ProfileInfo,
	"ProfileInfo",
	0, //sizeof(struct X3DBrowser), 
	NULL, //no constructor for Browser
	NULL, //no constructor args
	ProfileInfoProperties,
	NULL, //no special has
	ProfileInfoGetter,
	NULL,
	0,0, //takes int index in prop
	NULL,
};


int X3DExecutionContext_deleteRoute(FWType fwtype, void * fwn, int argc, FWval fwpars, FWval fwretval){
	//we're expecting an X3DRoute struct parameter.
	int iret;
	struct FWVAL fwpars2[4];
	X3DRoute *route = fwpars[0]._pointer.native;
	fwpars2[0]._web3dval.fieldType = FIELDTYPE_SFNode;
	fwpars2[0]._web3dval.native = route->fromNode;
	fwpars2[0].itype = 'W';
	fwpars2[1]._string = route->fromField;
	fwpars2[1].itype = 'S';
	fwpars2[2]._web3dval.fieldType = FIELDTYPE_SFNode;
	fwpars2[2]._web3dval.native = route->toNode;
	fwpars2[2].itype = 'W';
	fwpars2[3]._string = route->toField;
	fwpars2[3].itype = 'S';

	iret = addDeleteRoute("deleteRoute",4,fwpars2,fwretval);
	return 0;
}
int X3DExecutionContext_getNamedNode(FWType fwtype, void * fwn, int argc, FWval fwpars, FWval fwretval){
	int nr = 0;
	//broto warning - DEF name list should be per-executionContext
	struct X3D_Node* node = parser_getNodeFromName(fwpars[0]._string);
	if(node){
		fwretval->_web3dval.native = node;
		fwretval->_web3dval.fieldType = FIELDTYPE_SFNode;
		fwretval->itype = 'W';
		nr = 1;
	}
	return nr;
}

static FWFunctionSpec (X3DExecutionContextFunctions)[] = {
	//executionContext
	//{"addRoute", X3DExecutionContext_addRoute, 'P',{4,-1,'F',"WSWS"}},
	{"addRoute", VrmlBrowserAddRoute, 'P',{4,-1,'F',"WSWS"}},
	{"deleteRoute", X3DExecutionContext_deleteRoute,'0',{1,-1,'F',"P"}},
	//{"createNode", X3DExecutionContext_createNode, 'W',{1,-1,'F',"S"}},
	{"createNode", VrmlBrowserCreateX3DFromString, 'W',{1,-1,'F',"S"}},
	//{"createProto", X3DExecutionContext_createProto, 'W',{1,-1,'F',"S"}},
	//{"getImportedNode", X3DExecutionContext_getImportedNode, 'W',{2,-1,'F',"SS"}},
	//{"updateImportedNode", X3DExecutionContext_updateImportedNode, '0',{2,-1,'F',"SS"}},
	//{"removeImportedNode", X3DExecutionContext_removeImportedNode, '0',{1,-1,'F',"S"}},
	{"getNamedNode", X3DExecutionContext_getNamedNode, 'W',{1,-1,'F',"S"}},
	//{"updateNamedNode", X3DExecutionContext_updateNamedNode, '0',{2,-1,'F',"SW"}},
	//{"removeNamedNode", X3DExecutionContext_removeNamedNode, '0',{1,-1,'F',"S"}},
	////scene
	//{"setMetaData", X3DScene_setMetaData, '0',{2,-1,'F',"SS"}},
	//{"getMetaData", X3DScene_getMetaData, 'S',{1,-1,'F',"S"}},
	//{"getExportedNode", X3DScene_getExportedNode, 'W',{1,-1,'F',"S"}},
	//{"updateExportedNode", X3DScene_updateExportedNode, '0',{2,-1,'F',"SW"}},
	//{"removeExportedNode", X3DScene_removeExportedNode, '0',{1,-1,'F',"S"}},
	{0}
};



static FWPropertySpec (X3DExecutionContextProperties)[] = {
	//executionContext
	{"specificationVersion", 0, 'S', 'T'},
	{"encoding", 1, 'S', 'T'},
	{"profile", 2, 'P', 'T'},
	{"components", 3, 'P', 'T'},
	{"worldURL", 4, 'S', 'T'},
	{"rootNodes", 5, 'W', 'T'},
	{"protos", 6, 'P', 'T'},
	{"externprotos", 7, 'P', 'T'},
	{"routes", 8, 'P', 'T'},
	//scene
	{"isScene", 9, 'B', 'T'}, //else protoInstance. extra beyond specs - I think flux has it.
	{0}
};

int X3DExecutionContextGetter(int index, void * fwn, FWval fwretval){
	struct X3D_Node * ec;
	int nr = 1;
	ec = (struct X3D_Node*)fwn;
	//fwretval->itype = 'S'; //0 = null, N=numeric I=Integer B=Boolean S=String, W=Object-web3d O-js Object P=ptr F=flexiString(SFString,MFString[0] or ecmaString)
	switch (index) {
		case 0: //specificationVersion
		{
			char cs[100];
			sprintf(cs,"{%d,%d,%d}",inputFileVersion[0],inputFileVersion[1],inputFileVersion[2]);
			fwretval->_string = strdup(cs); //leaky, should be 1:1 with executionContext
			fwretval->itype = 'S';
		}
			break;
		case 1: //encoding string readonly
			//Valid values are "ASCII", "VRML", "XML", "BINARY", "SCRIPTED", "BIFS", "NONE" 
			fwretval->_string = "not filled in yet sb. VRML or XML or ..";
			fwretval->itype = 'S';
			break;
		case 2: //profile
		{
			struct proftablestruct * profile;
			int index = gglobal()->Mainloop.scene_profile;
			profile = getProfTable();
			fwretval->_pointer.native = &profile[index];
			fwretval->_pointer.fieldType = AUXTYPE_ProfileInfo;
			fwretval->itype = 'P';
		}
			break;
		case 3: //components
			fwretval->_pointer.native = (void *)gglobal()->Mainloop.scene_components;
			fwretval->_pointer.fieldType = AUXTYPE_ComponentInfoArray;
			fwretval->itype = 'P';
			break;
		case 4: //worldURL
			fwretval->_string =  gglobal()->Mainloop.url; //this is settable
			fwretval->itype = 'S';
			break;
		case 5: //rootNodes
			fwretval->_web3dval.native = (void *)&((struct X3D_Group*)rootNode())->children;  //broto warning: inside a proto should be the rootnodes of the protobody
			fwretval->_web3dval.fieldType = FIELDTYPE_MFNode;
			fwretval->itype = 'W';
			break;
		case 6: //protos
			fwretval->itype = '0'; //not implemented yet
			nr = 0;
			break;
		case 7: //externprotos
			fwretval->itype = '0'; //not implemented yet
			nr = 0;
			break;
		case 8: //routes
			fwretval->_pointer.fieldType = AUXTYPE_X3DRouteArray; 
			fwretval->_pointer.native = NULL; //broto warning: this should be a per-context array
			fwretval->itype = 'P';
			break;
		case 9: //isScene
			fwretval->_boolean = TRUE; //broto warning: should be false inside a protoinstance body
			fwretval->itype = 'B';
			break;
		default:
			nr = 0;
		}
	return nr;
}

FWTYPE X3DExecutionContextType = {
	AUXTYPE_X3DExecutionContext,
	"X3DExecutionContext",
	0, //sizeof(struct X3DBrowser), 
	NULL, //no constructor
	NULL, //no constructor args
	X3DExecutionContextProperties,
	NULL, //no special has
	X3DExecutionContextGetter,
	NULL,
	0,0, //takes int index in prop
	NULL,
};

struct CRStruct *getCRoutes();
int getCRouteCount();
int X3DRouteArrayGetter(int index, void * fwn, FWval fwretval){
	int nr = 0;
	//fwretval->itype = 'S'; //0 = null, N=numeric I=Integer B=Boolean S=String, W=Object-web3d O-js Object P=ptr F=flexiString(SFString,MFString[0] or ecmaString)
	if(index == -1){
		int _length = getCRouteCount();
		fwretval->_integer = _length;
		fwretval->itype = 'I';
		nr = 1;
	}else if(index > -1 && index < getCRouteCount() ){
		struct CRStruct *routes = getCRoutes();
		fwretval->_pointer.native = &routes[index];
		fwretval->_pointer.fieldType = AUXTYPE_X3DRoute;
		fwretval->itype = 'P';
		nr = 1;
	}
	return nr;
}


FWPropertySpec (X3DRouteArrayProperties)[] = {
	{"length", -1, 'I', 'T'},
	{NULL,0,0,0},
};

FWTYPE X3DRouteArrayType = {
	AUXTYPE_X3DRouteArray,
	"X3DRouteArray",
	0, //sizeof(struct X3DRoute), 
	NULL, //no constructor for X3DRoute
	NULL, //no constructor args
	X3DRouteArrayProperties,
	NULL, //no special has
	X3DRouteArrayGetter,
	NULL,
	'P', 'T',//takes int index in prop, readonly
	NULL,
};

//X3DRoute{
//SFNode sourceNode;
//String sourceField;
//SFNode destinationNode;
//String destinationField;
//}

FWPropertySpec (X3DRouteProperties)[] = {
	{"sourceNode", 0, 'W', 'T'},
	{"sourceField", 1, 'S', 'T'},
	{"destinationNode", 2, 'W', 'T'},
	{"destinationField", 3, 'S', 'T'},
	{NULL,0,0,0},
};
void getFieldFromNodeAndIndex(struct X3D_Node* node, int iifield, const char **fieldname, int *type, int *kind, union anyVrml **value);
int X3DRouteGetter(int index, void * fwn, FWval fwretval){
	union anyVrml *value;
	int type, kind;
	char *fieldname;
	struct CRStruct *route;
	int nr = 1;
	route = (struct CRStruct *)fwn;
	//fwretval->itype = 'S'; //0 = null, N=numeric I=Integer B=Boolean S=String, W=Object-web3d O-js Object P=ptr F=flexiString(SFString,MFString[0] or ecmaString)
	switch(index){
	case 0: //fromNode
		fwretval->_web3dval.native = (void*)route->routeFromNode;
		fwretval->_web3dval.fieldType = FIELDTYPE_SFNode;
		fwretval->itype = 'W';
		break;
	case 1: //fromField
		getFieldFromNodeAndIndex(route->routeFromNode,route->fnptr,&fieldname,&type,&kind,&value);
		fwretval->_string = fieldname; //NULL;
		fwretval->itype = 'S';
		break;
	case 2: //toNode
		fwretval->_web3dval.native = (void*)route->routeFromNode;
		fwretval->_web3dval.fieldType = FIELDTYPE_SFNode;
		fwretval->itype = 'W';
		break;
	case 3: //toField
		getFieldFromNodeAndIndex(route->tonodes[0].routeToNode,route->tonodes[0].foffset,&fieldname,&type,&kind,&value);
		fwretval->_string = fieldname;
		fwretval->itype = 'S';
		break;
	default:
		nr = 0;
	}
	return nr;
}


FWTYPE X3DRouteType = {
	AUXTYPE_X3DRoute,
	"X3DRoute",
	0, //sizeof(struct X3DRoute), 
	NULL, //no constructor for X3DRoute
	NULL, //no constructor args
	X3DRouteProperties,
	NULL, //no special has
	X3DRouteGetter,
	NULL,
	0,0, //takes int index in prop
	NULL,
};

struct string_int{
	char *c;
	int i;
};

struct string_int lookup_X3DConstants[] = {
	{"INITIALIZED_EVENT",1},
	{"SHUTDOWN_EVENT",1},
	{"CONNECTION_ERROR",1},
	{"INITIALIZED_ERROR",1},
	{"NOT_STARTED_STATE",1},
	{"IN_PROGRESS_STATE",1},
	{"COMPLETE_STATE",1},
	{"FAILED_STATE",0},
	{"SFBool",FIELDTYPE_SFBool},
	{"MFBool",FIELDTYPE_MFBool},
	{"MFInt32",FIELDTYPE_MFInt32},
	{"SFInt32",FIELDTYPE_SFInt32},
	{"SFFloat",FIELDTYPE_SFFloat},
	{"MFFloat",FIELDTYPE_MFFloat},
	{"SFDouble",FIELDTYPE_SFDouble},
	{"MFDouble",FIELDTYPE_MFDouble},
	{"SFTime",FIELDTYPE_SFTime},
	{"MFTime",FIELDTYPE_MFTime},
	{"SFNode",FIELDTYPE_SFNode},
	{"MFNode",FIELDTYPE_MFNode},
	{"SFVec2f",FIELDTYPE_SFVec2f},
	{"MFVec2f",FIELDTYPE_MFVec2f},
	{"SFVec3f",FIELDTYPE_SFVec3f},
	{"MFVec3f",FIELDTYPE_MFVec3f},
	{"SFVec3d",FIELDTYPE_SFVec3d},
	{"MFVec3d",FIELDTYPE_MFVec3d},
	{"SFRotation",FIELDTYPE_SFRotation},
	{"MFRotation",FIELDTYPE_MFRotation},
	{"SFColor",FIELDTYPE_SFColor},
	{"MFColor",FIELDTYPE_MFColor},
	{"SFImage",FIELDTYPE_SFImage},
//	{"MFImage",FIELDTYPE_MFImage},
	{"SFColorRGBA",FIELDTYPE_SFColorRGBA},
	{"MFColorRGBA",FIELDTYPE_MFColorRGBA},
	{"SFString",FIELDTYPE_SFString},
	{"MFString",FIELDTYPE_MFString},
/*
	{"X3DBoundedObject",},
	{"X3DMetadataObject",},
	{"X3DUrlObject",},
	{"X3DTriggerNode",},
	{"X3DInfoNode",},
	{"X3DAppearanceNode",},
	{"X3DAppearanceChildNode",},
	{"X3DMaterialNode",},
	{"X3DTextureNode",},
	{"X3DTexture2DNode",},
	{"X3DTexture3DNode",},
	{"X3DTextureTransformNode",},
	{"X3DGeometryNode",},
	{"X3DGeometry3DNode",},
	{"X3DCoordinateNode",},
	{"X3DParametricGeometryNode",},
	{"X3DGeometricPropertyNode",},
	{"X3DColorNode",},
	{"X3DProtoInstance",},
	{"X3DNormalNode",},
	{"X3DTextureCoordinateNode",},
	{"X3DFontStyleNode",},
	{"X3DGroupingNode ",},
	{"X3DChildNode",},
	{"X3DBindableNode",},
	{"X3DBackgroundNode",},
	{"X3DInterpolatorNode",},
	{"X3DShapeNode",},
	{"X3DScriptNode",},
	{"X3DSensorNode",},
	{"X3DEnvironmentalSensorNode",},
	{"X3DLightNode",},
	{"X3DNetworkSensorNode",},
	{"X3DPointingDeviceSensorNode",},
	{"X3DDragSensorNode",},
	{"X3DKeyDeviceSensorNode",},
	{"X3DSequencerNode",},
	{"X3DTimeDependentNode",},
	{"X3DSoundNode",},
	{"X3DSoundSourceNode",},
	{"X3DTouchSensorNode",},
*/
	{"inputOnly",PKW_inputOnly},
	{"outputOnly",PKW_outputOnly},
	{"inputOutput",PKW_inputOutput},
	{"initializeOnly",PKW_initializeOnly},
	{NULL,0}
};

struct string_int *lookup_string_int(struct string_int *table, char *searchkey, int *index){
	int i;
	struct string_int *retval = NULL;
	*index = -1;
	if(!table) return NULL;
	i = 0;
	while(table[i].c){
		if(!strcmp(table[i].c,searchkey)){
			//found it
			(*index) = i;
			return &table[i];
		}
		i++;
	}
	return NULL;
}

int X3DConstantsGetter(int index, void * fwn, FWval fwretval){
	int nr = 1;
	fwretval->_integer = lookup_X3DConstants[index].i;
	fwretval->itype = 'I';
	return nr;
}

int len_constants(){
	int len = (sizeof(lookup_X3DConstants) / sizeof(struct string_int)) -1;
	return len;
}
int X3DConstantsIterator(int index, FWTYPE *fwt, FWPointer *pointer, char **name, int *lastProp, int *jndex, char *type, char *readOnly){
	index ++;
	(*jndex) = 0;
	if(index < len_constants()){
		(*name) = lookup_X3DConstants[index].c;
		(*jndex) = index;
		(*lastProp) = index;
		(*type) = 'I';
		(*readOnly) = 'T';
		return index;
	}
	return -1;
}
FWTYPE X3DConstantsType = {
	AUXTYPE_X3DConstants,
	"X3DConstants",
	0, //sizeof(struct X3DRoute), 
	NULL, //no constructor for X3DRoute
	NULL, //no constructor args
	NULL, //X3DConstantsProperties - lets have fun and use the custom HAS function
	X3DConstantsIterator, //custom Iterator function - returns the index used in the Getter and has
	X3DConstantsGetter,
	NULL,
	0,0, //takes int index in prop
	NULL,
};


void initVRMLBrowser(FWTYPE** typeArray, int *n){
	typeArray[*n] = &X3DRouteType; (*n)++;
	typeArray[*n] = &X3DRouteArrayType; (*n)++;
	typeArray[*n] = &X3DExecutionContextType; (*n)++;
	typeArray[*n] = &ProfileInfoType; (*n)++;
	typeArray[*n] = &ProfileInfoArrayType; (*n)++;
	typeArray[*n] = &ComponentInfoType; (*n)++;
	typeArray[*n] = &ComponentInfoArrayType; (*n)++;
	typeArray[*n] = &BrowserType; (*n)++;
	typeArray[*n] = &X3DConstantsType; (*n)++;
}
#endif /* ifdef JAVASCRIPT_DUK */