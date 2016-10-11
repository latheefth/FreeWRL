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
	newcurrent = current;
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
	//printf("PlugName %s PlugParameters %s\n",PlugName,PlugParameters);
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
	int Result;
	//i = 0;
	//while(CodeForPlugDeclarations[i]){
		char *S;
		char MainPlugName[100], MainPlugParams[1000];
		//char PlugDeclarationBuffer[10000];
		int AnyOccurrencesHere = FALSE; //:= false
	Result = FALSE;
		//strcpy_s(PlugDeclarationBuffer,10000,*CodeForPlugDeclarations);
		S = CodeForPlugDeclarations;
		do {
			//while we can find an occurrence
			//  of /* PLUG: PlugName (...) */ inside S do
			//begin
			S = strstr(S,"/* PLUG: ");
			//if(S)
			//	printf("found PLUG:\n");
			//else
			//	printf("PLUG: not found\n");
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
					//printf("found a PLUG: %s but doesn't match PLUG_%s\n",MainPlugName,PlugName);
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
		}else{
			printf("didn't find PLUG_%s\n",PlugName);
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
//char *strpbrk(const char *str1, const char *str2) finds the first character in the string str1 that matches any character specified in str2. This does not include the terminating null-characters.
void extractPlugName(char *start, char *PlugName,char *PlugDeclaredParameters){
	//from the addon shader, get the name PLUG_<name>(declaredParameters)
	char *pns, *pne, *pps, *ppe;
	int len;
	pns = strstr(start,"PLUG_");
	pns += strlen("PLUG_");
	//pne = strchr(pns,' ');
	pne = strpbrk(pns," (");
	len = pne - pns;
	strncpy(PlugName,pns,len);
	PlugName[len] = '\0';
	pps = strchr(pne,'(');
	ppe = strchr(pps,')') + 1;
	len = ppe - pps;
	strncpy(PlugDeclaredParameters,pps,len);
	PlugDeclaredParameters[len] = '\0';
	//printf("PlugName %s PlugDeclaredParameters %s\n",PlugName,PlugDeclaredParameters);
}
#define SBUFSIZE 32767 //must hold final size of composited shader part, could do per-gglobal-instance malloced buffer instead and resize to largest composited shader
#define PBUFSIZE 16384 //must hold largets PlugValue
int fw_strcpy_s(char *dest, int destsize, const char *source){
	int ier = -1;
	if(dest)
	if(source && strlen(source) < (unsigned)destsize){
		strcpy(dest,source);
		ier = 0;
	}
	return ier;
}
int fw_strcat_s(char *dest, int destsize, const char *source){
	int ier = -1;
	if(dest){
		int destlen = strlen(dest);
		if(source)
			if(strlen(source)+destlen < (unsigned)destsize){
				strcat(dest,source);
				ier = 0;
			}
	}
	return ier;
}
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
	char *found;
	int err;

	//var
	// Code: string list;
	//begin
	
	if(!CompleteCode[EffectPartType]) return;
	err = fw_strcpy_s(Code,SBUFSIZE, CompleteCode[EffectPartType]);
	err = fw_strcpy_s(Plug,PBUFSIZE, PlugValue);

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
	err = fw_strcat_s(Code,SBUFSIZE,Plug);
	FREE_IF_NZ(CompleteCode[EffectPartType]);
	CompleteCode[EffectPartType] = strdup(Code);
} //end

void AddVersion( int EffectPartType, int versionNumber, char **CompleteCode){
	//puts #version <number> at top of shader, first line
	char Code[SBUFSIZE], line[1000];
	char *found;
	int err;

	if (!CompleteCode[EffectPartType]) return;
	err = fw_strcpy_s(Code, SBUFSIZE, CompleteCode[EffectPartType]);

	found = Code;
	if (found) {
		sprintf(line, "#version %d \n", versionNumber);
		insertBefore(found, line, Code, SBUFSIZE);
		FREE_IF_NZ(CompleteCode[EffectPartType]);
		CompleteCode[EffectPartType] = strdup(Code);
	}
}
void AddDefine0( int EffectPartType, const char *defineName, int defineValue, char **CompleteCode)
{
	//same as AddDEfine but you can say a number other than 1
	char Code[SBUFSIZE], line[1000];
	char *found;
	int err;

	if(!CompleteCode[EffectPartType]) return;
	err = fw_strcpy_s(Code,SBUFSIZE, CompleteCode[EffectPartType]);

	found = strstr(Code,"/* DEFINE"); 
	if(found){
		sprintf(line,"#define %s %d \n",defineName,defineValue);
		insertBefore(found,line,Code,SBUFSIZE);
		FREE_IF_NZ(CompleteCode[EffectPartType]);
		CompleteCode[EffectPartType] = strdup(Code);
	}
} 
void AddDefine( int EffectPartType, const char *defineName, char **CompleteCode){
	//adds #define <defineName> 1 to shader part, just above "/* DEFINES */" line in ShaderPart
	// char *CompleteCode[3] has char *vs *gs *fs parts, and will be realloced inside
	AddDefine0(EffectPartType,defineName,1,CompleteCode);
}

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
void EnableEffect(struct X3D_Node * node, char **CompletedCode, int *unique_int){
	int i, ipart;
	char *str;
	struct X3D_Effect *effect = (struct X3D_Effect *)node;
	for(i=0;i<effect->parts.n;i++){
		struct X3D_EffectPart *part = (struct X3D_EffectPart*)effect->parts.p[i];
		if(part->_nodeType == NODE_EffectPart){
			if(!strcmp(part->type->strptr,"FRAGMENT"))
				ipart = SHADERPART_FRAGMENT;
			else if(!strcmp(part->type->strptr,"VERTEX"))
				ipart = SHADERPART_VERTEX;
			str = part->url.p[0]->strptr;
			if(!strncmp(str,"data:text/plain,",strlen("data:text/plain,")))
				str += strlen("data:text/plain,");
			Plug(ipart,str, CompletedCode, unique_int);
		}
	}
}
Stack *getEffectStack();
void EnableEffects( char **CompletedCode, int *unique_int){
	int i;
	Stack *effect_stack;
	effect_stack = getEffectStack();
	for(i=0;i<vectorSize(effect_stack);i++){
		struct X3D_Node *node = vector_get(struct X3D_Node*,effect_stack,i);
		if(node->_nodeType == NODE_Effect){
			EnableEffect(node,CompletedCode,unique_int);
		}
	}
}



//maxLights = 8;\n\
//#if defined (GL_ES_VERSION_2_0)\n\
//precision highp float;\n\
//precision mediump float;\n\
//#endif\n\

/*
started with: http://svn.code.sf.net/p/castle-engine/code/trunk/castle_game_engine/src/x3d/opengl/glsl/template_mobile.vs
 castle						freewrl
 uniforms:
 castle_ModelViewMatrix		fw_ModelViewMatrix
 castle_ProjectionMatrix	fw_ProjectionMatrix
 castle_NormalMatrix		fw_NormalMatrix
 castle_MaterialDiffuseAlpha fw_FrontMaterial.diffuse.a
 castle_MaterialShininess	fw_FrontMaterial.shininess
 castle_SceneColor			fw_FrontMaterial.ambient
 castle_castle_UnlitColor	fw_FrontMaterial.emission
							fw_FrontMaterial.specular
 per-vertex attributes
 castle_Vertex				fw_Vertex
 castle_Normal				fw_Normal
 castle_ColorPerVertex		fw_Color

 defines
 LIT
 COLOR_PER_VERTEX
 CASTLE_BUGGY_GLSL_READ_VARYING

define LIT if have Shape->appearance->material and NOT linepoints
define TWO if you have backface colors ie X3DTwoSidedMaterial
  http://www.web3d.org/documents/specifications/19775-1/V3.3/Part01/components/shape.html#TwoSidedMaterial
define LINE if you have Shape->appearance->material and is linepoints (lines and points use mat.emissive)
define TEX if you have texture
define CPV if colorNode && image_channels < 3
define MAT if material is valid
*/

/* Generic GLSL vertex shader.
   Used by ../castlerendererinternalshader.pas to construct the final shader.

   This is converted to template.vs.inc, and is then compiled
   in program's binary.
   When you change this file, rerun `make' and then recompile Pascal sources.
*/

static const GLchar *genericVertexGLES2 = "\
/* DEFINES */ \n\
/* Generic GLSL vertex shader, used on OpenGL ES. */ \n\
 \n\
uniform mat4 fw_ModelViewMatrix; \n\
uniform mat4 fw_ProjectionMatrix; \n\
uniform mat3 fw_NormalMatrix; \n\
#ifdef CUB \n\
uniform mat4 fw_ModelViewInverseMatrix; \n\
#endif //CUB \n\
attribute vec4 fw_Vertex; \n\
attribute vec3 fw_Normal; \n\
 \n\
#ifdef TEX \n\
uniform mat4 fw_TextureMatrix0; \n\
attribute vec4 fw_MultiTexCoord0; \n\
//varying vec3 v_texC; \n\
varying vec3 fw_TexCoord[4]; \n\
#ifdef TEX3D \n\
uniform int tex3dUseVertex; \n\
#endif //TEX3D \n\
#ifdef MTEX \n\
uniform mat4 fw_TextureMatrix1; \n\
uniform mat4 fw_TextureMatrix2; \n\
uniform mat4 fw_TextureMatrix3; \n\
attribute vec4 fw_MultiTexCoord1; \n\
attribute vec4 fw_MultiTexCoord2; \n\
attribute vec4 fw_MultiTexCoord3; \n\
#endif //MTEX \n\
#ifdef TGEN \n\
 #define TCGT_CAMERASPACENORMAL    0  \n\
 #define TCGT_CAMERASPACEPOSITION    1 \n\
 #define TCGT_CAMERASPACEREFLECTION    2 \n\
 #define TCGT_COORD    3 \n\
 #define TCGT_COORD_EYE    4 \n\
 #define TCGT_NOISE    5 \n\
 #define TCGT_NOISE_EYE    6 \n\
 #define TCGT_SPHERE    7 \n\
 #define TCGT_SPHERE_LOCAL    8 \n\
 #define TCGT_SPHERE_REFLECT    9 \n\
 #define TCGT_SPHERE_REFLECT_LOCAL    10 \n\
 uniform int fw_textureCoordGenType; \n\
#endif //TGEN \n\
#endif //TEX \n\
#ifdef FILL \n\
varying vec2 hatchPosition; \n\
#endif //FILL \n\
\n\
/* PLUG-DECLARATIONS */ \n\
 \n\
varying vec4 castle_vertex_eye; \n\
varying vec3 castle_normal_eye; \n\
varying vec4 castle_Color; //DA diffuse ambient term \n\
 \n\
//uniform float castle_MaterialDiffuseAlpha; \n\
//uniform float castle_MaterialShininess; \n\
/* Color summed with all the lights. \n\
   Like gl_Front/BackLightModelProduct.sceneColor: \n\
   material emissive color + material ambient color * global (light model) ambient. \n\
*/ \n\
\n\
#ifdef LITE \n\
#define MAX_LIGHTS 8 \n\
uniform int lightcount; \n\
//uniform float lightRadius[MAX_LIGHTS]; \n\
uniform int lightType[MAX_LIGHTS];//ANGLE like this \n\
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
  float lightRadius; \n\
}; \n\
\n\
uniform fw_LightSourceParameters fw_LightSource[MAX_LIGHTS] /* gl_MaxLights */ ;\n\
#endif //LITE \n\
\n\
//uniform vec3 castle_SceneColor; \n\
//uniform vec4 castle_UnlitColor; \n\
#ifdef LIT \n\
struct fw_MaterialParameters { \n\
  vec4 emission; \n\
  vec4 ambient; \n\
  vec4 diffuse; \n\
  vec4 specular; \n\
  float shininess; \n\
}; \n\
uniform fw_MaterialParameters fw_FrontMaterial; \n\
varying vec3 castle_ColorES; //emissive shininess term \n\
vec3 castle_Emissive; \n\
#ifdef TWO \n\
uniform fw_MaterialParameters fw_BackMaterial; \n\
#endif //TWO \n\
#endif //LIT \n\
#ifdef FOG \n\
struct fogParams \n\
{  \n\
  vec4 fogColor; \n\
  float visibilityRange; \n\
  float fogScale; //applied on cpu side to visrange \n\
  int fogType; // 0 None, 1= FOGTYPE_LINEAR, 2 = FOGTYPE_EXPONENTIAL \n\
  // ifdefed int haveFogCoords; \n\
}; \n\
uniform fogParams fw_fogparams; \n\
#ifdef FOGCOORD \n\
attribute float fw_FogCoords; \n\
#endif \n\
#endif //FOG \n\
float castle_MaterialDiffuseAlpha; \n\
float castle_MaterialShininess; \n\
vec3 castle_SceneColor; \n\
vec4 castle_UnlitColor; \n\
vec4 castle_Specular; \n\
 \n\
#ifdef CPV \n\
attribute vec4 fw_Color; //castle_ColorPerVertex; \n\
varying vec4 cpv_Color; \n\
#endif //CPV \n\
 \n\
 vec3 dehomogenize(in mat4 matrix, in vec4 vector){ \n\
	vec4 tempv = vector; \n\
	if(tempv.w == 0.0) tempv.w = 1.0; \n\
	vec4 temp = matrix * tempv; \n\
	float winv = 1.0/temp.w; \n\
	return temp.xyz * winv; \n\
 } \n\
void main(void) \n\
{ \n\
  #ifdef LIT \n\
  castle_MaterialDiffuseAlpha = fw_FrontMaterial.diffuse.a; \n\
  #ifdef TEX \n\
  #ifdef TAREP \n\
  //to modulate or not to modulate, this is the question \n\
  //in here, we turn off modulation and use image alpha \n\
  castle_MaterialDiffuseAlpha = 1.0; \n\
  #endif //TAREP \n\
  #endif //TEX \n\
  castle_MaterialShininess =	fw_FrontMaterial.shininess; \n\
  castle_SceneColor = fw_FrontMaterial.ambient.rgb; \n\
  castle_Specular =	fw_FrontMaterial.specular; \n\
  castle_Emissive = fw_FrontMaterial.emission.rgb; \n\
  #ifdef LINE \n\
   castle_SceneColor = vec3(0.0,0.0,0.0); //line gets color from castle_Emissive \n\
  #endif //LINE\n\
  #else //LIT \n\
  castle_UnlitColor = vec4(1.0,1.0,1.0,1.0); \n\
  castle_MaterialDiffuseAlpha = 1.0; \n\
  #endif //LIT \n\
  \n\
  #ifdef FILL \n\
  hatchPosition = fw_Vertex.xy; \n\
  #endif //FILL \n\
  \n\
  vec4 vertex_object = fw_Vertex; \n\
  vec3 normal_object = fw_Normal; \n\
  /* PLUG: vertex_object_space_change (vertex_object, normal_object) */ \n\
  /* PLUG: vertex_object_space (vertex_object, normal_object) */ \n\
   \n\
  #ifdef CASTLE_BUGGY_GLSL_READ_VARYING \n\
  /* use local variables, instead of reading + writing to varying variables, \n\
     when VARYING_NOT_READABLE */ \n\
  vec4 temp_castle_vertex_eye; \n\
  vec3 temp_castle_normal_eye; \n\
  vec4 temp_castle_Color; \n\
  #define castle_vertex_eye temp_castle_vertex_eye \n\
  #define castle_normal_eye temp_castle_normal_eye \n\
  #define castle_Color      temp_castle_Color \n\
  #endif //CASTLE_BUGGY_GLSL_READ_VARYING \n\
  \n\
  castle_vertex_eye = fw_ModelViewMatrix * vertex_object; \n\
  castle_normal_eye = normalize(fw_NormalMatrix * normal_object); \n\
  \n\
  /* PLUG: vertex_eye_space (castle_vertex_eye, castle_normal_eye) */ \n\
   \n\
  #ifdef LIT \n\
  castle_ColorES = castle_Emissive; \n\
  castle_Color = vec4(castle_SceneColor, 1.0); \n\
  /* PLUG: add_light_contribution2 (castle_Color, castle_ColorES, castle_vertex_eye, castle_normal_eye, castle_MaterialShininess) */ \n\
  /* PLUG: add_light_contribution (castle_Color, castle_vertex_eye, castle_normal_eye, castle_MaterialShininess) */ \n\
  castle_Color.a = castle_MaterialDiffuseAlpha; \n\
  /* Clamp sum of lights colors to be <= 1. See template.fs for comments. */ \n\
  castle_Color.rgb = min(castle_Color.rgb, 1.0); \n\
  #else //LIT \n\
  castle_Color.rgb = castle_UnlitColor.rgb; \n\
  #endif //LIT \n\
  \n\
  #ifdef CPV //color per vertex \n\
  cpv_Color = fw_Color; \n\
  #endif //CPV \n\
  \n\
  #ifdef TEX \n\
  vec4 texcoord = fw_MultiTexCoord0; \n\
  #ifdef TEX3D \n\
  //to re-use vertex coords as texturecoords3D, we need them in 0-1 range: CPU calc of fw_TextureMatrix0 \n\
  if(tex3dUseVertex == 1) \n\
    texcoord = vec4(fw_Vertex.xyz,1.0); \n\
  #endif //TEX3D \n\
  #ifdef TGEN  \n\
  { \n\
    vec3 vertexNorm; \n\
    vec4 vertexPos; \n\
	vec3 texcoord3 = texcoord.xyz; \n\
    vertexNorm = normalize(fw_NormalMatrix * fw_Normal); \n\
    vertexPos = fw_ModelViewMatrix * fw_Vertex; \n\
    /* sphereEnvironMapping Calculation */  \n\
    vec3 u=normalize(vec3(vertexPos)); /* u is normalized position, used below more than once */ \n\
    vec3 r= reflect(u,vertexNorm); \n\
    if (fw_textureCoordGenType==TCGT_SPHERE) { /* TCGT_SPHERE  GL_SPHERE_MAP OpenGL Equiv */ \n\
      float m=2.0 * sqrt(r.x*r.x + r.y*r.y + (r.z*1.0)*(r.z*1.0)); \n\
      texcoord3 = vec3(r.x/m+0.5,r.y/m+0.5,0.0); \n\
    }else if (fw_textureCoordGenType==TCGT_CAMERASPACENORMAL) { \n\
      /* GL_REFLECTION_MAP used for sampling cubemaps */ \n\
      float dotResult = 2.0 * dot(u,r); \n\
      texcoord3 = vec3(u-r)*dotResult; \n\
    }else if (fw_textureCoordGenType==TCGT_COORD) { \n\
      /* 3D textures can use coords in 0-1 range */ \n\
      texcoord3 = fw_Vertex.xyz; //xyz; \n\
    } else { /* default usage - like default CubeMaps */ \n\
      vec3 u=normalize(vec3(fw_ProjectionMatrix * fw_Vertex)); /* myEyeVertex */  \n\
      texcoord3 =    reflect(u,vertexNorm); \n\
    } \n\
	texcoord.xyz = texcoord3; \n\
  } \n\
  #endif //TGEN \n\
  fw_TexCoord[0] = dehomogenize(fw_TextureMatrix0, texcoord); \n\
  #ifdef MTEX \n\
  fw_TexCoord[1] = dehomogenize(fw_TextureMatrix1,fw_MultiTexCoord1); \n\
  fw_TexCoord[2] = dehomogenize(fw_TextureMatrix2,fw_MultiTexCoord2); \n\
  fw_TexCoord[3] = dehomogenize(fw_TextureMatrix3,fw_MultiTexCoord3); \n\
  #endif //MTEX \n\
  #endif //TEX \n\
  \n\
  gl_Position = fw_ProjectionMatrix * castle_vertex_eye; \n\
  \n\
  #ifdef CUB \n\
  //cubemap \n\
  vec4 camera = fw_ModelViewInverseMatrix * vec4(0.0,0.0,0.0,1.0); \n\
  //vec3 u = normalize( vec4(castle_vertex_eye - camera).xyz ); \n\
  vec3 u = normalize( vec4(vertex_object + camera).xyz ); \n\
  vec3 v = normalize(fw_Normal); \n\
  fw_TexCoord[0] = normalize(reflect(u,v)); //computed in object space \n\
  fw_TexCoord[0].st = -fw_TexCoord[0].st; //helps with renderman cubemap convention \n\
  #endif //CUB \n\
  #ifdef CASTLE_BUGGY_GLSL_READ_VARYING \n\
  #undef castle_vertex_eye \n\
  #undef castle_normal_eye \n\
  #undef castle_Color \n\
  castle_vertex_eye = temp_castle_vertex_eye; \n\
  castle_normal_eye = temp_castle_normal_eye; \n\
  castle_Color      = temp_castle_Color; \n\
  #endif //CASTLE_BUGGY_GLSL_READ_VARYING \n\
  \n\
  #ifdef FOG \n\
  #ifdef FOGCOORD \n\
  castle_vertex_eye.z = fw_FogCoords; \n\
  #endif //FOGCOORD \n\
  #endif //FOG \n\
} \n";


/* Generic GLSL fragment shader.
   Used by ../castlerendererinternalshader.pas to construct the final shader.

   This is converted to template.fs.inc, and is then compiled
   in program's binary.
   When you change this file, rerun `make' and then recompile Pascal sources.
*/
/* 
	started with: http://svn.code.sf.net/p/castle-engine/code/trunk/castle_game_engine/src/x3d/opengl/glsl/template_mobile.fs
  defines:
  GL_ES_VERSION_2_0 - non-desktop
  HAS_GEOMETRY_SHADER - version 3+ gles
*/



















static const GLchar *genericFragmentGLES2 = "\
/* DEFINES */ \n\
#ifdef MOBILE \n\
//precision highp float; \n\
precision mediump float; \n\
#endif //MOBILE \n\
/* Generic GLSL fragment shader, used on OpenGL ES. */ \n\
 \n\
varying vec4 castle_Color; \n\
 \n\
#ifdef LITE \n\
#define MAX_LIGHTS 8 \n\
uniform int lightcount; \n\
//uniform float lightRadius[MAX_LIGHTS]; \n\
uniform int lightType[MAX_LIGHTS];//ANGLE like this \n\
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
  float lightRadius; \n\
}; \n\
\n\
uniform fw_LightSourceParameters fw_LightSource[MAX_LIGHTS] /* gl_MaxLights */ ;\n\
#endif //LITE \n\
\n\
#ifdef CPV \n\
varying vec4 cpv_Color; \n\
#endif //CPV \n\
\n\
#ifdef TEX \n\
#ifdef CUB \n\
uniform samplerCube fw_Texture_unit0; \n\
#else //CUB \n\
uniform sampler2D fw_Texture_unit0; \n\
#endif //CUB \n\
varying vec3 fw_TexCoord[4]; \n\
#ifdef TEX3D \n\
uniform int tex3dDepth; \n\
uniform int repeatSTR[3]; \n\
uniform int magFilter; \n\
#endif //TEX3D \n\
#ifdef TEX3DLAY \n\
uniform sampler2D fw_Texture_unit1; \n\
uniform sampler2D fw_Texture_unit2; \n\
uniform sampler2D fw_Texture_unit3; \n\
uniform int textureCount; \n\
#endif //TEX3DLAY \n\
#ifdef MTEX \n\
uniform sampler2D fw_Texture_unit1; \n\
uniform sampler2D fw_Texture_unit2; \n\
uniform sampler2D fw_Texture_unit3; \n\
uniform ivec2 fw_Texture_mode0;  \n\
uniform ivec2 fw_Texture_mode1;  \n\
uniform ivec2 fw_Texture_mode2;  \n\
uniform ivec2 fw_Texture_mode3;  \n\
uniform ivec2 fw_Texture_source0;  \n\
uniform ivec2 fw_Texture_source1;  \n\
uniform ivec2 fw_Texture_source2;  \n\
uniform ivec2 fw_Texture_source3;  \n\
uniform int fw_Texture_function0;  \n\
uniform int fw_Texture_function1;  \n\
uniform int fw_Texture_function2;  \n\
uniform int fw_Texture_function3;  \n\
uniform int textureCount; \n\
uniform vec4 mt_Color; \n\
#define MTMODE_ADD	1\n \
#define MTMODE_ADDSIGNED	2\n \
#define MTMODE_ADDSIGNED2X	3\n \
#define MTMODE_ADDSMOOTH	4\n \
#define MTMODE_BLENDCURRENTALPHA	5\n \
#define MTMODE_BLENDDIFFUSEALPHA	6\n \
#define MTMODE_BLENDFACTORALPHA	7\n \
#define MTMODE_BLENDTEXTUREALPHA	8\n \
#define MTMODE_DOTPRODUCT3	9\n \
#define MTMODE_MODULATE	10\n \
#define MTMODE_MODULATE2X	11\n \
#define MTMODE_MODULATE4X	12\n \
#define MTMODE_MODULATEALPHA_ADDCOLOR	13\n \
#define MTMODE_MODULATEINVALPHA_ADDCOLOR	14\n \
#define MTMODE_MODULATEINVCOLOR_ADDALPHA	15\n \
#define MTMODE_OFF	16\n \
#define MTMODE_REPLACE	17\n \
#define MTMODE_SELECTARG1	18\n \
#define MTMODE_SELECTARG2	19\n \
#define MTMODE_SUBTRACT	20\n \
#define MTSRC_DIFFUSE	1 \n\
#define MTSRC_FACTOR	2 \n\
#define MTSRC_SPECULAR	3 \n\
#define MTFN_ALPHAREPLICATE	0 \n\
#define MTFN_COMPLEMENT	1 \n\
#define MT_DEFAULT -1 \n\
\n\
void finalColCalc(inout vec4 prevColour, in int mode, in int modea, in int func, in sampler2D tex, in vec2 texcoord) { \n\
  vec4 texel = texture2D(tex,texcoord); \n\
  vec4 rv = vec4(1.,0.,1.,1.);   \n\
  if (mode==MTMODE_OFF) {  \n\
    rv = vec4(prevColour); \n\
  } else if (mode==MTMODE_REPLACE) { \n\
    rv = vec4(texture2D(tex, texcoord)); \n\
  }else if (mode==MTMODE_MODULATE) {  \n\
    vec3 ct,cf;  \n\
    float at,af;  \n\
    cf = prevColour.rgb;  \n\
    af = prevColour.a;  \n\
    ct = texel.rgb;  \n\
    at = texel.a;  \n\
    rv = vec4(ct*cf, at*af);  \n\
  } else if (mode==MTMODE_MODULATE2X) {  \n\
    vec3 ct,cf;  \n\
    float at,af;  \n\
    cf = prevColour.rgb;  \n\
    af = prevColour.a;  \n\
    ct = texel.rgb;  \n\
    at = texel.a;  \n\
    rv = vec4(vec4(ct*cf, at*af)*vec4(2.,2.,2.,2.));  \n\
  }else if (mode==MTMODE_MODULATE4X) {  \n\
    vec3 ct,cf;  \n\
    float at,af;  \n\
    cf = prevColour.rgb; \n\
    af = prevColour.a;  \n\
    ct = texel.rgb;  \n\
    at = texel.a;  \n\
    rv = vec4(vec4(ct*cf, at*af)*vec4(4.,4.,4.,4.));  \n\
  }else if (mode== MTMODE_ADDSIGNED) { \n\
    rv = vec4 (prevColour + texel - vec4 (0.5, 0.5, 0.5, -.5));  \n\
  } else if (mode== MTMODE_ADDSIGNED2X) { \n\
    rv = vec4 ((prevColour + texel - vec4 (0.5, 0.5, 0.5, -.5))*vec4(2.,2.,2.,2.));  \n\
  } else if (mode== MTMODE_ADD) { \n\
    rv= vec4 (prevColour + texel);  \n\
  } else if (mode== MTMODE_SUBTRACT) { \n\
    rv = vec4 (texel - prevColour); //jas had prev - tex \n\
  } else if (mode==MTMODE_ADDSMOOTH) {  \n\
    rv = vec4 (prevColour + (prevColour - vec4 (1.,1.,1.,1.)) * texel);  \n\
  } else if (mode==MTMODE_BLENDDIFFUSEALPHA) {  \n\
    rv = vec4 (mix(prevColour,texel,castle_Color.a)); \n\
  } else if (mode==MTMODE_BLENDTEXTUREALPHA) {  \n\
    rv = vec4 (mix(prevColour,texel,texel.a)); \n\
  } else if (mode==MTMODE_BLENDFACTORALPHA) {  \n\
    rv = vec4 (mix(prevColour,texel,mt_Color.a)); \n\
  } else if (mode==MTMODE_BLENDCURRENTALPHA) {  \n\
    rv = vec4 (mix(prevColour,texel,prevColour.a)); \n\
  } else if (mode==MTMODE_SELECTARG1) {  \n\
    rv = texel;  \n\
  } else if (mode==MTMODE_SELECTARG2) {  \n\
    rv = prevColour;  \n\
  } \n\
  if(modea != 0){ \n\
    if (modea==MTMODE_OFF) {  \n\
      rv.a = prevColour.a; \n\
    } else if (modea==MTMODE_REPLACE) { \n\
      rv.a = 1.0; \n\
    }else if (modea==MTMODE_MODULATE) {  \n\
      float at,af;  \n\
      af = prevColour.a;  \n\
      at = texel.a;  \n\
      rv.a = at*af;  \n\
    } else if (modea==MTMODE_MODULATE2X) {  \n\
      float at,af;  \n\
      af = prevColour.a;  \n\
      at = texel.a;  \n\
      rv.a = at*af*2.0;  \n\
    }else if (modea==MTMODE_MODULATE4X) {  \n\
      float at,af;  \n\
      af = prevColour.a;  \n\
      at = texel.a;  \n\
      rv.a = at*af*4.0;  \n\
    }else if (modea== MTMODE_ADDSIGNED) { \n\
      rv.a = (prevColour.a + texel.a + .5);  \n\
    } else if (modea== MTMODE_ADDSIGNED2X) { \n\
      rv.a = ((prevColour.a + texel.a + .5))*2.0;  \n\
    } else if (modea== MTMODE_ADD) { \n\
      rv.a = prevColour.a + texel.a;  \n\
    } else if (modea== MTMODE_SUBTRACT) { \n\
      rv.a = texel.a - prevColour.a;  //jas had prev - texel \n\
    } else if (modea==MTMODE_ADDSMOOTH) {  \n\
      rv.a = (prevColour.a + (prevColour.a - 1.)) * texel.a;  \n\
    } else if (modea==MTMODE_BLENDDIFFUSEALPHA) {  \n\
      rv.a = mix(prevColour.a,texel.a,castle_Color.a); \n\
    } else if (modea==MTMODE_BLENDTEXTUREALPHA) {  \n\
      rv.a = mix(prevColour.a,texel.a,texel.a); \n\
    } else if (modea==MTMODE_BLENDFACTORALPHA) {  \n\
      rv.a = mix(prevColour.a,texel.a,mt_Color.a); \n\
    } else if (modea==MTMODE_BLENDCURRENTALPHA) {  \n\
      rv.a = mix(prevColour.a,texel.a,prevColour.a); \n\
    } else if (modea==MTMODE_SELECTARG1) {  \n\
      rv.a = texel.a;  \n\
    } else if (modea==MTMODE_SELECTARG2) {  \n\
      rv.a = prevColour.a;  \n\
    } \n\
  } \n\
  if(func == MTFN_COMPLEMENT){ \n\
	//rv = vec4(1.0,1.0,1.0,1.0) - rv; \n\
	rv = vec4( vec3(1.0,1.0,1.0) - rv.rgb, rv.a); \n\
  }else if(func == MTFN_ALPHAREPLICATE){ \n\
	rv = vec4(rv.a,rv.a,rv.a,rv.a); \n\
  } \n\
  prevColour = rv;  \n\
} \n\
#endif //MTEX \n\
#endif //TEX \n\
#ifdef FILL \n\
uniform vec4 HatchColour; \n\
uniform bool hatched; uniform bool filled;\n\
uniform vec2 HatchScale; \n\
uniform vec2 HatchPct; \n\
uniform int algorithm; \n\
varying vec2 hatchPosition; \n\
void fillPropCalc(inout vec4 prevColour, vec2 MCposition, int algorithm) { \n\
  vec4 colour; \n\
  vec2 position, useBrick; \n\
  \n\
  position = MCposition / HatchScale; \n\
  \n\
  if (algorithm == 0) {/* bricking  */ \n\
    if (fract(position.y * 0.5) > 0.5) \n\
      position.x += 0.5; \n\
  } \n\
  \n\
  /* algorithm 1, 2 = no futzing required here  */ \n\
  if (algorithm == 3) { /* positive diagonals */ \n\
    vec2 curpos = position; \n\
    position.x -= curpos.y; \n\
  } \n\
  \n\
  if (algorithm == 4) {  /* negative diagonals */ \n\
    vec2 curpos = position; \n\
    position.x += curpos.y; \n\
  } \n\
  \n\
  if (algorithm == 6) {  /* diagonal crosshatch */ \n\
    vec2 curpos = position; \n\
    if (fract(position.y) > 0.5)  { \n\
      if (fract(position.x) < 0.5) position.x += curpos.y; \n\
      else position.x -= curpos.y; \n\
    } else { \n\
      if (fract(position.x) > 0.5) position.x += curpos.y; \n\
      else position.x -= curpos.y; \n\
    } \n\
  } \n\
  \n\
  position = fract(position); \n\
  \n\
  useBrick = step(position, HatchPct); \n\
  \n\
  if (filled) {colour = prevColour;} else { colour=vec4(0.,0.,0.,0); }\n\
  if (hatched) { \n\
      colour = mix(HatchColour, colour, useBrick.x * useBrick.y); \n\
  } \n\
  prevColour = colour; \n\
} \n\
#endif //FILL \n\
#ifdef FOG \n\
struct fogParams \n\
{  \n\
  vec4 fogColor; \n\
  float visibilityRange; \n\
  float fogScale; \n\
  int fogType; // 0 None, 1= FOGTYPE_LINEAR, 2 = FOGTYPE_EXPONENTIAL \n\
  // ifdefed int haveFogCoords; \n\
}; \n\
uniform fogParams fw_fogparams; \n\
#endif //FOG \n\
 \n\
/* PLUG-DECLARATIONS */ \n\
 \n\
#ifdef HAS_GEOMETRY_SHADER \n\
#define castle_vertex_eye castle_vertex_eye_geoshader \n\
#define castle_normal_eye castle_normal_eye_geoshader \n\
#endif \n\
 \n\
varying vec4 castle_vertex_eye; \n\
varying vec3 castle_normal_eye; \n\
#ifdef LIT \n\
#ifdef LITE \n\
//per-fragment lighting ie phong \n\
struct fw_MaterialParameters { \n\
  vec4 emission; \n\
  vec4 ambient; \n\
  vec4 diffuse; \n\
  vec4 specular; \n\
  float shininess; \n\
}; \n\
uniform fw_MaterialParameters fw_FrontMaterial; \n\
#ifdef TWO \n\
uniform fw_MaterialParameters fw_BackMaterial; \n\
#endif //TWO \n\
vec3 castle_ColorES; \n\
#else //LITE \n\
//per-vertex lighting - interpolated Emissive-specular \n\
varying vec3 castle_ColorES; //emissive shininess term \n\
#endif //LITE \n\
#endif //LIT\n\
 \n\
/* Wrapper for calling PLUG texture_coord_shift */ \n\
vec2 texture_coord_shifted(in vec2 tex_coord) \n\
{ \n\
  /* PLUG: texture_coord_shift (tex_coord) */ \n\
  return tex_coord; \n\
} \n\
 \n\
vec4 matdiff_color; \n\
void main(void) \n\
{ \n\
  vec4 fragment_color = vec4(1.0,1.0,1.0,1.0); \n\
  matdiff_color = castle_Color; \n\
  float castle_MaterialDiffuseAlpha = castle_Color.a; \n\
  \n\
  #ifdef LITE \n\
  //per-fragment lighting aka PHONG \n\
  //start over with the color, since we have material and lighting in here \n\
  castle_MaterialDiffuseAlpha = fw_FrontMaterial.diffuse.a; \n\
  matdiff_color = vec4(0,0,0,1.0); \n\
  castle_ColorES = fw_FrontMaterial.emission.rgb; \n\
  /* PLUG: add_light_contribution2 (matdiff_color, castle_ColorES, castle_vertex_eye, castle_normal_eye, fw_FrontMaterial.shininess) */ \n\
  #endif //LITE \n\
  \n\
  #ifdef LIT \n\
  #ifdef MATFIR \n\
  fragment_color.rgb = matdiff_color.rgb; \n\
  #endif //MATFIR \n\
  #endif //LIT \n\
  \n\
  #ifdef CPV \n\
  #ifdef CPVREP \n\
  fragment_color = cpv_Color; //CPV replaces mat.diffuse prior \n\
  fragment_color.a *= castle_MaterialDiffuseAlpha; \n\
  #else \n\
  fragment_color *= cpv_Color; //CPV modulates prior \n\
  #endif //CPVREP \n\
  #endif //CPV \n\
  \n\
  #ifdef TEX \n\
  #ifdef TEXREP \n\
  fragment_color = vec4(1.0,1.0,1.0,1.0); //texture replaces prior \n\
  #endif //TEXREP \n\
  #endif //TEX \n\
  \n\
  /* Fragment shader on mobile doesn't get a normal vector now, for speed. */ \n\
  #define normal_eye_fragment castle_normal_eye //vec3(0.0) \n\
  \n\
  #ifdef FILL \n\
  fillPropCalc(fragment_color, hatchPosition, algorithm); \n\
  #endif //FILL \n\
  \n\
  #ifdef LIT \n\
  #ifndef MATFIR \n\
  //modulate texture with mat.diffuse \n\
  fragment_color.rgb *= matdiff_color.rgb; \n\
  fragment_color.a *= castle_MaterialDiffuseAlpha; \n\
  #endif //MATFIR \n\
  fragment_color.rgb = clamp(fragment_color.rgb + castle_ColorES, 0.0, 1.0); \n\
  #endif //LIT \n\
  \n\
  /* PLUG: texture_apply (fragment_color, normal_eye_fragment) */ \n\
  /* PLUG: steep_parallax_shadow_apply (fragment_color) */ \n\
  /* PLUG: fog_apply (fragment_color, normal_eye_fragment) */ \n\
  \n\
  #undef normal_eye_fragment \n\
  \n\
  gl_FragColor = fragment_color; \n\
  \n\
  /* PLUG: fragment_end (gl_FragColor) */ \n\
} \n";



const char *getGenericVertex(void){
	return genericVertexGLES2; //genericVertexDesktop
}
const char *getGenericFragment(){
	return genericFragmentGLES2; //genericFragmentDesktop;
}
#include "../scenegraph/Component_Shape.h"

static const GLchar *plug_fragment_end_anaglyph =	"\
void PLUG_fragment_end (inout vec4 finalFrag){ \n\
	float gray = dot(finalFrag.rgb, vec3(0.299, 0.587, 0.114)); \n\
	finalFrag = vec4(gray,gray,gray, finalFrag.a); \n\
}\n";

//TEXTURE 3D
/*	
	4 scenarios:
	1. GL has texture3D/EXT_texture3D/OES_texture3D
	2. GL no texture3D - emulate
	A. 3D image: source imagery is i) 3D image or ii) composed image with image layers all same size
	B. 2D layers: source imagery is composed image with z < 7 layers and layers can be different sizes
		1					2
	A	texture3D			tiled texture2D	
	B	multi texure2D		multi texture2D

	for tiled texture2D, there are a few ways to do the tiles:
	a) vertical strip: nx x (ny * nz) - our first attempt
		layer 0 at top, and sequential layers follow down
	b) squarish tiled: ix = iy = ceil(sqrt(nz)); (nx*ix) x (ny*iy)
		there will be blank squares. Order:
		y-first layer order: fill column, first at top left, before advancing ix right
		(option: x-first layer order: fill row, first at top left, before advancing down iy)
*/
static const GLchar *plug_fragment_texture3D_apply =	"\
void PLUG_texture_apply (inout vec4 finalFrag, in vec3 normal_eye_fragment ){ \n\
\n\
  #ifdef TEX3D \n\
  vec3 texcoord = fw_TexCoord[0]; \n\
  texcoord.z = 1.0 - texcoord.z; //flip z from RHS to LHS\n\
  float depth = max(1.0,float(tex3dDepth)); \n\
  if(repeatSTR[0] == 0) texcoord.x = clamp(texcoord.x,0.0001,.9999); \n\
  else texcoord.x = mod(texcoord.x,1.0); \n\
  if(repeatSTR[1] == 0) texcoord.y = clamp(texcoord.y,0.0001,.9999); \n\
  else texcoord.y = mod(texcoord.y,1.0); \n\
  if(repeatSTR[2] == 0) texcoord.z = clamp(texcoord.z,0.0001,.9999); \n\
  else texcoord.z = mod(texcoord.z,1.0); \n\
  vec4 texel; \n\
  int flay = int(floor(texcoord.z*depth)); \n\
  int clay = int(ceil(texcoord.z*depth)); \n\
  clay = clay == tex3dDepth ? 0 : clay; \n\
  vec4 ftexel, ctexel; \n\
  vec3 ftexcoord, ctexcoord; \n\
  ftexcoord = texcoord; \n\
  ctexcoord = texcoord; \n\
  ftexcoord.y += float(flay); \n\
  ftexcoord.y /= depth; \n\
  ctexcoord.y += float(clay); \n\
  ctexcoord.y /= depth; \n\
  ftexel = texture2D(fw_Texture_unit0,ftexcoord.st); \n\
  ctexel = texture2D(fw_Texture_unit0,ctexcoord.st); \n\
  float fraction = mod(ftexcoord.z*depth,1.0); \n\
  if(magFilter == 1) \n\
	texel = mix(ctexel,ftexel,1.0-fraction); //lerp GL_LINEAR \n\
  else \n\
	texel = ftexel; //fraction > .5 ? ctexel : ftexel; //GL_NEAREST \n\
  finalFrag *= texel; \n\
  #endif //TEX3D \n\
  \n\
}\n";

static const GLchar *plug_fragment_texture3Dlayer_apply =	"\
void PLUG_texture_apply (inout vec4 finalFrag, in vec3 normal_eye_fragment ){ \n\
\n\
  #ifdef TEX3DLAY \n\
  vec3 texcoord = fw_TexCoord[0]; \n\
  texcoord.z = 1.0 - texcoord.z; //flip z from RHS to LHS\n\
  float depth = max(1.0,float(textureCount-1)); \n\
  float delta = 1.0/depth; \n\
  if(repeatSTR[0] == 0) texcoord.x = clamp(texcoord.x,0.0001,.9999); \n\
  else texcoord.x = mod(texcoord.x,1.0); \n\
  if(repeatSTR[1] == 0) texcoord.y = clamp(texcoord.y,0.0001,.9999); \n\
  else texcoord.y = mod(texcoord.y,1.0); \n\
  if(repeatSTR[2] == 0) texcoord.z = clamp(texcoord.z,0.0001,.9999); \n\
  else texcoord.z = mod(texcoord.z,1.0); \n\
  int flay = int(floor(texcoord.z*depth)); \n\
  int clay = int(ceil(texcoord.z*depth)); \n\
  vec4 ftexel, ctexel; \n\
  //flay = 0; \n\
  //clay = 1; \n\
  if(flay == 0) ftexel = texture2D(fw_Texture_unit0,texcoord.st);  \n\
  if(clay == 0) ctexel = texture2D(fw_Texture_unit0,texcoord.st);  \n\
  if(flay == 1) ftexel = texture2D(fw_Texture_unit1,texcoord.st);  \n\
  if(clay == 1) ctexel = texture2D(fw_Texture_unit1,texcoord.st);  \n\
  if(flay == 2) ftexel = texture2D(fw_Texture_unit2,texcoord.st);  \n\
  if(clay == 2) ctexel = texture2D(fw_Texture_unit2,texcoord.st);  \n\
  if(flay == 3) ftexel = texture2D(fw_Texture_unit3,texcoord.st);  \n\
  if(clay == 3) ctexel = texture2D(fw_Texture_unit3,texcoord.st); \n\
  float fraction = mod(texcoord.z*depth,1.0); \n\
  vec4 texel; \n\
  if(magFilter == 1) \n\
	texel = mix(ctexel,ftexel,(1.0-fraction)); //lerp GL_LINEAR \n\
  else \n\
	texel = fraction > .5 ? ctexel : ftexel; //GL_NEAREST \n\
  finalFrag *= texel; \n\
  #endif //TEX3DLAY \n\
  \n\
}\n";

//MULTITEXTURE
// http://www.web3d.org/documents/specifications/19775-1/V3.3/Part01/components/texturing.html#MultiTexture
  /* PLUG: texture_apply (fragment_color, normal_eye_fragment) */
static const GLchar *plug_fragment_texture_apply =	"\
void PLUG_texture_apply (inout vec4 finalFrag, in vec3 normal_eye_fragment ){ \n\
\n\
  #ifdef MTEX \n\
  vec4 source; \n\
  int isource,iasource, mode; \n\
  //finalFrag = texture2D(fw_Texture_unit0, fw_TexCoord[0].st) * finalFrag; \n\
  if(textureCount>0){ \n\
    if(fw_Texture_mode0[0] != MTMODE_OFF) { \n\
      isource = fw_Texture_source0[0]; //castle-style dual sources \n\
      iasource = fw_Texture_source0[1]; \n\
      if(isource == MT_DEFAULT) source = finalFrag; \n\
      else if(isource == MTSRC_DIFFUSE) source = matdiff_color; \n\
      else if(isource == MTSRC_SPECULAR) source = vec4(castle_ColorES.rgb,1.0); \n\
      else if(isource == MTSRC_FACTOR) source = mt_Color; \n\
      if(iasource != 0){ \n\
        if(iasource == MT_DEFAULT) source.a = finalFrag.a; \n\
        else if(iasource == MTSRC_DIFFUSE) source.a = matdiff_color.a; \n\
        else if(iasource == MTSRC_SPECULAR) source.a = 1.0; \n\
        else if(iasource == MTSRC_FACTOR) source.a = mt_Color.a; \n\
      } \n\
      finalColCalc(source,fw_Texture_mode0[0],fw_Texture_mode0[1],fw_Texture_function0, fw_Texture_unit0,fw_TexCoord[0].st); \n\
      finalFrag = source; \n\
    } \n\
  } \n\
  if(textureCount>1){ \n\
    if(fw_Texture_mode1[0] != MTMODE_OFF) { \n\
      isource = fw_Texture_source1[0]; //castle-style dual sources \n\
      iasource = fw_Texture_source1[1]; \n\
      if(isource == MT_DEFAULT) source = finalFrag; \n\
      else if(isource == MTSRC_DIFFUSE) source = matdiff_color; \n\
      else if(isource == MTSRC_SPECULAR) source = vec4(castle_ColorES.rgb,1.0); \n\
      else if(isource == MTSRC_FACTOR) source = mt_Color; \n\
      if(iasource != 0){ \n\
        if(iasource == MT_DEFAULT) source.a = finalFrag.a; \n\
        else if(iasource == MTSRC_DIFFUSE) source.a = matdiff_color.a; \n\
        else if(iasource == MTSRC_SPECULAR) source.a = 1.0; \n\
        else if(iasource == MTSRC_FACTOR) source.a = mt_Color.a; \n\
      } \n\
      finalColCalc(source,fw_Texture_mode1[0],fw_Texture_mode1[1],fw_Texture_function1, fw_Texture_unit1,fw_TexCoord[1].st); \n\
      finalFrag = source; \n\
    } \n\
  } \n\
  if(textureCount>2){ \n\
    if(fw_Texture_mode2[0] != MTMODE_OFF) { \n\
      isource = fw_Texture_source2[0]; //castle-style dual sources \n\
      iasource = fw_Texture_source2[1]; \n\
      if(isource == MT_DEFAULT) source = finalFrag; \n\
      else if(isource == MTSRC_DIFFUSE) source = matdiff_color; \n\
      else if(isource == MTSRC_SPECULAR) source = vec4(castle_ColorES.rgb,1.0); \n\
      else if(isource == MTSRC_FACTOR) source = mt_Color; \n\
      if(iasource != 0){ \n\
        if(iasource == MT_DEFAULT) source.a = finalFrag.a; \n\
        else if(iasource == MTSRC_DIFFUSE) source.a = matdiff_color.a; \n\
        else if(iasource == MTSRC_SPECULAR) source.a = 1.0; \n\
        else if(iasource == MTSRC_FACTOR) source.a = mt_Color.a; \n\
      } \n\
      finalColCalc(source,fw_Texture_mode2[0],fw_Texture_mode2[1],fw_Texture_function2,fw_Texture_unit2,fw_TexCoord[2].st); \n\
      finalFrag = source; \n\
    } \n\
  } \n\
  if(textureCount>3){ \n\
    if(fw_Texture_mode3[0] != MTMODE_OFF) { \n\
      isource = fw_Texture_source3[0]; //castle-style dual sources \n\
      iasource = fw_Texture_source3[1]; \n\
      if(isource == MT_DEFAULT) source = finalFrag; \n\
      else if(isource == MTSRC_DIFFUSE) source = matdiff_color; \n\
      else if(isource == MTSRC_SPECULAR) source = vec4(castle_ColorES.rgb,1.0); \n\
      else if(isource == MTSRC_FACTOR) source = mt_Color; \n\
      if(iasource != 0){ \n\
        if(iasource == MT_DEFAULT) source.a = finalFrag.a; \n\
        else if(iasource == MTSRC_DIFFUSE) source.a = matdiff_color.a; \n\
        else if(iasource == MTSRC_SPECULAR) source.a = 1.0; \n\
        else if(iasource == MTSRC_FACTOR) source.a = mt_Color.a; \n\
      } \n\
      finalColCalc(source,fw_Texture_mode3[0],fw_Texture_mode3[1],fw_Texture_function3,fw_Texture_unit3,fw_TexCoord[3].st); \n\
      finalFrag = source; \n\
    } \n\
  } \n\
  #else //MTEX \n\
  /* ONE TEXTURE */ \n\
  #ifdef CUB \n\
  finalFrag = textureCube(fw_Texture_unit0, fw_TexCoord[0]) * finalFrag; \n\
  #else //CUB \n\
  finalFrag = texture2D(fw_Texture_unit0, fw_TexCoord[0].st) * finalFrag; \n\
  #endif //CUB \n\
  #endif //MTEX \n\
  \n\
}\n";


//add_light_contribution (castle_Color, castle_vertex_eye, castle_normal_eye, castle_MaterialShininess)
// http://www.web3d.org/documents/specifications/19775-1/V3.3/Part01/components/lighting.html#Lightingequations
// simplified thoery: lightOut = emissive + f(light_in,material,light_eqn)
// ADS: Ambient + Diffuse + Specular
// http://www.matrix44.net/cms/notes/opengl-3d-graphics/the-ads-lighting-model
// http://http.developer.nvidia.com/CgTutorial/cg_tutorial_chapter05.html
// incoming eyeposition and eyenormal are of the surface vertex and normal
// .. in the view/eye coordinate system (so eye is at 0,0,0 and eye direction is 0,0,-1

static const GLchar *plug_vertex_lighting_matemissive = "\n\
void PLUG_add_light_contribution (inout vec4 vertexcolor, in vec4 myPosition, in vec3 myNormal, in float shininess ) {\n\
	vertexcolor.rgb += fw_FrontMaterial.emissive.rgb; \n\
";

static const GLchar *plug_vertex_lighting_ADSLightModel = "\n\
/* use ADSLightModel here the ADS colour is returned from the function.  */ \n\
void PLUG_add_light_contribution2 (inout vec4 vertexcolor, inout vec3 specularcolor, in vec4 myPosition, in vec3 myNormal, in float shininess ) { \n\
  //working in eye space: eye is at 0,0,0 looking generally in direction 0,0,-1 \n\
  //myPosition, myNormal - of surface vertex, in eyespace \n\
  //vertexcolor - diffuse+ambient -will be replaced or modulated by texture color \n\
  //specularcolor - specular+emissive or non-diffuse (emissive added outside this function) \n\
  //algo: uses Blinn-Phong specular reflection: half-vector pow(N*H,shininess) \n\
  int i; \n\
  vec4 diffuse = vec4(0., 0., 0., 0.); \n\
  vec4 ambient = vec4(0., 0., 0., 0.); \n\
  vec4 specular = vec4(0., 0., 0., 1.); \n\
  vec3 N = normalize (myNormal); \n\
  \n\
  vec3 E = -normalize(myPosition.xyz); \n \
  vec4 matdiffuse = vec4(1.0,1.0,1.0,1.0); \n\
  float myAlph = 0.0;\n\
  \n\
  fw_MaterialParameters myMat = fw_FrontMaterial; \n\
  \n\
  /* back Facing materials - flip the normal and grab back materials */ \n\
  bool backFacing = (dot(N,E) < 0.0); \n\
  if (backFacing) { \n\
	N = -N; \n\
    #ifdef TWO \n\
	myMat = fw_BackMaterial; \n\
    #endif //TWO \n\
  } \n\
  \n\
  myAlph = myMat.diffuse.a; \n\
  //if(useMatDiffuse) \n\
  matdiffuse = myMat.diffuse; \n\
  \n\
  /* apply the lights to this material */ \n\
  /* weird but ANGLE needs constant loop */ \n\
  for (i=0; i<MAX_LIGHTS; i++) {\n\
    if(i < lightcount) { \n\
      vec4 myLightDiffuse = fw_LightSource[i].diffuse; \n\
      vec4 myLightAmbient = fw_LightSource[i].ambient; \n\
      vec4 myLightSpecular = fw_LightSource[i].specular; \n\
      vec4 myLightPosition = fw_LightSource[i].position; \n\
      int myLightType = lightType[i]; \n\
      vec3 myLightDir = fw_LightSource[i].spotDirection.xyz; \n\
      vec3  VP;     /* vector of light direction and distance */ \n\
      VP = myLightPosition.xyz - myPosition.xyz; \n\
      vec3 L = myLightDir; /*directional light*/ \n\
      if(myLightType < 2) /*point and spot*/ \n\
        L = normalize(VP); \n\
      float NdotL = max(dot(N, L), 0.0); //Lambertian diffuse term \n\
	  /*specular reflection models, phong or blinn-phong*/ \n\
	  //#define PHONG 1 \n\
	  #ifdef PHONG \n\
	  //Phong \n\
	  vec3 R = normalize(-reflect(L,N)); \n\
	  float RdotE = max(dot(R,E),0.0); \n\
	  float specbase = RdotE; \n\
	  float specpow = .3 * myMat.shininess; //assume shini tuned to blinn, adjust for phong \n\
	  #else //PHONG \n\
	  //Blinn-Phong \n\
      vec3 H = normalize(L + E); //halfvector\n\
      float NdotH = max(dot(N,H),0.0); \n\
	  float specbase = NdotH; \n\
	  float specpow = myMat.shininess; \n\
	  #endif //PHONG \n\
      float powerFactor = 0.0; /* for light dropoff */ \n\
      if (specbase > 0.0) { \n\
        powerFactor = pow(specbase,specpow); \n\
        /* tone down the power factor if myMat.shininess borders 0 */ \n\
        if (myMat.shininess < 1.0) { \n\
          powerFactor *= myMat.shininess; \n\
        } \n\
      } \n\
      \n\
      if (myLightType==1) { \n\
        /* SpotLight */ \n\
        float spotDot; \n\
        float spotAttenuation = 0.0; \n\
        float attenuation; /* computed attenuation factor */ \n\
        float D; /* distance to vertex */ \n\
        D = length(VP); \n\
        attenuation = 1.0/(fw_LightSource[i].Attenuations.x + (fw_LightSource[i].Attenuations.y * D) + (fw_LightSource[i].Attenuations.z *D*D)); \n\
        spotDot = dot (-L,myLightDir); \n\
        /* check against spotCosCutoff */ \n\
        if (spotDot > fw_LightSource[i].spotCutoff) { \n\
          spotAttenuation = pow(spotDot,fw_LightSource[i].spotExponent); \n\
        } \n\
        attenuation *= spotAttenuation; \n\
        /* diffuse light computation */ \n\
        diffuse += NdotL* matdiffuse*myLightDiffuse * attenuation; \n\
        /* ambient light computation */ \n\
        ambient += myMat.ambient*myLightAmbient; \n\
        /* specular light computation */ \n\
        specular += myLightSpecular * powerFactor * attenuation; \n\
        \n\
      } else if (myLightType == 2) { \n\
        /* DirectionalLight */ \n\
        /* Specular light computation */ \n\
        specular += myMat.specular *myLightSpecular*powerFactor; \n\
        /* diffuse light computation */ \n\
        diffuse += NdotL*matdiffuse*myLightDiffuse; \n\
        /* ambient light computation */ \n\
        ambient += myMat.ambient*myLightAmbient; \n\
      } else { \n\
        /* PointLight */ \n\
        float attenuation = 0.0; /* computed attenuation factor */ \n\
        float D = length(VP);  /* distance to vertex */ \n\
        /* are we within range? */ \n\
        if (D <= fw_LightSource[i].lightRadius) { \n\
          /* this is actually the SFVec3f attenuation field */ \n\
          attenuation = 1.0/(fw_LightSource[i].Attenuations.x + (fw_LightSource[i].Attenuations.y * D) + (fw_LightSource[i].Attenuations.z *D*D)); \n\
          /* diffuse light computation */ \n\
          diffuse += NdotL* matdiffuse*myLightDiffuse * attenuation; \n\
          /* ambient light computation */ \n\
          ambient += myMat.ambient*myLightAmbient; \n\
          /* specular light computation */ \n\
          attenuation *= (myMat.shininess/128.0); \n\
          specular += myLightSpecular * powerFactor * attenuation; \n\
        } \n\
      } \n\
    } \n\
  } \n\
  vertexcolor = clamp(vec4(vec3(ambient + diffuse ) + vertexcolor.rgb ,myAlph), 0.0, 1.0); \n\
  specularcolor = clamp(specular.rgb + specularcolor, 0.0, 1.0); \n\
} \n\
";

// http://www.web3d.org/documents/specifications/19775-1/V3.3/Part01/components/lighting.html#t-foginterpolant
// PLUG: fog_apply (fragment_color, normal_eye_fragment)
static const GLchar *plug_fog_apply =	"\
void PLUG_fog_apply (inout vec4 finalFrag, in vec3 normal_eye_fragment ){ \n\
  float ff = 1.0; \n\
  float depth = abs(castle_vertex_eye.z/castle_vertex_eye.w); \n\
  if(fw_fogparams.fogType > 0){ \n\
    ff = 0.0;  \n\
    if(fw_fogparams.fogType == 1){ //FOGTYPE_LINEAR \n\
      if(depth < fw_fogparams.visibilityRange) \n\
        ff = (fw_fogparams.visibilityRange-depth)/fw_fogparams.visibilityRange; \n\
    } else { //FOGTYPE_EXPONENTIAL \n\
        if(depth < fw_fogparams.visibilityRange){ \n\
          ff = exp(-depth/(fw_fogparams.visibilityRange -depth) ); \n\
          ff = clamp(ff, 0.0, 1.0);  \n\
        } \n\
	} \n\
    finalFrag = mix(finalFrag,fw_fogparams.fogColor,1.0 - ff);  \n\
  } \n\
} \n\
";

static const GLchar *vertex_plug_clip_apply =	"\
#ifdef CLIP \n\
#define FW_MAXCLIPPLANES 4 \n\
uniform int fw_nclipplanes; \n\
uniform vec4 fw_clipplanes[FW_MAXCLIPPLANES]; \n\
varying float fw_ClipDistance[FW_MAXCLIPPLANES]; \n\
 \n\
void PLUG_vertex_object_space (in vec4 vertex_object, in vec3 normal_object){ \n\
  for ( int i=0; i<fw_nclipplanes; i++ ) \n\
    fw_ClipDistance[i] = dot( fw_clipplanes[i], vertex_object); \n\
} \n\
#endif //CLIP \n\
";

static const GLchar *frag_plug_clip_apply =	"\
#ifdef CLIP \n\
#define FW_MAXCLIPPLANES 4 \n\
uniform int fw_nclipplanes; \n\
varying float fw_ClipDistance[FW_MAXCLIPPLANES]; \n\
void PLUG_fog_apply (inout vec4 finalFrag, in vec3 normal_eye_fragment ){ \n\
	for(int i=0;i<fw_nclipplanes;i++) \n\
      if(normal_eye_fragment.z > fw_ClipDistance[i]) discard;  \n\
} \n\
#endif //CLIP \n\
";

#if defined(GL_ES_VERSION_2_0)
static int isMobile = TRUE;
#else
static int isMobile = FALSE;
#endif

#define DESIRE(whichOne,zzz) ((whichOne & zzz)==zzz)
int getSpecificShaderSourceCastlePlugs (const GLchar **vertexSource, const GLchar **fragmentSource, shaderflagsstruct whichOne) 
{
	//for building the Builtin (similar to fixed-function pipeline, except from shader parts)
	//in OpenGL_Utils.c L.2553 set usingCastlePlugs = 1 to get in here.
	//whichone - a bitmask of shader requirements, one bit for each requirement, so shader permutation can be built

	int retval, unique_int;
	char *CompleteCode[3];
	char *vs, *fs;
	retval = FALSE;
	if(whichOne.usershaders ) //& USER_DEFINED_SHADER_MASK) 
		return retval; //not supported yet as of Aug 9, 2016
	retval = TRUE;

	//generic
	vs = strdup(getGenericVertex());
	fs = strdup(getGenericFragment());
		
	CompleteCode[SHADERPART_VERTEX] = vs;
	CompleteCode[SHADERPART_GEOMETRY] = NULL;
	CompleteCode[SHADERPART_FRAGMENT] = fs;

	// what we really have here: UberShader with CastlePlugs
	// UberShader: one giant shader peppered with #ifdefs, and you add #defines at the top for permutations
	// CastlePlugs: allows users to add effects on to uberShader with PLUGs
	// - and internally, we can do a few permutations with PLUGs too

	if(isMobile){
		AddVersion(SHADERPART_VERTEX, 100, CompleteCode); //lower precision floats
		AddVersion(SHADERPART_FRAGMENT, 100, CompleteCode); //lower precision floats
		AddDefine(SHADERPART_FRAGMENT,"MOBILE",CompleteCode); //lower precision floats
	}else{
		//desktop, emulating GLES2
		AddVersion(SHADERPART_VERTEX, 110, CompleteCode); //lower precision floats
		AddVersion(SHADERPART_FRAGMENT, 110, CompleteCode); //lower precision floats
	}

	unique_int = 0; //helps generate method name PLUG_xxx_<unique_int> to avoid clash when multiple PLUGs supplied for same PLUG point
	//Add in:
	//Lit
	//Fog
	//analglyph
	if(DESIRE(whichOne.base,WANT_ANAGLYPH))
		Plug(SHADERPART_FRAGMENT,plug_fragment_end_anaglyph,CompleteCode,&unique_int);  //works, converts frag to gray
	//color per vertex
	if DESIRE(whichOne.base,COLOUR_MATERIAL_SHADER) {
		AddDefine(SHADERPART_VERTEX,"CPV",CompleteCode);
		AddDefine(SHADERPART_FRAGMENT,"CPV",CompleteCode);
		if(DESIRE(whichOne.base,CPV_REPLACE_PRIOR)){
			AddDefine(SHADERPART_VERTEX,"CPVREP",CompleteCode);
			AddDefine(SHADERPART_FRAGMENT,"CPVREP",CompleteCode);
		}
	}
	//material appearance
	//2 material appearance
	//phong vs gourard
	if(DESIRE(whichOne.base,MATERIAL_APPEARANCE_SHADER) || DESIRE(whichOne.base,TWO_MATERIAL_APPEARANCE_SHADER)){
		//if(isLit)
		if(DESIRE(whichOne.base,MAT_FIRST)){
			//strict table 17-3 with no other modulation means Texture > CPV > mat.diffuse > (111)
			AddDefine(SHADERPART_VERTEX,"MATFIR",CompleteCode);
			AddDefine(SHADERPART_FRAGMENT,"MATFIR",CompleteCode);
		}
		if(DESIRE(whichOne.base,SHADINGSTYLE_PHONG)){
			//when we say phong in freewrl, we really mean per-fragment lighting
			AddDefine(SHADERPART_FRAGMENT,"LIT",CompleteCode);
			AddDefine(SHADERPART_FRAGMENT,"LITE",CompleteCode);  //add some lights
			Plug(SHADERPART_FRAGMENT,plug_vertex_lighting_ADSLightModel,CompleteCode,&unique_int); //use lights
			if(DESIRE(whichOne.base,TWO_MATERIAL_APPEARANCE_SHADER))
				AddDefine(SHADERPART_FRAGMENT,"TWO",CompleteCode);
			//but even if we mean per-fragment, for another dot product per fragment we can upgrade
			//from blinn-phong to phong and get the real phong reflection model 
			//(although dug9 can't tell the difference):
			AddDefine(SHADERPART_FRAGMENT,"PHONG",CompleteCode);
		}else{
			AddDefine(SHADERPART_VERTEX,"LIT",CompleteCode);
			AddDefine(SHADERPART_FRAGMENT,"LIT",CompleteCode);
			AddDefine(SHADERPART_VERTEX,"LITE",CompleteCode);  //add some lights
			Plug(SHADERPART_VERTEX,plug_vertex_lighting_ADSLightModel,CompleteCode,&unique_int); //use lights
			if(DESIRE(whichOne.base,TWO_MATERIAL_APPEARANCE_SHADER))
				AddDefine(SHADERPART_VERTEX,"TWO",CompleteCode);
		}
	}
	//lines and points 
	if( DESIRE(whichOne.base,HAVE_LINEPOINTS_APPEARANCE) ) {
		AddDefine(SHADERPART_VERTEX,"LIT",CompleteCode);
		AddDefine(SHADERPART_FRAGMENT,"LIT",CompleteCode);
		AddDefine(SHADERPART_VERTEX,"LINE",CompleteCode);
	}
	//textureCoordinategen
	//cubemap texure
	//one tex appearance
	//multi tex appearance
	//cubemap tex
	/*	http://www.web3d.org/documents/specifications/19775-1/V3.3/Part01/components/lighting.html#Lightingon
		"The Material's transparency field modulates the alpha in the texture. Hence, 
		a transparency of 0 will result in an alpha equal to that of the texture. 
		A transparency of 1 will result in an alpha of 0 regardless of the value in the texture."
		That doesn't seem to me to be what browsers Octaga, InstantReality, or Cortona are doing, 
		and its not what table 17-3 and the Lighting equation say is happening. 
		In the table, alpha is never 'modulated' ie there's never an alpha= AT * (1-TM) term.
		- freewrl version 3 and vivaty do modulate.
		I've put a define to set if you don't want modulation ie table 17-3.
		If you do want to modulate ie the above quote "to modulate", comment out the define
		I put a mantis issue to web3d.org for clarification Aug 16, 2016
	*/
	#define NOT_MODULATE_IMG_AND_MAT_ALPHAS 1  

	if (DESIRE(whichOne.base,ONE_TEX_APPEARANCE_SHADER) ||
		DESIRE(whichOne.base,HAVE_TEXTURECOORDINATEGENERATOR) ||
		DESIRE(whichOne.base,HAVE_CUBEMAP_TEXTURE) ||
		DESIRE(whichOne.base,MULTI_TEX_APPEARANCE_SHADER)) {
		AddDefine(SHADERPART_VERTEX,"TEX",CompleteCode);
		AddDefine(SHADERPART_FRAGMENT,"TEX",CompleteCode);
		if(DESIRE(whichOne.base,HAVE_TEXTURECOORDINATEGENERATOR) )
			AddDefine(SHADERPART_VERTEX,"TGEN",CompleteCode);
		if(DESIRE(whichOne.base,TEX3D_SHADER)){
			//in theory, if the texcoordgen "COORD" and TextureTransform3D are set in scenefile 
			// and working properly in freewrl, then don't need TEX3D for that node in VERTEX shader
			// which is using tex3dbbox (shape->_extent reworked) to get vertex coords in 0-1 range
			// x Sept 4, 2016 either TextureTransform3D or CoordinateGenerator "COORD" isn't working right for Texture3D
			// so we're using the bbox method
			//vertex str texture coords computed same for both volume and layered tex3d
			AddDefine(SHADERPART_VERTEX,"TEX3D",CompleteCode); 
			AddDefine(SHADERPART_FRAGMENT,"TEX3D",CompleteCode); 
			//fragment part different:
			if(DESIRE(whichOne.base,TEX3D_LAYER_SHADER)){
				//up to 6 textures, with lerp between floor,ceil textures
				AddDefine(SHADERPART_FRAGMENT,"TEX3DLAY",CompleteCode);
				Plug(SHADERPART_FRAGMENT,plug_fragment_texture3Dlayer_apply,CompleteCode,&unique_int);
			}else{
				//TEX3D_VOLUME_SHADER
				//AddDefine(SHADERPART_FRAGMENT,"TEX3D",CompleteCode);
				Plug(SHADERPART_FRAGMENT,plug_fragment_texture3D_apply,CompleteCode,&unique_int);
			}
		}else{
			if(DESIRE(whichOne.base,HAVE_CUBEMAP_TEXTURE)){
				AddDefine(SHADERPART_VERTEX,"CUB",CompleteCode);
				AddDefine(SHADERPART_FRAGMENT,"CUB",CompleteCode);
			} else if(DESIRE(whichOne.base,MULTI_TEX_APPEARANCE_SHADER)){
				AddDefine(SHADERPART_VERTEX,"MTEX",CompleteCode);
				AddDefine(SHADERPART_FRAGMENT,"MTEX",CompleteCode);
			}
			if(DESIRE(whichOne.base,TEXTURE_REPLACE_PRIOR) )
				AddDefine(SHADERPART_FRAGMENT,"TEXREP",CompleteCode);
			if(DESIRE(whichOne.base,TEXALPHA_REPLACE_PRIOR))
				AddDefine(SHADERPART_VERTEX,"TAREP",CompleteCode);

			Plug(SHADERPART_FRAGMENT,plug_fragment_texture_apply,CompleteCode,&unique_int);

			//if(texture has alpha ie channels == 2 or 4) then vertex diffuse = 111 and fragment diffuse*=texture
			//H: we currently assume image alpha, and maybe fill the alpha channel with (1-material.transparency)?
			//AddDefine(SHADERPART_VERTEX,"TAT",CompleteCode);
			//AddDefine(SHADERPART_FRAGMENT,"TAT",CompleteCode);
		}
	}

	//fill properties / hatching
	if(DESIRE(whichOne.base,FILL_PROPERTIES_SHADER)) {
		AddDefine(SHADERPART_VERTEX,"FILL",CompleteCode);		
		AddDefine(SHADERPART_FRAGMENT,"FILL",CompleteCode);		
	}
	//FOG
	if(DESIRE(whichOne.base,FOG_APPEARANCE_SHADER)){
		AddDefine(SHADERPART_VERTEX,"FOG",CompleteCode);		
		AddDefine(SHADERPART_FRAGMENT,"FOG",CompleteCode);	
		if(DESIRE(whichOne.base,HAVE_FOG_COORDS))
			AddDefine(SHADERPART_VERTEX,"FOGCOORD",CompleteCode);
		Plug(SHADERPART_FRAGMENT,plug_fog_apply,CompleteCode,&unique_int);	
	}
	//CLIPPLANE
	//FOG
	if(DESIRE(whichOne.base,CLIPPLANE_SHADER)){
		AddDefine(SHADERPART_VERTEX,"CLIP",CompleteCode);	
		Plug(SHADERPART_VERTEX,vertex_plug_clip_apply,CompleteCode,&unique_int);	
		AddDefine(SHADERPART_FRAGMENT,"CLIP",CompleteCode);	
		Plug(SHADERPART_FRAGMENT,frag_plug_clip_apply,CompleteCode,&unique_int);	
	}

	//EFFECTS - castle game engine effect nodes X3D_Effect with plugs applied here
	EnableEffects(CompleteCode,&unique_int);

	// stripUnusedDefines(CompleteCode);
    // http://freecode.com/projects/unifdef/  example: unifdef -UTEX -UGMTEX shader.vs > out.vs will strip the TEX and MTEX sections out


	*fragmentSource = CompleteCode[SHADERPART_FRAGMENT]; //original_fragment; //fs;
	*vertexSource = CompleteCode[SHADERPART_VERTEX]; //original_vertex; //vs;
	return retval;
}


/* Generic GLSL vertex shader, used on OpenGL ES. */
static const GLchar *volumeVertexGLES2 = " \n\
uniform mat4 fw_ModelViewMatrix; \n\
uniform mat4 fw_ProjectionMatrix; \n\
uniform float fw_FocalLength; \n\
uniform vec4 fw_viewport; \n\
attribute vec4 fw_Vertex; \n\
 \n\
/* PLUG-DECLARATIONS */ \n\
 \n\
varying vec4 castle_vertex_eye; \n\
varying vec4 castle_Color; \n\
varying vec3 vertex_model; \n\
 \n\
uniform vec4 fw_UnlitColor; \n\
 \n\
void main(void) \n\
{ \n\
  vec4 vertex_object = fw_Vertex; \n\
   \n\
  castle_vertex_eye = fw_ModelViewMatrix * vertex_object; \n\
   \n\
   castle_Color = vec4(1.0,.5,.5,1.0); \n\
  \n\
  gl_Position = fw_ProjectionMatrix * castle_vertex_eye; \n\
  //#ifdef XYZ \n\
  castle_Color.rgb = gl_Position.xyz; \n\
  vertex_model = fw_Vertex.xyz; \n\
  //#endif \n\
   \n\
} \n\
";
/* Generic GLSL fragment shader, used on OpenGL ES. */
static const GLchar *volumeFragmentGLES2 = " \n\
/* DEFINES */ \n\
#ifdef MOBILE \n\
//precision highp float; \n\
precision mediump float; \n\
#endif //MOBILE \n\
 \n\
varying vec3 vertex_model; \n\
varying vec4 castle_vertex_eye; \n\
varying vec4 castle_Color; \n\
uniform mat4 fw_ModelViewMatrix; \n\
uniform mat4 fw_ProjectionMatrix; \n\
uniform mat4 fw_ModelViewProjInverse; \n\
uniform float fw_FocalLength; \n\
uniform vec4 fw_viewport; \n\
uniform vec3 fw_dimensions; \n\
uniform vec3 fw_RayOrigin; \n\
uniform sampler2D fw_Texture_unit0; \n\
#ifdef TEX3D \n\
uniform int tex3dDepth; \n\
uniform int repeatSTR[3]; \n\
uniform int magFilter; \n\
#endif //TEX3D \n\
 \n\
struct Ray { \n\
  vec3 Origin; \n\
  vec3 Dir; \n\
}; \n\
struct AABB { \n\
  vec3 Min; \n\
  vec3 Max; \n\
}; \n\
bool IntersectBox(Ray r, AABB aabb, out float t0, out float t1) \n\
{ \n\
    vec3 invR = 1.0 / r.Dir; \n\
    vec3 tbot = invR * (aabb.Min-r.Origin); \n\
    vec3 ttop = invR * (aabb.Max-r.Origin); \n\
    vec3 tmin = min(ttop, tbot); \n\
    vec3 tmax = max(ttop, tbot); \n\
    vec2 t = max(tmin.xx, tmin.yz); \n\
    t0 = max(t.x, t.y); \n\
    t = min(tmax.xx, tmax.yz); \n\
    t1 = min(t.x, t.y); \n\
    return t0 <= t1; \n\
} \n\
/* PLUG-DECLARATIONS */ \n\
 \n\
vec3 fw_TexCoord[1]; \n\
void main(void) \n\
{ \n\
	float maxDist = 1.414214; //sqrt(2.0); \n\
	float densityFactor = 5.0; \n\
	float Absorption = 1.0; \n\
	int numSamples = 128; \n\
	float fnumSamples = float(numSamples); \n\
	float stepSize = maxDist/fnumSamples; \n\
	 \n\
    vec4 fragment_color; \n\
	vec4 fragment_color_main = castle_Color; \n\
    vec3 rayDirection; \n\
	//convert window to frustum \n\
    rayDirection.xy = 2.0 * (gl_FragCoord.xy - fw_viewport.xy) / fw_viewport.zw - vec2(1.0); \n\
	if(true){ \n\
		//the equivalent of gluUnproject \n\
		vec4 ray4 = vec4(rayDirection,1.0); \n\
		// out = modelviewProjectionInverse x in \n\
		ray4 = fw_ModelViewProjInverse * ray4; \n\
		//ray4 = ray4 * fw_ModelViewProjInverse; //transpose \n\
		// out = out/out.w; \n\
		ray4 /= ray4.w; \n\
		rayDirection.xyz = -ray4.xyz; \n\
	}else{ \n\
		rayDirection.z = -fw_FocalLength; \n\
		rayDirection = (vec4(rayDirection, 0) * fw_ModelViewMatrix).xyz; \n\
	} \n\
	\n\
    Ray eye = Ray( fw_RayOrigin, normalize(rayDirection) ); \n\
    //AABB aabb = AABB(vec3(-1.0), vec3(+1.0)); \n\
	//AABB aabb = AABB(vec3(fw_dimensions*-.5),vec3(fw_dimensions*.5)); \n\
	vec3 half_dimensions = fw_dimensions * .5; \n\
	vec3 minus_half_dimensions = half_dimensions * -1.0; \n\
	AABB aabb = AABB(minus_half_dimensions,half_dimensions); \n\
	\n\
    float tnear, tfar; \n\
    IntersectBox(eye, aabb, tnear, tfar); \n\
    if (tnear < 0.0) tnear = 0.0; \n\
	\n\
    vec3 rayStart = eye.Origin + eye.Dir * tnear; \n\
    vec3 rayStop = eye.Origin + eye.Dir * tfar; \n\
    // Transform from object space to texture coordinate space: \n\
    //rayStart = 0.5 * (rayStart + 1.0); \n\
    //rayStop = 0.5 * (rayStop + 1.0); \n\
	\n\
    // Perform the ray marching: \n\
    vec3 pos = rayStart; \n\
    vec3 step = normalize(rayStop-rayStart) * stepSize; \n\
    float travel = distance(rayStop, rayStart); \n\
    float T = 1.0; \n\
    vec3 Lo = vec3(0.0); \n\
	vec3 normal_eye_fragment = vec3(0.0); //not used in plug \n\
	fragment_color.a = 1.0; \n\
	if(travel <= 0.0) fragment_color.rgb = vec3(.5,.5,.5); \n\
	if(numSamples <= 0) fragment_color.rgb = vec3(.1,.5,.1); \n\
	//numSamples = 0; \n\
	vec3 pos2 = pos; \n\
    // Transform from object space to texture coordinate space: \n\
	pos2 = (pos2+half_dimensions)/fw_dimensions; \n\
	fw_TexCoord[0] = pos2; //vertex_model; //vec3(.2,.2,.5); \n\
	fragment_color = vec4(.8,.8,.8,1.0); \n\
	//fragment_color = texture2D(fw_Texture_unit0,fw_TexCoord[0].st); \n\
	/* P_LUG: texture_apply (fragment_color, normal_eye_fragment) */ \n\
	fragment_color_main = fragment_color; \n\
	//fragment_color_main.a = 1.0; \n\
	\n\
    for (int i=0; i < numSamples; ++i) { \n\
       // ...lighting and absorption stuff here... \n\
		fragment_color = vec4(1.0); \n\
		pos2 = pos; \n\
	    // Transform from object space to texture coordinate space: \n\
		pos2 = (pos2+half_dimensions)/fw_dimensions; \n\
		fw_TexCoord[0] = pos2; \n\
		/* PLUG: texture_apply (fragment_color, normal_eye_fragment) */ \n\
        //float density = texture3D(Density, pos).x * densityFactor; \n\
		float density = fragment_color.a * densityFactor; \n\
		\n\
        if (density <= 0.0) \n\
            continue; \n\
		\n\
        T *= 1.0-density*stepSize*Absorption; \n\
		fragment_color_main.a = 1.0 - T; \n\
		//fragment_color_main.rgb = fragment_color.rgb; \n\
        if (T <= 0.01) { \n\
            break; \n\
		} \n\
		travel -= stepSize; \n\
		if(travel <= 0.0) break; \n\
		pos += step; \n\
		\n\
    }  \n\
	gl_FragColor = fragment_color_main; \n\
} \n\
";


static const GLchar *plug_fragment_texture3D_apply_volume =	"\
void PLUG_texture_apply (inout vec4 finalFrag, in vec3 normal_eye_fragment ){ \n\
\n\
  #ifdef TEX3D \n\
  vec3 texcoord = fw_TexCoord[0]; \n\
  texcoord.z = 1.0 - texcoord.z; //flip z from RHS to LHS\n\
  float depth = max(1.0,float(tex3dDepth)); \n\
  if(repeatSTR[0] == 0) texcoord.x = clamp(texcoord.x,0.0001,.9999); \n\
  else texcoord.x = mod(texcoord.x,1.0); \n\
  if(repeatSTR[1] == 0) texcoord.y = clamp(texcoord.y,0.0001,.9999); \n\
  else texcoord.y = mod(texcoord.y,1.0); \n\
  if(repeatSTR[2] == 0) texcoord.z = clamp(texcoord.z,0.0001,.9999); \n\
  else texcoord.z = mod(texcoord.z,1.0); \n\
  vec4 texel; \n\
  int flay = int(floor(texcoord.z*depth)); \n\
  int clay = int(ceil(texcoord.z*depth)); \n\
  clay = clay == tex3dDepth ? 0 : clay; \n\
  vec4 ftexel, ctexel; \n\
  vec3 ftexcoord, ctexcoord; \n\
  ftexcoord = texcoord; \n\
  ctexcoord = texcoord; \n\
  ftexcoord.y += float(flay); \n\
  ftexcoord.y /= depth; \n\
  ctexcoord.y += float(clay); \n\
  ctexcoord.y /= depth; \n\
  ftexel = texture2D(fw_Texture_unit0,ftexcoord.st); \n\
  ctexel = texture2D(fw_Texture_unit0,ctexcoord.st); \n\
  float fraction = mod(ftexcoord.z*depth,1.0); \n\
  if(magFilter == 1) \n\
	texel = mix(ctexel,ftexel,1.0-fraction); //lerp GL_LINEAR \n\
  else \n\
	texel = ftexel; //fraction > .5 ? ctexel : ftexel; //GL_NEAREST \n\
  finalFrag *= texel; \n\
  #endif //TEX3D \n\
  \n\
}\n";

const char *getVolumeVertex(void){
	return volumeVertexGLES2; //genericVertexDesktop
}
const char *getVolumeFragment(){
	return volumeFragmentGLES2; //genericFragmentDesktop;
}
int getSpecificShaderSourceVolume (const GLchar **vertexSource, const GLchar **fragmentSource, shaderflagsstruct whichOne) 
{
	//for building the Builtin (similar to fixed-function pipeline, except from shader parts)
	//in OpenGL_Utils.c L.2553 set usingCastlePlugs = 1 to get in here.
	//whichone - a bitmask of shader requirements, one bit for each requirement, so shader permutation can be built

	int retval, unique_int;
	char *CompleteCode[3];
	char *vs, *fs;
	retval = FALSE;
	if(whichOne.usershaders ) //& USER_DEFINED_SHADER_MASK) 
		return retval; //not supported yet as of Aug 9, 2016
	retval = TRUE;

	//generic
	vs = strdup(getVolumeVertex());
	fs = strdup(getVolumeFragment());
		
	CompleteCode[SHADERPART_VERTEX] = vs;
	CompleteCode[SHADERPART_GEOMETRY] = NULL;
	CompleteCode[SHADERPART_FRAGMENT] = fs;

	// what we really have here: UberShader with CastlePlugs
	// UberShader: one giant shader peppered with #ifdefs, and you add #defines at the top for permutations
	// CastlePlugs: allows users to add effects on to uberShader with PLUGs
	// - and internally, we can do a few permutations with PLUGs too

	if(isMobile){
		AddVersion(SHADERPART_VERTEX, 100, CompleteCode); //lower precision floats
		AddVersion(SHADERPART_FRAGMENT, 100, CompleteCode); //lower precision floats
		AddDefine(SHADERPART_FRAGMENT,"MOBILE",CompleteCode); //lower precision floats
	}else{
		//desktop, emulating GLES2
		AddVersion(SHADERPART_VERTEX, 110, CompleteCode); //lower precision floats
		AddVersion(SHADERPART_FRAGMENT, 110, CompleteCode); //lower precision floats
	}

	unique_int = 0; //helps generate method name PLUG_xxx_<unique_int> to avoid clash when multiple PLUGs supplied for same PLUG 

	if(DESIRE(whichOne.volume,SHADERFLAGS_VOLUME_XYZ)){
		AddDefine(SHADERPART_VERTEX,"XYZ",CompleteCode);
	}
	if(DESIRE(whichOne.volume,TEX3D_SHADER)){
		AddDefine(SHADERPART_FRAGMENT,"TEX3D",CompleteCode); 
		Plug(SHADERPART_FRAGMENT,plug_fragment_texture3D_apply,CompleteCode,&unique_int);
	}


	*fragmentSource = CompleteCode[SHADERPART_FRAGMENT]; //original_fragment; //fs;
	*vertexSource = CompleteCode[SHADERPART_VERTEX]; //original_vertex; //vs;
	return retval;

}