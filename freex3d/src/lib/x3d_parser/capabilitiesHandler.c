/*
  $Id$

  FreeWRL support library.
  X3D capabilities.

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

#include <io_files.h>

#include "../vrml_parser/Structs.h"
#include "../main/headers.h"


/* table showing which levels are supported by which component */
static const int capabilities[] = {
	COM_Geometry2D,	2, 		/* May 12, 2009 */
	COM_Rendering,	4, 		/* May 12, 2009 */
	COM_PickingSensor,	0, 	/* May 12, 2009 */
	COM_DIS,	0, 		/* May 12, 2009 */
	COM_EnvironmentalSensor,	3, /* May 12, 2009 */
	COM_Text,	1, 		/* May 12, 2009 */
	COM_NURBS,	0, 		/* May 12, 2009 */
	COM_CubeMapTexturing,	1, 	/* May 12, 2009 */
	COM_EventUtilities,	1, 	/* May 12, 2009 */
	COM_Interpolation,	3, 	/* May 12, 2009 */
	COM_Shaders,	1, 		/* May 12, 2009 */
	COM_Navigation,	3, 		/* July 29 2010 */
	COM_Grouping,		3,	/* October 29, 2008 */
	COM_Texturing,	3, 		/* May 12, 2009 */
	COM_Geospatial,	2, 		/* May 12, 2009 */
	COM_CADGeometry,	2, 	/* July 10 2013 */
	COM_EnvironmentalEffects,	3, /* May 12, 2009 */
	COM_Shape,	4, 		/* May 12, 2009 */
	COM_Texturing3D,	0, 	/* May 12, 2009 */
	COM_PointDeviceSensor,	1, 	/* May 12, 2009 */
	COM_HAnim,	0, 		/* May 12, 2009 */
	COM_RigidBodyPhysics,	0, 	/* May 12, 2009 */
	COM_Core,		2,	/* October 29, 2008 */
	COM_Layout,	0, 		/* May 12, 2009 */
	COM_Time,		2, 	/* October 29, 2008 */
	COM_Geometry3D,	4, 		/* May 12, 2009 */
	COM_Followers,	0, 		/* May 12, 2009 */
	COM_Scripting,	1, 		/* May 12, 2009 */
	COM_Lighting,	3, 		/* May 12, 2009 */
	COM_KeyDeviceSensor,	2, 	/* May 12, 2009 */
	COM_Layering,	0, 		/* May 12, 2009 */
	COM_Networking,	3, 		/* May 12, 2009 */
	COM_ParticleSystems,	0, 	/* May 12, 2009 */
	COM_Sound,	1, 		/* May 12, 2009 */
	INT_ID_UNDEFINED, 	INT_ID_UNDEFINED,
};

/* profiles... */

/* ISO-IEC-FDISINT_ID_UNDEFINED9775:1.2 H3 Component support */
static const int CADInterchangeProfile[] = {
	COM_Core,		1,
	COM_Networking,		1,
	COM_Grouping,		1,
	COM_Rendering,		4,
	COM_Shape,		2,
	COM_Lighting,		1,
	COM_Texturing,		2,
	COM_Navigation,		2,
	COM_Shaders,		1,
	COM_CADGeometry,	2,
	INT_ID_UNDEFINED, 		INT_ID_UNDEFINED};


/* ISO-IEC-FDISINT_ID_UNDEFINED9775:1.2 A3 Component support */
static const int CoreProfile[] = {
	COM_Core,		1,
	INT_ID_UNDEFINED, 		INT_ID_UNDEFINED};


/* ISO-IEC-FDISINT_ID_UNDEFINED9775:1.2 F3 Component support */
static const int FullProfile[] = {
	COM_Core,			2,
	COM_Time,			2,
	COM_Networking,			3,
	COM_Grouping,			3,
	COM_Rendering,			5,
	COM_Shape,			4,
	COM_Geometry3D,			4,
	COM_Geometry2D,			2,
	COM_Text,			1,
	COM_Sound,			1,
	COM_Lighting,			3,
	COM_Texturing,			3,
	COM_Interpolation,		5,
	COM_Navigation,			3,
	COM_PointDeviceSensor,		1,
	COM_KeyDeviceSensor,		2,
	COM_EnvironmentalSensor,	3,
	COM_EnvironmentalEffects,	4,
	COM_Geospatial,			2,
	COM_HAnim,			1,
	COM_NURBS,			4,
	COM_DIS,			2,
	COM_Scripting,			1,
	COM_EventUtilities,		1,
	COM_Shaders,			1,
	COM_CADGeometry,		2,
	COM_Texturing3D,		2,
	COM_CubeMapTexturing,		3,
	COM_Layering,			1,
	COM_Layout,			2,
	COM_RigidBodyPhysics,		2,
	COM_PickingSensor,		3,
	COM_Followers,			1,
	COM_ParticleSystems,		3,
	INT_ID_UNDEFINED, 			INT_ID_UNDEFINED};


/* ISO-IEC-FDISINT_ID_UNDEFINED9775:1.2 E3 Component support */
static const int ImmersiveProfile[] = {
	COM_Core,			2,
	COM_Time,			1,
	COM_Networking,			3,
	COM_Grouping,			2,
	COM_Rendering,			3,
	COM_Shape,			2,
	COM_Geometry3D,			4,
	COM_Geometry2D,			1,
	COM_Text,			1,
	COM_Sound,			1,
	COM_Lighting,			2,
	COM_Texturing,			3,
	COM_Interpolation,		2,
	COM_PointDeviceSensor,		1,
	COM_KeyDeviceSensor,		2,
	COM_EnvironmentalSensor,	2,
	COM_EnvironmentalEffects,	2,
	COM_Scripting,			1,
	COM_EventUtilities,		1,
	INT_ID_UNDEFINED, 			INT_ID_UNDEFINED};


/* ISO-IEC-FDISINT_ID_UNDEFINED9775:1.2 C3 Component support */
static const int InteractiveProfile[] = {
	COM_Core,			1,
	COM_Time,			1,
	COM_Networking,			2,
	COM_Grouping,			2,
	COM_Rendering,			3,
	COM_Shape,			1,
	COM_Geometry3D,			3,
	COM_Lighting,			2,
	COM_Texturing,			2,
	COM_Interpolation,		2,
	COM_Navigation,			1,
	COM_PointDeviceSensor,		1,
	COM_KeyDeviceSensor,		1,
	COM_EnvironmentalSensor,	1,
	COM_EnvironmentalEffects,	1,
	COM_EventUtilities,		1,
	INT_ID_UNDEFINED, 			INT_ID_UNDEFINED};


/* ISO-IEC-FDISINT_ID_UNDEFINED9775:1.2 B3 Component support */
static const int InterchangeProfile[] = {
	COM_Core,			1,
	COM_Time,			1,
	COM_Networking,			1,
	COM_Grouping,			1,
	COM_Rendering,			3,
	COM_Shape,			1,
	COM_Geometry3D,			2,
	COM_Lighting,			1,
	COM_Texturing,			2,
	COM_Interpolation,		2,
	COM_Navigation,			1,
	COM_EnvironmentalEffects,	1,
	INT_ID_UNDEFINED, 			INT_ID_UNDEFINED};


/* ISO-IEC-FDISINT_ID_UNDEFINED9775:1.2 D3 Component support */
static const int MPEG4Profile[] = {
	COM_Core,			1,
	COM_Time,			1,
	COM_Networking,			2,
	COM_Grouping,			2,
	COM_Rendering,			1,
	COM_Shape,			1,
	COM_Geometry3D,			2,
	COM_Lighting,			2,
	COM_Texturing,			1,
	COM_Interpolation,		2,
	COM_Navigation,			1,
	COM_PointDeviceSensor,		1,
	COM_EnvironmentalSensor,	1,
	COM_Navigation,			1,
	COM_EnvironmentalEffects,	1,
	INT_ID_UNDEFINED, 			INT_ID_UNDEFINED};


//dug9 Aug,2013 ecmascript interface V3 says a ProfileInfo has a name, level??, Title, providerUrl, componentInfoArray
//
//Q. how do you assign a Level to a Profile?
//H0: 1 if you have it, else 0
//H1: minimum of component levels in profile
//H2: maximum of component levels in profile
//H3: maximum nesting level - see diagram ie if you have Geospatial,Hanim,Nurbs you are at Full or Level=4
//http://www.web3d.org/realtime-3d/x3d/profiles   
//I'll just make up and hardcode some values now

struct proftablestruct {
	int profileName;
	const int *profileTable;
	int level; //dug9
};

static struct proftablestruct profTable[] = {
	{PRO_Interchange,		InterchangeProfile,		1},
	{PRO_CADInterchange, 	CADInterchangeProfile,	1},
	{PRO_MPEG4,				MPEG4Profile,			1},
	{PRO_Interactive, 		InteractiveProfile,		1},
	{PRO_Full,				FullProfile,			1},
	{PRO_Immersive,			ImmersiveProfile,		1},
	{PRO_Core,				CoreProfile,			1},
	{INT_ID_UNDEFINED, 		(const int*) INT_ID_UNDEFINED, INT_ID_UNDEFINED}
};


void handleVersion(const char *versionString) {
	int xa=0;
	int xb=0;
	int xc=0;
	int rt;
	
	/* printf ("handleVersion - x3d version :%s:\n", versionString); */
	rt = sscanf (versionString,"%d.%d.%d",&xa, &xb,&xc);
	/* printf ("rt %d xa %d xb %d xc %d\n",rt,xa,xb,xc); */

	/* we could (should?) do some more checking here, but for now... */
	inputFileVersion[0] = xa, inputFileVersion[1] = xb; inputFileVersion[2] = xc;
}



void handleMetaDataStringString(struct Uni_String *val1, struct Uni_String *val2) {
	#ifdef CAPABILITIESVERBOSE
	printf ("handleMetaDataStringString, :%s:, :%s:\n",val1->strptr, val2->strptr);
	#endif
}

void handleProfile (int myProfile) {
	int *myTable = NULL;
	int i;
	/* myProfile is a valid profile number - bounds checked before entry */
	#ifdef CAPABILITIESVERBOSE
	printf ("handleProfile, my profile is %s (%d)\n",stringProfileType(myProfile), myProfile);
	#endif

	i=0;
	while ((profTable[i].profileName != INT_ID_UNDEFINED) && (profTable[i].profileName != myProfile)) i++;

	/* we really should have found this, unless we have a new profile that is not coded properly here */
	if (profTable[i].profileName == INT_ID_UNDEFINED) {
		ConsoleMessage ("Something wrong in handleProfile for profile %s\n",
			stringProfileType(myProfile));
	} else {
		int comp; 
		int lev;
		gglobal()->Mainloop.scene_profile = i;
		myTable = (int *)profTable[i].profileTable;
		/* go through the selected table, and see if each component is within range */
		comp = *myTable; myTable++; lev = *myTable; myTable++;
		while (comp != INT_ID_UNDEFINED) {
			handleComponent(comp,lev);
			comp = *myTable; myTable++; lev = *myTable; myTable++;
		}
	}
}
//>> exported to jsVRMLBrowser.c
int capabilitiesHandler_getComponentLevel(int *table, int comp)
{
	return table[(comp*2) +1];
}
int capabilitiesHandler_getProfileLevel(int prof)
{
	return profTable[prof].level;
}
const int *capabilitiesHandler_getProfileComponent(int prof)
{
	return profTable[prof].profileTable;
}
const int *capabilitiesHandler_getCapabilitiesTable()
{
	return capabilities;
}
int capabilitiesHandler_getTableLength(int* table){
	int len = 0;
	if(table == NULL) return 0;
	while(table[2*len] != INT_ID_UNDEFINED)
		len++;
	return len;
}

void scene_addComponent(int myComponent, int mylevel){
	//besides the static tables for freewrl, we need a table for scene (desired) components
	//generated during parsing
	//we need an init to clear this
	//and we need a Scene or ProtoInstance (ie broto) struct to store this in
	int *scene_comps = gglobal()->Mainloop.scene_components;
	int len = capabilitiesHandler_getTableLength(scene_comps);
	scene_comps = realloc(scene_comps,sizeof(int)*2*(len+2));
	scene_comps[len*2] = myComponent;
	scene_comps[len*2 +1] = mylevel;
	len++;
	scene_comps[len*2] = INT_ID_UNDEFINED;
	scene_comps[len*2 +1] = INT_ID_UNDEFINED;
	gglobal()->Mainloop.scene_components = scene_comps;
}
void scene_clearComponents(){
	FREE_IF_NZ(gglobal()->Mainloop.scene_components);
}

//<< 

void handleComponent (int myComponent, int myLevel) {
	int i;

	/* myComponent is a valid component number - bounds checked before entry */
	#ifdef CAPABILITIESVERBOSE
	printf ("handleComponent: my Component is %s, level %d\n",COMPONENTS[myComponent], myLevel);
	#endif

	i=0;
	while ((capabilities[i] != myComponent) && (capabilities[i] != INT_ID_UNDEFINED)) {
		i+=2; 
	}

	/* did we find the component? */
	if (capabilities[i] == myComponent) {
		scene_addComponent(myComponent,myLevel);

		#ifdef CAPABILITIESVERBOSE
		printf ("handleComponent, comparing requested level %d with supported level %d\n",myLevel, capabilities[i+1]);
		#endif

		if (myLevel > capabilities[i+1]) {
			ConsoleMessage ("Component %s support level %d, requested %d",
				COMPONENTS[myComponent], capabilities[i+1], myLevel);
		}
	} else {
		ConsoleMessage ("did not find component %s in capabilities table!",COMPONENTS[myComponent]);
	}
}

void handleExport (char *node, char *as) {
	/* handle export statements. as will be either a string pointer, or NULL */
	
	#ifdef CAPABILITIESVERBOSE
	printf ("handleExport: node :%s: ",node);
	if (as != NULL) printf (" AS :%s: ",node);
	printf ("\n");
	#endif
}

void handleImport (char *nodeName,char *nodeImport, char *as) {
	/* handle Import statements. as will be either a string pointer, or NULL */
	
	#ifdef CAPABILITIESVERBOSE
	printf ("handleImport: inlineNodeName :%s: nodeToImport :%s:",nodeName, nodeImport);
	if (as != NULL) printf (" AS :%s: ",as);
	printf ("\n");
	#endif
}
