
/*

  FreeWRL support library.
  OpenGL initialization and functions. Rendering functions.
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


#include "../vrml_parser/Structs.h"
#include "../main/headers.h"
#include "../main/ProdCon.h"
#include "../vrml_parser/CParseGeneral.h"
#include "../scenegraph/Vector.h"
#include "../vrml_parser/CFieldDecls.h"
#include "../vrml_parser/CParseParser.h"
#include "../vrml_parser/CParseLexer.h"
#include "../vrml_parser/CParse.h"
#include "../vrml_parser/CRoutes.h"
#include "../scenegraph/quaternion.h"
#include "../scenegraph/Viewer.h"
#include "../scenegraph/sounds.h"
#include "../scenegraph/LinearAlgebra.h"
#include "../scenegraph/Component_KeyDevice.h"	/* resolving implicit declarations */
#include "../input/EAIHeaders.h"		/* resolving implicit declarations */
#include "../input/InputFunctions.h"
#include "Frustum.h"
#include "../opengl/Material.h"
#include "../scenegraph/Component_Core.h"
#include "../scenegraph/Component_Networking.h"
#include "LoadTextures.h"
#include "OpenGL_Utils.h"
#include "Textures.h"
#include "../scenegraph/RenderFuncs.h"
#include "../scenegraph/Component_Shape.h"
#include <float.h>

#include "../x3d_parser/Bindable.h"

#include "../ui/common.h"

void kill_rendering(void);

//static void killNode_hide_obsolete (int index);

static void mesa_Frustum(GLDOUBLE left, GLDOUBLE right, GLDOUBLE bottom, GLDOUBLE top, GLDOUBLE nearZ, GLDOUBLE farZ, GLDOUBLE *m);

#undef DEBUG_FW_LOADMAT
#ifdef DEBUG_FW_LOADMAT
static void fw_glLoadMatrixd(GLDOUBLE *val,char *, int);
#define FW_GL_LOADMATRIX(aaa) fw_glLoadMatrixd(aaa,__FILE__,__LINE__);
#else
static void fw_glLoadMatrixd(GLDOUBLE *val);
#define FW_GL_LOADMATRIX(aaa) fw_glLoadMatrixd(aaa);
#endif


struct shaderTableEntry {
    unsigned int whichOne;
    s_shader_capabilities_t *myCapabilities;

};

int unload_broto(struct X3D_Proto* node);

static void mesa_Ortho(GLDOUBLE left, GLDOUBLE right, GLDOUBLE bottom, GLDOUBLE top, GLDOUBLE nearZ, GLDOUBLE farZ, GLDOUBLE *m);
static void getShaderCommonInterfaces (s_shader_capabilities_t *me);
static void makeAndCompileShader(struct shaderTableEntry *,bool);

///* is this 24 bit depth? 16? 8?? Assume 24, unless set on opening */
//int displayDepth = 24;
//
////static float cc_red = 0.0f, cc_green = 0.0f, cc_blue = 0.0f, cc_alpha = 1.0f;
//int cc_changed = FALSE;

//static pthread_mutex_t  memtablelock = PTHREAD_MUTEX_INITIALIZER;
#define LOCK_MEMORYTABLE 		pthread_mutex_lock(&p->memtablelock);
#define UNLOCK_MEMORYTABLE		pthread_mutex_unlock(&p->memtablelock);
/*
#define LOCK_MEMORYTABLE 		{printf ("LOCK_MEMORYTABLE at %s:%d\n",__FILE__,__LINE__); pthread_mutex_lock(&memtablelock);}
#define UNLOCK_MEMORYTABLE		{printf ("UNLOCK_MEMORYTABLE at %s:%d\n",__FILE__,__LINE__); pthread_mutex_unlock(&memtablelock);}
*/

/* OpenGL perform matrix state here */
#define MAX_LARGE_MATRIX_STACK 256	/* depth of stacks */
#define MAX_SMALL_MATRIX_STACK 2	/* depth of stacks */
#define MATRIX_SIZE 16		/* 4 x 4 matrix */
typedef GLDOUBLE MATRIX4[MATRIX_SIZE];



typedef struct pOpenGL_Utils{
	// list of all X3D nodes in this system.
	// scene graph is tree-structured. this is a linear list.
	struct Vector *linearNodeTable;
	// how many holes might we have in this table, due to killing nodes, etc?
	int potentialHoleCount;

	float cc_red, cc_green, cc_blue, cc_alpha;
	pthread_mutex_t  memtablelock;// = PTHREAD_MUTEX_INITIALIZER;
	MATRIX4 FW_ModelView[MAX_LARGE_MATRIX_STACK];
	MATRIX4 FW_ProjectionView[MAX_SMALL_MATRIX_STACK];
	MATRIX4 FW_TextureView[MAX_SMALL_MATRIX_STACK];

	int modelviewTOS;// = 0;
	int projectionviewTOS;// = 0;
	int textureviewTOS;// = 0;
	//int pickrayviewTOS;// = 0;

	int whichMode;// = GL_MODELVIEW;
	GLDOUBLE *currentMatrix;// = FW_ModelView[0];
#ifdef GLEW_MX
	GLEWContext glewC;
#endif

    struct Vector *myShaderTable; /* list of all active shaders requested by input */
	int userDefinedShaderCount;	/* if the user actually has a Shader node */
    char *userDefinedFragmentShader[MAX_USER_DEFINED_SHADERS];
    char *userDefinedVertexShader[MAX_USER_DEFINED_SHADERS];

    bool usePhongShaders; /* phong shaders == better rendering, but slower */
	int maxStackUsed;
}* ppOpenGL_Utils;

#ifdef DISABLER
void fwl_glGenQueries(GLsizei n, GLuint* ids)
{
#if defined(IPHONE)
	s_renderer_capabilities_t *rdr_caps;
    ttglobal tg = gglobal();
	rdr_caps = tg->display.rdr_caps;
    if (rdr_caps->have_GL_VERSION_3_0)
    {
        glGenQueries(n, ids);
    }
    else
    {
        glGenQueriesEXT(n, ids);
    }
#else
    glGenQueries(n, ids);
#endif
}
void fwl_glDeleteQueries(GLsizei n, const GLuint* ids)
{
#if defined(IPHONE)
	s_renderer_capabilities_t *rdr_caps;
    ttglobal tg = gglobal();
	rdr_caps = tg->display.rdr_caps;
    if (rdr_caps->have_GL_VERSION_3_0)
    {
        glDeleteQueries(n, ids);
    }
    else
    {
        glDeleteQueriesEXT(n, ids);
    }
#else
    glDeleteQueries(n, ids);
#endif
}
void fwl_glGetQueryObjectuiv(GLuint id, GLenum pname, GLuint* params)
{
#if defined(IPHONE)
	s_renderer_capabilities_t *rdr_caps;
    ttglobal tg = gglobal();
	rdr_caps = tg->display.rdr_caps;
    if (rdr_caps->have_GL_VERSION_3_0)
    {
        glGetQueryObjectuiv(id, pname, params);
    }
    else
    {
        glGetQueryObjectuivEXT(id, pname, params);
    }
#else
    glGetQueryObjectuiv(id, pname, params);
#endif
}
#endif

void *OpenGL_Utils_constructor(){
	void *v = MALLOCV(sizeof(struct pOpenGL_Utils));
	memset(v,0,sizeof(struct pOpenGL_Utils));
	return v;
}

void OpenGL_Utils_init(struct tOpenGL_Utils *t)
{
	//public
	/* is this 24 bit depth? 16? 8?? Assume 24, unless set on opening */
	t->displayDepth = 24;

	//static float cc_red = 0.0f, cc_green = 0.0f, cc_blue = 0.0f, cc_alpha = 1.0f;
	t->cc_changed = FALSE;

	//private
	t->prv = OpenGL_Utils_constructor();
	{
		ppOpenGL_Utils p = (ppOpenGL_Utils)t->prv;
		p->linearNodeTable = NULL;
		p->potentialHoleCount = 0;
		p->cc_red = 0.0f;
		p->cc_green = 0.0f;
		p->cc_blue = 0.0f;
		p->cc_alpha = 1.0f;
		//p->memtablelock = PTHREAD_MUTEX_INITIALIZER;
		pthread_mutex_init(&(p->memtablelock), NULL);
		// LoadIdentity will initialize these

		p->modelviewTOS = 0;
		p->projectionviewTOS = 0;
		p->textureviewTOS = 0;
		//p->pickrayviewTOS = 0;

		p->whichMode = GL_MODELVIEW;
		p->currentMatrix = p->FW_ModelView[0];

        // load identity matricies in here
        loadIdentityMatrix(p->FW_ModelView[0]);
        loadIdentityMatrix(p->FW_ProjectionView[0]);
        loadIdentityMatrix(p->FW_TextureView[0]);


        // create room for some shaders. The order in this table is
        // the order in which they are first referenced.
        p->myShaderTable = newVector(struct shaderTableEntry *, 8);

        // userDefinedShaders - assume 0, unless the user is a geek.
        p->userDefinedShaderCount = 0;

        // usePhongShaders set to false for now. Can be changed
        // during runtime, then re-build shaders.
        p->usePhongShaders = false;
        //ConsoleMessage ("setting usePhongShaders to true"); p->usePhongShaders=true;
		p->maxStackUsed = 0;
	}
}
void OpenGL_Utils_clear(struct tOpenGL_Utils *t)
{
	//public
	//private
	{
		ppOpenGL_Utils p = (ppOpenGL_Utils)t->prv;
		deleteVector(struct X3D_Node*,p->linearNodeTable);
		if(p->myShaderTable){
			int i;
			for(i=0;i<vectorSize(p->myShaderTable);i++){
				struct shaderTableEntry *entry = vector_get(struct shaderTableEntry *,p->myShaderTable,i);
				FREE_IF_NZ(entry->myCapabilities);
				FREE_IF_NZ(entry);
			}
			deleteVector(struct shaderTableEntry *, p->myShaderTable);
		}
	}
}
#ifdef GLEW_MX
GLEWContext * glewGetContext()
{
	ppOpenGL_Utils p = (ppOpenGL_Utils)gglobal()->OpenGL_Utils.prv;
	return &(p->glewC);
}
#endif

GLDOUBLE *getPickrayMatrix(int index)
{
	//feature-AFFINE_GLU_UNPROJECT
	bindablestack *bstack;
	ttglobal tg = gglobal();
	bstack = getActiveBindableStacks(tg);
	return bstack->pickraymatrix[index];
}
void setPickrayMatrix(int index, GLDOUBLE *mat)
{
	//feature-AFFINE_GLU_UNPROJECT
	bindablestack *bstack;
	ttglobal tg = gglobal();
	bstack = getActiveBindableStacks(tg);
	memcpy(bstack->pickraymatrix[index],mat,16*sizeof(GLDOUBLE));
}

// we have a new world, get rid of any old user defined shaders here
void kill_userDefinedShaders() {
	int i;
	ppOpenGL_Utils p;
	ttglobal tg = gglobal();
	p = (ppOpenGL_Utils)tg->OpenGL_Utils.prv;

	//ConsoleMessage ("start kill_userDefinedShaders");
	p->userDefinedShaderCount = 0;


	// free the strings for the shader source, if they exist
	for (i=0; i<MAX_USER_DEFINED_SHADERS; i++) {

		//ConsoleMessage ("udf shader %d source is %p %p",i,p->userDefinedVertexShader[i], p->userDefinedFragmentShader[i]);
		FREE_IF_NZ (p->userDefinedFragmentShader[i]);
		FREE_IF_NZ (p->userDefinedVertexShader[i]);
	}

	for (i=0; i <vectorSize(p->myShaderTable); i++) {
        	struct shaderTableEntry *me = vector_get(struct shaderTableEntry *,p->myShaderTable, i);
		FREE_IF_NZ(me->myCapabilities);
	
		//me->whichOne = 0;
		FREE_IF_NZ(me);
	}

	// set the vector to 0 size. we will keep the Vector around, for the next set
	// of shaders for the next world.
	p->myShaderTable->n = 0;
}


// we allow a certain number of user-defined shaders to (somehow) fit in here.
int getNextFreeUserDefinedShaderSlot() {
    int rv;
    ppOpenGL_Utils p;
	ttglobal tg = gglobal();
	p = (ppOpenGL_Utils)tg->OpenGL_Utils.prv;

    if (p->userDefinedShaderCount == MAX_USER_DEFINED_SHADERS) return -1;

    rv = p->userDefinedShaderCount;
    p->userDefinedShaderCount++;

    return rv;
}

// from a user defined shader, we capture the shader text here.
void sendShaderTextToEngine(int ste, int parts, char ** vertSource, char ** fragSource) {
    char *fs = NULL;
    char *vs = NULL;
    int i;

    ppOpenGL_Utils p;
	ttglobal tg = gglobal();
	p = (ppOpenGL_Utils)tg->OpenGL_Utils.prv;

    // find the non-null for each shader text.
    for (i=0; i<parts; i++) {
        //ConsoleMessage ("for ptr ind %d, :%s: :%s:",i,vertSource[i],fragSource[i]);
        if (vertSource[i] != NULL) vs=vertSource[i];
        if (fragSource[i] != NULL) fs=fragSource[i];
    }
    //ConsoleMessage ("sendShaderTextToEngine, saving in %d",ste);

    p->userDefinedFragmentShader[ste] = fs;
    p->userDefinedVertexShader[ste] = vs;
    //printf ("so for shaderTableEntry %d, we have %d %d\n",ste,strlen(fs),strlen(vs));
}

#if defined (_ANDROID)

/***************************************************************************************/
/*                                                                                     */
/* UI FrontEnd Scenegraph manipulation nodes. Right now, the FreeX3D UI uses these,    */
/* but they are NOT Android specific. Feel free to look and use. JAS.		       */
/*                                                                                     */
/***************************************************************************************/


/* pass in a X3D_Shape pointer, and from that, we go and return a bunch of fields of
   the Shape. If a field is NULL, NULL is returned. If a field is a PROTO, the PROTO
   expansion is returned */

void fwl_decomposeShape(struct X3D_Shape * node,
		struct X3D_FillProperties **fpptr,
		struct X3D_LineProperties **lpptr,
		struct X3D_Material **matptr,
		struct X3D_ImageTexture **texptr,
		struct X3D_TextureTransform **texttptr,
		struct X3D_Node **geomptr) {

	struct X3D_Appearance *app = X3D_APPEARANCE(node->appearance);
	struct X3D_Node * geom = node->geometry;

	// initialize every return value to NULL, possibly filed in later
	*fpptr = NULL; *lpptr = NULL; *matptr = NULL; *texptr = NULL;
	*texttptr = NULL; *geomptr = NULL;

	POSSIBLE_PROTO_EXPANSION(struct X3D_Appearance *,X3D_NODE(app),app);
	POSSIBLE_PROTO_EXPANSION(struct X3D_Node*,geom,geom);

	if (app != NULL) {
		struct X3D_FillProperties *fp = X3D_FILLPROPERTIES(app->fillProperties);
		struct X3D_LineProperties *lp = X3D_LINEPROPERTIES(app->lineProperties);
		struct X3D_Material *mat = X3D_MATERIAL(app->material);
		struct X3D_ImageTexture *tex = X3D_IMAGETEXTURE(app->texture);
		struct X3D_TextureTransform *tt = X3D_TEXTURE_TRANSFORM(app->textureTransform);

		POSSIBLE_PROTO_EXPANSION(struct X3D_FillProperties *,X3D_NODE(fp),*fpptr);
		POSSIBLE_PROTO_EXPANSION(struct X3D_LineProperties *,X3D_NODE(lp),*lpptr);
		POSSIBLE_PROTO_EXPANSION(struct X3D_Material *,X3D_NODE(mat),*matptr);
		POSSIBLE_PROTO_EXPANSION(struct X3D_ImageTexture *,X3D_NODE(tex),*texptr);
		POSSIBLE_PROTO_EXPANSION(struct X3D_TextureTransform *,X3D_NODE(tt),*texttptr);
	}

	if (geom != NULL) {
		POSSIBLE_PROTO_EXPANSION(struct X3D_Node *,geom,*geomptr);
	}
}

/* returns a Shape node list - Shape nodes must have valid, light (lightable?) geometry.
   Expects a pointer to a Vector, will create Vector if required.
   Returns an int with the number of valid nodes found;
   Returns the filled in Vector pointer, with NODE_Shape pointers. */

int fwl_android_get_valid_shapeNodes(struct Vector **shapeNodes) {

	struct Vector *me;
	int tc;
        ppOpenGL_Utils p = (ppOpenGL_Utils)gglobal()->OpenGL_Utils.prv;

	//ConsoleMessage ("fwl_android_get_valid_shapeNodes, passed in vector %p",*shapeNodes);

	// create the new vector, if required
	if (*shapeNodes == NULL) {
		*shapeNodes = newVector (struct X3D_Shape *, 16);
	}

	// are we running yet?
	if (p==NULL) return 0;
	if (p->linearNodeTable==NULL) return 0;

	// ease of use of this vector -
	me = *shapeNodes;
	vectorSize(me) = 0;

	LOCK_MEMORYTABLE
	for (tc=0; tc<vectorSize(p->linearNodeTable); tc++){
		struct X3D_Node *node = vector_get(struct X3D_Node *,p->linearNodeTable, tc);

		// do we have a valid node?
		if (node != NULL) {

			POSSIBLE_PROTO_EXPANSION (struct X3D_Node *,node,node);

			// do we have a shape, that is actually used?
			if ((node->_nodeType == NODE_Shape) && (node->referenceCount>0)) {
				struct X3D_Node *geom = X3D_SHAPE(node)->geometry;

				// does it actually have a lit geometry underneath it?
				if (geom != NULL) {
					POSSIBLE_PROTO_EXPANSION (struct X3D_Node *, geom, geom);

					if ((geom->_nodeType != NODE_IndexedLineSet) &&
						(geom->_nodeType != NODE_LineSet) &&
						(geom->_nodeType != NODE_PointSet)) {

						// yep! return this one!
						vector_pushBack(struct X3D_Node *,me, node);
					}
				}
			}
		}
	}
	UNLOCK_MEMORYTABLE

/*
uncomment to print out the shape nodes found

	for (tc=0; tc<vectorSize(me); tc++) {
		struct X3D_FillProperties *fp;
		struct X3D_LineProperties *lp;
		struct X3D_Material *mat;
		struct X3D_ImageTexture *tex;
		struct X3D_TextureTransform *tt;
		struct X3D_Node *geom;

		struct X3D_Node *node = vector_get(struct X3D_Node *,me, tc);
		ConsoleMessage ("node %d is a %s",tc,stringNodeType(node->_nodeType));
		fwl_decomposeShape(X3D_SHAPE(node),&fp,&lp,&mat,&tex,&tt,&geom);

		ConsoleMessage ("EOL, geometry is %s\n",stringNodeType(geom->_nodeType));
		if (fp == NULL) ConsoleMessage ("fillProperties NULL"); else ConsoleMessage ("fillProperties %s",stringNodeType(fp->_nodeType));
		if (lp == NULL) ConsoleMessage ("lineProperties NULL"); else ConsoleMessage ("lineProperties %s",stringNodeType(lp->_nodeType));
		if (mat == NULL) ConsoleMessage ("material NULL"); else ConsoleMessage ("material %s",stringNodeType(mat->_nodeType));
		if (tex == NULL) ConsoleMessage ("texture NULL"); else ConsoleMessage ("texture %s",stringNodeType(tex->_nodeType));
		if (tt == NULL) ConsoleMessage ("texureTransform NULL"); else ConsoleMessage ("texureTransform %s",stringNodeType(tt->_nodeType));

	}
*/

	return vectorSize(me);
}

/* Zeroes the Shape Node table, for instance, when loading in a new world,
   you want to zero this, otherwise all of the existing node pointers will
   be invalid */

void fwl_android_zero_shapeNodeTable(struct Vector **shapeNodes) {
	if (*shapeNodes == NULL) {
		*shapeNodes = newVector (struct X3D_Shape *, 16);
	}
	//ConsoleMessage ("fwl_android_zero_shapeNodeTable, was %d, should be 0 after this",vectorSize(*shapeNodes));
	vectorSize(*shapeNodes) = 0;
	//ConsoleMessage ("fwl_android_zero_shapeNodeTable, is  %d, should be 0 after this",vectorSize(*shapeNodes));
}



/* returns TRUE if the shape node actually has a fillProperties node,
   and the FillProperties field "_enabled" is TRUE,
   returns FALSE if the node does not exist or does not have a FillProperty,
   or the FillProperties field "_enabled" is FALSE */
int fwl_get_FillPropStatus(struct Vector **shapeNodes, int whichEntry) {
	struct X3D_FillProperties *fp;
	struct X3D_LineProperties *lp;
	struct X3D_Material *mat;
	struct X3D_ImageTexture *tex;
	struct X3D_TextureTransform *tt;
	struct X3D_Node *geom;

	//ConsoleMessage ("fwl_get_FillPropStatus, pointer is %p");
	//ConsoleMessage ("fwl_get_FillPropStatus, vecto size %d",vectorSize(*shapeNodes));

	// If we do not have any node entries, maybe this is a new scene, and we have to get
	// the valid nodes?
	if (vectorSize(*shapeNodes) == 0 ) {
		if (fwl_android_get_valid_shapeNodes(shapeNodes) == 0) return FALSE;
	}

	// if we are here, we really do have at least one Shape node.

	struct X3D_Node *node = vector_get(struct X3D_Node *,*shapeNodes, whichEntry);
	//ConsoleMessage ("node %d is a %s",whichEntry,stringNodeType(node->_nodeType));
	fwl_decomposeShape(X3D_SHAPE(node),&fp,&lp,&mat,&tex,&tt,&geom);
	//ConsoleMessage ("and the fp field is %p",fp);

	if (fp != NULL) {
		return (fp->_enabled);
	}
	return (FALSE);
}

void fwl_set_FillPropStatus (struct Vector **shapeNodes, int whichEntry, int yesNo, float fillScale) {
	struct X3D_FillProperties *fp;
	struct X3D_LineProperties *lp;
	struct X3D_Material *mat;
	struct X3D_ImageTexture *tex;
	struct X3D_TextureTransform *tt;
	struct X3D_Node *geom;
	struct X3D_Appearance *ap;

	// If we do not have any node entries, maybe this is a new scene, and we have to get
	// the valid nodes?
	if (vectorSize(*shapeNodes) == 0 ) {
		if (fwl_android_get_valid_shapeNodes(shapeNodes) == 0) return;
	}

	// if we are here, we really do have at least one Shape node.

	struct X3D_Node *node = vector_get(struct X3D_Node *,*shapeNodes, whichEntry);

	//ConsoleMessage ("node %d is a %s",whichEntry,stringNodeType(node->_nodeType));
	fwl_decomposeShape(X3D_SHAPE(node),&fp,&lp,&mat,&tex,&tt,&geom);

	if (yesNo) {
		struct X3D_FillProperties *fp;

		// does the shape have an Appearance node yet?
		if (X3D_SHAPE(node)->appearance == NULL) {
			struct X3D_Material *mat;
			ap = createNewX3DNode(NODE_Appearance);
			AddRemoveSFNodeFieldChild(node,
				offsetPointer_deref(struct X3D_Node **,node,offsetof (struct X3D_Shape, appearance)),
				X3D_NODE(ap),0,__FILE__,__LINE__);

			mat = createNewX3DNode(NODE_Material);
			AddRemoveSFNodeFieldChild(X3D_NODE(ap),
				offsetPointer_deref(struct X3D_Node **,ap,offsetof (struct X3D_Appearance, material)),
				X3D_NODE(mat),0,__FILE__,__LINE__);

		}

		ap = X3D_APPEARANCE(X3D_SHAPE(node)->appearance);

		// create the node, then "set" it in place. If a node previously existed in the
		// fillProperties field, then it gets removed by AddRemoveChild

		if (ap->fillProperties == NULL) {
			//ConsoleMessage ("fwl_set_FillPropStatus, creating a FillProperties");
			fp = X3D_FILLPROPERTIES(createNewX3DNode(NODE_FillProperties));
			AddRemoveSFNodeFieldChild(X3D_NODE(ap),
				offsetPointer_deref(struct X3D_Node **,X3D_NODE(ap),offsetof (struct X3D_Appearance, fillProperties)),
				X3D_NODE(fp),0,__FILE__,__LINE__);
		} else {
			//ConsoleMessage ("fwl_set_FilPropStatus, just enabling FillProperties _enabled field");
			fp = X3D_FILLPROPERTIES(ap->fillProperties);
			// ok, we have a node here, if it is a FillProperties, set the enabled flag
			if (fp->_nodeType == NODE_FillProperties) fp->_enabled = TRUE;
		}

		// ensure that the FillProperties scale is ok.
		fp->_hatchScale.c[0] = fillScale;
		fp->_hatchScale.c[1] = fillScale;
	} else {
		//ConsoleMessage ("fwl_set_FillPropStatus, removing a FillProperties");
		/* do not bother removing it - keep it around in case we re-enable and want original settings
		AddRemoveSFNodeFieldChild(X3D_NODE(X3D_SHAPE(node)->appearance),
			offsetPointer_deref(struct X3D_Node **,X3D_NODE(X3D_SHAPE(node)->appearance),offsetof (struct X3D_Appearance, fillProperties)),
			X3D_NODE(fp),2,__FILE__,__LINE__);
		*/
		ap = X3D_APPEARANCE(X3D_SHAPE(node)->appearance);
		if (ap != NULL) {
			if (ap->fillProperties != NULL) {
				struct X3D_FillProperties *fp = X3D_FILLPROPERTIES(ap->fillProperties);
				// ok, we have a node here, if it is a FillProperties, set the enabled flag
				if (fp->_nodeType == NODE_FillProperties) fp->_enabled = FALSE;
			}
		}
	}
	// tell the Shape node that we need to check the shaders it uses...
	node->_change ++;
}

/* return whether FillProperties hatched is true/false */
int fwl_get_FillPropHatched(struct Vector **shapeNodes, int whichEntry) {
	struct X3D_FillProperties *fp;
	struct X3D_LineProperties *lp;
	struct X3D_Material *mat;
	struct X3D_ImageTexture *tex;
	struct X3D_TextureTransform *tt;
	struct X3D_Node *geom;

	// Assume that we have a Shape node
	if (vectorSize(*shapeNodes) == 0 ) {
		return FALSE;
	}

	// if we are here, we really do have at least one Shape node.
	struct X3D_Node *node = vector_get(struct X3D_Node *,*shapeNodes, whichEntry);
	fwl_decomposeShape(X3D_SHAPE(node),&fp,&lp,&mat,&tex,&tt,&geom);

	return (fp->hatched);
}

/* set current FillProperties to hatched */
void fwl_set_FillPropHatched (struct Vector **shapeNodes, int whichEntry, int yesNo) {
	struct X3D_FillProperties *fp;
	struct X3D_LineProperties *lp;
	struct X3D_Material *mat;
	struct X3D_ImageTexture *tex;
	struct X3D_TextureTransform *tt;
	struct X3D_Node *geom;
	struct X3D_Appearance *ap;

	// Assume that we have a Shape node
	if (vectorSize(*shapeNodes) == 0 ) {
		return;
	}

	// if we are here, we really do have at least one Shape node.
	struct X3D_Node *node = vector_get(struct X3D_Node *,*shapeNodes, whichEntry);
	fwl_decomposeShape(X3D_SHAPE(node),&fp,&lp,&mat,&tex,&tt,&geom);

	if (fp!=NULL) fp->hatched = yesNo;
}

/* return whether FillProperties filled is true/false */
int fwl_get_FillPropFilled(struct Vector **shapeNodes, int whichEntry) {
	struct X3D_FillProperties *fp;
	struct X3D_LineProperties *lp;
	struct X3D_Material *mat;
	struct X3D_ImageTexture *tex;
	struct X3D_TextureTransform *tt;
	struct X3D_Node *geom;

	// Assume that we have a Shape node
	if (vectorSize(*shapeNodes) == 0 ) {
		return FALSE;
	}

	// if we are here, we really do have at least one Shape node.
	struct X3D_Node *node = vector_get(struct X3D_Node *,*shapeNodes, whichEntry);
	fwl_decomposeShape(X3D_SHAPE(node),&fp,&lp,&mat,&tex,&tt,&geom);

	return (fp->filled);
}

/* set current FillProperties to filled */
void fwl_set_FillPropFilled (struct Vector **shapeNodes, int whichEntry, int yesNo) {
	struct X3D_FillProperties *fp;
	struct X3D_LineProperties *lp;
	struct X3D_Material *mat;
	struct X3D_ImageTexture *tex;
	struct X3D_TextureTransform *tt;
	struct X3D_Node *geom;
	struct X3D_Appearance *ap;

	// Assume that we have a Shape node
	if (vectorSize(*shapeNodes) == 0 ) {
		return;
	}

	// if we are here, we really do have at least one Shape node.
	struct X3D_Node *node = vector_get(struct X3D_Node *,*shapeNodes, whichEntry);
	fwl_decomposeShape(X3D_SHAPE(node),&fp,&lp,&mat,&tex,&tt,&geom);

	if (fp!=NULL) fp->filled = yesNo;
}


/* return FillProperties style */
int fwl_get_FillPropStyle(struct Vector **shapeNodes, int whichEntry) {
	struct X3D_FillProperties *fp;
	struct X3D_LineProperties *lp;
	struct X3D_Material *mat;
	struct X3D_ImageTexture *tex;
	struct X3D_TextureTransform *tt;
	struct X3D_Node *geom;

	// Assume that we have a Shape node
	if (vectorSize(*shapeNodes) == 0 ) {
		return 0;
	}

	// if we are here, we really do have at least one Shape node.
	struct X3D_Node *node = vector_get(struct X3D_Node *,*shapeNodes, whichEntry);
	fwl_decomposeShape(X3D_SHAPE(node),&fp,&lp,&mat,&tex,&tt,&geom);

	if (fp==NULL) return 0;
	return (fp->hatchStyle);
}

/* set current FillProperties hatchStyle */
void fwl_set_FillPropStyle (struct Vector **shapeNodes, int whichEntry, int which) {
	struct X3D_FillProperties *fp;
	struct X3D_LineProperties *lp;
	struct X3D_Material *mat;
	struct X3D_ImageTexture *tex;
	struct X3D_TextureTransform *tt;
	struct X3D_Node *geom;
	struct X3D_Appearance *ap;

	// Assume that we have a Shape node
	if (vectorSize(*shapeNodes) == 0 ) {
		return;
	}

	// if we are here, we really do have at least one Shape node.
	struct X3D_Node *node = vector_get(struct X3D_Node *,*shapeNodes, whichEntry);
	fwl_decomposeShape(X3D_SHAPE(node),&fp,&lp,&mat,&tex,&tt,&geom);

	if (fp!=NULL) fp->hatchStyle = which;
}


/* return FillProperties colour */
int fwl_get_FillPropColour(struct Vector **shapeNodes, int whichEntry) {
	struct X3D_FillProperties *fp;
	struct X3D_LineProperties *lp;
	struct X3D_Material *mat;
	struct X3D_ImageTexture *tex;
	struct X3D_TextureTransform *tt;
	struct X3D_Node *geom;
	int integer_colour;

	// Assume that we have a Shape node
	if (vectorSize(*shapeNodes) == 0 ) {
		return 0;
	}

	// if we are here, we really do have at least one Shape node.
	struct X3D_Node *node = vector_get(struct X3D_Node *,*shapeNodes, whichEntry);
	fwl_decomposeShape(X3D_SHAPE(node),&fp,&lp,&mat,&tex,&tt,&geom);

	#define CLAMP(value, min, max) (((value) >(max)) ? (max) : (((value) <(min)) ? (min) : (value)))
	if (fp==NULL) return 0;

	integer_colour = 0xFF000000 + (
		((uint8_t)(255.0f *CLAMP(fp->hatchColor.c[0], 0.0, 1.0)) <<16) |
               ((uint8_t)(255.0f *CLAMP(fp->hatchColor.c[1], 0.0, 1.0)) <<8) |
               ((uint8_t)(255.0f *CLAMP(fp->hatchColor.c[2], 0.0, 1.0))));

	//ConsoleMessage ("fwl_get_fp, is %x",integer_colour);
	return (integer_colour);
}

/* set current FillProperties hatchColour */
void fwl_set_FillPropColour (struct Vector **shapeNodes, int whichEntry, int argbColour) {
	struct X3D_FillProperties *fp;
	struct X3D_LineProperties *lp;
	struct X3D_Material *mat;
	struct X3D_ImageTexture *tex;
	struct X3D_TextureTransform *tt;
	struct X3D_Node *geom;
	struct X3D_Appearance *ap;

	// Assume that we have a Shape node
	if (vectorSize(*shapeNodes) == 0 ) {
		return;
	}

	// if we are here, we really do have at least one Shape node.
	struct X3D_Node *node = vector_get(struct X3D_Node *,*shapeNodes, whichEntry);
	fwl_decomposeShape(X3D_SHAPE(node),&fp,&lp,&mat,&tex,&tt,&geom);

	if (fp!=NULL) {
		fp->hatchColor.c[0] =CLAMP(((float)((argbColour &0xff0000) >>16)) /255.0, 0.0, 1.0);
		fp->hatchColor.c[1] =CLAMP(((float)((argbColour &0x00ff00) >>8)) /255.0, 0.0, 1.0);
		fp->hatchColor.c[2] =CLAMP(((float)((argbColour &0x0000ff))) /255.0, 0.0, 1.0);
		//ConsoleMessage ("converted colour to %f %f %f ", fp->hatchColor.c[0], fp->hatchColor.c[1], fp->hatchColor.c[2]);
	}
}


// MAterial - we need to ensure that this is node has a Material; we make it a TwoSidedMaterial
// even if it was a normal Material node.
// returns whether two-sided is true or not.
//JNIEXPORT jboolean JNICALL Java_org_freex3d_FreeX3DLib_setMaterialExisting(JNIEnv *env, jobject obj) {
//        return fwl_set_MaterialExisting(&shapeNodes,0);
//}

int fwl_set_MaterialExisting(struct Vector **shapeNodes, int whichEntry) {
	struct X3D_FillProperties *fp;
	struct X3D_LineProperties *lp;
	struct X3D_Material *mat;
	struct X3D_ImageTexture *tex;
	struct X3D_TextureTransform *tt;
	struct X3D_Node *geom;
	struct X3D_Appearance *ap;
	struct X3D_TwoSidedMaterial *tsm;

	int twoSided = FALSE;

	// If we do not have any node entries, maybe this is a new scene, and we have to get
	// the valid nodes?
	if (vectorSize(*shapeNodes) == 0 ) {
		if (fwl_android_get_valid_shapeNodes(shapeNodes) == 0) return FALSE;
	}

	// if we are here, we really do have at least one Shape node.

	struct X3D_Node *node = vector_get(struct X3D_Node *,*shapeNodes, whichEntry);

	//ConsoleMessage ("node %d is a %s",whichEntry,stringNodeType(node->_nodeType));
	fwl_decomposeShape(X3D_SHAPE(node),&fp,&lp,&mat,&tex,&tt,&geom);


	// does the shape have an Appearance node yet?
	if (X3D_SHAPE(node)->appearance == NULL) {
		struct X3D_Material *mat;
		ap = createNewX3DNode(NODE_Appearance);
		AddRemoveSFNodeFieldChild(node,
			offsetPointer_deref(struct X3D_Node **,node,offsetof (struct X3D_Shape, appearance)),
			X3D_NODE(ap),0,__FILE__,__LINE__);

		mat = createNewX3DNode(NODE_TwoSidedMaterial);
		AddRemoveSFNodeFieldChild(X3D_NODE(ap),
			offsetPointer_deref(struct X3D_Node **,ap,offsetof (struct X3D_Appearance, material)),
			X3D_NODE(mat),0,__FILE__,__LINE__);
	}

	ap = X3D_APPEARANCE(X3D_SHAPE(node)->appearance);
	//ConsoleMessage ("so, at this point in time, we have an appearance, type %s",stringNodeType(ap->_nodeType));

	// create the node, then "set" it in place. If a node previously existed in the
	// fillProperties field, then it gets removed by AddRemoveChild

	if (ap->material == NULL) {
		//ConsoleMessage ("fwl_set_MaterialPropStatus, creating a MaterialProperties");
		tsm = X3D_TWOSIDEDMATERIAL(createNewX3DNode(NODE_TwoSidedMaterial));
		AddRemoveSFNodeFieldChild(X3D_NODE(ap),
			offsetPointer_deref(struct X3D_Node **,X3D_NODE(ap),offsetof (struct X3D_Appearance, material)),
			X3D_NODE(tsm),0,__FILE__,__LINE__);
	}

	tsm = X3D_TWOSIDEDMATERIAL(ap->material);
	// ok, we have a node here, if it is a FillProperties, set the enabled flag

	// do we have to change a Material to TwoSidedMaterial??
	if (tsm->_nodeType != NODE_TwoSidedMaterial) {
		struct X3D_Material *mat = X3D_MATERIAL(tsm);
		struct X3D_TwoSidedMaterial *ntsm = createNewX3DNode(NODE_TwoSidedMaterial);
		if (mat->_nodeType == NODE_Material) {
			//copy over the fields
			ntsm->ambientIntensity = mat->ambientIntensity;
			ntsm->backAmbientIntensity = mat->ambientIntensity;
			ntsm->shininess = mat->shininess;
			ntsm->backShininess = mat->shininess;
			ntsm->transparency = mat->transparency;
			ntsm->backTransparency = mat->transparency;
			memcpy((void *)&ntsm->diffuseColor, (void *)&mat->diffuseColor,sizeof(struct SFColor));
			memcpy((void *)&ntsm->backDiffuseColor, (void *)&mat->diffuseColor,sizeof(struct SFColor));
			memcpy((void *)&ntsm->emissiveColor, (void *)&mat->emissiveColor,sizeof(struct SFColor));
			memcpy((void *)&ntsm->backEmissiveColor, (void *)&mat->emissiveColor,sizeof(struct SFColor));
			memcpy((void *)&ntsm->specularColor, (void *)&mat->specularColor,sizeof(struct SFColor));
			memcpy((void *)&ntsm->backSpecularColor, (void *)&mat->specularColor,sizeof(struct SFColor));
			ntsm->separateBackColor=FALSE;
		} else {
			ConsoleMessage ("somehow the Material is not a node of Material type for this node");
		}

		// now, make the child our TwoSidedMaterial node
		AddRemoveSFNodeFieldChild(X3D_NODE(ap),
			offsetPointer_deref(struct X3D_Node **,ap,offsetof (struct X3D_Appearance, material)),
						X3D_NODE(ntsm),0,__FILE__,__LINE__);
	} else {
		// We DO have a TwoSidedMaterial...
		twoSided = X3D_TWOSIDEDMATERIAL(tsm)->separateBackColor;
	}

	// tell the Shape node that we need to check the shaders it uses...
	node->_change ++;

	return twoSided;
}

/* fwl_get_MaterialColourValue(xx) - example usage:
                <item>Front Diffuse</item>
                <item>Front Emissive</item>
                <item>Front Specular</item>
                <item>Back Diffuse</item>
                <item>Back Emissive</item>
                <item>Back Specular</item>

        int frontDiffuse = FreeX3DLib.getMaterialColourValue(0);
        int frontEmissive = FreeX3DLib.getMaterialColourValue(1);
        int frontSpecular = FreeX3DLib.getMaterialColourValue(2);
        int backDiffuse = FreeX3DLib.getMaterialColourValue(3);
        int backEmissive = FreeX3DLib.getMaterialColourValue(4);
        int backSpecular = FreeX3DLib.getMaterialColourValue(5);

*/

int fwl_get_MaterialColourValue(struct Vector **shapeNodes, int whichEntry, int whichValue) {
        struct X3D_FillProperties *fp;
        struct X3D_LineProperties *lp;
        struct X3D_Material *mat;
        struct X3D_ImageTexture *tex;
        struct X3D_TextureTransform *tt;
        struct X3D_Node *geom;
        struct X3D_Appearance *ap;
        struct X3D_TwoSidedMaterial *tsm;

        // If we do not have any node entries, maybe this is a new scene, and we have to get
        // the valid nodes?
        if (vectorSize(*shapeNodes) == 0 ) {
                if (fwl_android_get_valid_shapeNodes(shapeNodes) == 0) return 0;
        }

        // if we are here, we really do have at least one Shape node.

        struct X3D_Node *node = vector_get(struct X3D_Node *,*shapeNodes, whichEntry);

        //ConsoleMessage ("node %d is a %s",whichEntry,stringNodeType(node->_nodeType));
        fwl_decomposeShape(X3D_SHAPE(node),&fp,&lp,&mat,&tex,&tt,&geom);

	if (mat == NULL) return 0;

	if (mat->_nodeType == NODE_TwoSidedMaterial) {
		struct SFColor *col = NULL;
		struct X3D_TwoSidedMaterial *tmat = X3D_TWOSIDEDMATERIAL(mat);
		switch (whichValue) {
			case 0: col = &tmat->diffuseColor; break;
			case 1: col = &tmat->emissiveColor; break;
			case 2: col = &tmat->specularColor; break;
			case 3: col = &tmat->backDiffuseColor; break;
			case 4: col = &tmat->backEmissiveColor; break;
			case 5: col = &tmat->backSpecularColor; break;
			default: {}
		}
		if (col != NULL) {
			int integer_colour;
			integer_colour = 0xFF000000 + (
				((uint8_t)(255.0f *CLAMP(col->c[0], 0.0, 1.0)) <<16) |
               			((uint8_t)(255.0f *CLAMP(col->c[1], 0.0, 1.0)) <<8) |
               			((uint8_t)(255.0f *CLAMP(col->c[2], 0.0, 1.0))));
			//ConsoleMessage ("getMaterialValue, returning colour %d\n",integer_colour);
			return integer_colour;
		}
	} else {
		ConsoleMessage ("getMaterialValue, expected a TwoSidedMaterial, not a %s\n",stringNodeType(mat->_nodeType));
	}
	return 0;
}


void fwl_set_TwoSidedMaterialStatus( struct Vector **shapeNodes, int whichEntry, int oneTwo) {
	struct X3D_FillProperties *fp;
	struct X3D_LineProperties *lp;
	struct X3D_Material *mat;
	struct X3D_ImageTexture *tex;
	struct X3D_TextureTransform *tt;
	struct X3D_Node *geom;
	struct X3D_Appearance *ap;

	// Assume that we have a Shape node
	if (vectorSize(*shapeNodes) == 0 ) {
		return;
	}

	// if we are here, we really do have at least one Shape node.
	struct X3D_Node *node = vector_get(struct X3D_Node *,*shapeNodes, whichEntry);
	fwl_decomposeShape(X3D_SHAPE(node),&fp,&lp,&mat,&tex,&tt,&geom);

	if (mat == NULL) return;
	if (mat->_nodeType == NODE_TwoSidedMaterial) {
		// ok - we are in the correct place.
		X3D_TWOSIDEDMATERIAL(mat)->separateBackColor = oneTwo;
		mat->_change++; // signal that this node has changed
	}
}

/* set current colour */
void fwl_set_MaterialColourValue (struct Vector **shapeNodes, int whichEntry, int whichValue, int argbColour) {
	struct X3D_FillProperties *fp;
	struct X3D_LineProperties *lp;
	struct X3D_Material *mat;
	struct X3D_ImageTexture *tex;
	struct X3D_TextureTransform *tt;
	struct X3D_Node *geom;
	struct X3D_Appearance *ap;

	// Assume that we have a Shape node
	if (vectorSize(*shapeNodes) == 0 ) {
		return;
	}

	// if we are here, we really do have at least one Shape node.
	struct X3D_Node *node = vector_get(struct X3D_Node *,*shapeNodes, whichEntry);
	fwl_decomposeShape(X3D_SHAPE(node),&fp,&lp,&mat,&tex,&tt,&geom);

	if (mat == NULL) return;
	if (mat->_nodeType == NODE_TwoSidedMaterial) {
		struct SFColor *col = NULL;
		struct X3D_TwoSidedMaterial *tmat = X3D_TWOSIDEDMATERIAL(mat);
		switch (whichValue) {
			case 0: col = &tmat->diffuseColor; break;
			case 1: col = &tmat->emissiveColor; break;
			case 2: col = &tmat->specularColor; break;
			case 3: col = &tmat->backDiffuseColor; break;
			case 4: col = &tmat->backEmissiveColor; break;
			case 5: col = &tmat->backSpecularColor; break;
			default: {}
		}
		if (col != NULL) {
			col->c[0] =CLAMP(((float)((argbColour &0xff0000) >>16)) /255.0, 0.0, 1.0);
			col->c[1] =CLAMP(((float)((argbColour &0x00ff00) >>8)) /255.0, 0.0, 1.0);
			col->c[2] =CLAMP(((float)((argbColour &0x0000ff))) /255.0, 0.0, 1.0);
			tmat->_change ++; // signal that this has been updated
		}
	}
}



/* for a SeekBar, get a field, for a side, and return it as a % 0-100
	whichSide - 0->2 front side,
		  - 3-n, back side,
	whichField - 1 - shininess, 2 transparency, 3- Ambient Intensity
*/
int fwl_get_MaterialFloatValue(struct Vector **shapeNodes, int whichEntry, int whichSide, int whichField) {
        struct X3D_FillProperties *fp;
        struct X3D_LineProperties *lp;
        struct X3D_Material *mat;
        struct X3D_ImageTexture *tex;
        struct X3D_TextureTransform *tt;
        struct X3D_Node *geom;
        struct X3D_Appearance *ap;
        struct X3D_TwoSidedMaterial *tsm;

	//ConsoleMessage ("gwl_get_materialFloatValue, entry %d, side %d, value %d",whichEntry, whichSide, whichField);

        // If we do not have any node entries, maybe this is a new scene, and we have to get
        // the valid nodes?
        if (vectorSize(*shapeNodes) == 0 ) {
                if (fwl_android_get_valid_shapeNodes(shapeNodes) == 0) return 0;
        }

        // if we are here, we really do have at least one Shape node.

        struct X3D_Node *node = vector_get(struct X3D_Node *,*shapeNodes, whichEntry);

        //ConsoleMessage ("node %d is a %s",whichEntry,stringNodeType(node->_nodeType));
        fwl_decomposeShape(X3D_SHAPE(node),&fp,&lp,&mat,&tex,&tt,&geom);

	if (mat == NULL) return 0;

	if (mat->_nodeType == NODE_TwoSidedMaterial) {
		float fcol = 0.0;
		struct X3D_TwoSidedMaterial *tmat = X3D_TWOSIDEDMATERIAL(mat);
		if (whichSide <3) {
			switch (whichField) {
				case 1: fcol = tmat->shininess; break;
				case 2: fcol = tmat->transparency; break;
				case 3: fcol = tmat->ambientIntensity; break;
				default: {
					ConsoleMessage ("hmmm - expect 1-3, got %d",whichField);
					return 0;
				}
			}
		} else {
			switch (whichField) {
				case 1: fcol = tmat->backShininess; break;
				case 2: fcol = tmat->backTransparency; break;
				case 3: fcol = tmat->backAmbientIntensity; break;
				default: {
					ConsoleMessage ("hmmm - expect 1-3, got %d",whichField);
					return 0;
				}
			}
		}

		return (int)(fcol*100.0); // fcol;
	} else {
		ConsoleMessage ("getMaterialValue, expected a TwoSidedMaterial, not a %s\n",stringNodeType(mat->_nodeType));
	}

	return 0;
}

void fwl_set_MaterialFloatValue(struct Vector **shapeNodes, int whichEntry, int whichSide, int whichField, int nv) {
        struct X3D_FillProperties *fp;
        struct X3D_LineProperties *lp;
        struct X3D_Material *mat;
        struct X3D_ImageTexture *tex;
        struct X3D_TextureTransform *tt;
        struct X3D_Node *geom;
        struct X3D_Appearance *ap;
        struct X3D_TwoSidedMaterial *tsm;

	//ConsoleMessage ("gwl_set_materialFloatValue, entry %d, side %d, value %d new value %d",whichEntry, whichSide, whichField, nv);

        // If we do not have any node entries, maybe this is a new scene, and we have to get
        // the valid nodes?
        if (vectorSize(*shapeNodes) == 0 ) {
                if (fwl_android_get_valid_shapeNodes(shapeNodes) == 0) return;
        }

        // if we are here, we really do have at least one Shape node.

        struct X3D_Node *node = vector_get(struct X3D_Node *,*shapeNodes, whichEntry);

        //ConsoleMessage ("node %d is a %s",whichEntry,stringNodeType(node->_nodeType));
        fwl_decomposeShape(X3D_SHAPE(node),&fp,&lp,&mat,&tex,&tt,&geom);

	if (mat == NULL) return;

	if (mat->_nodeType == NODE_TwoSidedMaterial) {
		struct X3D_TwoSidedMaterial *tmat = X3D_TWOSIDEDMATERIAL(mat);
		float nnv = CLAMP(((float)nv)/100.0,0.0,1.0);
		// ConsoleMessage("nv as int %d, as float %f",nv,nnv);
		if (whichSide <3) {
			switch (whichField) {
				case 1: tmat->shininess=nnv; break;
				case 2: tmat->transparency=nnv; break;
				case 3: tmat->ambientIntensity=nnv; break;
				default: {
					ConsoleMessage ("hmmm - expect 1-3, got %d",whichField);
				}
			}
		} else {
			switch (whichField) {
				case 1: tmat->backShininess=nnv; break;
				case 2: tmat->backTransparency=nnv; break;
				case 3: tmat->backAmbientIntensity=nnv; break;
				default: {
					ConsoleMessage ("hmmm - expect 1-3, got %d",whichField);
				}
			}
		}
	  	tmat->_change++; // signal that this node has changed
	} else {
		ConsoleMessage ("getMaterialValue, expected a TwoSidedMaterial, not a %s\n",stringNodeType(mat->_nodeType));
	}
}


#undef JASTESTING
#ifdef JASTESTING


/* this is for looking at and manipulating the node memory table. Expect it to disappear sometime */
void printNodeMemoryTable(void) {

        int tc;
        ppOpenGL_Utils p = (ppOpenGL_Utils)gglobal()->OpenGL_Utils.prv;

	int foundHoleCount = 0;

LOCK_MEMORYTABLE
	for (tc=0; tc<vectorSize(p->linearNodeTable); tc++){
		struct X3D_Node *node = vector_get(struct X3D_Node *,p->linearNodeTable,tc);

		if (node != NULL) {
		if (node->_nodeType == NODE_Shape)  {
		//ConsoleMessage ("have shape/n");
		struct X3D_Shape *sh = X3D_SHAPE(node);
		if (sh->appearance != NULL) {
			struct X3D_Appearance *ap = X3D_APPEARANCE(sh->appearance);
					//ConsoleMessage ("have appearance\n");
			if (ap->material != NULL) {
				int i;
				struct X3D_Material *mt = X3D_MATERIAL(ap->material);
				//ConsoleMessage("have material\n");

/*
				for (i=0; i<3; i++) {
				mt->diffuseColor.c[i] += 0.2;
				if (mt->diffuseColor.c[i] > 0.95) mt->diffuseColor.c[i] = 0.2;
				}
*/

				mt->transparency += 0.05;
				if (mt->transparency > 1.0) mt->transparency=0.0;
				mt->_change ++;
			}

		}

/*
		ConsoleMessage ("mem table %d is %s ref %d\n",tc,stringNodeType(node->_nodeType),node->referenceCount);
		ConsoleMessage ("   shape appearance %p, geometry %p bbox %f %f %f bbcen %f %f %f\n",
			sh->appearance, sh->geometry,sh->bboxSize.c[0],sh->bboxSize.c[1],sh->bboxSize.c[2],
			sh->bboxCenter.c[0],sh->bboxCenter.c[1],sh->bboxCenter.c[2]);
*/


		}
		} else {
			foundHoleCount ++;
		}
        }

	//ConsoleMessage ("potentialHoleCount %d, foundHoleCount %d",p->potentialHoleCount, foundHoleCount);



UNLOCK_MEMORYTABLE

}
#endif //JASTESTING
#endif //ANDROID

#define TURN_OFF_SHOULDSORTCHILDREN node->_renderFlags = node->_renderFlags & (0xFFFF^ VF_shouldSortChildren);
/******************************************************************/
/* textureTransforms of all kinds */

/* change the clear colour, selected from the GUI, but do the command in the
   OpenGL thread */

void fwl_set_glClearColor (float red , float green , float blue , float alpha) {
	ppOpenGL_Utils p;
	ttglobal tg = gglobal();
	p = (ppOpenGL_Utils)tg->OpenGL_Utils.prv;
	p->cc_red = red; p->cc_green = green ; p->cc_blue = blue ; p->cc_alpha = alpha ;
	tg->OpenGL_Utils.cc_changed = TRUE;
}

void setglClearColor (float *val) {
	ppOpenGL_Utils p;
	ttglobal tg = gglobal();
	p = (ppOpenGL_Utils)tg->OpenGL_Utils.prv;
	p->cc_red = *val; val++;
	p->cc_green = *val; val++;
	p->cc_blue = *val;
#ifdef AQUA
	val++;
	p->cc_alpha = *val;
#endif
	tg->OpenGL_Utils.cc_changed = TRUE;
}

// use phong shading - better light reflectivity if set to true
void fwl_set_phongShading (int val) {
	ppOpenGL_Utils p;
	ttglobal tg = gglobal();
	p = (ppOpenGL_Utils)tg->OpenGL_Utils.prv;
	p->usePhongShaders = val;
}


int fwl_get_phongShading () {
	ppOpenGL_Utils p;
	ttglobal tg = gglobal();
	p = (ppOpenGL_Utils)tg->OpenGL_Utils.prv;
	return p->usePhongShaders;
}



/**************************************************************************************

		Determine near far clip planes.

We have 2 choices; normal geometry, or we have a Geospatial sphere.

If we have normal geometry (normal Viewpoint, or GeoViewpoint with GC coordinates)
then, we take our AABB (axis alligned bounding box), rotate the 8 Vertices, and
find the min/max Z distance, and just use this.

This works very well for examine objects, or when we are within a virtual world.
----

If we are Geospatializing around the earth, so we have GeoSpatial and have UTM or GD
coordinates, lets do some optimizations here.

First optimization, we know our height above the origin, and we most certainly are not
going to see things past the origin, so we assume far plane is height above the origin.

Second, we know our AABB contains the Geospatial sphere, and it ALSO contains the highest
mountain peak, so we just go and find the value representing the highest peak. Our
near plane is thus farPlane - highestPeak.

**************************************************************************************/

#undef MAXREADSIZE


static void shaderErrorLog(GLuint myShader, char *which) {
        #if defined  (GL_VERSION_2_0) || defined (GL_ES_VERSION_2_0)
#define MAX_INFO_LOG_SIZE 512
                GLchar infoLog[MAX_INFO_LOG_SIZE];
		char outline[MAX_INFO_LOG_SIZE*2];
                glGetShaderInfoLog(myShader, MAX_INFO_LOG_SIZE, NULL, infoLog);
		sprintf(outline,"problem with %s shader: %s",which, infoLog);
                ConsoleMessage (outline);
        #else
                ConsoleMessage ("Problem compiling shader");
        #endif
}


/****************************************************************************************/



/* find a shader that matches the capabilities requested. If no match, recreate it */
s_shader_capabilities_t *getMyShader(unsigned int rq_cap0) {

    /* GL_ES_VERSION_2_0 has GL_SHADER_COMPILER */
    #ifdef GL_SHADER_COMPILER
    GLboolean b;
    static bool haveDoneThis = false;
    #endif
	unsigned int rq_cap;
    int i;



    ppOpenGL_Utils p = gglobal()->OpenGL_Utils.prv;
    struct Vector *myShaderTable = p->myShaderTable;
    struct shaderTableEntry *new = NULL;

	rq_cap = rq_cap0;
	//rq_cap = NO_APPEARANCE_SHADER; //for thunking to simplest when debugging

    for (i=0; i<vectorSize(myShaderTable); i++) {
        struct shaderTableEntry *me = vector_get(struct shaderTableEntry *,myShaderTable, i);
        if (me->whichOne == rq_cap) {
            return me->myCapabilities;
        }
    }


    // if here, we did not find the shader already compiled for us.

    //ConsoleMessage ("getMyShader, looking for %x",rq_cap);

    //ConsoleMessage ("getMyShader, not found, have to create");
    //for (i=0; i<vectorSize(myShaderTable); i++) {
        //struct shaderTableEntry *me = vector_get(struct shaderTableEntry *,myShaderTable, i);
        //ConsoleMessage ("getMyShader, i %d, rq_cap %x, me->whichOne %x myCap %p\n",i,rq_cap,me->whichOne,me->myCapabilities);
     //}





    /* GL_ES_VERSION_2_0 has GL_SHADER_COMPILER */
#ifdef GL_SHADER_COMPILER
      glGetBooleanv(GL_SHADER_COMPILER,&b);
      if (!haveDoneThis) {
          haveDoneThis = true;
          if (!b) {
			//I found desktop openGL version 2.1.2  comes in here, but does still render OK
			//ConsoleMessage("NO SHADER COMPILER - have to sometime figure out binary shader distros");
			ConsoleMessage("no shader compiler\n");
			return NULL;
          }
      }
#endif

    // ConsoleMessage ("getMyShader, here now");

#ifdef VERBOSE
#if defined (GL_SHADER_COMPILER) && defined (GL_HIGH_FLOAT)
    /* GL_ES_VERSION_2_0 variables for shaders */
	{ /* debugging */
	GLint range[2]; GLint precision;
	GLboolean b;

	ConsoleMessage("starting getMyShader");

	glGetBooleanv(GL_SHADER_COMPILER,&b);
	if (b) ConsoleMessage("have shader compiler"); else ConsoleMessage("NO SHADER COMPILER");


	glGetShaderPrecisionFormat(GL_VERTEX_SHADER,GL_LOW_FLOAT, range, &precision);
	ConsoleMessage ("GL_VERTEX_SHADER, GL_LOW_FLOAT range [%d,%d],precision %d",range[0],range[1],precision);
	glGetShaderPrecisionFormat(GL_VERTEX_SHADER,GL_MEDIUM_FLOAT, range, &precision);
	ConsoleMessage ("GL_VERTEX_SHADER, GL_MEDIUM_FLOAT range [%d,%d],precision %d",range[0],range[1],precision);
	glGetShaderPrecisionFormat(GL_VERTEX_SHADER,GL_HIGH_FLOAT, range, &precision);
	ConsoleMessage ("GL_VERTEX_SHADER, GL_HIGH_FLOAT range [%d,%d],precision %d",range[0],range[1],precision);

	glGetShaderPrecisionFormat(GL_VERTEX_SHADER,GL_LOW_INT, range, &precision);
	ConsoleMessage ("GL_VERTEX_SHADER, GL_LOW_INT range [%d,%d],precision %d",range[0],range[1],precision);
	glGetShaderPrecisionFormat(GL_VERTEX_SHADER,GL_MEDIUM_INT, range, &precision);
	ConsoleMessage ("GL_VERTEX_SHADER, GL_MEDIUM_INT range [%d,%d],precision %d",range[0],range[1],precision);
	glGetShaderPrecisionFormat(GL_VERTEX_SHADER,GL_HIGH_INT, range, &precision);
	ConsoleMessage ("GL_VERTEX_SHADER, GL_HIGH_INT range [%d,%d],precision %d",range[0],range[1],precision);

	glGetShaderPrecisionFormat(GL_FRAGMENT_SHADER,GL_LOW_FLOAT, range, &precision);
	ConsoleMessage ("GL_FRAGMENT_SHADER, GL_LOW_FLOAT range [%d,%d],precision %d",range[0],range[1],precision);
	glGetShaderPrecisionFormat(GL_FRAGMENT_SHADER,GL_MEDIUM_FLOAT, range, &precision);
	ConsoleMessage ("GL_FRAGMENT_SHADER, GL_MEDIUM_FLOAT range [%d,%d],precision %d",range[0],range[1],precision);
	glGetShaderPrecisionFormat(GL_FRAGMENT_SHADER,GL_HIGH_FLOAT, range, &precision);
	ConsoleMessage ("GL_FRAGMENT_SHADER, GL_HIGH_FLOAT range [%d,%d],precision %d",range[0],range[1],precision);

	glGetShaderPrecisionFormat(GL_FRAGMENT_SHADER,GL_LOW_INT, range, &precision);
	ConsoleMessage ("GL_FRAGMENT_SHADER, GL_LOW_INT range [%d,%d],precision %d",range[0],range[1],precision);
	glGetShaderPrecisionFormat(GL_FRAGMENT_SHADER,GL_MEDIUM_INT, range, &precision);
	ConsoleMessage ("GL_FRAGMENT_SHADER, GL_MEDIUM_INT range [%d,%d],precision %d",range[0],range[1],precision);
	glGetShaderPrecisionFormat(GL_FRAGMENT_SHADER,GL_HIGH_INT, range, &precision);
	ConsoleMessage ("GL_FRAGMENT_SHADER, GL_HIGH_INT range [%d,%d],precision %d",range[0],range[1],precision);



	}
#endif // #ifdef GL_ES_VERSION_2_0 specific debugging
#endif //VERBOSE

    new = MALLOC(struct shaderTableEntry *, sizeof (struct shaderTableEntry));

    new ->whichOne = rq_cap;
    new->myCapabilities = MALLOC(s_shader_capabilities_t*, sizeof (s_shader_capabilities_t));

    //ConsoleMessage ("going to compile new shader for %x",rq_cap);
    makeAndCompileShader(new,p->usePhongShaders);

    vector_pushBack(struct shaderTableEntry*, myShaderTable, new);

    //ConsoleMessage ("going to return new %p",new);
    //ConsoleMessage ("... myCapabilities is %p",new->myCapabilities);
    return new->myCapabilities;
}



#define DESIRE(whichOne,zzz) ((whichOne & zzz)==zzz)

/* VERTEX inputs */

static const GLchar *vertPosDec = "\
    attribute      vec4 fw_Vertex; \n \
    uniform         mat4 fw_ModelViewMatrix; \n \
    uniform         mat4 fw_ProjectionMatrix; \n ";

static const GLchar *vertNormDec = " \
    uniform        mat3 fw_NormalMatrix;\n \
    attribute      vec3 fw_Normal; \n";

static const GLchar *vertSimColDec = "\
    attribute  vec4 fw_Color;\n ";

static const GLchar *vertTexMatrixDec = "\
    uniform mat4 fw_TextureMatrix;\n";

static const GLchar *vertTexCoordGenDec ="\
uniform int fw_textureCoordGenType;\n";

static const GLchar *vertTexCoordDec = "\
    attribute vec2 fw_MultiTexCoord0;\n";

static const GLchar *vertOneMatDec = "\
    uniform fw_MaterialParameters\n\
    fw_FrontMaterial; \n";
static const GLchar *vertBackMatDec = "\
    uniform fw_MaterialParameters fw_BackMaterial; \n";



/* VERTEX outputs */

static const GLchar *vecNormPos = " \
    vec3 vertexNorm; \
    vec4 vertexPos; \n";

static const GLchar *varyingNormPos = " \
    varying vec3 vertexNorm; \
    varying vec4 vertexPos; \n";

static const GLchar *varyingTexCoord = "\
    varying vec3 v_texC;\n";

static const GLchar *varyingFrontColour = "\
    varying vec4    v_front_colour; \n";

static const GLchar *varyingHatchPosition = "\
    varying vec2 hatchPosition; \n";

/* VERTEX Calculations */

static const GLchar *vertMainStart = "void main(void) { \n";

static const GLchar *vertEnd = "}";

static const GLchar *vertPos = "gl_Position = fw_ProjectionMatrix * fw_ModelViewMatrix * fw_Vertex;\n ";

static const GLchar *vertNormPosCalc = "\
	vertexNorm = normalize(fw_NormalMatrix * fw_Normal);\n \
	vertexPos = fw_ModelViewMatrix * fw_Vertex;\n ";

static const GLchar *vertSimColUse = "v_front_colour = fw_Color; \n";

static const GLchar *vertEmissionOnlyColourAss = "v_front_colour = fw_FrontMaterial.emission;\n";
static const GLchar *vertSingTexCalc = "v_texC = vec3(vec4(fw_TextureMatrix *vec4(fw_MultiTexCoord0,0,0))).stp;\n";

static const GLchar *vertSingTexCubeCalc = "\
    vec3 u=normalize(vec3(fw_ProjectionMatrix * fw_Vertex)); /* myEyeVertex */ \
    /* vec3 n=normalize(vec3(fw_NormalMatrix*fw_Normal)); \
    v_texC = reflect(u,n); myEyeNormal */ \n \
    /* v_texC = reflect(normalize(vec3(vertexPos)),vertexNorm);\n */ \
    v_texC = reflect(u,vertexNorm);\n";


/* TextureCoordinateGenerator mapping  */
const static GLchar *fragTCGTDefs = TEXTURECOORDINATEGENERATORDefs;

/*
Good hints for code here: http://www.opengl.org/wiki/Mathematics_of_glTexGen
*/
static const GLchar *sphEnvMapCalc = " \n     \
/* sphereEnvironMapping Calculation */ \
/* vec3 u=normalize(vec3(fw_ModelViewMatrix * fw_Vertex));  (myEyeVertex)  \
vec3 n=normalize(vec3(fw_NormalMatrix*fw_Normal)); \
vec3 r = reflect(u,n);  (myEyeNormal) */ \n\
vec3 u=normalize(vec3(vertexPos)); /* u is normalized position, used below more than once */ \n \
vec3 r= reflect(u,vertexNorm); \n\
if (fw_textureCoordGenType==TCGT_SPHERE) { /* TCGT_SPHERE  GL_SPHERE_MAP OpenGL Equiv */ \n\
    float m=2.0 * sqrt(r.x*r.x + r.y*r.y + (r.z*1.0)*(r.z*1.0)); \n\
    v_texC = vec3(r.x/m+0.5,r.y/m+0.5,0.0); \n \
}else if (fw_textureCoordGenType==TCGT_CAMERASPACENORMAL) /* GL_REFLECTION_MAP used for sampling cubemaps */ {\n \
	float dotResult = 2.0 * dot(u,r); \n\
	v_texC = vec3(u-r)*dotResult;\n\
} else { /* default usage - like default CubeMaps */ \n\
    vec3 u=normalize(vec3(fw_ProjectionMatrix * fw_Vertex)); /* myEyeVertex */ \
    v_texC = reflect(u,vertexNorm);\n \
}\n\
";

static const GLchar *vertHatchPosCalc = "hatchPosition = fw_Vertex.xy;\n";

static const GLchar *fillPropDefines = "\
uniform vec4 HatchColour; \n\
uniform bool hatched; uniform bool filled;\n\
uniform vec2 HatchScale; uniform vec2 HatchPct; uniform int algorithm; ";

//=============STRUCT METHOD FOR LIGHTS==================
// use for opengl, and angleproject desktop/d3d9
static const GLchar *lightDefines = "\
struct fw_MaterialParameters {\n\
  vec4 emission;\n\
  vec4 ambient;\n\
  vec4 diffuse;\n\
  vec4 specular;\n\
  float shininess;\n\
};\n\
uniform int lightcount;\n\
//uniform float lightRadius[MAX_LIGHTS];\n\
uniform int lightType[MAX_LIGHTS];//ANGLE like this\n\
struct fw_LightSourceParameters { \n\
  vec4 ambient;  \n\
  vec4 diffuse;   \n\
  vec4 specular; \n\
  vec4 position;   \n\
  vec4 halfVector;  \n\
  vec4 spotDirection; \n\
  float spotExponent; \n\
  float spotCutoff; \n\
  float spotCosCutoff; \n\
  vec3 Attenuations; \n\
  //float constantAttenuation; \n\
  //float linearAttenuation;  \n\
  //float quadraticAttenuation; \n\
  float lightRadius; \n\
  //int lightType; ANGLE doesnt like int in struct array \n\
}; \n\
\n\
uniform fw_LightSourceParameters fw_LightSource[MAX_LIGHTS] /* gl_MaxLights */ ;\n\
";



/* replace:
 linearAttenuation uniform float light_linAtten[MAX_LIGHTS];    \n\
 constantAttenuation		uniform float light_constAtten[MAX_LIGHTS];    \n\
 quadraticAttenuation	uniform float light_quadAtten[MAX_LIGHTS];    \n\
 spotCutoff				uniform float lightSpotCutoffAngle[MAX_LIGHTS];    \n\
 spotExponent			uniform float lightSpotBeamWidth[MAX_LIGHTS];    \n\
 ambient					uniform vec4 lightAmbient[MAX_LIGHTS];\n\
 diffuse					uniform vec4 lightDiffuse[MAX_LIGHTS];\n\
 position				uniform vec4 lightPosition[MAX_LIGHTS];\n\
 spotDirection			uniform vec4 lightSpotDirection[MAX_LIGHTS];\n\
 specular				uniform vec4 lightSpecular[MAX_LIGHTS];\n\
*/


static const GLchar *ADSLLightModel = "\n\
/* use ADSLightModel here the ADS colour is returned from the function.  */\n\
vec4 ADSLightModel(in vec3 myNormal, in vec4 myPosition, in bool useMatDiffuse) {\n\
  int i;\n\
  vec4 diffuse = vec4(0., 0., 0., 0.);\n\
  vec4 ambient = vec4(0., 0., 0., 0.);\n\
  vec4 specular = vec4(0., 0., 0., 1.);\n\
  vec3 normal = normalize (myNormal);\n\
\n\
  vec3 viewv = -normalize(myPosition.xyz); \n \
  bool backFacing = (dot(normal,viewv) < 0.0); \n \
  vec4 emissive;\n\
  vec4 matdiffuse = vec4(1.0,1.0,1.0,1.0);\n\
  float myAlph = 0.0;\n\
\n\
  fw_MaterialParameters myMat = fw_FrontMaterial;\n\
\n\
/* back Facing materials - flip the normal and grab back materials */ \n \
if (backFacing) { \n \
	normal = -normal; \n \
	myMat = fw_BackMaterial; \n \
} \n \
\n\
  emissive = myMat.emission;\n\
  myAlph = myMat.diffuse.a;\n\
  if(useMatDiffuse)\n\
    matdiffuse = myMat.diffuse;\n\
\n\
  /* apply the lights to this material */\n\
  for (i=0; i<MAX_LIGHTS; i++) {\n\
	if(i<lightcount) { /*weird but ANGLE needs constant loop*/ \n\
      vec4 myLightDiffuse = fw_LightSource[i].diffuse;\n\
      vec4 myLightAmbient = fw_LightSource[i].ambient;\n\
      vec4 myLightSpecular = fw_LightSource[i].specular;\n\
      vec4 myLightPosition = fw_LightSource[i].position; \n\
	  int myLightType = lightType[i]; //fw_LightSource[i].lightType;\n\
	  vec3 myLightDir = fw_LightSource[i].spotDirection.xyz; \n\
      vec3 eyeVector = normalize(myPosition.xyz);\n\
      vec3  VP;     /* vector of light direction and distance */\n\
	  VP = myLightPosition.xyz - myPosition.xyz;\n\
	  vec3 L = myLightDir; /*directional light*/ \n\
	  if(myLightType < 2) /*point and spot*/ \n\
	    L = normalize(VP); \n\
      float nDotL = max(dot(normal, L), 0.0);\n\
      vec3 halfVector = normalize(L - eyeVector);\n\
      /* normal dot light half vector */\n\
      float nDotHV = max(dot(normal,halfVector),0.0);\n\
      \n\
      if (myLightType==1) {\n\
        /* SpotLight */\n\
        float spotDot; \n\
        float spotAttenuation = 0.0; \n\
        float powerFactor = 0.0; /* for light dropoff */ \n\
        float attenuation; /* computed attenuation factor */\n\
        float d;            /* distance to vertex */            \n\
        d = length(VP);\n\
		if (nDotL > 0.0) {\n\
		  powerFactor = pow(nDotL,myMat.shininess); \n\
          /* tone down the power factor if myMat.shininess borders 0 */\n\
          if (myMat.shininess < 1.0) {\n\
		    powerFactor *= myMat.shininess; \n\
          } \n\
        } \n\
		attenuation = 1.0/(fw_LightSource[i].Attenuations.x + (fw_LightSource[i].Attenuations.y * d) + (fw_LightSource[i].Attenuations.z *d *d));\n\
        spotDot = dot (-L,myLightDir);\n\
        /* check against spotCosCutoff */\n\
		if (spotDot > fw_LightSource[i].spotCutoff) {\n\
          spotAttenuation = pow(spotDot,fw_LightSource[i].spotExponent);\n\
        }\n\
        attenuation *= spotAttenuation;\n\
        /* diffuse light computation */\n\
		diffuse += nDotL* matdiffuse*myLightDiffuse * attenuation;\n\
        /* ambient light computation */\n\
        ambient += myMat.ambient*myLightAmbient;\n\
        /* specular light computation */\n\
        specular += myLightSpecular * powerFactor * attenuation;\n\
        \n\
	  } else if (myLightType == 2) { \n\
        /* DirectionalLight */ \n\
        float powerFactor = 0.0; /* for light dropoff */\n\
		if (nDotL > 0.0) {\n\
		  powerFactor = pow(nDotHV, myMat.shininess);\n\
          /* tone down the power factor if myMat.shininess borders 0 */\n\
          if (myMat.shininess < 1.0) {\n\
		    powerFactor *= myMat.shininess;\n\
          }\n\
        }\n\
        /* Specular light computation */\n\
        specular += myMat.specular *myLightSpecular*powerFactor;\n\
        /* diffuse light computation */\n\
		diffuse += nDotL*matdiffuse*myLightDiffuse;\n\
        /* ambient light computation */\n\
        ambient += myMat.ambient*myLightAmbient; \n\
      } else {\n\
        /* PointLight */\n\
        float powerFactor=0.0; /* for light dropoff */\n\
        float attenuation = 0.0; /* computed attenuation factor */\n\
        float d = length(VP);  /* distance to vertex */ \n\
        /* are we within range? */\n\
        if (d <= fw_LightSource[i].lightRadius) {\n\
          if (nDotL > 0.0) {\n\
		    powerFactor = pow(nDotL, myMat.shininess);\n\
            //attenuation = (myMat.shininess-128.0);\n\
          }\n\
          /* this is actually the SFVec3f attenuation field */\n\
		  attenuation = 1.0/(fw_LightSource[i].Attenuations.x + (fw_LightSource[i].Attenuations.y * d) + (fw_LightSource[i].Attenuations.z *d *d));\n\
          /* diffuse light computation */\n\
          diffuse += nDotL* matdiffuse*myLightDiffuse * attenuation;\n\
          /* ambient light computation */\n\
          ambient += myMat.ambient*myLightAmbient;\n\
          /* specular light computation */\n\
          attenuation *= (myMat.shininess/128.0);\n\
          specular += myLightSpecular * powerFactor * attenuation;\n\
        }\n\
      }\n\
	 }\n\
  }\n\
  return clamp(vec4(vec3(ambient+diffuse+specular+emissive),myAlph), 0.0, 1.0);\n\
}\n\
";
#ifdef USING_SHADER_LIGHT_ARRAY_METHOD
//============= USING_SHADER_LIGHT_ARRAY_METHOD FOR LIGHTS==================
//used for angleproject winRT d3d11 (which can't/doesn't convert GLSL struct[] array to HLSL properly - just affects lights)
//will break custom shader node vertex/pixel shaders that try to use gl_LightSource[] ie teapot-Toon.wrl

static const GLchar *lightDefinesArrayMethod = "\
struct fw_MaterialParameters {\n\
  vec4 emission;\n\
  vec4 ambient;\n\
  vec4 diffuse;\n\
  vec4 specular;\n\
  float shininess;\n\
};\n\
uniform int lightcount;\n\
uniform int lightType[MAX_LIGHTS];\n\
uniform vec4 lightambient[MAX_LIGHTS];  \n\
uniform vec4 lightdiffuse[MAX_LIGHTS];   \n\
uniform vec4 lightspecular[MAX_LIGHTS]; \n\
uniform vec4 lightposition[MAX_LIGHTS];   \n\
uniform vec4 lighthalfVector[MAX_LIGHTS];  \n\
uniform vec4 lightspotDirection[MAX_LIGHTS]; \n\
uniform float lightspotExponent[MAX_LIGHTS]; \n\
uniform float lightspotCutoff[MAX_LIGHTS]; \n\
uniform float lightspotCosCutoff[MAX_LIGHTS]; \n\
uniform float lightRadius[MAX_LIGHTS]; \n\
uniform vec3 lightAttenuations[MAX_LIGHTS]; \n\
";

static const GLchar *ADSLLightModelArrayMethod = "\n\
/* use ADSLightModel here the ADS colour is returned from the function.  */\n\
vec4 ADSLightModel(in vec3 myNormal, in vec4 myPosition, in bool useMatDiffuse) {\n\
  int i;\n\
  vec4 diffuse = vec4(0., 0., 0., 0.);\n\
  vec4 ambient = vec4(0., 0., 0., 0.);\n\
  vec4 specular = vec4(0., 0., 0., 1.);\n\
  vec3 normal = normalize (myNormal);\n\
\n\
  vec3 viewv = -normalize(myPosition.xyz); \n \
  bool backFacing = (dot(normal,viewv) < 0.0); \n \
  vec4 emissive;\n\
  vec4 matdiffuse = vec4(1.0,1.0,1.0,1.0);\n\
  float myAlph = 0.0;\n\
\n\
  fw_MaterialParameters myMat = fw_FrontMaterial;\n\
\n\
/* back Facing materials - flip the normal and grab back materials */ \n \
if (backFacing) { \n \
	normal = -normal; \n \
	myMat = fw_BackMaterial; \n \
} \n \
\n\
  emissive = myMat.emission;\n\
  myAlph = myMat.diffuse.a;\n\
  if(useMatDiffuse)\n\
    matdiffuse = myMat.diffuse;\n\
\n\
  /* apply the lights to this material */\n\
  for (i=0; i<MAX_LIGHTS; i++) {\n\
  	if(i<lightcount){ /* weird but ANGLE for d3d9 needs constant loop (d3d11/winrt OK with variable loop)*/ \n\
      vec4 myLightDiffuse = lightdiffuse[i];\n\
      vec4 myLightAmbient = lightambient[i];\n\
      vec4 myLightSpecular = lightspecular[i];\n\
      vec4 myLightPosition = lightposition[i]; \n\
      vec4 myspotDirection = lightspotDirection[i]; \n\
      int myLightType = lightType[i]; \n\
      vec3 myLightDir = myspotDirection.xyz; \n\
      vec3 eyeVector = normalize(myPosition.xyz);\n\
      vec3  VP;     /* vector of light direction and distance */\n\
      VP = myLightPosition.xyz - myPosition.xyz;\n\
      vec3 L = myLightDir; /*directional light*/ \n\
      if(myLightType < 2) /*point and spot*/ \n\
        L = normalize(VP); \n\
      float nDotL = max(dot(normal, L), 0.0);\n\
      vec3 halfVector = normalize(L - eyeVector);\n\
      /* normal dot light half vector */\n\
      float nDotHV = max(dot(normal,halfVector),0.0);\n\
      \n\
      if (myLightType==1) {\n\
        /* SpotLight */\n\
        float spotDot; \n\
        float spotAttenuation = 0.0; \n\
        float powerFactor = 0.0; /* for light dropoff */ \n\
        float attenuation; /* computed attenuation factor */\n\
        float d;            /* distance to vertex */            \n\
        d = length(VP);\n\
        if (nDotL > 0.0) {\n\
          powerFactor = pow(nDotL,myMat.shininess); \n\
          /* tone down the power factor if myMat.shininess borders 0 */\n\
          if (myMat.shininess < 1.0) {\n\
              powerFactor *= myMat.shininess; \n\
          } \n\
        } \n\
        attenuation = 1.0/(lightAttenuations[i].x + (lightAttenuations[i].y * d) + (lightAttenuations[i].z *d *d));\n\
        spotDot = dot (-L,myLightDir);\n\
        /* check against spotCosCutoff */\n\
        if (spotDot > lightspotCutoff[i]) {\n\
          spotAttenuation = pow(spotDot,lightspotExponent[i]);\n\
        }\n\
        attenuation *= spotAttenuation;\n\
        /* diffuse light computation */\n\
        diffuse += nDotL* matdiffuse*myLightDiffuse * attenuation;\n\
        /* ambient light computation */\n\
        ambient += myMat.ambient*myLightAmbient;\n\
        /* specular light computation */\n\
        specular += myLightSpecular * powerFactor * attenuation;\n\
        \n\
      } else if (myLightType == 2) { \n\
        /* DirectionalLight */ \n\
        float powerFactor = 0.0; /* for light dropoff */\n\
        if (nDotL > 0.0) {\n\
          powerFactor = pow(nDotHV, myMat.shininess);\n\
          /* tone down the power factor if myMat.shininess borders 0 */\n\
          if (myMat.shininess < 1.0) {\n\
              powerFactor *= myMat.shininess;\n\
          }\n\
        }\n\
        /* Specular light computation */\n\
        specular += myMat.specular *myLightSpecular*powerFactor;\n\
        /* diffuse light computation */\n\
        diffuse += nDotL*matdiffuse*myLightDiffuse;\n\
        /* ambient light computation */\n\
        ambient += myMat.ambient*myLightAmbient; \n\
      } else {\n\
        /* PointLight */\n\
        float powerFactor=0.0; /* for light dropoff */\n\
        float attenuation = 0.0; /* computed attenuation factor */\n\
        float d = length(VP);  /* distance to vertex */ \n\
        /* are we within range? */\n\
        if (d <= lightRadius[i]) {\n\
          if (nDotL > 0.0) {\n\
              powerFactor = pow(nDotL, myMat.shininess);\n\
          }\n\
          /* this is actually the SFVec3f attenuation field */\n\
            attenuation = 1.0/(lightAttenuations[i].x + (lightAttenuations[i].y * d) + (lightAttenuations[i].z *d *d));\n\
          /* diffuse light computation */\n\
          diffuse += nDotL* matdiffuse*myLightDiffuse * attenuation;\n\
          /* ambient light computation */\n\
          ambient += myMat.ambient*myLightAmbient;\n\
          /* specular light computation */\n\
          attenuation *= (myMat.shininess/128.0);\n\
          specular += myLightSpecular * powerFactor * attenuation;\n\
        }\n\
      }\n\
     }\n\
  }\n\
  return clamp(vec4(vec3(ambient+diffuse+specular+emissive),myAlph), 0.0, 1.0);\n\
}\n\
";

#endif

/* FRAGMENT bits */
//#if defined (GL_HIGH_FLOAT) &&  defined(GL_MEDIUM_FLOAT)


/* GL_ES and Desktop GL are different... */
#if defined (GL_ES_VERSION_2_0)
	static const GLchar *fragHighPrecision = "precision highp float;\n ";
	static const GLchar *fragMediumPrecision = "precision mediump float;\n ";
	static const GLchar *maxLights = STR_MAX_LIGHTS; //"\n#define MAX_LIGHTS 2\n ";
#else
	static const GLchar *maxLights = STR_MAX_LIGHTS; //"\n#define MAX_LIGHTS 8\n ";
#endif


/* NOTE that we write to the vec4 "finalFrag", and at the end we assign
    the gl_FragColor, because we might have textures, fill properties, etc
   dug9 Jan 5, 2014 change of strategy to accommodate ONE_MAT + ONE_TEX = 0x2 + 0x8 = 0x10
    - in frag main change to 'cascade of v4*v4' so it's easier to combine MAT with TEX
	- frag main:
		void main() {
			vec4 finalFrag = vec4(1.0,1.0,1.0,1.0);
			finalFrag = v_front_color * finalFrag; //material
			finalFrag = texture2D(fw_Texture_unit0, v_texC.st) * finalFrag; //texture
			gl_FragColor = finalFrag;
		}


*/

//dug9 Jan 5, 2014 static const GLchar *fragMainStart = "void main() { vec4 finalFrag = vec4(0.,0.,0.,0.);\n";
static const GLchar *fragMainStart = "void main() { vec4 finalFrag = vec4(1.,1.,1.,1.);\n";
static const GLchar *anaglyphGrayFragEnd =	"float gray = dot(finalFrag.rgb, vec3(0.299, 0.587, 0.114)); \n \
                                              gl_FragColor = vec4(gray, gray, gray, finalFrag.a);}";

/* discard operations needed for really doing a good job in transparent situations (FillProperties, filled = false,
   for instance - drawing operations preclude sorting individual triangles for best rendering, so when the user
   requests best shaders, we add in this discard. */

static const GLchar *discardInFragEnd = "if (finalFrag.a==0.0) {discard; } else {gl_FragColor = finalFrag;}}";
static const GLchar *fragEnd = "gl_FragColor = finalFrag;}";


static const GLchar *fragTex0Dec = "uniform sampler2D fw_Texture_unit0; \n";
static const GLchar *fragTex0CubeDec = "uniform samplerCube fw_Texture_unit0; \n";


//dug9 Jan 5,2014 change to 'cascade of v4*v4' in frag main
static const GLchar *fragSimColAss = "finalFrag = v_front_colour * finalFrag;\n ";
static const GLchar *fragNoAppAss = "finalFrag = vec4(1.0, 1.0, 1.0, 1.0);\n";
static const GLchar *fragFrontColAss=    " finalFrag = v_front_colour * finalFrag;";
const static GLchar *fragADSLAss = "finalFrag = ADSLightModel(vertexNorm,vertexPos,true) * finalFrag;";
const static GLchar *vertADSLCalc = "v_front_colour = ADSLightModel(vertexNorm,vertexPos,true);";
const static GLchar *vertADSLCalc0 = "v_front_colour = ADSLightModel(vertexNorm,vertexPos,false);";

const static GLchar *fragSingTexAss = "finalFrag = texture2D(fw_Texture_unit0, v_texC.st) * finalFrag;\n";
const static GLchar *fragSingTexCubeAss = "finalFrag = textureCube(fw_Texture_unit0, v_texC) * finalFrag;\n";


/* MultiTexture stuff */
/* still to do:
 #define MTMODE_BLENDCURRENTALPHA       2
 #define MTMODE_DOTPRODUCT3     4
 #define MTMODE_SELECTARG2      5
 #define MTMODE_SELECTARG1      6
 #define MTMODE_BLENDDIFFUSEALPHA       7
 #define MTMODE_MODULATEINVCOLOR_ADDALPHA       10
 #define MTMODE_MODULATEINVALPHA_ADDCOLOR       15
 #define MTMODE_MODULATEALPHA_ADDCOLOR  16
 */
const static GLchar *fragMultiTexDef = MULTITEXTUREDefs;

static const GLchar *fragMultiTexUniforms = " \
/* defined for single textures... uniform sampler2D fw_Texture_unit0; */\
uniform sampler2D fw_Texture_unit1; \
uniform sampler2D fw_Texture_unit2; \
/* REMOVE these as shader compile takes long \
uniform sampler2D fw_Texture_unit3; \
uniform sampler2D fw_Texture_unit4; \
uniform sampler2D fw_Texture_unit5; \
uniform sampler2D fw_Texture_unit6; \
uniform sampler2D fw_Texture_unit7; \
*/  \
uniform int fw_Texture_mode0; \
uniform int fw_Texture_mode1; \
uniform int fw_Texture_mode2; \
/* REMOVE these as shader compile takes long \
uniform int fw_Texture_mode3; \
uniform int fw_Texture_mode4; \
uniform int fw_Texture_mode5; \
uniform int fw_Texture_mode6; \
uniform int fw_Texture_mode7; \
*/ \n\
\
uniform int textureCount;\n";

static const GLchar *fragFillPropFunc = "\
vec4 fillPropCalc(in vec4 prevColour, vec2 MCposition, int algorithm) {\
vec4 colour; \
vec2 position, useBrick; \
\
position = MCposition / HatchScale; \
\
if (algorithm == 0) {/* bricking  */ \
    if (fract(position.y * 0.5) > 0.5) \
        position.x += 0.5; \
        }\
\
/* algorithm 1, 2 = no futzing required here  */ \
if (algorithm == 3) {/* positive diagonals */ \
    vec2 curpos = position; \
    position.x -= curpos.y; \
} \
\
if (algorithm == 4) {/* negative diagonals */ \
    vec2 curpos = position; \
    position.x += curpos.y; \
} \
\
if (algorithm == 6) {/* diagonal crosshatch */ \
    vec2 curpos = position; \
    if (fract(position.y) > 0.5)  { \
        if (fract(position.x) < 0.5) position.x += curpos.y; \
        else position.x -= curpos.y; \
    } else { \
        if (fract(position.x) > 0.5) position.x += curpos.y; \
        else position.x -= curpos.y; \
    } \
} \
\
position = fract(position); \
\
useBrick = step(position, HatchPct); \
\
    if (filled) {colour = prevColour;} else { colour=vec4(0.,0.,0.,0); }\
if (hatched) { \
    colour = mix(HatchColour, colour, useBrick.x * useBrick.y); \
} \
return colour; } ";

static const GLchar *fragFillPropCalc = "\
finalFrag= fillPropCalc(finalFrag, hatchPosition, algorithm);\n";

static const GLchar *fragMulTexFunc ="\
vec4 finalColCalc(in vec4 prevColour, in int mode, in sampler2D tex, in vec2 texcoord) { \
vec4 texel = texture2D(tex,texcoord); \
vec4 rv = vec4(1.,0.,1.,1.);  \
\
if (mode==MTMODE_OFF) { rv = vec4(prevColour);} \
else if (mode==MTMODE_REPLACE) {rv = vec4(texture2D(tex, texcoord));}\
else if (mode==MTMODE_MODULATE) { \
\
vec3 ct,cf; \
float at,af; \
\
cf = prevColour.rgb; \
af = prevColour.a; \
\
ct = texel.rgb; \
at = texel.a; \
\
\
rv = vec4(ct*cf, at*af); \
\
} \
else if (mode==MTMODE_MODULATE2X) { \
vec3 ct,cf; \
float at,af; \
\
cf = prevColour.rgb; \
af = prevColour.a; \
\
ct = texel.rgb; \
at = texel.a; \
rv = vec4(vec4(ct*cf, at*af)*vec4(2.,2.,2.,2.)); \
}\
else if (mode==MTMODE_MODULATE4X) { \
vec3 ct,cf; \
float at,af; \
\
cf = prevColour.rgb; \
af = prevColour.a; \
\
ct = texel.rgb; \
at = texel.a; \
rv = vec4(vec4(ct*cf, at*af)*vec4(4.,4.,4.,4.)); \
}\
else if (mode== MTMODE_ADDSIGNED) {\
rv = vec4 (prevColour + texel - vec4 (0.5, 0.5, 0.5, -.5)); \
} \
else if (mode== MTMODE_ADDSIGNED2X) {\
rv = vec4 ((prevColour + texel - vec4 (0.5, 0.5, 0.5, -.5))*vec4(2.,2.,2.,2.)); \
} \
else if (mode== MTMODE_ADD) {\
rv= vec4 (prevColour + texel); \
} \
else if (mode== MTMODE_SUBTRACT) {\
rv = vec4 (prevColour - texel); \
} \
else if (mode==MTMODE_ADDSMOOTH) { \
rv = vec4 (prevColour + (prevColour - vec4 (1.,1.,1.,1.)) * texel); \
} \
return rv; \
\
} \n";

const static GLchar *fragMulTexCalc = "\
if(textureCount>=1) {finalFrag=finalColCalc(finalFrag,fw_Texture_mode0,fw_Texture_unit0,v_texC.st);} \n\
if(textureCount>=2) {finalFrag=finalColCalc(finalFrag,fw_Texture_mode1,fw_Texture_unit1,v_texC.st);} \n\
if(textureCount>=3) {finalFrag=finalColCalc(finalFrag,fw_Texture_mode2,fw_Texture_unit2,v_texC.st);} \n\
/* REMOVE these as shader compile takes long \
if(textureCount>=4) {finalFrag=finalColCalc(finalFrag,fw_Texture_mode3,fw_Texture_unit3,v_texC.st);} \n\
if(textureCount>=5) {finalFrag=finalColCalc(finalFrag,fw_Texture_mode4,fw_Texture_unit4,v_texC.st);} \n\
if(textureCount>=6) {finalFrag=finalColCalc(finalFrag,fw_Texture_mode5,fw_Texture_unit5,v_texC.st);} \n\
if(textureCount>=7) {finalFrag=finalColCalc(finalFrag,fw_Texture_mode6,fw_Texture_unit6,v_texC.st);} \n\
if(textureCount>=8) {finalFrag=finalColCalc(finalFrag,fw_Texture_mode7,fw_Texture_unit7,v_texC.st);} \n\
*/ \n";




const static GLchar *pointSizeDeclare="uniform float pointSize;\n";
const static GLchar *pointSizeAss="gl_PointSize = pointSize; \n";


static int getSpecificShaderSource (const GLchar *vertexSource[vertexEndMarker], const GLchar *fragmentSource[fragmentEndMarker], unsigned int whichOne, int usePhongShading) {

    bool doThis;
	bool didADSLmaterial;
#ifdef USING_SHADER_LIGHT_ARRAY_METHOD
	//for angleproject winRT d3d11 - can't do struct[] array for lights
	const GLchar *lightDefines0 = lightDefinesArrayMethod;
	const GLchar *ADSLLightModel0 = ADSLLightModelArrayMethod;
#else
	const GLchar *lightDefines0 = lightDefines;
	const GLchar *ADSLLightModel0 = ADSLLightModel;
#endif
	
	/* GL_ES - do we have medium precision, or just low precision?? */
    /* Phong shading - use the highest we have */
    /* GL_ES_VERSION_2_0 has these definitions */

#if defined (GL_ES_VERSION_2_0)
    bool haveHighPrecisionFragmentShaders = false;

#ifdef VARY_VERTEX_PRECISION
    bool haveHighPrecisionVertexShaders = false;
#endif

    GLint range[2]; GLint precision;

	// see where we are doing the lighting. Use highest precision there, if we can.
	if (usePhongShading) {
        	glGetShaderPrecisionFormat(GL_FRAGMENT_SHADER,GL_HIGH_FLOAT, range, &precision);
        	if (precision!=0) {
        	    haveHighPrecisionFragmentShaders=true;
        	} else {
        	    haveHighPrecisionFragmentShaders=false;
        	    glGetShaderPrecisionFormat(GL_FRAGMENT_SHADER,GL_MEDIUM_FLOAT, range, &precision);
        	    if (precision == 0) {
        	        ConsoleMessage("low precision Fragment shaders only available - view may not work so well");
        	    }
        	}
#ifdef VARY_VERTEX_PRECISION
	// if we do lighting on the Vertex shader side, do we have to worry about precision?
	} else {
        	glGetShaderPrecisionFormat(GL_VERTEX_SHADER,GL_HIGH_FLOAT, range, &precision);
        	if (precision!=0) {
        	    haveHighPrecisionVertexShaders=true;
        	} else {
        	    haveHighPrecisionVertexShaders=false;
        	    glGetShaderPrecisionFormat(GL_VERTEX_SHADER,GL_MEDIUM_FLOAT, range, &precision);
        	    if (precision == 0) {
        	        ConsoleMessage("low precision Vertex shaders only available - view may not work so well");
        	    }
        	}
#endif //VARY_VERTEX_PRECISION

	}
#else
    // ConsoleMessage ("seem to not have GL_MEDIUM_FLOAT or GL_HIGH_FLOAT");
#endif // GL_ES_VERSION_2_0 for GL_HIGH_FLOAT or GL_MEDIUM_FLOAT

	#if defined (VERBOSE) && defined (GL_ES_VERSION_2_0)
        { /* debugging - only */
        GLboolean b;

        glGetBooleanv(GL_SHADER_COMPILER,&b);
        if (b) ConsoleMessage("have shader compiler"); else ConsoleMessage("NO SHADER COMPILER");


        glGetShaderPrecisionFormat(GL_VERTEX_SHADER,GL_LOW_FLOAT, range, &precision);
        ConsoleMessage ("GL_VERTEX_SHADER, GL_LOW_FLOAT range [%d,%d],precision %d",range[0],range[1],precision);
        glGetShaderPrecisionFormat(GL_VERTEX_SHADER,GL_MEDIUM_FLOAT, range, &precision);
        ConsoleMessage ("GL_VERTEX_SHADER, GL_MEDIUM_FLOAT range [%d,%d],precision %d",range[0],range[1],precision);
        glGetShaderPrecisionFormat(GL_VERTEX_SHADER,GL_HIGH_FLOAT, range, &precision);
        ConsoleMessage ("GL_VERTEX_SHADER, GL_HIGH_FLOAT range [%d,%d],precision %d",range[0],range[1],precision);

        glGetShaderPrecisionFormat(GL_VERTEX_SHADER,GL_LOW_INT, range, &precision);
        ConsoleMessage ("GL_VERTEX_SHADER, GL_LOW_INT range [%d,%d],precision %d",range[0],range[1],precision);
        glGetShaderPrecisionFormat(GL_VERTEX_SHADER,GL_MEDIUM_INT, range, &precision);
        ConsoleMessage ("GL_VERTEX_SHADER, GL_MEDIUM_INT range [%d,%d],precision %d",range[0],range[1],precision);
        glGetShaderPrecisionFormat(GL_VERTEX_SHADER,GL_HIGH_INT, range, &precision);
        ConsoleMessage ("GL_VERTEX_SHADER, GL_HIGH_INT range [%d,%d],precision %d",range[0],range[1],precision);

        glGetShaderPrecisionFormat(GL_FRAGMENT_SHADER,GL_LOW_FLOAT, range, &precision);
        ConsoleMessage ("GL_FRAGMENT_SHADER, GL_LOW_FLOAT range [%d,%d],precision %d",range[0],range[1],precision);
        glGetShaderPrecisionFormat(GL_FRAGMENT_SHADER,GL_MEDIUM_FLOAT, range, &precision);
        ConsoleMessage ("GL_FRAGMENT_SHADER, GL_MEDIUM_FLOAT range [%d,%d],precision %d",range[0],range[1],precision);
        glGetShaderPrecisionFormat(GL_FRAGMENT_SHADER,GL_HIGH_FLOAT, range, &precision);
        ConsoleMessage ("GL_FRAGMENT_SHADER, GL_HIGH_FLOAT range [%d,%d],precision %d",range[0],range[1],precision);

        glGetShaderPrecisionFormat(GL_FRAGMENT_SHADER,GL_LOW_INT, range, &precision);
        ConsoleMessage ("GL_FRAGMENT_SHADER, GL_LOW_INT range [%d,%d],precision %d",range[0],range[1],precision);
        glGetShaderPrecisionFormat(GL_FRAGMENT_SHADER,GL_MEDIUM_INT, range, &precision);
        ConsoleMessage ("GL_FRAGMENT_SHADER, GL_MEDIUM_INT range [%d,%d],precision %d",range[0],range[1],precision);
        glGetShaderPrecisionFormat(GL_FRAGMENT_SHADER,GL_HIGH_INT, range, &precision);
        ConsoleMessage ("GL_FRAGMENT_SHADER, GL_HIGH_INT range [%d,%d],precision %d",range[0],range[1],precision);
        }
	#endif //VERBOSE for GL_ES_VERSION_2_0

    #ifdef VERBOSE
    if DESIRE(whichOne,NO_APPEARANCE_SHADER) ConsoleMessage ("want NO_APPEARANCE_SHADER");
    if DESIRE(whichOne,MATERIAL_APPEARANCE_SHADER) ConsoleMessage ("want MATERIAL_APPEARANCE_SHADER");
    if DESIRE(whichOne,TWO_MATERIAL_APPEARANCE_SHADER) ConsoleMessage ("want TWO_MATERIAL_APPEARANCE_SHADER");
    if DESIRE(whichOne,ONE_TEX_APPEARANCE_SHADER)ConsoleMessage("want ONE_TEX_APPEARANCE_SHADER");
    if DESIRE(whichOne,MULTI_TEX_APPEARANCE_SHADER)ConsoleMessage("want MULTI_TEX_APPEARANCE_SHADER");
    if DESIRE(whichOne,COLOUR_MATERIAL_SHADER)ConsoleMessage("want COLOUR_MATERIAL_SHADER");
    if DESIRE(whichOne,FILL_PROPERTIES_SHADER)ConsoleMessage("want FILL_PROPERTIES_SHADER");
    if DESIRE(whichOne,HAVE_LINEPOINTS_COLOR)ConsoleMessage ("want LINE_POINTS_COLOR");
    if DESIRE(whichOne,HAVE_LINEPOINTS_APPEARANCE)ConsoleMessage ("want LINE_POINTS_APPEARANCE");
    if DESIRE(whichOne,HAVE_TEXTURECOORDINATEGENERATOR) ConsoleMessage ("want HAVE_TEXTURECOORDINATEGENERATOR");
    if DESIRE(whichOne,HAVE_CUBEMAP_TEXTURE) ConsoleMessage ("want HAVE_CUBEMAP_TEXTURE");
    #endif //VERBOSE
#undef VERBOSE


    /* Cross shader Fragment bits - GL_ES_VERSION_2_0 has this */
#if defined(GL_ES_VERSION_2_0)
	fragmentSource[fragmentGLSLVersion] = "#version 100\n";
	vertexSource[vertexGLSLVersion] = "#version 100\n";
	if (haveHighPrecisionFragmentShaders)  {
		fragmentSource[fragmentPrecisionDeclare] = fragHighPrecision;
		//ConsoleMessage("have high precision fragment shaders");
	} else {
		fragmentSource[fragmentPrecisionDeclare] = fragMediumPrecision;
		//ConsoleMessage("have medium precision fragment shaders");
	}

#ifdef VARY_VERTEX_PRECISION
	// if we do lighting on the Vertex shader side, do we have to worry about precision?
	if (haveHighPrecisionVertexShaders)  {
		vertexSource[vertexPrecisionDeclare] = fragHighPrecision;
		ConsoleMessage("have high precision vertex shaders");
	} else {
		vertexSource[vertexPrecisionDeclare] = fragMediumPrecision;
		ConsoleMessage("have medium precision vertex shaders");
	}
#endif //VARY_VERTEX_PRECISION

#else
	//changed from 120 to 110 Apr2014: main shaders still seem to work the same, openGL 2.0 now compiles them, and 2.1 (by specs) compiles 110
	fragmentSource[fragmentGLSLVersion] = "#version 110\n";//"#version 120\n";
	vertexSource[vertexGLSLVersion] = "#version 110\n"; //"#version 120\n";
#endif

    fragmentSource[fragMaxLightsDeclare] = maxLights;
    vertexSource[vertMaxLightsDeclare] = maxLights;
    vertexSource[vertexPositionDeclare] = vertPosDec;



    /* User defined shaders - only give the defines, let the user do the rest */

    if ((whichOne & USER_DEFINED_SHADER_MASK) == 0) {
	/* initialize */

    /* Generic things first */

    /* Cross shader Vertex bits */

    vertexSource[vertexMainStart] = vertMainStart;
    vertexSource[vertexPositionCalculation] = vertPos;
    vertexSource[vertexMainEnd] = vertEnd;


    fragmentSource[fragmentMainStart] = fragMainStart;
	if(Viewer()->anaglyph)
		fragmentSource[fragmentMainEnd] = anaglyphGrayFragEnd;
	else {
        if (usePhongShading) fragmentSource[fragmentMainEnd] = discardInFragEnd;
        else fragmentSource[fragmentMainEnd] = fragEnd;
        //fragmentSource[fragmentMainEnd] = discardInFragEnd;
    }

    //ConsoleMessage ("whichOne %x mask %x",whichOne,~whichOne);


    /* specific strings for specific shader capabilities */

    if DESIRE(whichOne,COLOUR_MATERIAL_SHADER) {
        vertexSource[vertexSimpleColourDeclare] = vertSimColDec;
        vertexSource[vertFrontColourDeclare] = varyingFrontColour;
        vertexSource[vertexSimpleColourCalculation] = vertSimColUse;
	    vertexSource[vertexPointSizeDeclare] = pointSizeDeclare;
	    vertexSource[vertexPointSizeAssign] = pointSizeAss;
        fragmentSource[fragmentSimpleColourDeclare] = varyingFrontColour;
        fragmentSource[fragmentSimpleColourAssign] = fragSimColAss;
    }

    if DESIRE(whichOne,NO_APPEARANCE_SHADER) {
        fragmentSource[fragmentSimpleColourAssign] = fragNoAppAss;
	    vertexSource[vertexPointSizeDeclare] = pointSizeDeclare;
	    vertexSource[vertexPointSizeAssign] = pointSizeAss;

    }


    /* One or TWO material no texture shaders - one material, choose between
     Phong shading (slower) or Gouraud shading (faster). */

    if (usePhongShading) {
        doThis = (DESIRE(whichOne,MATERIAL_APPEARANCE_SHADER)) ||
            (DESIRE(whichOne,TWO_MATERIAL_APPEARANCE_SHADER));
    } else {
        doThis = DESIRE(whichOne,TWO_MATERIAL_APPEARANCE_SHADER);
    }

    if (doThis) {
        vertexSource[vertexNormPosOutput] = varyingNormPos;
        vertexSource[vertexNormalDeclare] = vertNormDec;
        vertexSource[vertexNormPosCalculation] = vertNormPosCalc;

        fragmentSource[fragmentLightDefines] = lightDefines0;
        fragmentSource[fragmentOneColourDeclare] = vertOneMatDec;
        fragmentSource[fragmentBackColourDeclare] = vertBackMatDec;
        fragmentSource[fragmentNormPosDeclare] = varyingNormPos;
        fragmentSource[fragmentADSLLightModel] = ADSLLightModel0;
        fragmentSource[fragmentADSLAssign] = fragADSLAss;

    }


        /* TWO_MATERIAL_APPEARANCE_SHADER - this does not crop up
         that often, so just use the PHONG shader. */
	didADSLmaterial = false;
    if((DESIRE(whichOne,MATERIAL_APPEARANCE_SHADER)) && (!usePhongShading)) {
        vertexSource[vertexNormalDeclare] = vertNormDec;
        vertexSource[vertexLightDefines] = lightDefines0;
        vertexSource[vertexOneMaterialDeclare] = vertOneMatDec;
        vertexSource[vertFrontColourDeclare] = varyingFrontColour;
        vertexSource[vertexNormPosCalculation] = vertNormPosCalc;
        vertexSource[vertexNormPosOutput] = vecNormPos;
        vertexSource[vertexLightingEquation] = ADSLLightModel0;
        vertexSource[vertexBackMaterialDeclare] = vertBackMatDec;
        vertexSource[vertexADSLCalculation] = vertADSLCalc;
		didADSLmaterial = true;
        fragmentSource[fragmentOneColourDeclare] = varyingFrontColour;
        fragmentSource[fragmentOneColourAssign] = fragFrontColAss;
    }


        if DESIRE(whichOne,HAVE_LINEPOINTS_APPEARANCE) {
            vertexSource[vertexLightDefines] = lightDefines0;
            vertexSource[vertFrontColourDeclare] = varyingFrontColour;
            vertexSource[vertexOneMaterialDeclare] = vertOneMatDec;

    		#if defined  (AQUA) || defined (GL_ES_VERSION_2_0)
	    vertexSource[vertexPointSizeDeclare] = pointSizeDeclare;
	    vertexSource[vertexPointSizeAssign] = pointSizeAss;
		#endif

            vertexSource[vertexOneMaterialCalculation] = vertEmissionOnlyColourAss;
            fragmentSource[fragmentSimpleColourDeclare] = varyingFrontColour;
            fragmentSource[fragmentSimpleColourAssign] = fragSimColAss;
        }


        /* texturing - MULTI_TEX builds on ONE_TEX */
        if (DESIRE(whichOne,ONE_TEX_APPEARANCE_SHADER) ||
            DESIRE(whichOne,HAVE_TEXTURECOORDINATEGENERATOR) ||
            DESIRE(whichOne,HAVE_CUBEMAP_TEXTURE) ||
            DESIRE(whichOne,MULTI_TEX_APPEARANCE_SHADER)) {
            vertexSource[vertexTexCoordInputDeclare] = vertTexCoordDec;
            vertexSource[vertexTexCoordOutputDeclare] = varyingTexCoord;
            vertexSource[vertexTextureMatrixDeclare] = vertTexMatrixDec;
            vertexSource[vertexSingleTextureCalculation] = vertSingTexCalc;
            if(didADSLmaterial)
            	vertexSource[vertexADSLCalculation] = vertADSLCalc0; //over-ride material diffuseColor with texture

            fragmentSource[fragmentTexCoordDeclare] = varyingTexCoord;
            fragmentSource[fragmentTex0Declare] = fragTex0Dec;
            fragmentSource[fragmentTextureAssign] = fragSingTexAss;
        }

        /* Cubemaps - do not multi-texture these yet */
    if (DESIRE(whichOne,HAVE_CUBEMAP_TEXTURE)) {
        vertexSource[vertexSingleTextureCalculation] = vertSingTexCubeCalc;

        fragmentSource[fragmentTex0Declare] = fragTex0CubeDec;
        fragmentSource[fragmentTextureAssign] = fragSingTexCubeAss;
    }

        /* MULTI_TEX builds on ONE_TEX */
        if DESIRE(whichOne,MULTI_TEX_APPEARANCE_SHADER) {
            /* we have to do the material params, in case we need to
                modulate/play with this. */

            vertexSource[vertexOneMaterialDeclare] = vertOneMatDec;
            vertexSource[vertexLightDefines] = lightDefines0;
            vertexSource[vertexNormPosCalculation] = vertNormPosCalc;
            vertexSource[vertexNormPosOutput] = vecNormPos;
            vertexSource[vertexLightingEquation] = ADSLLightModel0;
            vertexSource[vertexBackMaterialDeclare] = vertBackMatDec;

            fragmentSource[fragmentMultiTexDefines]= fragMultiTexUniforms;
            fragmentSource[fragmentMultiTexDeclare] = fragMultiTexDef;
            fragmentSource[fragmentTex0Declare] = fragTex0Dec;
            fragmentSource[fragmentMultiTexModel] = fragMulTexFunc;
            fragmentSource[fragmentTextureAssign] = fragMulTexCalc;
        }

    /* TextureCoordinateGenerator - do calcs in Vertex, fragment like one texture */
    if DESIRE(whichOne,HAVE_TEXTURECOORDINATEGENERATOR) {
        /* the vertex single texture calculation is different from normal single texture */
        /* pass in the type of generator, and do the calculations */
        vertexSource[vertexTextureMatrixDeclare] = vertTexCoordGenDec;
        vertexSource[vertexSingleTextureCalculation] = sphEnvMapCalc;

        vertexSource[vertexTCGTDefines] = fragTCGTDefs;

    }

        if DESIRE(whichOne,FILL_PROPERTIES_SHADER) {
            /* just add on top of the other shaders the fill properties "stuff" */

            vertexSource[vertexHatchPositionDeclare] = varyingHatchPosition;
            vertexSource[vertexHatchPositionCalculation] = vertHatchPosCalc;

            fragmentSource[fragmentFillPropDefines] = fillPropDefines;
            fragmentSource[fragmentHatchPositionDeclare] = varyingHatchPosition;
            fragmentSource[fragmentFillPropModel] = fragFillPropFunc;
            fragmentSource[fragmentFillPropAssign] = fragFillPropCalc;
        }

    } else {
    // user defined shaders
    if (whichOne >= USER_DEFINED_SHADER_START) {
        int me = 0;
        ppOpenGL_Utils p;
        ttglobal tg = gglobal();
        p = (ppOpenGL_Utils)tg->OpenGL_Utils.prv;

        me = (whichOne / USER_DEFINED_SHADER_START) -1;
        //ConsoleMessage ("HAVE USER DEFINED SHADER %x",whichOne);

        // add the following:
        // this has both Vertex manipulations, and lighting, etc.
//                    #define HEADLIGHT_LIGHT (MAX_LIGHTS-1)\n
        vertexSource[vertexMainStart] = "  \n \
					#define HEADLIGHT_LIGHT 0\n \
					#define ftransform() (fw_ProjectionMatrix*fw_ModelViewMatrix*fw_Vertex)\n \
                    #define gl_ModelViewProjectionMatrix (fw_ProjectionMatrix*fw_ModelViewMatrix)\n \
                    #define gl_NormalMatrix fw_NormalMatrix\n \
                    #define gl_ProjectionMatrix fw_ProjectionMatrix \n\
                    #define gl_ModelViewMatrix fw_ModelViewMatrix \n\
                    #define gl_TextureMatrix fw_TextureMatrix \n\
                    #define gl_Vertex fw_Vertex \n \
                    #define gl_Normal fw_Normal\n \
                    #define gl_Texture_unit0 fw_Texture_unit0\n \
                    #define gl_MultiTexCoord0 fw_MultiTexCoord0\n \
                    #define gl_Texture_unit1 fw_Texture_unit1\n \
                    #define gl_MultiTexCoord1 fw_MultiTexCoord1\n \
                    #define gl_Texture_unit2 fw_Texture_unit2\n \
                    #define gl_MultiTexCoord2 fw_MultiTexCoord2\n \
                    #define gl_LightSource fw_LightSource\n ";

	// copy over the same defines, but for the fragment shader.
    // Some GLSL compilers will complain about the "fttransform()"
    // definition if defined in a Fragment shader, so we judiciously
    // copy over things that are fragment-only.

//                   #define HEADLIGHT_LIGHT (MAX_LIGHTS-1)\n
	fragmentSource[fragmentMainStart] = " \
					#define HEADLIGHT_LIGHT 0\n \
                    #define gl_NormalMatrix fw_NormalMatrix\n \
                    #define gl_Normal fw_Normal\n \
                    #define gl_LightSource fw_LightSource\n ";




        vertexSource[vertexLightDefines] = lightDefines0;
        vertexSource[vertexSimpleColourDeclare] = vertSimColDec;
        vertexSource[vertFrontColourDeclare] = varyingFrontColour;



        vertexSource[vertexNormalDeclare] = vertNormDec;
        fragmentSource[fragmentLightDefines] = lightDefines0;
        //ConsoleMessage ("sources here for %d are %p and %p", me, p->userDefinedVertexShader[me], p->userDefinedFragmentShader[me]);

        if ((p->userDefinedVertexShader[me] == NULL) || (p->userDefinedFragmentShader[me]==NULL)) {
            ConsoleMessage ("no Shader Source found for user defined shaders...");
            return false;

        }
        fragmentSource[fragmentUserDefinedInput] = p->userDefinedFragmentShader[me];
        vertexSource[vertexUserDefinedInput] = p->userDefinedVertexShader[me];


    }
    }

//#define VERBOSE
    #ifdef VERBOSE
	/* print out the vertex source here */
		{
			vertexShaderResources_t x1;
			fragmentShaderResources_t x2;
            int i;

			ConsoleMessage ("Vertex source:\n");
			for (x1=vertexGLSLVersion; x1<vertexEndMarker; x1++) {
                    if (strlen(vertexSource[x1])>0)
				ConsoleMessage(vertexSource[x1]);
        }
			ConsoleMessage("Fragment Source:\n");
            i=0;
			for (x2=fragmentGLSLVersion; x2<fragmentEndMarker; x2++) {
				if (strlen(fragmentSource[x2])>0)
                ConsoleMessage(fragmentSource[x2]);
            }
		}
	#endif //VERBOSE
	return TRUE;
}
#undef VERBOSE


static void makeAndCompileShader(struct shaderTableEntry *me, bool phongShading) {

    GLint success;
	GLuint myVertexShader = 0;
	GLuint myFragmentShader= 0;

	GLuint myProg = 0;
	s_shader_capabilities_t *myShader = me->myCapabilities;
	const GLchar *vertexSource[vertexEndMarker];
	const GLchar  *fragmentSource[fragmentEndMarker];
   	vertexShaderResources_t x1;
	fragmentShaderResources_t x2;


#ifdef VERBOSE
        ConsoleMessage ("makeAndCompileShader called");
#endif //VERBOSE
#undef VERBOSE

   	/* initialize shader sources to blank strings, later we'll fill it in */
	for (x1=vertexGLSLVersion; x1<vertexEndMarker; x1++)
		vertexSource[x1] = "";
	for (x2=fragmentGLSLVersion; x2<fragmentEndMarker; x2++)
		fragmentSource[x2] = "";


	/* pointerize this */
	myProg = glCreateProgram(); /* CREATE_PROGRAM */
	(*myShader).myShaderProgram = myProg;

	/* assume the worst... */
	(*myShader).compiledOK = FALSE;

    /* we put the sources in 2 formats, allows for differing GL/GLES prefixes */
	if (!getSpecificShaderSource(vertexSource, fragmentSource, me->whichOne, phongShading)) {
		return;
	}


	myVertexShader = CREATE_SHADER (VERTEX_SHADER);
	SHADER_SOURCE(myVertexShader, vertexEndMarker, ((const GLchar **)vertexSource), NULL);
	COMPILE_SHADER(myVertexShader);
	GET_SHADER_INFO(myVertexShader, COMPILE_STATUS, &success);
	if (!success) {
		shaderErrorLog(myVertexShader,"VERTEX");
	} else {

		ATTACH_SHADER(myProg, myVertexShader);
	}

	/* get Fragment shader */
	myFragmentShader = CREATE_SHADER (FRAGMENT_SHADER);
	SHADER_SOURCE(myFragmentShader, fragmentEndMarker, (const GLchar **) fragmentSource, NULL);
	COMPILE_SHADER(myFragmentShader);
	GET_SHADER_INFO(myFragmentShader, COMPILE_STATUS, &success);
	if (!success) {
		shaderErrorLog(myFragmentShader,"FRAGMENT");
	} else {
		ATTACH_SHADER(myProg, myFragmentShader);
	}

	LINK_SHADER(myProg);

	glGetProgramiv(myProg,GL_LINK_STATUS, &success);
	(*myShader).compiledOK = (success == GL_TRUE);

	getShaderCommonInterfaces(myShader);
}
static void getShaderCommonInterfaces (s_shader_capabilities_t *me) {
	GLuint myProg = me->myShaderProgram;
    int i;


	#ifdef SHADERVERBOSE
	{
	GLsizei count;
	GLuint shaders[10];
	GLint  xxx[10];
	int i;
	GLchar sl[3000];


	printf ("getShaderCommonInterfaces, I am program %d\n",myProg);

	if (glIsProgram(myProg)) printf ("getShaderCommonInterfaces, %d is a program\n",myProg); else printf ("hmmm - it is not a program!\n");
	glGetAttachedShaders(myProg,10,&count,shaders);
	printf ("got %d attached shaders, they are: \n",count);
	for (i=0; i<count; i++) {
		GLsizei len;

		printf ("%d\n",shaders[i]);
		glGetShaderSource(shaders[i],3000,&len,sl);
		printf ("len %d\n",len);
		printf ("sl: %s\n",sl);
	}
	glGetProgramiv(myProg,GL_INFO_LOG_LENGTH, xxx); printf ("GL_INFO_LOG_LENGTH_STATUS %d\n",xxx[0]);
	glGetProgramiv(myProg,GL_LINK_STATUS, xxx); printf ("GL_LINK_STATUS %d\n",xxx[0]);
	glGetProgramiv(myProg,GL_VALIDATE_STATUS, xxx); printf ("GL_VALIDATE_STATUS %d\n",xxx[0]);
	glGetProgramiv(myProg,GL_ACTIVE_ATTRIBUTES, xxx); printf ("GL_ACTIVE_ATTRIBUTES %d\n",xxx[0]);
	glGetProgramiv(myProg,GL_ACTIVE_UNIFORMS, xxx); printf ("GL_ACTIVE_UNIFORMS %d\n",xxx[0]);

	glGetProgramiv(myProg,GL_INFO_LOG_LENGTH, xxx);
	if (xxx[0] != 0) {
		#define MAX_INFO_LOG_SIZE 512
                GLchar infoLog[MAX_INFO_LOG_SIZE];
                glGetProgramInfoLog(myProg, MAX_INFO_LOG_SIZE, NULL, infoLog);
		printf ("log: %s\n",infoLog);


	}
	}
	#endif /* DEBUG */


	me->myMaterialEmission = GET_UNIFORM(myProg,"fw_FrontMaterial.emission");
	me->myMaterialDiffuse = GET_UNIFORM(myProg,"fw_FrontMaterial.diffuse");
	me->myMaterialShininess = GET_UNIFORM(myProg,"fw_FrontMaterial.shininess");
	me->myMaterialAmbient = GET_UNIFORM(myProg,"fw_FrontMaterial.ambient");
	me->myMaterialSpecular = GET_UNIFORM(myProg,"fw_FrontMaterial.specular");

	me->myMaterialBackEmission = GET_UNIFORM(myProg,"fw_BackMaterial.emission");
	me->myMaterialBackDiffuse = GET_UNIFORM(myProg,"fw_BackMaterial.diffuse");
	me->myMaterialBackShininess = GET_UNIFORM(myProg,"fw_BackMaterial.shininess");
	me->myMaterialBackAmbient = GET_UNIFORM(myProg,"fw_BackMaterial.ambient");
	me->myMaterialBackSpecular = GET_UNIFORM(myProg,"fw_BackMaterial.specular");

        //me->lightState = GET_UNIFORM(myProg,"lightState");
        //me->lightType = GET_UNIFORM(myProg,"lightType");
        //me->lightRadius = GET_UNIFORM(myProg,"lightRadius");
	me->lightcount = GET_UNIFORM(myProg,"lightcount");

    /* get lights in a more normal OpenGL GLSL format */

    /*
     struct gl_LightSourceParameters
     {
        vec4 ambient;              // Aclarri
        vec4 diffuse;              // Dcli
        vec4 specular;             // Scli
        vec4 position;             // Ppli
        vec4 halfVector;           // Derived: Hi
        vec4 spotDirection;        // Sdli
        float spotExponent;        // Srli
        float spotCutoff;          // Crli
        float spotCosCutoff;       // Derived: cos(Crli)
		vec3 Attenuations (const,lin,quad)
        //float constantAttenuation; // K0
        //float linearAttenuation;   // K1
        //float quadraticAttenuation;// K2
		float lightRadius;
		int lightType;
     };


     uniform gl_LightSourceParameters gl_LightSource[gl_MaxLights];
     */
		{
			//using lighsource arrays - see shader
			char uniformName[100];
			me->haveLightInShader = false;
#ifdef USING_SHADER_LIGHT_ARRAY_METHOD
			//char* sndx;
			for (i = 0; i<MAX_LIGHTS; i++) {
				char* sndx;
				/* go through and modify the array for each variable */
				strcpy(uniformName, "lightambient[0]");
				sndx = strstr(uniformName, "["); 
				sndx[1] = '0' + i;
				me->lightAmbient[i] = GET_UNIFORM(myProg, uniformName);

				//ConsoleMessage ("light Uniform test for %d is %s, %d",i,uniformName,me->lightAmbient[i]);

				strcpy(uniformName, "lightdiffuse[0]");
				sndx = strstr(uniformName, "[");
				sndx[1] = '0' + i;
				me->lightDiffuse[i] = GET_UNIFORM(myProg, uniformName);
				//ConsoleMessage ("light Uniform test for %d is %s, %d",i,uniformName,me->lightDiffuse[i]);


				strcpy(uniformName, "lightspecular[0]");
				sndx = strstr(uniformName, "[");
				sndx[1] = '0' + i;
				me->lightSpecular[i] = GET_UNIFORM(myProg, uniformName);
				//ConsoleMessage ("light Uniform test for %d is %s, %d",i,uniformName,me->lightSpecular[i]);


				strcpy(uniformName, "lightposition[0]");
				sndx = strstr(uniformName, "[");
				sndx[1] = '0' + i;
				me->lightPosition[i] = GET_UNIFORM(myProg, uniformName);
				//ConsoleMessage ("light Uniform test for %d is %s, %d",i,uniformName,me->lightPosition[i]);


				// flag used to determine if we have to send light position info to this shader
				if (me->lightPosition[i] != -1) me->haveLightInShader = true;

				strcpy(uniformName, "lightspotDirection[0]");
				sndx = strstr(uniformName, "[");
				sndx[1] = '0' + i;
				me->lightSpotDir[i] = GET_UNIFORM(myProg, uniformName);
				//ConsoleMessage ("light Uniform test for %d is %s, %d",i,uniformName,me->lightSpotDir[i]);


				strcpy(uniformName, "lightspotExponent[0]");
				sndx = strstr(uniformName, "[");
				sndx[1] = '0' + i;
				me->lightSpotBeamWidth[i] = GET_UNIFORM(myProg, uniformName);
				//ConsoleMessage ("light Uniform test for %d is %s, %d",i,uniformName,me->lightSpotBeamWidth[i]);


				strcpy(uniformName, "lightspotCutoff[0]");
				sndx = strstr(uniformName, "[");
				sndx[1] = '0' + i;
				me->lightSpotCutoffAngle[i] = GET_UNIFORM(myProg, uniformName);
				//ConsoleMessage ("light Uniform test for %d is %s, %d",i,uniformName,me->lightSpotCutoffAngle[i]);


				strcpy(uniformName, "lightAttenuations[0]");
				sndx = strstr(uniformName, "[");
				sndx[1] = '0' + i;
				me->lightAtten[i] = GET_UNIFORM(myProg, uniformName);

				strcpy(uniformName, "lightRadius[0]");
				sndx = strstr(uniformName, "[");
				sndx[1] = '0' + i;
				me->lightRadius[i] = GET_UNIFORM(myProg, uniformName);
				//ConsoleMessage ("light Uniform test for %d is %s, %d",i,uniformName,me->lightQuadAtten[i]);

			}

#else //USING_SHADER_LIGHT_ARRAY_METHOD
        strcpy(uniformName,"fw_LightSource[0].");
        for (i=0; i<MAX_LIGHTS; i++) {
            /* go through and modify the array for each variable */
            uniformName[15] = '0' + i;

            strcpy(&uniformName[18],"ambient");

            //ConsoleMessage ("have uniform name request :%s:",uniformName);
            me->lightAmbient[i] = GET_UNIFORM(myProg,uniformName);

            //ConsoleMessage ("light Uniform test for %d is %s, %d",i,uniformName,me->lightAmbient[i]);

            strcpy(&uniformName[18],"diffuse");
            me->lightDiffuse[i] = GET_UNIFORM(myProg,uniformName);
            //ConsoleMessage ("light Uniform test for %d is %s, %d",i,uniformName,me->lightDiffuse[i]);


            strcpy(&uniformName[18],"specular");
            me->lightSpecular[i] = GET_UNIFORM(myProg,uniformName);
            //ConsoleMessage ("light Uniform test for %d is %s, %d",i,uniformName,me->lightSpecular[i]);


            strcpy(&uniformName[18],"position");
            me->lightPosition[i] = GET_UNIFORM(myProg,uniformName);
            //ConsoleMessage ("light Uniform test for %d is %s, %d",i,uniformName,me->lightPosition[i]);


            // flag used to determine if we have to send light position info to this shader
            if (me->lightPosition[i] != -1) me->haveLightInShader = true;

            strcpy(&uniformName[18],"spotDirection");
            me->lightSpotDir[i] = GET_UNIFORM(myProg,uniformName);
            //ConsoleMessage ("light Uniform test for %d is %s, %d",i,uniformName,me->lightSpotDir[i]);


            strcpy(&uniformName[18],"spotExponent");
            me->lightSpotBeamWidth[i] = GET_UNIFORM(myProg,uniformName);
            //ConsoleMessage ("light Uniform test for %d is %s, %d",i,uniformName,me->lightSpotBeamWidth[i]);


            strcpy(&uniformName[18],"spotCutoff");
            me->lightSpotCutoffAngle[i] = GET_UNIFORM(myProg,uniformName);
            //ConsoleMessage ("light Uniform test for %d is %s, %d",i,uniformName,me->lightSpotCutoffAngle[i]);


            strcpy(&uniformName[18],"Attenuations");
            me->lightAtten[i] = GET_UNIFORM(myProg,uniformName);

	//strcpy(&uniformName[18],"constantAttenuation");
            //me->lightConstAtten[i] = GET_UNIFORM(myProg,uniformName);
            ////ConsoleMessage ("light Uniform test for %d is %s, %d",i,uniformName,me->lightConstAtten[i]);
            //

            //strcpy(&uniformName[18],"linearAttenuation");
            //me->lightLinAtten[i] = GET_UNIFORM(myProg,uniformName);
            ////ConsoleMessage ("light Uniform test for %d is %s, %d",i,uniformName,me->lightLinAtten[i]);
            //

            //strcpy(&uniformName[18],"quadraticAttenuation");
            //me->lightQuadAtten[i] = GET_UNIFORM(myProg,uniformName);
            ////ConsoleMessage ("light Uniform test for %d is %s, %d",i,uniformName,me->lightQuadAtten[i]);

            strcpy(&uniformName[18],"lightRadius");
            me->lightRadius[i] = GET_UNIFORM(myProg,uniformName);
            //ConsoleMessage ("light Uniform test for %d is %s, %d",i,uniformName,me->lightQuadAtten[i]);

			//strcpy(&uniformName[18],"lightType");
            //me->lightType[i] = GET_UNIFORM(myProg,uniformName);
            //ConsoleMessage ("light Uniform test for %d is %s, %d",i,uniformName,me->lightQuadAtten[i]);

        }
#endif // USING_SHADER_LIGHT_ARRAY_METHOD
		strcpy(uniformName,"lightType[0]");
		for (i = 0; i < MAX_LIGHTS; i++) {
			/* go through and modify the array for each variable */
			uniformName[10] = '0' + i;
			me->lightType[i] = GET_UNIFORM(myProg, uniformName);
		}
    }

    //if (me->haveLightInShader) ConsoleMessage ("this shader HAS lightfields");

	me->ModelViewMatrix = GET_UNIFORM(myProg,"fw_ModelViewMatrix");
	me->ProjectionMatrix = GET_UNIFORM(myProg,"fw_ProjectionMatrix");
	me->NormalMatrix = GET_UNIFORM(myProg,"fw_NormalMatrix");
	me->TextureMatrix = GET_UNIFORM(myProg,"fw_TextureMatrix");
	me->Vertices = GET_ATTRIB(myProg,"fw_Vertex");

	me->Normals = GET_ATTRIB(myProg,"fw_Normal");
	me->Colours = GET_ATTRIB(myProg,"fw_Color");

	me->TexCoords = GET_ATTRIB(myProg,"fw_MultiTexCoord0");


    for (i=0; i<MAX_MULTITEXTURE; i++) {
        char line[200];
        sprintf (line,"fw_Texture_unit%d",i);
        me->TextureUnit[i]= GET_UNIFORM(myProg,line);
        sprintf (line,"fw_Texture_mode%d",i);
        me->TextureMode[i] = GET_UNIFORM(myProg,line);
        //printf ("   i %d tu %d mode %d\n",i,me->TextureUnit[i],me->TextureMode[i]);

    }

    me->textureCount = GET_UNIFORM(myProg,"textureCount");
    //printf ("GETUNIFORM for textureCount is %d\n",me->textureCount);


	/* for FillProperties */
	me->myPointSize = GET_UNIFORM(myProg, "pointSize");
	me->hatchColour = GET_UNIFORM(myProg,"HatchColour");
	me->hatchPercent = GET_UNIFORM(myProg,"HatchPct");
	me->hatchScale = GET_UNIFORM(myProg,"HatchScale");
	me->filledBool = GET_UNIFORM(myProg,"filled");
	me->hatchedBool = GET_UNIFORM(myProg,"hatched");
	me->algorithm = GET_UNIFORM(myProg,"algorithm");

    /* TextureCoordinateGenerator */
    me->texCoordGenType = GET_UNIFORM(myProg,"fw_textureCoordGenType");


	#ifdef VERBOSE
	printf ("shader uniforms: vertex %d normal %d modelview %d projection %d\n",
		me->Vertices, me->Normals, me->ModelViewMatrix, me->ProjectionMatrix);
        printf ("hatchColour %d, hatchPercent %d",me->hatchColour, me->hatchPercent);
	#endif


}

void calculateViewingSpeed();
static void handle_GeoLODRange(struct X3D_GeoLOD *node) {
	int oldInRange;
	GLDOUBLE cx,cy,cz;
	/* find the length of the line between the moved center and our current viewer position */
	getCurrentPosInModel(FALSE);
	calculateViewingSpeed();
	cx = Viewer()->currentPosInModel.x - node->__movedCoords.c[0];
	cy = Viewer()->currentPosInModel.y - node->__movedCoords.c[1];
	cz = Viewer()->currentPosInModel.z - node->__movedCoords.c[2];

	 //printf ("geoLOD, distance between me and center is %lf\n", sqrt (cx*cx + cy*cy + cz*cz));

	/* try to see if we are closer than the range */
	oldInRange = node->__inRange;

    /* handle squares, as it is faster than doing square roots */
	if((cx*cx+cy*cy+cz*cz) > (node->range * node->range)) {
		node->__inRange = FALSE;
	} else {
		node->__inRange = TRUE;
	}


	if (oldInRange != node->__inRange) {

		#ifdef VERBOSE
		if (node->__inRange) printf ("TRUE:  "); else printf ("FALSE: ");
		printf ("range changed; level %d, comparing %lf:%lf:%lf and range %lf node %u\n",
			node->__level, cx,cy,cz, node->range, node);
		#endif

		/* initialize the "children" field, if required */
		if (node->children.p == NULL) node->children.p=MALLOC(struct X3D_Node **,sizeof(struct X3D_Node *) * 4);

		if (node->__inRange ==  TRUE) { 
			#ifdef VERBOSE
			printf ("GeoLOD %u level %d, inRange set to FALSE, range %lf\n",node, node->__level, node->range);
			#endif
			node->level_changed = 1;
			node->children.p[0] = node->__child1Node;
			node->children.p[1] = node->__child2Node;
			node->children.p[2] = node->__child3Node;
			node->children.p[3] = node->__child4Node;
			node->children.n = 4;
		} else {
			#ifdef VERBOSE
			printf ("GeoLOD %u level %d, inRange set to TRUE range %lf\n",node, node->__level, node->range);
			#endif
			node->level_changed = 0;
			node->children.n = 0;
			if( node->__rootUrl )
			{
				node->children.p[0] = node->__rootUrl;
				node->children.n = 1;
			}
			else if( node->rootNode.p && node->rootNode.p[0] )
			{
				node->children.p[0] = node->rootNode.p[0];
				node->children.n = 1;
			}
		}
		MARK_EVENT(X3D_NODE(node), offsetof (struct X3D_GeoLOD, level_changed));
		MARK_EVENT(X3D_NODE(node), offsetof (struct X3D_GeoLOD, children));
		oldInRange = X3D_GEOLOD(node)->__inRange;

		/* lets work out extent here */
		INITIALIZE_EXTENT;
		/* printf ("geolod range changed, initialized extent, czyzsq %4.2f rangesw %4.2f from %4.2f %4.2f %4.2f\n",
cx*cx+cy*cy+cz*cz,node->range*node->range,cx,cy,cz); */
		update_node(X3D_NODE(node));
	}
}

#ifdef DEBUGGING_CODE
/* draw a simple bounding box around an object */
void drawBBOX(struct X3D_Node *node) {

/* debugging */	FW_GL_COLOR3F((float)1.0,(float)0.6,(float)0.6);
/* debugging */
/* debugging */	/* left group */
/* debugging */	glBegin(GL_LINES);
/* debugging */	glVertex3d(node->EXTENT_MIN_X, node->EXTENT_MIN_Y, node->EXTENT_MIN_Z);
/* debugging */	glVertex3d(node->EXTENT_MIN_X, node->EXTENT_MIN_Y, node->EXTENT_MAX_Z);
/* debugging */	glEnd();
/* debugging */
/* debugging */	glBegin (GL_LINES);
/* debugging */	glVertex3d(node->EXTENT_MIN_X, node->EXTENT_MIN_Y, node->EXTENT_MIN_Z);
/* debugging */	glVertex3d(node->EXTENT_MIN_X, node->EXTENT_MAX_Y, node->EXTENT_MIN_Z);
/* debugging */	glEnd();
/* debugging */
/* debugging */	glBegin (GL_LINES);
/* debugging */	glVertex3d(node->EXTENT_MIN_X, node->EXTENT_MAX_Y, node->EXTENT_MIN_Z);
/* debugging */	glVertex3d(node->EXTENT_MIN_X, node->EXTENT_MAX_Y, node->EXTENT_MAX_Z);
/* debugging */	glEnd();
/* debugging */
/* debugging */	glBegin (GL_LINES);
/* debugging */	glVertex3d(node->EXTENT_MIN_X, node->EXTENT_MIN_Y, node->EXTENT_MAX_Z);
/* debugging */	glVertex3d(node->EXTENT_MIN_X, node->EXTENT_MAX_Y, node->EXTENT_MAX_Z);
/* debugging */	glEnd();
/* debugging */
/* debugging */	/* right group */
/* debugging */	glBegin (GL_LINES);
/* debugging */	glVertex3d(node->EXTENT_MAX_X, node->EXTENT_MIN_Y, node->EXTENT_MIN_Z);
/* debugging */	glVertex3d(node->EXTENT_MAX_X, node->EXTENT_MIN_Y, node->EXTENT_MAX_Z);
/* debugging */	glEnd();
/* debugging */
/* debugging */	glBegin (GL_LINES);
/* debugging */	glVertex3d(node->EXTENT_MAX_X, node->EXTENT_MIN_Y, node->EXTENT_MIN_Z);
/* debugging */	glVertex3d(node->EXTENT_MAX_X, node->EXTENT_MAX_Y, node->EXTENT_MIN_Z);
/* debugging */	glEnd();
/* debugging */
/* debugging */	glBegin (GL_LINES);
/* debugging */	glVertex3d(node->EXTENT_MAX_X, node->EXTENT_MAX_Y, node->EXTENT_MIN_Z);
/* debugging */	glVertex3d(node->EXTENT_MAX_X, node->EXTENT_MAX_Y, node->EXTENT_MAX_Z);
/* debugging */	glEnd();
/* debugging */
/* debugging */	glBegin (GL_LINES);
/* debugging */	glVertex3d(node->EXTENT_MAX_X, node->EXTENT_MIN_Y, node->EXTENT_MAX_Z);
/* debugging */	glVertex3d(node->EXTENT_MAX_X, node->EXTENT_MAX_Y, node->EXTENT_MAX_Z);
/* debugging */	glEnd();
/* debugging */
/* debugging */	/* joiners */
/* debugging */	glBegin (GL_LINES);
/* debugging */	glVertex3d(node->EXTENT_MIN_X, node->EXTENT_MIN_Y, node->EXTENT_MIN_Z);
/* debugging */	glVertex3d(node->EXTENT_MAX_X, node->EXTENT_MIN_Y, node->EXTENT_MIN_Z);
/* debugging */	glEnd();
/* debugging */
/* debugging */	glBegin (GL_LINES);
/* debugging */	glVertex3d(node->EXTENT_MIN_X, node->EXTENT_MIN_Y, node->EXTENT_MAX_Z);
/* debugging */	glVertex3d(node->EXTENT_MAX_X, node->EXTENT_MIN_Y, node->EXTENT_MAX_Z);
/* debugging */	glEnd();
/* debugging */
/* debugging */	glBegin (GL_LINES);
/* debugging */	glVertex3d(node->EXTENT_MIN_X, node->EXTENT_MAX_Y, node->EXTENT_MIN_Z);
/* debugging */	glVertex3d(node->EXTENT_MAX_X, node->EXTENT_MAX_Y, node->EXTENT_MIN_Z);
/* debugging */	glEnd();
/* debugging */
/* debugging */	glBegin (GL_LINES);
/* debugging */	glVertex3d(node->EXTENT_MIN_X, node->EXTENT_MAX_Y, node->EXTENT_MAX_Z);
/* debugging */	glVertex3d(node->EXTENT_MAX_X, node->EXTENT_MAX_Y, node->EXTENT_MAX_Z);
/* debugging */	glEnd();

}
#endif //DEBUGGING_CODE

static void calculateNearFarplanes(struct X3D_Node *vpnode, int layerid ) {
	struct point_XYZ bboxPoints[8];
	GLDOUBLE cfp = -DBL_MAX;
	GLDOUBLE cnp = DBL_MAX;
	GLDOUBLE MM[16];
    bool doingGeoSpatial = false;
    double bboxMovedCentreZ = 0.0;
    double bboxSphereRadius = 0.0;

#ifdef VERBOSE
    int smooger = 0;
#endif

	int ci;
    struct X3D_Node* rn = rootNode();
	ttglobal tg = gglobal();
	X3D_Viewer *viewer = ViewerByLayerId(layerid);



	#ifdef VERBOSE
    if (smooger == 0) {
	printf ("have a bound viewpoint... lets calculate our near/far planes from it \n");
	printf ("we are currently at %4.2f %4.2f %4.2f\n",Viewer()->currentPosInModel.x, Viewer()->currentPosInModel.y, Viewer()->currentPosInModel.z);
    }
	#endif


	/* verify parameters here */
	if ((vpnode->_nodeType != NODE_Viewpoint) &&
		(vpnode->_nodeType != NODE_OrthoViewpoint) &&
		(vpnode->_nodeType != NODE_GeoViewpoint)) {
		printf ("can not do this node type yet %s, for cpf\n",stringNodeType(vpnode->_nodeType));
		viewer->nearPlane = DEFAULT_NEARPLANE;
		viewer->farPlane = DEFAULT_FARPLANE;
		viewer->backgroundPlane = DEFAULT_BACKGROUNDPLANE;
		return;
	}

    if (vpnode->_nodeType == NODE_GeoViewpoint) {
        doingGeoSpatial = true;
    }

	if (rn == NULL) {
		return; /* nothing to display yet */
	}

    /* if doing GeoSpatial, use radius to view model, rather than a rotated bounding box */
    if (doingGeoSpatial) {
        if ((rn->EXTENT_MAX_X - rn->EXTENT_MIN_X) > bboxSphereRadius) {
            bboxSphereRadius = rn->EXTENT_MAX_X - rn->EXTENT_MIN_X;
        }
        if ((rn->EXTENT_MAX_Y - rn->EXTENT_MIN_Y) > bboxSphereRadius) {
            bboxSphereRadius = rn->EXTENT_MAX_Y - rn->EXTENT_MIN_Y;
        }
        if ((rn->EXTENT_MAX_Z - rn->EXTENT_MIN_Z) > bboxSphereRadius) {
            bboxSphereRadius = rn->EXTENT_MAX_Z - rn->EXTENT_MIN_Z;
        }
        bboxSphereRadius /=2.0; // diameter to radius

#ifdef VERBOSE
        if (smooger == 0) {
            ConsoleMessage ("bboxSphereRadius %lf",bboxSphereRadius);
        }
#endif

    }

	FW_GL_GETDOUBLEV(GL_MODELVIEW_MATRIX, MM);

		#ifdef VERBOSE
		printf ("rootNode extents x: %4.2f %4.2f  y:%4.2f %4.2f z: %4.2f %4.2f\n",rootNode()->EXTENT_MAX_X, rootNode()->EXTENT_MIN_X,rootNode()->EXTENT_MAX_Y, rootNode()->EXTENT_MIN_Y,rootNode()->EXTENT_MAX_Z, rootNode()->EXTENT_MIN_Z);
		#endif

		/* make up 8 vertices for our bounding box, and place them within our view */
        moveAndRotateThisPoint(&bboxPoints[0], rn->EXTENT_MIN_X, rn->EXTENT_MIN_Y, rn->EXTENT_MIN_Z,MM);
        moveAndRotateThisPoint(&bboxPoints[1], rn->EXTENT_MIN_X, rn->EXTENT_MIN_Y, rn->EXTENT_MAX_Z,MM);
        moveAndRotateThisPoint(&bboxPoints[2], rn->EXTENT_MIN_X, rn->EXTENT_MAX_Y, rn->EXTENT_MIN_Z,MM);
        moveAndRotateThisPoint(&bboxPoints[3], rn->EXTENT_MIN_X, rn->EXTENT_MAX_Y, rn->EXTENT_MAX_Z,MM);
        moveAndRotateThisPoint(&bboxPoints[4], rn->EXTENT_MAX_X, rn->EXTENT_MIN_Y, rn->EXTENT_MIN_Z,MM);
        moveAndRotateThisPoint(&bboxPoints[5], rn->EXTENT_MAX_X, rn->EXTENT_MIN_Y, rn->EXTENT_MAX_Z,MM);
        moveAndRotateThisPoint(&bboxPoints[6], rn->EXTENT_MAX_X, rn->EXTENT_MAX_Y, rn->EXTENT_MIN_Z,MM);
        moveAndRotateThisPoint(&bboxPoints[7], rn->EXTENT_MAX_X, rn->EXTENT_MAX_Y, rn->EXTENT_MAX_Z,MM);



		for (ci=0; ci<8; ci++) {
            bboxMovedCentreZ += bboxPoints[ci].z;

			#ifdef XXVERBOSE
            if (smooger == 0)
			printf ("moved bbox node %d is %4.2f %4.2f %4.2f\n",ci,bboxPoints[ci].x, bboxPoints[ci].y, bboxPoints[ci].z);
			#endif

            if (!doingGeoSpatial) {
                if (-(bboxPoints[ci].z) > cfp) cfp = -(bboxPoints[ci].z);
                if (-(bboxPoints[ci].z) < cnp) cnp = -(bboxPoints[ci].z);
            }
		}

    bboxMovedCentreZ /= 8.0; // average of 8 z values from bbox

    if (doingGeoSpatial) {
        cnp = -bboxMovedCentreZ - bboxSphereRadius;
        cfp = -bboxMovedCentreZ; // + bboxSphereRadius;
    }

#ifdef VERBOSE
    if (smooger==0) {
        ConsoleMessage ("centre of bbox is %lf Z away",bboxMovedCentreZ);
        ConsoleMessage ("bboxMovedCentreZ minus bboxRadius %lf",-bboxMovedCentreZ - bboxSphereRadius);
    }
#endif

	/* lets bound check here, both must be positive, and farPlane more than DEFAULT_NEARPLANE */
	/* because we may be navigating towards the shapes, we give the nearPlane a bit of room, otherwise
	   we might miss part of the geometry that comes closest to us */
	cnp = cnp/2.0;
	if (cnp<DEFAULT_NEARPLANE) cnp = DEFAULT_NEARPLANE;

	if (cfp<1.0) cfp = 1.0;
	/* if we are moving, or if we have something with zero depth, floating point calculation errors could
	   give us a geometry that is at (or, over) the far plane. Eg, tests/49.wrl, where we have Text nodes,
	   can give us this issue; so lets give us a bit of leeway here, too */
	cfp *= 1.25;


	#ifdef VERBOSE
	if (smooger == 0) {

        printf ("cnp %lf cfp before leaving room for Background %lf\n",cnp,cfp);
        //cnp = 0.1; cfp = 75345215.0 * 2.0;
    }
#endif

    /* do we have a GeoViewpoint, and is the near plane about zero?                     */
    /* we CAN have the issue if we have the world in an AABB, and we have one of the    */
    /* corners of the AABB behind us; the near plane will be <1, but the surface        */
    /* will still be really far away                                                    */
    /*      In this case, we try and use the elevation to give us a hand                */
    if ((cnp<1.0) && (vpnode->_nodeType == NODE_GeoViewpoint)) {
#ifdef VERBOSE
        cnp = Viewer()->currentPosInModel.z/16.0;
        if (smooger == 0) {
            ConsoleMessage ("vp height %lf moved height %lf posinModel %f",X3D_GEOVIEWPOINT(vpnode)->position.c[2],
                                    X3D_GEOVIEWPOINT(vpnode)->__movedPosition.c[2],Viewer()->currentPosInModel.z);
            smooger ++; if (smooger == 100) smooger = 0;
        }
#endif
#undef VERBOSE

    }

	/* lets use these values; leave room for a Background or TextureBackground node here */
	viewer->nearPlane = min(cnp,DEFAULT_NEARPLANE);
	/* backgroundPlane goes between the farthest geometry, and the farPlane */
	if (vectorSize(getActiveBindableStacks(tg)->background)!= 0) {
		viewer->farPlane = max(cfp * 10.0,DEFAULT_FARPLANE);
		viewer->backgroundPlane = max(cfp*5.0,DEFAULT_BACKGROUNDPLANE);
	} else {
		viewer->farPlane = max(cfp,DEFAULT_FARPLANE);
		viewer->backgroundPlane = max(cfp,DEFAULT_BACKGROUNDPLANE); /* just set it to something */
	}
}

void doglClearColor() {
	ppOpenGL_Utils p;
	ttglobal tg = gglobal();
	p = (ppOpenGL_Utils)tg->OpenGL_Utils.prv;
	FW_GL_CLEAR_COLOR(p->cc_red, p->cc_green, p->cc_blue, p->cc_alpha);
	tg->OpenGL_Utils.cc_changed = FALSE;
}





/* did we have a TextureTransform in the Appearance node? */
void do_textureTransform (struct X3D_Node *textureNode, int ttnum) {
    FW_GL_MATRIX_MODE(GL_TEXTURE);
	FW_GL_LOAD_IDENTITY();

	/* is this a simple TextureTransform? */
	if (textureNode->_nodeType == NODE_TextureTransform) {
        //ConsoleMessage ("do_textureTransform, node is indeed a NODE_TextureTransform");
		struct X3D_TextureTransform  *ttt = (struct X3D_TextureTransform *) textureNode;
		/*  Render transformations according to spec.*/
        	FW_GL_TRANSLATE_F(-((ttt->center).c[0]),-((ttt->center).c[1]), 0);		/*  5*/
        	FW_GL_SCALE_F(((ttt->scale).c[0]),((ttt->scale).c[1]),1);			/*  4*/
        	FW_GL_ROTATE_RADIANS(ttt->rotation,0,0,1);					/*  3*/
        	FW_GL_TRANSLATE_F(((ttt->center).c[0]),((ttt->center).c[1]), 0);		/*  2*/
        	FW_GL_TRANSLATE_F(((ttt->translation).c[0]), ((ttt->translation).c[1]), 0);	/*  1*/

	/* is this a MultiTextureTransform? */
	} else  if (textureNode->_nodeType == NODE_MultiTextureTransform) {
		struct X3D_MultiTextureTransform *mtt = (struct X3D_MultiTextureTransform *) textureNode;
		if (ttnum < mtt->textureTransform.n) {
			struct X3D_TextureTransform *ttt = (struct X3D_TextureTransform *) mtt->textureTransform.p[ttnum];
			/* is this a simple TextureTransform? */
			if (ttt->_nodeType == NODE_TextureTransform) {
				/*  Render transformations according to spec.*/
        			FW_GL_TRANSLATE_F(-((ttt->center).c[0]),-((ttt->center).c[1]), 0);		/*  5*/
        			FW_GL_SCALE_F(((ttt->scale).c[0]),((ttt->scale).c[1]),1);			/*  4*/
        			FW_GL_ROTATE_RADIANS(ttt->rotation,0,0,1);					/*  3*/
        			FW_GL_TRANSLATE_F(((ttt->center).c[0]),((ttt->center).c[1]), 0);		/*  2*/
        			FW_GL_TRANSLATE_F(((ttt->translation).c[0]), ((ttt->translation).c[1]), 0);	/*  1*/
			} else {
				printf ("MultiTextureTransform expected a textureTransform for texture %d, got %d\n",
					ttnum, ttt->_nodeType);
			}
		} else {
			printf ("not enough textures in MultiTextureTransform....\n");
        	}
	} else {
		printf ("expected a textureTransform node, got %d\n",textureNode->_nodeType);
	}

	FW_GL_MATRIX_MODE(GL_MODELVIEW);
}

void clear_shader_table()
{
	/* clearing the shader table forces the shaders to be re-lego-assembled and compiled
		- useful for switching anaglyph on/off (a different fragEnd pixel shader is used)
		- used in fwl_initialize_GL()
	*/
	ppOpenGL_Utils p;
	ttglobal tg = gglobal();
	p = (ppOpenGL_Utils)tg->OpenGL_Utils.prv;

	if (p->myShaderTable != NULL) {
		int i;

		for (i=0; i<vectorSize(p->myShaderTable); i++) {
        		struct shaderTableEntry *me = vector_get(struct shaderTableEntry *,p->myShaderTable, i);
			FREE_IF_NZ(me);
        	}
		deleteVector (struct shaderTableEntry *,p->myShaderTable);
		p->myShaderTable = newVector(struct shaderTableEntry *, 8);

	}
}
/**
 *   fwl_initializa_GL: initialize GLEW (->rdr caps) and OpenGL initial state
 */
bool fwl_initialize_GL()
{
	char blankTexture[] = {0x40, 0x40, 0x40, 0xFF};
	float gl_linewidth;
	ppOpenGL_Utils p;
	ttglobal tg = gglobal();
	p = (ppOpenGL_Utils)tg->OpenGL_Utils.prv;

	PRINT_GL_ERROR_IF_ANY("fwl_initialize_GL start 1");
	initialize_rdr_caps();

	PRINT_GL_ERROR_IF_ANY("fwl_initialize_GL start 3");

	/* lets make sure everything is sync'd up */

#if KEEP_FV_INLIB
#if defined(TARGET_X11) || defined(TARGET_MOTIF)
	XFlush(Xdpy);
#endif
#endif /* KEEP_FV_INLIB */


	PRINT_GL_ERROR_IF_ANY("fwl_initialize_GL start 4");

	FW_GL_MATRIX_MODE(GL_PROJECTION);
	FW_GL_LOAD_IDENTITY();
	FW_GL_MATRIX_MODE(GL_MODELVIEW);

	PRINT_GL_ERROR_IF_ANY("fwl_initialize_GL start 6");

	FW_GL_CLEAR_COLOR(p->cc_red, p->cc_green, p->cc_blue, p->cc_alpha);

	PRINT_GL_ERROR_IF_ANY("fwl_initialize_GL start 7");

	PRINT_GL_ERROR_IF_ANY("fwl_initialize_GL start 8");


	FW_GL_DEPTHFUNC(GL_LEQUAL);
	glEnable(GL_DEPTH_TEST);

	PRINT_GL_ERROR_IF_ANY("fwl_initialize_GL start 9");

	gl_linewidth = gglobal()->Mainloop.gl_linewidth;

    // dp pointSize in shaders on more modern OpenGL renderings
    // keep Windows and Linux doing old way, as we have failures
    // circa 2013 in this.

    #if defined  (AQUA) || defined (GL_ES_VERSION_2_0)
    	#if defined (GL_PROGRAM_POINT_SIZE)
    	glEnable(GL_PROGRAM_POINT_SIZE);
    	#endif
    	#if defined (GL_PROGRAM_POINT_SIZE_EXT)
    	glEnable(GL_PROGRAM_POINT_SIZE_EXT);
    	#endif
    #else
	glPointSize (gl_linewidth);
    #endif

	glLineWidth(gl_linewidth);

	PRINT_GL_ERROR_IF_ANY("fwl_initialize_GL start a");



	/*
     * JAS - ALPHA testing for textures - right now we just use 0/1 alpha
     * JAS   channel for textures - true alpha blending can come when we sort
     * JAS   nodes.
	 */

	glEnable(GL_BLEND);
	FW_GL_BLENDFUNC(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	FW_GL_CLEAR(GL_COLOR_BUFFER_BIT);

	PRINT_GL_ERROR_IF_ANY("fwl_initialize_GL start b");

	/* for textured appearance add specular highlights as a separate secondary color
	   redbook p.270, p.455 and http://www.gamedev.net/reference/programming/features/oglch9excerpt/

	   if we don't have texture we can disable this (less computation)...
	   but putting this here is already a saving ;)...
	*/

	PRINT_GL_ERROR_IF_ANY("fwl_initialize_GL start c0");

	/* keep track of light states; initial turn all lights off except for headlight */
	initializeLightTables();

	PRINT_GL_ERROR_IF_ANY("fwl_initialize_GL start c1");


	/* ensure state of GL_CULL_FACE */
	CULL_FACE_INITIALIZE;

	FW_GL_PIXELSTOREI(GL_UNPACK_ALIGNMENT,1);
	FW_GL_PIXELSTOREI(GL_PACK_ALIGNMENT,1);

	PRINT_GL_ERROR_IF_ANY("fwl_initialize_GL start c");


        /* create an empty texture, defaultBlankTexture, to be used when a texture is loading, or if it fails */
        FW_GL_GENTEXTURES (1,&tg->Textures.defaultBlankTexture);
        glBindTexture (GL_TEXTURE_2D, tg->Textures.defaultBlankTexture);
        FW_GL_TEXPARAMETERI( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        FW_GL_TEXPARAMETERI( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        FW_GL_TEXIMAGE2D(GL_TEXTURE_2D, 0, GL_RGBA,  1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, blankTexture);

        PRINT_GL_ERROR_IF_ANY("fwl_initialize_GL start d");

	/* remove entries in the shader table, if they exist. Android, on "bring to front" will
	   call this routine, and shaders will be re-created as they are needed to display geometry.
	*/
	clear_shader_table();
	/* moved to clear_shader_table() for anaglyph toggling, dug9 Aug 5, 2012
	if (p->myShaderTable != NULL) {
		int i;

		for (i=0; i<vectorSize(p->myShaderTable); i++) {
        		struct shaderTableEntry *me = vector_get(struct shaderTableEntry *,p->myShaderTable, i);
			FREE_IF_NZ(me);
        	}
		deleteVector (struct shaderTableEntry *,p->myShaderTable);
		p->myShaderTable = newVector(struct shaderTableEntry *, 8);

	}
	*/
	return TRUE;
}

void BackEndClearBuffer(int which) {
	if(which == 2) {
		FW_GL_CLEAR(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	} else {
		if(which==1) {
			FW_GL_CLEAR(GL_DEPTH_BUFFER_BIT);
		}
	}
}

/* turn off all non-headlight lights; will turn them on if required. */
void BackEndLightsOff() {
	int i;
	for (i=0; i<HEADLIGHT_LIGHT; i++) {
		setLightState(i, FALSE);
	}
}


void fw_glMatrixMode(GLint mode) {
	ppOpenGL_Utils p = (ppOpenGL_Utils)gglobal()->OpenGL_Utils.prv;

	p->whichMode = mode;
	#ifdef VERBOSE
	printf ("fw_glMatrixMode, projTOS %d, modTOS %d texvTOS %d\n",p->projectionviewTOS,p->modelviewTOS, p->textureviewTOS);

	switch (p->whichMode) {
		case GL_PROJECTION: printf ("glMatrixMode(GL_PROJECTION)\n"); break;
		case GL_MODELVIEW: printf ("glMatrixMode(GL_MODELVIEW)\n"); break;
		case GL_TEXTURE: printf ("glMatrixMode(GL_TEXTURE)\n"); break;
	}
	#endif

	switch (p->whichMode) {
		case GL_PROJECTION: p->currentMatrix = (GLDOUBLE *) &p->FW_ProjectionView[p->projectionviewTOS]; break;
		case GL_MODELVIEW: p->currentMatrix = (GLDOUBLE *) &p->FW_ModelView[p->modelviewTOS]; break;
		case GL_TEXTURE: p->currentMatrix = (GLDOUBLE *) &p->FW_TextureView[p->textureviewTOS]; break;
		default: printf ("invalid mode sent in it is %d, expected one of %d %d %d\n",p->whichMode, GL_PROJECTION,GL_MODELVIEW,GL_TEXTURE);
	}

}

void fw_glLoadIdentity(void) {
	ppOpenGL_Utils p = (ppOpenGL_Utils)gglobal()->OpenGL_Utils.prv;
    //ConsoleMessage ("fw_glLoadIdentity, whichMode %d, tex %d",p->whichMode,GL_TEXTURE);
	loadIdentityMatrix(p->currentMatrix);
	FW_GL_LOADMATRIX(p->currentMatrix);
}

//#define PUSHMAT(a,b,c,d) case a: \
//	b++;\
//	if (b>=c) {b=c-1; \
//		printf ("stack overflow, whichmode %d\n",p->whichMode); } \
//	memcpy ((void *)d[b], (void *)d[b-1],sizeof(GLDOUBLE)*16);\
//	p->currentMatrix = d[b];\
//	break;

MATRIX4* PushMat( int a, int *b, int c, MATRIX4 *d){
	(*b)++;
	if (*b >= c) {
		printf("stack overflow, depth %d whichmode %d\n", *b, a);
		*b = c - 1;
	}
	memcpy((void *)d[*b], (void *)d[*b - 1], sizeof(GLDOUBLE)* 16);
	return &d[*b];
}


void printMaxStackUsed(){
	ppOpenGL_Utils p = (ppOpenGL_Utils)gglobal()->OpenGL_Utils.prv;
	ConsoleMessage("%25s %d\n","max modelview stack used", p->maxStackUsed);
}
void fw_glPushMatrix(void) {
	ppOpenGL_Utils p = (ppOpenGL_Utils)gglobal()->OpenGL_Utils.prv;

	switch (p->whichMode) {
	case GL_PROJECTION: p->currentMatrix = *PushMat(GL_PROJECTION, &p->projectionviewTOS, MAX_SMALL_MATRIX_STACK, p->FW_ProjectionView); break;
	case GL_MODELVIEW:  p->currentMatrix = *PushMat(GL_MODELVIEW, &p->modelviewTOS, MAX_LARGE_MATRIX_STACK, p->FW_ModelView); break;
	case GL_TEXTURE:	p->currentMatrix = *PushMat(GL_TEXTURE, &p->textureviewTOS, MAX_SMALL_MATRIX_STACK, p->FW_TextureView); break;
	default:printf("wrong mode in popMatrix\n");
	}
	p->maxStackUsed = max(p->maxStackUsed, p->modelviewTOS);
	FW_GL_LOADMATRIX(p->currentMatrix);
}
//void fw_glPushMatrix(void) {
//	ppOpenGL_Utils p = (ppOpenGL_Utils)gglobal()->OpenGL_Utils.prv;
//
//	switch (p->whichMode) {
//		PUSHMAT (GL_PROJECTION,p->projectionviewTOS,MAX_SMALL_MATRIX_STACK,p->FW_ProjectionView)
//		PUSHMAT (GL_MODELVIEW,p->modelviewTOS,MAX_LARGE_MATRIX_STACK,p->FW_ModelView)
//		PUSHMAT (GL_TEXTURE,p->textureviewTOS,MAX_SMALL_MATRIX_STACK,p->FW_TextureView)
//		default :printf ("wrong mode in popMatrix\n");
//	}
//
// 	FW_GL_LOADMATRIX(p->currentMatrix);
//}
//#undef PUSHMAT

//#define POPMAT(a,b,c) case a: b--; if (b<0) {b=0;printf ("popMatrix, stack underflow, whichMode %d\n",p->whichMode);} p->currentMatrix = c[b]; break;
MATRIX4 *PopMat(int a, int *b, MATRIX4 *c){
	(*b)--;
	if (*b < 0) {
		*b = 0;
		printf("popMatrix, stack underflow, whichMode %d\n", a);
	}
	return &c[*b];
}
void fw_glPopMatrix(void) {
	ppOpenGL_Utils p = (ppOpenGL_Utils)gglobal()->OpenGL_Utils.prv;

	switch (p->whichMode) {
	case GL_PROJECTION: p->currentMatrix = *PopMat(GL_PROJECTION, &p->projectionviewTOS, p->FW_ProjectionView); break;
	case GL_MODELVIEW:  p->currentMatrix = *PopMat(GL_MODELVIEW, &p->modelviewTOS, p->FW_ModelView); break;
	case GL_TEXTURE:   p->currentMatrix = *PopMat(GL_TEXTURE, &p->textureviewTOS, p->FW_TextureView); break;

	default: printf ("wrong mode in popMatrix\n");
	}

 	FW_GL_LOADMATRIX(p->currentMatrix);
}
//void fw_glPopMatrix(void) {
//	ppOpenGL_Utils p = (ppOpenGL_Utils)gglobal()->OpenGL_Utils.prv;
//
//	switch (p->whichMode) {
//		POPMAT(GL_PROJECTION, p->projectionviewTOS, p->FW_ProjectionView)
//			POPMAT(GL_MODELVIEW, p->modelviewTOS, p->FW_ModelView)
//			POPMAT(GL_TEXTURE, p->textureviewTOS, p->FW_TextureView)
//	default:printf("wrong mode in popMatrix\n");
//	}
//
//	FW_GL_LOADMATRIX(p->currentMatrix);
//}
//#undef POPMAT


void fw_glTranslated(GLDOUBLE x, GLDOUBLE y, GLDOUBLE z) {
	ppOpenGL_Utils p = (ppOpenGL_Utils)gglobal()->OpenGL_Utils.prv;

	//printf ("fw_glTranslated %lf %lf %lf\n",x,y,z);
	//printf ("translated, currentMatrix %p\n",p->currentMatrix);

	p->currentMatrix[12] = p->currentMatrix[0] * x + p->currentMatrix[4] * y + p->currentMatrix[8]  * z + p->currentMatrix[12];
	p->currentMatrix[13] = p->currentMatrix[1] * x + p->currentMatrix[5] * y + p->currentMatrix[9]  * z + p->currentMatrix[13];
	p->currentMatrix[14] = p->currentMatrix[2] * x + p->currentMatrix[6] * y + p->currentMatrix[10] * z + p->currentMatrix[14];
	p->currentMatrix[15] = p->currentMatrix[3] * x + p->currentMatrix[7] * y + p->currentMatrix[11] * z + p->currentMatrix[15];

 	FW_GL_LOADMATRIX(p->currentMatrix);
}

void fw_glTranslatef(float x, float y, float z) {
	ppOpenGL_Utils p = (ppOpenGL_Utils)gglobal()->OpenGL_Utils.prv;

	//printf ("fw_glTranslatef %f %f %f\n",x,y,z);
	p->currentMatrix[12] = p->currentMatrix[0] * x + p->currentMatrix[4] * y + p->currentMatrix[8]  * z + p->currentMatrix[12];
	p->currentMatrix[13] = p->currentMatrix[1] * x + p->currentMatrix[5] * y + p->currentMatrix[9]  * z + p->currentMatrix[13];
	p->currentMatrix[14] = p->currentMatrix[2] * x + p->currentMatrix[6] * y + p->currentMatrix[10] * z + p->currentMatrix[14];
	p->currentMatrix[15] = p->currentMatrix[3] * x + p->currentMatrix[7] * y + p->currentMatrix[11] * z + p->currentMatrix[15];

	FW_GL_LOADMATRIX(p->currentMatrix);
}

/* perform rotation, assuming that the angle is in radians. */
void fw_glRotateRad (GLDOUBLE angle, GLDOUBLE x, GLDOUBLE y, GLDOUBLE z) {
	MATRIX4 myMat;
	GLDOUBLE mag;
	ppOpenGL_Utils p = (ppOpenGL_Utils)gglobal()->OpenGL_Utils.prv;

	//printf ("fw_glRotateRad %lf %lf %lf %lf modTOS %d projTOS %d\n",angle,x,y,z,p->modelviewTOS,p->projectionviewTOS);
	//printmatrix2(p->currentMatrix,"in rad");
	loadIdentityMatrix (myMat);

	/* FIXME - any way we can ensure that the angles are normalized? */
	mag =  x*x + y*y + z*z;

	/* bounds check - the axis is invalid. */
	if (APPROX(mag,0.00)) {
		return;
	}

	/* bounds check - angle is zero, no rotation happening here */
	if (APPROX(angle,0.0)) {
		return;
	}

	if (!APPROX(mag,1.0)) {
		struct point_XYZ in; struct point_XYZ out;
		in.x = x; in.y = y, in.z = z;
		vecnormal(&out,&in);
		x = out.x; y = out.y; z = out.z;
	}
	//printf ("rad, normalized axis %lf %lf %lf\n",x,y,z);


	matrotate(myMat,angle,x,y,z);

	//printmatrix2 (myMat, "rotation matrix");
	matmultiplyAFFINE(p->currentMatrix,myMat,p->currentMatrix);

	//printmatrix2 (p->currentMatrix,"currentMatrix after rotate");

	FW_GL_LOADMATRIX(p->currentMatrix);
}

/* perform the rotation, assuming that the angle is in degrees */
void fw_glRotated (GLDOUBLE angle, GLDOUBLE x, GLDOUBLE y, GLDOUBLE z) {
	MATRIX4 myMat;
	GLDOUBLE mag;
	GLDOUBLE radAng;
	ppOpenGL_Utils p = (ppOpenGL_Utils)gglobal()->OpenGL_Utils.prv;

	/* convert angle from degrees to radians */
	/* FIXME - must try and make up a rotate-radians call to get rid of these incessant conversions */
	radAng = angle * 3.1415926536/ 180.0;

	loadIdentityMatrix (myMat);

	/* FIXME - any way we can ensure that the angles are normalized? */
	mag =  x*x + y*y + z*z;

	/* bounds check - the axis is invalid. */
	if (APPROX(mag,0.00)) {
		return;
	}

	/* bounds check - angle is zero, no rotation happening here */
	if (APPROX(angle,0.0)) {
		return;
	}

	if (!APPROX(mag,1.0)) {
		struct point_XYZ in; struct point_XYZ out;
		in.x = x; in.y = y, in.z = z;
		vecnormal(&out,&in);
		x = out.x; y = out.y; z = out.z;
	}


	/* are the axis close to zero? */
	if (mag < 0.001) {
		return;
	}
	matrotate(myMat,radAng,x,y,z);
	matmultiplyAFFINE(p->currentMatrix,p->currentMatrix,myMat);

	FW_GL_LOADMATRIX(p->currentMatrix);
}

void fw_glRotatef (float a, float x, float y, float z) {
	fw_glRotated((GLDOUBLE)a, (GLDOUBLE)x, (GLDOUBLE)y, (GLDOUBLE)z);
}

void fw_glScaled (GLDOUBLE x, GLDOUBLE y, GLDOUBLE z) {
	ppOpenGL_Utils p = (ppOpenGL_Utils)gglobal()->OpenGL_Utils.prv;

//	printf ("glScaled(%5.4lf %5.4lf %5.4lf)\n",x,y,z);

	p->currentMatrix[0] *= x;   p->currentMatrix[4] *= y;   p->currentMatrix[8]  *= z;
	p->currentMatrix[1] *= x;   p->currentMatrix[5] *= y;   p->currentMatrix[9]  *= z;
	p->currentMatrix[2] *= x;   p->currentMatrix[6] *= y;   p->currentMatrix[10] *= z;
	p->currentMatrix[3] *= x;   p->currentMatrix[7] *= y;   p->currentMatrix[11] *= z;

	FW_GL_LOADMATRIX(p->currentMatrix);
}

void fw_glScalef (float x, float y, float z) {
	ppOpenGL_Utils p = (ppOpenGL_Utils)gglobal()->OpenGL_Utils.prv;

//      printf ("glScalef(%5.4f %5.4f %5.4f)\n",x,y,z);

        p->currentMatrix[0] *= x;   p->currentMatrix[4] *= y;   p->currentMatrix[8]  *= z;
        p->currentMatrix[1] *= x;   p->currentMatrix[5] *= y;   p->currentMatrix[9]  *= z;
        p->currentMatrix[2] *= x;   p->currentMatrix[6] *= y;   p->currentMatrix[10] *= z;
        p->currentMatrix[3] *= x;   p->currentMatrix[7] *= y;   p->currentMatrix[11] *= z;

        FW_GL_LOADMATRIX(p->currentMatrix);
}


void fw_glGetDoublev (int ty, GLDOUBLE *mat) {
	GLDOUBLE *dp;
	ppOpenGL_Utils p = (ppOpenGL_Utils)gglobal()->OpenGL_Utils.prv;

/*
	switch (ty) {
		case GL_PROJECTION_MATRIX: printf ("getDoublev(GL_PROJECTION_MATRIX)\n"); break;
		case GL_MODELVIEW_MATRIX: printf ("getDoublev(GL_MODELVIEW_MATRIX)\n"); break;
		case GL_TEXTURE_MATRIX: printf ("getDoublev(GL_TEXTURE_MATRIX)\n"); break;
	}
*/

	switch (ty) {
		case GL_PROJECTION_MATRIX: dp = p->FW_ProjectionView[p->projectionviewTOS]; break;
		case GL_MODELVIEW_MATRIX: dp = p->FW_ModelView[p->modelviewTOS]; break;
		case GL_TEXTURE_MATRIX: dp = p->FW_TextureView[p->textureviewTOS]; break;
		default: {
			loadIdentityMatrix(mat);
		printf ("invalid mode sent in it is %d, expected one of %d %d %d\n",ty,GL_PROJECTION_MATRIX,GL_MODELVIEW_MATRIX,GL_TEXTURE_MATRIX);
			return;}
	}
	memcpy((void *)mat, (void *) dp, sizeof (GLDOUBLE) * MATRIX_SIZE);
}

void fw_glSetDoublev (int ty, GLDOUBLE *mat) {
	GLDOUBLE *dp;
	ppOpenGL_Utils p = (ppOpenGL_Utils)gglobal()->OpenGL_Utils.prv;

/*
	switch (ty) {
		case GL_PROJECTION_MATRIX: printf ("getDoublev(GL_PROJECTION_MATRIX)\n"); break;
		case GL_MODELVIEW_MATRIX: printf ("getDoublev(GL_MODELVIEW_MATRIX)\n"); break;
		case GL_TEXTURE_MATRIX: printf ("getDoublev(GL_TEXTURE_MATRIX)\n"); break;
	}
*/

	switch (ty) {
		case GL_PROJECTION_MATRIX: dp = p->FW_ProjectionView[p->projectionviewTOS]; break;
		case GL_MODELVIEW_MATRIX: dp = p->FW_ModelView[p->modelviewTOS]; break;
		case GL_TEXTURE_MATRIX: dp = p->FW_TextureView[p->textureviewTOS]; break;
		default: {
		printf ("invalid mode sent in it is %d, expected one of %d %d %d\n",ty,GL_PROJECTION_MATRIX,GL_MODELVIEW_MATRIX,GL_TEXTURE_MATRIX);
			return;}
	}
	memcpy((void *) dp, (void *)mat, sizeof (GLDOUBLE) * MATRIX_SIZE);
}


/* for Sarah's front end - should be removed sometime... */
void kill_rendering() {
/* printf ("kill_rendering called...\n"); */
}


/* if we have a ReplaceWorld style command, we have to remove the old world. */
/* NOTE: There are 2 kinds of of replaceworld commands - sometimes we have a URL
   (eg, from an Anchor) and sometimes we have a list of nodes (from a Javascript
   replaceWorld, for instance). The URL ones can really replaceWorld, the node
   ones, really, it's just replace the rootNode children, as WE DO NOT KNOW
   what the user has programmed, and what nodes are (re) used in the Scene Graph */

void killNodes();

void kill_oldWorld(int kill_EAI, int kill_JavaScript, char *file, int line) {
	int i;
	struct X3D_Node* rootnode;
	#ifndef AQUA
        char mystring[20];
	#endif
	struct VRMLParser *globalParser = (struct VRMLParser *)gglobal()->CParse.globalParser;
    //printf ("kill_oldWorld called...\n");


#ifdef VERBOSE
	printf ("kill 1 myThread %u displayThread %u\n",pthread_self(), gglobal()->threads.DispThrd);
#ifdef _MSC_VER
	if (pthread_self().p != gglobal()->threads.DispThrd.p ) {
#else
	if (pthread_self() != gglobal()->threads.DispThrd) {
#endif
		ConsoleMessage ("kill_oldWorld must run in the displayThread called at %s:%d\n",file,line);
		return;
	}
#endif

	/* get rid of sensor events */
	resetSensorEvents();


	/* make the root_res equal NULL - this throws away all old resource info */
	/*
		if (gglobal()->resources.root_res != NULL) {
			printf ("root_res %p has the following...\n",gglobal()->resources.root_res);
			resource_dump(gglobal()->resources.root_res);
		}else {printf ("root_res is null, no need to dump\n");}
	*/
	resource_tree_destroy();  //dug9 sep2,2013 added this call. just comment out if giving trouble before a release


	gglobal()->resources.root_res = NULL;





    /* mark all rootNode children for Dispose */
	rootnode = rootNode();
    if (rootnode != NULL) {
		if(usingBrotos()>1 && rootnode->_nodeType == NODE_Proto){
			unload_broto(X3D_PROTO(rootnode));
		}else{
			struct Multi_Node *children, *sortedChildren;

			if(usingBrotos()>1) {
				children = &X3D_PROTO(rootNode())->__children;
				sortedChildren = &X3D_PROTO(rootNode())->_sortedChildren;
			}else{
				children = &X3D_GROUP(rootNode())->children;
				sortedChildren = &X3D_GROUP(rootNode())->_sortedChildren;
			}
			//children = childrenField(rootNode());
			if (children->n != 0) {
				for (i=0; i<children->n; i++) {
					markForDispose(children->p[i], TRUE);
				}
			}
			//if (sortedChildren->n != 0) {
			//    for (i=0; i<sortedChildren->n; i++) {
			//        markForDispose(sortedChildren->p[i], TRUE);
			//    }
			//}


			/* stop rendering */
			sortedChildren->n = 0;
			children->n = 0;
		}
    }

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

        /* any user defined Shader nodes - ComposedShader, PackagedShader, ProgramShader?? */
        kill_userDefinedShaders();

	/* free textures */
/*
	kill_openGLTextures();
*/

	/* free scripts */
	#ifdef HAVE_JAVASCRIPT
	kill_javascript();
	#endif

#if !defined(EXCLUDE_EAI)
	/* free EAI */
	if (kill_EAI) {
	       	/* shutdown_EAI(); */
		fwlio_RxTx_control(CHANNEL_EAI, RxTx_STOP) ;
	}
#endif

	#ifndef AQUA
		sprintf (mystring, "QUIT");
		Sound_toserver(mystring);
	#endif


	/* reset any VRML Parser data */
	if (globalParser != NULL) {
		parser_destroyData(globalParser);
		//globalParser = NULL;
		//moved to CParse_clear gglobal()->CParse.globalParser = NULL;
	}

	kill_X3DDefs();

	/* tell statusbar that we have none */
	//viewer_default();
	setMenuStatus("NONE");
}
void unload_globalParser() {
	// unload any string tables, and signal to any replacworld scene that it needs a new parser+lexer struct
	struct VRMLParser *globalParser = (struct VRMLParser *)gglobal()->CParse.globalParser;
	if(globalParser){
		parser_destroyData(globalParser); //destroys lexer data too
		FREE_IF_NZ(globalParser->lexer);
	}
	FREE_IF_NZ(globalParser);
	gglobal()->CParse.globalParser = NULL; //set to null to trigger a fresh createParser on replaceworld
}
void unload_libraryscenes();
void reset_Browser(){
	// erase old world but keep gglobal in good shape, ie everything in _init() functions still good
	// -gglobal is erased elsewhere in finalizeRenderSceneUpdateScene()
	// also don't erase browser metadata key,value pairs, which could be avatar state to be 
	// carried over between room-scenes in multi-scene game
	struct X3D_Node *rootnode = rootNode();
    if (rootnode != NULL) {
		if(usingBrotos()>1 && rootnode->_nodeType == NODE_Proto){
			unload_broto(X3D_PROTO(rootnode)); //we still want a rootnode: empty and waiting for parsing (destroy in finalizeRenderSceneUpdateScene only on exit)
			unload_globalParser();
			resource_tree_destroy();
			unload_libraryscenes(); //debate: should proto libraryscenes hang around in case the same protos are used in a different, anchored-to scene?
			kill_javascript();
		}else{
			kill_oldWorld(TRUE,TRUE,__FILE__,__LINE__);
		}
	}
}


#if  defined (_ANDROID)

/* Android wants the OpenGL system to re-create assets like textures on onSurfaceCreated call */
void fwl_Android_reloadAssets(void) {
        int tc;
	struct X3D_Node *node;
	ppOpenGL_Utils p;

	p = (ppOpenGL_Utils)gglobal()->OpenGL_Utils.prv;

	/* reset the RenderFuncs cache */
	resetGlobalShader();

	//ConsoleMessage("fwl_Android_reloadAssets called");

	//ConsoleMessage ("fwl_Android_reloadAssets - reloading shader code");
	fwl_initialize_GL();

	//ConsoleMessage("fwl_Android_reloadAssets - reload the current active shaders");

	if (p->linearNodeTable != NULL) {
	        LOCK_MEMORYTABLE
		for (tc=0; tc<vectorSize(p->linearNodeTable); tc++){
			node = vector_get(struct X3D_Node *,p->linearNodeTable,tc);

			//ConsoleMessage ("rla, node %p\n",node);

			if (node!=NULL) {

				/* tell each node to update itself */
				node->_change ++;
				switch (node->_nodeType) {
					case NODE_Sphere: {
						struct X3D_Sphere *me = (struct X3D_Sphere *)node;
						//ConsoleMessage ("Sphere - zeroing VBO");
						me->_sideVBO = 0;
						me->__SphereIndxVBO = 0;
						FREE_IF_NZ(me->__points.p);
						me->__points.p = NULL;
						me->__points.n = 0;
						node->_change ++;
						break;

					}
					case NODE_Cone: {
						struct X3D_Cone *me = (struct X3D_Cone *)node;
						me->__coneVBO = 0;
						node->_change ++;
						break;
					}
					case NODE_Cylinder: {
						struct X3D_Cylinder *me = (struct X3D_Cylinder *)node;
						me->__cylinderVBO = 0;
						node->_change ++;
						break;
					}
					case NODE_Background: {
						struct X3D_Background *me = (struct X3D_Background *)node;
						//ConsoleMessage ("Background - zeroing VBO");
						me->__VBO = 0;
						node->_change ++;
						break;
					}
					default: {
						struct X3D_PolyRep *pr = node->_intern;
						int i;

						//ConsoleMessage ("node Type %s, intern %p",stringNodeType(node->_nodeType),pr);

						// get rid of the PolyRep VBOs.
						if (pr!=NULL) {
							for (i=0; i<VBO_COUNT; i++) pr->VBO_buffers[i] = 0;
							pr->irep_change ++;
							node->_change ++;
						}
					}

				}
			}

	        }
	        UNLOCK_MEMORYTABLE
	}
}
#endif

/* for verifying that a memory pointer exists */
int checkNode(struct X3D_Node *node, char *fn, int line) {
	int tc;
	ppOpenGL_Utils p = (ppOpenGL_Utils)gglobal()->OpenGL_Utils.prv;

	if (node == NULL) {
		printf ("checkNode, node is NULL at %s %d\n",fn,line);
		return FALSE;
	}

	if (node == X3D_NODE(rootNode())) return FALSE;

	LOCK_MEMORYTABLE;

	for (tc=0; tc<vectorSize(p->linearNodeTable); tc++){
		if (vector_get(struct X3D_Node *,p->linearNodeTable,tc) == node) {
			if (node->referenceCount > 0) {
			UNLOCK_MEMORYTABLE;
			return TRUE;
			}
		}
	}



	//printf ("checkNode: did not find %p in memory table at i%s %d\n",node,fn,line);

	UNLOCK_MEMORYTABLE;

	return FALSE;
}


/*creating node table*/
static void createMemoryTable(){
	ppOpenGL_Utils p = (ppOpenGL_Utils)gglobal()->OpenGL_Utils.prv;

	p->linearNodeTable = newVector(struct X3D_Node*, 128);


}

/*keep track of node created*/
void registerX3DNode(struct X3D_Node * tmp){
	ppOpenGL_Utils p = (ppOpenGL_Utils)gglobal()->OpenGL_Utils.prv;
	int tc;
	bool filledHole = FALSE;

	LOCK_MEMORYTABLE

	if (p->linearNodeTable == NULL) {
		createMemoryTable();
	}

	// fill in a hole, or just tack it on the end?
	if (p->potentialHoleCount > 0) {
		for (tc=0; tc<vectorSize(p->linearNodeTable); tc++){
			if (!filledHole) {
				if (vector_get(struct X3D_Node *,p->linearNodeTable,tc) == NULL) {
					vector_set(struct X3D_Node *, p->linearNodeTable, tc, tmp);
					p->potentialHoleCount--;
					filledHole = TRUE;
//ConsoleMessage ("registerX3DNode, found a hole at %d, now phc %d for type %s",tc,p->potentialHoleCount,stringNodeType(tmp->_nodeType));
				}
			}
		}
	}


/*
if (filledHole) ConsoleMessage ("registerX3DNode, filled hole, now phc %d for type %s",p->potentialHoleCount,stringNodeType(tmp->_nodeType));
if (!filledHole) ConsoleMessage ("registerX3DNode, no hole, phc %d for type %s",p->potentialHoleCount,stringNodeType(tmp->_nodeType));
*/

	if (!filledHole) vector_pushBack(struct X3D_Node *, p->linearNodeTable, tmp);

	UNLOCK_MEMORYTABLE
}
int removeNodeFromVector(int iaction, struct Vector *v, struct X3D_Node *node);
void unRegisterX3DNode(struct X3D_Node * tmp){
	ppOpenGL_Utils p = (ppOpenGL_Utils)gglobal()->OpenGL_Utils.prv;
	LOCK_MEMORYTABLE
	if (p->linearNodeTable ) {
		removeNodeFromVector(1, p->linearNodeTable, tmp);
		//int tc;
		//for (tc=0; tc<vectorSize(p->linearNodeTable); tc++){
		//	struct X3D_Node *ns;
		//	ns = vector_get(struct X3D_Node *,p->linearNodeTable,tc);
		//	if(ns == tmp) {
		//		vector_set(struct X3D_Node *, p->linearNodeTable, tc, NULL);
		//		break;
		//	}
		//}
	}
	UNLOCK_MEMORYTABLE
}


/*We don't register the first node created for reload reason*/
void doNotRegisterThisNodeForDestroy(struct X3D_Node * nodePtr){
	int i;

	ppOpenGL_Utils p = (ppOpenGL_Utils)gglobal()->OpenGL_Utils.prv;

	LOCK_MEMORYTABLE
	/* find this node, and if found, just delete it from the memory table */

	for (i=0; i<vectorSize(p->linearNodeTable); i++) {
		if (vector_get(struct X3D_Node *,p->linearNodeTable,i) == nodePtr) {
			vector_set(struct X3D_Node *,p->linearNodeTable,i,NULL);
			p->potentialHoleCount++;
		}
	}

	UNLOCK_MEMORYTABLE
}



/* sort children - use bubble sort with early exit flag */
/* we use this for z buffer rendering; drawing scene in correct z-buffer order */
/* we ONLY sort if:
	1) the node has changed - this is the "needsCompiling" field;
	2) the number of children has changed - again, the "needsCompiling" flag should be set,
		but we enforce it;
	3) the first pass shows that nodes are out of order
*/

static void sortChildren (int line, struct Multi_Node *ch, struct Multi_Node *sortedCh, int sortForDistance) {
	int i,j;
	int nc;
	int noswitch;
	struct X3D_Node *a, *b, *c;


	/* Thread Race Warning: the (non-broto) parser can still be
	  adding children in its thread, while we are rendering in this thread.
	  We don't have mutex locks, so we are relying on 'atomic operations'
	  if it bombs in here check also REINITIALIZE_SORTED_NODES_FIELD macro which does the same thing
	*/
	nc = ch->n; //ATOMIC OP - saves the instantaneous size of ch, which may keep growing

	#ifdef VERBOSE
	printf ("sortChildren line %d nc %d ",line,nc);
		if (sortForDistance) printf ("sortForDistance ");
		printf ("\n");
	#endif //VERBOSE


	/* has this changed size? */
	if (nc != sortedCh->n) {
		FREE_IF_NZ(sortedCh->p); //Mar 11, 2014:
		sortedCh->p = MALLOC(void *, sizeof (struct X3DNode *) * nc);
		memcpy(sortedCh->p, ch->p, sizeof(struct X3DNode *) * nc); //ATOMIC-OP - ch->p gets realloced frequently, we need a snapshot which may be bigger than nc above
		sortedCh->n = nc;
	}

	#ifdef VERBOSE
	printf ("sortChildren start, %d, chptr %u\n",nc,ch);
	#endif

	/* do we care about rendering order? */

	if (!sortForDistance) return;
	if (nc < 2) return;
	/* simple, inefficient bubble sort */
	/* this is a fast sort when nodes are already sorted;
	may wish to go and "QuickSort" or so on, when nodes
	move around a lot. (Bubblesort is bad when nodes
	have to be totally reversed) */

	for(i=0; i<nc; i++) {
		noswitch = TRUE;
		for (j=(nc-1); j>i; j--) {
			/* printf ("comparing %d %d\n",i,j); */
			a = X3D_NODE(sortedCh->p[j-1]);
			b = X3D_NODE(sortedCh->p[j]);

			/* check to see if a child is NULL - if so, skip it */
			if (a && b) {
				if (a->_dist > b->_dist) {
					// printf ("sortChildren at %lf, have to switch %d %d dists %lf %lf\n",TickTime(),i,j,a->_dist, b->_dist); 
					c = a;
					sortedCh->p[j-1] = b;
					sortedCh->p[j] = c;
					noswitch = FALSE;

				}
			}
		}
		/* did we have a clean run? */
		if (noswitch) {
			break;
		}
	}

	#ifdef VERBOSE
	printf ("sortChildren returning.\n");
	for(i=0; i<nc; i++) {
		b = sortedCh->p[i];
		if (b)
			printf ("child %d %u %f %s",i,b,b->_dist,stringNodeType(b->_nodeType));
		else
			printf ("no child %d", i);
		b = ch->p[i];
		printf (" unsorted %u\n",b);
	}
	#endif
}

/* zero the Visibility flag in all nodes */
void zeroVisibilityFlag(void) {
	struct X3D_Node* node;
	int i;
	ppOpenGL_Utils p = (ppOpenGL_Utils)gglobal()->OpenGL_Utils.prv;

	/* do not bother doing this if the inputThread is active. */
	if (fwl_isinputThreadParsing()) return;
 	LOCK_MEMORYTABLE

	/* do we have GL_ARB_occlusion_query, or are we still parsing Textures? */
	if ((gglobal()->Frustum.OccFailed) || fwl_isTextureParsing()) {
		/* if we have textures still loading, display all the nodes, so that the textures actually
		   get put into OpenGL-land. If we are not texture parsing... */
		/* no, we do not have GL_ARB_occlusion_query, just tell every node that it has visible children
		   and hope that, sometime, the user gets a good computer graphics card */
		for (i=0; i<vectorSize(p->linearNodeTable); i++){
			node = vector_get(struct X3D_Node *,p->linearNodeTable,i);
			if (node != NULL) {
				node->_renderFlags = node->_renderFlags | VF_hasVisibleChildren;
			}
		}
	}
    else if (p->linearNodeTable)
    {
		/* we do... lets zero the hasVisibleChildren flag */
		for (i=0; i<vectorSize(p->linearNodeTable); i++){
			node = vector_get(struct X3D_Node *,p->linearNodeTable,i);

			if (node != NULL) {

			#ifdef OCCLUSIONVERBOSE
			if (((node->_renderFlags) & VF_hasVisibleChildren) != 0) {
			printf ("%lf, zeroVisibility - %d is a %s, flags %x\n",TickTime(), i,stringNodeType(node->_nodeType), (node->_renderFlags) & VF_hasVisibleChildren);
			}
			#endif

			node->_renderFlags = node->_renderFlags & (0xFFFF^VF_hasVisibleChildren);
			}

		}
	}

	UNLOCK_MEMORYTABLE
}

/* go through the linear list of nodes, and do "special things" for special nodes, like
   Sensitive nodes, Viewpoint nodes, ... */

#define CMD(TTT,YYY) \
	/* printf ("nt %s change %d ichange %d\n",stringNodeType(X3D_NODE(YYY)->_nodeType),X3D_NODE(YYY)->_change, X3D_NODE(YYY)->_ichange); */ \
	if (NODE_NEEDS_COMPILING) compile_Metadata##TTT((struct X3D_Metadata##TTT *) YYY)

#define BEGIN_NODE(thistype) case NODE_##thistype:
#define END_NODE break;

#define SIBLING_SENSITIVE(thistype) \
			/* make Sensitive */ \
			if (((struct X3D_##thistype *)node)->enabled) { \
				nParents = vectorSize((struct X3D_##thistype *)pnode->_parentVector); \
				parentVector = (((struct X3D_##thistype *)pnode)->_parentVector); \
			}

#define ANCHOR_SENSITIVE(thistype) \
			/* make THIS Sensitive - most nodes make the parents sensitive, Anchors have children...*/ \
			anchorPtr = (struct X3D_Anchor *)node;

#ifdef VIEWPOINT
#undef VIEWPOINT /* defined for the EAI,SAI, does not concern us uere */
#endif
#define VIEWPOINT(thistype) \
			setBindPtr = (int *)(((char*)(node))+offsetof (struct X3D_##thistype, set_bind)); \
			if ((*setBindPtr) == 100) {setBindPtr = NULL; } //else {printf ("OpenGL, BINDING %d\n",*setBindPtr);}/* already done */

#define CHILDREN_NODE(thistype) \
			addChildren = NULL; removeChildren = NULL; \
			offsetOfChildrenPtr = offsetof (struct X3D_##thistype, children); \
			if (((struct X3D_##thistype *)node)->addChildren.n > 0) { \
				addChildren = &((struct X3D_##thistype *)node)->addChildren; \
				childrenPtr = &((struct X3D_##thistype *)node)->children; \
			} \
			if (((struct X3D_##thistype *)node)->removeChildren.n > 0) { \
				removeChildren = &((struct X3D_##thistype *)node)->removeChildren; \
				childrenPtr = &((struct X3D_##thistype *)node)->children; \
			}

#define CHILDREN_SWITCH_NODE(thistype) \
			addChildren = NULL; removeChildren = NULL; \
			offsetOfChildrenPtr = offsetof (struct X3D_##thistype, choice); \
			if (((struct X3D_##thistype *)node)->addChildren.n > 0) { \
				addChildren = &((struct X3D_##thistype *)node)->addChildren; \
				childrenPtr = &((struct X3D_##thistype *)node)->choice; \
			} \
			if (((struct X3D_##thistype *)node)->removeChildren.n > 0) { \
				removeChildren = &((struct X3D_##thistype *)node)->removeChildren; \
				childrenPtr = &((struct X3D_##thistype *)node)->choice; \
			}

#define CHILDREN_LOD_NODE \
			addChildren = NULL; removeChildren = NULL; \
			offsetOfChildrenPtr = offsetof (struct X3D_LOD, children); \
			if (X3D_LODNODE(node)->addChildren.n > 0) { \
				addChildren = &X3D_LODNODE(node)->addChildren; \
				if (X3D_LODNODE(node)->__isX3D == 0) childrenPtr = &X3D_LODNODE(node)->level; \
				else childrenPtr = &X3D_LODNODE(node)->children; \
			} \
			if (X3D_LODNODE(node)->removeChildren.n > 0) { \
				removeChildren = &X3D_LODNODE(node)->removeChildren; \
				if (X3D_LODNODE(node)->__isX3D == 0) childrenPtr = &X3D_LODNODE(node)->level; \
				else childrenPtr = &X3D_LODNODE(node)->children; \
			}

#define CHILDREN_ANY_NODE(thistype,thischildren) \
			addChildren = NULL; removeChildren = NULL; \
			offsetOfChildrenPtr = offsetof (struct X3D_##thistype, thischildren); \
			if (((struct X3D_##thistype *)node)->addChildren.n > 0) { \
				addChildren = &((struct X3D_##thistype *)node)->addChildren; \
				childrenPtr = &((struct X3D_##thistype *)node)->thischildren; \
			} \
			if (((struct X3D_##thistype *)node)->removeChildren.n > 0) { \
				removeChildren = &((struct X3D_##thistype *)node)->removeChildren; \
				childrenPtr = &((struct X3D_##thistype *)node)->thischildren; \
			}


#define EVIN_AND_FIELD_SAME(thisfield, thistype) \
			if ((((struct X3D_##thistype *)node)->set_##thisfield.n) > 0) { \
				((struct X3D_##thistype *)node)->thisfield.n = 0; \
				FREE_IF_NZ (((struct X3D_##thistype *)node)->thisfield.p); \
				((struct X3D_##thistype *)node)->thisfield.p = ((struct X3D_##thistype *)node)->set_##thisfield.p; \
				((struct X3D_##thistype *)node)->thisfield.n = ((struct X3D_##thistype *)node)->set_##thisfield.n; \
				((struct X3D_##thistype *)node)->set_##thisfield.n = 0; \
				((struct X3D_##thistype *)node)->set_##thisfield.p = NULL; \
			}

/* just tell the parent (a grouping node) that there is a locally scoped light as a child */
/* do NOT send this up the scenegraph! */
#define LOCAL_LIGHT_PARENT_FLAG \
{ int i; \
	for (i = 0; i < vectorSize(pnode->_parentVector); i++) { \
		struct X3D_Node *n = vector_get(struct X3D_Node*, pnode->_parentVector, i); \
		if( n != 0 ) n->_renderFlags = n->_renderFlags | VF_localLight; \
	} \
}

#define  CHECK_MATERIAL_TRANSPARENCY \
if (((struct X3D_Material *)node)->transparency > 0.0001) { \
/* printf ("node %p MATERIAL HAS TRANSPARENCY of %f \n", node, ((struct X3D_Material *)node)->transparency); */ \
update_renderFlag(X3D_NODE(pnode),VF_Blend | VF_shouldSortChildren);\
gglobal()->RenderFuncs.have_transparency = TRUE; \
}

#define CHECK_FILL_PROPERTY_FILLED \
    if ((((struct X3D_FillProperties *)node)->_enabled == TRUE) && (((struct X3D_FillProperties *)node)->filled == FALSE)) { \
    /* printf ("node %p FillProperty filled FALSE\n",node); */ \
    update_renderFlag(X3D_NODE(pnode),VF_Blend | VF_shouldSortChildren);\
    gglobal()->RenderFuncs.have_transparency = TRUE; \
}

#define  CHECK_TWOSIDED_MATERIAL_TRANSPARENCY \
	if ((((struct X3D_TwoSidedMaterial *)node)->transparency > 0.0001)  || (((struct X3D_TwoSidedMaterial *)node)->backTransparency > 0.0001)){ \
		/* printf ("node %p TwoSidedMATERIAL HAS TRANSPARENCY of %f %f \n", node, ((struct X3D_TwoSidedMaterial *)node)->transparency,((struct X3D_TwoSidedMaterial *)node)->backTransparency);*/ \
		update_renderFlag(X3D_NODE(pnode),VF_Blend | VF_shouldSortChildren);\
		gglobal()->RenderFuncs.have_transparency = TRUE; \
	}

#define CHECK_IMAGETEXTURE_TRANSPARENCY \
	if (isTextureAlpha(((struct X3D_ImageTexture *)node)->__textureTableIndex)) { \
		/* printf ("node %d IMAGETEXTURE HAS TRANSPARENCY\n", node); */ \
		update_renderFlag(X3D_NODE(pnode),VF_Blend | VF_shouldSortChildren);\
		gglobal()->RenderFuncs.have_transparency = TRUE; \
	}

#define CHECK_PIXELTEXTURE_TRANSPARENCY \
	if (isTextureAlpha(((struct X3D_PixelTexture *)node)->__textureTableIndex)) { \
		/* printf ("node %d PIXELTEXTURE HAS TRANSPARENCY\n", node); */ \
		update_renderFlag(X3D_NODE(pnode),VF_Blend | VF_shouldSortChildren);\
		gglobal()->RenderFuncs.have_transparency = TRUE; \
	}

#define CHECK_MOVIETEXTURE_TRANSPARENCY \
	if (isTextureAlpha(((struct X3D_MovieTexture *)node)->__textureTableIndex)) { \
		/* printf ("node %d MOVIETEXTURE HAS TRANSPARENCY\n", node); */ \
		update_renderFlag(X3D_NODE(pnode),VF_Blend | VF_shouldSortChildren);\
		gglobal()->RenderFuncs.have_transparency = TRUE; \
	}

//dug9 dec 13 >>
//also used in renderfuncs.c setSensitive()
//definition: type node: its the node that represents the type of the node
//which for builtin nodes is the same as the node
// it's only different for protoInstances, where the type node is the first node in the ProtoBody
// or if the first node in the protoBody is a Proto, then its first node ad infinitum
// goal: get the type node, given the node
// then the Proto IS-A typeNode or can be used as one in some places
struct X3D_Node* getTypeNode(struct X3D_Node *node)
{
	struct X3D_Node* dnode;
	dnode = node; //for builtin types, the type node is just the node
	if(node){
		if(isProto(node))
		{
			if(node->_nodeType == NODE_Group)
			{
				struct X3D_Group *gpn = (struct X3D_Group*)node;
				if(gpn->FreeWRL__protoDef != INT_ID_UNDEFINED)
				{
					//the first node in a protobody determines its type
					if(gpn->children.n > 0)
						dnode = getTypeNode(gpn->children.p[0]);
					else
						dnode = NULL;
				}
			}
			else if(node->_nodeType == NODE_Proto)
			{
				struct X3D_Proto *pn = (struct X3D_Proto*)node;
				//if(pn->FreeWRL__protoDef != INT_ID_UNDEFINED)
				if(1) //some flag to say it's not the scene, but a protoInstance where only the first node is rendered - see isProto
				{
					//the first node in a protobody determines its type
					if(pn->__children.n > 0)
						dnode = getTypeNode(pn->__children.p[0]);
					else
						dnode = NULL;
				}
			}
		}
	}
	return dnode;
}
void printStatsNodes(){
	ppOpenGL_Utils p;
	ttglobal tg = gglobal();
	p = (ppOpenGL_Utils)tg->OpenGL_Utils.prv;

	ConsoleMessage("%25s %d\n","Nodes count", p->linearNodeTable->n);
}

int unRegisterX3DAnyNode(struct X3D_Node *node);

void killNodes()
{
	int i;
	struct X3D_Node* node;
	ppOpenGL_Utils p;
	ttglobal tg = gglobal();
	p = (ppOpenGL_Utils)tg->OpenGL_Utils.prv;

	for (i = 0; i < vectorSize(p->linearNodeTable); i++){
		node = vector_get(struct X3D_Node *, p->linearNodeTable, i);
		if (node != NULL) {
			if (node->referenceCount <= 0) {
				//ConsoleMessage ("%d ref %d\n",i,node->referenceCount);
				//killNode(i);
				unRegisterX3DAnyNode(node);
				FREE_IF_NZ(node);
				vector_set(struct X3D_Node *,p->linearNodeTable,i,NULL);
			}
			//else{
			//	printf("%d ", i);
			//}
		}
	}
}
//dug9 dec 13 <<
int needs_updating_Inline(struct X3D_Node *node);
void update_Inline(struct X3D_Inline *node);
// Dec 14, 2012 new proto IS-A version (see below for older version)
void startOfLoopNodeUpdates(void) {
	struct X3D_Node* node;
	struct X3D_Node* pnode;
	struct X3D_Anchor* anchorPtr;
	struct Vector *parentVector;
	int nParents;
	int i,j,k,foundbound;
	int* setBindPtr;

	struct Multi_Node *addChildren;
	struct Multi_Node *removeChildren;
	struct Multi_Node *childrenPtr;
	size_t offsetOfChildrenPtr;

	/* process one inline per loop; do it outside of the lock/unlock memory table */
	//struct Vector *loadInlines;
	ppOpenGL_Utils p;
	ttglobal tg = gglobal();
	p = (ppOpenGL_Utils)tg->OpenGL_Utils.prv;

	if (rootNode() == NULL) return; /* nothing to do, and we have not really started yet */

	/* initialization */
	addChildren = NULL;
	removeChildren = NULL;
	childrenPtr = NULL;
	parentVector = NULL;
	//loadInlines = NULL;
	offsetOfChildrenPtr = 0;

	/* assume that we do not have any sensitive nodes at all... */
	tg->Mainloop.HaveSensitive = FALSE;
	tg->RenderFuncs.have_transparency = FALSE;


	/* do not bother doing this if the inputparsing thread is active */
	if (fwl_isinputThreadParsing()) return;
	profile_start("loopnodeupdt");
	LOCK_MEMORYTABLE

	//printf ("\n******************************************\nstartOfLoopNodeUpdates\n");

	/* go through the node table, and zero any bits of interest */

	for (i=0; i<vectorSize(p->linearNodeTable); i++){
		node = vector_get(struct X3D_Node *,p->linearNodeTable,i);
		if (node != NULL) {
			if (node->referenceCount <= 0) {
				//unload_brotos -a new way to clean out an old scene of 2014- doesn't rely on reference counting 
				// when cleaning up a scene - it calls unregisterX3Dnode()
				//ConsoleMessage ("%d ref %d\n",i,node->referenceCount);
				//killNode(i);
				FREE_IF_NZ(node);
				vector_set(struct X3D_Node *,p->linearNodeTable,i,NULL);
			} else {
				/* turn OFF these flags */
				node->_renderFlags = node->_renderFlags & (0xFFFF^VF_Sensitive);
				node->_renderFlags = node->_renderFlags & (0xFFFF^VF_Viewpoint);
				node->_renderFlags = node->_renderFlags & (0xFFFF^VF_localLight);
				node->_renderFlags = node->_renderFlags & (0xFFFF^VF_globalLight);
				node->_renderFlags = node->_renderFlags & (0xFFFF^VF_Blend);
			}
		}
	}
	/* turn OFF these flags */
	{
		struct X3D_Node* rn = rootNode();
		rn->_renderFlags = rn->_renderFlags & (0xFFFF^VF_Sensitive);
		rn->_renderFlags = rn->_renderFlags & (0xFFFF^VF_Viewpoint);
		rn->_renderFlags = rn->_renderFlags & (0xFFFF^VF_localLight);
		rn->_renderFlags = rn->_renderFlags & (0xFFFF^VF_globalLight);
		rn->_renderFlags = rn->_renderFlags & (0xFFFF^VF_Blend);
	}

	/* sort the rootNode, if it is Not NULL */
	/* remember, the rootNode is not in the linearNodeTable, so we have to do this outside
	   of that loop */
	//if (rootNode() != NULL && !usingBrotos()) {
	if (rootNode() != NULL) {
		struct Multi_Node *children, *_sortedChildren;
		node = (struct X3D_Node*)rootNode();
		children = NULL;
		_sortedChildren = NULL;
		if(node->_nodeType == NODE_Proto){
			children = &X3D_PROTO(node)->__children;
			_sortedChildren = &X3D_PROTO(node)->_sortedChildren;
		}
		if(node->_nodeType == NODE_Group) {
			children = &X3D_GROUP(node)->children;
			_sortedChildren = &X3D_GROUP(node)->_sortedChildren;
		}
		sortChildren (__LINE__,children, _sortedChildren,rootNode()->_renderFlags & VF_shouldSortChildren);
		rootNode()->_renderFlags=rootNode()->_renderFlags & (0xFFFF^VF_shouldSortChildren);
		if(node->_nodeType == NODE_Proto){
			//CHILDREN_NODE(Proto)
			/* DRracer/t85.wrl has 'children' user fields on protos. This works with other browsers.
				But not freewrl. Unless I hide the children field as _children. Then it works.
			*/
			addChildren = NULL; removeChildren = NULL; 
			offsetOfChildrenPtr = offsetof (struct X3D_Proto, __children); 
			if (((struct X3D_Proto *)node)->addChildren.n > 0) { 
				addChildren = &((struct X3D_Proto *)node)->addChildren; 
				childrenPtr = &((struct X3D_Proto *)node)->__children; 
			} 
			if (((struct X3D_Proto *)node)->removeChildren.n > 0) { 
				removeChildren = &((struct X3D_Proto *)node)->removeChildren; 
				childrenPtr = &((struct X3D_Proto *)node)->__children; 
			}

		}else{
			CHILDREN_NODE(Group)
		}
		/* this node possibly has to do add/remove children? */
		if (childrenPtr != NULL) {
			if (addChildren != NULL) {
				AddRemoveChildren(node,childrenPtr,(struct X3D_Node * *) addChildren->p,addChildren->n,1,__FILE__,__LINE__);
				addChildren->n=0;
			}
			if (removeChildren != NULL) {
				AddRemoveChildren(node,childrenPtr,(struct X3D_Node * *) removeChildren->p,removeChildren->n,2,__FILE__,__LINE__);
				removeChildren->n=0;
			}
			/* printf ("OpenGL, marking children changed\n"); */
			MARK_EVENT(node,offsetOfChildrenPtr);
			childrenPtr = NULL;
		}
	}

	/* go through the list of nodes, and "work" on any that need work */
	nParents = 0;
	setBindPtr = NULL;
	anchorPtr = NULL;


	for (i=0; i<vectorSize(p->linearNodeTable); i++){
		node = vector_get(struct X3D_Node *,p->linearNodeTable,i);
		if (node != NULL)
		if (node->referenceCount > 0) {
			pnode = node;
			node = getTypeNode(node); //+ dug9 dec 13
			if(node == NULL && pnode != NULL)  //+ dug9 sept 2014
				if(pnode->_nodeType == NODE_Proto){
					UNLOCK_MEMORYTABLE
					/*dug9 sept 2014: I put load_EPI (externProtoInstance) here because I designed it like Inline
						where we check and give a time slice to loading when we visit the node instance during render().
						But that doesn't work for EPIs which have no concrete NODE type, nor VF_ flag
						so render_hier/render() never visits them until they are loaded and we can
						see the concrete type of their first node, and perculate VF_ flags up to the EPI
						There were other options such as event queue, or following a cascade of protoInstance arrays
						down the context heirarchy, or tinkering with the VF_ flag visitation rules in render()
						or inventing a VF_Proto flag, or expanding the role of the resource thread to do more work 
						and keep a list of subscribers with the EPD (externProtoDeclare).
						And any of those might work too. This was just convenient/easy/quick 
						for me, so feel free to move it.
					*/
					load_externProtoInstance(X3D_PROTO(pnode));
					LOCK_MEMORYTABLE
					node = getTypeNode(pnode);
				}
			if (node != NULL)
			//switch (node->_nodeType) { //- dug9 dec 13
			switch (node->_nodeType) { //+ dug9 dec 13
				BEGIN_NODE(Shape)
					/* send along a "look at me" flag if we are visible, or we should look again */
					if ((X3D_SHAPE(node)->__occludeCheckCount <=0) ||
							(X3D_SHAPE(node)->__visible)) {
						update_renderFlag (X3D_NODE(pnode),VF_hasVisibleChildren);
						/* printf ("shape occludecounter, pushing visiblechildren flags\n");  */

					}
					X3D_SHAPE(node)->__occludeCheckCount--;
					/* printf ("shape occludecounter %d\n",X3D_SHAPE(node)->__occludeCheckCount); */
				END_NODE

				/* Lights. DirectionalLights are "scope relative", PointLights and
				   SpotLights are transformed */

				BEGIN_NODE(DirectionalLight)
					if (X3D_DIRECTIONALLIGHT(node)->on) {
						if (X3D_DIRECTIONALLIGHT(node)->global)
							update_renderFlag(pnode,VF_globalLight);
						else
							LOCAL_LIGHT_PARENT_FLAG
					}
				END_NODE
				BEGIN_NODE(SpotLight)
					if (X3D_SPOTLIGHT(node)->on) {
						if (X3D_SPOTLIGHT(node)->global)
							update_renderFlag(pnode,VF_globalLight);
						else
							LOCAL_LIGHT_PARENT_FLAG
					}
				END_NODE
				BEGIN_NODE(PointLight)
					if (X3D_POINTLIGHT(node)->on) {
						if (X3D_POINTLIGHT(node)->global)
							update_renderFlag(pnode,VF_globalLight);
						else
							LOCAL_LIGHT_PARENT_FLAG
					}
				END_NODE


				/* some nodes, like Extrusions, have "set_" fields same as normal internal fields,
				   eg, "set_spine" and "spine". Here we just copy the fields over, and remove the
				   "set_" fields. This works for MF* fields ONLY */
				BEGIN_NODE(IndexedLineSet)
					EVIN_AND_FIELD_SAME(colorIndex,IndexedLineSet)
					EVIN_AND_FIELD_SAME(coordIndex,IndexedLineSet)
				END_NODE

				BEGIN_NODE(IndexedTriangleFanSet)
					EVIN_AND_FIELD_SAME(index,IndexedTriangleFanSet)
				END_NODE
				BEGIN_NODE(IndexedTriangleSet)
					EVIN_AND_FIELD_SAME(index,IndexedTriangleSet)
				END_NODE
				BEGIN_NODE(IndexedTriangleStripSet)
					EVIN_AND_FIELD_SAME(index,IndexedTriangleStripSet)
				END_NODE
				BEGIN_NODE(GeoElevationGrid)
					EVIN_AND_FIELD_SAME(height,GeoElevationGrid)
				END_NODE
				BEGIN_NODE(ElevationGrid)
					EVIN_AND_FIELD_SAME(height,ElevationGrid)
				END_NODE
				BEGIN_NODE(Extrusion)
					EVIN_AND_FIELD_SAME(crossSection,Extrusion)
					EVIN_AND_FIELD_SAME(orientation,Extrusion)
					EVIN_AND_FIELD_SAME(scale,Extrusion)
					EVIN_AND_FIELD_SAME(spine,Extrusion)
				END_NODE
				BEGIN_NODE(IndexedFaceSet)
					EVIN_AND_FIELD_SAME(colorIndex,IndexedFaceSet)
					EVIN_AND_FIELD_SAME(coordIndex,IndexedFaceSet)
					EVIN_AND_FIELD_SAME(normalIndex,IndexedFaceSet)
					EVIN_AND_FIELD_SAME(texCoordIndex,IndexedFaceSet)
				END_NODE
/* GeoViewpoint works differently than other nodes - see compile_GeoViewpoint for manipulation of these fields
				BEGIN_NODE(GeoViewpoint)
					EVIN_AND_FIELD_SAME(orientation,GeoViewpoint)
					EVIN_AND_FIELD_SAME(position,GeoViewpoint)
				END_NODE
*/

				/* get ready to mark these nodes as Mouse Sensitive */
				BEGIN_NODE(LineSensor) SIBLING_SENSITIVE(LineSensor) END_NODE
				BEGIN_NODE(PlaneSensor) SIBLING_SENSITIVE(PlaneSensor) END_NODE
				BEGIN_NODE(SphereSensor) SIBLING_SENSITIVE(SphereSensor) END_NODE
				BEGIN_NODE(CylinderSensor) SIBLING_SENSITIVE(CylinderSensor) END_NODE
				BEGIN_NODE(TouchSensor) SIBLING_SENSITIVE(TouchSensor) END_NODE
				BEGIN_NODE(GeoTouchSensor) SIBLING_SENSITIVE(GeoTouchSensor) END_NODE

				/* Anchor is Mouse Sensitive, AND has Children nodes */
				BEGIN_NODE(Anchor)
					propagateExtent(X3D_NODE(node));
					ANCHOR_SENSITIVE(Anchor)
					CHILDREN_NODE(Anchor)
				END_NODE

				BEGIN_NODE(CADFace)
					// the attached Shape node will do the occlusion testing, if enabled.
					update_renderFlag (X3D_NODE(pnode),VF_hasVisibleChildren);
				END_NODE

				BEGIN_NODE(CADLayer)
					propagateExtent(X3D_NODE(node));
					CHILDREN_NODE(Switch)
				END_NODE


				BEGIN_NODE(CADPart)
					sortChildren (__LINE__,&X3D_CADPART(node)->children,&X3D_CADPART(node)->_sortedChildren,pnode->_renderFlags & VF_shouldSortChildren);
					TURN_OFF_SHOULDSORTCHILDREN
					propagateExtent(X3D_NODE(node));
					CHILDREN_NODE(CADPart)
				END_NODE


				BEGIN_NODE(CADAssembly)
					sortChildren (__LINE__,&X3D_CADASSEMBLY(node)->children,&X3D_CADASSEMBLY(node)->_sortedChildren,pnode->_renderFlags & VF_shouldSortChildren);
					TURN_OFF_SHOULDSORTCHILDREN
					propagateExtent(X3D_NODE(node));
					CHILDREN_NODE(CADAssembly)
				END_NODE

				/* maybe this is the current Viewpoint? */
				BEGIN_NODE(Viewpoint) VIEWPOINT(Viewpoint) END_NODE
				BEGIN_NODE(OrthoViewpoint) VIEWPOINT(OrthoViewpoint) END_NODE
				BEGIN_NODE(GeoViewpoint) VIEWPOINT(GeoViewpoint) END_NODE

				BEGIN_NODE(NavigationInfo)
					render_NavigationInfo ((struct X3D_NavigationInfo *)node);
				END_NODE

				BEGIN_NODE(StaticGroup)
					/* we should probably not do this, but... */
					sortChildren (__LINE__,&X3D_STATICGROUP(node)->children,&X3D_STATICGROUP(node)->_sortedChildren,pnode->_renderFlags & VF_shouldSortChildren);
					TURN_OFF_SHOULDSORTCHILDREN
					propagateExtent(X3D_NODE(node));
				END_NODE


				/* does this one possibly have add/removeChildren? */
				BEGIN_NODE(Group)
					sortChildren (__LINE__,&X3D_GROUP(node)->children,&X3D_GROUP(node)->_sortedChildren,pnode->_renderFlags & VF_shouldSortChildren);
					TURN_OFF_SHOULDSORTCHILDREN
					propagateExtent(X3D_NODE(node));
					CHILDREN_NODE(Group)
				END_NODE

#ifdef DJTRACK_PICKSENSORS
				/* DJTRACK_PICKSENSORS */
				BEGIN_NODE(PickableGroup)
					sortChildren (__LINE__,&X3D_PICKABLEGROUP(node)->children,&X3D_PICKABLEGROUP(node)->_sortedChildren,pnode->_renderFlags & VF_shouldSortChildren);
					TURN_OFF_SHOULDSORTCHILDREN

					CHILDREN_NODE(PickableGroup)
				END_NODE
				/* PointPickSensor needs its own flag sent up the chain */
				BEGIN_NODE (PointPickSensor)
							if (X3D_POINTPICKSENSOR(node)->enabled) update_renderFlag(pnode,VF_PickingSensor);
				END_NODE

#endif

				BEGIN_NODE(Inline)
					sortChildren (__LINE__,&X3D_INLINE(node)->__children,&X3D_INLINE(node)->_sortedChildren,node->_renderFlags & VF_shouldSortChildren);
					TURN_OFF_SHOULDSORTCHILDREN
					propagateExtent(X3D_NODE(node));
					CHILDREN_ANY_NODE(Inline,__children)
				END_NODE

				BEGIN_NODE(Transform)
					sortChildren (__LINE__,&X3D_TRANSFORM(node)->children,&X3D_TRANSFORM(node)->_sortedChildren,pnode->_renderFlags & VF_shouldSortChildren);
					TURN_OFF_SHOULDSORTCHILDREN
					propagateExtent(X3D_NODE(node));
					CHILDREN_NODE(Transform)
				END_NODE



/*				BEGIN_NODE(NurbsGroup)
					CHILDREN_NODE(NurbsGroup)
				END_NODE
*/
				BEGIN_NODE(Contour2D)
					CHILDREN_NODE(Contour2D)
				END_NODE

				BEGIN_NODE(HAnimSite)
					CHILDREN_NODE(HAnimSite)
				END_NODE

				BEGIN_NODE(HAnimSegment)
					CHILDREN_NODE(HAnimSegment)
				END_NODE

				BEGIN_NODE(HAnimJoint)
					CHILDREN_NODE(HAnimJoint)
				END_NODE

				BEGIN_NODE(Billboard)
					propagateExtent(X3D_NODE(node));
					CHILDREN_NODE(Billboard)
					update_renderFlag(pnode,VF_Proximity);
				END_NODE

				BEGIN_NODE(Collision)
					propagateExtent(X3D_NODE(node));
					CHILDREN_NODE(Collision)
				END_NODE

				BEGIN_NODE(Switch)
					propagateExtent(X3D_NODE(node));
					CHILDREN_SWITCH_NODE(Switch)
				END_NODE

				BEGIN_NODE(LOD)
					propagateExtent(X3D_NODE(node));
					CHILDREN_LOD_NODE
							update_renderFlag(pnode,VF_Proximity);
				END_NODE

				/* Material - transparency of materials */
				BEGIN_NODE(Material) CHECK_MATERIAL_TRANSPARENCY END_NODE
				BEGIN_NODE(TwoSidedMaterial) CHECK_TWOSIDED_MATERIAL_TRANSPARENCY END_NODE
				BEGIN_NODE(FillProperties) CHECK_FILL_PROPERTY_FILLED END_NODE

				/* Textures - check transparency  */
				BEGIN_NODE(ImageTexture) CHECK_IMAGETEXTURE_TRANSPARENCY END_NODE
				BEGIN_NODE(PixelTexture) CHECK_PIXELTEXTURE_TRANSPARENCY END_NODE
				BEGIN_NODE(MovieTexture) CHECK_MOVIETEXTURE_TRANSPARENCY END_NODE


				/* Backgrounds, Fog */
				BEGIN_NODE(Background)
					if (X3D_BACKGROUND(node)->isBound) update_renderFlag (X3D_NODE(pnode),VF_hasVisibleChildren);
				END_NODE

				BEGIN_NODE(TextureBackground)
					if (X3D_TEXTUREBACKGROUND(node)->isBound) update_renderFlag (X3D_NODE(pnode),VF_hasVisibleChildren);
				END_NODE

				BEGIN_NODE(Fog)
					if (X3D_FOG(node)->isBound) update_renderFlag (X3D_NODE(pnode),VF_hasVisibleChildren);
				END_NODE


				/* VisibilitySensor needs its own flag sent up the chain */
				BEGIN_NODE (VisibilitySensor)
					/* send along a "look at me" flag if we are visible, or we should look again */
					if ((X3D_VISIBILITYSENSOR(node)->__occludeCheckCount <=0) ||
							(X3D_VISIBILITYSENSOR(node)->__visible)) {
						update_renderFlag (X3D_NODE(pnode),VF_hasVisibleChildren);
						/* printf ("vis occludecounter, pushing visiblechildren flags\n"); */

					}
					X3D_VISIBILITYSENSOR(node)->__occludeCheckCount--;
					/* VisibilitySensors have a transparent bounding box we have to render */

					update_renderFlag(pnode,VF_Blend & VF_shouldSortChildren);
				END_NODE

				/* ProximitySensor needs its own flag sent up the chain */
				BEGIN_NODE (ProximitySensor)
                			if (X3D_PROXIMITYSENSOR(node)->enabled) update_renderFlag(pnode,VF_Proximity);
				END_NODE

				/* GeoProximitySensor needs its own flag sent up the chain */
				BEGIN_NODE (GeoProximitySensor)
					if (X3D_GEOPROXIMITYSENSOR(node)->enabled) update_renderFlag(pnode,VF_Proximity);
				END_NODE

				/* GeoLOD needs its own flag sent up the chain, and it has to push extent up, too */
				BEGIN_NODE (GeoLOD)
					if (!(NODE_NEEDS_COMPILING)) {
						handle_GeoLODRange(X3D_GEOLOD(node));
					}
					/* update_renderFlag(pnode,VF_Proximity); */
					propagateExtent(X3D_NODE(node));
				END_NODE

				BEGIN_NODE (GeoTransform)
					sortChildren (__LINE__,&X3D_GEOTRANSFORM(node)->children,&X3D_GEOTRANSFORM(node)->_sortedChildren,pnode->_renderFlags & VF_shouldSortChildren);
					TURN_OFF_SHOULDSORTCHILDREN
					propagateExtent(X3D_NODE(node));
					CHILDREN_NODE(GeoTransform)
				END_NODE

				BEGIN_NODE (GeoLocation)
					sortChildren (__LINE__,&X3D_GEOLOCATION(node)->children,&X3D_GEOLOCATION(node)->_sortedChildren,pnode->_renderFlags & VF_shouldSortChildren);
					TURN_OFF_SHOULDSORTCHILDREN
					propagateExtent(X3D_NODE(node));
					CHILDREN_NODE(GeoLocation)
				END_NODE

				BEGIN_NODE(MetadataSFBool) CMD(SFBool,node); END_NODE
				BEGIN_NODE(MetadataSFFloat) CMD(SFFloat,node); END_NODE
				BEGIN_NODE(MetadataMFFloat) CMD(MFFloat,node); END_NODE
				BEGIN_NODE(MetadataSFRotation) CMD(SFRotation,node); END_NODE
				BEGIN_NODE(MetadataMFRotation) CMD(MFRotation,node); END_NODE
				BEGIN_NODE(MetadataSFVec3f) CMD(SFVec3f,node); END_NODE
				BEGIN_NODE(MetadataMFVec3f) CMD(MFVec3f,node); END_NODE
				BEGIN_NODE(MetadataMFBool) CMD(MFBool,node); END_NODE
				BEGIN_NODE(MetadataSFInt32) CMD(SFInt32,node); END_NODE
				BEGIN_NODE(MetadataMFInt32) CMD(MFInt32,node); END_NODE
				BEGIN_NODE(MetadataSFNode) CMD(SFNode,node); END_NODE
				BEGIN_NODE(MetadataMFNode) CMD(MFNode,node); END_NODE
				BEGIN_NODE(MetadataSFColor) CMD(SFColor,node); END_NODE
				BEGIN_NODE(MetadataMFColor) CMD(MFColor,node); END_NODE
				BEGIN_NODE(MetadataSFColorRGBA) CMD(SFColorRGBA,node); END_NODE
				BEGIN_NODE(MetadataMFColorRGBA) CMD(MFColorRGBA,node); END_NODE
				BEGIN_NODE(MetadataSFTime) CMD(SFTime,node); END_NODE
				BEGIN_NODE(MetadataMFTime) CMD(MFTime,node); END_NODE
				BEGIN_NODE(MetadataSFString) CMD(SFString,node); END_NODE
				BEGIN_NODE(MetadataMFString) CMD(MFString,node); END_NODE
				BEGIN_NODE(MetadataSFVec2f) CMD(SFVec2f,node); END_NODE
				BEGIN_NODE(MetadataMFVec2f) CMD(MFVec2f,node); END_NODE
				BEGIN_NODE(MetadataSFImage) CMD(SFImage,node); END_NODE
				BEGIN_NODE(MetadataSFVec3d) CMD(SFVec3d,node); END_NODE
				BEGIN_NODE(MetadataMFVec3d) CMD(MFVec3d,node); END_NODE
				BEGIN_NODE(MetadataSFDouble) CMD(SFDouble,node); END_NODE
				BEGIN_NODE(MetadataMFDouble) CMD(MFDouble,node); END_NODE
				BEGIN_NODE(MetadataSFMatrix3f) CMD(SFMatrix3f,node); END_NODE
				BEGIN_NODE(MetadataMFMatrix3f) CMD(MFMatrix3f,node); END_NODE
				BEGIN_NODE(MetadataSFMatrix3d) CMD(SFMatrix3d,node); END_NODE
				BEGIN_NODE(MetadataMFMatrix3d) CMD(MFMatrix3d,node); END_NODE
				BEGIN_NODE(MetadataSFMatrix4f) CMD(SFMatrix4f,node); END_NODE
				BEGIN_NODE(MetadataMFMatrix4f) CMD(MFMatrix4f,node); END_NODE
				BEGIN_NODE(MetadataSFMatrix4d) CMD(SFMatrix4d,node); END_NODE
				BEGIN_NODE(MetadataMFMatrix4d) CMD(MFMatrix4d,node); END_NODE
				BEGIN_NODE(MetadataSFVec2d) CMD(SFVec2d,node); END_NODE
				BEGIN_NODE(MetadataMFVec2d) CMD(MFVec2d,node); END_NODE
				BEGIN_NODE(MetadataSFVec4f) CMD(SFVec4f,node); END_NODE
				BEGIN_NODE(MetadataMFVec4f) CMD(MFVec4f,node); END_NODE
				BEGIN_NODE(MetadataSFVec4d) CMD(SFVec4d,node); END_NODE
				BEGIN_NODE(MetadataMFVec4d) CMD(MFVec4d,node); END_NODE
			}

		}


		/* now, act on this node  for Sensitive nodes. here we tell the PARENTS that they are sensitive */
		if (nParents != 0) {
			for (j=0; j<nParents; j++) {
				struct X3D_Node *n = vector_get(struct X3D_Node *, parentVector, j);
				n->_renderFlags = n->_renderFlags  | VF_Sensitive;
			}
			/* tell mainloop that we have to do a sensitive pass now */
			tg->Mainloop.HaveSensitive = TRUE;
			nParents = 0;
		}

		/* Anchor nodes are slightly different than sibling-sensitive nodes */
		if (anchorPtr != NULL) {
			anchorPtr->_renderFlags = anchorPtr->_renderFlags  | VF_Sensitive;

			/* tell mainloop that we have to do a sensitive pass now */
			tg->Mainloop.HaveSensitive = TRUE;
			anchorPtr = NULL;
		}

		/* do BINDING of Viewpoint Nodes */
		if (setBindPtr != NULL) {
			/* check the set_bind eventin to see if it is TRUE or FALSE */
			if (*setBindPtr < 100) {
				/* up_vector is reset after a bind */
				//if (*setBindPtr==1) reset_upvector();
				bind_node (node, getActiveBindableStacks(tg)->viewpoint);

				//dug9 added July 24, 2009: when you bind, it should set the
				//avatar to the newly bound viewpoint pose and forget any
				// cumulative avatar navigation from the last viewpoint parent
				if (node->_nodeType==NODE_Viewpoint) {
					struct X3D_Viewpoint* vp = (struct X3D_Viewpoint *) node;
					bind_Viewpoint(vp);
					setMenuStatusVP (vp->description->strptr);
				} else if (node->_nodeType==NODE_OrthoViewpoint) {
					struct X3D_OrthoViewpoint *ovp = (struct X3D_OrthoViewpoint *) node;
					bind_OrthoViewpoint(ovp);
					setMenuStatusVP (ovp->description->strptr);
				} else {
					struct X3D_GeoViewpoint *gvp = (struct X3D_GeoViewpoint *) node;
					bind_GeoViewpoint(gvp);
					setMenuStatusVP (gvp->description->strptr);
				}
			}
			setBindPtr = NULL;
		}

		/* this node possibly has to do add/remove children? */
		if (childrenPtr != NULL) {
			if (addChildren != NULL) {
				AddRemoveChildren(node,childrenPtr,(struct X3D_Node * *) addChildren->p,addChildren->n,1,__FILE__,__LINE__);
				addChildren->n=0;
			}
			if (removeChildren != NULL) {
				AddRemoveChildren(node,childrenPtr,(struct X3D_Node * *) removeChildren->p,removeChildren->n,2,__FILE__,__LINE__);
				removeChildren->n=0;
			}
			/* printf ("OpenGL, marking children changed\n"); */
			MARK_EVENT(node,offsetOfChildrenPtr);
			childrenPtr = NULL;
		}
	}



	UNLOCK_MEMORYTABLE

	/* now, we can go and tell the grouping nodes which ones are the lucky ones that contain the current Viewpoint node */
	foundbound = FALSE;
	for(k=0;k<vectorSize(tg->Bindable.bstacks);k++){
		bindablestack *bstack = vector_get(bindablestack*,tg->Bindable.bstacks,k);
		//if (vectorSize(getActiveBindableStacks(tg)->viewpoint) > 0) {
		if( vectorSize(bstack->viewpoint) > 0){
			//ConsoleMessage ("going to updateRF on viewpoint, stack is %d in size\n", vectorSize(tg->Bindable.viewpoint_stack));

			//struct X3D_Node *boundvp = vector_back(struct X3D_Node*,getActiveBindableStacks(tg)->viewpoint);
			struct X3D_Node *boundvp = vector_back(struct X3D_Node*,bstack->viewpoint);
			update_renderFlag(boundvp, VF_Viewpoint);
			calculateNearFarplanes(boundvp, bstack->layerId);
			//update_renderFlag(vector_back(struct X3D_Node*,
			//	tg->Bindable.viewpoint_stack), VF_Viewpoint);
			//calculateNearFarplanes(vector_back(struct X3D_Node*, tg->Bindable.viewpoint_stack));
		}
	}
	if(!foundbound){
		/* keep these at the defaults, if no viewpoint is present. */
		X3D_Viewer *viewer = Viewer();
		viewer->nearPlane = DEFAULT_NEARPLANE;
		viewer->farPlane = DEFAULT_FARPLANE;
		viewer->backgroundPlane = DEFAULT_BACKGROUNDPLANE;
	}
	profile_end("loopnodeupdt");

}



//#define VERBOSE 1
void markForDispose(struct X3D_Node *node, int recursive){

	int *fieldOffsetsPtr;
	char * fieldPtr;

	if (node==NULL) return;
	if (node==X3D_NODE(rootNode()) && node->_nodeType != NODE_Proto) {
		ConsoleMessage ("not disposing rootNode");
		return;
	}


	#ifdef VERBOSE
	ConsoleMessage ("\nmarkingForDispose %p (%s) currently at %d",node,
		stringNodeType(node->_nodeType),node->referenceCount);
	#endif


	if (node->referenceCount > 0)
		node->referenceCount--;
	else
		return; //already marked

	if (recursive) {

	/* cast a "const int" to an "int" */
	fieldOffsetsPtr = (int*) NODE_OFFSETS[node->_nodeType];
	/*go thru all field*/
	while (*fieldOffsetsPtr != -1) {
		fieldPtr = offsetPointer_deref(char *, node,*(fieldOffsetsPtr+1));
		#ifdef VERBOSE
		ConsoleMessage ("looking at field %s type %s",FIELDNAMES[*fieldOffsetsPtr],FIELDTYPES[*(fieldOffsetsPtr+2)]);
		#endif

		/* some fields we skip, as the pointers are duplicated, and we CAN NOT free both */
		if (*fieldOffsetsPtr == FIELDNAMES_setValue)
			break; /* can be a duplicate SF/MFNode pointer */

		if (*fieldOffsetsPtr == FIELDNAMES_valueChanged)
			break; /* can be a duplicate SF/MFNode pointer */

		if (*fieldOffsetsPtr == FIELDNAMES__selected)
			break; /* can be a duplicate SFNode pointer - field only in NODE_LOD and NODE_GeoLOD */

		if (*fieldOffsetsPtr == FIELDNAMES___oldChildren)
			break; /* can be a duplicate SFNode pointer - field only in NODE_LOD and NODE_GeoLOD */

		if (*fieldOffsetsPtr == FIELDNAMES___oldKeyPtr)
			break; /* used for seeing if interpolator values change */

		if (*fieldOffsetsPtr == FIELDNAMES___oldKeyValuePtr)
			break; /* used for seeing if interpolator values change */

		/* GeoLOD nodes, the children field exports either the rootNode, or the list of child nodes */
		if (node->_nodeType == NODE_GeoLOD) {
			if (*fieldOffsetsPtr == FIELDNAMES_children) break;
		}

		if (*fieldOffsetsPtr == FIELDNAMES__shaderUserDefinedFields)
			break; /* have to get rid of the fields of a shader here....*/

		/* nope, not a special field, lets just get rid of it as best we can */
		switch(*(fieldOffsetsPtr+2)){
			case FIELDTYPE_MFNode: {
				int i;
				struct Multi_Node* MNode;
				struct X3D_Node *tp;

				MNode=(struct Multi_Node *)fieldPtr;
		/* printf (" ... field MFNode, %s type %s\n",FIELDNAMES[*fieldOffsetsPtr],FIELDTYPES[*(fieldOffsetsPtr+2)]); */

				for (i=0; i<MNode->n; i++) {
					tp = MNode->p[i];

					if (tp!=NULL) {
						#ifdef VERBOSE
						ConsoleMessage ("calling markForDispose on node %p as it is an element of an MFNode",tp);
						#endif
						markForDispose(tp,TRUE);
					}
				}
				// MNode->n=0;  unlink_node needs this in order to properly unlink children.
				break;
				}
			case FIELDTYPE_SFNode: {
				struct X3D_Node *SNode;

				memcpy(&SNode,fieldPtr,sizeof(struct X3D_Node *));

				#ifdef VERBOSE
				ConsoleMessage ("SFNode, field is %p...",SNode);
				if (SNode != NULL) {
					ConsoleMessage ("SFNode, .... and it is of type %s",stringNodeType(SNode->_nodeType));
					ConsoleMessage (" ... field SFNode, %s type %s\n",FIELDNAMES[*fieldOffsetsPtr],FIELDTYPES[*(fieldOffsetsPtr+2)]);
				}
				#endif

				if(SNode && SNode->referenceCount > 0)  {
					#ifdef VERBOSE
					ConsoleMessage ("calling markForDispose on node %p as it is contents of SFNode field",SNode);
					#endif
					markForDispose(SNode, TRUE);
				}
				break;


			}
			default:; /* do nothing - field not malloc'd */
		}
		fieldOffsetsPtr+=5;
	}


	}
}
#undef VERBOSE

#define DELETE_IF_IN_PRODCON(aaa) \
	if (tg->ProdCon.aaa) { \
		bool foundIt = FALSE; \
		/* ConsoleMessage ("ProdCon stack is %d in size\n",vectorSize(tg->ProdCon.aaa)); */ \
		for (i=0; i<vectorSize(tg->ProdCon.aaa); i++) { \
			if (vector_get(struct X3D_Node*,tg->ProdCon.aaa, i) == structptr) { \
				foundIt = TRUE; \
				/* ConsoleMessage ("found it in the stack!\n"); */ \
			} \
		} \
		if (foundIt) { \
			struct Vector *newStack = newVector(struct X3D_Node*, 2); \
			for (i=0; i<vectorSize(tg->ProdCon.aaa); i++) { \
				if (vector_get(struct X3D_Node*,tg->ProdCon.aaa, i) != structptr) { \
					vector_pushBack(struct X3D_Node*, newStack,  \
						vector_get(struct X3D_Node*,tg->ProdCon.aaa,i)); \
				} \
			} \
			deleteVector(struct X3D_Node*, tg->ProdCon.aaa); \
			tg->ProdCon.aaa = newStack; \
		} \
	}

#ifdef OLDCODE
#define DELETE_IF_IN_STACK(aaa) \
	if (tg->Bindable.aaa) { \
		bool foundIt = FALSE; \
		/* ConsoleMessage ("Bindable stack is %d in size\n",vectorSize(tg->Bindable.aaa)); */ \
		for (i=0; i<vectorSize(tg->Bindable.aaa); i++) { \
			if (vector_get(struct X3D_Node*,tg->Bindable.aaa, i) == structptr) { \
				foundIt = TRUE; \
				/* ConsoleMessage ("found it in the stack!\n"); */ \
			} \
		} \
		if (foundIt) { \
			struct Vector *newStack = newVector(struct X3D_Node*, 2); \
			for (i=0; i<vectorSize(tg->Bindable.aaa); i++) { \
				if (vector_get(struct X3D_Node*,tg->Bindable.aaa, i) != structptr) { \
					vector_pushBack(struct X3D_Node*, newStack,  \
						vector_get(struct X3D_Node*,tg->Bindable.aaa,i)); \
				} \
			} \
			deleteVector(struct X3D_Node*, tg->Bindable.aaa); \
			tg->Bindable.aaa = newStack; \
		} \
	}
#endif

//#define WRLMODE(val) (((val) % 4)+4) //jan 2013 codegen PROTOKEYWORDS[] was ordered with x3d synonyms first, wrl last
//#define X3DMODE(val)  ((val) % 4)
#ifndef DISABLER
BOOL walk_fields(struct X3D_Node* node, BOOL (*callbackFunc)(), void* callbackData)
#else
BOOL walk_fields(struct X3D_Node* node, BOOL (*callbackFunc)(void *callbackData,struct X3D_Node* node,int jfield,union anyVrml *fieldPtr,
                                            const char *fieldName, indexT mode, indexT type,int isource,BOOL publicfield), void* callbackData)
#endif
{
	//field isource: 0=builtin 1=script user field 2=shader_program user field 3=Proto/Broto user field 4=group __protoDef
	int type,mode,source;
	BOOL publicfield;
	int *fieldOffsetsPtr;
	int jfield;
	union anyVrml *fieldPtr;
	const char* fname;
	int	foundField = 0;

	source = -1;
	mode = 0;
	type = 0;
	fieldPtr = NULL;
	jfield = -1;
	// field/event exists on the node?
	fieldOffsetsPtr = (int *)NODE_OFFSETS[node->_nodeType];
	/*go thru all builtin fields (borrowed from OpenGL_Utils killNode() L.3705*/
	while (*fieldOffsetsPtr != -1) {
		fname = FIELDNAMES[fieldOffsetsPtr[0]];
		//skip private fields which scene authors shouldn't be routing or ISing to
		publicfield = fname && (fname[0] != '_') ? TRUE : FALSE;
		mode = PKW_from_KW(fieldOffsetsPtr[3]);
		type = fieldOffsetsPtr[2];
		//retrieve nodeinstance values
		fieldPtr = (union anyVrml*)offsetPointer_deref(char *, node,*(fieldOffsetsPtr+1));
		source = 0;
		jfield++;
		foundField = callbackFunc(callbackData,node,jfield,fieldPtr,fname,mode,type,source,publicfield);
		if( foundField )break;
		fieldOffsetsPtr+=5;
	}
	if(!foundField)
	{
		//user-field capable node?
		int user;
		user = nodeTypeSupportsUserFields(node);
		jfield = -1;
		publicfield = 1; //assume all user fields are public
		if(user)
		{
			//lexer_stringUser_fieldName(me->lexer, name, mode);
			//struct VRMLParser* parser;
			//struct VRMLLexer* lexer;
			//ttglobal tg = gglobal();
			//lexer = NULL;
			//parser = tg->CParse.globalParser;
			//if (parser)
			//	lexer = parser->lexer;

			//user fields on user-field-capable nodes
			switch(node->_nodeType)
			{
				case NODE_Script:
				case NODE_ComposedShader:
				case NODE_ShaderProgram :
				case NODE_PackagedShader:
					{
						int j; //, nameIndex;
						struct ScriptFieldDecl* sfield;
						struct Shader_Script* shader = NULL;

						switch(node->_nodeType)
						{
  							case NODE_Script:         shader =(struct Shader_Script *)(X3D_SCRIPT(node)->__scriptObj); break;
  							case NODE_ComposedShader: shader =(struct Shader_Script *)(X3D_COMPOSEDSHADER(node)->_shaderUserDefinedFields); break;
  							case NODE_ShaderProgram:  shader =(struct Shader_Script *)(X3D_SHADERPROGRAM(node)->_shaderUserDefinedFields); break;
  							case NODE_PackagedShader: shader =(struct Shader_Script *)(X3D_PACKAGEDSHADER(node)->_shaderUserDefinedFields); break;
						}
						if (shader)
							for(j=0; j!=vectorSize(shader->fields); ++j)
							{
								sfield= vector_get(struct ScriptFieldDecl*, shader->fields, j);
								mode = sfield->fieldDecl->PKWmode;
								fname = ScriptFieldDecl_getName(sfield);
								type = sfield->fieldDecl->fieldType;
								fieldPtr = &sfield->value;
								source = node->_nodeType == NODE_Script ? 1 : 2;
								jfield = j;
								foundField = callbackFunc(callbackData,node,jfield,fieldPtr,fname,mode,type,source,publicfield);
								if( foundField)
									break;
							}
					}
					break;
				case NODE_Proto:
					{
						int j; //, nameIndex;
						struct ProtoFieldDecl* pfield;
						struct X3D_Proto* pnode = (struct X3D_Proto*)node;
						if(pnode) {
							struct ProtoDefinition* pstruct = (struct ProtoDefinition*) pnode->__protoDef;
							if(pstruct)
							if(pstruct->iface)
							for(j=0; j!=vectorSize(pstruct->iface); ++j)
							{
								pfield= vector_get(struct ProtoFieldDecl*, pstruct->iface, j);
								mode = pfield->mode;
								fname = pfield->cname;
								type = pfield->type;
								fieldPtr = &pfield->defaultVal;
								source = 3;
								jfield = j;
								foundField = callbackFunc(callbackData,node,jfield,fieldPtr,fname,mode,type,source,publicfield);
								if( foundField)
									break;
							}
						}
					}
				case NODE_Group:
					//not sure how to do it with the metanode interface and mangled names
					break;
				default:
					break;
			}
		}
	}
	return foundField;
}


/*
	memory management policy, Feb 22, 2013, dug9 after adding unlink_node() call to killNode()
	Background: we aren't using any smart pointers / garbage collection library in freewrl. Just old fashioned
	malloc and free, with one exception:

	We register malloced nodes in linearNodeTable, and when their reference
	count goes to zero we call killNode (which calls unlink_node()) and deallocate their memory.

	There's one place -startofloopnodeupdates- and one function -killNode- where they get deleted. But there's
	two basic scenarios:
		a) a node is markForDispose() during the freewrl session. tests/46.wrl
		b) you anchor or File > Open another scene. The old scene -in kill_OldWorld() markForDispose() calls
			referenceCount-- and then when the new scene comes into startofloopnodeupdates() all the old
			nodes trigger a call for killNode().
	So to test, I used 46.wrl and File>Open from the statusbar, and that would trigger some killNode activity
	(for FileOpen you can open an empty scene with just the vrml header, to simplify debugging)

	As I started to do this, freewrl bombed here and there with different data sets. One thing that will
	help is to be consistent about what we manage. For that have a policy: what malloced things we are
	managing now, so as programmers we are clear on what gets linked, and deleted. We'll call the carefully
	linked and unlinked and deallocated nodes 'managed nodes' and the fields we manage 'managed fields'.

	Examples,
	a) a node hold pointers to other nodes in its SFNode and MFNode fields such as 'children' and
		'material'. We'll call all those children.
	b) a node holds pointers to other nodes that have it as a child, in its parentVector.
		A DEF/USE pair of parent nodes mean the child's parentVector will have 2 entries, one for each
		of the DEF and USE.

	Here's the rule (I used) for managing nodes:
	- Definitions:
		public field: a field with no _ prefix on the name, or else its a user field in a Script/shader or Proto
		value holding field: initializeOnly/field or inputOutput/exposedField
		event field: inputOnly/eventIn or outputOnly/eventOut
		node field: SFNode or MFNode type
		managed field: value holding public node field, plus prescribed fields, minus exempted fields
		prescribed fields: fields we decided to also manage: (none)
		exempted fields: fields we decided not to manage: (none)

	- if a node is in a managed field of another node -a parent- then it
		registers/adds that parent in its own parentVector.
	- when the node is taken out/removed from a managed field, it removes the parent
		from its parentVector (if it's in just one managed field. If in 2, and removed from one, then parent stays)
	- when a node deletes itself, it:
		a) goes through all its managed fields and it tells those  nodes -called children- to delete it
			from their parentVector. Then it clears/zeros that field in itself to zero so its not
			pointing at those children. And
		b) it goes through its parentVector and tells each parent to remove it from any managed fields.
	- if a node is in an event field or private field then it does not register that node in its parent field
	- if a node gets routed to an event field of a target node, no parent is registered.
	- if a node gets routed from an event field of a source node, the source node is not
		registerd in its parentVector (example: script node generates a node in an eventOut field,
		and whether or not it's routed anywhere, the script is not put into the nodes' parent list)
	HTH
*/
BOOL isManagedField(int mode, int type, BOOL isPublic)
{
	BOOL isManaged = (type == FIELDTYPE_SFNode || type == FIELDTYPE_MFNode);
	isManaged = isManaged && (mode == PKW_initializeOnly || mode == PKW_inputOutput);
	isManaged = isManaged && isPublic;
	return isManaged;
}


/* print the parent vector and and SFNode and MFNode subfield entry pointers
   for debugging unlink_node (and any linking code)
   -needs generalization for FILE*
   -maybe a dumpscreen option to get all rootnodes
*/
void indentf(int indent){
	int m;
	for(m=0;m<indent;m++) printf(" ");
}

void print_node_links0(struct X3D_Node* sfn, int *level);
BOOL cbPrintLinks(void *callbackData,struct X3D_Node* node,int jfield,
	union anyVrml *fieldPtr,char *fieldName, int mode,int type,int source,BOOL publicfield)
{
	int *level = (int*)callbackData;
	(*level)++;
	//if((*level) > 80)
	//	return FALSE;
	//if(publicfield && (type == FIELDTYPE_SFNode || type == FIELDTYPE_MFNode))
	if(isManagedField(type,mode,publicfield))
	{
		int n,k,haveSomething;
		struct X3D_Node **plist, *sfn;
		haveSomething = (type==FIELDTYPE_SFNode && fieldPtr->sfnode) || (type==FIELDTYPE_MFNode && fieldPtr->mfnode.n);
		if(haveSomething){
			indentf(*level);
			printf("%s  ",fieldName);
			if(type==FIELDTYPE_SFNode){
				plist = &fieldPtr->sfnode;
				n = 1;
			}else{
				plist = fieldPtr->mfnode.p;
				n = fieldPtr->mfnode.n;
				if(n > 1){
					printf("[\n");
					(*level)++;
					//indentf((*level));
				}
			}
			for(k=0;k<n;k++)
			{
				sfn = plist[k];
				if(n>1) indentf((*level));
				print_node_links0(sfn,level);
			}
			if(type==FIELDTYPE_MFNode && n > 1){
				(*level)--;
				indentf((*level));
				printf("]\n");
			}
		}
	}
	(*level)--;
	return FALSE; //false to keep walking fields, true to break out
}

void print_node_links0(struct X3D_Node* sfn, int *level)
{
	if(sfn)
	{
		/* unlink children in any SFNode or MFNode field of node */
		printf("node %p ",sfn);
		if(sfn->_parentVector && vectorSize(sfn->_parentVector)){
			int j;
			printf(" parents={");
			for(j=0;j<vectorSize(sfn->_parentVector);j++){
				struct X3D_Node *parent = vector_get(struct X3D_Node *,sfn->_parentVector, j);
				printf("%p, ",parent);
			}
			printf("}");
		}
		printf("\n");
		walk_fields(sfn,cbPrintLinks,level);
	}
}
void print_node_links(struct X3D_Node* sfn)
{
	int level = 0;
	print_node_links0(sfn,&level);
	if(level != 0)
		printf("ouch level =%d\n",level);
}


BOOL cbUnlinkChild(void *callbackData,struct X3D_Node* node,int jfield,
	union anyVrml *fieldPtr,char *fieldName, int mode,int type,int source,BOOL publicfield)
{
	if(isManagedField(mode,type,publicfield)){
		if(type == FIELDTYPE_SFNode){
			struct X3D_Node **sfn = &fieldPtr->sfnode;
			AddRemoveSFNodeFieldChild(node,sfn,*sfn,2,__FILE__,__LINE__);
			if(fieldPtr->sfnode)
				printf("didn't delete sfnode child\n");
		}else if(type == FIELDTYPE_MFNode){
			struct Multi_Node* mfn = &fieldPtr->mfnode;
			AddRemoveChildren(node,mfn,mfn->p,mfn->n,2,__FILE__,__LINE__);
		}
	}
	return FALSE; //false to keep walking fields, true to break out
}
BOOL cbUnlinkParent(void *callbackData,struct X3D_Node* parent,int jfield,
	union anyVrml *fieldPtr,char *fieldName, int mode,int type,int source,BOOL publicfield)
{
	struct X3D_Node* node = X3D_NODE(callbackData);
	if(isManagedField(mode,type,publicfield)){
		if(type == FIELDTYPE_SFNode){
			struct X3D_Node **sfn = &fieldPtr->sfnode;
			AddRemoveSFNodeFieldChild(parent,sfn,node,2,__FILE__,__LINE__);
		}else if(type == FIELDTYPE_MFNode){
			struct Multi_Node* mfn = &fieldPtr->mfnode;
			AddRemoveChildren(parent,mfn,&node,1,2,__FILE__,__LINE__); //Q. is 2 remove?
		}
	}
	return FALSE; //false to keep walking fields, true to break out
}
void unlink_node(struct X3D_Node* node)
{
	if(node)
	{
		/* unlink children in any SFNode or MFNode field of node */
		walk_fields(node,cbUnlinkChild,NULL);
		/* unlink/remove node from any SFNode or MFNode field of parents */
		if(node->_parentVector && vectorSize(node->_parentVector)){
			int j;
			struct Vector* pp = newVector(struct X3D_Node*,vectorSize(node->_parentVector));
			for(j=0;j<vectorSize(node->_parentVector);j++){
				struct X3D_Node *parent = vector_get(struct X3D_Node *,node->_parentVector, j);
				vector_pushBack(struct X3D_Node*,pp,parent);
			}
			for(j=0;j<vectorSize(pp);j++){
				struct X3D_Node *parent = vector_get(struct X3D_Node *,pp, j);
				walk_fields(parent,cbUnlinkParent,node);
			}
			node->_parentVector->n = 0;
			deleteVector(struct X3D_Node*, pp);
		}
	}
}

void deleteMallocedFieldValue(int type,union anyVrml *fieldPtr);
BOOL cbFreeMallocedBuiltinField(void *callbackData,struct X3D_Node* node,int jfield,
	union anyVrml *fieldPtr,char *fieldName, int mode,int type,int source,BOOL publicfield)
{
	//for builtins, the field is malloced as part of the node size, so we don't free the field itself
	// .. just if its a complex field type holding a malloced pointer
	// .. like MF.p or SFString.ptr
	// and only if the node owns the pointer, which we determine by if the field is initializeOnly or inputOutput
	if(source == 0){
		//if(mode == PKW_initializeOnly || mode == PKW_inputOutput){
		if(1){
			//#define FIELDTYPE_FreeWRLPTR	22
			//#define FIELDTYPE_SFImage	23
			//if(strcmp(fieldName,"__oldurl") && strcmp(fieldName,"__oldSFString") && strcmp(fieldName,"__oldMFString") && strcmp(fieldName,"_parentVector")) {
			if(strcmp(fieldName,"__oldurl") && strcmp(fieldName,"_parentVector")) {
			//if(1){
				//skip double underscore prefixed fields, which we will treat as not-to-be-deleted, because duplicates like GeoViewpoint __oldMFString which is a duplicate of navType
				deleteMallocedFieldValue(type,fieldPtr);
				if(type == FIELDTYPE_FreeWRLPTR){
					if(!strncmp(fieldName,"__x",3) || !strncmp(fieldName,"__v",3)) {  //May 2015 special field name prefixes __x and __v signals OK to FREE_IF_NZ
						FREE_IF_NZ(fieldPtr->sfnode); //free it as a pointer
					}
				}
			}
		}
	}
	return FALSE; //false to keep walking fields, true to break out
}
BOOL cbFreePublicMallocedBuiltinField(void *callbackData,struct X3D_Node* node,int jfield,
	union anyVrml *fieldPtr,char *fieldName, int mode,int type,int source,BOOL publicfield)
{
	//for builtins, the field is malloced as part of the node size, so we don't free the field itself
	// .. just if its a complex field type holding a malloced pointer
	// .. like MF.p or SFString.ptr
	// and only if the node owns the pointer, which we determine by if the field is initializeOnly or inputOutput
	if(source == 0){
		//if(mode == PKW_initializeOnly || mode == PKW_inputOutput){
		if(1){
			//#define FIELDTYPE_FreeWRLPTR	22
			//#define FIELDTYPE_SFImage	23
			if(strncmp(fieldName,"_",1)) { //only public fields, skip _ and __ private fields
			//if(1){
				//skip double underscore prefixed fields, which we will treat as not-to-be-deleted, because duplicates like GeoViewpoint __oldMFString which is a duplicate of navType
				deleteMallocedFieldValue(type,fieldPtr);
			}
		}
	}
	return FALSE; //false to keep walking fields, true to break out
}
BOOL cbFreeMallocedUserField(void *callbackData,struct X3D_Node* node,int jfield,
	union anyVrml *fieldPtr,char *fieldName, int mode,int type,int source,BOOL publicfield)
{
	//for userFields (ie on Script, Proto), the field is malloced as part of a bigger struct
	// .. so we free any pointers contained in the field like MF.p or SFString.ptr
	// and only if the node owns the pointer, which we determine by if the field is initializeOnly or inputOutput
	// .. but we don't free the anyVrml* pointer itself
	// .. we rely on something freeing the user fields elsewhere to free the vector of userfields and
	// .. a few of their malloced contents

	if(source > 0){
		//user field in source = {script=1, shaders etc 2, protos = 3}
		//if(mode == PKW_initializeOnly || mode == PKW_inputOutput){
		if(1){
			if(strncmp(fieldName,"__",2)) {
			//if(1){
				//skip double underscore prefixed fields, which we will treat as not-to-be-deleted, because duplicates like GeoViewpoint __oldMFString which is a duplicate of navType
				deleteMallocedFieldValue(type,fieldPtr);
			}
		}
	}
	return FALSE; //false to keep walking fields, true to break out
}
struct Shader_Script *getShader(struct X3D_Node *node){
	struct Shader_Script *shader = NULL;
	switch(node->_nodeType)
	{
  		case NODE_Script:         shader =(struct Shader_Script *)(X3D_SCRIPT(node)->__scriptObj); break;
  		case NODE_ComposedShader: shader =(struct Shader_Script *)(X3D_COMPOSEDSHADER(node)->_shaderUserDefinedFields); break;
  		case NODE_ShaderProgram:  shader =(struct Shader_Script *)(X3D_SHADERPROGRAM(node)->_shaderUserDefinedFields); break;
  		case NODE_PackagedShader: shader =(struct Shader_Script *)(X3D_PACKAGEDSHADER(node)->_shaderUserDefinedFields); break;
	}
	return shader;
}
void setShader(struct X3D_Node *node, struct Shader_Script *shader){
	switch(node->_nodeType)
	{
  		case NODE_Script:         X3D_SCRIPT(node)->__scriptObj = (void *)shader; break;
  		case NODE_ComposedShader: X3D_COMPOSEDSHADER(node)->_shaderUserDefinedFields = (void *)shader;; break;
  		case NODE_ShaderProgram:  X3D_SHADERPROGRAM(node)->_shaderUserDefinedFields = (void *)shader;; break;
  		case NODE_PackagedShader: X3D_PACKAGEDSHADER(node)->_shaderUserDefinedFields = (void *)shader;; break;
	}

}
void deleteShaderDefinition(struct Shader_Script *shader){
	if(shader){
		if(shader->fields){
			int i;
			for(i=0;i<vectorSize(shader->fields);i++){
				struct ScriptFieldDecl *field = vector_get(struct ScriptFieldDecl*,shader->fields,i);
				deleteScriptFieldDecl(field);
			
			}
			deleteVector(struct ScriptFieldDecl*,shader->fields);
			FREE_IF_NZ(shader->fields);
		}
		FREE_IF_NZ(shader);
	}
}
//static struct Vector freed;
//static struct fieldFree ffs[100];
void freeMallocedNodeFields0(struct X3D_Node* node){
	//PIMPL Idiom in C is like objects in C++ - each object should know how to delete itself
	//we don't have good pimpl habits yet in freewrl
	//Here we try and generically free what a node may have allocated
	//(a pimpl alternative might be node-type specific freeing)
	//assume node->_intern = polyrep, node->_parents vector, and other common private fields are already done
	//then this walks over node-specific fields, attempting to free any potentially malloced fields
	//except SFNode (and SFNode contents of MFNode) which are garbage collected from 
	//a per-broto/executioncontext node table
	if(node){
		int isScriptType, isBrotoType, hasUserFields;
		isScriptType = node->_nodeType == NODE_Script || node->_nodeType == NODE_ComposedShader || node->_nodeType == NODE_ShaderProgram || node->_nodeType == NODE_PackagedShader;
		isBrotoType = node->_nodeType == NODE_Proto; //inlines have no fields, freed elsewhere || node->_nodeType == NODE_Inline;
		hasUserFields = isScriptType || isBrotoType;
		//freed.data = ffs;
		//freed.allocn = 100;
		//freed.n = 0;
		if(hasUserFields){
			walk_fields(node,cbFreeMallocedUserField,NULL); //&freed);
			if(isScriptType){
				struct Shader_Script *shader = getShader(node);
				if (shader){
					deleteShaderDefinition(shader);
					setShader(node,NULL);
				}
			}else if(isBrotoType){
				struct X3D_Proto* pnode = (struct X3D_Proto*)node;
				deleteProtoDefinition(pnode->__protoDef);
				FREE_IF_NZ(pnode->__protoDef);
				FREE_IF_NZ(pnode->__typename);
				//struct ProtoDefinition* pstruct = (struct ProtoDefinition*) pnode->__protoDef;
				//if(pstruct){
				//	//for vectorget.n field->malloced stuff
				//	deleteVector(struct ProtoDefinition*,pstruct);
				//	pnode->__protoDef = NULL;
				//}
			}
		}
		/* free malloced public fields */
		walk_fields(node,cbFreeMallocedBuiltinField,NULL); //&freed);
	}
}
//void freePublicBuiltinNodeFields(struct X3D_Node* node){
//	if(node)
//		walk_fields(node,cbFreePublicMallocedBuiltinField,NULL); //&freed);
//}
void freeMallocedNodeFields(struct X3D_Node* node){
	if(node){
		deleteVector(sizeof(void*),node->_parentVector);
		freeMallocedNodeFields0(node);
	}
}
/*delete node created
static void killNode_hide_obsolete (int index) {
	int j=0;
	int *fieldOffsetsPtr;
	char * fieldPtr;
	struct X3D_Node* structptr;
	struct Multi_Float* MFloat;
	struct Multi_Rotation* MRotation;
	struct Multi_Vec3f* MVec3f;
	struct Multi_Bool* Mbool;
	struct Multi_Int32* MInt32;
	struct Multi_Node* MNode;
	struct Multi_Color* MColor;
	struct Multi_ColorRGBA* MColorRGBA;
	struct Multi_Time* MTime;
	struct Multi_String* MString;
	struct Multi_Vec2f* MVec2f;
	intptr_t * VPtr;
	struct Uni_String *MyS;
 	int i;

	ppOpenGL_Utils p;
	ttglobal tg = gglobal();
	p = (ppOpenGL_Utils)tg->OpenGL_Utils.prv;

	structptr = vector_get(struct X3D_Node *,p->linearNodeTable,index);
	//ConsoleMessage("killNode - looking for node %p of type %s in one of the stacks\n", structptr,stringNodeType(structptr->_nodeType));

	if( structptr->referenceCount > -1 ){
		// unlinking the node from special arrays, parents and children
		//   we just need to do this once, and early in the kill process
		//   - I wish we had a sentinal value for 'unlinked' 
		DELETE_IF_IN_STACK(viewpoint_stack);
		DELETE_IF_IN_STACK(background_stack);
		DELETE_IF_IN_STACK(fog_stack);
		DELETE_IF_IN_STACK(navigation_stack);
		DELETE_IF_IN_PRODCON(viewpointNodes);
		delete_first(structptr);
		//print_node_links(structptr);
		unlink_node(structptr); //unlink before settledown deleting..
		//printf("after: \n");
		//print_node_links(structptr);
	}

	// give this time for things to "settle" in terms of rendering, etc
	//JAS: "OpenGL - old code called flush() or finish(), but when the front-end does the actual rendering,
	//what happens is that the GL calls get queued up for the GPU, then run when possible. So, there
	//is a "hidden" multi-threading going on there. IIRC, I gave it 10 rendering loops for an unused
	//node before deleting any of the items in it; really 1 or 2 loops should be fine. (1, but don't
	//know about double buffering; 10 is a safe overkill) Without that, having OpenGL issues was a
	//random certainty when removing nodes, and data from these nodes."
	
	structptr->referenceCount --;
	if (structptr->referenceCount > -10) {
		//ConsoleMessage ("ref count for %p is just %d, waiting\n",structptr,structptr->referenceCount);
		return;
	}
	//ConsoleMessage ("kn %d %s\n",index,stringNodeType(structptr->_nodeType));

	#ifdef VERBOSE
	printf("killNode: Node pointer	= %p entry %d of %d ",structptr,i,vectorSize(p->linearNodeTable));
	if (structptr) {
	if (structptr->_parentVector)
	printf (" number of parents %d ", vectorSize(structptr->_parentVector));
	printf("Node Type	= %s",stringNodeType(structptr->_nodeType));
	} printf ("\n");
	#endif
	// node must be already unlinked with unlink_node() when we get here 
	// delete parent vector. 
 	deleteVector(char*, structptr->_parentVector);
	// clear child vector - done below 

	fieldOffsetsPtr = (int *)NODE_OFFSETS[structptr->_nodeType];
	//go thru all field
	while (*fieldOffsetsPtr != -1) {
		fieldPtr = offsetPointer_deref(char *, structptr,*(fieldOffsetsPtr+1));
		#ifdef VERBOSE
		printf ("looking at field %s type %s\n",FIELDNAMES[*fieldOffsetsPtr],FIELDTYPES[*(fieldOffsetsPtr+2)]);
		#endif

		// some fields we skip, as the pointers are duplicated, and we CAN NOT free both 
		if (*fieldOffsetsPtr == FIELDNAMES_setValue)
			break; // can be a duplicate SF/MFNode pointer 

		if (*fieldOffsetsPtr == FIELDNAMES_valueChanged)
			break; // can be a duplicate SF/MFNode pointer 

		if (*fieldOffsetsPtr == FIELDNAMES__parentResource)
			break; // can be a duplicate SF/MFNode pointer 


		if (*fieldOffsetsPtr == FIELDNAMES___oldmetadata)
			break; // can be a duplicate SFNode pointer 

		if (*fieldOffsetsPtr == FIELDNAMES__selected)
			break; // can be a duplicate SFNode pointer - field only in NODE_LOD and NODE_GeoLOD 

		if (*fieldOffsetsPtr == FIELDNAMES___oldChildren)
			break; // can be a duplicate SFNode pointer - field only in NODE_LOD and NODE_GeoLOD 

		if (*fieldOffsetsPtr == FIELDNAMES___oldMFString)
			break;

		if (*fieldOffsetsPtr == FIELDNAMES___scriptObj)
			break;

		if (*fieldOffsetsPtr == FIELDNAMES___oldSFString)
			break;

		if (*fieldOffsetsPtr == FIELDNAMES___oldKeyPtr)
			break; // used for seeing if interpolator values change 

		if (*fieldOffsetsPtr == FIELDNAMES___oldKeyValuePtr)
			break; // used for seeing if interpolator values change 


		// GeoLOD nodes, the children field exports either the rootNode, or the list of child nodes 
		if (structptr->_nodeType == NODE_GeoLOD) {
			if (*fieldOffsetsPtr == FIELDNAMES_children) break;
		}

		// nope, not a special field, lets just get rid of it as best we can 
		//	dug9 sept 2014: GC garbage collection: I wonder if it would be easier/simpler when we malloc something,
		//	to put it into a flat scene-GC list (and inline-GC list?) - as we do for a few things already, like nodes - 
		//	and don't GC here for fields on occassionally removed nodes, just when we change scenes
		//	wipe out the whole GC table(s)?
		//
		switch(*(fieldOffsetsPtr+2)){
			case FIELDTYPE_MFFloat:
				MFloat=(struct Multi_Float *)fieldPtr;
				MFloat->n=0;
				FREE_IF_NZ(MFloat->p);
				break;
			case FIELDTYPE_MFRotation:
				MRotation=(struct Multi_Rotation *)fieldPtr;
				MRotation->n=0;
				FREE_IF_NZ(MRotation->p);
				break;
			case FIELDTYPE_MFVec3f:
				MVec3f=(struct Multi_Vec3f *)fieldPtr;
				MVec3f->n=0;
				FREE_IF_NZ(MVec3f->p);
				break;
			case FIELDTYPE_MFBool:
				Mbool=(struct Multi_Bool *)fieldPtr;
				Mbool->n=0;
				FREE_IF_NZ(Mbool->p);
				break;
			case FIELDTYPE_MFInt32:
				MInt32=(struct Multi_Int32 *)fieldPtr;
				MInt32->n=0;
				FREE_IF_NZ(MInt32->p);
				break;
			case FIELDTYPE_MFNode:
				MNode=(struct Multi_Node *)fieldPtr;
				#ifdef VERBOSE
				//verify node structure. Each child should point back to me. 
				{
					int i;
					struct X3D_Node *tp;
					for (i=0; i<MNode->n; i++) {
						tp = MNode->p[i];
						printf ("	MNode field has child %p\n",tp);
						if (tp!=NULL)
						printf ("	ct %s\n",stringNodeType(tp->_nodeType));
					}
				}
				#endif
				MNode->n=0;
				FREE_IF_NZ(MNode->p);
				break;

			case FIELDTYPE_MFColor:
				MColor=(struct Multi_Color *)fieldPtr;
				MColor->n=0;
				FREE_IF_NZ(MColor->p);
				break;
			case FIELDTYPE_MFColorRGBA:
				MColorRGBA=(struct Multi_ColorRGBA *)fieldPtr;
				MColorRGBA->n=0;
				FREE_IF_NZ(MColorRGBA->p);
				break;
			case FIELDTYPE_MFTime:
				MTime=(struct Multi_Time *)fieldPtr;
				MTime->n=0;
				FREE_IF_NZ(MTime->p);
				break;
			case FIELDTYPE_MFString:
				MString=(struct Multi_String *)fieldPtr;
				{
				struct Uni_String* ustr;
				for (j=0; j<MString->n; j++) {
					ustr=MString->p[j];
					if (ustr != NULL) {
					ustr->len=0;
					ustr->touched=0;
					FREE_IF_NZ(ustr->strptr);
					}
				}
				MString->n=0;
				FREE_IF_NZ(MString->p);
				}
				break;
			case FIELDTYPE_MFVec2f:
				MVec2f=(struct Multi_Vec2f *)fieldPtr;
				MVec2f->n=0;
				FREE_IF_NZ(MVec2f->p);
				break;
			case FIELDTYPE_FreeWRLPTR:
				VPtr = (intptr_t *) fieldPtr;
				VPtr = (intptr_t *) (*VPtr);
				FREE_IF_NZ(VPtr);
				break;
			case FIELDTYPE_SFString:
				VPtr = (intptr_t *) fieldPtr;
				MyS = (struct Uni_String *) *VPtr;
				MyS->len = 0;
				FREE_IF_NZ(MyS->strptr);
				FREE_IF_NZ(MyS);
				break;

			default:; // do nothing - field not malloc'd 
		}
		fieldOffsetsPtr+=5;
	}

	FREE_IF_NZ(structptr);
	vector_set(struct X3D_Node *, p->linearNodeTable,index,NULL);
	p->potentialHoleCount++;
	//ConsoleMessage ("kill, index %d, phc %d",index,p->potentialHoleCount);
}
*/

#ifdef DEBUG_FW_LOADMAT
	static void fw_glLoadMatrixd(GLDOUBLE *val,char *where, int line) {
	{int i;
	 for (i=0; i<16; i++) {
		if (val[i] > 2000.0) printf ("FW_GL_LOADMATRIX, val %d %lf at %s:%d\n",i,val[i],where,line);
		if (val[i] < -2000.0) printf ("FW_GL_LOADMATRIX, val %d %lf at %s:%d\n",i,val[i],where,line);
	}
	}
#else
	static void fw_glLoadMatrixd(GLDOUBLE *val) {
#endif

	/* printf ("loading matrix...\n"); */
	#ifndef GL_ES_VERSION_2_0
	glLoadMatrixd(val);
	#endif
}
BOOL matrix3x3_inverse_float(float *inn, float *outt);

static void sendExplicitMatriciesToShader (GLint ModelViewMatrix, GLint ProjectionMatrix, GLint NormalMatrix, GLint TextureMatrix)

{

	float spval[16];
	int i,j;
	float *sp;
	GLDOUBLE *dp;
	ppOpenGL_Utils p = (ppOpenGL_Utils)gglobal()->OpenGL_Utils.prv;


	/* ModelView first */
	dp = p->FW_ModelView[p->modelviewTOS];
	sp = spval;

	/* convert GLDOUBLE to float */
	for (i=0; i<16; i++) {
		*sp = (float) *dp;
		sp ++; dp ++;
	}
	profile_start("sendmtx");
	GLUNIFORMMATRIX4FV(ModelViewMatrix,1,GL_FALSE,spval);
	profile_end("sendmtx");
	/* ProjectionMatrix */
	sp = spval;
	dp = p->FW_ProjectionView[p->projectionviewTOS];

	matdouble2float4(sp,dp);
	///* convert GLDOUBLE to float */
	//for (i=0; i<16; i++) {
	//	*sp = (float) *dp;
	//	sp ++; dp ++;
	//}
	profile_start("sendmtx");
	GLUNIFORMMATRIX4FV(ProjectionMatrix,1,GL_FALSE,spval);
	profile_end("sendmtx");
	/* TextureMatrix */
	if (TextureMatrix != -1) {
		sp = spval;
		dp = p->FW_TextureView[p->textureviewTOS];

		//ConsoleMessage ("sendExplicitMatriciesToShader, sizeof GLDOUBLE %d, sizeof float %d",sizeof(GLDOUBLE), sizeof(float));
		/* convert GLDOUBLE to float */
		for (i=0; i<16; i++) {
			*sp = (float) *dp;
			sp ++; dp ++;
		}
        profile_start("sendmtx");
		GLUNIFORMMATRIX4FV(TextureMatrix,1,GL_FALSE,spval);
		profile_end("sendmtx");
	}


	/* send in the NormalMatrix */
	/* Uniform mat3  gl_NormalMatrix;  transpose of the inverse of the upper
                               		  leftmost 3x3 of gl_ModelViewMatrix */
	if (NormalMatrix != -1) {
		float normMat[9];
		dp = p->FW_ModelView[p->modelviewTOS];

		if(1){
			//trying to find another .01 FPS
			//switch from 4x4 double to 3x3 float inverse
			float ftemp[9];
			/* convert GLDOUBLE to float */
			for (i=0; i<3; i++)
				for(j=0;j<3;j++)
					spval[i*3 +j] = (float) dp[i*4 + j];

			matrix3x3_inverse_float(spval, ftemp);
			//transpose
			for (i = 0; i < 3; i++)
				for (j = 0; j < 3; j++)
					normMat[i*3 +j] = ftemp[j*3 + i];

		}
		if(0){
		GLDOUBLE inverseMV[16];
		GLDOUBLE transInverseMV[16];
		GLDOUBLE MV[16];
		memcpy(MV,dp,sizeof(GLDOUBLE)*16);

		matinverse (inverseMV,MV);
		mattranspose(transInverseMV,inverseMV);
		/* get the 3x3 normal matrix from this guy */
		normMat[0] = (float) transInverseMV[0];
		normMat[1] = (float) transInverseMV[1];
		normMat[2] = (float) transInverseMV[2];

		normMat[3] = (float) transInverseMV[4];
		normMat[4] = (float) transInverseMV[5];
		normMat[5] = (float) transInverseMV[6];

		normMat[6] = (float) transInverseMV[8];
		normMat[7] = (float) transInverseMV[9];
		normMat[8] = (float) transInverseMV[10];
		}
/*
printf ("NormalMatrix: \n \t%4.3f %4.3f %4.3f\n \t%4.3f %4.3f %4.3f\n \t%4.3f %4.3f %4.3f\n",
normMat[0],normMat[1],normMat[2],
normMat[3],normMat[4],normMat[5],
normMat[6],normMat[7],normMat[8]);
*/
		profile_start("sendmtx");
		GLUNIFORMMATRIX3FV(NormalMatrix,1,GL_FALSE,normMat);
		profile_end("sendmtx");
	}

}


/* make this more generic, so that the non-OpenGL-ES 2.0 FillProperties, etc, still work */

void sendMatriciesToShader(s_shader_capabilities_t *me) {
	sendExplicitMatriciesToShader (me->ModelViewMatrix, me->ProjectionMatrix, me->NormalMatrix,me->TextureMatrix);

}
#define SEND_VEC2(myMat,myVal) \
if (me->myMat != -1) { GLUNIFORM2FV(me->myMat,1,myVal);}

#define SEND_VEC4(myMat,myVal) \
if (me->myMat != -1) { GLUNIFORM4FV(me->myMat,1,myVal);}

#define SEND_FLOAT(myMat,myVal) \
if (me->myMat != -1) { GLUNIFORM1F(me->myMat,myVal);}

#define SEND_INT(myMat,myVal) \
if (me->myMat != -1) { GLUNIFORM1I(me->myMat,myVal);}



void sendMaterialsToShader(s_shader_capabilities_t *me) {
    struct matpropstruct *myap = getAppearanceProperties();
    struct fw_MaterialParameters *fw_FrontMaterial;
	struct fw_MaterialParameters *fw_BackMaterial;

    if (!myap) return;
    fw_FrontMaterial = &myap->fw_FrontMaterial;
    fw_BackMaterial = &myap->fw_BackMaterial;


	/* go through all of the Uniforms for this shader */

    /* ConsoleMessage ("sending in front diffuse %f %f %f %f ambient %f %f %f %f spec %f %f %f %f emission %f %f %f %f, shin %f",
                    fw_FrontMaterial.diffuse[0],fw_FrontMaterial.diffuse[1],fw_FrontMaterial.diffuse[2],fw_FrontMaterial.diffuse[3],
                    fw_FrontMaterial.ambient[0],fw_FrontMaterial.ambient[1],fw_FrontMaterial.ambient[2],fw_FrontMaterial.ambient[3],
                    fw_FrontMaterial.specular[0],fw_FrontMaterial.specular[1],fw_FrontMaterial.specular[2],fw_FrontMaterial.specular[3],
                    fw_FrontMaterial.emission[0],fw_FrontMaterial.emission[1],fw_FrontMaterial.emission[2],fw_FrontMaterial.emission[3],
                    fw_FrontMaterial.shininess);

ConsoleMessage ("sending in back diffuse %f %f %f %f ambient %f %f %f %f spec %f %f %f %f emission %f %f %f %f, shin %f",
                fw_BackMaterial.diffuse[0],fw_BackMaterial.diffuse[1],fw_BackMaterial.diffuse[2],fw_BackMaterial.diffuse[3],
                fw_BackMaterial.ambient[0],fw_BackMaterial.ambient[1],fw_BackMaterial.ambient[2],fw_BackMaterial.ambient[3],
                fw_BackMaterial.specular[0],fw_BackMaterial.specular[1],fw_BackMaterial.specular[2],fw_BackMaterial.specular[3],
                fw_BackMaterial.emission[0],fw_BackMaterial.emission[1],fw_BackMaterial.emission[2],fw_BackMaterial.emission[3],
                fw_BackMaterial.shininess);
*/

PRINT_GL_ERROR_IF_ANY("BEGIN sendMaterialsToShader");

/* eventually do this with code blocks in glsl */
	profile_start("sendvec");
	SEND_VEC4(myMaterialAmbient,fw_FrontMaterial->ambient);
	SEND_VEC4(myMaterialDiffuse,fw_FrontMaterial->diffuse);
	SEND_VEC4(myMaterialSpecular,fw_FrontMaterial->specular);
	SEND_VEC4(myMaterialEmission,fw_FrontMaterial->emission);
	SEND_FLOAT(myMaterialShininess,fw_FrontMaterial->shininess);

	SEND_VEC4(myMaterialBackAmbient,fw_BackMaterial->ambient);
	SEND_VEC4(myMaterialBackDiffuse,fw_BackMaterial->diffuse);
	SEND_VEC4(myMaterialBackSpecular,fw_BackMaterial->specular);
	SEND_VEC4(myMaterialBackEmission,fw_BackMaterial->emission);
	SEND_FLOAT(myMaterialBackShininess,fw_BackMaterial->shininess);
	profile_end("sendvec");

	if (me->haveLightInShader) sendLightInfo(me);

    /* FillProperties, LineProperty lineType */
    #if defined  (AQUA) || defined (GL_ES_VERSION_2_0)
	SEND_FLOAT(myPointSize,myap->pointSize);
    #else
	glPointSize(myap->pointSize > 0 ? myap->pointSize : 1);
    #endif

	profile_start("sendmat");
    //ConsoleMessage ("rlp %d %d %d %d",me->hatchPercent,me->filledBool,me->hatchedBool,me->algorithm,me->hatchColour);
    SEND_INT(filledBool,myap->filledBool);
    SEND_INT(hatchedBool,myap->hatchedBool);
    SEND_INT(algorithm,myap->algorithm);
    SEND_VEC4(hatchColour,myap->hatchColour);
    SEND_VEC2(hatchScale,myap->hatchScale);
    SEND_VEC2(hatchPercent,myap->hatchPercent);

    //TextureCoordinateGenerator
    SEND_INT(texCoordGenType,myap->texCoordGeneratorType);
	profile_end("sendmat");
	PRINT_GL_ERROR_IF_ANY("END sendMaterialsToShader");
}

static void __gluMultMatrixVecd(const GLDOUBLE matrix[16], const GLDOUBLE in[4],
                      GLDOUBLE out[4])
{
    int i;

    for (i=0; i<4; i++) {
        out[i] =
            in[0] * matrix[0*4+i] +
            in[1] * matrix[1*4+i] +
            in[2] * matrix[2*4+i] +
            in[3] * matrix[3*4+i];
    }
}


void fw_gluProject
(GLDOUBLE objx, GLDOUBLE objy, GLDOUBLE objz,
	      const GLDOUBLE modelMatrix[16],
	      const GLDOUBLE projMatrix[16],
              const GLint viewport[4],
	      GLDOUBLE *winx, GLDOUBLE *winy, GLDOUBLE *winz)
{
    GLDOUBLE in[4];
    GLDOUBLE out[4];

    in[0]=objx;
    in[1]=objy;
    in[2]=objz;
    in[3]=1.0;
    __gluMultMatrixVecd(modelMatrix, in, out);
    __gluMultMatrixVecd(projMatrix, out, in);
    if (in[3] == 0.0) return;
    in[0] /= in[3];
    in[1] /= in[3];
    in[2] /= in[3];
    /* Map x, y and z to range 0-1 */
    in[0] = in[0] * 0.5 + 0.5;
    in[1] = in[1] * 0.5 + 0.5;
    in[2] = in[2] * 0.5 + 0.5;

    /* Map x,y to viewport */
    in[0] = in[0] * viewport[2] + viewport[0];
    in[1] = in[1] * viewport[3] + viewport[1];

    *winx=in[0];
    *winy=in[1];
    *winz=in[2];
}

static void __gluMultMatricesd(const GLDOUBLE a[16], const GLDOUBLE b[16],
                                GLDOUBLE r[16])
{
    int i, j;

    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            r[i*4+j] =
                a[i*4+0]*b[0*4+j] +
                a[i*4+1]*b[1*4+j] +
                a[i*4+2]*b[2*4+j] +
                a[i*4+3]*b[3*4+j];
        }
    }
}


/*
** Invert 4x4 matrix.
** Contributed by David Moore (See Mesa bug #6748)
*/
static int __gluInvertMatrixd(const GLDOUBLE m[16], GLDOUBLE invOut[16])
{
    GLDOUBLE inv[16], det;
    int i;

    inv[0] =   m[5]*m[10]*m[15] - m[5]*m[11]*m[14] - m[9]*m[6]*m[15]
             + m[9]*m[7]*m[14] + m[13]*m[6]*m[11] - m[13]*m[7]*m[10];
    inv[4] =  -m[4]*m[10]*m[15] + m[4]*m[11]*m[14] + m[8]*m[6]*m[15]
             - m[8]*m[7]*m[14] - m[12]*m[6]*m[11] + m[12]*m[7]*m[10];
    inv[8] =   m[4]*m[9]*m[15] - m[4]*m[11]*m[13] - m[8]*m[5]*m[15]
             + m[8]*m[7]*m[13] + m[12]*m[5]*m[11] - m[12]*m[7]*m[9];
    inv[12] = -m[4]*m[9]*m[14] + m[4]*m[10]*m[13] + m[8]*m[5]*m[14]
             - m[8]*m[6]*m[13] - m[12]*m[5]*m[10] + m[12]*m[6]*m[9];
    inv[1] =  -m[1]*m[10]*m[15] + m[1]*m[11]*m[14] + m[9]*m[2]*m[15]
             - m[9]*m[3]*m[14] - m[13]*m[2]*m[11] + m[13]*m[3]*m[10];
    inv[5] =   m[0]*m[10]*m[15] - m[0]*m[11]*m[14] - m[8]*m[2]*m[15]
             + m[8]*m[3]*m[14] + m[12]*m[2]*m[11] - m[12]*m[3]*m[10];
    inv[9] =  -m[0]*m[9]*m[15] + m[0]*m[11]*m[13] + m[8]*m[1]*m[15]
             - m[8]*m[3]*m[13] - m[12]*m[1]*m[11] + m[12]*m[3]*m[9];
    inv[13] =  m[0]*m[9]*m[14] - m[0]*m[10]*m[13] - m[8]*m[1]*m[14]
             + m[8]*m[2]*m[13] + m[12]*m[1]*m[10] - m[12]*m[2]*m[9];
    inv[2] =   m[1]*m[6]*m[15] - m[1]*m[7]*m[14] - m[5]*m[2]*m[15]
             + m[5]*m[3]*m[14] + m[13]*m[2]*m[7] - m[13]*m[3]*m[6];
    inv[6] =  -m[0]*m[6]*m[15] + m[0]*m[7]*m[14] + m[4]*m[2]*m[15]
             - m[4]*m[3]*m[14] - m[12]*m[2]*m[7] + m[12]*m[3]*m[6];
    inv[10] =  m[0]*m[5]*m[15] - m[0]*m[7]*m[13] - m[4]*m[1]*m[15]
             + m[4]*m[3]*m[13] + m[12]*m[1]*m[7] - m[12]*m[3]*m[5];
    inv[14] = -m[0]*m[5]*m[14] + m[0]*m[6]*m[13] + m[4]*m[1]*m[14]
             - m[4]*m[2]*m[13] - m[12]*m[1]*m[6] + m[12]*m[2]*m[5];
    inv[3] =  -m[1]*m[6]*m[11] + m[1]*m[7]*m[10] + m[5]*m[2]*m[11]
             - m[5]*m[3]*m[10] - m[9]*m[2]*m[7] + m[9]*m[3]*m[6];
    inv[7] =   m[0]*m[6]*m[11] - m[0]*m[7]*m[10] - m[4]*m[2]*m[11]
             + m[4]*m[3]*m[10] + m[8]*m[2]*m[7] - m[8]*m[3]*m[6];
    inv[11] = -m[0]*m[5]*m[11] + m[0]*m[7]*m[9] + m[4]*m[1]*m[11]
             - m[4]*m[3]*m[9] - m[8]*m[1]*m[7] + m[8]*m[3]*m[5];
    inv[15] =  m[0]*m[5]*m[10] - m[0]*m[6]*m[9] - m[4]*m[1]*m[10]
             + m[4]*m[2]*m[9] + m[8]*m[1]*m[6] - m[8]*m[2]*m[5];

    det = m[0]*inv[0] + m[1]*inv[4] + m[2]*inv[8] + m[3]*inv[12];
    if (det == 0)
        return GL_FALSE;

    det = 1.0 / det;

    for (i = 0; i < 16; i++)
        invOut[i] = inv[i] * det;

    return GL_TRUE;
}



void fw_gluUnProject(GLDOUBLE winx, GLDOUBLE winy, GLDOUBLE winz,
		const GLDOUBLE modelMatrix[16],
		const GLDOUBLE projMatrix[16],
                const GLint viewport[4],
	        GLDOUBLE *objx, GLDOUBLE *objy, GLDOUBLE *objz)
{
	/* https://www.opengl.org/sdk/docs/man2/xhtml/gluUnProject.xml
	FLOPs 196 double: full matmult 64, full mat inverse 102, full transform 16, miscalaneous 8
	*/
    GLDOUBLE finalMatrix[16];
    GLDOUBLE in[4];
    GLDOUBLE out[4];

    __gluMultMatricesd(modelMatrix, projMatrix, finalMatrix);
    if (!__gluInvertMatrixd(finalMatrix, finalMatrix)) return;

    in[0]=winx;
    in[1]=winy;
    in[2]=winz;
    in[3]=1.0;

    /* Map x and y from window coordinates */
    in[0] = (in[0] - viewport[0]) / viewport[2];
    in[1] = (in[1] - viewport[1]) / viewport[3];

    /* Map to range -1 to 1 */
    in[0] = in[0] * 2 - 1;
    in[1] = in[1] * 2 - 1;
    in[2] = in[2] * 2 - 1;

    __gluMultMatrixVecd(finalMatrix, in, out);
    if (out[3] == 0.0) return;
    out[0] /= out[3];
    out[1] /= out[3];
    out[2] /= out[3];
    *objx = out[0];
    *objy = out[1];
    *objz = out[2];
}


void fw_Ortho (GLDOUBLE left, GLDOUBLE right, GLDOUBLE bottom, GLDOUBLE top, GLDOUBLE nearZ, GLDOUBLE farZ) {
	GLDOUBLE *dp;
	ppOpenGL_Utils p = (ppOpenGL_Utils)gglobal()->OpenGL_Utils.prv;

	/* do the glOrtho on the top of the stack, and send that along */
	dp = p->FW_ProjectionView[p->projectionviewTOS];

	/* do some bounds checking here */
	if (right <= left) right = left+1.0;   /* resolve divide by zero possibility */
	if (top <= bottom) top= bottom+1.0;    /* resolve divide by zero possibility */
	if (farZ <= nearZ) farZ= nearZ + 2.0;  /* resolve divide by zero possibility */

	/* {int i; for (i=0; i<16;i++) { printf ("ModView before  %d: %4.3f \n",i,dp[i]); } } */
	mesa_Ortho(left,right,bottom,top,nearZ,farZ,dp);

	 /* {int i; for (i=0; i<16;i++) { printf ("ModView after   %d: %4.3f \n",i,dp[i]); } } */

	FW_GL_LOADMATRIX(dp);
}


/* gluPerspective replacement */
void fw_gluPerspective(GLDOUBLE fovy, GLDOUBLE aspect, GLDOUBLE zNear, GLDOUBLE zFar) {
	GLDOUBLE xmin, xmax, ymin, ymax;

	GLDOUBLE *dp;
	GLDOUBLE ndp[16];
	GLDOUBLE ndp2[16];
	ppOpenGL_Utils p = (ppOpenGL_Utils)gglobal()->OpenGL_Utils.prv;



	ymax = zNear * tan(fovy * M_PI / 360.0);
	ymin = -ymax;
	xmin = ymin * aspect;
	xmax = ymax * aspect;

	/* do the glFrsutum on the top of the stack, and send that along */
	FW_GL_MATRIX_MODE(GL_PROJECTION);
	//FW_GL_LOAD_IDENTITY();
	dp = p->FW_ProjectionView[p->projectionviewTOS];

	mesa_Frustum(xmin, xmax, ymin, ymax, zNear, zFar, ndp);
	mattranspose(ndp2,ndp);

	//printmatrix2(ndp,"ndp");
	//printmatrix2(ndp2,"ndp2 = transpose(ndp)");
	//JAS printmatrix2(dp,"dp");

	matmultiplyFULL(ndp,ndp2,dp);

	//printmatrix2(ndp,"ndp = ndp2*dp");

	/* method = 1; */
	#define TRY_PERSPECTIVE_METHOD_1
	#ifdef TRY_PERSPECTIVE_METHOD_1
	  	FW_GL_LOADMATRIX(ndp);
		/* put the matrix back on our matrix stack */
		memcpy (p->FW_ProjectionView[p->projectionviewTOS],ndp,16*sizeof (GLDOUBLE));
	#endif


	#ifdef TRY_PERSPECTIVE_METHOD_2
/* testing... */
{
	GLDOUBLE m[16];
    GLDOUBLE sine, cotangent, deltaZ;
    GLDOUBLE radians = fovy / 2.0 * M_PI / 180.0;

    deltaZ = zFar - zNear;
    sine = sin(radians);
    if ((deltaZ == 0) || (sine == 0) || (aspect == 0)) {
        return;
    }
    cotangent = cos(radians) / sine;

	loadIdentityMatrix(m); //(&m);
    //__gluMakeIdentityd(&m[0][0]);
    m[0*4+0] = cotangent / aspect;
    m[1*4+1] = cotangent;
    m[2*4+2] = -(zFar + zNear) / deltaZ;
    m[2*4+3] = -1;
    m[3*4+2] = -2 * zNear * zFar / deltaZ;
    m[3*4+3] = 0;
	matmultiplyFULL(m,m,dp);
	if(method==2)
	  FW_GL_LOADMATRIX(m);

    //glMultMatrixd(&m[0][0]);
}
	#endif


	#ifdef TRY_PERSPECTIVE_METHOD_3
	{
	GLDOUBLE yyy[16];
//printf ("fw_gluPerspective, have...\n");

	if(method==3)
	  gluPerspective(fovy,aspect,zNear,zFar);
	FW_GL_GETDOUBLEV(GL_PROJECTION_MATRIX,yyy);
	//JAS printmatrix2(dp,"dp orig");
	//JAS printmatrix2(ndp,"ndp myPup");
	//JAS printmatrix2(yyy,"yyy gluP");
	//JAS printmatrix2(m,"m mesa");
	//for (i=0; i<16;i++) {printf ("%d orig: %5.2lf myPup: %5.2lf gluP: %5.2lf mesa %5.2lf\n",i,dp[i],
	//	ndp[i],yyy[i],m[i]);
	//}
	}
	#endif

}



/* gluPickMatrix replacement */
void fw_gluPickMatrix(GLDOUBLE xx, GLDOUBLE yy, GLDOUBLE width, GLDOUBLE height, GLint *vp) {
	#ifdef VERBOSE
	printf ("PickMat %lf %lf %lf %lf %d %d %d %d\n",xx,yy,width,height,vp[0], vp[1],vp[2],vp[3]);
	#endif

	if ((width < 0.0) || (height < 0.0)) return;
	/* Translate and scale the picked region to the entire window */
	FW_GL_TRANSLATE_D((vp[2] - 2.0 * (xx - vp[0])) / width, (vp[3] - 2.0 * (yy - vp[1])) / height, 0.0);
	FW_GL_SCALE_D(vp[2] / width, vp[3] / height, 1.0);

}


/* glFrustum replacement - taken from the MESA source;

 * matrix.c
 *
 * Some useful matrix functions.
 *
 * Brian Paul
 * 10 Feb 2004
 */

/**
 * Build a glFrustum matrix.
 */

static void
mesa_Frustum(GLDOUBLE left, GLDOUBLE right, GLDOUBLE bottom, GLDOUBLE top, GLDOUBLE nearZ, GLDOUBLE farZ, GLDOUBLE *m)
{
 /* http://www.songho.ca/opengl/gl_projectionmatrix.html shows derivation*/
   GLDOUBLE x = (2.0*nearZ) / (right-left);
   GLDOUBLE y = (2.0*nearZ) / (top-bottom);
   GLDOUBLE a = (right+left) / (right-left);
   GLDOUBLE b = (top+bottom) / (top-bottom);
   GLDOUBLE c = -(farZ+nearZ) / ( farZ-nearZ);
   GLDOUBLE d = -(2.0F*farZ*nearZ) / (farZ-nearZ);

	/* printf ("mesa_Frustum (%lf, %lf, %lf, %lf, %lf, %lf)\n",left,right,bottom,top,nearZ, farZ); */
	m[0] = x;
	m[1] = 0.0;
	m[2] = a;
	m[3] = 0.0;

	m[4] = 0.0;
	m[5] = y;
	m[6] = b;
	m[7] = 0.0;

	m[8] = 0.0;
	m[9] = 0.0;
	m[10] = c;
	m[11] = d;

	m[12] = 0.0;
	m[13] = 0.0;
	m[14] = -1.0;
	m[15] = 0.0;
/*

#define M(row,col)  m[col*4+row]
   M(0,0) = x;     M(0,1) = 0.0F;  M(0,2) = a;      M(0,3) = 0.0F;
   M(1,0) = 0.0F;  M(1,1) = y;     M(1,2) = b;      M(1,3) = 0.0F;
   M(2,0) = 0.0F;  M(2,1) = 0.0F;  M(2,2) = c;      M(2,3) = d;
   M(3,0) = 0.0F;  M(3,1) = 0.0F;  M(3,2) = -1.0F;  M(3,3) = 0.0F;
#undef M
*/
}

/**
 * Build a glOrtho marix.
 */
static void
mesa_Ortho(GLDOUBLE left, GLDOUBLE right, GLDOUBLE bottom, GLDOUBLE top, GLDOUBLE nearZ, GLDOUBLE farZ, GLDOUBLE *m)
{
#define M(row,col)  m[col*4+row]
   M(0,0) = 2.0F / (right-left);
   M(0,1) = 0.0F;
   M(0,2) = 0.0F;
   M(0,3) = -(right+left) / (right-left);

   M(1,0) = 0.0F;
   M(1,1) = 2.0F / (top-bottom);
   M(1,2) = 0.0F;
   M(1,3) = -(top+bottom) / (top-bottom);

   M(2,0) = 0.0F;
   M(2,1) = 0.0F;
   M(2,2) = -2.0F / (farZ-nearZ);
   M(2,3) = -(farZ+nearZ) / (farZ-nearZ);

   M(3,0) = 0.0F;
   M(3,1) = 0.0F;
   M(3,2) = 0.0F;
   M(3,3) = 1.0F;
#undef M
}
