/*

  Main functions II (how to define the purpose of this file?).
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
#include <system.h>
#include <system_threads.h>
#include <display.h>
#include <internal.h>

#include <libFreeWRL.h>
#include <list.h>

#include <io_files.h>
#include <io_http.h>


#include <threads.h>

#include "../vrml_parser/Structs.h"
#include "headers.h"
#include "../vrml_parser/CParseGeneral.h"
#include "../scenegraph/Vector.h"
#include "../vrml_parser/CFieldDecls.h"
#include "../world_script/JScript.h"
#include "../world_script/CScripts.h"
#include "../vrml_parser/CParseParser.h"
#include "../vrml_parser/CParseLexer.h"
#include "../vrml_parser/CParse.h"
#include "Snapshot.h"
#include "../scenegraph/Collision.h"
#include "../scenegraph/Component_KeyDevice.h"
#include "../opengl/Frustum.h"

#if defined(INCLUDE_NON_WEB3D_FORMATS)
#include "../non_web3d_formats/ColladaParser.h"
#endif //INCLUDE_NON_WEB3D_FORMATS

#if defined (INCLUDE_STL_FILES)
#include "../input/convertSTL.h"
#endif //INCLUDE_STL_FILES

#include "../scenegraph/quaternion.h"
#include "../scenegraph/Viewer.h"
#include "../input/SensInterps.h"
#include "../x3d_parser/Bindable.h"
#include "../input/InputFunctions.h"
#ifndef DISABLER
#include "../plugin/pluginUtils.h"
#include "../plugin/PluginSocket.h"
#endif
#include "../ui/common.h"

#include "../opengl/Textures.h"
#include "../opengl/LoadTextures.h"


#include "MainLoop.h"
#include "ProdCon.h"



/* used by the paser to call back the lexer for EXTERNPROTO */
void embedEXTERNPROTO(struct VRMLLexer *me, char *myName, char *buffer, char *pound);

//true statics:
char *EAI_Flag = "From the EAI bootcamp of life ";
char* PluginPath = "/private/tmp";
int PluginLength = 12;

//I think these are Aqua - I'll leave them static pending Aqua guru plugin review:
int _fw_browser_plugin = 0;
int _fw_pipe = 0;
uintptr_t _fw_instance = 0;


/*******************************/


struct PSStruct {
	unsigned type;		/* what is this task? 			*/
	char *inp;		/* data for task (eg, vrml text)	*/
	void *ptr;		/* address (node) to put data		*/
	unsigned ofs;		/* offset in node for data		*/
	int zeroBind;		/* should we dispose Bindables?	 	*/
	int bind;		/* should we issue a bind? 		*/
	char *path;		/* path of parent URL			*/
	int *comp;		/* pointer to complete flag		*/

	char *fieldname;	/* pointer to a static field name	*/
	int jparamcount;	/* number of parameters for this one	*/
	struct Uni_String *sv;			/* the SV for javascript		*/
};

static bool parser_do_parse_string(const char *input, const int len, struct X3D_Node *ectx, struct X3D_Node *nRn);

/* Bindables */
typedef struct pProdCon {
		struct Vector *fogNodes;
		struct Vector *backgroundNodes;
		struct Vector *navigationNodes;

		/* thread synchronization issues */
		int _P_LOCK_VAR;// = 0;
		s_list_t *resource_list_to_parse; // = NULL;
		s_list_t *frontend_list_to_get; // = NULL;
		int frontend_gets_files;
		/* psp is the data structure that holds parameters for the parsing thread */
		struct PSStruct psp;
		/* is the inputParse thread created? */
		//int inputParseInitialized; //=FALSE;

		/* is the parsing thread active? this is read-only, used as a "flag" by other tasks */
		int inputThreadParsing; //=FALSE;
		int haveParsedCParsed; // = FALSE; 	/* used to tell when we need to call destroyCParserData  as destroyCParserData can segfault otherwise */

#if defined (INCLUDE_STL_FILES)
		/* stl files have no implicit scale. This scale will make it fit into a reasonable boundingBox */
		float lastSTLScaling;
#endif

}* ppProdCon;
void *ProdCon_constructor(){
	void *v = MALLOCV(sizeof(struct pProdCon));
	memset(v,0,sizeof(struct pProdCon));
	return v;
}
void ProdCon_init(struct tProdCon *t)
{
	//public
	t->viewpointNodes = newVector(struct X3D_Node *,8);
	t->currboundvpno=0;

	/* bind nodes in display loop, NOT in parsing threadthread */
	t->setViewpointBindInRender = NULL;
	t->setFogBindInRender = NULL;
	t->setBackgroundBindInRender = NULL;
	t->setNavigationBindInRender = NULL;
	/* make up a new parser for parsing from createVrmlFromURL and createVrmlFromString */
	t->savedParser = NULL; //struct VRMLParser* savedParser;

	//private
	t->prv = ProdCon_constructor();
	{
		ppProdCon p = (ppProdCon)t->prv;
		p->fogNodes = newVector(struct X3D_Node *, 2);
		p->backgroundNodes = newVector(struct X3D_Node *, 2);
		p->navigationNodes = newVector(struct X3D_Node *, 2);
        	//printf ("created new navigationNodes of %p, at A\n",p->navigationNodes);


		/* thread synchronization issues */
		p->_P_LOCK_VAR = 0;
		p->resource_list_to_parse = NULL;
		p->frontend_list_to_get = NULL;
#ifndef DISABLER
		p->frontend_gets_files = 2; //dug9 Sep 1, 2013 used to test new fgf method in win32; July2014 we're back, for Async 1=main.c 2=_displayThread
#else
		p->frontend_gets_files = 0; //Disabler
#endif
		/* psp is the data structure that holds parameters for the parsing thread */
		//p->psp;
		/* is the inputParse thread created? */
		//p->inputParseInitialized=FALSE;
		/* is the parsing thread active? this is read-only, used as a "flag" by other tasks */
		p->inputThreadParsing=FALSE;
		p->haveParsedCParsed = FALSE; 	/* used to tell when we need to call destroyCParserData
				   as destroyCParserData can segfault otherwise */

#if defined (INCLUDE_STL_FILES)
                /* stl files have no implicit scale. This scale will make it fit into a reasonable boundingBox */
                p->lastSTLScaling = 0.1;
#endif

	}
}
void ProdCon_clear(struct tProdCon *t){
	deleteVector(struct X3D_Node *,t->viewpointNodes);
	if(t->prv)
	{
		ppProdCon p = (ppProdCon)t->prv;
		deleteVector(struct X3D_Node *, p->fogNodes);
		deleteVector(struct X3D_Node *, p->backgroundNodes);
		deleteVector(struct X3D_Node *, p->navigationNodes);
	}
}
///* is the inputParse thread created? */
//static int inputParseInitialized=FALSE;
//
///* is the parsing thread active? this is read-only, used as a "flag" by other tasks */
//int inputThreadParsing=FALSE;

/* /\* Is the initial URL loaded ? Robert Sim *\/ */
/* int URLLoaded = FALSE; */
/* int isURLLoaded() { return (URLLoaded && !inputThreadParsing); } */

///* psp is the data structure that holds parameters for the parsing thread */
//struct PSStruct psp;

//static int haveParsedCParsed = FALSE; 	/* used to tell when we need to call destroyCParserData
//				   as destroyCParserData can segfault otherwise */



#if defined (INCLUDE_STL_FILES)
/* stl files have no implicit scale. This scale will make it fit into a reasonable boundingBox */
float fwl_getLastSTLScaling() {
	ppProdCon p = (ppProdCon)gglobal()->ProdCon.prv;
	if (p!=NULL) return p->lastSTLScaling;
	return 1.0;
}
#endif


/* is a parser running? this is a function, because if we need to mutex lock, we
   can do all locking in this file */
int fwl_isInputThreadInitialized() {
	//ppProdCon p = (ppProdCon)gglobal()->ProdCon.prv;
	//return p->inputParseInitialized;
	return gglobal()->threads.ResourceThreadRunning;
}

/* statusbar uses this to tell user that we are still loading */
int fwl_isinputThreadParsing() {
	ppProdCon p = (ppProdCon)gglobal()->ProdCon.prv;
	return(p->inputThreadParsing);
}
void sceneInstance(struct X3D_Proto* proto, struct X3D_Group *scene);
/* BOOL usingBrotos(); -- moved to CParseParser.h */
void dump_scene2(FILE *fp, int level, struct X3D_Node* node, int recurse, Stack *DEFedNodes) ;

int indexChildrenName(struct X3D_Node *node){
	int index = -1; //we'll have it work like a bool too, and I don't think any of our children field are at 0
	if(node)
		switch(node->_nodeType){
			case NODE_Group:
				index = FIELDNAMES_children;
				break;
			case NODE_Transform:
				index = FIELDNAMES_children;
				break;
			case NODE_Switch:
				index = FIELDNAMES_children;
				break;
			case NODE_Billboard:
				index = FIELDNAMES_children;
				break;
			case NODE_Proto:
				index = FIELDNAMES___children;
				break;
			case NODE_Inline:  //Q. do I need this in here? Saw code in x3dparser.
				index = FIELDNAMES___children;
				break;
			case NODE_GeoLOD:  //Q. do I need this in here? A. No - leave null and the correct field is found for geoOrigin (otherwise it puts it in rootNode which is a kind of parent node field -sick - see glod1/b.x3d)
				index = FIELDNAMES_rootNode;
				break;
			//switch?
		}
	return index;

}
struct Multi_Node *childrenField(struct X3D_Node *node){
	struct Multi_Node *childs = NULL;
	if(node)
		switch(node->_nodeType){
			case NODE_Group:
				childs = offsetPointer_deref(void*, node,  offsetof(struct X3D_Group,children));
				break;
			case NODE_Transform:
				childs = offsetPointer_deref(void*, node,  offsetof(struct X3D_Transform,children));
				break;
			case NODE_Switch:
				childs = offsetPointer_deref(void*, node,  offsetof(struct X3D_Switch,children));
				break;
			case NODE_Billboard:
				childs = offsetPointer_deref(void*, node,  offsetof(struct X3D_Billboard,children));
				break;
			case NODE_Proto:
				childs = offsetPointer_deref(void*, node,  offsetof(struct X3D_Proto,__children));
				break;
			case NODE_Inline:  //Q. do I need this in here? Saw code in x3dparser.
				childs = offsetPointer_deref(void*, node,  offsetof(struct X3D_Inline,__children));
				break;
			case NODE_GeoLOD:  //Q. do I need this in here?
				childs = offsetPointer_deref(void*, node, offsetof(struct X3D_GeoLOD,rootNode));
				break;
		}
	return childs;
}
int offsetofChildren(struct X3D_Node *node){
	int offs = 0; //we'll have it work like a bool too, and I don't think any of our children field are at 0
	if(node)
		switch(node->_nodeType){
			case NODE_Group:
				offs = offsetof(struct X3D_Group,children);
				break;
			case NODE_Transform:
				offs = offsetof(struct X3D_Transform,children);
				break;
			case NODE_Switch:
				offs = offsetof(struct X3D_Switch,children);
				break;
			case NODE_Billboard:
				offs = offsetof(struct X3D_Billboard,children);
				break;
			case NODE_Proto:
				//offs = offsetof(struct X3D_Proto,__children); //addRemoveChildren reallocs p, so mfnode .p field not stable enough for rendering thread
				offs = offsetof(struct X3D_Proto,addChildren); //this is the designed way to add
				break;
			case NODE_Inline:  //Q. do I need this in here? Saw code in x3dparser.
				offs = offsetof(struct X3D_Inline,addChildren); //__children);
				break;
			case NODE_GeoLOD:  //Q. do I need this in here? Saw code in x3dparser.
				offs = offsetof(struct X3D_GeoLOD,rootNode);
				break;
		}
	return offs;
}

/**
 *   parser_do_parse_string: actually calls the parser.
 */
static bool parser_do_parse_string(const char *input, const int len, struct X3D_Node *ectx, struct X3D_Node *nRn)
{
	bool ret;
	int kids;
	ppProdCon p = (ppProdCon)gglobal()->ProdCon.prv;


#if defined (INCLUDE_STL_FILES)
                /* stl files have no implicit scale. This scale will make it fit into a reasonable boundingBox */
                p->lastSTLScaling = 0.1;
#endif


	ret = FALSE;

	inputFileType = determineFileType(input,len);
	DEBUG_MSG("PARSE STRING, ft %d, fv %d.%d.%d\n",
		  inputFileType, inputFileVersion[0], inputFileVersion[1], inputFileVersion[2]);
	kids = offsetofChildren(nRn);

	switch (inputFileType) {
	case IS_TYPE_XML_X3D:
		if(kids){
		//if(nRn->_nodeType == NODE_Group || nRn->_nodeType == NODE_Proto){
			ret = X3DParse(ectx, X3D_NODE(nRn), (const char*)input);
		}
		break;
	case IS_TYPE_VRML:
		if(kids){
			ret = cParse(ectx, nRn, kids, (const char*)input);
			p->haveParsedCParsed = TRUE;
		}
		break;
	case IS_TYPE_VRML1: {
        char *newData = STRDUP("#VRML V2.0 utf8\n\
        Shape {appearance Appearance {material Material {diffuseColor 0.0 1.0 1.0}}\
        geometry Text {\
            string [\"This build\" \"is not made with\" \"VRML1 support\"]\
            fontStyle FontStyle{\
                justify [\"MIDDLE\",\"MIDDLE\"]\
                size 0.5\
            }\
        }}\
        ");
		if(!usingBrotos()){
			ret = cParse (ectx,nRn,(int) offsetof (struct X3D_Group, children), newData);
			FREE_IF_NZ(newData);
		}

		//ret = cParse (nRn,(int) offsetof (struct X3D_Proto, children), newData);
		FREE_IF_NZ(newData);
		break;
    }

#if defined (INCLUDE_NON_WEB3D_FORMATS)
	case IS_TYPE_COLLADA:
		ConsoleMessage ("Collada not supported yet");
		ret = ColladaParse (nRn, (const char*)input);
		break;
	case IS_TYPE_SKETCHUP:
		ConsoleMessage ("Google Sketchup format not supported yet");
		break;
	case IS_TYPE_KML:
		ConsoleMessage ("KML-KMZ  format not supported yet");
		break;
#endif //INCLUDE_NON_WEB3D_FORMATS

#if defined (INCLUDE_STL_FILES)
        case IS_TYPE_ASCII_STL: {
            char *newData = convertAsciiSTL(input);
            p->lastSTLScaling = getLastSTLScale();

            //ConsoleMessage("IS_TYPE_ASCII_STL, now file is :%s:",newData);

            ret = cParse (nRn,(int) offsetof (struct X3D_Group, children), newData);
            FREE_IF_NZ(newData);
            break;
        }
        case IS_TYPE_BINARY_STL: {
            char *newData = convertBinarySTL(input);
            p->lastSTLScaling = getLastSTLScale();

            ret = cParse (nRn,(int) offsetof (struct X3D_Group, children), newData);
            FREE_IF_NZ(newData);
            break;
        }
#endif //INCLUDE_STL_FILES

	default: {
		if (gglobal()->internalc.global_strictParsing) {
			ConsoleMessage("unknown text as input");
		} else {
			inputFileType = IS_TYPE_VRML;
			inputFileVersion[0] = 2; /* try VRML V2 */
			cParse (ectx,nRn,(int) offsetof (struct X3D_Proto, __children), (const char*)input);
			p->haveParsedCParsed = TRUE; }
	}
	}


	if (!ret) {
		ConsoleMessage ("Parser Unsuccessful");
	}

	return ret;
}

/* interface for telling the parser side to forget about everything...  */
void EAI_killBindables (void) {
	int complete;
	ttglobal tg = gglobal();
	ppProdCon p = (ppProdCon)tg->ProdCon.prv;

	complete=0;
	p->psp.comp = &complete;
	p->psp.type = ZEROBINDABLES;
	p->psp.ofs = 0;
	p->psp.ptr = NULL;
	p->psp.path = NULL;
	p->psp.zeroBind = FALSE;
	p->psp.bind = FALSE; /* should we issue a set_bind? */
	p->psp.inp = NULL;
	p->psp.fieldname = NULL;

}

void new_root(){
	//clean up before loading a new scene
	int i;
	//ConsoleMessage ("SHOULD CALL KILL_OLDWORLD HERE\n");

	//struct VRMLParser *globalParser = (struct VRMLParser *)gglobal()->CParse.globalParser;

	/* get rid of sensor events */
	resetSensorEvents();


	/* close the Console Message system, if required. */
	closeConsoleMessage();

	/* occlusion testing - zero total count, but keep MALLOC'd memory around */
	zeroOcclusion();

	/* clock events - stop them from ticking */
	kill_clockEvents();


	/* kill DEFS, handles */
	EAI_killBindables();
	kill_bindables();
	killKeySensorNodeList();


	/* stop routing */
	kill_routing();

	/* tell the statusbar that it needs to reinitialize */
	//kill_status();
	setMenuStatus(NULL);

	/* free textures */
/*
	kill_openGLTextures();
*/

	/* free scripts */
	#ifdef HAVE_JAVASCRIPT
	kill_javascript();
	#endif


#ifdef DO_NOT_KNOW
	/* free EAI */
	if (kill_EAI) {
	       	/* shutdown_EAI(); */
		fwlio_RxTx_control(CHANNEL_EAI, RxTx_STOP) ;
	}
#endif

	/* reset any VRML Parser data */
/*
	if (globalParser != NULL) {
		parser_destroyData(globalParser);
		//globalParser = NULL;
		gglobal()->CParse.globalParser = NULL;
	}
*/
	kill_X3DDefs();

	/* tell statusbar that we have none */
	viewer_default();
	setMenuStatus("NONE");

	//ConsoleMessage ("new_root, right now rootNode has %d children\n",rootNode()->children.n);

	//ConsoleMessage("send_resource_to_parser, new_root\n");
    	/* mark all rootNode children for Dispose */
	{
		struct Multi_Node *children;
		//if(usingBrotos()>1) children = &X3D_PROTO(rootNode())->_children;
		//else children = &X3D_GROUP(rootNode())->children;
		children = childrenField(rootNode());
		for (i = 0; i < children->n; i++) {
			markForDispose(children->p[i], TRUE);
		}

		// force rootNode to have 0 children, compile_Group will make
		// the _sortedChildren field mimic the children field.
		children->n = 0;
		rootNode()->_change++;
	}
	// set the extents back to initial
	{
		struct X3D_Node *node = rootNode();
		INITIALIZE_EXTENT
		;
	}

	//printf ("send_resource_to_parser, rootnode children count set to 0\n");

}
void resitem_enqueue(s_list_t* item);

//void send_resource_to_parser(resource_item_t *res)
//{
//	ppProdCon p;
//	ttglobal tg;
//
//    //ConsoleMessage ("send_resource_to_parser, res->new_root %s",BOOL_STR(res->new_root));
//
//	if (res->new_root) {
//		new_root();
//	}
//
//
//	/* We are not in parser thread, most likely
//	   in main or display thread, and we successfully
//	   parsed a resource request.
//
//	   We send it to parser.
//	*/
//	tg = gglobal();
//	p = tg->ProdCon.prv;
//
//	/* Wait for display thread to be fully initialized */
//	while (IS_DISPLAY_INITIALIZED == FALSE) {
//		usleep(50);
//	}
//
//	///* wait for the parser thread to come up to speed */
//	//while (!p->inputParseInitialized)
//	//	usleep(50);
//
//	resitem_enqueue(ml_new(res));
//}



//bool send_resource_to_parser_if_available(resource_item_t *res)
//{
//	/* We are not in parser thread, most likely
//	   in main or display thread, and we successfully
//	   parsed a resource request.
//
//	   We send it to parser.
//	*/
//	ppProdCon p;
//	ttglobal tg = gglobal();
//	p = (ppProdCon)tg->ProdCon.prv;
//
//	/* Wait for display thread to be fully initialized */
//	/* dug9 Aug 24, 2013 - don't wait (it seems to hang apartment-threaded apps) and see what happens.
//		display_initialized flag is set in a worker thread.
//		H: perhaps the usleep and pthread_create compete in an apartment thread, causing deadlock
//	*/
//	//while (IS_DISPLAY_INITIALIZED == FALSE) {
//	//	usleep(50);
//	//}
//
//	/* wait for the parser thread to come up to speed */
//	//while (!p->inputParseInitialized) usleep(50);
//
//	resitem_enqueue(ml_new(res));
//    return TRUE;
//}

void dump_resource_waiting(resource_item_t* res)
{
#ifdef FW_DEBUG
	printf("%s\t%s\n",( res->complete ? "<finished>" : "<waiting>" ), res->URLrequest);
#endif
}

void send_resource_to_parser_async(resource_item_t *res){

	resitem_enqueue(ml_new(res));
}


void dump_parser_wait_queue()
{
#ifdef FW_DEBUG
	ppProdCon p;
	struct tProdCon *t = &gglobal()->ProdCon;
	p = (ppProdCon)t->prv;
	printf("Parser wait queue:\n");
	ml_foreach(p->resource_list_to_parse, dump_resource_waiting((resource_item_t*)ml_elem(__l)));
	printf(".\n");
#endif
}

/**
 *   parser_process_res_VRML_X3D: this is the final parser (loader) stage, then call the real parser.
 */
bool parser_process_res_VRML_X3D(resource_item_t *res)
{
	//s_list_t *l;
	openned_file_t *of;
	struct X3D_Node *nRn;
	struct X3D_Node *ectx;
	struct X3D_Node *insert_node;
	int i;
	int offsetInNode;
	int shouldBind, shouldUnBind;
    int parsedOk = FALSE; // results from parser
    bool fromEAI_SAI = FALSE;
	/* we only bind to new nodes, if we are adding via Inlines, etc */
	int origFogNodes, origBackgroundNodes, origNavigationNodes, origViewpointNodes;
	ppProdCon p;
	struct tProdCon *t;
	ttglobal tg = gglobal();
	t = &tg->ProdCon;
	p = (ppProdCon)t->prv;

	UNUSED(parsedOk); // compiler warning mitigation

    //printf ("entering parser_process_res_VRML_X3D\n");

	/* printf("processing VRML/X3D resource: %s\n", res->URLrequest);  */
	offsetInNode = 0;
	insert_node = NULL;
	shouldBind = FALSE;
	shouldUnBind = FALSE;
	origFogNodes = vectorSize(p->fogNodes);
	origBackgroundNodes = vectorSize(p->backgroundNodes);
	origNavigationNodes = vectorSize(p->navigationNodes);
	origViewpointNodes = vectorSize(t->viewpointNodes);

    //ConsoleMessage ("parser_process_res_VRML_X3D, url %s",res->parsed_request);
	/* save the current URL so that any local-url gets are relative to this */
    if (res->parsed_request != NULL)
        if (strncmp(res->parsed_request,EAI_Flag,strlen(EAI_Flag)) == 0) {
            //ConsoleMessage("parser_process_res_VRML_X3D, from EAI, ignoring");
            fromEAI_SAI = TRUE;
        }

	if (!fromEAI_SAI)
		pushInputResource(res);

	ectx = res->ectx;
	/* OK Boyz - here we go... if this if from the EAI, just parse it, as it will be a simple string */
	if (res->parsed_request != NULL && strcmp(res->parsed_request, EAI_Flag) == 0) {

		/* EAI/SAI parsing */
		/* printf ("have the actual text here \n"); */
		/* create a container so that the parser has a place to put the nodes */
		nRn = (struct X3D_Node *) createNewX3DNode(NODE_Group);

		insert_node = X3D_NODE(res->whereToPlaceData); /* casting here for compiler */
		offsetInNode = res->offsetFromWhereToPlaceData;

		parsedOk = parser_do_parse_string((const char *)res->URLrequest,(const int)strlen(res->URLrequest), ectx, nRn);
		//printf("after parse_string in EAI/SAI parsing\n");
	} else {
		/* standard file parsing */
		//l = (s_list_t *) res->openned_files;
		//if (!l) {
		//	/* error */
		//	return FALSE;
		//}

		//of = ml_elem(l);
		of = res->openned_files;
		if (!of) {
			/* error */
			return FALSE;
		}


		if (!of->fileData) {
			/* error */
			return FALSE;
		}

		/*
		printf ("res %p root_res %p\n",res,gglobal()->resources.root_res);
		ConsoleMessage ("pc - res %p root_res %p\n",res,gglobal()->resources.root_res);
		*/

		/* bind ONLY in main - do not bind for Inlines, etc */
		if (res->treat_as_root || res == gglobal()->resources.root_res) {
			kill_bindables();
			//kill_oldWorld(TRUE, TRUE, TRUE, __FILE__, __LINE__);
			shouldBind = TRUE;
			shouldUnBind = TRUE;
			origFogNodes = origBackgroundNodes = origNavigationNodes = origViewpointNodes = 0;
			//ConsoleMessage ("pc - shouldBind");
		} else {
			if (!tg->resources.root_res->complete) {
				/* Push the parser state : re-entrance here */
				/* "save" the old classic parser state, so that names do not cross-pollute */
				t->savedParser = (void *)tg->CParse.globalParser;
				tg->CParse.globalParser = NULL;
			}
		}

		/* create a container so that the parser has a place to put the nodes */
		if(usingBrotos()){
			if(res->whereToPlaceData){
				nRn = X3D_NODE(res->whereToPlaceData);
				//if(nRn->_nodeType == NODE_Inline){
					shouldBind = TRUE; 
					shouldUnBind = FALSE; //brotos > Inlines > additively bind (not sure about other things like externProto 17.wrl)
				//}
			}else{
				// we do a kind of hot-swap: we parse into a new broto,
				// then delete the old rootnode broto, then register the new one
				// assumes uload_broto(old root node) has already been done elsewhere
				struct X3D_Proto *sceneProto;
				struct X3D_Node *rn;
				sceneProto = (struct X3D_Proto *) createNewX3DNode(NODE_Proto);
				sceneProto->__protoFlags = ciflag_set(sceneProto->__protoFlags,1,0);
				//if(usingBrotos() > 1){
					//((char *)(&sceneProto->__protoFlags))[2] = 2; // 2=scene type object, render all children
					sceneProto->__protoFlags = ciflag_set(sceneProto->__protoFlags,2,2);
				//}
				nRn = X3D_NODE(sceneProto);
				ectx = nRn;
				rn = rootNode(); //save a pointer to old rootnode
				setRootNode(X3D_NODE(sceneProto)); //set new rootnode
				if(rn){
					//old root node cleanup
					deleteVector(sizeof(void*),rn->_parentVector); //perhaps unlink first
					freeMallocedNodeFields(rn);
					unRegisterX3DNode(rn);
					FREE_IF_NZ(rn);
				}
			}
		}else{
			nRn = (struct X3D_Node *) createNewX3DNode(NODE_Group);
			ectx = nRn; //or should it be null
		}

		/* ACTUALLY CALLS THE PARSER */
		parsedOk = parser_do_parse_string(of->fileData, of->fileDataSize, ectx, nRn);
		//printf("after parse_string in standard file parsing\n");

		if ((res != tg->resources.root_res) && ((!tg->resources.root_res) ||(!tg->resources.root_res->complete))) {
			tg->CParse.globalParser = t->savedParser;
		}

		if (shouldBind) {
			if(shouldUnBind){
				if (vectorSize(p->fogNodes) > 0) {
					for (i=origFogNodes; i < vectorSize(p->fogNodes); ++i)
						send_bind_to(vector_get(struct X3D_Node*,p->fogNodes,i), 0);
					/* Initialize binding info */
					t->setFogBindInRender = vector_get(struct X3D_Node*, p->fogNodes,0);
				}
				if (vectorSize(p->backgroundNodes) > 0) {
					for (i=origBackgroundNodes; i < vectorSize(p->backgroundNodes); ++i)
						send_bind_to(vector_get(struct X3D_Node*,p->backgroundNodes,i), 0);
					/* Initialize binding info */
					t->setBackgroundBindInRender = vector_get(struct X3D_Node*, p->backgroundNodes,0);
				}
				if (vectorSize(p->navigationNodes) > 0) {
					for (i=origNavigationNodes; i < vectorSize(p->navigationNodes); ++i)
						send_bind_to(vector_get(struct X3D_Node*,p->navigationNodes,i), 0);
					/* Initialize binding info */
					t->setNavigationBindInRender = vector_get(struct X3D_Node*, p->navigationNodes,0);
				}
				if (vectorSize(t->viewpointNodes) > 0) {
					for (i = origViewpointNodes; i < vectorSize(t->viewpointNodes); ++i)
						send_bind_to(vector_get(struct X3D_Node*, t->viewpointNodes, i), 0);

					/* Initialize binding info */
					t->setViewpointBindInRender = vector_get(struct X3D_Node*, t->viewpointNodes,0);
					if (res->afterPoundCharacters)
						fwl_gotoViewpoint(res->afterPoundCharacters);
				}
			}else{
				// for broto inlines, we want to add to what's in the main scene, and bind to the last item if its new
				if (vectorSize(p->fogNodes) > origFogNodes) {
					t->setFogBindInRender = vector_get(struct X3D_Node*, p->fogNodes,origFogNodes);
				}
				if (vectorSize(p->backgroundNodes) > origBackgroundNodes) {
					t->setBackgroundBindInRender = vector_get(struct X3D_Node*, p->backgroundNodes,origBackgroundNodes);
				}
				if (vectorSize(p->navigationNodes) > origNavigationNodes) {
					t->setNavigationBindInRender = vector_get(struct X3D_Node*, p->navigationNodes,origNavigationNodes);
				}
				if (vectorSize(t->viewpointNodes) > origViewpointNodes) {
					t->setViewpointBindInRender = vector_get(struct X3D_Node*, t->viewpointNodes,origViewpointNodes); 
					if (res->afterPoundCharacters)
						fwl_gotoViewpoint(res->afterPoundCharacters);
				}

			}
		}

		/* we either put things at the rootNode (ie, a new world) or we put them as a children to another node */
		if (res->whereToPlaceData == NULL) {
			if(!usingBrotos()){
				ASSERT(rootNode());
				insert_node = rootNode();
				offsetInNode = (int) offsetof(struct X3D_Group, children);
			}else{
				//brotos have to maintain various lists, which is done in the parser. 
				//therefore pass the rootnode / executioncontext into the parser
			}
		} else {
			insert_node = X3D_NODE(res->whereToPlaceData); /* casting here for compiler */
			offsetInNode = res->offsetFromWhereToPlaceData;
		}
	}

	/* printf ("parser_process_res_VRML_X3D, res->where %u, insert_node %u, rootNode %u\n",res->where, insert_node, rootNode); */

	/* now that we have the VRML/X3D file, load it into the scene. */
	/* add the new nodes to wherever the caller wanted */

	/* take the nodes from the nRn node, and put them into the place where we have decided to put them */
	//if(!usingBrotos() ){
	if(X3D_NODE(nRn)->_nodeType == NODE_Group){
		struct X3D_Group *nRng = X3D_GROUP(nRn);
		AddRemoveChildren(X3D_NODE(insert_node),
				  offsetPointer_deref(void*, insert_node, offsetInNode),
				  (struct X3D_Node * *)nRng->children.p,
				  nRng->children.n, 1, __FILE__,__LINE__);

		/* and, remove them from this nRn node, so that they are not multi-parented */
		AddRemoveChildren(X3D_NODE(nRng),
				  (struct Multi_Node *)((char *)nRng + offsetof (struct X3D_Group, children)),
				  (struct X3D_Node* *)nRng->children.p,nRng->children.n,2,__FILE__,__LINE__);
	}
	res->complete = TRUE;


	/* remove this resource from the stack */
	if (!fromEAI_SAI)
		popInputResource();

	//printf ("exiting praser_process_res_VRML_X3D\n");

	return TRUE;
}

/* interface for creating VRML for EAI */
int EAI_CreateVrml(const char *tp, const char *inputstring,
		struct X3D_Node *ectx, struct X3D_Group *where) {
	resource_item_t *res;
	char *newString;
	bool retval = FALSE;

	newString = NULL;

	if (strncmp(tp, "URL", 3) == 0) {

		res = resource_create_single(inputstring);
		res->ectx = ectx;
		res->whereToPlaceData = where;
		res->offsetFromWhereToPlaceData = (int) offsetof (struct X3D_Group, children);
		/* printf ("EAI_CreateVrml, res->where is %u, root is %u parameter where %u\n",res->where, rootNode, where); */

	} else { // all other cases are inline code to parse... let the parser do the job ;P...

		const char *sendIn;

		if (strncmp(inputstring,"#VRML V2.0", 6) == 0) {
			sendIn = inputstring;
		} else {
			newString = MALLOC (char *, strlen(inputstring) + strlen ("#VRML V2.0 utf8\n") + 3);
			strcpy (newString,"#VRML V2.0 utf8\n");
			strcat (newString,inputstring);
			sendIn = newString;
			/* printf ("EAI_Create, had to append, now :%s:\n",newString); */
		}

		res = resource_create_from_string(sendIn);
		res->media_type=resm_vrml;
		res->parsed_request = EAI_Flag;
		res->ectx = ectx;
		res->whereToPlaceData = where;
		res->offsetFromWhereToPlaceData = (int) offsetof (struct X3D_Group, children);
	}
	retval = parser_process_res_VRML_X3D(res);
	FREE_IF_NZ(newString);
	return retval;
	//send_resource_to_parser(res);
	//resource_wait(res);
	//FREE_IF_NZ(newString);
	//return (res->status == ress_parsed);
}

/* interface for creating X3D for EAI - like above except x3d */
int EAI_CreateX3d(const char *tp, const char *inputstring, struct X3D_Node *ectx, struct X3D_Group *where)
{
	//int retval;
	resource_item_t *res;
	char *newString;

	newString = NULL;

	if (strncmp(tp, "URL", 3) == 0) {

		res = resource_create_single(inputstring);
		res->ectx = ectx;
		res->whereToPlaceData = where;
		res->offsetFromWhereToPlaceData = (int) offsetof (struct X3D_Group, children);
		/* printf ("EAI_CreateVrml, res->where is %u, root is %u parameter where %u\n",res->where, rootNode, where); */

	} else { // all other cases are inline code to parse... let the parser do the job ;P...

		const char *sendIn;
		// the x3dparser doesn't like multiple root xml elements
		// and it doesn't seem to hurt to give it an extra wrapping in <x3d>
		// that way you can have multiple root elements and they all get
	    // put into the target children[] field
		newString = MALLOC (char *, strlen(inputstring) + strlen ("<X3D>\n\n</X3D>\n") + 3);
		strcpy(newString,"<X3D>\n");
		strcat(newString,inputstring);
		strcat(newString,"\n</X3D>\n");
		sendIn = newString;
		//printf("EAI_createX3d string[%s]\n",sendIn);
		res = resource_create_from_string(sendIn);
		res->media_type=resm_x3d; //**different than vrml
		res->parsed_request = EAI_Flag;
		res->ectx = ectx;
		res->whereToPlaceData = where;
		res->offsetFromWhereToPlaceData = (int) offsetof (struct X3D_Group, children);
	}
	return parser_process_res_VRML_X3D(res);

	//send_resource_to_parser(res);
	//resource_wait(res);
	//FREE_IF_NZ(newString);
	//return (res->status == ress_parsed);
}


/**
 *   parser_process_res_SHADER: this is the final parser (loader) stage, then call the real parser.
 */
static bool parser_process_res_SCRIPT(resource_item_t *res)
{
	//s_list_t *l;
	openned_file_t *of;
	struct Shader_Script* ss;
	const char *buffer;

	buffer = NULL;

	switch (res->type) {
	case rest_invalid:
		return FALSE;
		break;

	case rest_string:
		buffer = res->URLrequest;
		break;
	case rest_url:
	case rest_file:
	case rest_multi:
		//l = (s_list_t *) res->openned_files;
		//if (!l) {
		//	/* error */
		//	return FALSE;
		//}

		//of = ml_elem(l);
		of = res->openned_files;
		if (!of) {
			/* error */
			return FALSE;
		}

		buffer = (const char*)of->fileData;
		break;
	}

	ss = (struct Shader_Script *) res->whereToPlaceData;

	return script_initCode(ss, buffer);
}


#if !defined(HAVE_PTHREAD_CANCEL)
void Parser_thread_exit_handler(int sig) {
	ConsoleMessage("Parser_thread_exit_handler: parserThread exiting");
	pthread_exit(0);
}
#endif //HAVE_PTHREAD_CANCEL


/**
 *   _inputParseThread: parser (loader) thread.
 */


/*
	QUEUE method uses DesignPatterns: CommandPattern + ThreadsafeQueue + SingleThread_ThreadPool/MonoThreading
	It doesn't block the queue while processing/doing_work. That allows the involked
	commands to chain new commands into the queue without deadlocking.

*/
//recently added list functions:
//void ml_enqueue(s_list_t **list, s_list_t *item);
//s_list_t *ml_dequeue(s_list_t **list);


void *getProdConQueueContentStatus() {

	/*void resitem_enqueue(s_list_t *item){ */
	ppProdCon p;
	ttglobal tg = gglobal();
	p = (ppProdCon) tg->ProdCon.prv;

	return (p->resource_list_to_parse);
}


void threadsafe_enqueue_item_signal(s_list_t *item, s_list_t** queue, pthread_mutex_t* queue_lock, pthread_cond_t *queue_nonzero)
{
	pthread_mutex_lock(queue_lock);
	if (*queue == NULL)
		pthread_cond_signal(queue_nonzero);
	ml_enqueue(queue,item);
	pthread_mutex_unlock(queue_lock);
}

s_list_t* threadsafe_dequeue_item_wait(s_list_t** queue, pthread_mutex_t *queue_lock, pthread_cond_t *queue_nonzero, int *waiting )
{
	s_list_t *item = NULL;
	pthread_mutex_lock(queue_lock);
	while (*queue == NULL){
		*waiting = TRUE;
		pthread_cond_wait(queue_nonzero, queue_lock);
		*waiting = FALSE;
	}
	item = ml_dequeue(queue);
	pthread_mutex_unlock(queue_lock);
	return item;
}
void resitem_enqueue(s_list_t *item){
	ppProdCon p;
	ttglobal tg = gglobal();
	p = (ppProdCon)tg->ProdCon.prv;

	threadsafe_enqueue_item_signal(item,&p->resource_list_to_parse, &tg->threads.mutex_resource_list, &tg->threads.resource_list_condition );
}
void resitem_enqueue_tg(s_list_t *item, void* tg){
	fwl_setCurrentHandle(tg, __FILE__, __LINE__);
	resitem_enqueue(item);
	fwl_clearCurrentHandle();
}
s_list_t *resitem_dequeue(){
	ppProdCon p;
	ttglobal tg = gglobal();
	p = (ppProdCon)tg->ProdCon.prv;

	return threadsafe_dequeue_item_wait(&p->resource_list_to_parse, &tg->threads.mutex_resource_list, &tg->threads.resource_list_condition, &tg->threads.ResourceThreadWaiting );
}



void threadsafe_enqueue_item(s_list_t *item, s_list_t** queue, pthread_mutex_t* queue_lock)
{
	pthread_mutex_lock(queue_lock);
	ml_enqueue(queue,item);
	pthread_mutex_unlock(queue_lock);
}

s_list_t* threadsafe_dequeue_item(s_list_t** queue, pthread_mutex_t *queue_lock )
{
	s_list_t *item = NULL;
	pthread_mutex_lock(queue_lock);
	item = ml_dequeue(queue);
	pthread_mutex_unlock(queue_lock);
	return item;
}

void frontenditem_enqueue(s_list_t *item){
	ppProdCon p;
	ttglobal tg = gglobal();
	p = (ppProdCon)tg->ProdCon.prv;

	threadsafe_enqueue_item(item,&p->frontend_list_to_get, &tg->threads.mutex_frontend_list );
}
void frontenditem_enqueue_tg(s_list_t *item, void *tg){
	fwl_setCurrentHandle(tg, __FILE__, __LINE__);
	frontenditem_enqueue(item);
	fwl_clearCurrentHandle();
}
s_list_t *frontenditem_dequeue(){
	ppProdCon p;
	ttglobal tg = gglobal();
	p = (ppProdCon)tg->ProdCon.prv;

	return threadsafe_dequeue_item(&p->frontend_list_to_get, &tg->threads.mutex_frontend_list );
}
s_list_t *frontenditem_dequeue_tg(void *tg){
	s_list_t *item;
	fwl_setCurrentHandle(tg, __FILE__, __LINE__);
	item = frontenditem_dequeue();
	fwl_clearCurrentHandle();
	return item;
}
void *fwl_frontenditem_dequeue(){
	void *res = NULL;
	s_list_t *item = frontenditem_dequeue();
	if (item){
		res = item->elem;
		FREE(item);
	}
	return res;
}
void fwl_resitem_enqueue(void *res){
	resitem_enqueue(ml_new(res));
}

int frontendGetsFiles(){
	return ((ppProdCon)(gglobal()->ProdCon.prv))->frontend_gets_files;
}

void process_res_texitem(resource_item_t *res);
bool parser_process_res_SHADER(resource_item_t *res);
bool process_res_audio(resource_item_t *res);
/**
 *   parser_process_res: for each resource state, advance the process of loading.
 *   this version assumes the item has been dequeued for processing,
 *   and if it needs another step in the processing it will enqueue it in here
 */
static bool parser_process_res(s_list_t *item)
{
	bool remove_it = FALSE;
	bool destroy_it = FALSE;
	bool retval = TRUE;
	resource_item_t *res;
	//ppProdCon p;
	//ttglobal tg = gglobal();
	//p = (ppProdCon)tg->ProdCon.prv;

	if (!item || !item->elem)
		return retval;

	res = ml_elem(item);

	//printf("\nprocessing resource: type %s, status %s\n", resourceTypeToString(res->type), resourceStatusToString(res->status));
	switch (res->status) {

	case ress_invalid:
	case ress_none:
            retval = FALSE;
		if(!res->actions || (res->actions & resa_identify)){
			resource_identify(res->parent, res);
			if (res->type == rest_invalid) {
				remove_it = TRUE;
				res->complete = TRUE; //J30
			}
		}
		break;

	case ress_starts_good:
		if(!res->actions || (res->actions & resa_download)){
#ifndef DISABLER
		//if(p->frontend_gets_files){
			frontenditem_enqueue(ml_new(res));
			remove_it = TRUE;
		//}else{
		//	resource_fetch(res);
		//}
#else
		if(p->frontend_gets_files){
			frontenditem_enqueue(ml_new(res));
			remove_it = TRUE;
		}else{
			resource_fetch(res);
		}
#endif
		}
		break;

	case ress_downloaded:
		if(!res->actions || (res->actions & resa_load))
		/* Here we may want to delegate loading into another thread ... */
		//if (!resource_load(res)) {
		if(res->_loadFunc){
			if(!res->_loadFunc(res)){
				ERROR_MSG("failure when trying to load resource: %s\n", res->URLrequest);
				remove_it = TRUE;
				res->complete = TRUE; //J30
				retval = FALSE;
			}
		}
		break;

	case ress_failed:
		retval = FALSE;
		remove_it = TRUE;
		res->complete = TRUE; //J30
		destroy_it = TRUE;
		break;

	case ress_loaded:
			// printf("processing resource, media_type %s\n",resourceMediaTypeToString(res->media_type));
		if(!res->actions || (res->actions & resa_process))
		switch (res->media_type) {
		case resm_unknown:
			ConsoleMessage ("deciphering file: 404 file not found or unknown file type encountered.");
			remove_it = TRUE;
			res->complete=TRUE; /* not going to do anything else with this one */
			res->status = ress_not_loaded;
			break;
		case resm_vrml:
		case resm_x3d:
			if (parser_process_res_VRML_X3D(res)) {
				DEBUG_MSG("parser successfull: %s\n", res->URLrequest);
				res->status = ress_parsed;

			} else {
				ERROR_MSG("parser failed for resource: %s\n", res->URLrequest);
				retval = FALSE;
			}
			break;
		case resm_script:
			if (parser_process_res_SCRIPT(res)) {
				DEBUG_MSG("parser successfull: %s\n", res->URLrequest);
				res->status = ress_parsed;
			} else {
				retval = FALSE;
				ERROR_MSG("parser failed for resource: %s\n", res->URLrequest);
			}
			break;
		case resm_pshader:
		case resm_fshader:
			if (parser_process_res_SHADER(res)) {
				DEBUG_MSG("parser successfull: %s\n", res->URLrequest);
				res->status = ress_parsed;
			} else {
				retval = FALSE;
				ERROR_MSG("parser failed for resource: %s\n", res->URLrequest);
			}
			break;
		case resm_image:
		case resm_movie:
			/* Texture file has been loaded into memory
				the node could be updated ... i.e. texture created */
			res->complete = TRUE; /* small hack */
			process_res_texitem(res);
			break;
		case resm_audio:
			res->complete = TRUE;
			if(process_res_audio(res)){
				res->status = ress_parsed;
			}else{
				retval = FALSE;
				res->status = ress_failed;
			}
			break;
		case resm_x3z:
			process_x3z(res);
			printf("processed x3z\n");
		}
		/* Parse only once ! */
		res->complete = TRUE; //J30
		remove_it = TRUE;
		break;

	case ress_not_loaded:
		remove_it = TRUE;
		res->complete = TRUE; //J30
		retval = FALSE;
		break;

	case ress_parsed:
		res->complete = TRUE; //J30
		remove_it = TRUE;
		break;

	case ress_not_parsed:
		res->complete = TRUE; //J30
		retval = FALSE;
		remove_it = TRUE;
		break;
	}

	if (remove_it) {
		/* Remove the parsed resource from the list */
		if(res->status == ress_parsed){
			//just x3d and vrml. if you clear images nothing shows up
			if(res->openned_files){
				openned_file_t *of = res->openned_files;
				//remove BLOB
				FREE_IF_NZ(of->fileData);
			}
		}
		FREE_IF_NZ(item);
	}else{
		// chain command by adding it back into the queue
		resitem_enqueue(item);
	}
	dump_parser_wait_queue();

	// printf ("end of process resource\n");

    return retval;
}
//we want the void* addresses of the following, so the int value doesn't matter
static const int res_command_exit;

void resitem_queue_exit(){
	resitem_enqueue(ml_new(&res_command_exit));
}
void _inputParseThread(void *globalcontext)
{
	ttglobal tg = (ttglobal)globalcontext;

        #if !defined (HAVE_PTHREAD_CANCEL)
        struct sigaction actions;
        int rc;
        memset(&actions, 0, sizeof(actions));
        sigemptyset(&actions.sa_mask);
        actions.sa_flags = 0;
        actions.sa_handler = Parser_thread_exit_handler;
        rc = sigaction(SIGUSR2,&actions,NULL);
	// ConsoleMessage ("for parserThread, have defined exit handler");
        #endif //HAVE_PTHREAD_CANCEL

	{
		ppProdCon p = (ppProdCon)tg->ProdCon.prv;
		tg->threads.PCthread = pthread_self();
		//set_thread2global(tg, tg->threads.PCthread ,"parse thread");
		fwl_setCurrentHandle(tg,__FILE__,__LINE__);

		//p->inputParseInitialized = TRUE;
		tg->threads.ResourceThreadRunning = TRUE;
		ENTER_THREAD("input parser");

		viewer_default();

		/* now, loop here forever, waiting for instructions and obeying them */

		for (;;) {
			void *elem;
       		//bool result = TRUE;
			s_list_t* item = resitem_dequeue();
			elem = ml_elem(item);
			if (elem == &res_command_exit){
				FREE_IF_NZ(item);
				break;
			}
			if (tg->threads.flushing){
				FREE_IF_NZ(item);
				continue;
			}
			p->inputThreadParsing = TRUE;
			//result = 
			parser_process_res(item); //,&p->resource_list_to_parse);
			p->inputThreadParsing = FALSE;
			//#if defined (IPHONE) || defined (_ANDROID)
   //         		if (result) setMenuStatus ("ok"); else setMenuStatus("not ok");
			//#endif
		}

		tg->threads.ResourceThreadRunning = FALSE;


	}
}


#ifdef OLDCODE
OLDCODEstatic void unbind_node(struct X3D_Node* node) {
OLDCODE	switch (node->_nodeType) {
OLDCODE		case NODE_Viewpoint:
OLDCODE			X3D_VIEWPOINT(node)->isBound = 0;
OLDCODE			break;
OLDCODE		case NODE_OrthoViewpoint:
OLDCODE			X3D_ORTHOVIEWPOINT(node)->isBound = 0;
OLDCODE			break;
OLDCODE		case NODE_GeoViewpoint:
OLDCODE			X3D_GEOVIEWPOINT(node)->isBound = 0;
OLDCODE			break;
OLDCODE		case NODE_Background:
OLDCODE			X3D_BACKGROUND(node)->isBound = 0;
OLDCODE			break;
OLDCODE		case NODE_TextureBackground:
OLDCODE			X3D_TEXTUREBACKGROUND(node)->isBound = 0;
OLDCODE			break;
OLDCODE		case NODE_NavigationInfo:
OLDCODE			X3D_NAVIGATIONINFO(node)->isBound = 0;
OLDCODE			break;
OLDCODE		case NODE_Fog:
OLDCODE			X3D_FOG(node)->isBound = 0;
OLDCODE			break;
OLDCODE		default: {
OLDCODE			/* do nothing with this node */
OLDCODE			return;
OLDCODE		}
OLDCODE	}
OLDCODE}
OLDCODE
OLDCODE /* for ReplaceWorld (or, just, on start up) forget about previous bindables */
OLDCODE #define KILL_BINDABLE(zzz) \
OLDCODE     printf("KILL_BINDABLE, stack %p size %d\n",zzz,vectorSize(zzz)); \
OLDCODE 	{ int i; for (i=0; i<vectorSize(zzz); i++) { \
OLDCODE         printf ("KILL_BINDABLE %d of %d\n",i,vectorSize(zzz)); \
OLDCODE 		struct X3D_Node* me = vector_get(struct X3D_Node*,zzz,i); \
OLDCODE 		unbind_node(me); \
OLDCODE 	} \
OLDCODE 	deleteVector(struct X3D_Node *,zzz); \
OLDCODE 	zzz = newVector(struct X3D_Node *,8);\
OLDCODE     printf ("KILL_BINDABLE, new stack is %p\n",zzz); \
OLDCODE 	/*causes segfault, do not do this zzz = NULL;*/ \
OLDCODE 	}
#endif //OLDCODE


void kill_bindables (void) {
	ppProdCon p;
    ttglobal tg = gglobal();

	struct tProdCon *t = &gglobal()->ProdCon;
	p = (ppProdCon)t->prv;

    //printf ("kill_bindables called\n");
    t->viewpointNodes->n=0;
    p->backgroundNodes->n=0;
    p->navigationNodes->n=0;
    p->fogNodes->n=0;
    tg->Bindable.navigation_stack->n=0;
    tg->Bindable.background_stack->n=0;
    tg->Bindable.viewpoint_stack->n=0;
    tg->Bindable.fog_stack->n=0;
    return;

	/*
    printf ("before tvp %p ",t->viewpointNodes);
	KILL_BINDABLE(t->viewpointNodes);
    printf ("after, tvp %p\n",t->viewpointNodes);

	KILL_BINDABLE(p->backgroundNodes);
    printf ("calling KILL_BINDABLE on navigationNodes %p at B\n",p->navigationNodes);

	KILL_BINDABLE(p->navigationNodes);
	KILL_BINDABLE(p->fogNodes);

    printf ("calling KILL_BINDABLE on the global navigation stack\n");
    KILL_BINDABLE(tg->Bindable.navigation_stack);
    KILL_BINDABLE(tg->Bindable.background_stack);
    KILL_BINDABLE(tg->Bindable.viewpoint_stack);
    KILL_BINDABLE(tg->Bindable.fog_stack);
    */
    /*
     struct Vector *background_stack;
     struct Vector *viewpoint_stack;
     struct Vector *navigation_stack;
     struct Vector *fog_stack;

     ttglobal tg = gglobal();
     if (node->set_bind < 100) {
     if (node->set_bind == 1) set_naviinfo(node);
     bind_node (X3D_NODE(node), tg->Bindable.navigation_stack,"Component_Navigation");
*/


}


void registerBindable (struct X3D_Node *node) {
	ppProdCon p;
	struct tProdCon *t = &gglobal()->ProdCon;
	p = (ppProdCon)t->prv;


	switch (node->_nodeType) {
		case NODE_Viewpoint:
			X3D_VIEWPOINT(node)->set_bind = 100;
			X3D_VIEWPOINT(node)->isBound = 0;
			vector_pushBack (struct X3D_Node*,t->viewpointNodes, node);
			break;
		case NODE_OrthoViewpoint:
			X3D_ORTHOVIEWPOINT(node)->set_bind = 100;
			X3D_ORTHOVIEWPOINT(node)->isBound = 0;
			vector_pushBack (struct X3D_Node*,t->viewpointNodes, node);
			break;
		case NODE_GeoViewpoint:
			X3D_GEOVIEWPOINT(node)->set_bind = 100;
			X3D_GEOVIEWPOINT(node)->isBound = 0;
			vector_pushBack (struct X3D_Node*,t->viewpointNodes, node);
			break;
		case NODE_Background:
			X3D_BACKGROUND(node)->set_bind = 100;
			X3D_BACKGROUND(node)->isBound = 0;
			vector_pushBack (struct X3D_Node*,p->backgroundNodes, node);
			break;
		case NODE_TextureBackground:
			X3D_TEXTUREBACKGROUND(node)->set_bind = 100;
			X3D_TEXTUREBACKGROUND(node)->isBound = 0;
			vector_pushBack (struct X3D_Node*,p->backgroundNodes, node);
			break;
		case NODE_NavigationInfo:
			X3D_NAVIGATIONINFO(node)->set_bind = 100;
			X3D_NAVIGATIONINFO(node)->isBound = 0;
			vector_pushBack (struct X3D_Node*,p->navigationNodes, node);
			break;
		case NODE_Fog:
			X3D_FOG(node)->set_bind = 100;
			X3D_FOG(node)->isBound = 0;
			vector_pushBack (struct X3D_Node*,p->fogNodes, node);
			break;
		default: {
			/* do nothing with this node */
			/* printf ("got a registerBind on a node of type %s - ignoring\n",
					stringNodeType(node->_nodeType));
			*/
			return;
		}

	}
}
int removeNodeFromVector(int iaction, struct Vector *v, struct X3D_Node *node){
	//iaction = 0 pack vector
	//iaction = 1 set NULL
	int noisy, iret = FALSE;
	noisy = FALSE;
	if(v && node){
		struct X3D_Node *tn;
		int i, ii, n; //idx, 
		//idx = -1;
		n = vectorSize(v);
		for(i=0;i<n;i++){
			ii = n - i - 1; //reverse walk, so we can remove without losing our loop counter
			tn = vector_get(struct X3D_Node*,v,ii);
			if(tn == node){
				iret++;
				if(iaction == 1){
					vector_set(struct X3D_Node*,v,ii,NULL);
					if(noisy) printf("NULLing %d %p\n",ii,node);
				}else if(iaction == 0){
					if(noisy) printf("REMOVing %d %p\n",ii,node);
					vector_remove_elem(struct X3D_Node*,v,ii);
				}
			}
		}
	}
	if(!iret && noisy){
		int i;
		printf("not found in stack node=%p stack.n=%d:\n",node,vectorSize(v));
		for(i=0;i<vectorSize(v);i++){
			printf(" %p",vector_get(struct X3D_Node*,v,i));
		}
		printf("\n");
	}
	return iret;
}
void unRegisterBindable (struct X3D_Node *node) {
	ppProdCon p;
	struct tProdCon *t = &gglobal()->ProdCon;
	p = (ppProdCon)t->prv;


	switch (node->_nodeType) {
		case NODE_Viewpoint:
			X3D_VIEWPOINT(node)->set_bind = 100;
			X3D_VIEWPOINT(node)->isBound = 0;
			//printf ("unRegisterBindable %p Viewpoint, description :%s:\n",node,X3D_VIEWPOINT(node)->description->strptr);
			send_bind_to(node,0);
			removeNodeFromVector(0, t->viewpointNodes, node);
			break;
		case NODE_OrthoViewpoint:
			X3D_ORTHOVIEWPOINT(node)->set_bind = 100;
			X3D_ORTHOVIEWPOINT(node)->isBound = 0;
			send_bind_to(node,0);
			removeNodeFromVector(0, t->viewpointNodes, node);
			break;
		case NODE_GeoViewpoint:
			X3D_GEOVIEWPOINT(node)->set_bind = 100;
			X3D_GEOVIEWPOINT(node)->isBound = 0;
			send_bind_to(node,0);
			removeNodeFromVector(0, t->viewpointNodes, node);
			break;
		case NODE_Background:
			X3D_BACKGROUND(node)->set_bind = 100;
			X3D_BACKGROUND(node)->isBound = 0;
			send_bind_to(node,0);
			removeNodeFromVector(0, p->backgroundNodes, node);
			break;
		case NODE_TextureBackground:
			X3D_TEXTUREBACKGROUND(node)->set_bind = 100;
			X3D_TEXTUREBACKGROUND(node)->isBound = 0;
			removeNodeFromVector(0, p->backgroundNodes, node);
			break;
		case NODE_NavigationInfo:
			X3D_NAVIGATIONINFO(node)->set_bind = 100;
			X3D_NAVIGATIONINFO(node)->isBound = 0;
			send_bind_to(node,0);
			removeNodeFromVector(0, p->navigationNodes, node);
			break;
		case NODE_Fog:
			X3D_FOG(node)->set_bind = 100;
			X3D_FOG(node)->isBound = 0;
			send_bind_to(node,0);
			removeNodeFromVector(0, p->fogNodes, node);
			break;
		default: {
			/* do nothing with this node */
			/* printf ("got a registerBind on a node of type %s - ignoring\n",
					stringNodeType(node->_nodeType));
			*/
			return;
		}

	}
}
