/*

  FreeWRL support library.
  Scenegraph rendering.

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
#include <display.h>
#include <internal.h>

#include <libFreeWRL.h>

#include "../vrml_parser/Structs.h"
#include "../main/headers.h"
#include "../opengl/Textures.h"
#include "../scenegraph/Component_ProgrammableShaders.h"

#include "Polyrep.h"
#include "Collision.h"
#include "../scenegraph/quaternion.h"
#include "Viewer.h"
#include "LinearAlgebra.h"
#include "../input/SensInterps.h"
#include "system_threads.h"
#include "threads.h"

#include "../opengl/OpenGL_Utils.h"
#include "../scenegraph/Component_Shape.h"
#include "RenderFuncs.h"


typedef float shaderVec4[4];


struct profile_entry {
	char *name;
	double start;
	double accum;
	int hits;
};

struct point_XYZ3 {
	struct point_XYZ p1;
	struct point_XYZ p2;
	struct point_XYZ p3;
};

typedef struct pRenderFuncs{
	int profile_entry_count;
	struct profile_entry profile_entries[100];
	int profiling_on;
	float light_linAtten[MAX_LIGHT_STACK];
	float light_constAtten[MAX_LIGHT_STACK];
	float light_quadAtten[MAX_LIGHT_STACK];
	float light_spotCutoffAngle[MAX_LIGHT_STACK];
	float light_spotBeamWidth[MAX_LIGHT_STACK];
	shaderVec4 light_amb[MAX_LIGHT_STACK];
	shaderVec4 light_dif[MAX_LIGHT_STACK];
	shaderVec4 light_pos[MAX_LIGHT_STACK];
	shaderVec4 light_spec[MAX_LIGHT_STACK];
	shaderVec4 light_spotDir[MAX_LIGHT_STACK];
    float light_radius[MAX_LIGHT_STACK];
	GLint lightType[MAX_LIGHT_STACK]; //0=point 1=spot 2=directional
	/* Rearrange to take advantage of headlight when off */
	int nextFreeLight;// = 0;
	unsigned int currentLoop;
	unsigned int lastLoop;
	unsigned int sendCount;
	//int firstLight;//=0;
	/* lights status. Light HEADLIGHT_LIGHT is the headlight */
	GLint lightOnOff[MAX_LIGHT_STACK];
	GLint lightChanged[MAX_LIGHT_STACK]; //optimization
	GLint lastShader;
	//int cur_hits;//=0;
	void *empty_group;//=0;
	//struct point_XYZ ht1, ht2; not used
	struct point_XYZ hyper_r1,hyper_r2; /* Transformed ray for the hypersensitive node */
	struct currayhit rayph;
	struct X3D_Node *rootNode;//=NULL;	/* scene graph root node */
	struct Vector *libraries; //vector of extern proto library scenes in X3D_Proto format that are parsed shallow (not instanced scenes) - the library protos will be in X3D_Proto->protoDeclares vector
	struct X3D_Anchor *AnchorsAnchor;// = NULL;
	struct currayhit rayHit,rayHitHyper;
	struct trenderstate renderstate;
	int renderLevel;

	// which Shader is currently in use?
	GLint currentShader;
	Stack *render_geom_stack;
	Stack *sensor_stack;
	Stack *ray_stack;
}* ppRenderFuncs;
void *RenderFuncs_constructor(){
	void *v = MALLOCV(sizeof(struct pRenderFuncs));
	memset(v,0,sizeof(struct pRenderFuncs));
	return v;
}
void RenderFuncs_init(struct tRenderFuncs *t){
	//public

	t->BrowserAction = FALSE;
	//	t->hitPointDist; /* distance in ray: 0 = r1, 1 = r2, 2 = 2*r2-r1... */
	///* used to save rayhit and hyperhit for later use by C functions */
	//t->hyp_save_posn;
	//t->hyp_save_norm;t->ray_save_posn;
	t->hypersensitive = 0;
	t->hyperhit = 0;
	t->have_transparency=FALSE;/* did any Shape have transparent material? */
	/* material node usage depends on texture depth; if rgb (depth1) we blend color field
	   and diffusecolor with texture, else, we dont bother with material colors */
	t->last_texture_type = NOTEXTURE;
	t->usingAffinePickmatrix = 1; /*use AFFINE matrix to transform points to align with pickray, instead of using GLU_UNPROJECT, feature-AFFINE_GLU_UNPROJECT*/

	//private
	t->prv = RenderFuncs_constructor();
	{
		ppRenderFuncs p = (ppRenderFuncs)t->prv;
		p->profile_entry_count = 0;
		p->profiling_on = 0; //toggle on with '.' on keyboard
		/* which arrays are enabled, and defaults for each array */
		/* Rearrange to take advantage of headlight when off */
		p->nextFreeLight = 0;
		//p->firstLight = 0;
		//p->cur_hits=0;
		p->empty_group=0;
		p->rootNode=NULL;	/* scene graph root node */
		p->libraries=newVector(void3 *,1);
		p->AnchorsAnchor = NULL;
		t->rayHit = (void *)&p->rayHit;
		t->rayHitHyper = (void *)&p->rayHitHyper;
		p->renderLevel = 0;
		p->lastShader = -1;
		p->currentLoop = 0;
		p->lastLoop = 10000000;
		p->sendCount = 0;
		p->render_geom_stack = newStack(int);
		p->sensor_stack = newStack(struct currayhit);
		p->ray_stack = newStack(struct point_XYZ3);

	}

	//setLightType(HEADLIGHT_LIGHT,2); // ensure that this is a DirectionalLight.
}
void unload_libraryscenes();
void RenderFuncs_clear(struct tRenderFuncs *t){
	ppRenderFuncs p = (ppRenderFuncs)t->prv;
	unload_libraryscenes();
	deleteVector(void3 *,p->libraries);
	deleteVector(int,p->render_geom_stack);
	deleteVector(struct currayhit,p->sensor_stack);
	deleteVector(struct point_XYZ3,p->ray_stack);
}
void unload_libraryscenes(){
	ppRenderFuncs p = (ppRenderFuncs)gglobal()->RenderFuncs.prv;
	//freeing these library scenes should be done during exit procedures before gglobal gc, perhaps in 
	// finalizeRenderSceneUpdateScene
	// or perhaps when changing scenes. Perhaps libraries should be in a Scene context. 
	// One old idea not implemented: all scenes should first be parsed to libraryScene (nothing registered, empty protoInstance bodies)
	// then scene instanced like a proto. That would speed up Anchoring between scene files ie between rooms. 
	// (Avatar state would be carried between scenes in browser key,value attributes like metadata
	if(p->libraries){
		int i;
		for(i=0;i<vectorSize(p->libraries);i++){
			struct X3D_Proto *libscn;
			char *url;
			resource_item_t *res;
			void3 *ul;
			ul = vector_get(struct void3*,p->libraries,i);
			if(ul){
				url = (char *)ul->one;
				libscn = (struct X3D_Proto*) ul->two;
				res = (resource_item_t*)ul->three;
				//unload_broto(libscn); //nothing to un-register - library scenes aren't registered
				gc_broto_instance(libscn);
				deleteVector(struct X3D_Node*,libscn->_parentVector);
				freeMallocedNodeFields(libscn);
				FREE_IF_NZ(libscn);
				FREE_IF_NZ(url);
				FREE_IF_NZ(ul);
				//FREE_IF_NZ(res);
				vector_set(struct void3*,p->libraries,i,NULL);
			}
		}
		p->libraries->n = 0;
	}
}
void clearLightTable(){ //unsigned int loop_count){
	//int i;
	ppRenderFuncs p = (ppRenderFuncs)gglobal()->RenderFuncs.prv;
	p->nextFreeLight = 0;
	//p->currentLoop = loop_count;
	p->sendCount = 0;
	//for(i=0;i<MAX_LIGHT_STACK;i++){
	//	p->lightChanged[i] = 0;
	//}
}
/* we assume max MAX_LIGHTS lights. The max light is the Headlight, so we go through 0-HEADLIGHT_LIGHT for Lights */
int nextlight() {
	ppRenderFuncs p = (ppRenderFuncs)gglobal()->RenderFuncs.prv;
	int rv = p->nextFreeLight;
	if(rv == HEADLIGHT_LIGHT) { 
		return -1; 
	}
	p->lightChanged[rv] = 0;
	p->nextFreeLight ++;
	return rv;
}

/* lightType 0=point 1=spot 2=directional */
void setLightType(GLint light, int type) {
	ppRenderFuncs p = (ppRenderFuncs)gglobal()->RenderFuncs.prv;
    p->lightType[light] = type;
}
void setLightChangedFlag(GLint light) {
	ppRenderFuncs p = (ppRenderFuncs)gglobal()->RenderFuncs.prv;
    p->lightChanged[light] = 1;
}

/* keep track of lighting */
void setLightState(GLint light, int status) {
	ppRenderFuncs p = (ppRenderFuncs)gglobal()->RenderFuncs.prv;
    //ConsoleMessage ("start lightState, light %d, status %d\n",light,status);

    
    PRINT_GL_ERROR_IF_ANY("start lightState");

	if (light<0) return; /* nextlight will return -1 if too many lights */
	p->lightOnOff[light] = status;
    PRINT_GL_ERROR_IF_ANY("end lightState");
}

/* for local lights, we keep track of what is on and off */
void saveLightState2(int *last) {
	ppRenderFuncs p = (ppRenderFuncs)gglobal()->RenderFuncs.prv;
	*last = p->nextFreeLight;
} 

void restoreLightState2(int last) {
	ppRenderFuncs p = (ppRenderFuncs)gglobal()->RenderFuncs.prv;
	p->nextFreeLight = last;
}


void transformLightToEye(float *pos, float* dir)
{
	int i;
    GLDOUBLE modelMatrix[16], *b;
	float *a;
	shaderVec4 aux, auxt;
    FW_GL_GETDOUBLEV(GL_MODELVIEW_MATRIX, modelMatrix);

/*
ConsoleMessage ("nvm %4.2f %4.2f %4.2f %4.2f",modelMatrix[0],modelMatrix[1],modelMatrix[2],modelMatrix[3]);
ConsoleMessage ("nvm %4.2f %4.2f %4.2f %4.2f",modelMatrix[4],modelMatrix[5],modelMatrix[6],modelMatrix[7]);
ConsoleMessage ("nvm %4.2f %4.2f %4.2f %4.2f",modelMatrix[8],modelMatrix[9],modelMatrix[10],modelMatrix[11]);
ConsoleMessage ("nvm %4.2f %4.2f %4.2f %4.2f",modelMatrix[12],modelMatrix[13],modelMatrix[14],modelMatrix[15]);
*/

    /* pre-multiply the light position, as per the orange book, page 216,
     "OpenGL specifies that light positions are transformed by the modelview
     matrix when they are provided to OpenGL..." */
    /* DirectionalLight?  PointLight, SpotLight? */

	// assumes pos[3] = 0.0; only use first 3 of these numbers
	transformf(auxt,pos,modelMatrix);
	auxt[3] = 0.0;

/*
ConsoleMessage("LightToEye, after transformf, auxt %4.2f %4.2f %4.2f %4.2f, pos %4.2f %4.2f %4.2f %4.2f",
auxt[0],auxt[1],auxt[2],auxt[3],
pos[0],pos[1],pos[2],pos[3]);
*/

	for(i=0;i<4;i++){
		pos[i] = auxt[i];
	}
	b = modelMatrix;
	a = dir;
	aux[0] = (float) (b[0]*a[0] +b[4]*a[1] +b[8]*a[2] );
	aux[1] = (float) (b[1]*a[0] +b[5]*a[1] +b[9]*a[2] );
	aux[2] = (float) (b[2]*a[0] +b[6]*a[1] +b[10]*a[2]);
	for(i=0;i<3;i++)
		dir[i] = aux[i];

	// just initialize this to 0.0
	dir[3] = 0.0;

}

void fwglLightfv (int light, int pname, GLfloat *params) {
	ppRenderFuncs p = (ppRenderFuncs)gglobal()->RenderFuncs.prv;
	/*printf ("fwglLightfv light: %d ",light);
	switch (pname) {
		case GL_AMBIENT: printf ("GL_AMBIENT"); break;
		case GL_DIFFUSE: printf ("GL_DIFFUSE"); break;
		case GL_POSITION: printf ("GL_POSITION"); break;
		case GL_SPECULAR: printf ("GL_SPECULAR"); break;
		case GL_SPOT_DIRECTION: printf ("GL_SPOT_DIRECTION"); break;
        case GL_LIGHT_RADIUS: printf ("GL_LIGHT_RADIUS"); break;
	}
	printf (" %f %f %f %f\n",params[0], params[1],params[2],params[3]);
     */
    
	//printLTDebug(__FILE__,__LINE__);


	switch (pname) {
		case GL_AMBIENT:
			memcpy ((void *)p->light_amb[light],(void *)params,sizeof(shaderVec4));
			break;
		case GL_DIFFUSE:
			memcpy ((void *)p->light_dif[light],(void *)params,sizeof(shaderVec4));
			break;
		case GL_POSITION:
			memcpy ((void *)p->light_pos[light],(void *)params,sizeof(shaderVec4));
			//the following function call assumes spotdir has already been set - set it first from render_light

/*
ConsoleMessage("fwglLightfv - NOT transforming pos %3.2f %3.2f %3.2f %3.2f spd %3.2f %3.2f %3.2f %3.2f",
			p->light_pos[light][0],
			p->light_pos[light][1],
			p->light_pos[light][2],
			p->light_pos[light][3],
			p->light_spotDir[light][0],
			p->light_spotDir[light][1],
			p->light_spotDir[light][2],
			p->light_spotDir[light][3]);
*/
			if (light != HEADLIGHT_LIGHT)  transformLightToEye(p->light_pos[light], p->light_spotDir[light]);
			break;
		case GL_SPECULAR:
			memcpy ((void *)p->light_spec[light],(void *)params,sizeof(shaderVec4));
			break;
		case GL_SPOT_DIRECTION:
			//call spot_direction before spot_position, so direction gets transformed above in spot position
			memcpy ((void *)p->light_spotDir[light],(void *)params,sizeof(shaderVec4));
			break;
		default: {printf ("help, unknown fwgllightfv param %d\n",pname);}
	}
}

void fwglLightf (int light, int pname, GLfloat param) {
	ppRenderFuncs p = (ppRenderFuncs)gglobal()->RenderFuncs.prv;

#ifdef RENDERVERBOSE
	printf ("fwglLightf light: %d ",light);
	switch (pname) {
		case GL_CONSTANT_ATTENUATION: printf ("GL_CONSTANT_ATTENUATION"); break;
		case GL_LINEAR_ATTENUATION: printf ("GL_LINEAR_ATTENUATION"); break;
		case GL_QUADRATIC_ATTENUATION: printf ("GL_QUADRATIC_ATTENUATION"); break;
		case GL_SPOT_CUTOFF: printf ("GL_SPOT_CUTOFF"); break;
		case GL_SPOT_BEAMWIDTH: printf ("GL_SPOT_BEAMWIDTH"); break;
	}
	printf (" %f\n",param);
#endif
	
    
	switch (pname) {
		case GL_CONSTANT_ATTENUATION:
			p->light_constAtten[light] = param;
			break;
		case GL_LINEAR_ATTENUATION:
			p->light_linAtten[light] = param;
			break;
		case GL_QUADRATIC_ATTENUATION:
			p->light_quadAtten[light] = param;
			break;
		case GL_SPOT_CUTOFF:
			p->light_spotCutoffAngle[light] = param;
            //ConsoleMessage ("setting light_spotCutoffAngle for %d to %f\n",light,param);
			break;
		case GL_SPOT_BEAMWIDTH:
			p->light_spotBeamWidth[light] = param;
            //ConsoleMessage ("setting light_spotBeamWidth for %d to %f\n",light,param);

			break;
        case GL_LIGHT_RADIUS:
            p->light_radius[light] = param;
            break;

		default: {printf ("help, unknown fwgllightfv param %d\n",pname);}
	}
}


/* send light info into Shader. if OSX gets glGetUniformBlockIndex calls, we can do this with 1 call 
	On old pentium with old board in old PCI slot, 8 lights take 1050bytes and 12% of mainloop load
	3 optimizations reduce the light sending traffic:
	1. send only active lights 
		-Android had a problem on startup with this, I changed the flavor a bit - lets see if it works now
		-cuts from 12% of loop to 4%
	2. for an active light, send only parameters that light type needs in the shader calc
		-cuts from 4% of loop to 3%
	3. if the active light set and shader haven't changed since last shape, don't resend any lights.
		(active light sets can change during scene graph traversal. But a light itself doesn't change
		settings during traversal. Just during routing and scripting. So in render_heir 
		lastShader is set to -1 to trigger a fresh send.
		- cuts from 3% of loop to .5%

*/
void sendLightInfo (s_shader_capabilities_t *me) {
	ppRenderFuncs p = (ppRenderFuncs)gglobal()->RenderFuncs.prv;
    int i,j, lightcount, lightsChanged;
	int lightIndexesToSend[MAX_LIGHTS];
    		
	// in case we are trying to render a node that has just been killed...
	if (me==NULL) return;

	PRINT_GL_ERROR_IF_ANY("BEGIN sendLightInfo");
	/* if one of these are equal to -1, we had an error in the shaders... */
	//Optimization 3>> if the shader and lights haven't changed since the last shape,
	//then don't resend the lights to the shader
	if(0){
		lightsChanged = FALSE;
		for(i=0;i<MAX_LIGHT_STACK;i++){
			if(p->lightChanged[i]) lightsChanged = TRUE;
		}
		if(!lightsChanged && (p->currentShader == p->lastShader)) 
			return;
		p->lastShader = p->currentShader;
		//p->lastLoop = p->currentLoop;
		//p->sendCount++;
	}
	//<<end optimization 3
	profile_start("sendlight");
	//GLUNIFORM1I(me->lightcount,lightcount);
	//GLUNIFORM1IV(me->lightState,MAX_LIGHTS,p->lightOnOff); //don't need with lightcount
	//GLUNIFORM1IV(me->lightType,MAX_LIGHTS,p->lightType); //need to pack into light struct
    //GLUNIFORM1FV(me->lightRadius,MAX_LIGHTS,p->light_radius); //need to pack into lightstruct
	PRINT_GL_ERROR_IF_ANY("MIDDLE1 sendLightInfo");
    
    // send in lighting info, but only for lights that are "on"
	// reason: at 1100+ bytes per shape for 8 lights, it takes up 11.2% of mainloop activity on an old pentium
	// so this cuts it down to about 200 bytes per shape if you have a headlight and another light.

	lightcount = 0;
	lightsChanged = FALSE;
	//by looping from the top down, we'll give headlight first chance,
	//then local lights pushed onto the stack
	//then global lights last chance
	for(i=MAX_LIGHT_STACK-1;i>-1;i--){
		if(i==HEADLIGHT_LIGHT || i<p->nextFreeLight){
			if (p->lightOnOff[i]){
				lightIndexesToSend[lightcount] = i;
				lightcount++;
				lightsChanged = lightsChanged || p->lightChanged[i];
				if(lightcount >= MAX_LIGHTS) break;
			}
		}
	}
	if(!lightsChanged && (p->currentShader == p->lastShader)) 
			return;
	p->lastShader = p->currentShader;
    for (j=0;j<lightcount; j++) {
		i = lightIndexesToSend[j];
		p->lightChanged[i] = 0;
		// this causes initial screen on Android to fail.
		// dug9 - I added another parameter lightcount above and in ADSL shader
		// and pack the lights ie. moving headlight up here so its at 
		// lightcount-1 instead of MAX_LIGHTS-1 on the GPU.
		// LMK if breaks android
		//0 - pointlight
		//1 - spotlight
		//2 - directionlight
		//save a bit of bandwidth by not sending unused parameters for a light type
		if(p->lightType[i]<2 ){ //not direction
			shaderVec4 light_Attenuations;
			light_Attenuations[0] = p->light_constAtten[i];
			light_Attenuations[1] = p->light_linAtten[i];
			light_Attenuations[2] = p->light_quadAtten[i];
			GLUNIFORM3FV(me->lightAtten[j],1,light_Attenuations);
			//GLUNIFORM1F (me->lightConstAtten[j], p->light_constAtten[i]);
			//GLUNIFORM1F (me->lightLinAtten[j], p->light_linAtten[i]);
			//GLUNIFORM1F(me->lightQuadAtten[j], p->light_quadAtten[i]);
		}
		if(p->lightType[i]==1 ){ //spot
			GLUNIFORM1F(me->lightSpotCutoffAngle[j], p->light_spotCutoffAngle[i]);
			GLUNIFORM1F(me->lightSpotBeamWidth[j], p->light_spotBeamWidth[i]);
		}
		if(p->lightType[i]==0){ //point
			GLUNIFORM1F(me->lightRadius[j],p->light_radius[i]);
		}
		GLUNIFORM4FV(me->lightSpotDir[j],1, p->light_spotDir[i]);
		GLUNIFORM4FV(me->lightPosition[j],1,p->light_pos[i]);
		GLUNIFORM4FV(me->lightAmbient[j],1,p->light_amb[i]);
		GLUNIFORM4FV(me->lightDiffuse[j],1,p->light_dif[i]);
		GLUNIFORM4FV(me->lightSpecular[j],1,p->light_spec[i]);
		GLUNIFORM1I(me->lightType[j],p->lightType[i]);
    }
	GLUNIFORM1I(me->lightcount,lightcount);

	profile_end("sendlight");
    PRINT_GL_ERROR_IF_ANY("END sendLightInfo");
}

/* finished rendering thisshape. */
void finishedWithGlobalShader(void) {
    //printf ("finishedWithGlobalShader\n");


    /* get rid of the shader */
    getAppearanceProperties()->currentShaderProperties = NULL;
FW_GL_BINDBUFFER(GL_ARRAY_BUFFER, 0);

FW_GL_BINDBUFFER(GL_ELEMENT_ARRAY_BUFFER, 0);

}


/* should the system need to rebuild the OpenGL system (eg, Android, 
on restore of screen, iPhone?? Blackberry???) we ensure that the system
state is such that new information will get cached */

void resetGlobalShader() {
	ppRenderFuncs p = (ppRenderFuncs)gglobal()->RenderFuncs.prv;

	//ConsoleMessage ("resetGlobalShader called");

	/* no shader currently active */
	p->currentShader = 0;
}

void restoreGlobalShader(){
	ppRenderFuncs p = (ppRenderFuncs)gglobal()->RenderFuncs.prv;
    if (p->currentShader)
		USE_SHADER(p->currentShader);
}
/* choose and turn on a shader for this geometry */

void enableGlobalShader(s_shader_capabilities_t *myShader) {
	ppRenderFuncs p = (ppRenderFuncs)gglobal()->RenderFuncs.prv;

    //ConsoleMessage ("enableGlobalShader, have myShader %d",myShader->myShaderProgram);
    if (myShader == NULL) {
        finishedWithGlobalShader(); 
        return;
    };
    
    
    getAppearanceProperties()->currentShaderProperties = myShader;
    if (myShader->myShaderProgram != p->currentShader) {
		USE_SHADER(myShader->myShaderProgram);
		p->currentShader = myShader->myShaderProgram;
	}
}


/* send in vertices, normals, etc, etc... to either a shader or via older opengl methods */
void sendAttribToGPU(int myType, int dataSize, int dataType, int normalized, int stride, float *pointer, char *file, int line){

    s_shader_capabilities_t *me = getAppearanceProperties()->currentShaderProperties;

	// checking to see that we really have the data
	if (me==NULL) return;

#ifdef RENDERVERBOSE

ConsoleMessage ("sendAttribToGPU, getAppearanceProperties()->currentShaderProperties %p\n",getAppearanceProperties()->currentShaderProperties);
ConsoleMessage ("myType %d, dataSize %d, dataType %d, stride %d\n",myType,dataSize,dataType,stride);
	if (me != NULL) {
		switch (myType) {
			case FW_NORMAL_POINTER_TYPE:
				ConsoleMessage ("glVertexAttribPointer  Normals %d at %s:%d\n",me->Normals,file,line);
				break;
			case FW_VERTEX_POINTER_TYPE:
				ConsoleMessage ("glVertexAttribPointer  Vertexs %d at %s:%d\n",me->Vertices,file,line);
				break;
			case FW_COLOR_POINTER_TYPE:
				ConsoleMessage ("glVertexAttribPointer  Colours %d at %s:%d\n",me->Colours,file,line);
				break;
			case FW_TEXCOORD_POINTER_TYPE:
				ConsoleMessage ("glVertexAttribPointer  TexCoords %d at %s:%d\n",me->TexCoords,file,line);
				break;

			default : {ConsoleMessage ("sendAttribToGPU, unknown type in shader\n");}
		}
	}
#endif
#undef RENDERVERBOSE

	switch (myType) {
		case FW_NORMAL_POINTER_TYPE:
		if (me->Normals != -1) {
			glEnableVertexAttribArray(me->Normals);
			glVertexAttribPointer(me->Normals, 3, dataType, normalized, stride, pointer);
		}
			break;
		case FW_VERTEX_POINTER_TYPE:
		if (me->Vertices != -1) {
			glEnableVertexAttribArray(me->Vertices);
			glVertexAttribPointer(me->Vertices, dataSize, dataType, normalized, stride, pointer);
		}
			break;
		case FW_COLOR_POINTER_TYPE:
		if (me->Colours != -1) {
			glEnableVertexAttribArray(me->Colours);
			glVertexAttribPointer(me->Colours, dataSize, dataType, normalized, stride, pointer);
		}
			break;
		case FW_TEXCOORD_POINTER_TYPE:
		if (me->TexCoords != -1) {
			glEnableVertexAttribArray(me->TexCoords);
			glVertexAttribPointer(me->TexCoords, dataSize, dataType, normalized, stride, pointer);
		}
			break;

		default : {printf ("sendAttribToGPU, unknown type in shader\n");}
	}
}


void sendBindBufferToGPU (GLenum target, GLuint buffer, char *file, int line) {

	
/*
	if (target == GL_ARRAY_BUFFER_BINDING) printf ("glBindBuffer, GL_ARRAY_BUFFER_BINDING %d at %s:%d\n",buffer,file,line);
	else if (target == GL_ARRAY_BUFFER) printf ("glBindBuffer, GL_ARRAY_BUFFER %d at %s:%d\n",buffer,file,line);
	else if (target == GL_ELEMENT_ARRAY_BUFFER) printf ("glBindBuffer, GL_ELEMENT_ARRAY_BUFFER %d at %s:%d\n",buffer,file,line);
	else printf ("glBindBuffer, %d %d at %s:%d\n",target,buffer,file,line);
	
*/

	glBindBuffer(target,buffer);
}



static bool setupShader() {

    s_shader_capabilities_t *mysp = getAppearanceProperties()->currentShaderProperties;

PRINT_GL_ERROR_IF_ANY("BEGIN setupShader");
	if (mysp == NULL) return FALSE;
        

		/* if we had a shader compile problem, do not draw */
		if (!(mysp->compiledOK)) {
#ifdef RENDERVERBOSE
			printf ("shader compile error\n");
#endif
			PRINT_GL_ERROR_IF_ANY("EXIT(false) setupShader");
			return false;
		}
       
#ifdef RENDERVERBOSE
        printf ("setupShader, we have Normals %d Vertices %d Colours %d TexCoords %d \n",
                mysp->Normals,
                mysp->Vertices,
                mysp->Colours,
                mysp->TexCoords);
           
#endif

        
        /* send along lighting, material, other visible properties */
        sendMaterialsToShader(mysp);
        sendMatriciesToShader(mysp);
    
    return true;
    
}


void sendArraysToGPU (int mode, int first, int count) {
#ifdef RENDERVERBOSE
	printf ("sendArraysToGPU start\n"); 
#endif


	// when glDrawArrays bombs it's usually some function left an array
	// enabled that's not supposed to be - try disabling something
//glDisableClientState(GL_VERTEX_ARRAY);
//glDisableClientState(GL_NORMAL_ARRAY);
//glDisableClientState(GL_INDEX_ARRAY);
//glDisableClientState(GL_COLOR_ARRAY);
//glDisableClientState(GL_SECONDARY_COLOR_ARRAY);
//glDisableClientState(GL_FOG_COORDINATE_ARRAY);
//glDisableClientState(GL_TEXTURE_COORD_ARRAY);
//glDisableClientState(GL_EDGE_FLAG_ARRAY);

	if (setupShader()){
		profile_start("draw_arr");
		glDrawArrays(mode,first,count);
		profile_end("draw_arr");
	}
    #ifdef RENDERVERBOSE
	printf ("sendArraysToGPU end\n"); 
    #endif
}



void sendElementsToGPU (int mode, int count, ushort *indices) {
    #ifdef RENDERVERBOSE
	printf ("sendElementsToGPU start\n"); 
    #endif
    
	if (setupShader()){
		profile_start("draw_el");
        glDrawElements(mode,count,GL_UNSIGNED_SHORT,indices);
		profile_end("draw_el");
	}

	#ifdef RENDERVERBOSE
	printf ("sendElementsToGPU finish\n"); 
	#endif
}


void initializeLightTables() {
	int i;
        float pos[] = { 0.0f, 0.0f, 1.0f, 0.0f };
        float dif[] = { 1.0f, 1.0f, 1.0f, 1.0f };
        float shin[] = { 0.0f, 0.0f, 0.0f, 1.0f }; /* light defaults - headlight is here, too */
        float As[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	ppRenderFuncs p = (ppRenderFuncs)gglobal()->RenderFuncs.prv;

      PRINT_GL_ERROR_IF_ANY("start of initializeightTables");

	for(i=0; i<MAX_LIGHT_STACK; i++) {
                p->lightOnOff[i] = TRUE;
                setLightState(i,FALSE);
            
		FW_GL_LIGHTFV(i, GL_SPOT_DIRECTION, pos);
       		FW_GL_LIGHTFV(i, GL_POSITION, pos);
       		FW_GL_LIGHTFV(i, GL_AMBIENT, As);
       		FW_GL_LIGHTFV(i, GL_DIFFUSE, dif);
       		FW_GL_LIGHTFV(i, GL_SPECULAR, shin);
         	FW_GL_LIGHTF(i, GL_CONSTANT_ATTENUATION,1.0f);
       		FW_GL_LIGHTF(i, GL_LINEAR_ATTENUATION,0.0f);
       		FW_GL_LIGHTF(i, GL_QUADRATIC_ATTENUATION,0.0f);
       		FW_GL_LIGHTF(i, GL_SPOT_CUTOFF,0.0f);
       		FW_GL_LIGHTF(i, GL_SPOT_BEAMWIDTH,0.0f);
           	FW_GL_LIGHTF(i, GL_LIGHT_RADIUS, 100000.0); /* just make it large for now*/ 
            
            	PRINT_GL_ERROR_IF_ANY("initizlizeLight2.10");
        }
        setLightState(HEADLIGHT_LIGHT, TRUE);

#ifdef OLDCODE
OLDCODE	#ifndef GL_ES_VERSION_2_0
OLDCODE        FW_GL_LIGHTMODELI(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SEPARATE_SPECULAR_COLOR);
OLDCODE        FW_GL_LIGHTMODELI(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
OLDCODE        FW_GL_LIGHTMODELI(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_FALSE);
OLDCODE        FW_GL_LIGHTMODELI(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
OLDCODE        FW_GL_LIGHTMODELFV(GL_LIGHT_MODEL_AMBIENT,As);
OLDCODE	#else
OLDCODE	//printf ("skipping light setups\n");
OLDCODE	#endif
#endif //OLDCODE

    LIGHTING_INITIALIZE
	

    PRINT_GL_ERROR_IF_ANY("end initializeLightTables");
}


ttrenderstate renderstate()
{
	ppRenderFuncs p = (ppRenderFuncs)gglobal()->RenderFuncs.prv;
	return &p->renderstate;
}


//true statics:
GLint viewport[4] = {-1,-1,2,2};  //pseudo-viewport - doesn't change, used in glu unprojects
/* These two points (r2,r1) define a ray in pick-veiwport window coordinates 
	r2=viewpoint 
	r1=ray from center of pick-viewport in viewport coordinates
	- in setup_projection(pick=TRUE,,) the projMatrix is modified for the pick-ray-viewport
	- when unprojecting geometry-local xyz to bearing-local/pick-viewport-local, use pseudo-viewport defined above
*/
struct point_XYZ r1 = {0,0,-1}, r2 = {0,0,0}, r3 = {0,1,0}; //r3 y direction in case needed for testing


struct X3D_Anchor *AnchorsAnchor()
{
	ppRenderFuncs p = (ppRenderFuncs)gglobal()->RenderFuncs.prv;
	return p->AnchorsAnchor;
}
void setAnchorsAnchor(struct X3D_Anchor* anchor)
{
	ppRenderFuncs p = (ppRenderFuncs)gglobal()->RenderFuncs.prv;
	p->AnchorsAnchor = anchor;
}


//static struct currayhit rayph;
//struct currayhit rayHit,rayHitHyper;
/* used to test new hits */



//struct X3D_Group *_rootNode=NULL;	/* scene graph root node */
struct X3D_Node *rootNode()
{
	// ConsoleMessage ("rootNode called");
	ppRenderFuncs p = (ppRenderFuncs)gglobal()->RenderFuncs.prv;	
	if (p==NULL) {
		ConsoleMessage ("rootNode, p null");
		return NULL;
	}
	return p->rootNode;
}
void setRootNode(struct X3D_Node *rn)
{
	ppRenderFuncs p = (ppRenderFuncs)gglobal()->RenderFuncs.prv;
	p->rootNode = rn;
}
//struct Vector *libraries(){
//	ppRenderFuncs p = (ppRenderFuncs)gglobal()->RenderFuncs.prv;
//	if(!p->libraries) p->libraries = newVector(void3 *,1)	;
//	return p->libraries;
//}
//void setLibraries(struct Vector *libvector){
//	//might use this in KILL_oldWorld to NULL the library vector?
//	ppRenderFuncs p = (ppRenderFuncs)gglobal()->RenderFuncs.prv;	
//	p->libraries = libvector;
//}
void addLibrary(char *url, struct X3D_Proto *library, void *res){
	void3 *ul = MALLOC(void3 *,sizeof(void3));
	ppRenderFuncs p = (ppRenderFuncs)gglobal()->RenderFuncs.prv;	
	ul->one = (void *)STRDUP(url);
	ul->two = (void *)library;
	ul->three = res;
	vector_pushBack(void3 *,p->libraries,ul);
}
void3 *librarySearch(char *absoluteUniUrlNoPound){
	ppRenderFuncs p = (ppRenderFuncs)gglobal()->RenderFuncs.prv;	
	void3 *ul;
	struct Vector* libs;
	int n, i;
	libs = p->libraries;
	n = vectorSize(libs);
	for(i=0;i<n;i++){
		ul = vector_get(void3 *,libs,i);
		if(ul)
		if(!strcmp(absoluteUniUrlNoPound,ul->one)){
			return ul; //return res
		}
	}
	return NULL;
}

//void *empty_group=0;

/*******************************************************************************/

/* rayhit
	For PointingDeviceSensor component work, called from virt->rendray_<Shape> on VF_Sensitive pass
	- tests if this ray-geometry intersection is closer to the viewpoint than the closest one so far
	- if not it means it is occluded, do nothing
	- if so
	-- updates the closest distance to intersection of pick-ray/bearing with scene geometry so far
	How:
	- the calling rendray_<geometry> function already has the pickray/bearing in its geometry-local
		coordinates, and computes the distance from A as rat and passes it in here.
	- this makes sure its on the B side of A (otherwise its behind the pickray/viewpoint)
	- if its the closest intersection of pickray with scene geometry so far:
		1.records the point, in bearing-local coordinates
		a) for non-sensitive geometry: the point is used to occlude picksensors by being closer to the viewpoint/bearing A
		b) for <Drag>Sensor and TouchSensor, if the point succeeds as the closest point 
			at end of VF_Sensitive pass, the point will be transformed from bearing-local to sensor-local
			to generate eventOuts in do_<>Sensor in sensor-local coordinates
		2.snapshots the sensor's modelview matrix for later use

 */
void rayhit(float rat, float cx,float cy,float cz, float nx,float ny,float nz,
	    float tx,float ty, char *descr)  {
	GLDOUBLE modelMatrix[16];
	GLDOUBLE projMatrix[16];
	ppRenderFuncs p;
	ttglobal tg = gglobal();
	p = (ppRenderFuncs)tg->RenderFuncs.prv;

	/* Real rat-testing */
#ifdef RENDERVERBOSE
	//printf("RAY HIT %s! %f (%f %f %f) (%f %f %f)\n\tR: (%f %f %f) (%f %f %f)\n",
	//       descr, rat,cx,cy,cz,nx,ny,nz,
	//       t_r1.x, t_r1.y, t_r1.z,
	//       t_r2.x, t_r2.y, t_r2.z
	//	);
#endif

	if(rat<0 || (rat>tg->RenderFuncs.hitPointDist && tg->RenderFuncs.hitPointDist >= 0)) {
		return;
	}
	FW_GL_GETDOUBLEV(GL_MODELVIEW_MATRIX, modelMatrix); //snapshot of geometry's modelview matrix
	if(!tg->RenderFuncs.usingAffinePickmatrix){
		FW_GL_GETDOUBLEV(GL_PROJECTION_MATRIX, projMatrix);
		FW_GLU_PROJECT(cx,cy,cz, modelMatrix, projMatrix, viewport, &tg->RenderFuncs.hp.x, &tg->RenderFuncs.hp.y, &tg->RenderFuncs.hp.z);
	}
	if(tg->RenderFuncs.usingAffinePickmatrix){
		GLDOUBLE pmi[16];
		GLDOUBLE *pickMatrix = getPickrayMatrix(0);
		GLDOUBLE *pickMatrixi = getPickrayMatrix(1);
		struct point_XYZ tp; //note viewpoint/avatar Z=1 behind the viewer, to match the glu_unproject method WinZ = -1
		tp.x = cx; tp.y = cy; tp.z = cz;
		transform(&tp, &tp, modelMatrix);
		if(1){
			//pickMatrix is inverted in setup_projection
			transform(&tp,&tp,pickMatrixi);
		}else{
			//pickMatrix is not inverted in setup_projection
			matinverseAFFINE(pmi,pickMatrix);
			transform(&tp,&tp,pmi);
		}
		tg->RenderFuncs.hp = tp; //struct value copy
	}
	tg->RenderFuncs.hitPointDist = rat;
	p->rayHit=p->rayph;
	p->rayHitHyper=p->rayph;
#ifdef RENDERVERBOSE 
//	printf ("Rayhit, hp.x y z: - %f %f %f rat %f hitPointDist %f\n",hp.x,hp.y,hp.z, rat, tg->RenderFuncs.hitPointDist);
#endif
}


/* Call this when modelview and projection modified
	keeps bearing/pick-ray transformed into current geometry-local
	for use in virt->rendray_<geometry> calculations, on VF_Sensitive pass
	bearing-local == pick-viewport-local
*/
void upd_ray0(struct point_XYZ *t_r1, struct point_XYZ *t_r2, struct point_XYZ *t_r3) {
	//struct point_XYZ t_r1,t_r2,t_r3;
	GLDOUBLE modelMatrix[16];
	ttglobal tg = gglobal();
	FW_GL_GETDOUBLEV(GL_MODELVIEW_MATRIX, modelMatrix);
/*

{int i; printf ("\n"); 
printf ("upd_ray, pm %p\n",projMatrix);
for (i=0; i<16; i++) printf ("%4.3lf ",modelMatrix[i]); printf ("\n"); 
for (i=0; i<16; i++) printf ("%4.3lf ",projMatrix[i]); printf ("\n"); 
} 
*/

	if(!tg->RenderFuncs.usingAffinePickmatrix){
		GLDOUBLE projMatrix[16];
		FW_GL_GETDOUBLEV(GL_PROJECTION_MATRIX, projMatrix);
		// the projMatrix used here contains the GLU_PICK_MATRIX translation and scale
		//transform pick-ray (0,0,-1) B from pick-viewport-local to geometry-local
		//FLOPS 588 double: 3x glu_unproject 196
		//r1 = {0,0,-1} means WinZ = -1 in glu_unproject. It's expecting coords in 0-1 range. So -1 would be behind viewer? But x,y still foreward?
		FW_GLU_UNPROJECT(r1.x, r1.y, r1.z, modelMatrix, projMatrix, viewport,
				 &t_r1->x,&t_r1->y,&t_r1->z);
		//transform viewpoint A (0,0,0) in pick-ray-viewport-local to geometry-local
		FW_GLU_UNPROJECT(r2.x, r2.y, r2.z, modelMatrix, projMatrix, viewport,
				 &t_r2->x,&t_r2->y,&t_r2->z);
		//in case we need a viewpoint-y-up vector transform viewpoint y to geometry-local 
		FW_GLU_UNPROJECT(r3.x,r3.y,r3.z,modelMatrix,projMatrix,viewport,
				 &t_r3->x,&t_r3->y,&t_r3->z);
		if(0){
			//r2 is A, r1 is B relative to A in pickray [A,B)
			//we prove it here by moving B along the ray, to distance 1.0 from A, and no change to picking
			vecdiff(t_r1,t_r1,t_r2);
			vecnormal(t_r1,t_r1);
			vecadd(t_r1,t_r1,t_r2);
		}
		//printf("Upd_ray old: (%f %f %f) (%f %f %f) \n",	t_r1.x,t_r1.y,t_r1.z,t_r2.x,t_r2.y,t_r2.z);
	}
	if(tg->RenderFuncs.usingAffinePickmatrix){
		//feature-AFFINE_GLU_UNPROJECT
		//FLOPs	112 double:	matmultiplyAFFINE 36, matinverseAFFINE 49, 3x transform (affine) 9 =27
		GLDOUBLE mvp[16], mvpi[16];
		GLDOUBLE *pickMatrix = getPickrayMatrix(0);
		GLDOUBLE *pickMatrixi = getPickrayMatrix(1);
		struct point_XYZ r11 = {0.0,0.0,1.0}; //note viewpoint/avatar Z=1 behind the viewer, to match the glu_unproject method WinZ = -1

		if(0){
			//pickMatrix is inverted in setup_projection
			matmultiplyAFFINE(mvp,modelMatrix,pickMatrixi);
			matinverseAFFINE(mvpi,mvp);
		}else{
			//pickMatrix is not inverted in setup_projection
			double mvi[16];
			matinverseAFFINE(mvi,modelMatrix);
			matmultiplyAFFINE(mvpi,pickMatrix,mvi);
		}
		transform(t_r1,&r11,mvpi);
		transform(t_r2,&r2,mvpi);
		transform(t_r3,&r3,mvpi);
		//r2 is A, r1 is B relative to A in pickray [A,B)
		//we prove it here by moving B along the ray, to distance 1.0 from A, and no change to picking
		if(0){
			vecdiff(t_r1,t_r1,t_r2);
			vecnormal(t_r1,t_r1);
			vecadd(t_r1,t_r1,t_r2);
		}
		//printf("Upd_ray new: (%f %f %f) (%f %f %f) \n",	t_r1.x,t_r1.y,t_r1.z,t_r2.x,t_r2.y,t_r2.z);
	}
}
void upd_ray() {
	struct point_XYZ t_r1,t_r2,t_r3;
	ttglobal tg = gglobal();

	upd_ray0(&t_r1,&t_r2,&t_r3);
	VECCOPY(tg->RenderFuncs.t_r1,t_r1);
	VECCOPY(tg->RenderFuncs.t_r2,t_r2);
	VECCOPY(tg->RenderFuncs.t_r3,t_r3);

	/*
	printf("Upd_ray: (%f %f %f)->(%f %f %f) == (%f %f %f)->(%f %f %f)\n",
	r1.x,r1.y,r1.z,r2.x,r2.y,r2.z,
	t_r1.x,t_r1.y,t_r1.z,t_r2.x,t_r2.y,t_r2.z);
	*/

}
void transformMBB(GLDOUBLE *rMBBmin, GLDOUBLE *rMBBmax, GLDOUBLE *matTransform, GLDOUBLE* inMBBmin, GLDOUBLE* inMBBmax);
int pickrayHitsMBB(struct X3D_Node *node){
	//GOAL: on a sensitive (touch sensor) pass, before checking the ray against geometry, check first if the
	//ray goes through the extent / minimum-bounding-box (MBB) of the shape. If not, no need to check the ray 
	//against all the shape's triangles, speeding up the VF_Sensitive pass.
	//FLOPs 156 double: matmultiplyAffine 36, matInversAffine 48,  transformAffine 8 pts x 12= 72
	int retval;
	ttglobal tg = gglobal();
	retval = TRUE;
	if(tg->RenderFuncs.usingAffinePickmatrix){
		GLDOUBLE modelMatrix[16];
		int i, isIn;
		//if using new Sept 2014 pickmatrix, we can test the pickray against the shape node's bounding box
		//and if no hit, then no need to run through rendray testing all triangles
		//feature-AFFINE_GLU_UNPROJECT
		//FLOPs	112 double:	matmultiplyAFFINE 36, matinverseAFFINE 49, 3x transform (affine) 9 =27
		GLDOUBLE mvp[16]; //, mvpi[16];
		GLDOUBLE smin[3], smax[3], shapeMBBmin[3], shapeMBBmax[3];
		GLDOUBLE *pickMatrix = getPickrayMatrix(0);
		GLDOUBLE *pickMatrixi = getPickrayMatrix(1);
		//struct point_XYZ r11 = {0.0,0.0,-1.0}; //note viewpoint/avatar Z=1 behind the viewer, to match the glu_unproject method WinZ = -1
		FW_GL_GETDOUBLEV(GL_MODELVIEW_MATRIX, modelMatrix);

		if(1){
			//pickMatrix is inverted in setup_projection
			matmultiplyAFFINE(mvp,modelMatrix,pickMatrixi);
		}else{
			//pickMatrix is not inverted in setup_projection
			double pi[16];
			matinverseAFFINE(pi,pickMatrix);
			matmultiplyAFFINE(mvp,modelMatrix,pi);
		}

		/* generate mins and maxes for avatar cylinder in avatar space to represent the avatar collision volume */
		for(i=0;i<3;i++)
		{
			shapeMBBmin[i] = node->_extent[i*2 + 1];
			shapeMBBmax[i] = node->_extent[i*2];
		}
		transformMBB(smin,smax,mvp,shapeMBBmin,shapeMBBmax); //transform shape's MBB into pickray space
		// the pickray is now at 0,0,x
		isIn = TRUE;
		for(i=0;i<2;i++)
			isIn = isIn && (smin[i] <= 0.0 && smax[i] >= 0.0);
		retval = isIn;
		//printf("%d x %f %f y %f %f\n",isIn,smin[0],smax[0],smin[1],smax[1]);
		//retval = 1;
	}
	return retval;
}

/* if a node changes, void the display lists */
/* Courtesy of Jochen Hoenicke */

void update_node(struct X3D_Node *node) {
	int i;
	
#ifdef VERBOSE
	printf ("update_node for %d %s nparents %d renderflags %x\n",node, stringNodeType(node->_nodeType),node->_nparents, node->_renderFlags); 
	if (node->_nparents == 0) {
		if (node == rootNode) printf ("...no parents, this IS the rootNode\n"); 
		else printf ("...no parents, this IS NOT the rootNode\n");
	}



	for (i = 0; i < node->_nparents; i++) {
		struct X3D_Node *n = X3D_NODE(node->_parents[i]);
		if( n != 0 ) {
			printf ("	parent %u is %s\n",n,stringNodeType(n->_nodeType));
		} else {
			printf ("	parent %d is NULL\n",i);
		}
	}
#endif

	node->_change ++;

	/* parentVector here yet?? */
	if (node->_parentVector == NULL) {
		return;
	}

	for (i = 0; i < vectorSize(node->_parentVector); i++) {
		struct X3D_Node *n = vector_get(struct X3D_Node *, node->_parentVector,i);
		if(n == node) {
			fprintf(stderr, "Error: self-referential node structure! (node:'%s')\n", stringNodeType(node->_nodeType));
			vector_set(struct X3D_Node*, node->_parentVector, i,NULL);
		} else if( n != 0 ) {
			update_node(n);
		}
	}
}

/*********************************************************************
 *********************************************************************
 *
 * render_node : call the correct virtual functions to render the node
 * depending on what we are doing right now.
 */

//#ifdef RENDERVERBOSE
//static int renderLevel = 0;
//#endif

#define PRINT_NODE(_node, _v)  do {					\
		if (gglobal()->internalc.global_print_opengl_errors && (gglobal()->display._global_gl_err != GL_NO_ERROR)) { \
			printf("Render_node_v %p (%s) PREP: %p REND: %p CH: %p FIN: %p RAY: %p HYP: %p\n",_v, \
			       stringNodeType(_node->_nodeType),	\
			       _v->prep,				\
			       _v->rend,				\
			       _v->children,			\
			       _v->fin,				\
			       _v->rendray,			\
			       gglobal()->RenderFuncs.hypersensitive);			\
			printf("Render_state geom %d light %d sens %d\n", \
			       renderstate()->render_geom,				\
			       renderstate()->render_light,				\
			       renderstate()->render_sensitive);			\
			printf("pchange %d pichange %d \n", _node->_change, _node->_ichange); \
		}							\
	} while (0)

//static int renderLevel = 0;
//#define RENDERVERBOSE

/*poor man's memory profiler */
static int malloc_n_uses = 0;
static char *malloc_uses[100];
static int malloc_bytes[100];
void malloc_profile_add(char *use, int bytes){
	int i, ifound;
	ifound = -1;
	for(i=0;i<malloc_n_uses;i++){
		if(!strcmp(use,malloc_uses[i])){
			ifound = i;
			break;
		}
	}
	if(ifound == -1 && malloc_n_uses < 100){
		malloc_uses[malloc_n_uses] = strdup(use);
		malloc_bytes[malloc_n_uses] = 0;
		ifound = malloc_n_uses;
		malloc_n_uses++;
	}
	if(ifound > -1){
		malloc_bytes[ifound] += bytes;
	}
}
void malloc_profile_print(){
	if(malloc_n_uses){
		int i;
		printf("%15s %12s\n","mem use","bytes");
		for(i=0;i<malloc_n_uses;i++){
			printf("%15s %12d\n",malloc_uses[i],malloc_bytes[i]);
		}
	}
}

/* poor-man's performance profiler:
   wrap a section of code like this
     profile_start("section1");
	 ...code...
	 profile_end("section1");
	then let the browser loop for 10 seconds 
	and hit period '.' on the keyboard to get a printout
*/

void profile_start(char *name){
	ppRenderFuncs p;
	struct profile_entry *pe;
	int i, ifound = -1;
	ttglobal tg = gglobal();
	p = (ppRenderFuncs)tg->RenderFuncs.prv;

	if (!p->profiling_on) return;
	pe = p->profile_entries;

	for(i=0;i<p->profile_entry_count;i++){
		if(!strcmp(name,pe[i].name)){
			ifound = i;
			break;
		}
	}
	if(ifound == -1){
		pe[p->profile_entry_count].name = name;
		pe[p->profile_entry_count].hits = 0;
		ifound = p->profile_entry_count;
		p->profile_entry_count++;
	}
	pe[ifound].start = Time1970sec();
}
void profile_end(char *name){
	ppRenderFuncs p;
	struct profile_entry *pe;
	int i, ifound = -1;
	ttglobal tg = gglobal();
	p = (ppRenderFuncs)tg->RenderFuncs.prv;

	if (!p->profiling_on) return;
	pe = p->profile_entries;
	for(i=0;i<p->profile_entry_count;i++){
		if(!strcmp(name,pe[i].name)){
			ifound = i;
			break;
		}
	}
	if(ifound > -1){
		pe[ifound].accum += Time1970sec() - pe[ifound].start;
		pe[ifound].hits++;
	}
}
void profile_print_all(){
	//hit '.' in the graphics window to get here
	ppRenderFuncs p;
	struct profile_entry *pe;
	ttglobal tg = gglobal();
	p = (ppRenderFuncs)tg->RenderFuncs.prv;
	if (!p->profiling_on){
		p->profiling_on = 1;
		ConsoleMessage("turning profiling on\n");
	}else{
		int i;
		pe = p->profile_entries;
		ConsoleMessage("frame rate: %9.3f  number of items tracked: %d\n", gglobal()->Mainloop.BrowserFPS,p->profile_entry_count);
		ConsoleMessage("%15s %10s %15s %10s\n", "profile name", "hits", "time(sec)", "% of 1st");
		for (i = 0; i < p->profile_entry_count; i++){
			ConsoleMessage("%15s %10d %15.3f %10.2f\n", pe[i].name, pe[i].hits, pe[i].accum, pe[i].accum / pe[0].accum*100.0);
		}
	}
	malloc_profile_print();
}
//struct point_XYZ3 {
//	struct point_XYZ p1;
//	struct point_XYZ p2;
//	struct point_XYZ p3;
//};
void push_ray(){
	//upd_ray();
	struct point_XYZ t_r1,t_r2,t_r3;
	struct point_XYZ3 r123;
	ttglobal tg = gglobal();
	ppRenderFuncs p = (ppRenderFuncs)tg->RenderFuncs.prv;
	r123.p1 = tg->RenderFuncs.t_r1;
	r123.p2 = tg->RenderFuncs.t_r2;
	r123.p3 = tg->RenderFuncs.t_r3;

	stack_push(struct point_XYZ3,p->ray_stack,r123);

	upd_ray0(&t_r1,&t_r2,&t_r3);
	VECCOPY(tg->RenderFuncs.t_r1,t_r1);
	VECCOPY(tg->RenderFuncs.t_r2,t_r2);
	VECCOPY(tg->RenderFuncs.t_r3,t_r3);

}
void pop_ray(){
	struct point_XYZ t_r1,t_r2,t_r3;
	struct point_XYZ3 r123;
	ttglobal tg = gglobal();
	ppRenderFuncs p = (ppRenderFuncs)tg->RenderFuncs.prv;
	//upd_ray();
	r123 = stack_top(struct point_XYZ3,p->ray_stack);
	stack_pop(struct point_XYZ3,p->ray_stack);
	tg->RenderFuncs.t_r1 = r123.p1;
	tg->RenderFuncs.t_r2 = r123.p2;
	tg->RenderFuncs.t_r3 = r123.p3;

}
void push_render_geom(int igeom){
	ttglobal tg = gglobal();
	ppRenderFuncs p = (ppRenderFuncs)tg->RenderFuncs.prv;
	stack_push(int,p->render_geom_stack,p->renderstate.render_geom);
	p->renderstate.render_geom = igeom;
}
void pop_render_geom(){
	int igeom;
	ttglobal tg = gglobal();
	ppRenderFuncs p = (ppRenderFuncs)tg->RenderFuncs.prv;
	igeom = stack_top(int,p->render_geom_stack);
	stack_pop(int,p->render_geom_stack);
	p->renderstate.render_geom = igeom;
}
void push_sensor(struct X3D_Node *node){
	ttglobal tg = gglobal();
	ppRenderFuncs p = (ppRenderFuncs)tg->RenderFuncs.prv;

	push_render_geom(1);
	stack_push(struct currayhit,p->sensor_stack,p->rayph);
	//srh = MALLOC(struct currayhit *,sizeof(struct currayhit));
	////srh = p->rayph;
	//memcpy(srh,&p->rayph,sizeof(struct currayhit));
	p->rayph.hitNode = node; //will be the parent Transform or Group to a PointingDevice (Touch,Drag) Sensor node
	FW_GL_GETDOUBLEV(GL_MODELVIEW_MATRIX, p->rayph.modelMatrix); //snapshot of sensor's modelview matrix
	FW_GL_GETDOUBLEV(GL_PROJECTION_MATRIX, p->rayph.projMatrix);
	//PRINT_GL_ERROR_IF_ANY("render_sensitive"); PRINT_NODE(node,virt);
}
void pop_sensor(){
	ttglobal tg = gglobal();
	ppRenderFuncs p = (ppRenderFuncs)tg->RenderFuncs.prv;

	//memcpy(&p->rayph,srh,sizeof(struct currayhit));
	//FREE_IF_NZ(srh);
	p->rayph = stack_top(struct currayhit,p->sensor_stack);
	stack_pop(struct currayhit,p->sensor_stack);
	pop_render_geom();

}
void render_node(struct X3D_Node *node) {
	struct X3D_Virt *virt;

	//int srg = 0;
	//int sch = 0;
	int justGeom = 0;
	int pushed_ray;
	int pushed_render_geom;
	int pushed_sensor;
	//struct currayhit *srh = NULL;
	ppRenderFuncs p;
	ttglobal tg = gglobal();
	p = (ppRenderFuncs)tg->RenderFuncs.prv;

	X3D_NODE_CHECK(node);
//#define RENDERVERBOSE 1
#ifdef RENDERVERBOSE
	p->renderLevel ++;
#endif

	if(!node) {
#ifdef RENDERVERBOSE
		DEBUG_RENDER("%d no node, quick return\n", renderLevel);
		p->renderLevel--;
#endif
		return;
	}

	virt = virtTable[node->_nodeType];

#ifdef RENDERVERBOSE 
	//printf("%d =========================================NODE RENDERED===================================================\n",renderLevel);
	{
		int i;
		for(i=0;i<p->renderLevel;i++) printf(" ");
	}
	printf("%d node %u (%s) , v %u renderFlags %x ",p->renderLevel, node,stringNodeType(node->_nodeType),virt,node->_renderFlags);

	if ((node->_renderFlags & VF_Viewpoint) == VF_Viewpoint) printf (" VF_Viewpoint");
	if ((node->_renderFlags & VF_Geom )== VF_Geom) printf (" VF_Geom");
	if ((node->_renderFlags & VF_localLight )== VF_localLight) printf (" VF_localLight");
	if ((node->_renderFlags & VF_Sensitive) == VF_Sensitive) printf (" VF_Sensitive");
	if ((node->_renderFlags & VF_Blend) == VF_Blend) printf (" VF_Blend");
	if ((node->_renderFlags & VF_Proximity) == VF_Proximity) printf (" VF_Proximity");
	if ((node->_renderFlags & VF_Collision) == VF_Collision) printf (" VF_Collision");
	if ((node->_renderFlags & VF_globalLight) == VF_globalLight) printf (" VF_globalLight");
	if ((node->_renderFlags & VF_hasVisibleChildren) == VF_hasVisibleChildren) printf (" VF_hasVisibleChildren");
	if ((node->_renderFlags & VF_shouldSortChildren) == VF_shouldSortChildren) printf (" VF_shouldSortChildren");
	if ((node->_renderFlags & VF_Other) == VF_Other) printf (" VF_Other");
	/*
	if ((node->_renderFlags & VF_inPickableGroup == VF_inPickableGroup) printf (" VF_inPickableGroup");
	if ((node->_renderFlags & VF_PickingSensor == VF_PickingSensor) printf (" VF_PickingSensor");
	*/
	printf ("\n");

	//printf("PREP: %d REND: %d CH: %d FIN: %d RAY: %d HYP: %d\n",virt->prep, virt->rend, virt->children, virt->fin,
	//       virt->rendray, hypersensitive);
	//printf("%d state: vp %d geom %d light %d sens %d blend %d prox %d col %d ", renderLevel, 
 //        	render_vp,render_geom,render_light,render_sensitive,render_blend,render_proximity,render_collision); 
	//printf("change %d ichange %d \n",node->_change, node->_ichange);
#endif

        /* if we are doing Viewpoints, and we don't have a Viewpoint, don't bother doing anything here */ 
        //if (renderstate()->render_vp == VF_Viewpoint) { 
        if (p->renderstate.render_vp == VF_Viewpoint) { 
                if ((node->_renderFlags & VF_Viewpoint) != VF_Viewpoint) { 
#ifdef RENDERVERBOSE
                        printf ("doing Viewpoint, but this  node is not for us - just returning\n"); 
			p->renderLevel--;
#endif
                        return; 
                } 
        }

	/* are we working through global PointLights, DirectionalLights or SpotLights, but none exist from here on down? */
        if (p->renderstate.render_light == VF_globalLight) { 
                if ((node->_renderFlags & VF_globalLight) != VF_globalLight) { 
#ifdef RENDERVERBOSE
                        printf ("doing globalLight, but this  node is not for us - just returning\n"); 
			p->renderLevel--;
#endif
                        return; 
                }
        }
	justGeom = p->renderstate.render_geom && !p->renderstate.render_sensitive && !p->renderstate.render_blend;
	pushed_ray = FALSE;
	pushed_render_geom = FALSE;
	pushed_sensor = FALSE;
	if(virt->prep) {
		//transform types will pushmatrix and multiply in their translation.rotation,scale here (and popmatrix in virt->fin)
		DEBUG_RENDER("rs 2\n");
		profile_start("prep");
		if(justGeom)
			profile_start("prepgeom");
		virt->prep(node);  
		profile_end("prep");
		if(justGeom)
			profile_end("prepgeom");
		if(p->renderstate.render_sensitive && !tg->RenderFuncs.hypersensitive) {
			push_ray(); //upd_ray(); 
			pushed_ray = TRUE;
		}
		PRINT_GL_ERROR_IF_ANY("prep"); PRINT_NODE(node,virt);
	}
	if(p->renderstate.render_proximity && virt->proximity) {
		DEBUG_RENDER("rs 2a\n");
		profile_start("proximity");
		virt->proximity(node);
		profile_end("proximity");
		PRINT_GL_ERROR_IF_ANY("render_proximity"); PRINT_NODE(node,virt);
	}
	
	if(p->renderstate.render_collision && virt->collision) {
		DEBUG_RENDER("rs 2b\n");
		profile_start("collision");
		virt->collision(node);
		profile_end("collision");
		PRINT_GL_ERROR_IF_ANY("render_collision"); PRINT_NODE(node,virt);
	}

	if(p->renderstate.render_geom && !p->renderstate.render_sensitive && virt->rend) {
		DEBUG_RENDER("rs 3\n");
		PRINT_GL_ERROR_IF_ANY("BEFORE render_geom"); PRINT_NODE(node,virt);
		profile_start("rend");
		virt->rend(node);
		profile_end("rend");
		PRINT_GL_ERROR_IF_ANY("render_geom"); PRINT_NODE(node,virt);
	}

	if(p->renderstate.render_other && virt->other )
	{
#ifdef DJTRACK_PICKSENSORS
		DEBUG_RENDER("rs 4a\n");
		virt->other(node); //other() is responsible for push_renderingState(VF_inPickableGroup);
		PRINT_GL_ERROR_IF_ANY("render_other"); PRINT_NODE(node,virt);
#endif
	} //other

	if(p->renderstate.render_sensitive && ((node->_renderFlags & VF_Sensitive)|| Viewer()->LookatMode ==2)) {
		DEBUG_RENDER("rs 5\n");
		profile_start("sensitive");
		push_sensor(node);
		pushed_sensor = TRUE;
		profile_end("sensitive");
	}

	if(p->renderstate.render_geom && p->renderstate.render_sensitive && !tg->RenderFuncs.hypersensitive && virt->rendray) {
		DEBUG_RENDER("rs 6\n");
		profile_start("rendray");
		if(pickrayHitsMBB(node))
			virt->rendray(node);
		profile_end("rendray");
		PRINT_GL_ERROR_IF_ANY("rs 6"); PRINT_NODE(node,virt);
	}

    if((p->renderstate.render_sensitive) && (tg->RenderFuncs.hypersensitive == node)) {
		DEBUG_RENDER("rs 7\n");
		p->hyper_r1 = tg->RenderFuncs.t_r1;
		p->hyper_r2 = tg->RenderFuncs.t_r2;
		tg->RenderFuncs.hyperhit = 1;
    }

	/* start recursive section */
    if(virt->children) { 
		DEBUG_RENDER("rs 8 - has valid child node pointer\n");
		virt->children(node);
		PRINT_GL_ERROR_IF_ANY("children"); PRINT_NODE(node,virt);
    }
	/* end recursive section */

	if(p->renderstate.render_other && virt->other)
	{
#ifdef DJTRACK_PICKSENSORS
		//pop_renderingState(VF_inPickableGroup);
#endif
	}

	if(pushed_sensor)
		pop_sensor();

	if(virt->fin) {
		DEBUG_RENDER("rs A\n");
		profile_start("fin");
		if(justGeom)
			profile_start("fingeom");

		virt->fin(node);
		profile_end("fin");
		if(justGeom)
			profile_end("fingeom");
		//if(p->renderstate.render_sensitive && virt == &virt_Transform) {
		//	upd_ray();
		//}
		PRINT_GL_ERROR_IF_ANY("fin"); PRINT_NODE(node,virt);
	}
	if(pushed_ray)
		pop_ray();

#ifdef RENDERVERBOSE 
	{
		int i;
		for(i=0;i<p->renderLevel;i++)printf(" ");
	}
	printf("%d (end render_node)\n",p->renderLevel);
	p->renderLevel--;
#endif
}
//#undef RENDERVERBOSE

/*
 * The following code handles keeping track of the parents of a given
 * node. This enables us to traverse the scene on C level for optimizations.
 *
 * We use this array code because VRML nodes usually don't have
 * hundreds of children and don't usually shuffle them too much.
 */
//dug9 dec 13 >>
struct X3D_Node* getTypeNode(struct X3D_Node *node);

void add_parent(struct X3D_Node *node, struct X3D_Node *parent, char *file, int line) {
	struct X3D_Node* itype;
	if(!node) return;
	//if(node->_nodeType == NODE_PlaneSensor)
	//	printf("hi from add_parent, have a Planesensor");
#ifdef CHILDVERBOSE
	printf ("add_parent; adding node %u ,to parent %u at %s:%d\n",node,  parent,file,line);
	printf ("add_parent; adding node %x ,to parent %x (hex) at %s:%d\n",node,  parent,file,line);
	printf ("add_parent; adding node %p ,to parent %p (ptr) at %s:%d\n",node,  parent,file,line);


	printf ("add_parent; adding node %u (%s) to parent %u (%s) at %s:%d\n",node, stringNodeType(node->_nodeType), 
		parent, stringNodeType(parent->_nodeType),file,line);
#endif

	parent->_renderFlags = parent->_renderFlags | node->_renderFlags;

	/* add it to the parents list */
	vector_pushBack (struct X3D_Node*,node->_parentVector, parent);
	/* tie in sensitive nodes */
	itype = getTypeNode(node);
	if(itype)
		setSensitive (parent, itype );
}

void remove_parent(struct X3D_Node *child, struct X3D_Node *parent) {
	int i;
	int pi;

	if(!child) return;
	if(!parent) return;
	
#ifdef CHILDVERBOSE
	printf ("remove_parent, parent %u (%s) , child %u (%s)\n",parent, stringNodeType(parent->_nodeType),
		child, stringNodeType(child->_nodeType));
#endif

		pi = -1;
		for (i=0; i<vectorSize(child->_parentVector); i++) {
			struct X3D_Node *n = vector_get(struct X3D_Node *, child->_parentVector,i);
			if (n==parent) pi = i;
		}

		if (pi >=0) {
			struct X3D_Node *n = vector_get(struct X3D_Node *, child->_parentVector,vectorSize(child->_parentVector)-1);

			/* get the last entry, and overwrite the entry found */
			vector_set(struct X3D_Node*, child->_parentVector, pi,n);

			/* take that last entry off the vector */
			vector_popBack(struct X3D_Node*, child->_parentVector);
	}
}

void
render_hier(struct X3D_Node *g, int rwhat) {
	/// not needed now - see below struct point_XYZ upvec = {0,1,0};
	/// not needed now - see below GLDOUBLE modelMatrix[16];

	ppRenderFuncs p;
	ttglobal tg = gglobal();
	ttrenderstate rs;
	p = (ppRenderFuncs)tg->RenderFuncs.prv;
	rs = renderstate();

	rs->render_vp = rwhat & VF_Viewpoint;
	rs->render_geom =  rwhat & VF_Geom;
	rs->render_light = rwhat & VF_globalLight;
	rs->render_sensitive = rwhat & VF_Sensitive;
	rs->render_blend = rwhat & VF_Blend;
	rs->render_proximity = rwhat & VF_Proximity;
	rs->render_collision = rwhat & VF_Collision;
	rs->render_other = rwhat & VF_Other;
#ifdef DJTRACK_PICKSENSORS
	rs->render_picksensors = rwhat & VF_PickingSensor;
	rs->render_pickables = rwhat & VF_inPickableGroup;
#endif
	//p->nextFreeLight = 0;
	p->lastShader = -1; //in sendLights,and optimization
	tg->RenderFuncs.hitPointDist = -1;


#ifdef RENDERVERBOSE
	 printf ("render_hier vp %d geom %d light %d sens %d blend %d prox %d col %d\n",
	   rs->render_vp,rs->render_geom,rs->render_light,rs->render_sensitive,rs->render_blend,rs->render_proximity,rs->render_collision);  
#endif

	if (!g) {
		/* we have no geometry yet, sleep for a tiny bit */
		usleep(1000);
		return;
	}

#ifdef RENDERVERBOSE
	printf("Render_hier node=%d what=%d\n", g, rwhat);
#endif


	if (rs->render_sensitive) {
		upd_ray();
	}

	profile_start("render_hier");
	render_node(X3D_NODE(g));
	profile_end("render_hier");

}


/******************************************************************************
 *
 * shape compiler "thread"
 *
 ******************************************************************************/

void compileNode (void (*nodefn)(void *, void *, void *, void *, void *), void *node, void *Icoord, void *Icolor, void *Inormal, void *ItexCoord) {
	void *coord; void *color; void *normal; void *texCoord;

	/* are any of these SFNodes PROTOS? If so, get the underlying real node, as PROTOS are handled like Groups. */
	POSSIBLE_PROTO_EXPANSION(void *, Icoord,coord)
		POSSIBLE_PROTO_EXPANSION(void *, Icolor,color)
		POSSIBLE_PROTO_EXPANSION(void *, Inormal,normal)
		POSSIBLE_PROTO_EXPANSION(void *, ItexCoord,texCoord)

	nodefn(node, coord, color, normal, texCoord);
}

void do_NurbsPositionInterpolator (void *node);
void do_NurbsOrientationInterpolator (void *node);
void do_NurbsSurfaceInterpolator (void *node);
/* for CRoutes, we need to have a function pointer to an interpolator to run, if we
   route TO an interpolator */
void *returnInterpolatorPointer (const char *x) {
	if (strcmp("OrientationInterpolator",x)==0) { return (void *)do_Oint4;
	} else if (strcmp("CoordinateInterpolator2D",x)==0) { return (void *)do_OintCoord2D;
	} else if (strcmp("PositionInterpolator2D",x)==0) { return (void *)do_OintPos2D;
	} else if (strcmp("ScalarInterpolator",x)==0) { return (void *)do_OintScalar;
	} else if (strcmp("ColorInterpolator",x)==0) { return (void *)do_ColorInterpolator;
	} else if (strcmp("PositionInterpolator",x)==0) { return (void *)do_PositionInterpolator;
	} else if (strcmp("CoordinateInterpolator",x)==0) { return (void *)do_OintCoord;
	} else if (strcmp("NormalInterpolator",x)==0) { return (void *)do_OintNormal;
	} else if (strcmp("GeoPositionInterpolator",x)==0) { return (void *)do_GeoPositionInterpolator;
	} else if (strcmp("NurbsPositionInterpolator",x)==0) { return (void *)do_NurbsPositionInterpolator;
	} else if (strcmp("NurbsOrientationInterpolator",x)==0) { return (void *)do_NurbsPositionInterpolator;
	} else if (strcmp("NurbsSurfaceInterpolator",x)==0) { return (void *)do_NurbsSurfaceInterpolator;
	} else if (strcmp("BooleanFilter",x)==0) { return (void *)do_BooleanFilter;
	} else if (strcmp("BooleanSequencer",x)==0) { return (void *)do_BooleanSequencer;
	} else if (strcmp("BooleanToggle",x)==0) { return (void *)do_BooleanToggle;
	} else if (strcmp("BooleanTrigger",x)==0) { return (void *)do_BooleanTrigger;
	} else if (strcmp("IntegerTrigger",x)==0) { return (void *)do_IntegerTrigger;
	} else if (strcmp("IntegerSequencer",x)==0) { return (void *)do_IntegerSequencer;
	} else if (strcmp("TimeTrigger",x)==0) { return (void *)do_TimeTrigger;
	
	} else {
		return 0;
	}
}




void checkParentLink (struct X3D_Node *node,struct X3D_Node *parent) {
        int n;

	int *offsetptr;
	char *memptr;
	struct Multi_Node *mfn;
	uintptr_t *voidptr;

        if (node == NULL) return;

	/* printf ("checkParentLink for node %u parent %u type %s\n",node,parent,stringNodeType(node->_nodeType)); */
 
        if (parent != NULL) ADD_PARENT(node, parent);

	if ((node->_nodeType<0) || (node->_nodeType>NODES_COUNT)) {
		ConsoleMessage ("checkParentLink - %d not a valid nodeType",node->_nodeType);
		return;
	}

	/* find all the fields of this node */
	offsetptr = (int *) NODE_OFFSETS[node->_nodeType];

	/* FIELDNAMES_bboxCenter, offsetof (struct X3D_Group, bboxCenter),  FIELDTYPE_SFVec3f, KW_field, */
	while (*offsetptr >= 0) {

		/* 
		   printf ("	field %s",FIELDNAMES[offsetptr[0]]); 
		   printf ("	offset %d",offsetptr[1]);
		   printf ("	type %s",FIELDTYPES[offsetptr[2]]);
		   printf ("	kind %s\n",KEYWORDS[offsetptr[3]]);
		*/

		/* worry about SFNodes and MFNodes */
		if ((offsetptr[2] == FIELDTYPE_SFNode) || (offsetptr[2] == FIELDTYPE_MFNode)) {
			if ((offsetptr[3] == KW_initializeOnly) || (offsetptr[3] == KW_inputOutput)) {

				/* create a pointer to the actual field */
				memptr = (char *) node;
				memptr += offsetptr[1];

				if (offsetptr[2] == FIELDTYPE_SFNode) {
					/* get the field as a POINTER VALUE, not just a pointer... */
					voidptr = (intptr_t *) memptr;
					voidptr = (intptr_t *) *voidptr;

					/* is there a node here? */
					if (voidptr != NULL) {
						checkParentLink(X3D_NODE(voidptr),node);
					}
				} else {
					mfn = (struct Multi_Node*) memptr;
					/* printf ("MFNode has %d children\n",mfn->n); */
					for (n=0; n<mfn->n; n++) {
				                checkParentLink(mfn->p[n],node);
					}
				}
			}

		}
		offsetptr+=5;
	}
}

#define X3D_COORD(node) ((struct X3D_Coordinate*)node)
#define X3D_GEOCOORD(node) ((struct X3D_GeoCoordinate*)node)

/* get a coordinate array - (SFVec3f) from either a NODE_Coordinate or NODE_GeoCoordinate */
struct Multi_Vec3f *getCoordinate (struct X3D_Node *innode, char *str) {
	struct X3D_Coordinate * xc;
	struct X3D_GeoCoordinate *gxc;
	struct X3D_Node *node;

	POSSIBLE_PROTO_EXPANSION (struct X3D_Node *, innode,node)

		xc = X3D_COORD(node);
	/* printf ("getCoordinate, have a %s\n",stringNodeType(xc->_nodeType)); */

	if (xc->_nodeType == NODE_Coordinate) {
		return &(xc->point);
	} else if (xc->_nodeType == NODE_GeoCoordinate) {
		COMPILE_IF_REQUIRED_RETURN_NULL_ON_ERROR;
		gxc = X3D_GEOCOORD(node);
		return &(gxc->__movedCoords);
	} else {
		ConsoleMessage ("%s - coord expected but got %s\n", stringNodeType(xc->_nodeType));
	}
	return NULL;
}



/*
void printLTDebug(char * fn, int ln)
{
 ppRenderFuncs p = (ppRenderFuncs)gglobal()->RenderFuncs.prv;
ConsoleMessage ("headlight pos %f %f %f %f at %s:%d",
p->light_pos[HEADLIGHT_LIGHT][0],p->light_pos[HEADLIGHT_LIGHT][1],
                p->light_pos[HEADLIGHT_LIGHT][2],p->light_pos[HEADLIGHT_LIGHT][3],fn,ln);
}
*/

