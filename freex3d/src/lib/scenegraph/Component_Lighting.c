/*


X3D Lighting Component

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
#include "../opengl/OpenGL_Utils.h"
#include "RenderFuncs.h"
//#include "../opengl/OpenGL_Utils.h"
#include "LinearAlgebra.h"

#define RETURN_IF_LIGHT_STATE_NOT_US \
		if (renderstate()->render_light== VF_globalLight) { \
			if (!node->global) return;\
			/* printf ("and this is a global light\n"); */\
		} else if (node->global) return; \
		/* else printf ("and this is a local light\n"); */

#define CHBOUNDS(aaa) \
    if (aaa.c[0]>1.0) aaa.c[0] = 1.0; \
    if (aaa.c[0]<0.0) aaa.c[0] = 0.0; \
    if (aaa.c[1]>1.0) aaa.c[1] = 1.0; \
    if (aaa.c[1]<0.0) aaa.c[1] = 0.0; \
    if (aaa.c[2]>1.0) aaa.c[2] = 1.0; \
    if (aaa.c[2]<0.0) aaa.c[3] = 0.0; 


void compile_DirectionalLight (struct X3D_DirectionalLight *node) {
    struct point_XYZ vec;

    vec.x = (double) -((node->direction).c[0]);
    vec.y = (double) -((node->direction).c[1]);
    vec.z = (double) -((node->direction).c[2]);
    normalize_vector(&vec);
    node->_dir.c[0] = (float) vec.x;
    node->_dir.c[1] = (float) vec.y;
    node->_dir.c[2] = (float) vec.z;
    node->_dir.c[3] = 0.0f;/* 0.0 = this is a vector, not a position */

    node->_col.c[0] = ((node->color).c[0]) * (node->intensity);
    node->_col.c[1] = ((node->color).c[1]) * (node->intensity);
    node->_col.c[2] = ((node->color).c[2]) * (node->intensity);
    node->_col.c[3] = 1;
    CHBOUNDS(node->_col);
    
    
    node->_amb.c[0] = ((node->color).c[0]) * (node->ambientIntensity);
    node->_amb.c[1] = ((node->color).c[1]) * (node->ambientIntensity);
    node->_amb.c[2] = ((node->color).c[2]) * (node->ambientIntensity);
    node->_amb.c[3] = 1;
    CHBOUNDS(node->_amb);
    MARK_NODE_COMPILED;
}


void render_DirectionalLight (struct X3D_DirectionalLight *node) {
	/* if we are doing global lighting, is this one for us? */
	RETURN_IF_LIGHT_STATE_NOT_US
	/*
		if (renderstate()->render_light== VF_globalLight) { 
			if (!node->global){ 
				printf("x local dir,we want global %u\n",node);
				return;
			}
			 printf ("* global dir, we want global %u\n",node); 
		} else {
			if (node->global){
			  printf("x global dir, we want local %u\n",node);
			  return; 
			}
			else {
			   printf ("* local dir, we want local %u\n",node); 
			}
		}
	*/
    COMPILE_IF_REQUIRED;

	if(node->on) {
		int light = nextlight();
		if(light >= 0) {
			float pos[4] = {0.0f, 0.0f, 0.0f, 1.0f};
			setLightState(light,TRUE);
			setLightType(light,2);
			FW_GL_LIGHTFV(light, GL_SPOT_DIRECTION, (GLfloat* )node->_dir.c);
			FW_GL_LIGHTFV(light, GL_POSITION, (GLfloat* )pos);
			FW_GL_LIGHTFV(light, GL_DIFFUSE, node->_col.c);
			FW_GL_LIGHTFV(light, GL_SPECULAR, node->_col.c);
			FW_GL_LIGHTFV(light, GL_AMBIENT, node->_amb.c);
            /* used to test if a PointLight, SpotLight or DirectionalLight in shader  */
			// was used, using lightType now //FW_GL_LIGHTF(light, GL_SPOT_CUTOFF, 0);
            setLightChangedFlag(light);
            // not used in directionlight calc //FW_GL_LIGHTF(light,GL_LIGHT_RADIUS,100000.0); /* make it very large */
		}
	}
}

/* global lights  are done before the rendering of geometry */
void prep_DirectionalLight (struct X3D_DirectionalLight *node) {
	if (!renderstate()->render_light) return;
	render_DirectionalLight(node);
}

void compile_PointLight (struct X3D_PointLight *node) {
    int i;
    
    for (i=0; i<3; i++) node->_loc.c[i] = node->location.c[i];
    node->_loc.c[3] = 1.0f;/* 1 == this is a position, not a vector */
    
    //ConsoleMessage("compile_PointLight, loc %f %f %f %f",node->_loc.c[0],node->_loc.c[1],node->_loc.c[2],node->_loc.c[3]);

    node->_col.c[0] = ((node->color).c[0]) * (node->intensity);
    node->_col.c[1] = ((node->color).c[1]) * (node->intensity);
    node->_col.c[2] = ((node->color).c[2]) * (node->intensity);
    node->_col.c[3] = 1;
    CHBOUNDS(node->_col);
    
    
    node->_amb.c[0] = ((node->color).c[0]) * (node->ambientIntensity);
    node->_amb.c[1] = ((node->color).c[1]) * (node->ambientIntensity);
    node->_amb.c[2] = ((node->color).c[2]) * (node->ambientIntensity);
    node->_amb.c[3] = 1;
    CHBOUNDS(node->_amb);
    MARK_NODE_COMPILED;
    
    /* ConsoleMessage ("compile_PointLight, attenuation %f %f %f",
                 node->attenuation.c[0],
                     node->attenuation.c[1],
                    node->attenuation.c[2]);*/
    
 
    /* ConsoleMessage ("compile_PointLight, col %f %f %f %f amb %f %f %f %f",
                    node->_col.c[0],
                    node->_col.c[1],
                    node->_col.c[2],
                    node->_col.c[3],
                    node->_amb.c[0],
                    node->_amb.c[1],
                    node->_amb.c[2],
                    node->_amb.c[3]);
    */
}


void render_PointLight (struct X3D_PointLight *node) {

	/* if we are doing global lighting, is this one for us? */
	RETURN_IF_LIGHT_STATE_NOT_US
	/*
		if (renderstate()->render_light== VF_globalLight) { 
			if (!node->global) {
				printf("x local point, we want global %u\n", node);
				return;
			}
			 printf ("* global point,  we want global %u\n", node);
		} else {
			if (node->global){ 
				printf("x global point, we want local %u\n", node);
			  return; 
			}else {
			   printf ("* local point, we want local %u\n", node);
			}
		}
	*/
    COMPILE_IF_REQUIRED;

	if(node->on) {
		int light = nextlight();
		if(light >= 0) {
			float vec[4] = {0.0f, 0.0f, -1.0f, 1.0f};
            
			setLightState(light,TRUE);
			setLightType(light,0);
			FW_GL_LIGHTFV(light, GL_SPOT_DIRECTION, vec);
			FW_GL_LIGHTFV(light, GL_POSITION, node->_loc.c);

			FW_GL_LIGHTF(light, GL_CONSTANT_ATTENUATION,
				((node->attenuation).c[0]));
			FW_GL_LIGHTF(light, GL_LINEAR_ATTENUATION,
				((node->attenuation).c[1]));
			FW_GL_LIGHTF(light, GL_QUADRATIC_ATTENUATION,
				((node->attenuation).c[2]));


			FW_GL_LIGHTFV(light, GL_DIFFUSE, node->_col.c);
			FW_GL_LIGHTFV(light, GL_SPECULAR, node->_col.c);
			FW_GL_LIGHTFV(light, GL_AMBIENT, node->_amb.c);

			/* used to test if a PointLight, SpotLight or DirectionalLight in shader  */
			// was used, using lightType now //FW_GL_LIGHTF(light, GL_SPOT_CUTOFF, 0);
            
            FW_GL_LIGHTF(light,GL_LIGHT_RADIUS,node->radius);
            setLightChangedFlag(light);
		}
	}
}

/* pointLights are done before the rendering of geometry */
void prep_PointLight (struct X3D_PointLight *node) {

	if (!renderstate()->render_light) return;
	/* this will be a global light here... */
	render_PointLight(node);
}

void compile_SpotLight (struct X3D_SpotLight *node) {
    struct point_XYZ vec;
    int i;
    
    for (i=0; i<3; i++) node->_loc.c[i] = node->location.c[i];
    node->_loc.c[3] = 1.0f;/* 1 == this is a position, not a vector */


    vec.x = (double) node->direction.c[0];
    vec.y = (double) node->direction.c[1];
    vec.z = (double) node->direction.c[2];
    normalize_vector(&vec);
    node->_dir.c[0] = (float) vec.x;
    node->_dir.c[1] = (float) vec.y;
    node->_dir.c[2] = (float) vec.z;
    node->_dir.c[3] = 1.0f;/* 1.0 = SpotLight */

    node->_col.c[0] = ((node->color).c[0]) * (node->intensity);
    node->_col.c[1] = ((node->color).c[1]) * (node->intensity);
    node->_col.c[2] = ((node->color).c[2]) * (node->intensity);
    node->_col.c[3] = 1;
    CHBOUNDS(node->_col);
    
    
    node->_amb.c[0] = ((node->color).c[0]) * (node->ambientIntensity);
    node->_amb.c[1] = ((node->color).c[1]) * (node->ambientIntensity);
    node->_amb.c[2] = ((node->color).c[2]) * (node->ambientIntensity);
    node->_amb.c[3] = 1;
    CHBOUNDS(node->_amb);

    MARK_NODE_COMPILED;
}


void render_SpotLight(struct X3D_SpotLight *node) {
	float ft;

	/* if we are doing global lighting, is this one for us? */
	RETURN_IF_LIGHT_STATE_NOT_US

    COMPILE_IF_REQUIRED;

	if(node->on) {
		int light = nextlight();
		if(light >= 0) {
			setLightState(light,TRUE);
			setLightType(light,1);
			FW_GL_LIGHTFV(light, GL_SPOT_DIRECTION, node->_dir.c);
			FW_GL_LIGHTFV(light, GL_POSITION, node->_loc.c);
	
			FW_GL_LIGHTF(light, GL_CONSTANT_ATTENUATION,
					((node->attenuation).c[0]));
			FW_GL_LIGHTF(light, GL_LINEAR_ATTENUATION,
					((node->attenuation).c[1]));
			FW_GL_LIGHTF(light, GL_QUADRATIC_ATTENUATION,
					((node->attenuation).c[2]));
	
            FW_GL_LIGHTFV(light, GL_DIFFUSE, node->_col.c);
			FW_GL_LIGHTFV(light, GL_SPECULAR, node->_col.c);
			FW_GL_LIGHTFV(light, GL_AMBIENT, node->_amb.c);
            
			ft =(float)cos((node->beamWidth)/2.0); /*  / (PI/4.0); */
			FW_GL_LIGHTF(light, GL_SPOT_BEAMWIDTH,ft);
            //ConsoleMessage ("spotLight, bw %f, cuta %f, PI/4 %f", node->beamWidth,node->cutOffAngle, PI/4.0);
            
            /* create a ratio of light in relation to PI/4.0 */
            ft = (float)cos(node->cutOffAngle/2.0); /* / (PI/4.0); */ 
			FW_GL_LIGHTF(light, GL_SPOT_CUTOFF, ft);
            setLightChangedFlag(light);
            //not used in spotlight calculation FW_GL_LIGHTF(light,GL_LIGHT_RADIUS,node->radius);
		}
	}
}
/* SpotLights are done before the rendering of geometry */
void prep_SpotLight (struct X3D_SpotLight *node) {
	if (!renderstate()->render_light) return;
	render_SpotLight(node);
}

int getLocalLight();
void pushLocalLight(int lastlight);
void popLocalLight();


void sib_prep_DirectionalLight(struct X3D_Node *parent, struct X3D_Node *sibAffector){
	int lastlight;
	//if ((parent->_renderFlags & VF_localLight)==VF_localLight && renderstate()->render_light != VF_globalLight){
	if ( renderstate()->render_light != VF_globalLight){
	  saveLightState2(&lastlight);
	  pushLocalLight(lastlight);
	  render_DirectionalLight((struct X3D_DirectionalLight*)sibAffector);
	}
}
void sib_prep_SpotlLight(struct X3D_Node *parent, struct X3D_Node *sibAffector){
	int lastlight;
	if ( renderstate()->render_light != VF_globalLight){
	  saveLightState2(&lastlight);
	  pushLocalLight(lastlight);
	  render_SpotLight((struct X3D_SpotLight*)sibAffector);
	}

}
void sib_prep_PointLight(struct X3D_Node *parent, struct X3D_Node *sibAffector){
	int lastlight;
	if ( renderstate()->render_light != VF_globalLight){
	  saveLightState2(&lastlight);
	  pushLocalLight(lastlight);
	  render_PointLight((struct X3D_SpotLight*)sibAffector);
	}
}


void sib_fin_DirectionalLight(struct X3D_Node *parent, struct X3D_Node *sibAffector){
	int lastlight;
	if ( renderstate()->render_light != VF_globalLight) {
		lastlight = getLocalLight();
		if(numberOfLights() > lastlight) {
			setLightChangedFlag(numberOfLights()-1);
			refreshLightUniforms();
		}
		restoreLightState2(lastlight);
		popLocalLight();
	}
}
void sib_fin_SpotlLight(struct X3D_Node *parent, struct X3D_Node *sibAffector){
	int lastlight;
	if (renderstate()->render_light != VF_globalLight) {
		lastlight = getLocalLight();
		if(numberOfLights() > lastlight) {
			setLightChangedFlag(numberOfLights()-1);
			refreshLightUniforms();
		}
		restoreLightState2(lastlight);
		popLocalLight();
	}
}
void sib_fin_PointLight(struct X3D_Node *parent, struct X3D_Node *sibAffector){
	int lastlight;
	if (renderstate()->render_light != VF_globalLight) {
		lastlight = getLocalLight();
		if(numberOfLights() > lastlight) {
			setLightChangedFlag(numberOfLights()-1);
			refreshLightUniforms();
		}
		popLocalLight();
		restoreLightState2(lastlight);
	}
}

