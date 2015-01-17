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

/* To do list July 2014
X3DRoute type is broken
X3DScene/X3DExecutionContext - functions not implemented relating to protos
*/
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
	indexable arrays.

	AUXTYPES are never routed, never assigned to a Script Node's fields. They are never deep copied.
	But they may have FIELDTYPE properties.

	X3DRoute - as of july 2014 this is 'broken'
	X3DScene/X3DExecutionContext - not properly implemented

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
	//struct string_int *retval = NULL;
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

int X3DConstantsGetter(FWType fwt, int index, void *ec, void *fwn, FWval fwretval){
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
	'P',
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




int VrmlBrowserGetName(FWType fwtype, void *ec, void * fwn, int argc, FWval fwpars, FWval fwretval)
{
	fwretval->_string = BrowserName;
	fwretval->itype = 'S';
	return 1;
}
int VrmlBrowserGetVersion(FWType fwtype, void *ec, void * fwn, int argc, FWval fwpars, FWval fwretval)
{
	fwretval->_string = libFreeWRL_get_version();
	fwretval->itype = 'S';
	return 1;
}
int VrmlBrowserGetCurrentSpeed(FWType fwtype, void *ec, void * fwn, int argc, FWval fwpars, FWval fwretval)
{
	char string[1000];
	sprintf (string,"%f",gglobal()->Mainloop.BrowserSpeed);
	fwretval->_string = strdup(string);
	fwretval->itype = 'S';
	return 1;
}

int VrmlBrowserGetCurrentFrameRate(FWType fwtype, void *ec, void *fwn, int argc, FWval fwpars, FWval fwretval)
{
	char string[1000];
	sprintf (string,"%6.2f",gglobal()->Mainloop.BrowserFPS);
	fwretval->_string = strdup(string);
	fwretval->itype = 'S';
	return 1;
}
int VrmlBrowserGetWorldURL(FWType fwtype, void *ec, void *fwn, int argc, FWval fwpars, FWval fwretval)
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
	//if(fwpars[0].itype == 'S')
	//	_costr = fwpars[0]._string;
	//else 
	if(fwpars[0].itype == 'W'){
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

int VrmlBrowserReplaceWorld(FWType fwtype, void *ec, void *fwn, int argc, FWval fwpars, FWval fwretval)
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
int VrmlBrowserLoadURL(FWType fwtype, void *ec, void *fwn, int argc, FWval fwpars, FWval fwretval)
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
int VrmlBrowserSetDescription(FWType fwtype, void *ec, void *fwn, int argc, FWval fwpars, FWval fwretval)
{
	//const char *_costr = NULL;
	if(fwpars[0].itype == 'S')
		gglobal()->Mainloop.BrowserDescription = fwpars[0]._string;
	return 0;
}
int VrmlBrowserCreateX3DFromString(FWType fwtype, void *ec, void *fwn, int argc, FWval fwpars, FWval fwretval)
{
	/* for the return of the nodes */
	struct X3D_Group *retGroup;
	int i, iret = 0;
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
	if(usingBrotos()){
		retGroup = createNewX3DNode0(NODE_Group); //don't register, we'll gc here
		gglobal()->ProdCon.savedParser = (void *)globalParser; globalParser = NULL;
		ra = EAI_CreateX3d("String",_c,ec,retGroup); //includes executionContext for __nodes and __subContexts
		globalParser = (struct VRMLParser*)gglobal()->ProdCon.savedParser; /* restore it */
		if(retGroup->children.n > 0) {
			struct Multi_Node *mfn = (struct Multi_Node *)malloc(sizeof(struct Multi_Node));
			memcpy(mfn,&retGroup->children,sizeof(struct Multi_Node));
			FREE_IF_NZ(retGroup);
			for(i=0;i<mfn->n;i++){
				mfn->p[i]->_parentVector->n = 0; 
			}
			fwretval->_web3dval.native = mfn;
			fwretval->_web3dval.fieldType = FIELDTYPE_MFNode; //Group
			fwretval->_web3dval.gc = 1; //will be GCd by nodelist
			fwretval->itype = 'W';
			iret = 1;
		}
		FREE_IF_NZ(retGroup);
	}else{
		retGroup = createNewX3DNode(NODE_Group);
		gglobal()->ProdCon.savedParser = (void *)globalParser; globalParser = NULL;
		ra = EAI_CreateX3d("String",_c,X3D_NODE(retGroup),retGroup);
		globalParser = (struct VRMLParser*)gglobal()->ProdCon.savedParser; /* restore it */
		//fwretval->_web3dval.native = (void *)retGroup;
		if(retGroup->children.n > 0){
			fwretval->_web3dval.native = &retGroup->children;
			fwretval->_web3dval.fieldType = FIELDTYPE_MFNode; //Group
			fwretval->_web3dval.gc = 0; //will be GCd by nodelist
			fwretval->itype = 'W';
			iret = 1;
		}
	}

	return iret;
}
//int jsrrunScript(duk_context *ctx, char *script, FWval retval);
int VrmlBrowserCreateVrmlFromString(FWType fwtype, void *ec, void *fwn, int argc, FWval fwpars, FWval fwretval)
{
	/* for the return of the nodes */
	struct X3D_Group *retGroup;
	char *xstr; 
	char *tmpstr;
	char *separator;
	int i, ra;
	int count, iret;
	int wantedsize;
	int MallocdSize;
	ttglobal tg = gglobal();
	struct VRMLParser *globalParser = (struct VRMLParser *)tg->CParse.globalParser;
	const char *_c = fwpars[0]._string; //fwpars[0]._web3dval.anyvrml->sfstring->strptr; 

	iret = 0;
	if(usingBrotos()){
		retGroup = createNewX3DNode0(NODE_Group); //don't register, we'll gc here
		gglobal()->ProdCon.savedParser = (void *)globalParser; globalParser = NULL;
		ra = EAI_CreateVrml("String",_c,ec,retGroup); //includes executionContext for __nodes and __subContexts
		globalParser = (struct VRMLParser*)gglobal()->ProdCon.savedParser; /* restore it */
		if(retGroup->children.n > 0) {
			struct Multi_Node *mfn = (struct Multi_Node *)malloc(sizeof(struct Multi_Node));
			memcpy(mfn,&retGroup->children,sizeof(struct Multi_Node));
			FREE_IF_NZ(retGroup);
			for(i=0;i<mfn->n;i++){
				mfn->p[i]->_parentVector->n = 0; 
			}
			fwretval->_web3dval.native = mfn;
			fwretval->_web3dval.fieldType = FIELDTYPE_MFNode; //Group
			fwretval->_web3dval.gc = 0; //will be GCd by nodelist
			fwretval->itype = 'W';
			iret = 1;
		}
		FREE_IF_NZ(retGroup);
	}else{
		/* do the call to make the VRML code  - create a new browser just for this string */
		gglobal()->ProdCon.savedParser = (void *)globalParser; globalParser = NULL;
		retGroup = createNewX3DNode(NODE_Group);
		ra = EAI_CreateVrml("String",_c,X3D_NODE(retGroup),retGroup);
		globalParser = (struct VRMLParser*)gglobal()->ProdCon.savedParser; /* restore it */
		//if(retGroup->children.n < 1) return 0;
		fwretval->_web3dval.native = &retGroup->children;
		fwretval->_web3dval.fieldType = FIELDTYPE_MFNode; //Group
		fwretval->_web3dval.gc = 0;
		fwretval->itype = 'W';
		iret = 1;
	}
	return iret;

}
int VrmlBrowserCreateVrmlFromURL(FWType fwtype, void *ec, void *fwn, int argc, FWval fwpars, FWval fwretval)
{
	//from x3dnode, from char*field, to x3dnode, to char*field
	return 0;
}

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

void *addDeleteRoute0(void *fwn, char*callingFunc, struct X3D_Node* fromNode, char *sfromField, struct X3D_Node* toNode, char *stoField){
	void *retval;
	int fromType,toType,fromKind,toKind,fromField,toField;
	int i, len, fromOfs, toOfs;
	union anyVrml *fromValue, *toValue;

	getFieldFromNodeAndName(fromNode,sfromField,&fromType,&fromKind,&fromField,&fromValue);
	getFieldFromNodeAndName(toNode,stoField,&toType,&toKind,&toField,&toValue);

	/* do we have a mismatch here? */
	if (fromType != toType) {
		printf ("Javascript routing problem - can not route from %s to %s\n",
			stringNodeType(fromNode->_nodeType), 
			stringNodeType(toNode->_nodeType));
		return NULL;
	}

	len = returnRoutingElementLength(toType);
	if(usingBrotos()){
		struct brotoRoute *broute;
		struct X3D_Proto *ec = (struct X3D_Proto*)fwn;
		if(!strcmp(callingFunc,"addRoute")){
			broute = malloc(sizeof(struct brotoRoute));
			broute->from.node = fromNode;
			broute->from.ifield = fromField;
			//broute->from.Ofs = fromOfs;
			broute->from.ftype = fromType;
			broute->to.node = toNode;
			broute->to.ifield = toField;
			//broute->to.Ofs = toOfs;
			broute->to.ftype = toType;
			broute->lastCommand = 1; //added above (won't be added if an import weak route)
			CRoutes_RegisterSimpleB(broute->from.node,broute->from.ifield,broute->to.node,broute->to.ifield,broute->ft);
			broute->ft = fromType == toType ? fromType : -1;
			if(!ec->__ROUTES)
				ec->__ROUTES = newStack(struct brotoRoute *);
			stack_push(struct brotoRoute *, ec->__ROUTES, broute);
			retval = (void*)broute;
		}else{  
			//deleteRoute
			if(ec->__ROUTES)
				for(i=0;i<vectorSize(ec->__ROUTES);i++){
					broute = vector_get(struct brotoRoute*,ec->__ROUTES,i);
					if(broute->from.node == fromNode && broute->from.ifield == fromField
						&& broute->to.node == toNode && broute->to.ifield == toField){
						if(broute->lastCommand == 1)
							CRoutes_RemoveSimpleB(broute->from.node,broute->from.ifield,broute->to.node,broute->to.ifield,broute->ft);
						broute->lastCommand = 0;
						vector_remove_elem(struct brotoRoute*,ec->__ROUTES,i);
						break;
					}
				}
			retval = NULL;
		}
	}else{
		//unfortunately there's no stable X3DRoute table pointer.
		//We just added to a queue, and the queue entry is deep copied into the final route array.
		//we should be returning an X3DRoute type wrapping a table entry
		//but instead we'll return a tuple, so the route can be looked up later 
		//if(0){
		//	X3DRoute *route = malloc(sizeof(X3DRoute)); //leak
		//	route->fromNode = fromNode;
		//	route->toNode = toNode;
		//	route->fromField = fromFieldString;
		//	route->toField = toFieldString;
		//	fwretval->_pointer.native = (void *)route;
		//	fwretval->_pointer.fieldType = AUXTYPE_X3DRoute;
		//	fwretval->itype = 'P';
		//	return 1;
		//}
		fromOfs = fromField > 999? fromField -1000 : fromField*5; // * sizeof(function list item)
		toOfs = toField > 999? toField -1000 : toField*5;
		jsRegisterRoute(fromNode, fromOfs, toNode, toOfs, len,callingFunc);
		retval = NULL;
	}
	return retval;

}

void * addDeleteRoute(void *fwn, char* callingFunc, int argc, FWval fwpars, FWval fwretval){
	struct X3D_Node *fromNode;
	struct X3D_Node *toNode;
	const char *fromField, *toField;
	void *retval;

	fromNode = fwpars[0]._web3dval.native; 
	toNode   = fwpars[2]._web3dval.native; 
	fromField = fwpars[1]._string; 
	toField = fwpars[3]._string; 
	
	retval = addDeleteRoute0(fwn,callingFunc,fromNode, fromField, toNode, toField);
	return retval;
}
int X3DExecutionContext_deleteRoute(FWType fwtype, void *ec, void *fwn, int argc, FWval fwpars, FWval fwretval){
	//we're expecting an X3DRoute struct parameter.
	int nr = 0;
	void *xroute;
	struct X3D_Node *fromNode, *toNode;
	char *fromField, *toField;
	int fromIfield, toIfield;
	int ftype,kind;
	union anyVrml *value;

	if(fwpars[0].itype != 'P') return nr;

	if(usingBrotos()){
		struct brotoRoute* broute = (struct brotoRoute*)fwpars[0]._pointer.native;
		fromNode = broute->from.node;
		fromIfield = broute->from.ifield;
		toNode = broute->to.node;
		toIfield = broute->to.ifield;
	}else{
		//struct CRStruct *route = fwpars[0]._pointer.native;
		int index = *(int*)(fwpars[0]._pointer.native);
		getSpecificRoute (index,&fromNode, &fromIfield, &toNode, &toIfield);
	}
	getFieldFromNodeAndIndex(fromNode,fromIfield,&fromField,&ftype,&kind,&value);
	getFieldFromNodeAndIndex(toNode,toIfield,&toField,&ftype,&kind,&value);
	xroute = addDeleteRoute0(fwn,"deleteRoute",fromNode, fromField, toNode, toField);
	return nr;
}

int VrmlBrowserAddRoute(FWType fwtype, void *ec, void *fwn, int argc, FWval fwpars, FWval fwretval){
	void *xroute;
	int nr = 0;
	xroute = addDeleteRoute(fwn,"addRoute",argc,fwpars,fwretval);
	if(xroute){
		fwretval->_web3dval.fieldType = AUXTYPE_X3DRoute; //AUXTYPE_X3DScene;
		fwretval->_web3dval.native = xroute;
		fwretval->_web3dval.gc = 0;
		fwretval->itype = 'P';
		nr = 1;
	}
	return nr;
}
int VrmlBrowserDeleteRoute(FWType fwtype, void *ec, void *fwn, int argc, FWval fwpars, FWval fwretval){
	void *xroute;
	int nr = 0;
	xroute = addDeleteRoute(fwn,"deleteRoute",argc,fwpars,fwretval);
	return nr;
}
int VrmlBrowserPrint(FWType fwtype, void *ec, void *fwn, int argc, FWval fwpars, FWval fwretval)
{
	const char *_costr = NULL;
	if(fwpars[0].itype == 'S'){
		_costr = fwpars[0]._string; //fwpars[0]._web3dval.anyvrml->sfstring->strptr; 
		if(_costr)
			ConsoleMessage("%s",_costr);
	}
	return 0;
}
int VrmlBrowserPrintln(FWType fwtype, void *ec, void *fwn, int argc, FWval fwpars, FWval fwretval)
{
	const char *_costr = NULL;
	if(fwpars[0].itype == 'S'){
		_costr = fwpars[0]._string; //fwpars[0]._web3dval.anyvrml->sfstring->strptr; 
		if(_costr)
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
	{"replaceWorld", VrmlBrowserReplaceWorld, '0',{1,-1,0,"Z"}},
	{"loadURL", VrmlBrowserLoadURL, '0',{2,1,'T',"FF"}},
	{"setDescription", VrmlBrowserSetDescription, '0',{1,-1,0,"S"}},
	{"createVrmlFromString", VrmlBrowserCreateVrmlFromString, 'W',{1,-1,0,"S"}},
	{"createVrmlFromURL", VrmlBrowserCreateVrmlFromURL,'W',{3,2,0,"WSO"}},
	{"createX3DFromString", VrmlBrowserCreateX3DFromString, 'W',{1,-1,0,"S"}},
	{"createX3DFromURL", VrmlBrowserCreateVrmlFromURL, 'W',{3,2,0,"WSO"}},
	{"addRoute", VrmlBrowserAddRoute, 'P',{4,-1,0,"WSWS"}},
	{"deleteRoute", VrmlBrowserDeleteRoute, '0',{4,-1,0,"WSWS"}},
	{"print", VrmlBrowserPrint, '0',{1,-1,0,"S"}},
	{"println", VrmlBrowserPrintln, '0',{1,-1,0,"S"}},

	//{importDocument, X3dBrowserImportDocument, 0), //not sure we need/want this, what does it do?
	//{getRenderingProperty, X3dGetRenderingProperty, 0},
	//{addBrowserListener, X3dAddBrowserListener, 0},
	//{removeBrowserListener, X3dRemoveBrowserListener, 0},

	{0}
};


//typedef struct FWPropertySpec {
//  const char	*name; //NULL means index int: SFVec3f[0], MF[i]
//  char		index; //stable property index for switch/casing instead of strcmp on name
//  char		type; //0 = null, N=numeric I=Integer B=Boolean S=String, W=Object-web3d O-js Object P=ptr
//	char		isReadOnly; //T/F
//} FWPropertySpec;


FWPropertySpec (BrowserProperties)[] = {
	{"name", 0, 'S', 'T'},
	{"version", 1, 'S', 'T'},
	{"currentSpeed", 2, 'D', 'T'},
	{"currentFrameRate", 3, 'D', 'T'},
	{"description", 4, 'S', 0},
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
int BrowserGetter(FWType fwt, int index, void *ec, void *fwn, FWval fwretval){
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
			fwretval->itype = 'D';
			break;
		case 3: //currentFrameRate
			fwretval->_numeric = gglobal()->Mainloop.BrowserFPS;
			fwretval->itype = 'D';
			break;
		case 4: //description
			fwretval->_string = gglobal()->Mainloop.BrowserDescription; //this is settable
			fwretval->itype = 'S';
			break;
		case 5: //supportedComponents
			fwretval->_pointer.fieldType = AUXTYPE_ComponentInfoArray;
			fwretval->_pointer.native = (void*)getCapabilitiesTable(); //TO-DO something about component info array
			fwretval->_pointer.gc = 0;
			fwretval->itype = 'P';
			break;
		case 6: //supportedProfiles
			fwretval->_pointer.fieldType = AUXTYPE_ProfileInfoArray;
			fwretval->_pointer.native = (void*)getProfTable(); //TO-DO something about profile info array
			fwretval->_pointer.gc = 0;
			fwretval->itype = 'P';
			break;
		case 7: //currentScene
			fwretval->_web3dval.fieldType = AUXTYPE_X3DExecutionContext; //AUXTYPE_X3DScene;
			if(usingBrotos())
				fwretval->_web3dval.native = (void *)(struct X3D_Node*)ec; //X3DScene || X3DExecutionContext
			else
				fwretval->_web3dval.native = (void *)(struct X3D_Node*)rootNode(); //X3DScene || X3DExecutionContext
			fwretval->_web3dval.gc = 0;
				//change this to (Script)._executionContext when brotos working fully
			fwretval->itype = 'P';
			break;
		default:
			nr = 0;
		}
	return nr;
}
int BrowserSetter(FWType fwt, int index, void *ec, void *fwn, FWval fwval){
	switch (index) {
		case 4: //description is settable
			gglobal()->Mainloop.BrowserDescription = fwval->_string; //fwval->_web3dval.anyvrml->sfstring->strptr; 
			break;
		default:
			break;
	}
	return TRUE;
}


FWTYPE BrowserType = {
	AUXTYPE_X3DBrowser,
	'P',
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

int *intdup(int value){ 
	int* p = malloc(sizeof(int));
	memcpy(p,&value,sizeof(int));
	return p;
}
int ComponentInfoArrayGetter(FWType fwt, int index, void *ec, void *fwn, FWval fwretval){
	int *_table;
	int nr = 1;
	_table = (int *)fwn;
	//fwretval->itype = 'S'; //0 = null, N=numeric I=Integer B=Boolean S=String, W=Object-web3d O-js Object P=ptr F=flexiString(SFString,MFString[0] or ecmaString)
	if(index == -1){
//extern const char *COMPONENTS[];
//extern const int COMPONENTS_COUNT;
		int _length = capabilitiesHandler_getTableLength(_table); //COMPONENTS_COUNT;
		//fwretval->_integer = _length;
		fwretval->itype = 'I';
		fwretval->_integer = _length;

	}else if(index > -1 && index < COMPONENTS_COUNT ){
		fwretval->_pointer.native = &_table[2*index];
		fwretval->_pointer.fieldType = AUXTYPE_ComponentInfo;
		fwretval->_pointer.gc = 0;
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
	'P',
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
	{"level", 2, 'I', 'T'},
	{"providerUrl", 3, 'S', 'T'},
	{NULL,0,0,0},
};


int ComponentInfoGetter(FWType fwt, int index, void *ec, void *fwn, FWval fwretval){
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
	'P',
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


int ProfileInfoArrayGetter(FWType fwt, int index, void *ec, void *fwn, FWval fwretval){
	struct proftablestruct *_table;
	int nr = 1;
	_table = (struct proftablestruct *)fwn;
	//fwretval->itype = 'S'; //0 = null, N=numeric I=Integer B=Boolean S=String, W=Object-web3d O-js Object P=ptr F=flexiString(SFString,MFString[0] or ecmaString)
	if(index == -1){
//extern const char *COMPONENTS[];
//extern const int COMPONENTS_COUNT;
		fwretval->_integer = PROFILES_COUNT; // _length;
		fwretval->itype = 'I';
	}else if(index > -1 && index < PROFILES_COUNT ){
		fwretval->_pointer.native = &_table[index];
		fwretval->_pointer.fieldType = AUXTYPE_ProfileInfo;
		fwretval->_pointer.gc = 0;
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
	'P',
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
	{"Title", 1, 'I', 'T'},
	{"level", 2, 'S', 'T'},
	{"providerUrl", 3, 'S', 'T'},
	{"components", 4, 'P', 'T'}, //writable? if so then I need a constructor for ComponentInfo?
	{NULL,0,0,0},
};


int ProfileInfoGetter(FWType fwt, int index, void *ec, void *fwn, FWval fwretval){
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
			fwretval->_pointer.fieldType = AUXTYPE_ComponentInfoArray;
			fwretval->_pointer.gc = 0;
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
	'P',
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




struct X3D_Node *broto_search_DEFname(struct X3D_Proto *context, char *name);
struct X3D_Node * broto_search_ALLnames(struct X3D_Proto *context, char *name, int *source);
int X3DExecutionContext_getNamedNode(FWType fwtype, void *ec, void *fwn, int argc, FWval fwpars, FWval fwretval){
	int nr = 0;
	struct X3D_Node* node = NULL;
	//broto warning - DEF name list should be per-executionContext
	if(usingBrotos()){
		//struct X3D_Proto *ec = (struct X3D_Proto *)fwn; //we want the script node's parent context for imported nodes, I think
		node = broto_search_DEFname(ec, fwpars[0]._string);

	}else{
		node = parser_getNodeFromName(fwpars[0]._string); //_web3dval.anyvrml->sfstring->strptr); 
	}
	if(node){
		//fwretval->_web3dval.native = node;  //Q should this be &node? to convert it from X3D_Node to anyVrml->sfnode?
		fwretval->_web3dval.anyvrml = malloc(sizeof(union anyVrml));
		fwretval->_web3dval.anyvrml->sfnode = node;
		fwretval->_web3dval.fieldType = FIELDTYPE_SFNode;
		fwretval->_web3dval.gc = 1;
		fwretval->itype = 'W';
		nr = 1;
	}
	return nr;
}

int X3DExecutionContext_updateNamedNode(FWType fwtype, void *ec, void *fwn, int argc, FWval fwpars, FWval fwretval){
	int i, nr = 0;
	//broto warning - DEF name list should be per-executionContext
	if(usingBrotos()){
		struct X3D_Proto *ec = (struct X3D_Proto *)fwn;
		struct X3D_Node* node = NULL;
		struct brotoDefpair *bd;
		int found = 0;
		char *defname;
		defname = fwpars[0]._string;
		node = X3D_NODE(fwpars[1]._web3dval.native);
		if(ec->__DEFnames){
			for(i=0;i<vectorSize(ec->__DEFnames);i++){
				bd = vector_get(struct brotoDefpair *,ec->__DEFnames,i);
				//Q. is it the DEF we search for, and node we replace, OR
				//   is it the node we search for, and DEF we replace?
				if(!strcmp(bd->name,defname)){
					bd->node = node;
					found = 1;
					break;
				}
				if(bd->node == node){
					bd->name = strdup(defname);
					found = 2;
					break;
				}
			}
		}
		if(!found){
			//I guess its an add
			if(!ec->__DEFnames)
				ec->__DEFnames = newVector(struct brotoDefpair*,4);
			bd = (struct brotoDefpair*)malloc(sizeof(struct brotoDefpair));
			bd->node = node;
			bd->name = strdup(defname);
			stack_push(struct brotoDefpair *,ec->__DEFnames,bd);
		}
	}
	return nr;
}
int remove_broto_node(struct X3D_Proto *context, struct X3D_Node* node);
int X3DExecutionContext_removeNamedNode(FWType fwtype, void *ec, void *fwn, int argc, FWval fwpars, FWval fwretval){
	int i, nr = 0;
	//broto warning - DEF name list should be per-executionContext
	if(usingBrotos()){
		struct X3D_Proto *ec = (struct X3D_Proto *)fwn;
		struct X3D_Node* node = NULL;

		char *defname;
		defname = fwpars[0]._string;
		if(ec->__DEFnames){
			struct brotoDefpair *bd;
			for(i=0;i<vectorSize(ec->__DEFnames);i++){
				bd = vector_get(struct brotoDefpair *,ec->__DEFnames,i);
				if(!strcmp(bd->name,defname)){
					node = bd->node;
					//Q. are we supposed to delete the node 
					//OR are we just supposed to remove the DEF name mapping?
					//remove DEF name mapping:
					vector_remove_elem(struct brotoDefpair *,ec->__DEFnames,i);
					//remove node
					remove_broto_node(ec,node);
					break;
				}
			}
		}
	}
	return nr;
}
void add_node_to_broto_context(struct X3D_Proto *context,struct X3D_Node *node);
int X3DExecutionContext_createProto(FWType fwtype, void *ec, void *fwn, int argc, FWval fwpars, FWval fwretval){
	int nr = 0;
	struct X3D_Node* node = NULL;
	//broto warning - DEF name list should be per-executionContext
	if(usingBrotos()){
		struct X3D_Proto *ec = (struct X3D_Proto *)fwn;
		struct X3D_Proto *proto;
		proto = NULL;

		if( isAvailableBroto(fwpars[0]._string, ec, &proto))
		{
			struct X3D_Proto *source, *dest;
			node=X3D_NODE(brotoInstance(proto,1));
			node->_executionContext = X3D_NODE(ec); //me->ptr;
			add_node_to_broto_context(ec,node);
			//during parsing, setting of fields would occur between instance and body, 
			//so field values perculate down.
			//here we elect default field values
			source = X3D_PROTO(X3D_PROTO(node)->__prototype);
			dest = X3D_PROTO(node);
			deep_copy_broto_body2(&source,&dest);
		}
	}
	if(node){
		//fwretval->_web3dval.native = node;  //Q should this be &node? to convert it from X3D_Node to anyVrml->sfnode?
		fwretval->_web3dval.anyvrml = malloc(sizeof(union anyVrml));
		fwretval->_web3dval.anyvrml->sfnode = node;
		fwretval->_web3dval.fieldType = FIELDTYPE_SFNode;
		fwretval->_web3dval.gc = 1;
		fwretval->itype = 'W';
		nr = 1;
	}
	return nr;
}

int X3DExecutionContext_getImportedNode(FWType fwtype, void *ec, void *fwn, int argc, FWval fwpars, FWval fwretval){
	int nr = 0;
	struct X3D_Node* node = NULL;
	//broto warning - DEF name list should be per-executionContext
	if(usingBrotos()){
		struct X3D_Proto *ec = (struct X3D_Proto *)fwn;
		struct IMEXPORT *mxp;
		int source;
		//mxp = broto_search_IMPORTname(ec, fwpars[0]._string);
		//if(mxp){
		//	node = mxp->nodeptr;
		//}
		node = broto_search_ALLnames(ec, fwpars[0]._string,&source);
		if(source == 0) node = NULL;  //source ==1,2 is for IMPORT and EXPORT
	}
	if(node){
		//fwretval->_web3dval.native = node;  //Q should this be &node? to convert it from X3D_Node to anyVrml->sfnode?
		fwretval->_web3dval.anyvrml = malloc(sizeof(union anyVrml));
		fwretval->_web3dval.anyvrml->sfnode = node;
		fwretval->_web3dval.fieldType = FIELDTYPE_SFNode;
		fwretval->_web3dval.gc = 1;
		fwretval->itype = 'W';
		nr = 1;
	}
	return nr;
}

void update_weakRoutes(struct X3D_Proto *context);
int X3DExecutionContext_updateImportedNode(FWType fwtype, void *ec, void *fwn, int argc, FWval fwpars, FWval fwretval){
	// I think what they mean by updateImportedNode(string,string[,string]) is:
	//   updateImportedNode(Inline DEF name, Inline's Export AS name [,optional Import AS name])
	int i, nr = 0;
	//broto warning - DEF name list should be per-executionContext
	if(usingBrotos()){
		struct X3D_Proto *ec = (struct X3D_Proto *)fwn;
		struct X3D_Node* node = NULL;

		char *mxname, *as, *nline;
		int found = 0;
		struct IMEXPORT *mxp;

		nline = fwpars[0]._string;
		mxname = fwpars[1]._string;
		as = mxname;
		if(argc == 3)
			as = fwpars[2]._string;
		node = X3D_NODE(fwpars[1]._web3dval.native);
		if(ec->__IMPORTS){
			for(i=0;i<vectorSize(ec->__IMPORTS);i++){
				mxp = vector_get(struct IMEXPORT *,ec->__IMPORTS,i);
				//Q. is it the DEF we search for, and node we replace, OR
				//   is it the node we search for, and DEF we replace?
				if(!strcmp(nline,mxp->inlinename) && !strcmp(mxp->mxname,mxname)){
					mxp->as = strdup(as);
					found = 1;
					break;
				}
			}
		}
		if(!found){
			//I guess its an add
			if(!ec->__IMPORTS)
				ec->__IMPORTS = newVector(struct IMEXPORT *,4);
			mxp = (struct IMEXPORT *)malloc(sizeof(struct IMEXPORT));
			mxp->mxname = strdup(mxname);
			mxp->as = strdup(as);
			mxp->inlinename = strdup(nline);
			stack_push(struct IMEXPORT *,ec->__IMPORTS,mxp);
		}
		update_weakRoutes(ec);
	}
	return nr;
}


int X3DExecutionContext_removeImportedNode(FWType fwtype, void *ec, void *fwn, int argc, FWval fwpars, FWval fwretval){
	int i,nr = 0;
	//broto warning - DEF name list should be per-executionContext
	if(usingBrotos()){
		struct X3D_Proto *ec = (struct X3D_Proto *)fwn;
		//struct X3D_Node* node = NULL;

		char *defname;
		defname = fwpars[0]._string;
		if(ec->__IMPORTS){
			struct IMEXPORT *mxp;
			for(i=0;i<vectorSize(ec->__IMPORTS);i++){
				mxp = vector_get(struct IMEXPORT *,ec->__IMPORTS,i);
				if(!strcmp(mxp->as,defname)){
					//remove IMPORT name mapping:
					vector_remove_elem(struct IMEXPORT *,ec->__IMPORTS,i);
					update_weakRoutes(ec);
					break;
				}
			}
		}
	}
	return nr;
}


int X3DScene_getExportedNode(FWType fwtype, void *ec, void *fwn, int argc, FWval fwpars, FWval fwretval){
	int nr = 0;
	struct X3D_Node* node = NULL;
	//broto warning - DEF name list should be per-executionContext
	if(usingBrotos()){
		struct X3D_Proto *ec = (struct X3D_Proto *)fwn;
		struct IMEXPORT *mxp;
		mxp = broto_search_EXPORTname(ec, fwpars[0]._string);
		if(mxp){
			node = mxp->nodeptr;
		}
	}
	if(node){
		//fwretval->_web3dval.native = node;  //Q should this be &node? to convert it from X3D_Node to anyVrml->sfnode?
		fwretval->_web3dval.anyvrml = malloc(sizeof(union anyVrml));
		fwretval->_web3dval.anyvrml->sfnode = node;
		fwretval->_web3dval.fieldType = FIELDTYPE_SFNode;
		fwretval->_web3dval.gc = 1;
		fwretval->itype = 'W';
		nr = 1;
	}
	return nr;
}

int X3DScene_updateExportedNode(FWType fwtype, void *ec, void *fwn, int argc, FWval fwpars, FWval fwretval){
	int i, nr = 0;
	//broto warning - DEF name list should be per-executionContext
	if(usingBrotos()){
		struct X3D_Proto *ec = (struct X3D_Proto *)fwn;
		struct X3D_Node* node = NULL;

		char *defname;
		int found = 0;
		struct IMEXPORT *mxp;

		defname = fwpars[0]._string;
		node = X3D_NODE(fwpars[1]._web3dval.anyvrml->sfnode);
		if(ec->__EXPORTS){
			for(i=0;i<vectorSize(ec->__EXPORTS);i++){
				mxp = vector_get(struct IMEXPORT *,ec->__EXPORTS,i);
				//Q. is it the DEF we search for, and node we replace, OR
				//   is it the node we search for, and DEF we replace?
				if(!strcmp(mxp->as,defname)){
					mxp->nodeptr = node;
					found = 1;
					break;
				}
				if(mxp->nodeptr == node){
					mxp->as = strdup(defname);
					found = 2;
					break;
				}
			}
		}
		if(!found){
			//I guess its an add
			if(!ec->__EXPORTS)
				ec->__EXPORTS = newVector(struct IMEXPORT *,4);
			mxp = (struct IMEXPORT *)malloc(sizeof(struct IMEXPORT));
			mxp->nodeptr = node;
			mxp->mxname = strdup(defname);
			mxp->as = mxp->mxname;
			stack_push(struct IMEXPORT *,ec->__EXPORTS,mxp);
		}
		if(ec->_executionContext){
			//update weak routes in the importing context, which should be the parent context of the Inline node
			//and the Inline should be our current context. I think.
			update_weakRoutes(X3D_PROTO(ec->_executionContext));
		}
	}
	return nr;
}

int X3DScene_removeExportedNode(FWType fwtype, void *ec, void *fwn, int argc, FWval fwpars, FWval fwretval){
	int i, nr = 0;
	//broto warning - DEF name list should be per-executionContext
	if(usingBrotos()){
		struct X3D_Proto *ec = (struct X3D_Proto *)fwn;
		//struct X3D_Node* node = NULL;

		char *defname;
		defname = fwpars[0]._string;
		if(ec->__EXPORTS){
			struct IMEXPORT *mxp;
			for(i=0;i<vectorSize(ec->__EXPORTS);i++){
				mxp = vector_get(struct IMEXPORT *,ec->__EXPORTS,i);
				if(!strcmp(mxp->as,defname)){
					//remove EXPORT name mapping:
					vector_remove_elem(struct IMEXPORT *,ec->__EXPORTS,i);
					break;
				}
			}
		}
		if(ec->_executionContext){
			//update weak routes in the importing context, which should be the parent context of the Inline node
			//and the Inline should be our current context. I think.
			update_weakRoutes(X3D_PROTO(ec->_executionContext));
		}
	}
	return nr;
}
int X3DScene_setMetaData(FWType fwtype, void *ec, void *fwn, int argc, FWval fwpars, FWval fwretval){
	int nr = 0;
	char *name, *value;
	name = fwpars[0]._string;
	value = fwpars[1]._string;
	//strdup and put in a global or per-scene or per execution context (name,value) list
	return nr;
}
int X3DScene_getMetaData(FWType fwtype, void *ec, void *fwn, int argc, FWval fwpars, FWval fwretval){
	int nr = 0;
	char *name, *value;
	value = NULL;
	name = fwpars[0]._string;
	//do a search in the perscene/perexecution context array
	if(value){
		fwretval->_string = value;
		fwretval->itype = 'S';
		nr = 1;
	}
	return nr;
}

static FWFunctionSpec (X3DExecutionContextFunctions)[] = {
	//executionContext
	{"addRoute", VrmlBrowserAddRoute, 'P',{4,-1,0,"WSWS"}},
	{"deleteRoute", X3DExecutionContext_deleteRoute,'0',{1,-1,0,"P"}},
	{"createNode", VrmlBrowserCreateX3DFromString, 'W',{1,-1,0,"S"}},
	{"createProto", X3DExecutionContext_createProto, 'W',{1,-1,0,"S"}},
	{"getImportedNode", X3DExecutionContext_getImportedNode, 'W',{1,-1,0,"S"}},
	{"updateImportedNode", X3DExecutionContext_updateImportedNode, '0',{3,-1,0,"SSS"}},
	{"removeImportedNode", X3DExecutionContext_removeImportedNode, '0',{1,-1,0,"S"}},
	{"getNamedNode", X3DExecutionContext_getNamedNode, 'W',{1,-1,0,"S"}},
	{"updateNamedNode", X3DExecutionContext_updateNamedNode, '0',{2,-1,0,"SW"}},
	{"removeNamedNode", X3DExecutionContext_removeNamedNode, '0',{1,-1,0,"S"}},
	////scene
	{"setMetaData", X3DScene_setMetaData, '0',{2,-1,0,"SS"}},
	{"getMetaData", X3DScene_getMetaData, 'S',{1,-1,0,"S"}},
	{"getExportedNode", X3DScene_getExportedNode, 'W',{1,-1,0,"S"}},
	{"updateExportedNode", X3DScene_updateExportedNode, '0',{2,-1,0,"SW"}},
	{"removeExportedNode", X3DScene_removeExportedNode, '0',{1,-1,0,"S"}},
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
static int _TRUE = TRUE;
static int _FALSE = FALSE;
int X3DExecutionContextGetter(FWType fwt, int index, void *ec, void *fwn, FWval fwretval){
	struct X3D_Proto * ecc;
	int nr = 1;
	ecc = (struct X3D_Proto*)fwn;
	//fwretval->itype = 'S'; //0 = null, N=numeric I=Integer B=Boolean S=String, W=Object-web3d O-js Object P=ptr F=flexiString(SFString,MFString[0] or ecmaString)
	switch (index) {
		case 0: //specificationVersion
		{
			char str[32]; 
			sprintf(str,"{%d,%d,%d}",inputFileVersion[0],inputFileVersion[1],inputFileVersion[2]);
			fwretval->_string = strdup(str);
			fwretval->itype = 'S';
		}
			break;
		case 1: //encoding string readonly
		{
			//Valid values are "ASCII", "VRML", "XML", "BINARY", "SCRIPTED", "BIFS", "NONE" 
			fwretval->_string = "not filled in yet sb. VRML or XML or ..";
			fwretval->itype = 'S';
		}
			break;
		case 2: //profile
		{
			struct proftablestruct * profile;
			int index = gglobal()->Mainloop.scene_profile;
			profile = getProfTable();
			fwretval->_pointer.native = &profile[index];
			fwretval->_pointer.fieldType = AUXTYPE_ProfileInfo;
			fwretval->_pointer.gc = 0;
			fwretval->itype = 'P';
		}
			break;
		case 3: //components
			fwretval->_pointer.native = (void *)gglobal()->Mainloop.scene_components;
			fwretval->_pointer.fieldType = AUXTYPE_ComponentInfoArray;
			fwretval->_pointer.gc = 0;
			fwretval->itype = 'P';
			break;
		case 4: //worldURL
			fwretval->_string =  gglobal()->Mainloop.url; //this is settable
			fwretval->itype = 'S';
			break;
		case 5: //rootNodes
			if(usingBrotos())
				fwretval->_web3dval.native = (void *)&ecc->__children;  //broto warning: inside a proto should be the rootnodes of the protobody
			else
				fwretval->_web3dval.native = (void *)&((struct X3D_Group*)rootNode())->children;  //broto warning: inside a proto should be the rootnodes of the protobody
			fwretval->_web3dval.fieldType = FIELDTYPE_MFNode;
			fwretval->_web3dval.gc = 0;
			fwretval->itype = 'W';
			break;
		case 6: //protos
			if(usingBrotos()){
				fwretval->_pointer.fieldType = AUXTYPE_X3DProtoArray; 
				fwretval->_pointer.native = (void*)ecc->__protoDeclares; //broto: this should be a per-context array
				fwretval->_pointer.gc = 0;
				fwretval->itype = 'P';
			}else{
				fwretval->itype = '0'; //not implemented yet
				nr = 0;
			}
			break;
		case 7: //externprotos
			if(usingBrotos()){
				fwretval->_pointer.fieldType = AUXTYPE_X3DExternProtoArray; 
				fwretval->_pointer.native = (void*)ecc->__externProtoDeclares; //broto: this should be a per-context array
				fwretval->_pointer.gc = 0;
				fwretval->itype = 'P';
			}else{
				fwretval->itype = '0'; //not implemented yet
				nr = 0;
			}
			break;
		case 8: //routes
			fwretval->_pointer.fieldType = AUXTYPE_X3DRouteArray; 
			if(usingBrotos())
				fwretval->_pointer.native = (void*)ecc->__ROUTES; //broto: this should be a per-context array
			else
				fwretval->_pointer.native = NULL; 
			fwretval->_pointer.gc = 0;
			fwretval->itype = 'P';
			break;
		case 9: //isScene
			//fwretval->_boolean = TRUE; //broto warning: should be false inside a protoinstance body
			fwretval->itype = 'B';
			if(usingBrotos()){
				unsigned char flag = ciflag_get(ecc->__protoFlags,2);
				if(flag == 2)
					fwretval->_boolean = TRUE;
				else
					fwretval->_boolean = FALSE;
			}else
				fwretval->_boolean = TRUE;
			break;
		default:
			nr = 0;
		}
	return nr;
}

FWTYPE X3DExecutionContextType = {
	AUXTYPE_X3DExecutionContext,
	'P',
	"X3DExecutionContext",
	0, //sizeof(struct X3DBrowser), 
	NULL, //no constructor
	NULL, //no constructor args
	X3DExecutionContextProperties,
	NULL, //no special has
	X3DExecutionContextGetter,
	NULL,
	0,0, //takes int index in prop
	X3DExecutionContextFunctions,
};

struct CRStruct *getCRoutes();
int getCRouteCount();
int *getCRouteCounter();
int X3DRouteArrayGetter(FWType fwt, int index, void *ec, void *fwn, FWval fwretval){
	
	int nr = 0;
	//fwretval->itype = 'S'; //0 = null, N=numeric I=Integer B=Boolean S=String, W=Object-web3d O-js Object P=ptr F=flexiString(SFString,MFString[0] or ecmaString)
	if(index == -1){
		int _length;
		if(usingBrotos())
			_length = vectorSize(fwn);
		else
			_length = getCRouteCount();
		fwretval->_integer = _length;
		fwretval->itype = 'I';
		nr = 1;
	}else if(index > -1 ){
		if(usingBrotos()){
			if(index < vectorSize(fwn)){
				fwretval->_pointer.native = vector_get(void *, fwn, index); //struct brotoRoute *
				fwretval->_pointer.gc = 0;
				fwretval->_pointer.fieldType = AUXTYPE_X3DRoute;
				fwretval->itype = 'P';
				nr = 1;
			}
		}else{
			//struct CRStruct *routes = getCRoutes();
			if( index < getCRouteCount() ){
				fwretval->_pointer.native = intdup(index+1);
				fwretval->_pointer.gc = 1;
				fwretval->_pointer.fieldType = AUXTYPE_X3DRoute;
				fwretval->itype = 'P';
				nr = 1;
			}
		}
	}
	return nr;
}


FWPropertySpec (X3DRouteArrayProperties)[] = {
	{"length", -1, 'I', 'T'},
	{NULL,0,0,0},
};

FWTYPE X3DRouteArrayType = {
	AUXTYPE_X3DRouteArray,
	'P',
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
int X3DRouteGetter(FWType fwt, int index, void *ec, void *fwn, FWval fwretval){
	union anyVrml *value;
	int type, kind;
	char *fieldname, *sfromfield, *stofield;
	struct X3D_Node *fromNode, *toNode;
	int fromIndex, toIndex;
	int nr = 1;
	if(usingBrotos()){
		struct brotoRoute* broute = (struct brotoRoute*)fwn;
		fromNode = broute->from.node;
		fromIndex = broute->from.ifield;
		toNode = broute->to.node;
		toIndex = broute->to.ifield;
	}else{
		//route = (struct CRStruct *)fwn;
		struct CRStruct *route;
		int indexr = *(int *)fwn;
		getSpecificRoute (indexr,&fromNode, &fromIndex, &toNode, &toIndex);
	}
	if(!fromNode || !toNode) return 0;

	//fwretval->itype = 'S'; //0 = null, N=numeric I=Integer B=Boolean S=String, W=Object-web3d O-js Object P=ptr F=flexiString(SFString,MFString[0] or ecmaString)
	switch(index){
	case 0: //fromNode
		//fwretval->_web3dval.native = (void*)fromNode; //route->routeFromNode;
		//((union anyVrml*)fwpars[0]._web3dval.native)->sfnode
		fwretval->_web3dval.anyvrml = malloc(sizeof(union anyVrml));
		fwretval->_web3dval.anyvrml->sfnode = fromNode;
		fwretval->_web3dval.fieldType = FIELDTYPE_SFNode;
		fwretval->_web3dval.gc = 1;
		fwretval->itype = 'W';
		break;
	case 1: //fromField
		//fieldname = findFIELDNAMESfromNodeOffset0(fromNode,fromOffset);
		getFieldFromNodeAndIndex(fromNode,fromIndex,&fieldname,&type,&kind,&value);
		fwretval->_string = fieldname; //NULL;
		fwretval->itype = 'S';
		break;
	case 2: //toNode
		//fwretval->_web3dval.native = (void*)toNode; //route->routeFromNode;
		fwretval->_web3dval.anyvrml = malloc(sizeof(union anyVrml));
		fwretval->_web3dval.anyvrml->sfnode = toNode;
		fwretval->_web3dval.fieldType = FIELDTYPE_SFNode;
		fwretval->itype = 'W';
		fwretval->_web3dval.gc = 1;
		break;
	case 3: //toField
		//getFieldFromNodeAndIndex(route->tonodes[0].routeToNode,route->tonodes[0].foffset,&fieldname,&type,&kind,&value);
		getFieldFromNodeAndIndex(toNode,toIndex,&fieldname,&type,&kind,&value);
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
	'P',
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



int X3DProtoArrayGetter(FWType fwt, int index, void *ec, void *fwn, FWval fwretval){
	
	int nr = 0;
	//fwretval->itype = 'S'; //0 = null, N=numeric I=Integer B=Boolean S=String, W=Object-web3d O-js Object P=ptr F=flexiString(SFString,MFString[0] or ecmaString)
	if(index == -1){
		int _length;
		if(usingBrotos())
			_length = vectorSize(fwn);
		else
			_length = 0;
		fwretval->_integer = _length;
		fwretval->itype = 'I';
		nr = 1;
	}else if(index > -1 ){
		if(usingBrotos()){
			if(index < vectorSize(fwn)){
				fwretval->_pointer.native = vector_get(void *, fwn, index); //struct X3D_Proto *
				fwretval->_pointer.gc = 1;
				fwretval->_pointer.fieldType = AUXTYPE_X3DProto;
				fwretval->itype = 'P';
				nr = 1;
			}
		}else{
			nr = 0;
		}
	}
	return nr;
}


FWPropertySpec (X3DProtoArrayProperties)[] = {
	{"length", -1, 'I', 'T'},
	{NULL,0,0,0},
};

FWTYPE X3DProtoArrayType = {
	AUXTYPE_X3DProtoArray,
	'P',
	"X3DProtoArray",
	0, //sizeof(struct X3DProto), 
	NULL, //no constructor for X3DProto
	NULL, //no constructor args
	X3DProtoArrayProperties,
	NULL, //no special has
	X3DProtoArrayGetter,
	NULL,
	'P', 'T',//takes int index in prop, readonly
	NULL,
};

FWTYPE X3DExternProtoArrayType = {
	AUXTYPE_X3DExternProtoArray,
	'P',
	"X3DExternProtoArray",
	0, //sizeof(struct X3DProto), 
	NULL, //no constructor for X3DProto
	NULL, //no constructor args
	X3DProtoArrayProperties,
	NULL, //no special has
	X3DProtoArrayGetter,
	NULL,
	'P', 'T',//takes int index in prop, readonly
	NULL,
};


struct tuplePointerInt {
	void *pointer;
	int integer;
};

FWPropertySpec (X3DProtoProperties)[] = {
	{"name", 0, 'S', 'T'},
	{"fields", 1, 'W', 'T'},
	{"isExternProto", 2, 'B', 'T'},
	{NULL,0,0,0},
};
int X3DProtoGetter(FWType fwt, int index, void *ec, void *fwn, FWval fwretval){
	int nr = 0;
	//fwretval->itype = 'S'; //0 = null, N=numeric I=Integer B=Boolean S=String, W=Object-web3d O-js Object P=ptr F=flexiString(SFString,MFString[0] or ecmaString)
	switch(index){
	case 0: //name
		fwretval->_string = X3D_PROTO(fwn)->__typename; //NULL;
		fwretval->itype = 'S';
		break;
	case 1: //fields
		fwretval->_web3dval.native = fwn; //we'll get field[i] from the proto later (void*)X3D_PROTO(fwn)->__protoDef; //route->routeFromNode;
		fwretval->_web3dval.fieldType = AUXTYPE_X3DFieldDefinitionArray;
		fwretval->itype = 'W';
		fwretval->_web3dval.gc = 0;
		break;
	case 2: //isExternProto
		fwretval->itype = 'B';
		if(usingBrotos()){
			unsigned char flag = ciflag_get(X3D_PROTO(fwn)->__protoFlags,3);
			if(flag == 1)
				fwretval->_boolean = TRUE;
			else
				fwretval->_boolean = FALSE;
		}else
			fwretval->_boolean = TRUE;
		break;
	default:
		nr = 0;
	}
	return nr;
}


FWTYPE X3DProtoType = {
	AUXTYPE_X3DProto,
	'P',
	"X3DProtoDeclaration",
	0, //sizeof(struct X3DProto), 
	NULL, //no constructor for X3DProto
	NULL, //no constructor args
	X3DProtoProperties,
	NULL, //no special has
	X3DProtoGetter,
	NULL,
	0,0, //takes int index in prop
	NULL,
};
FWTYPE X3DExternProtoType = {
	AUXTYPE_X3DExternProto,
	'P',
	"X3DExternProtoDeclaration",
	0, //sizeof(struct X3DProto), 
	NULL, //no constructor for X3DProto
	NULL, //no constructor args
	X3DProtoProperties,  //it tests if its extern or not
	NULL, //no special has
	X3DProtoGetter,
	NULL,
	0,0, //takes int index in prop
	NULL,
};

int count_fields(struct X3D_Node* node);
int X3DFieldDefinitionArrayGetter(FWType fwt, int index, void *ec, void *fwn, FWval fwretval){
	
	int nr = 0;
	struct X3D_Node *node = (struct X3D_Node*)fwn;

	//fwretval->itype = 'S'; //0 = null, N=numeric I=Integer B=Boolean S=String, W=Object-web3d O-js Object P=ptr F=flexiString(SFString,MFString[0] or ecmaString)
	if(index == -1){
		int _length = 0;
		//I suspect this fieldDefinition stuff is for ProtoDeclares and ExternProtoDeclares only, not builtin or protoInstances or scripts
		if(usingBrotos()){ 
			_length = count_fields(node); 
		}else
			_length = 0;
		fwretval->_integer = _length;
		fwretval->itype = 'I';
		nr = 1;
	}else if(index > -1 ){
		if(usingBrotos()){
			if(index < vectorSize(fwn)){
				struct tuplePointerInt *tpi = malloc(sizeof(struct tuplePointerInt));
				tpi->pointer = (void*)node;  
				tpi->integer = index;
				fwretval->_pointer.native = tpi; //vector_get(void *, fwn, index); //struct X3D_Proto *
				fwretval->_pointer.gc = 1;
				fwretval->_pointer.fieldType = AUXTYPE_X3DFieldDefinition;
				fwretval->itype = 'P';
				nr = 1;
			}
		}else{
			nr = 0;
		}
	}
	return nr;
}


FWPropertySpec (X3DFieldDefinitionArrayProperties)[] = {
	{"length", -1, 'I', 'T'},
	{NULL,0,0,0},
};

FWTYPE X3DFieldDefinitionArrayType = {
	AUXTYPE_X3DFieldDefinitionArray,
	'P',
	"X3DFieldDefinitionArray",
	0, //sizeof(struct X3DProto), 
	NULL, //no constructor for X3DProto
	NULL, //no constructor args
	X3DFieldDefinitionArrayProperties,
	NULL, //no special has
	X3DFieldDefinitionArrayGetter,
	NULL,
	'P', 'T',//takes int index in prop, readonly
	NULL,
};

FWPropertySpec (X3DFieldDefinitionProperties)[] = {
	{"name", 0, 'S', 'T'},
	{"accessType", 1, 'I', 'T'},
	{"dataType", 2, 'I', 'T'},
	{NULL,0,0,0},
};
int X3DFieldDefinitionGetter(FWType fwt, int index, void *ec, void *fwn, FWval fwretval){
	int ifield, type,kind, konstindex, nr = 0;
	struct string_int * si;
	union anyVrml *value;
	struct X3D_Node* node;
	char *fname;
	struct tuplePointerInt *tpi = (struct tuplePointerInt*)fwn;
	node = tpi->pointer;
	ifield = tpi->integer;
	//I suspect FieldDefinitions are for ProtoDeclarations only, 
	// but freewrl usingBrotos() can use the same function for nodes and declares
	if(getFieldFromNodeAndIndex(node,ifield,&fname,&type,&kind,&value)){
		//fwretval->itype = 'S'; //0 = null, N=numeric I=Integer B=Boolean S=String, W=Object-web3d O-js Object P=ptr F=flexiString(SFString,MFString[0] or ecmaString)
		switch(index){
		case 0: //name
			fwretval->_string = fname; //NULL;
			fwretval->itype = 'S';
			break;
		case 1: //accessType
			si = lookup_string_int(lookup_X3DConstants,PROTOKEYWORDS[kind],&konstindex);
			fwretval->_integer = konstindex; //index into x3dconstants table
			fwretval->itype = 'I';
			break;
		case 2: //dataType
			si = lookup_string_int(lookup_X3DConstants,FIELDTYPES[type],&konstindex);
			fwretval->_integer = konstindex; //index into x3dconstants table
			fwretval->itype = 'I';
			break;
		default:
			nr = 0;
		}
	}
	return nr;
}


FWTYPE X3DFieldDefinitionType = {
	AUXTYPE_X3DFieldDefinition,
	'P',
	"X3DFieldDefinition",
	0, //sizeof(struct X3DFieldDefinition), 
	NULL, //no constructor for X3DProto
	NULL, //no constructor args
	X3DFieldDefinitionProperties,
	NULL, //no special has
	X3DFieldDefinitionGetter,
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

	typeArray[*n] = &X3DProtoType; (*n)++;
	typeArray[*n] = &X3DProtoArrayType; (*n)++;
	typeArray[*n] = &X3DExternProtoType; (*n)++;
	typeArray[*n] = &X3DExternProtoArrayType; (*n)++;
	typeArray[*n] = &X3DFieldDefinitionType; (*n)++;
	typeArray[*n] = &X3DFieldDefinitionArrayType; (*n)++;

}
#endif /* ifdef JAVASCRIPT_DUK */