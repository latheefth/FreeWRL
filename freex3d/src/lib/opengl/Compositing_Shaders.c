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

/*	Aug 9, 2016, Castle Game Engine has given us/freewrl-sourceforge 
	special permission to implement their system
	for compositing shader permutations via special PLUGS, for use in libfreewrl.
	For use outside of libfreewrl, please consult Castle Game Engine
	http://castle-engine.sourceforge.net/compositing_shaders.php

	In the starting default/base shader, structured for web3d lighting model, with PLUG deckarations:
		/* PLUG: texture_apply (fragment_color, normal_eye_fragment) */
/*
	In an additive effect shader:
        void PLUG_texture_color(inout vec4 texture_color,
          const in vec4 tex_coord)
        {

	The idea is to call the additive effect function from the main shader, where the same PLUG: <name> is
	1. paste the additive effect function at the bottom of the main shader, with a uniquized name
	2. put a forward declaration above the call
	3. call at the matching PLUG location
	4. (add in more effects same way)
	5. compile shaderparts and shaderprogram
	6. record permutation for re-use, as hash or bitflag of requirements/effects
*/
#include <config.h>
#include <system.h>
#include <system_threads.h>

#include <display.h>
#include <internal.h>

#include <libFreeWRL.h>
#define TRUE 1
#define FALSE 0
//type
//  TShaderType = (vertex, geometry, fragment);
//  TShaderSource = array [TShaderType] of a string list;
//
enum TShaderType {
	SHADERPART_VERTEX,
	SHADERPART_GEOMETRY,
	SHADERPART_FRAGMENT
};
//var
//  { shader for the whole shape }
//  FinalShader: TShaderSource;


char *insertBefore(char *current, char *insert, char* wholeBuffer, int wholeBuffersize){
	//inserts insert at current location
	char *newcurrent, *here;
	int insertlen, wholelen, need, movesize;

	wholelen = strlen(current) +1; //plus 1 to get the \0 at the end in memmove
	insertlen = strlen(insert);
	need = wholelen + insertlen + 1;
	movesize = wholelen;
	if(need < wholeBuffersize){
		here = current;
		newcurrent = current + insertlen;
		memmove(newcurrent,current,movesize);
		memcpy(here,insert,insertlen);
	}else{
		ConsoleMessage("Not enough buffer for compositing shader buffsz=%d need=%d\n",wholeBuffersize,need);
	}
	//if(1){
	//	char *tmp = strdup(wholeBuffer);
	//	printf("strdup: %s",tmp);
	//	free(tmp);
	//}
	return newcurrent;
}
void removeAt(char *here, int len){
	//removes len bytes from string, moving tail bytes up by len bytes
	int wholelen, movesize;
	char *source;
	wholelen = strlen(here) + 1; //take the \0
	source = here + len;
	movesize = wholelen - len;
	memmove(here,source,movesize);
}

void extractPlugCall(char *start, char *PlugName,char *PlugParameters){
	//from the addon shader, get the name PLUG_<name>(declaredParameters)
	char *pns, *pne, *pps, *ppe;
	int len;
	pns = strstr(start,"PLUG: ");
	pns += strlen("PLUG: ");
	pne = strchr(pns,' ');
	len = pne - pns;
	strncpy(PlugName,pns,len);
	PlugName[len] = '\0';
	pps = strchr(pne,'(');
	ppe = strchr(pps,')') + 1;
	len = ppe - pps;
	strncpy(PlugParameters,pps,len);
	PlugParameters[len] = '\0';
	printf("PlugName %s PlugParameters %s\n",PlugName,PlugParameters);
}
//{ Look for /* PLUG: PlugName (...) */ inside
	//given CodeForPlugDeclaration.
	//Return if any occurrence found. }
	//function LookForPlugDeclaration(
	//  CodeForPlugDeclaration: string list): boolean;
	//begin
int LookForPlugDeclarations( char * CodeForPlugDeclarations, int bsize, char *PlugName, char *ProcedureName, char *ForwardDeclaration) {
	//in the main code, look for matching PLUG: <PlugName>(plugparams) and place a call to ProcedureName(plugparams)
	//Result := false
	//for each S: string in CodeForPlugDeclaration do
	//begin
	int i, Result;
	Result = FALSE;
	//i = 0;
	//while(CodeForPlugDeclarations[i]){
		char *S;
		char MainPlugName[100], MainPlugParams[1000];
		//char PlugDeclarationBuffer[10000];
		int AnyOccurrencesHere = FALSE; //:= false
		//strcpy_s(PlugDeclarationBuffer,10000,*CodeForPlugDeclarations);
		S = CodeForPlugDeclarations;
		do {
			//while we can find an occurrence
			//  of /* PLUG: PlugName (...) */ inside S do
			//begin
			S = strstr(S,"/* PLUG: ");
			if(S)
				printf("found PLUG:\n");
			else
				printf("PLUG: not found\n");
			if(S){  ////where = strstr(haystack,needle)
				char ProcedureCallBuffer[500], *ProcedureCall;
				extractPlugCall(S,MainPlugName,MainPlugParams);
				if(!strcmp(MainPlugName,PlugName)){
					//found the place in the main shader to make the call to addon function
					//insert into S a call to ProcedureName,
					//with parameters specified inside the /* PLUG: PlugName (...) */,
					//right before the place where we found /* PLUG: PlugName (...) */
					ProcedureCall = ProcedureCallBuffer;
					sprintf(ProcedureCall,"%s%s;\n",ProcedureName,MainPlugParams);
					S = insertBefore(S,ProcedureCall,CodeForPlugDeclarations,bsize);
					AnyOccurrencesHere = TRUE; //:= true
					Result = TRUE; //:= true
				}else{
					printf("found a PLUG: %s but doesn't match PLUG_%s\n",MainPlugName,PlugName);
				}
				S += strlen("/* PLUG:") + strlen(MainPlugName) + strlen(MainPlugParams);
			}
		}
		while(S); //AnyOccurrencesHere);
		//end

		//if AnyOccurrencesHere then
		if(AnyOccurrencesHere){
			//insert the PlugForwardDeclaration into S,
			//at the place of /* PLUG-DECLARATIONS */ inside
			//(or at the beginning, if no /* PLUG-DECLARATIONS */)
			S = CodeForPlugDeclarations;
			S = strstr(S,"/* PLUG-DECLARATIONS */");
			if(!S) S = CodeForPlugDeclarations;
			S = insertBefore(S,ForwardDeclaration,CodeForPlugDeclarations,bsize);
			//S = *CodeForPlugDeclarations;
			//*CodeForPlugDeclarations = strdup(PlugDeclarationBuffer);
			//FREE_IF_NZ(S);
		}
	//	i++;
	//} //end
	return Result;
} //end


void replaceAll(char *buffer,int bsize, char *oldstr, char *newstr){
	char *found;
	while(found = strstr(buffer,oldstr)){
		removeAt(found,strlen(oldstr));
		insertBefore(found,newstr,buffer,bsize);
	}

}
//procedure Plug(
//  EffectPartType: TShaderType;
//  PlugValue: string;
//  CompleteCode: TShaderSource);

void extractPlugName(char *start, char *PlugName,char *PlugDeclaredParameters){
	//from the addon shader, get the name PLUG_<name>(declaredParameters)
	char *pns, *pne, *pps, *ppe;
	int len;
	pns = strstr(start,"PLUG_");
	pns += strlen("PLUG_");
	pne = strchr(pns,' ');
	len = pne - pns;
	strncpy(PlugName,pns,len);
	PlugName[len] = '\0';
	pps = strchr(pne,'(');
	ppe = strchr(pps,')') + 1;
	len = ppe - pps;
	strncpy(PlugDeclaredParameters,pps,len);
	PlugDeclaredParameters[len] = '\0';
	printf("PlugName %s PlugDeclaredParameters %s\n",PlugName,PlugDeclaredParameters);
}
#define SBUFSIZE 10000 //must hold final size of composited shader part, could do per-gglobal-instance malloced buffer instead and resize to largest composited shader
#define PBUFSIZE 1000 //must hold largets PlugValue
void Plug( int EffectPartType, const char *PlugValue, char **CompleteCode, int *unique_int)
{
	//Algo: 
	// outer loop: search for PLUG_<name> in (addon) effect
	//   inner loop: search for matching PLUG: <name> in main shader
	//     if found, paste addon into main:
	//        a) forward declaration at top, 
	//        b) method call at PLUG: point 
	//        c) method definition at bottom
	//var
	//  PlugName, ProcedureName, PlugForwardDeclaration: string;
	char PlugName[100], PlugDeclaredParameters[1000], PlugForwardDeclaration[1000], ProcedureName[100], PLUG_PlugName[100];
	char Code[SBUFSIZE], Plug[PBUFSIZE];
	int HasGeometryMain = FALSE, AnyOccurrences;
	char *found, *end;
	errno_t err;

	//var
	// Code: string list;
	//begin
	
	if(!CompleteCode[EffectPartType]) return;
	err = strcpy_s(Code,SBUFSIZE, CompleteCode[EffectPartType]);
	err = strcpy_s(Plug,PBUFSIZE, PlugValue);

	//HasGeometryMain := HasGeometryMain or
	//  ( EffectPartType = geometry and
	//    PlugValue contains 'main()' );
	HasGeometryMain = HasGeometryMain || EffectPartType == SHADERPART_GEOMETRY && strstr("main(",Plug);

	//while we can find PLUG_xxx inside PlugValue do
	//begin
	found = Plug;
	do {
		found = strstr(found,"void PLUG_"); //where = strstr(haystack,needle)
		//if(!found)
		//	printf("I'd like to complain: void PLUG_ isn't found in the addon\n");
		if(found){
			//PlugName := the plug name we found, the "xxx" inside PLUG_xxx
			//PlugDeclaredParameters := parameters declared at PLUG_xxx function
			extractPlugName(found,PlugName,PlugDeclaredParameters);
			found += strlen("void PLUG_") + strlen(PlugName) + strlen(PlugDeclaredParameters);
			//{ Rename found PLUG_xxx to something unique. }
			//ProcedureName := generate new unique procedure name,
			//for example take 'plugged_' + some unique integer
			sprintf(ProcedureName,"%s_%d",PlugName,(*unique_int));
			(*unique_int)++;

			//replace inside PlugValue all occurrences of 'PLUG_' + PlugName
			//with ProcedureName
			sprintf(PLUG_PlugName,"%s%s","PLUG_",PlugName);
			replaceAll(Plug,PBUFSIZE,PLUG_PlugName,ProcedureName);

			//PlugForwardDeclaration := 'void ' + ProcedureName +
			//PlugDeclaredParameters + ';' + newline
			sprintf(PlugForwardDeclaration,"void %s%s;\n",ProcedureName,PlugDeclaredParameters);

			//AnyOccurrences := LookForPlugDeclaration(Code)
			AnyOccurrences = LookForPlugDeclarations(Code,SBUFSIZE, PlugName,ProcedureName,PlugForwardDeclaration);

			/* If the plug declaration is not found in Code, then try to find it
				in the final shader. This happens if Code is special for given
				light/texture effect, but PLUG_xxx is not special to the
				light/texture effect (it is applicable to the whole shape as well).
				For example, using PLUG_vertex_object_space inside
				the X3DTextureNode.effects. 
			*/
			//if not AnyOccurrences and
			//	Code <> Source[EffectPartType] then
			//	AnyOccurrences := LookForPlugDeclaration(Source[EffectPartType])
			//if(!AnyOccurrences && Code != Source[EffectPartType]){
			//	AnyOccurrences = LookForPlugDeclarations(Source[EffectPartType]);
			//}
			//if not AnyOccurrences then
			//	Warning('Plug name ' + PlugName + ' not declared')
			//}
			if(!AnyOccurrences){
				ConsoleMessage("Plug name %s not declared\n",PlugName);
			}
		}
	}while(found);
	//end

	/*{ regardless if any (and how many) plug points were found,
	always insert PlugValue into Code. This way EffectPart with a library
	of utility functions (no PLUG_xxx inside) also works. }*/
	//Code.Add(PlugValue)
	//printf("strlen Code = %d strlen PlugValue=%d\n",strlen(Code),strlen(PlugValue));
	err = strcat_s(Code,SBUFSIZE,Plug);
	CompleteCode[EffectPartType] = strdup(Code);
} //end

//procedure EnableEffects(
//  Effects: list of Effect nodes;
//  CompleteCode: TShaderSource);
//begin
//  for each Effect in Effects do
//    if Effect.enabled and
//       Effect.language matches renderer shader language then
//      for each EffectPart in Effect.parts do
//        Plug(EffectPart.type, GetUrl(EffectPart.url), CompleteCode)
//end

void EnableEffects( struct Multi_Node *Effects, char **CompletedCode, int *unique_int){
	int i, ipart;
	for(i=0;i<Effects->n;i++){
		struct X3D_ShaderPart *node = (struct X3D_ShaderPart *)Effects->p[i];
		if(node->_nodeType == NODE_ShaderPart){
			if(!strcmp(node->type->strptr,"FRAGMENT"))
				ipart = SHADERPART_FRAGMENT;
			else if(!strcmp(node->type->strptr,"VERTEX"))
				ipart = SHADERPART_VERTEX;
			Plug(ipart,node->url.p[0]->strptr, CompletedCode, unique_int);
		}
	}
}


/* Generic GLSL vertex shader.
   Used by ../castlerendererinternalshader.pas to construct the final shader.

   This is converted to template.vs.inc, and is then compiled
   in program's binary.
   When you change this file, rerun `make' and then recompile Pascal sources.
*/


static const GLchar *genericVertex = "\
#version 110\n\
maxLights = 8;\n\
#if defined (GL_ES_VERSION_2_0)\n\
precision highp float;\n\
precision mediump float;\n\
#endif\n\
/* PLUG-DECLARATIONS */\n\
varying vec4 castle_vertex_eye;\n\
varying vec3 castle_normal_eye;\n\
\n\
void main(void)\n\
{\n\
  vec4 vertex_object = fw_Vertex;\n\
  vec3 normal_object = fw_Normal;\n\
  /* PLUG: vertex_object_space_change (vertex_object, normal_object) */\n\
  /* PLUG: vertex_object_space (vertex_object, normal_object) */\n\
  \n\
  castle_vertex_eye = fw_ModelViewMatrix * vertex_object;\n\
  /* Although we will normalize it again in the fragment shader\n\
     (otherwise interpolated result could be shorter < 1, imagine a case\n\
     when normals point the almost opposite directions on the opposite\n\
     vertexes), we also have to normalize it in vertex shader (otherwise\n\
     a much longer normal on one vertex would pull all the interpolated\n\
     normals, thus making their direction invalid in fragment shaders). */\n\
  castle_normal_eye = normalize(gl_NormalMatrix * normal_object);\n\
  \n\
  /* PLUG: vertex_eye_space (castle_vertex_eye, castle_normal_eye) */\n\
  \n\
#ifndef LIT\n\
  gl_FrontColor = gl_Color;\n\
  gl_BackColor = gl_Color;\n\
#endif\n\
\n\
#ifdef VERTEX_OBJECT_SPACE_CHANGED\n\
  gl_Position = fw_ProjectionMatrix * castle_vertex_eye;\n\
#else\n\
  gl_Position = ftransform();\n\
#endif\n\
}\n";

/* Generic GLSL fragment shader.
   Used by ../castlerendererinternalshader.pas to construct the final shader.

   This is converted to template.fs.inc, and is then compiled
   in program's binary.
   When you change this file, rerun `make' and then recompile Pascal sources.
*/


static const GLchar *genericFragment = "\
#version 110\n\
maxLights = 8;\n\
#if defined (GL_ES_VERSION_2_0)\n\
precision highp float;\n\
precision mediump float;\n\
#endif\n\
/* PLUG-DECLARATIONS */\n\
#ifdef HAS_GEOMETRY_SHADER\n\
  #define castle_vertex_eye castle_vertex_eye_geoshader\n\
  #define castle_normal_eye castle_normal_eye_geoshader\n\
#endif\n\
\n\
varying vec4 castle_vertex_eye;\n\
varying vec3 castle_normal_eye;\n\
\n\
/* Wrapper for calling PLUG texture_coord_shift */\n\
vec2 texture_coord_shifted(in vec2 tex_coord)\n\
{\n\
  /* PLUG: texture_coord_shift (tex_coord) */\n\
  return tex_coord;\n\
}\n\
\n\
void main(void)\n\
{\n\
  vec3 normal_eye_fragment = normalize(castle_normal_eye);\n\
  \n\
#ifndef CASTLE_BUGGY_FRONT_FACING\n\
  if (gl_FrontFacing)\n\
    /* Avoid AMD bug http://forums.amd.com/devforum/messageview.cfm?catid=392&threadid=148827&enterthread=y \n\
       Observed on fglrx (proprietary ATI Linux driver), \n\
       with ATI Mobility Radeon HD 4300 (castle computer czarny), \n\
       since Ubuntu 11.4 (fglrx OpenGL version 3.3.10665).\n\
	   \n\
       It causes both (gl_FrontFacing) and (!gl_FrontFacing) to be true...\n\
       To minimize the number of problems, never use if (!gl_FrontFacing),\n\
       only if (gl_FrontFacing).\n\
    */ ; else\n\
    normal_eye_fragment = -normal_eye_fragment;\n\
#endif\n\
\n\
  /* PLUG: fragment_eye_space (castle_vertex_eye, normal_eye_fragment) */\n\
  \n\
#ifdef LIT\n\
  vec4 fragment_color;\n\
  \n\
#ifndef CASTLE_BUGGY_FRONT_FACING\n\
  if (gl_FrontFacing)\n\
  {\n\
#endif\n\
    fragment_color = gl_FrontLightModelProduct.sceneColor;\n\
    /* PLUG: add_light_contribution_front (fragment_color, castle_vertex_eye, normal_eye_fragment, gl_FrontMaterial) */\n\
	\n\
    /* Otherwise, alpha is usually large after previous add_light_contribution,\n\
       and it's always opaque.\n\
       Using diffuse.a is actually exactly what fixed-function pipeline does\n\
       too, according to http://www.sjbaker.org/steve/omniv/opengl_lighting.html */\n\
    fragment_color.a = gl_FrontMaterial.diffuse.a;\n\
#ifndef CASTLE_BUGGY_FRONT_FACING\n\
  } else\n\
  {\n\
    fragment_color = gl_BackLightModelProduct.sceneColor;\n\
    /* PLUG: add_light_contribution_back (fragment_color, castle_vertex_eye, normal_eye_fragment, gl_BackMaterial) */\n\
    fragment_color.a = gl_BackMaterial.diffuse.a;\n\
  }\n\
#endif\n\
\n\
  /* Clamp sum of lights colors to be <= 1. Fixed-function OpenGL does it too.\n\
     This isn't really mandatory, but scenes with many lights could easily\n\
     have colors > 1 and then the textures will look burned out.\n\
     Of course, for future HDR rendering we will turn this off. */\n\
  fragment_color.rgb = min(fragment_color.rgb, 1.0);\n\
#else\n\
  vec4 fragment_color = gl_Color;\n\
#endif\n\
\n\
  /* PLUG: lighting_apply (fragment_color, castle_vertex_eye, normal_eye_fragment) */\n\
  \n\
  /* PLUG: texture_apply (fragment_color, normal_eye_fragment) */\n\
  /* PLUG: steep_parallax_shadow_apply (fragment_color) */\n\
  /* PLUG: fog_apply (fragment_color, normal_eye_fragment) */\n\
  \n\
  /* NVidia GeForce 450 GTS (kocury) fails to compile a shader when\n\
     we pass gl_FragColor as inout parameter to functions\n\
     (testcase even fresnel_and_toon.x3dv).\n\
     Although on Radeon X1600 (fglrx, chantal) it works OK.\n\
     So we just use fragment_color everywhere above, and only assign it\n\
     to gl_FragColor at the end. */\n\
  gl_FragColor = fragment_color;\n\
  \n\
  /* PLUG: fragment_end (gl_FragColor) */\n\
}\n";

const char *getGenericVertex(){
	return genericVertex;
}
const char *getGenericFragment(){
	return genericFragment;
}
#include "../scenegraph/Component_Shape.h"

static const GLchar *plug_fragment_end_anaglyph =	"\
void PLUG_fragment_end (inout vec3 finalFrag){ \n\
	float gray = dot(finalFrag.rgb, vec3(0.299, 0.587, 0.114)); \n\
	finalFrag = vec4(gray,gray,gray, finalFrag.a); \n\
}\n";

#define DESIRE(whichOne,zzz) ((whichOne & zzz)==zzz)
int getSpecificShaderSourceCastlePlugs (const GLchar **vertexSource, 
	const GLchar **fragmentSource, unsigned int whichOne, int usePhongShading) 
{
	//for building the Builtin (similar to fixed-function pipeline, except from shader parts)
	int retval, unique_int;
	char *CompleteCode[3];
	retval = FALSE;
	char *vs, *fs;
	if(whichOne & USER_DEFINED_SHADER_MASK) return retval; //not supported yet as of Aug 9, 2016

	//generic
	vs = strdup(getGenericVertex());
	fs = strdup(getGenericFragment());
	CompleteCode[SHADERPART_VERTEX] = vs;
	CompleteCode[SHADERPART_GEOMETRY] = NULL;
	CompleteCode[SHADERPART_FRAGMENT] = fs;

	unique_int = 0;
	//Add in:
	//Lit
	//Fog
	//analglyph
	if(DESIRE(whichOne,WANT_ANAGLYPH))
		Plug(SHADERPART_FRAGMENT,plug_fragment_end_anaglyph,CompleteCode,&unique_int);
	//color material
	//material appearance
	//2 material appearance
	//phong vs gourard
	//linespoints 
	//textureCoordinategen
	//cubemap texure
	//one tex appearance
	//multi tex appearance
	//cubemap tex
	//fill properties
	//
	*fragmentSource = fs;
	*vertexSource = vs;
	return retval;
}