/*


CProto ???

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
#include "../opengl/OpenGL_Utils.h"
#include "../main/headers.h"

#include "LinearAlgebra.h"
#include "quaternion.h"
#include "Viewer.h"
#include "../x3d_parser/Bindable.h"
#include "ui/common.h" // for ppcommon

//moved to libfreewrl.h
//enum {
//	CHORD_YAWZ,
//	CHORD_YAWPITCH,
//	CHORD_ROLL,
//	CHORD_XY
//} input_chords;

static void init_stereodefaults(X3D_Viewer *Viewer)
{
		/* must call this before getting values from command line in options.c */
	Viewer->shutterGlasses = 0;
	Viewer->anaglyph = 0;
	Viewer->sidebyside = 0;
	Viewer->updown = 0;
	Viewer->isStereo = 0;
		Viewer->eyedist = 0.065;
		//For sidebyside: average human eyebase 2.4inches/65mm. 
		// We want it narrower, 57mm or 2.25 inches 
		// for 6" wide viewport: 2.25/6 = .375
		// for shutter and anaglyph, .5 gets both eyes looking at the 
		// same point on the screen (for nominal distance object)
		Viewer->screendist = 0.375; //was .8 
		Viewer->stereoParameter = 0.01; //was .4 or toe-in. Toe-in can force your eyes wall-eyed esp. in side-by-side, so set near zero.
		Viewer->dominantEye = 1; /*0=Left 1=Right used for picking*/
		Viewer->eitherDominantEye = 1; //1=can pick with either eye depending on which stereoside mouse is in, see setup_pickside()
		Viewer->iprog[0] = 0; /* left red */
		Viewer->iprog[1] = 1; /* right green */
		Viewer->haveQuadbuffer = 0;
}


typedef struct pViewer{
	int examineCounter;// = 5;

	int viewer_initialized;
	X3D_Viewer_Walk viewer_walk;
	X3D_Viewer_Examine viewer_examine;
	X3D_Viewer_Fly viewer_fly;
	X3D_Viewer_Spherical viewer_ypz;

	FILE *exfly_in_file;
	struct point_XYZ viewer_lastP;
	int exflyMethod; //0 or 1;  /* could be a user settable option, which kind of exfly to do */
	int StereoInitializedOnce;//. = 0;
	GLboolean acMask[3][3]; //anaglyphChannelMask
	//X3D_Viewer Viewer; /* moved to Bindables.h > bindablestacks */
	/* viewpoint slerping */
	double viewpoint2rootnode[16];
	double viewpointnew2rootnode[16];
	int vp2rnSaved;
	double old2new[16];
	double identity[16];
	double tickFrac;
	Quaternion sq;
	double sp[3];
	int keychord;
	int dragchord;

}* ppViewer;
void *Viewer_constructor(){
	void *v = MALLOCV(sizeof(struct pViewer));
	memset(v,0,sizeof(struct pViewer));
	return v;
}
void Viewer_init(struct tViewer *t){
	//public
	//private
	t->prv = Viewer_constructor();
	{
		ppViewer p = (ppViewer)t->prv;

		p->examineCounter = 5;

		p->viewer_initialized = FALSE;
		#ifdef _MSC_VER
		p->exflyMethod = 1;  /* could be a user settable option, which kind of exfly to do */
		#else
		p->exflyMethod = 0;
		#endif
		p->StereoInitializedOnce = 0;
		p->acMask[0][0] = (GLboolean)1; // R = 1, 0, 0

		p->acMask[1][1] = (GLboolean)1; // C = 0, 1, 1
		p->acMask[1][2] = (GLboolean)1;

		/* viewpoint slerping */
		loadIdentityMatrix(p->viewpoint2rootnode);
		p->vp2rnSaved = FALSE; //on startup it binds before saving
		loadIdentityMatrix(p->old2new);
		loadIdentityMatrix(p->identity);
		p->tickFrac = 0.0; //for debugging slowly
//init_stereodefaults(viewer);
		p->StereoInitializedOnce = 1;
		p->keychord = CHORD_XY; // default on startup
		p->dragchord = CHORD_YAWZ;
	}
}



//static void handle_tick_walk(void);
static void handle_tick_walk2(double dtime);
static void handle_tick_fly(void);
static void handle_tick_exfly(void);
static void handle_tick_fly2(double dtime);

/* used for EAI calls to get the current speed. Not used for general calcs */
/* we DO NOT return as a float, as some gccs have trouble with this causing segfaults */
void getCurrentSpeed() {
	X3D_Viewer *viewer;
	ppViewer p;
	ttglobal tg = gglobal();
	p =  (ppViewer)tg->Viewer.prv;
	viewer = Viewer();
	tg->Mainloop.BrowserSpeed = tg->Mainloop.BrowserFPS * (fabs(viewer->VPvelocity.x) + fabs(viewer->VPvelocity.y) + fabs(viewer->VPvelocity.z));
}
void fwl_set_viewer_type0(X3D_Viewer *viewer, const int type);
void viewer_default0(X3D_Viewer *viewer, int vpnodetype) {
	Quaternion q_i;
	ppViewer p = (ppViewer)gglobal()->Viewer.prv;

	viewer->fieldofview = 45.0;
	viewer->fovZoom = 1.0;

	viewer->VPvelocity.x = 0.0; viewer->VPvelocity.y = 0.0; viewer->VPvelocity.z = 0.0; 
	viewer->Pos.x = 0; viewer->Pos.y = 0; viewer->Pos.z = 10;
	viewer->currentPosInModel.x = 0; viewer->currentPosInModel.y = 0; viewer->currentPosInModel.z = 10;
	viewer->AntiPos.x = 0; viewer->AntiPos.y = 0; viewer->AntiPos.z = 0;

	vrmlrot_to_quaternion (&viewer->Quat,1.0,0.0,0.0,0.0);
	vrmlrot_to_quaternion (&viewer->bindTimeQuat,1.0,0.0,0.0,0.0);
	vrmlrot_to_quaternion (&viewer->prepVPQuat,0.0,1.0,0.0,3.14);
	vrmlrot_to_quaternion (&q_i,1.0,0.0,0.0,0.0);
	quaternion_inverse(&(viewer->AntiQuat),&q_i);

	viewer->headlight = TRUE;
	/* tell the menu buttons of the state of this headlight */
	//setMenuButton_headlight(viewer->headlight);
	viewer->speed = 1.0;
	viewer->Dist = 10.0;
	memcpy (&viewer->walk, &p->viewer_walk,sizeof (X3D_Viewer_Walk));
	memcpy (&viewer->examine, &p->viewer_examine, sizeof (X3D_Viewer_Examine));
	memcpy (&viewer->fly, &p->viewer_fly, sizeof (X3D_Viewer_Fly));
	memcpy (&viewer->ypz,&p->viewer_ypz, sizeof (X3D_Viewer_Spherical));

	if(vpnodetype == NODE_OrthoViewpoint){
		fwl_set_viewer_type0(viewer,VIEWER_NONE); //for LayoutLayer default NONE
		viewer->orthoField[0] = -1.0;
		viewer->orthoField[1] =  1.0;
		viewer->orthoField[2] = -1.0;
		viewer->orthoField[3] =  1.0;
		viewer->ortho = TRUE;
	}else{
		//all other viewpoint types - Viewpoint, GeoViewpoint ...
		fwl_set_viewer_type0(viewer,VIEWER_EXAMINE);
	}
	viewer->LookatMode = 0;
	//set_eyehalf( Viewer.eyedist/2.0,
	//	atan2(Viewer.eyedist/2.0,Viewer.screendist)*360.0/(2.0*3.1415926));

	/* assume we are not bound to a GeoViewpoint */
	viewer->GeoSpatialNode = NULL;

}
//ppViewer p = (ppViewer)gglobal()->Viewer.prv;
//X3D_Viewer _Viewer; /* has to be defined somewhere, so it found itself stuck here */
X3D_Viewer *ViewerByLayerId(int layerid)
{
	X3D_Viewer *viewer;
	bindablestack *bstack;
	ttglobal tg;
	ppViewer p;
	tg = gglobal();
	p = (ppViewer)tg->Viewer.prv;
	//per-layer viewer
	bstack = getBindableStacksByLayer(tg,layerid);
	if(!bstack->viewer){
		int vpnodetype;
		viewer = MALLOCV(sizeof(X3D_Viewer));
		memset(viewer,0,sizeof(X3D_Viewer));
		vpnodetype = bstack->nodetype == NODE_LayoutLayer ? NODE_OrthoViewpoint : NODE_Viewpoint;
		viewer_default0(viewer,vpnodetype);
		init_stereodefaults(viewer);
		bstack->viewer = viewer;
	}
	return bstack->viewer;
}
X3D_Viewer *Viewer()
{
	ttglobal tg;
	tg = gglobal();
	return ViewerByLayerId(tg->Bindable.activeLayer);
}

void viewer_default() {
	X3D_Viewer *viewer;
	viewer = Viewer();
	viewer_default0(viewer,NODE_Viewpoint);
}


void resolve_pos20(X3D_Viewer *viewer);
void viewer_init (X3D_Viewer *viewer, int type) {
	Quaternion q_i;
	ppViewer p = (ppViewer)gglobal()->Viewer.prv;

	/* if we are brand new, set up our defaults */
	if (!p->viewer_initialized) {
		p->viewer_initialized = TRUE;

		/* what are we - EXAMINE, FLY, etc... */
		viewer->type = type;

		viewer->Pos.x = 0; viewer->Pos.y = 0; viewer->Pos.z = 10;
		viewer->currentPosInModel.x = 0; viewer->currentPosInModel.y = 0; viewer->currentPosInModel.z = 10;
		viewer->AntiPos.x = 0; viewer->AntiPos.y = 0; viewer->AntiPos.z = 0;


		vrmlrot_to_quaternion (&viewer->Quat,1.0,0.0,0.0,0.0);
		vrmlrot_to_quaternion (&viewer->bindTimeQuat,1.0,0.0,0.0,0.0);
		vrmlrot_to_quaternion (&viewer->prepVPQuat,1.0,0.0,0.0,0.0);
		vrmlrot_to_quaternion (&q_i,1.0,0.0,0.0,0.0);
		quaternion_inverse(&(viewer->AntiQuat),&q_i);

		viewer->headlight = TRUE;
		viewer->collision = FALSE;
		/* tell the menu buttons of the state of this headlight */
		//setMenuButton_headlight(viewer->headlight);
		viewer->speed = 1.0;
		viewer->Dist = 10.0;
		//viewer->exploreDist = 10.0;
        memcpy (&viewer->walk, &p->viewer_walk,sizeof (X3D_Viewer_Walk));
        memcpy (&viewer->examine, &p->viewer_examine, sizeof (X3D_Viewer_Examine));
        memcpy (&viewer->fly, &p->viewer_fly, sizeof (X3D_Viewer_Fly));
        memcpy (&viewer->ypz,&p->viewer_ypz, sizeof (X3D_Viewer_Spherical));


		/* SLERP code for moving between viewpoints */
		viewer->SLERPing = FALSE;
		viewer->startSLERPtime = 0.0;
		viewer->transitionType = 1; /* assume LINEAR */
		viewer->transitionTime = 1.0; /* assume 1 second */

		/* Orthographic projections */
		viewer->ortho = FALSE;

		viewer->doExamineModeDistanceCalculations = FALSE;

		/* orientation - 0 is normal */
		viewer->screenOrientation = 0;

		viewer->nearPlane=DEFAULT_NEARPLANE;                     /* near Clip plane - MAKE SURE that statusbar is not in front of this!! */
		viewer->farPlane=DEFAULT_FARPLANE;                       /* a good default value */
		viewer->backgroundPlane = DEFAULT_BACKGROUNDPLANE;       /* where Background and TextureBackground nodes go */
		viewer->fieldofview=45.0;
		viewer->fovZoom = 1.0;

		viewer->wasBound = FALSE;
	}

	resolve_pos20(viewer);

}


int getCRouteCount();
void printStatsRoutes()
{
	ConsoleMessage("%25s %d\n","Routes count", getCRouteCount());
}

void printStatsBindingStacks();
void printStatsResources();
void printStatsEvents();
void printStatsNodes();
void printStats()
{
	printMaxStackUsed();
	printStatsResources();
	printStatsEvents();
	printStatsNodes();
	printStatsRoutes();
	printStatsBindingStacks();
}

void
print_viewer()
{
	X3D_Viewer *viewer;

	struct orient_XYZA ori;
	ppViewer p = (ppViewer)gglobal()->Viewer.prv;
	viewer = Viewer();

	quaternion_to_vrmlrot(&(viewer->Quat), &(ori.x),&(ori.y),&(ori.z), &(ori.a));
	ConsoleMessage("Viewpoint local{\n");
	ConsoleMessage("\tPosition[%.4f, %.4f, %.4f]\n", (viewer->Pos).x, (viewer->Pos).y, (viewer->Pos).z);
	ConsoleMessage("\tQuaternion[%.4f, %.4f, %.4f, %.4f]\n", (viewer->Quat).w, (viewer->Quat).x, (viewer->Quat).y, (viewer->Quat).z);
	ConsoleMessage("\tOrientation[%.4f, %.4f, %.4f, %.4f]\n", ori.x, ori.y, ori.z, ori.a);
	ConsoleMessage("}\n");
	getCurrentPosInModel(FALSE);
	ConsoleMessage("World Coordinates of Avatar [%.4f, %.4f %.4f]\n",viewer->currentPosInModel.x,viewer->currentPosInModel.y,viewer->currentPosInModel.z);
	printStats();
}

int fwl_get_headlight() { 
	return(Viewer()->headlight);
}

void fwl_toggle_headlight() {
	X3D_Viewer *viewer;
	ppViewer p = (ppViewer)gglobal()->Viewer.prv;
	viewer = Viewer();

	if (viewer->headlight == TRUE) {
		viewer->headlight = FALSE;
	} else {
		viewer->headlight = TRUE;
	}
	/* tell the menu buttons of the state of this headlight */
	//setMenuButton_headlight(viewer->headlight);

}
/* July 7, 2012 I moved .collision from params to x3d_viewer struct, 
	so its like headlight and navmode */
void setNoCollision() {
	X3D_Viewer *viewer;
	ppViewer p = (ppViewer)gglobal()->Viewer.prv;
	viewer = Viewer();
	viewer->collision = 0;
	//fwl_setp_collision(0);
	//setMenuButton_collision(viewer->collision); //fwl_getp_collision());
}
int get_collision() { 
	return fwl_getCollision(); //fwl_getp_collision();
}
void toggle_collision() {
	X3D_Viewer *viewer;
	ppViewer p = (ppViewer)gglobal()->Viewer.prv;
	viewer = Viewer();
	viewer->collision = 1 - viewer->collision;

	//fwl_setp_collision(!fwl_getp_collision()); 
	//setMenuButton_collision(viewer->collision); //fwl_getp_collision());
}

int fwl_getCollision(){
	X3D_Viewer *viewer;
	ppViewer p = (ppViewer)gglobal()->Viewer.prv;
	viewer = Viewer();
	return viewer->collision;
}
void fwl_setCollision(int state) {
	X3D_Viewer *viewer;
	ppViewer p = (ppViewer)gglobal()->Viewer.prv;
	viewer = Viewer();
	viewer->collision = state;
}

void fwl_init_StereoDefaults()
{
	X3D_Viewer *viewer;
	ppViewer p = (ppViewer)gglobal()->Viewer.prv;
	viewer = Viewer();
	if(!p->StereoInitializedOnce)
		init_stereodefaults(viewer);
	p->StereoInitializedOnce = 1;
}


void set_eyehalf(const double eyehalf, const double eyehalfangle) {
	X3D_Viewer *viewer;
	ppViewer p = (ppViewer)gglobal()->Viewer.prv;
	viewer = Viewer();

	viewer->eyehalf = eyehalf;
	viewer->eyehalfangle = eyehalfangle;
	viewer->isStereo = 1;
}
void resolve_pos2();
void fwl_set_viewer_type0(X3D_Viewer *viewer, const int type) {
	ttglobal tg = gglobal();
	ppViewer p = (ppViewer)tg->Viewer.prv;

	if(viewer->type != type){
		tg->Mainloop.CTRL = FALSE; //turn off any leftover 3-state toggle
		switch(viewer->type){
			case VIEWER_LOOKAT:
			case VIEWER_EXPLORE:
				viewer->LookatMode = 0; //turn off leftover lookatMode
				break;
			default:
				break;
		}
	}

	switch(type) {
	case VIEWER_EXAMINE:
		resolve_pos20(viewer);
	case VIEWER_NONE:
	case VIEWER_WALK:
	case VIEWER_EXFLY:
	case VIEWER_TPLANE:
	case VIEWER_RPLANE:
	case VIEWER_TILT:
	case VIEWER_FLY2:
	case VIEWER_TURNTABLE:
	case VIEWER_DIST:
	case VIEWER_FLY:
		viewer->type = type;
		break;
	case VIEWER_SPHERICAL:
		//3 state toggle stack
		if(viewer->type == type){
			//this is a request to toggle on/off FOV (field-of-view) adjustment for SPHERICAL mode
			if(tg->Mainloop.CTRL){
				tg->Mainloop.CTRL = FALSE;
			}else{
				tg->Mainloop.CTRL = TRUE; //in handle_spherical, check if button==3 (RMB) or CTRL + button==1
			}
		}else{
			//request to toggle on EXPLORE mode
			viewer->type = type;
		}
		break;

	case VIEWER_EXPLORE:
		//3 state toggle stack
		if(viewer->type == type){
			//this is a request to toggle on/off CTRL for EXPLORE mode
			if(tg->Mainloop.CTRL){
				tg->Mainloop.CTRL = FALSE;
				viewer->LookatMode = 0;
			}else{
				tg->Mainloop.CTRL = TRUE;
				viewer->LookatMode = 1; //tells mainloop to turn off sensitive
			}
		}else{
			//request to toggle on EXPLORE mode
			viewer->type = type;
		}
		break;
	case VIEWER_LOOKAT:
		//2 state toggle
		if(viewer->type == type){
			//this is a request to toggle off LOOKAT mode
			viewer->type = viewer->lastType;
			viewer->LookatMode = 0;
		}else{
			//request to toggle on LOOKAT mode
			viewer->lastType = viewer->type;
			viewer->LookatMode = 1; //tells mainloop to turn off sensitive
			viewer->type = type;
		}
		break;
	default:
		ConsoleMessage ("Viewer type %d is not supported. See Viewer.h.\n", type);
		viewer->type = VIEWER_NONE;
		break;
	}

	/* set velocity array to zero again - used only for EAI */
	viewer->VPvelocity.x=0.0; viewer->VPvelocity.y=0.0; viewer->VPvelocity.z=0.0;

	/* can the currently bound viewer type handle this */
	/* if there is no bound viewer, just ignore (happens on initialization) */
	if (vectorSize(getActiveBindableStacks(tg)->navigation) >0)
		if (viewer->oktypes[type]==FALSE) {
			//setMenuButton_navModes(viewer->type);
			return;
		}

	if(1) viewer_init(viewer,type);  //feature-EXPLORE

	/* tell the window menu what we are */
	//setMenuButton_navModes(viewer->type);

}
void fwl_set_viewer_type(const int type) {
	X3D_Viewer *viewer;
	viewer = Viewer();

	fwl_set_viewer_type0(viewer, type);
}



//#define VIEWER_STRING(type) ( \
//	type == VIEWER_NONE ? "NONE" : ( \
//	type == VIEWER_EXAMINE ? "EXAMINE" : ( \
//	type == VIEWER_WALK ? "WALK" : ( \
//	type == VIEWER_EXFLY ? "EXFLY" : ( \
//	type == VIEWER_SPHERICAL ? "SPHERICAL" : (\
//	type == VIEWER_TURNTABLE ? "TURNTABLE" : (\
//	type == VIEWER_FLY ? "FLY" : "UNKNOWN"))))))
#ifdef _MSC_VER
#define strcasecmp _stricmp
#endif

struct navmode {
	char *key;
	int type;
} navmodes [] = {
	{"NONE",VIEWER_NONE},
	{"WALK",VIEWER_WALK},
	{"FLY",VIEWER_FLY},
	{"EXAMINE",VIEWER_EXAMINE},
	{"SPHERICAL",VIEWER_SPHERICAL},
	{"TURNTABLE",VIEWER_TURNTABLE},
	{"EXPLORE",VIEWER_EXPLORE},
	{"LOOKAT",VIEWER_LOOKAT},
	{"YAWZ",VIEWER_YAWZ},
	{"XY",VIEWER_XY},
	{"YAWPITCH",VIEWER_YAWPITCH},
	{"ROLL",VIEWER_ROLL},
	{"DIST",VIEWER_DIST},
	{NULL,0},
};
char * lookup_navmodestring(int navmode){
	int i;
	char *retval;
	struct navmode *nm;
	i = 0;
	retval = NULL;
	do{
		nm = &navmodes[i];
		if(nm->type == navmode){
			retval = nm->key;
			break;
		}
		i++;
	}while(navmodes[i].key);
	if(!retval) retval = "NONE";
	return retval;
}
int lookup_navmode(char *cmode){
	int i;
	int retval;
	struct navmode *nm;
	i = 0;
	retval = 0;
	do{
		nm = &navmodes[i];
		if(!strcasecmp(nm->key,cmode)){
			retval = nm->type;
			break;
		}
		i++;
	}while(navmodes[i].key);
	return retval;
}
char* fwl_getNavModeStr()
{
	X3D_Viewer *viewer;
	ttglobal tg = gglobal();
	ppViewer p = (ppViewer)tg->Viewer.prv;
	viewer = Viewer();
	return lookup_navmodestring(viewer->type);
	//switch(viewer->type) {
	//case VIEWER_NONE:
	//	return "NONE";
	//case VIEWER_EXAMINE:
	//	return "EXAMINE";
	//case VIEWER_WALK:
	//	return "WALK";
	//case VIEWER_EXFLY:
	//	return "EXFLY";
	//case VIEWER_TPLANE:
	//	return "TPLANE";
	//case VIEWER_RPLANE:
	//	return "RPLANE";
	//case VIEWER_TILT:
	//	return "TILT";
	//case VIEWER_FLY2:
	//	return "FLY2";
	//case VIEWER_SPHERICAL:
	//	return "SPHERICAL";
	//case VIEWER_TURNTABLE:
	//	return "TURNTABLE";
	//case VIEWER_FLY:
	//	return "FLY";
	//case VIEWER_LOOKAT:
	//	return "LOOKAT";
	//case VIEWER_EXPLORE:
	//	return "EXPLORE";
	//default:
	//	return "NONE";
	//}
	//return "NONE";
}
int fwl_getNavMode()
{
	X3D_Viewer *viewer;
	ttglobal tg = gglobal();
	ppViewer p = (ppViewer)tg->Viewer.prv;
	viewer = Viewer();
	return viewer->type;
}
int fwl_setNavMode(char *mode){
	int imode = lookup_navmode(mode);
	fwl_set_viewer_type(imode);
	return 0;
}

//int use_keys() {
//	ppViewer p = (ppViewer)gglobal()->Viewer.prv;
//
//	if (viewer->type == VIEWER_FLY) {
//		return TRUE;
//	}
//	return TRUE; //FALSE; //Navigation-key_and_drag
//}

void resolve_pos(){}
void resolve_pos20(X3D_Viewer *viewer) {
	/* my($this) = @_; */
	struct point_XYZ rot, z_axis = { 0, 0, 1 };
	Quaternion q_inv;
	//double dist = 0;
	ppViewer p = (ppViewer)gglobal()->Viewer.prv;

	X3D_Viewer_Examine *examine = &viewer->examine;


	//if (viewer->type == VIEWER_EXAMINE  || (viewer->type == VIEWER_LOOKAT && viewer->lastType == VIEWER_EXAMINE) ) {
		/* my $z = $this->{Quat}->invert->rotate([0,0,1]); */
		quaternion_inverse(&q_inv, &(viewer->Quat));
		quaternion_rotation(&rot, &q_inv, &z_axis);

		/* my $d = 0; for(0..2) {$d += $this->{Pos}[$_] * $z->[$_]} */
		//dist = VECPT(viewer->Pos, rot);

		/* $this->{Origin} = [ map {$this->{Pos}[$_] - $d * $z->[$_]} 0..2 ]; */
/*
printf ("RP, before orig calc %4.3f %4.3f %4.3f\n",examine->Origin.x, examine->Origin.y,examine->Origin.z);
*/
		(examine->Origin).x = (viewer->Pos).x - viewer->Dist * rot.x;
		(examine->Origin).y = (viewer->Pos).y - viewer->Dist * rot.y;
		(examine->Origin).z = (viewer->Pos).z - viewer->Dist * rot.z;
/*
printf ("RP, aft orig calc %4.3f %4.3f %4.3f\n",examine->Origin.x, examine->Origin.y,examine->Origin.z);
*/
	//}
}
void resolve_pos2() {
	X3D_Viewer *viewer;
	viewer = Viewer();
	resolve_pos20(viewer);
}
double vecangle2(struct point_XYZ* V1, struct point_XYZ* V2, struct point_XYZ* rotaxis) {
	/* similar full circle angle computation as:
	double matrotate2v() 
	*/

	double cosine, sine, ulen, vlen, scale, dot, angle;
	struct point_XYZ cross;
	/* use dot product to get cosine:  cosTheta = (U dot V)/(||u||||v||) */
	dot = vecdot(V1,V2);
	ulen = sqrt(vecdot(V1,V1));
	vlen = sqrt(vecdot(V2,V2));
	scale = ulen*vlen;
	if( APPROX(scale, 0.0) )
	{
		rotaxis->y = rotaxis->z = 0.0;
		rotaxis->x = 1.0; //arbitrary axis
		return 0.0;
	}
	cosine = dot/scale;
	/* use cross product to get sine: ||u X v|| = ||u||||v||sin(theta) or sinTheta = ||uXv||/(||u||||v||)*/
	veccross(&cross,*V1,*V2);
	sine = sqrt(vecdot(&cross,&cross))/scale;
	/* get full circle unambiguous angle using both cosine and sine */
	angle = atan2(sine,cosine);
	vecnormal(rotaxis,&cross);
	return angle;
}
void avatar2BoundViewpointVerticalAvatar(GLDOUBLE *matA2BVVA, GLDOUBLE *matBVVA2A)
{
	/* goal: make 2 transform matrices to go back and forth from Avatar A to 
	   Bound-Viewpoint-Vertical aligned Avatar-centric (no translations or scales - just 2 tilts) coordinates
    */
	X3D_Viewer *viewer;
	struct point_XYZ tilted;
	struct point_XYZ downvec = {0.0,-1.0,0.0};
	ppViewer p = (ppViewer)gglobal()->Viewer.prv;
	viewer = Viewer();

	//downvec is in bound viewpoint space
	quaternion_rotation(&tilted, &viewer->Quat, &downvec);
	//tilted is in avatar space.
	matrotate2v(matA2BVVA,downvec,tilted); 
	matrotate2v(matBVVA2A,tilted,downvec); 
	//printmatrix2(matA2BVVA,"A2BVVA" );
	//printmatrix2(matBVVA2A,"BVVA2A");
	return;
}

void viewer_level_to_bound() 
{
/* 
Goal: Gravity as per specs 
Gravity:
	From specs > abstract > architecture > 23.3.4 NavigationInfo:
	"The speed, avatarSize and visibilityLimit values are all scaled by the transformation being applied 
	to the currently bound Viewpoint node. 
	If there is no currently bound Viewpoint node, the values are interpreted in the world coordinate system. "

	"For purposes of terrain following, the browser maintains a notion of the down direction (down vector), since gravity 
	is applied in the direction of the down vector. This down vector shall be along the negative Y-axis in the 
	local coordinate system of the currently bound Viewpoint node (i.e., the accumulation of the Viewpoint node's 
	ancestors' transformations, not including the Viewpoint node's orientation field)."

	From specs > abstract > architecture > 23.3.5 Viewpoint
	"When a Viewpoint node is at the top of the stack, the user's view is 
	conceptually re-parented as a child of the Viewpoint node."
	
	"Navigation types (see 23.3.4 NavigationInfo) that require a definition of a down vector (e.g., terrain following) 
	shall use the negative Y-axis of the coordinate system of the currently bound Viewpoint node. 
	Likewise, navigation types that require a definition of an up vector shall use the positive Y-axis of the 
	coordinate system of the currently bound Viewpoint node. The orientation field of the Viewpoint node does 
	not affect the definition of the down or up vectors. This allows the author to separate the viewing direction 
	from the gravity direction."

	Implication: if your entire scene is tilted (ie. Z up), along with your viewpoint, you shouldn't notice. 
	Even when terrain following, stepping, colliding.

Transforms:
World > [TransformStack] > Bound-Viewpoint > [Quat + Pos] > viewer/avatar > [AntiQuat + AntiPos?] > Bound-Viewpoint > Inverse[TransformStack] > World   
Viewer.Quat, Viewer.Pos - local pose of avatar wrt its currently bound viewpoint parent. 
	Includes/contains the viewpoint node's position and orientation field info.

ViewerUpVector: - looks like a global tilt of the avatar - I don't use it here or in collision
ViewerUpVector computation - see RenderFuncs.c L595   
*/

	/*
	first attempts at leveling avatar to bound viewpoint:
	1. Transform a bound-viewpoint-coordinates down vector {0,-1,0} to avatar coords using Quat
	2. compute tilts of that down vector in avatar space
	3. apply inverse tilts to end of transform chain ie Quat = Quat*inverse(tilts)
	*/
	struct point_XYZ rotaxis, tilted;
	Quaternion q, Quat; //, AntiQuat;
	double angle;
	X3D_Viewer *viewer;
	struct point_XYZ downvec = {0.0,-1.0,0.0};
	ppViewer p = (ppViewer)gglobal()->Viewer.prv;

	viewer = Viewer();
	Quat = viewer->Quat;
	//AntiQuat = Viewer.AntiQuat;
	quaternion_rotation(&tilted, &Quat, &downvec);
	//tilted is in avatar space.
	angle = vecangle2(&downvec,&tilted,&rotaxis);
	if( APPROX(angle,0.0) ) return; //we're level already
	vrmlrot_to_quaternion(&q, rotaxis.x, rotaxis.y, rotaxis.z, -angle );
	quaternion_normalize(&q);
	quaternion_multiply(&(viewer->Quat), &q, &Quat);
	quaternion_normalize(&(viewer->Quat));

	/* make sure Viewer.Dist is configured properly for Examine mode */
	//CALCULATE_EXAMINE_DISTANCE
}

void viewer_togl(double fieldofview) 
{
	/* goal: take the curring Viewer pose (.Pos, .Quat) and set openGL transforms
	   to prepare for a separate call to move the viewpoint - 
	   (currently done in Mainloop.c setup_viewpoint())
	Explanation of AntiPos, AntiQuat:
		If there's a viewpoint vp, We want to 
			a) navigate away from the initial bind_viewpoint transform + (.position,.orientation) pose
			b) start navigation from where vp.position, vp.orientation tell us.
			c) remain responsive if there are changes to .position or .orientation. during run
			- either javascript or routing may change vp.position, .orientation
		To accomodate all this we:
			a) create variables Viewer.Pos, .Quat to hold the navigation 
			b) initially set Viewer.Pos, .Quat to vp.position, .orientation
			- see INITIALIZE_POSE_ANTIPOSE and its use in bind_viewpoint
			c) subtract off initially bound .position, .orientation and add on current .position, .orientation 
			- below, AntiPos and AntiQuat hold the original .orientation, .position values set during bind_viewpoint
			- prep_viewpoint in module Component_Navigation then adds back on 
				the current (javascript/routing changed) .position, .orientation
			- if no change to .position, .orientation after binding, then these 2 
				(AntiPos,AntiQuat) and (vp.position,vp.orientation) are equal and cancel
				leaving the .Pos, .Quat -initially with .position, .orientation- in the modelview matrix stack
    */
	X3D_Viewer *viewer;
	ttglobal tg;
	ppViewer p;
	tg = gglobal();
	p = (ppViewer)tg->Viewer.prv;

	viewer = Viewer();
	if (viewer->isStereo) /* buffer != GL_BACK)  */
		set_stereo_offset0(); /*Viewer.iside, Viewer.eyehalf, Viewer.eyehalfangle);*/

	if (viewer->SLERPing) {
		double tickFrac;
		Quaternion slerpedDiff;
		struct point_XYZ pos, antipos;

/*
printf ("SLERPing...\n");
printf ("\t	startSlerpPos %lf %lf %lf\n",Viewer.startSLERPPos.x,Viewer.startSLERPPos.y,Viewer.startSLERPPos.z);
printf ("\t	Pos           %lf %lf %lf\n",Viewer.Pos.x,Viewer.Pos.y,Viewer.Pos.z);
printf ("\t	startSlerpAntiPos %lf %lf %lf\n",Viewer.startSLERPAntiPos.x,Viewer.startSLERPAntiPos.y,Viewer.startSLERPAntiPos.z);
printf ("\t	AntiPos           %lf %lf %lf\n",Viewer.AntiPos.x,Viewer.AntiPos.y,Viewer.AntiPos.z);
*/

		/* printf ("slerping in togl, type %s\n", VIEWER_STRING(Viewer.type)); */
		tickFrac = (TickTime() - viewer->startSLERPtime)/viewer->transitionTime;
		//tickFrac = tickFrac/4.0;
		//printf ("tick frac %lf\n",tickFrac); 

		pos.x = viewer->Pos.x * tickFrac + (viewer->startSLERPPos.x * (1.0 - tickFrac));
		pos.y = viewer->Pos.y * tickFrac + (viewer->startSLERPPos.y * (1.0 - tickFrac));
		pos.z = viewer->Pos.z * tickFrac + (viewer->startSLERPPos.z * (1.0 - tickFrac));
		/* printf("ticfrac= %lf pos.xyz= %lf %lf %lf\n",tickFrac,pos.x,pos.y,pos.z); */
		antipos.x = viewer->AntiPos.x * tickFrac + (viewer->startSLERPAntiPos.x * (1.0 - tickFrac));
		antipos.y = viewer->AntiPos.y * tickFrac + (viewer->startSLERPAntiPos.y * (1.0 - tickFrac));
		antipos.z = viewer->AntiPos.z * tickFrac + (viewer->startSLERPAntiPos.z * (1.0 - tickFrac));

		quaternion_slerp (&slerpedDiff,&viewer->startSLERPQuat,&viewer->Quat,tickFrac);

		quaternion_togl(&slerpedDiff);
		FW_GL_TRANSLATE_D(-pos.x, -pos.y, -pos.z);
		FW_GL_TRANSLATE_D(antipos.x, antipos.y, antipos.z);
		quaternion_slerp (&slerpedDiff,&viewer->startSLERPAntiQuat,&viewer->AntiQuat,tickFrac);
		quaternion_togl(&slerpedDiff);


		if (tickFrac >= 1.0) viewer->SLERPing = FALSE;
	} else {
		quaternion_togl(&viewer->Quat);
		FW_GL_TRANSLATE_D(-(viewer->Pos).x, -(viewer->Pos).y, -(viewer->Pos).z);
		FW_GL_TRANSLATE_D((viewer->AntiPos).x, (viewer->AntiPos).y, (viewer->AntiPos).z);
		quaternion_togl(&viewer->AntiQuat);

	}

	getCurrentPosInModel(TRUE);
}

/* go through the modelMatrix and see where we are. Notes:
	- this should ideally be done in prep_Viewpoint, but if there is NO viewpoint... at least
	  here, it gets called. (that is why the antipos is added in here)

	- for X3D Viewpoints, this one adds in the AntiPos; for GeoViewpoints, we do a get after
	  doing Geo transform and rotation that are integral with the GeoViewpoint node.
*/


void getCurrentPosInModel (int addInAntiPos) {
	X3D_Viewer *viewer;
	struct point_XYZ rp;
	struct point_XYZ tmppt;

	GLDOUBLE modelMatrix[16];
	GLDOUBLE inverseMatrix[16];
	ppViewer p = (ppViewer)gglobal()->Viewer.prv;
	viewer = Viewer();

	/* "Matrix Quaternion FAQ: 8.050
	Given the current ModelView matrix, how can I determine the object-space location of the camera?

   	The "camera" or viewpoint is at (0., 0., 0.) in eye space. When you
   	turn this into a vector [0 0 0 1] and multiply it by the inverse of
   	the ModelView matrix, the resulting vector is the object-space
   	location of the camera.

   	OpenGL doesn't let you inquire (through a glGet* routine) the
   	inverse of the ModelView matrix. You'll need to compute the inverse
   	with your own code." */


       FW_GL_GETDOUBLEV(GL_MODELVIEW_MATRIX, modelMatrix);

/* printf ("togl, before inverse, %lf %lf %lf\n",modelMatrix[12],modelMatrix[13],modelMatrix[14]);
       printf ("Viewer end _togl modelview Matrix: \n\t%5.2f %5.2f %5.2f %5.2f\n\t%5.2f %5.2f %5.2f %5.2f\n\t%5.2f %5.2f %5.2f %5.2f\n\t%5.2f %5.2f %5.2f %5.2f\n",
                modelMatrix[0],  modelMatrix[4],  modelMatrix[ 8],  modelMatrix[12],
                modelMatrix[1],  modelMatrix[5],  modelMatrix[ 9],  modelMatrix[13],
                modelMatrix[2],  modelMatrix[6],  modelMatrix[10],  modelMatrix[14],
                modelMatrix[3],  modelMatrix[7],  modelMatrix[11],  modelMatrix[15]);
*/


	matinverseAFFINE(inverseMatrix,modelMatrix);

/*
printf ("togl, after inverse, %lf %lf %lf\n",inverseMatrix[12],inverseMatrix[13],inverseMatrix[14]);
       printf ("inverted modelview Matrix: \n\t%5.2f %5.2f %5.2f %5.2f\n\t%5.2f %5.2f %5.2f %5.2f\n\t%5.2f %5.2f %5.2f %5.2f\n\t%5.2f %5.2f %5.2f %5.2f\n",
                inverseMatrix[0],  inverseMatrix[4],  inverseMatrix[ 8],  inverseMatrix[12],
                inverseMatrix[1],  inverseMatrix[5],  inverseMatrix[ 9],  inverseMatrix[13],
                inverseMatrix[2],  inverseMatrix[6],  inverseMatrix[10],  inverseMatrix[14],
                inverseMatrix[3],  inverseMatrix[7],  inverseMatrix[11],  inverseMatrix[15]);
*/


	tmppt.x = inverseMatrix[12];
	tmppt.y = inverseMatrix[13];
	tmppt.z = inverseMatrix[14];



	if (addInAntiPos) {
		/* printf ("going to do rotation on %f %f %f\n",tmppt.x, tmppt.y, tmppt.z); */
		quaternion_rotation(&rp, &viewer->bindTimeQuat, &tmppt);
		/* printf ("new inverseMatrix  after rotation %4.2f %4.2f %4.2f\n",rp.x, rp.y, rp.z); */

		viewer->currentPosInModel.x = viewer->AntiPos.x + rp.x;
		viewer->currentPosInModel.y = viewer->AntiPos.y + rp.y;
		viewer->currentPosInModel.z = viewer->AntiPos.z + rp.z;
	} else {
		//at scene root level, after setup_viewpoint(), modelview matrix is the view matrix, and has all transforms 
		// including .orientation, .position anti-pos, antiquat applied
		if(0) quaternion_rotation(&rp, &viewer->bindTimeQuat, &tmppt);
		if(1) {rp.x = tmppt.x; rp.y = tmppt.y; rp.z = tmppt.z;}
		viewer->currentPosInModel.x = rp.x;
		viewer->currentPosInModel.y = rp.y;
		viewer->currentPosInModel.z = rp.z;
	}

	
/* 	printf ("getCurrentPosInModel, so, our place in object-land is %4.2f %4.2f %4.2f\n",
		Viewer.currentPosInModel.x, Viewer.currentPosInModel.y, Viewer.currentPosInModel.z);
*/
}




double quadratic(double x,double a,double b,double c)
{
	/* y = a*x*x + b*x + c; */
	return x*x*a + x*b + c;
}
double xsign_quadratic(double x,double a,double b,double c)
{
	/* y = sign(x)*(a*abs(x)*abs(x) + b*abs(x) + c); */
	double xSign;
	//xSign = _copysign(1.0,x); _MSC_VER
	if(x < 0.0) xSign = -1.0; else xSign = 1.0;
	x = fabs(x);
	return xSign*quadratic(x,a,b,c);
}
static void handle_walk(const int mev, const unsigned int button, const float x, const float y) {
/*
 * walk.xd,zd are in a plane parallel to the scene/global horizon.
 * walk.yd is vertical in the global/scene
 * walk.rd is an angle in the global/scene horizontal plane (around vertical axis)
*/
	ttglobal tg;
	ppViewer p;
	X3D_Viewer *viewer;

	X3D_Viewer_Walk *walk; 
	double frameRateAdjustment = 1.0;
	tg = gglobal();
	p = (ppViewer)tg->Viewer.prv;
	viewer = Viewer();
	walk = &viewer->walk;

	if( tg->Mainloop.BrowserFPS > 0)
		frameRateAdjustment = 20.0 / tg->Mainloop.BrowserFPS; /* lets say 20FPS is our speed benchmark for developing tuning parameters */
	else
		frameRateAdjustment = 1.0;
	

	if (mev == ButtonPress ) {
		walk->SY = y;
		walk->SX = x;
	} else if (mev == MotionNotify) {
		if (button == 1) {
			/* July31,2010 new quadratic speed: allows you slow speed with small mouse motions, or 
			   fast speeds with large mouse motions. The .05, 5.0 etc are tuning parameters - I tinkered / experimented
			   using the townsite scene http://dug9.users.sourceforge.net/web3d/townsite_2014/townsite.x3d
			   which has the default navigationInfo speed (1.0) and is to geographic scale in meters.
			   If the tuning params don't work for you please fix/iterate/re-tune/change back/put a switch
			   I find them amply speedy, maybe yaw a bit too fast 
			   dug9: button 1 ZD: .05 5.0 0.0  RD: .1 .5 0.0
				     button 3 XD: 5.0 10.0 0.0 YD: 5.0 10.0 0.0
			*/
			walk->ZD = -xsign_quadratic(y - walk->SY,.05,5.0,0.0)*viewer->speed * frameRateAdjustment;
			walk->RD = xsign_quadratic(x - walk->SX,0.1,0.5,0.0)*frameRateAdjustment;
			//walk->ZD = (y - walk->SY) * Viewer.speed;
			//walk->RD = (x - walk->SX) * 0.1;
		} else if (button == 3) {
			walk->XD =  xsign_quadratic(x - walk->SX,5.0,10.0,0.0)*viewer->speed * frameRateAdjustment;
			walk->YD =  xsign_quadratic(y - walk->SY,5.0,10.0,0.0)*viewer->speed * frameRateAdjustment;
			//walk->XD = (x - walk->SX) * Viewer.speed;
			//walk->YD = -(y - walk->SY) * Viewer.speed;
		}
	} else if (mev == ButtonRelease) {
		if (button == 1) {
			walk->ZD = 0;
			walk->RD = 0;
		} else if (button == 3) {
			walk->XD = 0;
			walk->YD = 0;
		}
	}
}


static double
  norm(const Quaternion *quat)
  {
        return(sqrt(
                                quat->w * quat->w +
                                quat->x * quat->x +
                                quat->y * quat->y +
                                quat->z * quat->z
                                ));
  }


void handle_examine(const int mev, const unsigned int button, float x, float y) {
	Quaternion q, q_i, arc;
	struct point_XYZ pp = { 0, 0, 0};
	double squat_norm;
	ppViewer p;
	X3D_Viewer *viewer;
	X3D_Viewer_Examine *examine;
	p = (ppViewer)gglobal()->Viewer.prv;
	viewer = Viewer();
	examine = &viewer->examine;
	pp.z=viewer->Dist;

	if (mev == ButtonPress) {
		if (button == 1) {
			resolve_pos2();
/*
			printf ("\n");
			printf ("bp, before SQ %4.3f %4.3f %4.3f %4.3f\n",examine->SQuat.x, examine->SQuat.y, examine->SQuat.z, examine->SQuat.w);
			printf ("bp, before OQ %4.3f %4.3f %4.3f %4.3f\n",examine->OQuat.x, examine->OQuat.y, examine->OQuat.z, examine->OQuat.w);
			printf ("bp, before Q %4.3f %4.3f %4.3f %4.3f\n",Viewer.Quat.x, Viewer.Quat.y, Viewer.Quat.z, Viewer.Quat.w);
			printf ("bp, before, pos %4.3f %4.3f %4.3f\n",Viewer.Pos.x, Viewer.Pos.y, Viewer.Pos.z);
			printf ("bp, before, aps %4.3f %4.3f %4.3f\n",Viewer.AntiPos.x, Viewer.AntiPos.y, Viewer.AntiPos.z);
*/
			xy2qua(&(examine->SQuat), x, y);
			quaternion_set(&(examine->OQuat), &(viewer->Quat));
/*
			printf ("bp, after SQ %4.3f %4.3f %4.3f %4.3f\n",examine->SQuat.x, examine->SQuat.y, examine->SQuat.z, examine->SQuat.w);
			printf ("bp, after OQ %4.3f %4.3f %4.3f %4.3f\n",examine->OQuat.x, examine->OQuat.y, examine->OQuat.z, examine->OQuat.w);
			printf ("bp, after Q %4.3f %4.3f %4.3f %4.3f\n",Viewer.Quat.x, Viewer.Quat.y, Viewer.Quat.z, Viewer.Quat.w);
			printf ("bp, after, pos %4.3f %4.3f %4.3f\n",Viewer.Pos.x, Viewer.Pos.y, Viewer.Pos.z);
			printf ("bp, after, aps %4.3f %4.3f %4.3f\n",Viewer.AntiPos.x, Viewer.AntiPos.y, Viewer.AntiPos.z);
*/

		} else if (button == 3) {
			examine->SY = y;
			examine->ODist = max(0.1,viewer->Dist);
		}
	} else if (mev == MotionNotify) {
		if (button == 1) {
			squat_norm = norm(&(examine->SQuat));
			/* we have missed the press */
			if (APPROX(squat_norm, 0)) {
				fprintf(stderr, "Viewer handle_examine: mouse event DRAG - missed press\n");
				/* 			$this->{SQuat} = $this->xy2qua($mx,$my); */
				xy2qua(&(examine->SQuat), x, y);
				/* 			$this->{OQuat} = $this->{Quat}; */
				quaternion_set(&(examine->OQuat), &(viewer->Quat));
			} else {
				/* my $q = $this->xy2qua($mx,$my); */
				xy2qua(&q, x, y);
				/* my $arc = $q->multiply($this->{SQuat}->invert()); */
				quaternion_inverse(&q_i, &(examine->SQuat));
				quaternion_multiply(&arc, &q, &q_i);


				/* $this->{Quat} = $arc->multiply($this->{OQuat}); */
				quaternion_multiply(&(viewer->Quat), &arc, &(examine->OQuat));
			}
		} else if (button == 3) {
			#ifndef DISABLER
			viewer->Dist = examine->ODist * exp(examine->SY - y);
			#else
			viewer->Dist = (0 != y) ? examine->ODist * examine->SY / y : 0;
			#endif
		}
 	}

	quaternion_inverse(&q_i, &(viewer->Quat));
	quaternion_rotation(&(viewer->Pos), &q_i, &pp);
/*
	printf ("bp, after quat rotation, pos %4.3f %4.3f %4.3f\n",Viewer.Pos.x, Viewer.Pos.y, Viewer.Pos.z);
*/
	viewer->Pos.x += (examine->Origin).x;
	viewer->Pos.y += (examine->Origin).y;
	viewer->Pos.z += (examine->Origin).z;
/*
printf ("examine->origin %4.3f %4.3f %4.3f\n",examine->Origin.x, examine->Origin.y, examine->Origin.z);
*/
}

void handle_dist(const int mev, const unsigned int button, float x, float y) {
	/* different than z, this adjusts the viewer->Dist value for examine, turntable, explore, lookat
		- all without using RMB (right mouse button), so mobile friendly
	*/
	//examine variant - doesn't move the vp/.pos
	Quaternion q_i;
	struct point_XYZ pp = { 0, 0, 0};
	double yy;
	X3D_Viewer *viewer;
	ppViewer p;
	X3D_Viewer_Examine *examine;
	p = (ppViewer)gglobal()->Viewer.prv;
	viewer = Viewer();
	examine = &viewer->examine;
	pp.z=viewer->Dist;

	//ConsoleMessage("handle_dist but %d mev %d\n", button, mev);
	//yy = 1.0 - y;
	yy = y;
	if (mev == ButtonPress) {
		if (button == 1) {
			resolve_pos2();
			examine->SY = yy;
			examine->ODist = max(0.1,viewer->Dist);
		}
	} else if (mev == MotionNotify) {
		if (button == 1) {
			#ifndef DISABLER
			viewer->Dist = examine->ODist * exp(2.0 * (examine->SY - yy));
			#else
			viewer->Dist = (0 != yy) ? examine->ODist * examine->SY / yy : 0;
			#endif
			//printf("v.dist=%lf\n",viewer->Dist);
		}
	}
	quaternion_inverse(&q_i, &(viewer->Quat));
	quaternion_rotation(&(viewer->Pos), &q_i, &pp);
/*
	printf ("bp, after quat rotation, pos %4.3f %4.3f %4.3f\n",Viewer.Pos.x, Viewer.Pos.y, Viewer.Pos.z);
*/
	viewer->Pos.x += (examine->Origin).x;
	viewer->Pos.y += (examine->Origin).y;
	viewer->Pos.z += (examine->Origin).z;

}

double display_screenRatio();
void handle_turntable(const int mev, const unsigned int button, float x, float y) {
	/*
	Like handle_spherical, except:
	move the viewer->Pos in the opposite direction from where we are looking
	*/
	double frameRateAdjustment;
	X3D_Viewer_Spherical *ypz;
	X3D_Viewer *viewer;
	ppViewer p;
	ttglobal tg = gglobal();
	p = (ppViewer)gglobal()->Viewer.prv;
	viewer = Viewer();
	ypz = &viewer->ypz; //just a place to store last mouse xy during drag

	if(APPROX(viewer->Dist,0.0)){
		//no pivot point yet
		viewer->Dist = 10.0;
	}

	if( tg->Mainloop.BrowserFPS > 0)
		frameRateAdjustment = 20.0 / tg->Mainloop.BrowserFPS; /* lets say 20FPS is our speed benchmark for developing tuning parameters */
	else
		frameRateAdjustment = 1.0;


	if (mev == ButtonPress) {
		if (button == 1 || button == 3) {
			ypz->x = x;
			ypz->y = y;
		}
	}
	else if (mev == MotionNotify) 
	{
		Quaternion qyaw, qpitch;
		double dyaw, dpitch;
		struct point_XYZ pp, yaxis;
		double yaw, pitch; //dist,
		Quaternion quat;

		yaw = pitch = 0.0;
		if (button == 1 || button == 3){
			struct point_XYZ dd,ddr;
			yaxis.x = yaxis.z = 0.0;
			yaxis.y = 1.0;
			//pp = viewer->Pos;
			//if(0) resolve_pos2();
			//if(1) {
				//(examine->Origin).x = (viewer->Pos).x - viewer->Dist * rot.x;
				dd.x = dd.y = 0.0; dd.z = viewer->Dist; //exploreDist;
				quat = viewer->Quat;
				quaternion_inverse(&quat,&quat);
				quaternion_rotation(&ddr, &quat, &dd);
				vecdiff(&viewer->examine.Origin,&viewer->Pos,&ddr);
			//}

			//if(0) vecdiff(&pp,&viewer->examine.Origin,&viewer->Pos);
			//if(1) vecdiff(&pp,&viewer->Pos,&viewer->examine.Origin);
			pp = ddr;
			//if(0) printf("D=%f O=%f %f %f P=%f %f %f pp=%f %f %f\n", viewer->Dist,
			//viewer->examine.Origin.x,viewer->examine.Origin.y,viewer->examine.Origin.z,
			//viewer->Pos.x,viewer->Pos.y,viewer->Pos.z,
			//pp.x,pp.y,pp.z
			//);
			//dist = veclength(pp);
			vecnormal(&pp, &pp);
			yaw = -atan2(pp.x, pp.z);
			pitch = -(acos(vecdot(&pp, &yaxis)) - PI*.5);
		}
		if (button == 1) {
			dyaw = -(ypz->x - x) * viewer->fieldofview*PI / 180.0*viewer->fovZoom * display_screenRatio(); //tg->display.screenRatio;
			dpitch = (ypz->y - y) * viewer->fieldofview*PI / 180.0*viewer->fovZoom;
			//if(0){
			//	dyaw = -dyaw;
			//	dpitch = -dpitch;
			//}
			yaw += dyaw;
			pitch += dpitch;
		}else if (button == 3) {
			//distance drag
			if(0){
				//peddling
				double d, fac;
				d = (y - ypz->y)*.5; // .25;
				if (d > 0.0)
					fac = ((d *  2.0) + (1.0 - d) * 1.0);
				else
				{
					d = fabs(d);
					fac = ((d * .5) + (1.0 - d) * 1.0);
				}
				//dist *= fac;
				viewer->Dist *= fac;
			}
			if(1) {
				//handle_tick_explore quadratic
				//double quadratic = -xsign_quadratic(y - ypz->y,5.0,10.0,0.0);
				ypz->ypz[1] = -xsign_quadratic(y - ypz->y,100.0,10.0,0.0)*viewer->speed * frameRateAdjustment *.15;
				//printf("quad=%f y-y %f s=%f fra=%f\n",quadratic,y-ypz->y,viewer->speed,frameRateAdjustment);
			}
		}
		if (button == 1 || button == 3)
		{
			vrmlrot_to_quaternion(&qyaw, 0.0, 1.0, 0.0, yaw);
			vrmlrot_to_quaternion(&qpitch, 1.0, 0.0, 0.0, pitch);
			quaternion_multiply(&quat, &qpitch, &qyaw);
			quaternion_normalize(&quat);

			quaternion_set(&(viewer->Quat), &quat);
			//move the viewer->pos in the opposite direction that we are looking
			quaternion_inverse(&quat, &quat);
			pp.x = 0.0;
			pp.y = 0.0;
			pp.z = viewer->Dist; //dist;
			quaternion_rotation(&(viewer->Pos), &quat, &pp);
			//remember the last drag coords for next motion
			vecadd(&viewer->Pos,&viewer->examine.Origin,&viewer->Pos);
		}
		if( button == 1){
			ypz->x = x;
			ypz->y = y;
		}
	}else if(mev == ButtonRelease) {
		if (button == 3) {
			ypz->ypz[1] = 0.0;
		}
	}
}


void handle_spherical(const int mev, const unsigned int button, float x, float y) {
	/* handle_examine almost works except we don't want roll-tilt, and we want to zoom */
	int ibutton;
	Quaternion qyaw, qpitch;
	double dyaw,dpitch;
	/* unused double dzoom; */
	X3D_Viewer *viewer;
	X3D_Viewer_Spherical *ypz;
	ppViewer p;
	ttglobal tg = gglobal();
	p = (ppViewer)gglobal()->Viewer.prv;
	viewer = Viewer();
	ypz = &viewer->ypz;
	ibutton = button;
	if(ibutton == 1 && tg->Mainloop.CTRL) ibutton = 3; //RMB method for mobile/touch

	if (mev == ButtonPress) {
		if (ibutton == 1 || ibutton == 3) {
			ypz->x = x;
			ypz->y = y;
		}
	} else if (mev == MotionNotify) {
		if (ibutton == 1) {
			double yaw, pitch;
			Quaternion quat;
			struct point_XYZ dd, ddr, yaxis;

			//step 1 convert Viewer.Quat to yaw, pitch (discard any roll)
			yaxis.x = yaxis.z = 0.0;
			yaxis.y = 1.0;

			dd.x = dd.y = 0.0; dd.z = 1.0; 
			quat = viewer->Quat;
			quaternion_inverse(&quat,&quat);
			quaternion_rotation(&ddr, &quat, &dd);
			yaw = -atan2(ddr.x,ddr.z);
			pitch = -(acos(vecdot(&ddr, &yaxis)) - PI*.5);

			//step 2 add on any mouse motion as yaw,pitch chord
			dyaw   = (ypz->x - x) * viewer->fieldofview*PI/180.0*viewer->fovZoom * display_screenRatio(); //tg->display.screenRatio; 
			dpitch = -(ypz->y - y) * viewer->fieldofview*PI/180.0*viewer->fovZoom;
			yaw += dyaw;
			pitch += dpitch;

			//step 3 convert yaw, pitch back to Viewer.Quat
			vrmlrot_to_quaternion(&qyaw, 0.0, 1.0, 0.0, yaw);
			vrmlrot_to_quaternion(&qpitch, 1.0, 0.0, 0.0, pitch);
			quaternion_multiply(&quat, &qpitch, &qyaw);
			quaternion_normalize(&quat);

			quaternion_set(&(viewer->Quat), &quat);

		} else if (ibutton == 3) {
			double d, fac;
			d = -(y - ypz->y)*.5;
			fac = pow(10.0,d);
			viewer->fovZoom = viewer->fovZoom * fac;
			//viewer->fovZoom = DOUBLE_MIN(2.0,DOUBLE_MAX(.125,viewer->fovZoom));  
		}
		if(ibutton == 1 || ibutton == 3){
			ypz->x = x;
			ypz->y = y;
		}
 	}
}



/* fly2, tilt, tplane, rplane form a set that replaces keyboard fly for 
	touch devices. Collision / gravity only differentiates WALK, and treats all 
	other modes the same as fly.
	When FLY mode is set from the scene, the front end (or statusbarHud)
	switches to FLY2 which navigates similar to walk mode except with 
	(default) no gravity and a (default) spherical collision volume.
*/
void viewer_lastQ_set(Quaternion *lastQ);
void handle_fly2(const int mev, const unsigned int button, float x, float y) {
	/* there's a handle_tick_fly2() so handle_fly2() must turn on/off the
		tick action based on mev (mouse up/down/move)
	*/
	X3D_Viewer *viewer;
	ttglobal tg;
	ppViewer p;
	X3D_Viewer_InPlane *inplane;
	tg = gglobal();
	p = (ppViewer)tg->Viewer.prv;
	viewer = Viewer();
	inplane = &viewer->inplane;
	
	if (mev == ButtonPress) {
		inplane->x = x;
		inplane->y = y;
		inplane->xx = x;
		inplane->yy = y;
		inplane->on = 1;
	} else if (mev == MotionNotify) {
		inplane->xx = x;
		inplane->yy = y;
	} else if (mev == ButtonRelease ) {
		inplane->on = 0;
	}
	
}



void handle_tick_fly2(double dtime) {
	ttglobal tg;
	ppViewer p;
	X3D_Viewer_InPlane *inplane;
	double frameRateAdjustment, xx, yy, zz, rot;
	struct point_XYZ xyz;
	Quaternion q, nq;
	X3D_Viewer *viewer;
	tg = gglobal();
	p = (ppViewer)tg->Viewer.prv;
	viewer = Viewer();
	inplane = &viewer->inplane;

	if( tg->Mainloop.BrowserFPS > 0)
		frameRateAdjustment = 20.0 / tg->Mainloop.BrowserFPS; 
	else
		frameRateAdjustment = 1.0;
	
	if (inplane->on) {
		xx = inplane->xx - inplane->x;
		yy = inplane->yy - inplane->y;
		zz = -xsign_quadratic(yy,.05,5.0,0.0)*viewer->speed * frameRateAdjustment;
		zz *= 0.15;

		xyz.x = 0.0;
		xyz.y = 0.0;
		xyz.z = zz;

		rot = xsign_quadratic(xx,0.1,0.5,0.0)*frameRateAdjustment;
		//printf("rot=%lf zz=%lf\n",rot,zz);
		memcpy(&q,&viewer->Quat,sizeof(Quaternion));
		vrmlrot_to_quaternion (&nq,0.0,1.0,0.0,0.4*rot);
		viewer_lastQ_set(&nq); //wall penetration - last avatar pose is stored before updating
		quaternion_multiply(&(viewer->Quat), &nq, &q); //Quat = walk->RD * Quat
		//does the Z gets transformed by the quat?
		increment_pos(&xyz);
		//inplane->x = x;
		//inplane->y = y;
		//CALCULATE_EXAMINE_DISTANCE
 	}
	
}

void handle_lookat(const int mev, const unsigned int button, float x, float y) {
	/* do nothing on mouse down or mouse move
		on mouse up, trigger node picking action in mainloop
	*/
	X3D_Viewer *viewer;
	ttglobal tg;
	ppViewer p;
	tg = gglobal();
	p = (ppViewer)tg->Viewer.prv;
	viewer = Viewer();
	
	switch(mev){
		case  ButtonPress:
		case MotionNotify:
		//do nothing
		break;
		case ButtonRelease:
		//trigger a node pick in mainloop, followed by viewpoint transition
		viewer->LookatMode = 2;
	}
	
}
void handle_tick_lookat() {
	X3D_Viewer *viewer;
	ttglobal tg;
	ppViewer p;
	tg = gglobal();
	p = (ppViewer)tg->Viewer.prv;
	//stub in case we need the viewer or viewpoint transition here	
	viewer = Viewer();
	switch(viewer->LookatMode){
		case 0: //not in use
		case 1: //someone set viewer to lookat mode: mainloop shuts off sensitive, turns on lookat cursor
		case 2: //mouseup tells mainloop to pick a node at current mousexy, turn off lookatcursor
		case 3: //mainloop picked a node, now transition
		case 4: //transition complete, restore previous nav type
		break;
	}
}

void handle_explore(const int mev, const unsigned int button, float x, float y) {
	/*
	Like handle_spherical, except:
	move the viewer->Pos in the opposite direction from where we are looking
	*/
	int ctrl;
	X3D_Viewer_Spherical *ypz;
	X3D_Viewer *viewer;
	ppViewer p;
	ttglobal tg = gglobal();
	p = (ppViewer)gglobal()->Viewer.prv;
	viewer = Viewer();
	ypz = &viewer->ypz; //just a place to store last mouse xy during drag
	ctrl = tg->Mainloop.CTRL;


	if(ctrl) {
		//we're in pick mode - we'll re-use some lookat code
		handle_lookat(mev,button,x,y);
		return;
	}
	if(APPROX(viewer->Dist,0.0)){
		//no pivot point yet
		handle_spherical(mev,button,x,y);
		return;
	}
	handle_turntable(mev, button, x, y);
}

void handle_tplane(const int mev, const unsigned int button, float x, float y) {
	/* handle_walk with 3button mouse, RMB, can do X,Y in plane, but not rotation
	   for touch screen with one finger, we want a nav mode called InPlane to 
	   do the X,Y shifts and rotation in the plane of the camera screen 
	   (about camera-axis/Z)
	*/
	X3D_Viewer *viewer;
	X3D_Viewer_InPlane *inplane;
	//double frameRateAdjustment;//,xx,yy;
	//struct point_XYZ xyz;
	ppViewer p;
	//ttglobal tg = gglobal();
	p = (ppViewer)gglobal()->Viewer.prv;
	viewer = Viewer();
	inplane = &viewer->inplane;

	//if( tg->Mainloop.BrowserFPS > 0)
	//	frameRateAdjustment = 20.0 / tg->Mainloop.BrowserFPS; /* lets say 20FPS is our speed benchmark for developing tuning parameters */
	//else
	//	frameRateAdjustment = 1.0;

	if (mev == ButtonPress) {
		inplane->x = x; //x;
		inplane->y = y; //y;
		inplane->on = 1;
	} else if (mev == MotionNotify) {
		inplane->xx =  x; //.15 * xsign_quadratic(x - inplane->x,5.0,10.0,0.0)*viewer->speed * frameRateAdjustment;
		inplane->yy =  y; //-.15f * xsign_quadratic(y - inplane->y,5.0,10.0,0.0)*viewer->speed * frameRateAdjustment;
 	} else if(mev == ButtonRelease){
		inplane->xx = 0.0f;
		inplane->yy = 0.0f;
		inplane->on = 0;
	}
}
void handle_tick_tplane(double dtime){
	X3D_Viewer *viewer;
	X3D_Viewer_InPlane *inplane;
	//Quaternion quatr, quatt, quat;
	struct point_XYZ pp;
	ttglobal tg;
	ppViewer p;
	tg = gglobal();
	p = (ppViewer)tg->Viewer.prv;
	viewer = Viewer();

	inplane = &viewer->inplane;
	if(inplane->on){
		pp.x =  xsign_quadratic(inplane->xx - inplane->x,300.0,100.0,0.0) *dtime;
		pp.y =  xsign_quadratic(inplane->yy - inplane->y,300.0,100.0,0.0) *dtime;
		pp.z = 0.0;
		//vecadd(&viewer->Pos,&viewer->Pos,&pp);
		increment_pos(&pp);
	}
}

void handle_rtplane(const int mev, const unsigned int button, float x, float y) {
	/* handle_walk with 3button mouse, RMB, can do X,Y in plane, but not rotation
	   for touch screen with one finger, we want a nav mode called InPlane to 
	   do the X,Y shifts and rotation in the plane of the camera screen 
	   (about camera-axis/Z)
	*/
	X3D_Viewer *viewer;
	X3D_Viewer_InPlane *inplane;
	Quaternion nq, q_v;
	double xx,yy, frameRateAdjustment;
	ppViewer p;
	ttglobal tg = gglobal();
	p = (ppViewer)tg->Viewer.prv;
	viewer = Viewer();
	inplane = &viewer->inplane;

	if( tg->Mainloop.BrowserFPS > 0)
		frameRateAdjustment = 20.0 / tg->Mainloop.BrowserFPS; /* lets say 20FPS is our speed benchmark for developing tuning parameters */
	else
		frameRateAdjustment = 1.0;

	if (mev == ButtonPress) {
		inplane->x = x; 
		inplane->y = y; 
	} else if (mev == MotionNotify) {
		if(0){
			//static drag
			double drot = atan2(yy,xx) - atan2(inplane->y,inplane->x); 
			//printf("y=%lf x=%lf inplane-y=%lf inplanex=%lf\n",yy,xx,inplane->y,inplane->x);
			quaternion_set(&q_v, &(viewer->Quat));
			vrmlrot_to_quaternion(&nq, 0.0, 0.0, 1.0, drot);
			quaternion_multiply(&(viewer->Quat), &nq, &q_v);
			inplane->x = xx;
			inplane->y = yy;
			//CALCULATE_EXAMINE_DISTANCE
		}
		if(1){
			//handle_tick quadratic drag
			inplane->xx = xsign_quadratic(x - inplane->x,0.1,0.5,0.0)*frameRateAdjustment;
			inplane->yy = xsign_quadratic(y - inplane->y,0.1,0.5,0.0)*frameRateAdjustment;
			}

	} else if (mev == ButtonRelease) {
		if (button == 1) {
			inplane->xx = 0.0f;
			inplane->yy = 0.0f;
		}
 	}
}

void handle_tick_rplane(double dtime){
	X3D_Viewer *viewer;
	X3D_Viewer_InPlane *inplane;
	Quaternion quatr;
	//struct point_XYZ pp;
	double roll;
	ttglobal tg;
	ppViewer p;
	tg = gglobal();
	p = (ppViewer)tg->Viewer.prv;
	viewer = Viewer();

	inplane = &viewer->inplane;
	if(inplane->on){
		roll = xsign_quadratic(inplane->xx - inplane->x,2.0,2.0,0.0)*dtime;
		vrmlrot_to_quaternion (&quatr,0.0,0.0,1.0,roll); //roll about z axis
		quaternion_multiply(&(viewer->Quat), &quatr,  &(viewer->Quat)); 
		quaternion_normalize(&(viewer->Quat));
	}

}
void handle_tick_tilt(double dtime) {
	X3D_Viewer *viewer;
	X3D_Viewer_InPlane *inplane;
	Quaternion quatt;
	//struct point_XYZ pp;
	double yaw, pitch;
	ttglobal tg;
	ppViewer p;
	tg = gglobal();
	p = (ppViewer)tg->Viewer.prv;
	viewer = Viewer();

	inplane = &viewer->inplane;
	if(inplane->on){
		yaw = xsign_quadratic(inplane->xx - inplane->x,2.0,2.0,0.0)*dtime;
		vrmlrot_to_quaternion (&quatt,0.0,1.0,0.0,yaw); //tilt about x axis
		quaternion_multiply(&(viewer->Quat), &quatt, &(viewer->Quat)); 
		pitch = -xsign_quadratic(inplane->yy - inplane->y,2.0,2.0,0.0)*dtime;
		vrmlrot_to_quaternion (&quatt,1.0,0.0,0.0,pitch); //tilt about x axis
		quaternion_multiply(&(viewer->Quat), &quatt, &(viewer->Quat)); 
		quaternion_normalize(&(viewer->Quat));
	}
}

/************************************************************************************/


void handle0(const int mev, const unsigned int button, const float x, const float yup)
{
	X3D_Viewer *viewer;
	ppViewer p = (ppViewer)gglobal()->Viewer.prv;
	viewer = Viewer();
	/* ConsoleMessage("Viewer handle: viewer_type %s, mouse event %d, button %u, x %f, y %f\n", 
	   lookup_navmodestring(viewer->type), mev, button, x, yup); */

	if (button == 2) {
		return;
	}
	switch(viewer->type) {
	case VIEWER_NONE:
		break;
	case VIEWER_EXAMINE:
		handle_examine(mev, button, ((float) x), ((float) yup));
		break;
	case VIEWER_WALK:
		handle_walk(mev, button, ((float) x), ((float) yup));
		break;
	case VIEWER_EXFLY:
		break;
	case VIEWER_FLY:
		handle_fly2(mev, button, ((float) x), ((float) yup)); //feature-Navigation_key_and_drag
		break;
	case VIEWER_FLY2:
		handle_fly2(mev,button,((float) x),((float)yup)); 
		break;
	case VIEWER_TILT:
	case VIEWER_RPLANE:
		handle_rtplane(mev,button,((float) x),((float)yup)); //roll, tilt: one uses x, one uses y - separate handle_ticks though
		break;
	case VIEWER_TPLANE:
		handle_tplane(mev,button,((float) x),((float)yup)); //translation in the viewer plane
		break;
	case VIEWER_SPHERICAL:
		handle_spherical(mev,button,((float) x),((float)yup)); //spherical panorama
		break;
	case VIEWER_TURNTABLE:
		handle_turntable(mev, button, ((float)x), ((float)yup));  //examine without roll around world 0,0,0 origin - like a 3D editor with authoring plane
		break;
	case VIEWER_LOOKAT:
		handle_lookat(mev, button, ((float)x), ((float)yup)); //as per navigationInfo specs, you toggle on, then click an object and it flys you there
		break;
	case VIEWER_EXPLORE:
		handle_explore(mev, button, ((float)x), ((float)yup)); //as per specs, like turntable around any point you pick with CTRL click
		break;
	case VIEWER_DIST:
		handle_dist(mev,button,(float)x,(float)yup);
	default:
		break;
	}
}

#define FLYREMAP {{'a',NUM0},{'z',NUMDEC},{'j',LEFT_KEY},{'l',RIGHT_KEY},{'p',UP_KEY},{';',DOWN_KEY},{'8',NUM8},{'k',NUM2},{'u',NUM4},{'o',NUM6 },{'7',NUM7},{'9',NUM9}}

//BEGIN dug9 Feb2015 >>>
//GOAL for this refactoring: CHORD mappings of arrow keys for keyboard navigation and mouse xy drags for FLY navigation
// and keeping in mind mobile devices may not want a keyboard on the screen, but they may have 4 arrow keys
// - and -if no change to UI menus/no use of chords by user- the default behaviour is what we do now
Key FLYREMAP2 [] = {{'a',NUM0},{'z',NUMDEC},{'j',LEFT_KEY},{'l',RIGHT_KEY},{'p',UP_KEY},{';',DOWN_KEY},{'8',NUM8},{'k',NUM2},{'u',NUM4},{'o',NUM6 },{'7',NUM7},{'9',NUM9}};
int FLYREMAP2SIZE = 12;
Key FLYCHORDREMAP [] = {
{'j',LEFT_KEY},{'l',RIGHT_KEY},{'p',UP_KEY},{';',DOWN_KEY}
};
int arrowkeys [] = {LEFT_KEY,RIGHT_KEY,UP_KEY,DOWN_KEY};
//int isArrowkey(int key){
//	int iret, i;
//	iret = 0;
//	for(i=0;i<4;i++) 
//		if(key == arrowkeys[i]) iret = 1;
//	return iret;
//}
int indexArrowkey(int key){
	int iret, i;
	iret = -1;
	for(i=0;i<4;i++) 
		if(key == arrowkeys[i]) iret = i;
	return iret;
}

//movements of the camera (with respect to the scene)
enum {
	FLY_X_LEFT,
	FLY_X_RIGHT,
	FLY_Y_DOWN,
	FLY_Y_UP,
	FLY_Z_FORWARD,
	FLY_Z_REVERSE,
	FLY_PITCH_UP,
	FLY_PITCH_DOWN,
	FLY_YAW_LEFT,
	FLY_YAW_RIGHT,
	FLY_ROLL_COUNTERCLOCKWISE,
	FLY_ROLL_CLOCKWISE,
} fly_key_command;
Key fly_normalkeys [] = {
	{'j',FLY_X_LEFT},
	{'l',FLY_X_RIGHT},
	{';',FLY_Y_DOWN},
	{'p',FLY_Y_UP},
	{'a',FLY_Z_FORWARD},
	{'z',FLY_Z_REVERSE},
	{'k',FLY_PITCH_UP},
	{'8',FLY_PITCH_DOWN},
	{'u',FLY_YAW_LEFT},
	{'o',FLY_YAW_RIGHT},
	{'7',FLY_ROLL_COUNTERCLOCKWISE},
	{'9',FLY_ROLL_CLOCKWISE},
};

//enum {
//	CHORD_YAWZ,
//	CHORD_YAWPITCH,
//	CHORD_ROLL,
//	CHORD_XY
//} input_chords;
char *chordnames [] = {"YAWZ","YAWPITCH","ROLL","XY"};
//the flychord table is bloated with redundancies, but explicit. FLYCHORDMAP2 int[4][4] would be briefer, but harder to trace.
typedef struct flychord {
	int chord;
	Key arrows[4];
} flychord;
flychord FLYCHORDREMAP2 [] = {
	{CHORD_YAWZ,	{{FLY_YAW_LEFT,LEFT_KEY},{FLY_YAW_RIGHT,RIGHT_KEY},{FLY_Z_FORWARD,UP_KEY},{FLY_Z_REVERSE,DOWN_KEY}}},
	{CHORD_YAWPITCH,{{FLY_YAW_LEFT,LEFT_KEY},{FLY_YAW_RIGHT,RIGHT_KEY},{FLY_PITCH_UP,UP_KEY},{FLY_PITCH_DOWN,DOWN_KEY}}},
	{CHORD_ROLL,	{{FLY_ROLL_COUNTERCLOCKWISE,LEFT_KEY},{FLY_ROLL_CLOCKWISE,RIGHT_KEY},{FLY_ROLL_COUNTERCLOCKWISE,UP_KEY},{FLY_ROLL_CLOCKWISE,DOWN_KEY}}},
	{CHORD_XY,		{{FLY_X_LEFT,LEFT_KEY},{FLY_X_RIGHT,RIGHT_KEY},{FLY_Y_UP,UP_KEY},{FLY_Y_DOWN,DOWN_KEY}}},
};

int viewer_getKeyChord(){
	ppViewer p = (ppViewer)gglobal()->Viewer.prv;
	return p->keychord;
}
void viewer_setKeyChord(int chord){
	int chord1;
	ppViewer p = (ppViewer)gglobal()->Viewer.prv;
	chord1 = chord;
	if(chord1 > 3) chord1 = 0;
	if(chord1 < 0) chord1 = 3;
	p->keychord = chord1;
}
char *fwl_getKeyChord(){
	return chordnames[viewer_getKeyChord()];
}

int fwl_setKeyChord(char *chordname){
	int i, ok;
	ok = FALSE;
	for(i=0;i<4;i++){
		if(!strcasecmp(chordname,chordnames[i])){
			viewer_setKeyChord(i); //or should I expand from i to CHORD_YAWZ etc
			ok = TRUE;
			break;
		}
	}
	return ok;
}
int viewer_getDragChord(){
	ppViewer p = (ppViewer)gglobal()->Viewer.prv;
	return p->dragchord;
}
void viewer_setDragChord(int chord){
	ppViewer p = (ppViewer)gglobal()->Viewer.prv;
	p->dragchord = chord;
}
char *fwl_getDragChord(){
	return chordnames[viewer_getDragChord()];
}
int fwl_setDragChord(char *chordname){
	int i, ok;
	ok = FALSE;
	for(i=0;i<4;i++){
		if(!strcasecmp(chordname,chordnames[i])){
			viewer_setDragChord(i); //or should I expand from i to CHORD_YAWZ etc
			ok = TRUE;
			break;
		}
	}
	return ok;
}

//next: in lookup_fly_key we would check if its an arrow key, and if so, use the current keychord to lookup the keyfly command.
// from that we would look up the normal key
int lookup_fly_arrow(int key){
	//check if this is an arrow key. If so lookup in the current chord to get the motion command
	//and from motion command lookup the 'normal' equivalent key
	ppViewer p = (ppViewer)gglobal()->Viewer.prv;
	int idxarrow, idxnormal;
	int iret = 0;
	idxarrow = indexArrowkey(key);
	if(idxarrow > -1){
		//rather than 2 nested loops, comparing, we will trust the ordering and index in
		idxnormal = FLYCHORDREMAP2[p->keychord].arrows[idxarrow].key;
		//same here - we'll trust the order and index in
		iret = fly_normalkeys[idxnormal].key;
	}
	return iret;
}
//<<< END dug9 Feb2015
char lookup_fly_extended(int key){
	int i;
	char kp = 0;
	Key ps[KEYS_HANDLED] = FLYREMAP;
	for(i=0;i<KEYS_HANDLED;i++){
		if(key==ps[i].hit){
			kp = ps[i].key;
			break;
		}
	}
	return kp;
}
char lookup_fly_key(int key){
	//check for special/extended characters related to fly mode, such as numpad and arrow keys
	char kp = 0;
	kp = lookup_fly_arrow(key); //check arrow keys first
	if(!kp)
		kp = lookup_fly_extended(key); //else other extended characters
	return kp;
}
static struct flykey_lookup_type {
	char key;
	int motion; //translation 0, rotation 1
	int axis; //0=x,1=y,2=z
	int sign; //-1 left 1 right
	int command;
} flykey_lookup [] = {
	{'j', 0, 0, -1, FLY_X_LEFT},
	{'l', 0, 0,  1, FLY_X_RIGHT},
	{';', 0, 1, -1, FLY_Y_DOWN},
	{'p', 0, 1,  1, FLY_Y_UP,},
	{'a', 0, 2, -1, FLY_Z_FORWARD},
	{'z', 0, 2,  1, FLY_Z_REVERSE},

	{'k', 1, 0, -1, FLY_YAW_LEFT},
	{'8', 1, 0,  1, FLY_YAW_RIGHT},
	{'u', 1, 1, -1, FLY_PITCH_UP},
	{'o', 1, 1,  1, FLY_PITCH_DOWN},
	{'7', 1, 2, -1, FLY_ROLL_COUNTERCLOCKWISE},
	{'9', 1, 2,  1, FLY_ROLL_CLOCKWISE}
};
	

struct flykey_lookup_type *getFlyIndex(char key){
	struct flykey_lookup_type *flykey;
	int index = -1;
	flykey = NULL;
	for(index=0;index<KEYS_HANDLED;index++){
		if(key == flykey_lookup[index].key ) break;
	}
	if(index > -1)
		flykey = &flykey_lookup[index];
	return flykey;
}
int isFlyKey(char key){
	int i, index = -1;
	index = indexArrowkey(key);
	if(index == -1)
	for(i=0;i<KEYS_HANDLED;i++)
		if(key == flykey_lookup[i].key ){
			index = i;
			break;
		}
	return index > -1 ? 1 : 0;
}
void handle_key(const char key, double keytime)
{
	char _key;
	//int i;
	X3D_Viewer *viewer;
	X3D_Viewer_Fly *fly; 
	struct flykey_lookup_type *flykey;
	ppViewer p = (ppViewer)gglobal()->Viewer.prv;
	viewer = Viewer();

	fly = &viewer->fly;
	//printf("%c",key);
	//if (viewer->type == VIEWER_FLY) {   //Navigation-key_and_drag
		/* $key = lc $key; */
		_key = (char) tolower((int) key);
		if(!isFlyKey(_key)){
			//printf("not fly key\n");
			return;
		}
		//printf("is flykey\n");
		flykey = getFlyIndex(_key);
		if(flykey){
			if(flykey->motion > -1 && flykey->motion < 2 && flykey->axis > -1 && flykey->axis < 3){
				fly->down[flykey->motion][flykey->axis].direction = flykey->sign;
				fly->down[flykey->motion][flykey->axis].epoch = keytime; //initial keydown
				fly->down[flykey->motion][flykey->axis].era = keytime;  //will decrement as we apply velocity in fly
				fly->down[flykey->motion][flykey->axis].once = 1;
			}
		}
	//} //Navigation-key_and_drag
}


void handle_keyrelease(const char key, double keytime)
{
	char _key;
	//int i;
	X3D_Viewer *viewer;
	X3D_Viewer_Fly *fly;
	struct flykey_lookup_type *flykey;
	ppViewer p = (ppViewer)gglobal()->Viewer.prv;
	viewer = Viewer();
	/* my($this,$time,$key) = @_; */

	fly = &viewer->fly;

	//if (viewer->type == VIEWER_FLY) { //Navigation-key_and_drag
		/* $key = lc $key; */
		_key = (char) tolower((int) key);
		if(!isFlyKey(_key)) return;
		flykey = getFlyIndex(_key);
		if(flykey){
			if(flykey->motion > -1 && flykey->motion < 2 && flykey->axis > -1 && flykey->axis < 3){
				int *ndown = &fly->ndown[flykey->motion][flykey->axis];
				if((*ndown) < 10){
					//up to 20 key chirps per axis are stored, with their elapsed time down measured in the keyboard's thread
					fly->wasDown[flykey->motion][flykey->axis][*ndown].direction = fly->down[flykey->motion][flykey->axis].direction;
					fly->wasDown[flykey->motion][flykey->axis][*ndown].epoch = keytime - fly->down[flykey->motion][flykey->axis].epoch; //total pressedTime
					fly->wasDown[flykey->motion][flykey->axis][*ndown].era = keytime - fly->down[flykey->motion][flykey->axis].era; //unused keydown time
					fly->wasDown[flykey->motion][flykey->axis][*ndown].once = fly->down[flykey->motion][flykey->axis].once;  //a flag for the handle_tick to play with
					(*ndown)++;
				}
				fly->down[flykey->motion][flykey->axis].direction = 0;
			}
		}
	//} //Navigation-key_and_drag
}

/* wall penetration detection variables
   lastP - last avatar position, relative to current avatar position at 0,0,0 in avatar space
		- is a sum of walk_tick and collision displacement increment_pos()
   lastQ - quaternion increment from walk_tick which applies to previous lastP:
         if current frame number is i, and lastP is from i-1, then lastQ applies to i-1 lastP
*/
//struct point_XYZ viewer_lastP;
void viewer_lastP_clear()
{
	ppViewer p = (ppViewer)gglobal()->Viewer.prv;

	p->viewer_lastP.x = p->viewer_lastP.y = p->viewer_lastP.z = 0.0;
}
void viewer_lastQ_set(Quaternion *lastQ)
{
	ppViewer p = (ppViewer)gglobal()->Viewer.prv;
	quaternion_rotation(&p->viewer_lastP,lastQ,&p->viewer_lastP); 
}
void viewer_lastP_add(struct point_XYZ *vec) 
{
	ppViewer p = (ppViewer)gglobal()->Viewer.prv;
	if(get_collision()) /* fw_params.collision use if(1) to test with toggling_collision */
	{
		VECADD(p->viewer_lastP,*vec);
	}
	else
		viewer_lastP_clear();
}

struct point_XYZ viewer_get_lastP()
{ 
	/* returns a vector from avatar to the last avatar location ie on the last loop, in avatar space */
	ppViewer p = (ppViewer)gglobal()->Viewer.prv;

	struct point_XYZ nv = p->viewer_lastP;
	vecscale(&nv,&nv,-1.0); 
	return nv; 
}



/*
 * handle_tick_walk: called once per frame.
 *
 * Sets viewer to next expected position.
 * This should be called before position sensor calculations
 * (and event triggering) take place.
 * Position dictated by this routine is NOT final, and is likely to
 * change if the viewer is left in a state of collision. (ncoder)
 * according to web3d specs, the gravity vector is determined by 
 * the currently bound viewpoint vertical CBVV
 * walk.xd,zd are in a plane parallel to the CBV horizon.
 * walk.yd is vertical in the CBVV direction
 * walk.rd is an angle in the CBVV horizontal plane (around vertical axis parallel to the CBVV)
 */

static void handle_tick_walk()
{
	X3D_Viewer *viewer;
	X3D_Viewer_Walk *walk; 
	Quaternion q, nq;
	struct point_XYZ pp;
	ppViewer p = (ppViewer)gglobal()->Viewer.prv;
	viewer = Viewer();
	walk = &viewer->walk;

	//for normal walking with left button down, only walk->ZD and walk->RD are non-zero
	pp.x = 0.15 * walk->XD;
	pp.y = 0.15 * walk->YD;
	pp.z = 0.15 * walk->ZD;
	///  see below //increment_pos(&pp);

	/* walk mode transforms: (dug9 July 15, 2011)
	0.World Coordinates
	-- transform stack
	---- 1.viewpoint node - currently bound viewpoint (CBV) gravity direction vector determined here
	------ .position/(.Pos during navigation)
	-------- 2.#avatar body proposed - collisions, gravity and wall penetration can be computed here and += to .Pos
	---------- .orientation/(.Quat during navigation) horizontal/pan part (this *= walk.RD)
	------------ 3.(walk->ZD in these coords, and must be transformed by inverse(.Quat) into .Pos delta)
	------------ 3.#avatar body current (BVVA) - collisions, gravity and wall penetration computed here and += to .Pos
	-------------- .orientation/.Quat tilts part (up/down and camera z axis tilts)^
	---------------- 4.avatar camera
	^There's no way for the user to tilt in walk mode. To tilt:
	a) switch to Fly, tilt with keyboard commands, then switch back to walk,
	b) script against viewpoint.orientation, or
	c) put non-zero viewpoint orientation in the scene file
	# since 2009 the walk avatar collisions,gravity,wall-pen have been done in what has been 
	  called avatar space or BVVA bound viewpoint vertical avatar - same as avatar camera
	  with tilts removed, but pan applied, so same space as walk->ZD is applied above
	  However because the avatar collision volume is symmetric around the vertical axis,
	  it doesn't have to pan-rotate with the avatar to do its job, so it could be done 
	  in .position space, with a few code touch ups. This would also still work in Fly mode 
	  which has a spherically symmetric collision volume.

	fly mode transforms:
	- simpler - you point, and fly in the direction you pointed, spherical collision volume:
	0.World
		1. viewpoint
			2. avatar position .Pos
				3. avatar orientation .Quat
					(collisions currently done here), input device XY mapped to XYZ motion here
	Notice the order of transforms is the same for Fly mode:
		.Pos += inverse(.Quat)*inputXYZ - see increment_pos()

	(dug9 May 2015) in a bit more detail:
	Shape
	Model part of ModelView transform
	(world coordinates)
	View part of ModelView transform
	viewpoint
	viewpoint.position
	viewpoint.rotation
	opengl camera

	*/

	q.w = (viewer->Quat).w;
	q.x = (viewer->Quat).x;
	q.y = (viewer->Quat).y;
	q.z = (viewer->Quat).z;
	vrmlrot_to_quaternion (&nq,0.0,1.0,0.0,0.4*walk->RD);
	//quaternion_to_vrmlrot(&nq,&ff[0],&ff[1],&ff[2],&ff[3]);
	//if(walk->RD != 0.0)
	//	printf("\n");
	viewer_lastQ_set(&nq); //wall penetration - last avatar pose is stored before updating
	//split .Quat into horizontal pan and 2 tilts, then:
	// .Quat = .Quat * walk->RD (if I reverse the order, the tilts don't rotate with the avatar)
	// .Pos += inverse(planar_part(.Quat)) * walk->ZD
	//this should rotate the tilts with the avatar
	quaternion_multiply(&(viewer->Quat), &q, &nq); //Quat = walk->RD * Quat
	quaternion_normalize(&(viewer->Quat));
	{
		double angle;
		struct point_XYZ tilted;
		struct point_XYZ rotaxis = {0.0, 1.0, 0.0};
		Quaternion qlevel,qplanar;
		struct point_XYZ down = {0.0, -1.0, 0.0};

		//split .Quat into horizontal pan and 2 vertical tilts
		quaternion_rotation(&tilted,&q,&down);
		angle = vecangle2(&down,&tilted, &rotaxis);
		vrmlrot_to_quaternion (&qlevel,rotaxis.x,rotaxis.y,rotaxis.z,-angle);

 		quaternion_multiply(&qplanar,&qlevel,&q);
		//quaternion_to_vrmlrot(&qplanar,&aa[0],&aa[1],&aa[2],&aa[3]);

		//use resulting horizontal pan quat to transform walk->Z
		{
			//from increment_pos()
			struct point_XYZ nv;
			struct point_XYZ vec;
			Quaternion q_i;
			//ppViewer p = (ppViewer)gglobal()->Viewer.prv;
			vec.x = pp.x;
			vec.y = pp.y;
			vec.z = pp.z;
			viewer_lastP_add(&vec); //wall penetration - last avatar pose is stored before updating

			/* bound-viewpoint-space > Viewer.Pos,Viewer.Quat > avatar-space */
			//quaternion_inverse(&q_i, &(viewer->Quat));  //<<increment_pos(vec)
			quaternion_inverse(&q_i, &qplanar); //<< I need this in increment_pos
			quaternion_rotation(&nv, &q_i, &vec);

			/* save velocity calculations for this mode; used for EAI calls only */
			viewer->VPvelocity.x = nv.x; viewer->VPvelocity.y = nv.y; viewer->VPvelocity.z = nv.z;
			/* and, act on this change of location. */
			viewer->Pos.x += nv.x;  /* Viewer.Pos must be in bound-viewpoint space */
			viewer->Pos.y += nv.y; 
			viewer->Pos.z += nv.z;
			

			/* printf ("increment_pos; oldpos %4.2f %4.2f %4.2f, anti %4.2f %4.2f %4.2f nv %4.2f %4.2f %4.2f \n",
				Viewer.Pos.x, Viewer.Pos.y, Viewer.Pos.z, 
				Viewer.AntiPos.x, Viewer.AntiPos.y, Viewer.AntiPos.z, 
				nv.x, nv.y, nv.z); */
		}

	}

	/* make sure Viewer.Dist is configured properly for Examine mode */
	//CALCULATE_EXAMINE_DISTANCE
}

//an external program or app may want to set or get the viewer pose, with no slerping
//SSR - these set/getpose are called from _DisplayThread
static int negate_pos = TRUE;
void viewer_setpose( double *quat4, double *vec3){
	/* sign change on pos, but not quat, because freewrl conventions are different
		+Quat goes in direction world2vp
		-Pos goes in direction world2vp
	*/
	X3D_Viewer *viewer;
	double vec[3];
	ttglobal tg = (ttglobal) gglobal();
	ppViewer p = (ppViewer)tg->Viewer.prv;
	viewer = Viewer();
	veccopyd(vec,vec3);
	if(negate_pos) vecnegated(vec,vec);
	double2pointxyz(&viewer->Pos,vec);
	double2quat(&viewer->Quat,quat4);
}
void viewer_getpose( double *quat4, double *vec3){
	/*	Freewrl initializes .Quat, .Pos from viewpoint.position, viewpoint.orientation during viewpoint binding
			(or gives a default if no bound viewpoint)
		Viewer.Quat = inverse(vp.orientation) //changes sense from x3d vp2world, to opengl sense world2vp
		Viewer.Pos = vp.position //remains in x3d sense vp2world
	*/
	X3D_Viewer *viewer;
	ttglobal tg = (ttglobal) gglobal();
	ppViewer p = (ppViewer)tg->Viewer.prv;
	viewer = Viewer();
	pointxyz2double(vec3,&viewer->Pos);
	if(negate_pos)
		vecnegated(vec3,vec3);
	quat2double(quat4,&viewer->Quat);
}
void viewer_getbindpose( double *quat4, double *vec3){
/*	The bind-time-equivalent viewpoint pose can be got 
	from the Anti variables intialized by INITIATE_POSITION_ANTIPOSITION macro
	which copies the .position, .orientation values from the viewpoint node fields
	(if a viewpoint is bound, otherwise defaults are set during startup)
*/
	X3D_Viewer *viewer;
	Quaternion q_i;
	ttglobal tg = (ttglobal) gglobal();
	ppViewer p = (ppViewer)tg->Viewer.prv;
	viewer = Viewer();
	pointxyz2double(vec3,&viewer->AntiPos); //.Pos
	if(negate_pos)
		vecnegated(vec3,vec3);
	quaternion_inverse(&q_i,&viewer->AntiQuat);
	quat2double(quat4,&q_i);
}
void viewer_getview( double *viewMatrix){
	/* world - View - Viewpoint - .position - .orientation */
	//view matrix includes Transform(s) * viewpoint.position * viewpoint.orientation
	//we need to separate the Transforms from the .position and .orientation
	//double vec3[3], quat4[4];
	FW_GL_GETDOUBLEV(GL_MODELVIEW_MATRIX, viewMatrix);
	//viewer_getpose(quat4,vec3);
	//viewMatrix *= inv_quat4
	//viewMatrix *= inv_vec3
}
void viewer_setview( double *viewMatrix){
	FW_GL_SETDOUBLEV(GL_MODELVIEW_MATRIX, viewMatrix);
}

/* formerly package VRML::Viewer::ExFly
 * entered via the "f" key.
 *
 * External input for x,y,z and quat. Reads in file
 * /tmp/inpdev (macro IN_FILE), which is a single line file that is
 * updated by some external program.
 *
 * eg:
 *    9.67    -1.89    -1.00  0.99923 -0.00219  0.01459  0.03640
 *
 * Do nothing for the mouse.
 */

/* my $in_file = "/tmp/inpdev"; */
/* #JAS my $in_file_date = stat($in_file)->mtime; */
/* my $string = ""; */
/* my $inc = 0; */
/* my $inf = 0; */
//#ifdef _MSC_VER
//static int exflyMethod = 1;  /* could be a user settable option, which kind of exfly to do */
//#else
//static int exflyMethod = 0;
//#endif
static void
handle_tick_exfly()
{
	X3D_Viewer *viewer;
	size_t len = 0;
	char string[STRING_SIZE];
	float px,py,pz,q1,q2,q3,q4;
	size_t rv; /* unused, but here for compile warnings */
	ppViewer p = (ppViewer)gglobal()->Viewer.prv;
	viewer = Viewer();

	UNUSED(rv);  // mitigate compiler warnings

	memset(string, 0, STRING_SIZE * sizeof(char));

	/*
	 * my $chk_file_date = stat($in_file)->mtime;
	 * following uncommented as time on file only change
	 * once per second - should change this...
     *
	 * $in_file_date = $chk_file_date;
	 */

/* 	sysopen ($inf, $in_file, O_RDONLY) or  */
/* 		die "Error reading external sensor input file $in_file\n"; */
/* 	$inc = sysread ($inf, $string, 100); */
/* 	close $inf; */
	if ((p->exfly_in_file = fopen(IN_FILE, "r")) == NULL) {
		fprintf(stderr,
				"Viewer handle_tick_exfly: could not open %s for read, returning to EXAMINE mode.\nSee the FreeWRL man page for further details on the usage of Fly - External Sensor input mode.\n",
				IN_FILE);

		/* allow the user to continue in default Viewer mode */
		viewer->type = VIEWER_EXAMINE;
		//setMenuButton_navModes(viewer->type);
		return;
	}
	rv = fread(string, sizeof(char), IN_FILE_BYTES, p->exfly_in_file);
	if (ferror(p->exfly_in_file)) {
		fprintf(stderr,
				"Viewer handle_tick_exfly: error reading from file %s.",
				IN_FILE);
		fclose(p->exfly_in_file);
		return;
	}
	fclose(p->exfly_in_file);

/* 	if (length($string)>0) */
	if ((len = strlen(string)) > 0) {
		if(p->exflyMethod == 0)
		{
			//MUFTI input data
			len = sscanf (string, "%f %f %f %f %f %f %f",&px,&py,&pz,
				&q1,&q2,&q3,&q4);

			/* read error? */
			if (len != 7) return;

			(viewer->Pos).x = px;
			(viewer->Pos).y = py;
			(viewer->Pos).z = pz;

			(viewer->Quat).w = q1;
			(viewer->Quat).x = q2;
			(viewer->Quat).y = q3;
			(viewer->Quat).z = q4;
		}else if(p->exflyMethod == 1){
			//dug9 WiiMote data written from a C# program
			static int lastbut = 0;
			int mev, but;
			len = sscanf (string, "%d %f %f ",&but,&px,&py);
			if (len != 3) return;
			mev = ButtonRelease;
			if(but) mev = MotionNotify;
			if(but != lastbut)
			{
				mev = (but==1 || but==4)? ButtonPress : ButtonRelease;
			}
			// change raw wii values from ( -1 to 1 ) to (0 - 1.0)
			//px = (px + 1.0)*.5;  //done in wiimote code
			//py = 1.0 - (py + 1.0)*.5;  //done in wiimote code
			handle_walk(mev,but,px,py);
			handle_tick_walk();
			lastbut = but;
		}
	}
}



/* FLY mode change Aug 29, 2014 dug9:
	I was having trouble adjusting speeds on a fast computer - 
	- multiple keystrokes didn't change linear speed
	- angluar speed was too fast.
	New design: 
		Goal: slow and fast frame rates both work
		Linear: if the user presses a key again/mulitple times before the related speed decay finishes, 
			those keystrokes are interpreted as a desire to increase speed.
		Angular: brief taps do small angle adjustments 1/64 of full circle, holding key down does full rotation in 6 seconds
		In both cases, the keydown elapsed time is measured in the keybaord thread, not the display/rendering thread
*/

static void handle_tick_fly()
{
	X3D_Viewer *viewer;
	X3D_Viewer_Fly *fly;
	Quaternion q_v, nq = { 1, 0, 0, 0 };
	struct point_XYZ v;
	double changed = 0.0, time_diff = -1.0;
	int i;
	ppViewer p = (ppViewer)gglobal()->Viewer.prv;
	viewer = Viewer();

	fly = &viewer->fly;

	//sleep(400); //slow frame rate to test frame-rate-dependent actions
	if (fly->lasttime < 0) {
		fly->lasttime = TickTime();
		return;
	} else {
		double dtime = TickTime();
		time_diff = dtime - fly->lasttime;
		if (APPROX(time_diff, 0)) {
			return;
		}
		fly->lasttime = dtime;
		if(time_diff < 0.0) return; //skip a frame if the clock wraps around
	}


	/* has anything changed? if so, then re-render */

	/* linear movement */
	for (i = 0; i < 3; i++) {
		//fade old velocity, using something like exponential decay Ni = N(i-1)*e**(k*t) where k < 0
		if(!fly->down[0][i].direction){
			double dtime = fly->lasttime - fly->down[0][i].epoch; //fly->ttransition[i][0];
			if(dtime > .25) //delay decay, waiting for more speed-indicating keystrokes
				fly->Velocity[0][i] *= pow(0.04, time_diff); 
		}
		//if its almost 0, clamp to zero
		if(fabs(fly->Velocity[0][i]) < .001){
			fly->Velocity[0][i] = 0.0;
		}
		//if key action, add new velocity
		if(fly->down[0][i].direction){ 
			//key is currently down
			fly->Velocity[0][i] += (fly->ndown[0][i]+fly->down[0][i].once)*fly->down[0][i].direction * viewer->speed;
			fly->down[0][i].once = 0;
			fly->ndown[0][i] = 0; //p->translaten[i] = 0; 
			//fly->ttransition[i][0] = fly->lasttime; //save time of last [i] translate, for delaying decay
		}
		changed += fly->Velocity[0][i];
	}

	/* if we do NOT have a GeoViewpoint node, constrain all 3 axis */
	if (viewer->GeoSpatialNode == NULL) 
		if(0) for (i = 0; i < 3; i++) {
			if (fabs(fly->Velocity[0][i]) >9.0) 
				fly->Velocity[0][i] /= (fabs(fly->Velocity[0][i]) /9.0);
		}

	/* angular movement 
		key chirp - a quck press and release on a key
		Velocity - (not velocity)  amount of angle in radians we want to turn on this tick
		era - elapsed time between key down and keyup, as measured in the keyboard thread
			- used to ramp up angular speed based on how long you hold the key down
			- quick chirps on the key will give you smaller 'touch-up' angles
		goal: so it works with both fast frame rate/FPS and slow
			fast: chirps and instant visual feedback on angle turned with key held down
			slow: count your chirps, 64 chirps per full circle/2PI, or hold key down and count seconds 6 seconds = 2PI
	*/
	for (i = 0; i < 3; i++) {
		static double radians_per_second = .6; //seems to turn 2x faster than this
		fly->Velocity[1][i] = 0.0;
		if(!fly->down[1][i].direction){
			fly->Velocity[1][i] *= pow(0.04, time_diff);
		}else{
			//the key is currently being held down, use a bit of it here
			double rps = radians_per_second;
			//double pressedEra = fly->lasttime - fly->down[1][i].epoch; //rEra[i];
			//normally not a chirp, but could be - a chirp here will hardly show, so no harm in double doing chirps here and below
			double era = fly->lasttime - fly->down[1][i].era; //- .25; //save a chirp worth because it gets chirped below when the key comes up
			fly->Velocity[1][i] += era * fly->down[1][i].direction * rps; // * 0.025;
			fly->down[1][i].era += era; //subtract what we just used
			//printf("*");
		}
		if(fly->ndown[1][i]){ 
			//there were some keydowns that happened between ticktimes, add their effects here
			int k;
			double rps = radians_per_second * .33;
			for(k=0; k<fly->ndown[1][i]; k++){
				double era = fly->wasDown[1][i][k].era; //unused keydown time
				double pressedEra = fly->wasDown[1][i][k].epoch; //total pressedTime
				//printf("+%f %f \n",era,pressedTime);
				if(pressedEra <= .1)
					era = .25; //a key chirp. Which can be too fast to measure in keyboard thread, so we give it a consistent down time (era)
				//printf("%d ",fly->wasDown[k][i][1].direction);
				fly->Velocity[1][i] += era * fly->wasDown[1][i][k].direction * rps; // * 0.025;
			}
			fly->ndown[1][i] = 0;
		}
		if (fabs(fly->Velocity[1][i]) > 0.8) {
			fly->Velocity[1][i] /= (fabs(fly->Velocity[1][i]) / 0.8);
		}
		changed += fly->Velocity[1][i];
		/* printf ("avel %d %f\n",i,fly->AVelocity[i]); */
	}

	/* have we done anything here? */
	if (APPROX(changed,0.0)) return;
	v.x = fly->Velocity[0][0] * time_diff;
	v.y = fly->Velocity[0][1] * time_diff;
	v.z = fly->Velocity[0][2] * time_diff;
	increment_pos(&v);

	nq.x = fly->Velocity[1][0];// * time_diff;
	nq.y = fly->Velocity[1][1]; // * time_diff;
	nq.z = fly->Velocity[1][2]; // * time_diff;
	quaternion_normalize(&nq);

	quaternion_set(&q_v, &(viewer->Quat));
	quaternion_multiply(&(viewer->Quat), &nq, &q_v);
	quaternion_normalize(&(viewer->Quat));

	/* make sure Viewer.Dist is configured properly for Examine mode */
	//CALCULATE_EXAMINE_DISTANCE

}

void
handle_tick()
{
	X3D_Viewer *viewer;
	double lasttime, dtime, time_diff;
	ppViewer p = (ppViewer)gglobal()->Viewer.prv;
	viewer = Viewer();
	lasttime = viewer->lasttime;

	time_diff = 0.0; 
	//sleep(400); //slow frame rate to test frame-rate-dependent actions
	if (lasttime < 0) {
		viewer->lasttime = TickTime(); 
		return;
	} else {
		dtime = TickTime();
		time_diff = dtime - viewer->lasttime; //TickTime is computed once per frame, and handle_tick() is called once per frame
		if (APPROX(time_diff, 0)) {
			return;
		}
		viewer->lasttime = dtime;
		if(time_diff < 0.0) return; //skip a frame if the clock wraps around
	}
	 
	switch(viewer->type) {
	case VIEWER_NONE:
		break;
	case VIEWER_EXAMINE:
		break;
	case VIEWER_WALK:
		handle_tick_walk();
		break;
	case VIEWER_EXFLY:
		handle_tick_exfly();
		break;
	case VIEWER_FLY:
		switch(p->dragchord){
			case CHORD_YAWPITCH:
				handle_tick_tilt(time_diff);
				break;
			case CHORD_ROLL:
				handle_tick_rplane(time_diff);
				break;
			case CHORD_XY:
				handle_tick_tplane(time_diff);
				break;
			case CHORD_YAWZ:
			default:
				handle_tick_fly2(time_diff);  //fly2 like (WALK - G) except no RMB PAN, drags aligned to Viewer (vs walk aligned to bound Viewpoint vertical)
				break;
		}
		break;
	case VIEWER_FLY2:
		handle_tick_fly2(time_diff); //yawz
		break;
	case VIEWER_LOOKAT:
		handle_tick_lookat();
		break;
	case VIEWER_TPLANE:
		handle_tick_tplane(dtime);
		break;
	case VIEWER_RPLANE:
		handle_tick_rplane(dtime);
		break;
	case VIEWER_TILT:
		handle_tick_tilt(dtime);
		break;
	case VIEWER_EXPLORE:
		break;
	case VIEWER_SPHERICAL:
		//do nothing special on tick
		break;
	case VIEWER_TURNTABLE:
		break;
	case VIEWER_DIST:
		break;
	default:
		break;
	}
	if(viewer->type != VIEWER_NONE){
		handle_tick_fly(); //Navigation-key_and_drag
	}
	if (viewer->doExamineModeDistanceCalculations) {
		/*
		printf ("handle_tick - doing calculations\n");
		*/
		CALCULATE_EXAMINE_DISTANCE
		resolve_pos();
		p->examineCounter --;

		if (p->examineCounter < 0) {
			viewer->doExamineModeDistanceCalculations = FALSE;
			p->examineCounter = 5;
		}
	}
}



/*
 * Semantics: given a viewpoint and orientation,
 * we take the center to revolve around to be the closest point to origin
 * on the z axis.
 * Changed Feb27 2003 JAS - by fixing $d to 10.0, we make the rotation
 * point to be 10 metres in front of the user.
 */

/* ArcCone from TriD */
void
xy2qua(Quaternion *ret, const double x, const double y)
{
	double _x = x - 0.5, _y = y - 0.5, _z, dist;
	_x *= 2;
	_y *= 2;

	dist = sqrt((_x * _x) + (_y * _y));

	if (dist > 1.0) {
		_x /= dist;
		_y /= dist;
		dist = 1.0;
	}
	_z = 1 - dist;

	ret->w = 0;
	ret->x = _x;
	ret->y = _y;
	ret->z = _z;
	quaternion_normalize(ret);
}



//static GLboolean acMask[2][3]; //anaglyphChannelMask
void setmask(GLboolean *mask,int r, int g, int b)
{
	mask[0] = (GLboolean)r;
	mask[1] = (GLboolean)g;
	mask[2] = (GLboolean)b;
}
void Viewer_anaglyph_setSide(int iside)
{
	ppViewer p = (ppViewer)gglobal()->Viewer.prv;
	/* draw in gray */
	/* and use channel masks */
	GLboolean t = 1;
	glColorMask(p->acMask[iside][0],p->acMask[iside][1],p->acMask[iside][2],t);
}
void Viewer_anaglyph_clearSides()
{
	glColorMask(1,1,1,1);
}
//true static:
static char * RGBACM = "RGBACM";
static int indexRGBACM(int a)
{
	return (int) (strchr(RGBACM,a)-RGBACM);
}
int getAnaglyphPrimarySide(int primary, int iside){
	//primary red=0, green=1, blue=2
	//iside left=0, right=1, neither=2
	ppViewer p = (ppViewer)gglobal()->Viewer.prv;
	return (int)p->acMask[iside][primary];
}

void setAnaglyphPrimarySide(int primary, int iside){
	//primary red=0, green=1, blue=2
	//iside left=0, right=1, neither=2
	//it assumes you are setting it to true, 
	//and turns other sides off the primary automatically
	//the user interface should look like this:
	//R G B
	//*     Left
	//    * Right
	//  *   Neither
	//the neither is side=2, and allows the user to turn a primary off all sides
	int i;
	ppViewer p = (ppViewer)gglobal()->Viewer.prv;
	for(i=0;i<3;i++)
		if(iside == i)
			p->acMask[i][primary] = (GLboolean)1;
		else
			p->acMask[i][primary] = (GLboolean)0;
}
void setAnaglyphSideColor(char val, int iside)
{
	X3D_Viewer *viewer;
	ppViewer p = (ppViewer)gglobal()->Viewer.prv;
	viewer = Viewer();
	viewer->iprog[iside] = indexRGBACM(val);
	if(viewer->iprog[iside] == -1 )
	{
		printf ("warning, command line anaglyph parameter incorrect - was %c need something like RG\n",val);
		viewer->iprog[iside] = iside;
	}
	/* used for anaglyphMethod==2 */
	switch (viewer->iprog[iside]) {
		case 0: //'R':
		   setmask(p->acMask[iside],1,0,0);
		   break;
		case 1: //'G':
		   setmask(p->acMask[iside],0,1,0);
			break;
		case 2: //'B':
		   setmask(p->acMask[iside],0,0,1);
		  break;
		case 3: //'A':
		   setmask(p->acMask[iside],1,1,0);
		  break;
		case 4: //'C':
		   setmask(p->acMask[iside],0,1,1);
		  break;
		case 5://'M':
		   setmask(p->acMask[iside],1,0,1);
		  break;
	}
}
void fwl_set_AnaglyphParameter(const char *optArg) {
/*
  NOTE: "const char" means that you wont modify it in the function :)
 */
	X3D_Viewer *viewer;
	const char* glasses;
	int len;
	ppViewer p = (ppViewer)gglobal()->Viewer.prv;
	viewer = Viewer();

	glasses = optArg;
	len = (int) strlen(optArg);
	if(len !=2 && len != 3)
	{
	  printf ("warning, command line anaglyph parameter incorrect - was %s need something like RC or LRN\n",optArg);
	  glasses ="RC"; len = 2;
	}
	if(len == 2){
		setAnaglyphSideColor(glasses[0],0);
		setAnaglyphSideColor(glasses[1],1);
	}else if(len == 3){
		int i, iside;
		for(i=0;i<3;i++){
			switch(optArg[i]){
				case 'L': iside = 0;break;
				case 'R': iside = 1;break;
				case 'N': iside = 2;break;
				default:
					iside = 2;
			}
			setAnaglyphPrimarySide(i,iside);
		}
	}
	//Viewer.iprog[0] = indexRGBACM(glasses[0]);
	//Viewer.iprog[1] = indexRGBACM(glasses[1]);
	//if(Viewer.iprog[0] == -1 || Viewer.iprog[1] == -1)
	//{
	//	printf ("warning, command line anaglyph parameter incorrect - was %s need something like RG\n",optArg);
	//	Viewer.iprog[0] = 0;
	//	Viewer.iprog[1] = 1;
	//}
	viewer->anaglyph = 1; /*0=none 1=active */
	viewer->shutterGlasses = 0;
	viewer->sidebyside = 0;
	viewer->updown = 0;
	viewer->isStereo = 1;
	setStereoBufferStyle(1);
}
/* shutter glasses, stereo view  from Mufti@rus */
/* handle setting shutter from parameters */
void fwl_init_Shutter (void)
{
	/* if you put --shutter on the command line, you'll come in here twice: 
	  first: from options.c but haveQuadbuffer will == 0 because we haven't init gl yet, so don't know
	  second: post_gl_init - we'll know haveQuadbuffer which might = 1 (if not it goes into flutter mode)
    */
	X3D_Viewer *viewer;
	ppViewer p; 
	ttglobal tg = gglobal();
	p= (ppViewer)tg->Viewer.prv;
	viewer = Viewer();

	tg->display.shutterGlasses = 2;
	viewer->shutterGlasses = 2;
	setStereoBufferStyle(1); 
	if(viewer->haveQuadbuffer)
	{
		tg->display.shutterGlasses = 1; /* platform specific pixelformat/window initialization code should hint PRF_STEREO */
		viewer->shutterGlasses = 1;
		setStereoBufferStyle(0); 
	}
	viewer->isStereo = 1;

}

void fwl_init_SideBySide()
{
	X3D_Viewer *viewer;
	ppViewer p = (ppViewer)gglobal()->Viewer.prv;
	viewer = Viewer();

	setStereoBufferStyle(1); 
	viewer->isStereo = 1;
	viewer->sidebyside = 1;
	viewer->screendist = min(viewer->screendist,.375);
	viewer->stereoParameter = min(viewer->stereoParameter,.01);
}
void fwl_init_UpDown()
{
	X3D_Viewer *viewer;
	ppViewer p = (ppViewer)gglobal()->Viewer.prv;
	viewer = Viewer();

	setStereoBufferStyle(1); 
	viewer->isStereo = 1;
	viewer->updown = 1;
	viewer->screendist = min(viewer->screendist,.375);
	viewer->stereoParameter = min(viewer->stereoParameter,.01);
}

void clear_shader_table();
void setAnaglyph()
{
	X3D_Viewer *viewer;
	ppViewer p = (ppViewer)gglobal()->Viewer.prv;
	viewer = Viewer();

	/* called from post_gl_init and hud/options (option.c calls fwl_set_AnaglyphParameter above) */
	viewer->anaglyph = 1; 
	viewer->isStereo = 1;
	clear_shader_table();
	setStereoBufferStyle(1);
}
void setMono()
{
	X3D_Viewer *viewer;
	ppViewer p; 
	ttglobal tg = gglobal();
	p = (ppViewer)tg->Viewer.prv;
	viewer = Viewer();

	viewer->isStereo = 0;
	if(viewer->anaglyph)
	{
		glColorMask(1,1,1,1);
		clear_shader_table();
	}
	viewer->anaglyph = 0;
	viewer->sidebyside = 0;
	viewer->updown = 0;
	viewer->shutterGlasses = 0;
	tg->display.shutterGlasses = 0;
}

/*
#define VIEWER_STEREO_OFF 0
#define VIEWER_STEREO_SHUTTERGLASSES 1
#define VIEWER_STEREO_SIDEBYSIDE 2
#define VIEWER_STEREO_ANAGLYPH 3
#define VIEWER_STEREO_UPDOWN 4
*/

static void setStereo(int type)
{
	/* type: 0 off  1 shutterglasses 2 sidebyside 3 analgyph */
	/* can only be called after opengl is initialized */
	//initStereoDefaults(); 
	setMono();
	switch(type)
	{
	case VIEWER_STEREO_OFF: {/*setMono()*/;break;}
	case VIEWER_STEREO_SHUTTERGLASSES: {fwl_init_Shutter(); break;}
	case VIEWER_STEREO_SIDEBYSIDE: {fwl_init_SideBySide(); break;}
	case VIEWER_STEREO_UPDOWN: {fwl_init_UpDown(); break;}
	case VIEWER_STEREO_ANAGLYPH: {setAnaglyph(); break;}
	default: break;
	}
}
void toggleOrSetStereo(int type)
{
	/* if user clicks the active stereovision type on a HUD, then it should turn it off - back to mono
	if it's not active, then it should be set active*/
	X3D_Viewer *viewer;
	int curtype, shut;
	ppViewer p = (ppViewer)gglobal()->Viewer.prv;
	viewer = Viewer();

	shut = viewer->shutterGlasses ? 1 : 0;
	curtype = viewer->isStereo*( (shut)*1 + viewer->sidebyside*2 + viewer->anaglyph*3 + viewer->updown*4);
	if(type != curtype) 
		setStereo(type);
	else
		setMono();
}
void fwl_setPickraySide(int ipreferredSide, int either){
	X3D_Viewer *viewer;
	ppViewer p = (ppViewer)gglobal()->Viewer.prv;
	viewer = Viewer();
	viewer->dominantEye = ipreferredSide;
	viewer->eitherDominantEye = either;

}
void fwl_getPickraySide(int *ipreferredSide, int *either){
	X3D_Viewer *viewer;
	ppViewer p = (ppViewer)gglobal()->Viewer.prv;
	viewer = Viewer();
	*ipreferredSide = viewer->dominantEye ;
	*either = viewer->eitherDominantEye;
}
void updateEyehalf()
{
	X3D_Viewer *viewer;
	ppViewer p = (ppViewer)gglobal()->Viewer.prv;
	viewer = Viewer();
	if( viewer->screendist != 0.0)
	{
		//old semantics (variable meanings)
		//eyedist - object space distance between left and right viewpoints
		//screendist - distance to toe-in target
		//stereoParameter - distance from infinity line to toe-in target
		//set_eyehalf( viewer->eyedist/2.0,atan2(viewer->eyedist/2.0,viewer->screendist)*360.0/(2.0*3.1415926));
		
		//new semantics as of March 12, 2012
		//eyedist - object space distance between left and right viewpoints
		//stereoParameter - tan(toe in angle per side) 
		//	0=looking at infinity 
		//	1= 45 degree toe-in per side (90 degree converengence)
		//	.4 = 22 degree toe-in angle per side 
		//screendist - distance from viewpoint center to 'central' viewport edge
		//			 - measured in fraction-of side-viewport
		//           - central viewport edge: 
		//				left edge of right stereo viewport
		//				right edge of left stereo viewport
		//	 = .5 - for shutterglasses and anaglyph, both sides are centered on the screen
		//        - for sidebyside, both sides are centered on their respective left and right viewports
		//				average human eyebase 65mm or 2.5" - we prefer 2.25" or 57mm. For a 6" screen 2.25/6 = .375
		set_eyehalf( viewer->eyedist/2.0,atan(viewer->stereoParameter)*180.0/3.1415926);
	}
}

void viewer_postGLinit_init(void)
{

//#if defined(FREEWRL_SHUTTER_GLASSES) || defined(FREEWRL_STEREO_RENDERING)
	X3D_Viewer *viewer;
	int type;
	s_renderer_capabilities_t *rdr_caps;
    ttglobal tg = gglobal();
	ppViewer p = (ppViewer)tg->Viewer.prv;
	viewer = Viewer();
	rdr_caps = tg->display.rdr_caps;
    
    // see if we can use quad buffer here or not.
    viewer->haveQuadbuffer = (rdr_caps->quadBuffer== GL_TRUE);
    
    //if (viewer->haveQuadbuffer) ConsoleMessage ("viewer_postGLinit_init, HAVE quad buffer"); else ConsoleMessage ("viewer_postGLinit, no quad buffer");
    
	updateEyehalf();

	type = VIEWER_STEREO_OFF;
	if( viewer->shutterGlasses ) type = VIEWER_STEREO_SHUTTERGLASSES;
	if( viewer->sidebyside ) type = VIEWER_STEREO_SIDEBYSIDE;
	if( viewer->updown ) type = VIEWER_STEREO_UPDOWN;
	if( viewer->anaglyph ==1 ) type = VIEWER_STEREO_ANAGLYPH;

	if(type==VIEWER_STEREO_SHUTTERGLASSES)
	{
		// does this opengl driver/hardware support GL_STEREO? p.469, p.729 RedBook and
		//   WhiteDune > swt.c L1306
		if (!viewer->haveQuadbuffer ) {
			ConsoleMessage("Unable to get quadbuffer stereo visual, switching to flutter mode\n");
		}
	}

	setStereo(type);

//#else
//setStereo(VIEWER_STEREO_OFF);
//#endif

}

void fwl_set_StereoParameter (const char *optArg) {

	X3D_Viewer *viewer;
	int i;

	//if(Viewer.isStereo == 0)
	//	initStereoDefaults();
	ppViewer p = (ppViewer)gglobal()->Viewer.prv;
	viewer = Viewer();

	i = sscanf(optArg,"%lf",&viewer->stereoParameter);
	if (i==0) printf ("warning, command line stereo parameter incorrect - was %s\n",optArg);
	else updateEyehalf();
}

void fwl_set_EyeDist (const char *optArg) {
	int i;
	//if(Viewer.isStereo == 0)
	//	initStereoDefaults();
	X3D_Viewer *viewer;
	ppViewer p = (ppViewer)gglobal()->Viewer.prv;
	viewer = Viewer();

	i= sscanf(optArg,"%lf",&viewer->eyedist);
	if (i==0) printf ("warning, command line eyedist parameter incorrect - was %s\n",optArg);
	else updateEyehalf();
}

void fwl_set_ScreenDist (const char *optArg) {
	int i;
	//if(Viewer.isStereo == 0)
	//	initStereoDefaults();
	X3D_Viewer *viewer;
	ppViewer p = (ppViewer)gglobal()->Viewer.prv;
	viewer = Viewer();

	i= sscanf(optArg,"%lf",&viewer->screendist);
	if (i==0) printf ("warning, command line screendist parameter incorrect - was %s\n",optArg);
	else updateEyehalf();
}
/* end of Shutter glasses, stereo mode configure */

void set_stereo_offset0() /*int iside, double eyehalf, double eyehalfangle)*/
{
	double x = 0.0, angle = 0.0;
	X3D_Viewer *viewer;
	ppViewer p = (ppViewer)gglobal()->Viewer.prv;
	viewer = Viewer();

	if (viewer->iside == 0) {
		/* left */
		x = viewer->eyehalf;
		angle = viewer->eyehalfangle; //old semantics: * viewer->stereoParameter; /*stereoparamter: 0-1 1=toe in to cross-over at Screendist 0=look at infinity, eyes parallel*/
	} else if (viewer->iside == 1) {
		/* right */
		x = -viewer->eyehalf;
		angle = -viewer->eyehalfangle; //old semantics: * viewer->stereoParameter;
	}
	FW_GL_TRANSLATE_D(x, 0.0, 0.0);
	FW_GL_ROTATE_D(angle, 0.0, 1.0, 0.0);
}

/* used to move, in WALK, FLY modes. */
void increment_pos(struct point_XYZ *vec) {
	struct point_XYZ nv;
	Quaternion q_i;
	X3D_Viewer *viewer;
	ppViewer p = (ppViewer)gglobal()->Viewer.prv;
	viewer = Viewer();

	viewer_lastP_add(vec);

	/* bound-viewpoint-space > Viewer.Pos,Viewer.Quat > avatar-space */
	quaternion_inverse(&q_i, &(viewer->Quat));
	quaternion_rotation(&nv, &q_i, vec);

	/* save velocity calculations for this mode; used for EAI calls only */
	viewer->VPvelocity.x = nv.x; viewer->VPvelocity.y = nv.y; viewer->VPvelocity.z = nv.z;
	/* and, act on this change of location. */
	viewer->Pos.x += nv.x;  /* Viewer.Pos must be in bound-viewpoint space */
	viewer->Pos.y += nv.y; 
	viewer->Pos.z += nv.z;
	

	/* printf ("increment_pos; oldpos %4.2f %4.2f %4.2f, anti %4.2f %4.2f %4.2f nv %4.2f %4.2f %4.2f \n",
		Viewer.Pos.x, Viewer.Pos.y, Viewer.Pos.z, 
		Viewer.AntiPos.x, Viewer.AntiPos.y, Viewer.AntiPos.z, 
		nv.x, nv.y, nv.z); */
	
}

/* We have a OrthoViewpoint node being bound. (not a GeoViewpoint node) */
void bind_OrthoViewpoint (struct X3D_OrthoViewpoint *vp) {
	Quaternion q_i;
	float xd, yd,zd;
	X3D_Viewer *viewer;
	ppViewer p = (ppViewer)gglobal()->Viewer.prv;
	viewer = Viewer();


	/* did bind_node tell us we could bind this guy? */
	if (!(vp->isBound)) return;

	/* SLERPing */
	/* record position BEFORE calculating new Viewpoint position */
	INITIATE_SLERP

	/* calculate distance between the node position and defined centerOfRotation */
	INITIATE_POSITION

	/* assume Perspective, unless Otrho set */
	viewer->ortho=TRUE;
	if (vp->fieldOfView.n == 4) {
			/* Ortho mapping - glOrtho order left/right/bottom/top
			   assume X3D says left bottom right top */
		viewer->orthoField[0] = (double) vp->fieldOfView.p[0];
		viewer->orthoField[1] = (double) vp->fieldOfView.p[2];
		viewer->orthoField[2] = (double) vp->fieldOfView.p[1];
		viewer->orthoField[3] = (double) vp->fieldOfView.p[3];
	} else {
		ERROR_MSG("OrthoViewpoint - fieldOfView must have 4 parameters");
		viewer->orthoField[0] = 0.0;
		viewer->orthoField[1] = 0.0;
		viewer->orthoField[2] = 0.0;
		viewer->orthoField[3] = 0.0;
	}

	/* printf ("orthoviewpoint binding distance %f\n",Viewer.Dist);  */

	/* since this is not a bind to a GeoViewpoint node... */
	viewer->GeoSpatialNode = NULL;

	/* set the examine mode rotation origin */
	INITIATE_ROTATION_ORIGIN

	/* printf ("BVP, origin %4.3f %4.3f %4.3f\n",Viewer.examine->Origin.x, Viewer.examine->Origin.y, Viewer.examine->Origin.z); */

	/* set Viewer position and orientation */

	/*
	printf ("bind_OrthoViewpoint, setting Viewer to %f %f %f orient %f %f %f %f\n",vp->position.c[0],vp->position.c[1],
	vp->position.c[2],vp->orientation.c[0],vp->orientation.c[1],vp->orientation.c[2], vp->orientation.c[3]);
	printf ("	node %d fieldOfView %f\n",vp,vp->fieldOfView); 
	printf ("	center of rotation %f %f %f\n",vp->centerOfRotation.c[0], vp->centerOfRotation.c[1],vp->centerOfRotation.c[2]);
	*/
	
	/* 
	
	From specs > abstract > architecture > 23.3.5 Viewpoint
	"When a Viewpoint node is at the top of the stack, the user's view is 
	conceptually re-parented as a child of the Viewpoint node. All subsequent changes to the Viewpoint node's
	coordinate system change the user's view (e.g., changes to any ancestor transformation nodes or to 
	the Viewpoint node's position or orientation fields)."
	
	"Navigation types (see 23.3.4 NavigationInfo) that require a definition of a down vector (e.g., terrain following) 
	shall use the negative Y-axis of the coordinate system of the currently bound Viewpoint node. 
	Likewise, navigation types that require a definition of an up vector shall use the positive Y-axis of the 
	coordinate system of the currently bound Viewpoint node. The orientation field of the Viewpoint node does 
	not affect the definition of the down or up vectors. This allows the author to separate the viewing direction 
	from the gravity direction."

	concept of transformations Jan3,2010:
world coords > [Transform stack] > bound Viewpoint > [Viewer.Pos,.Quat] > avatar 
"            < inverse[Transformstack] < "         < [AntiPos,AntiQuat] < avatar 
	gravity (according to specs): Y-down in the (bound Viewpoint local coords). 
	The viewpoint node orientation field doesn't count toward specs-gravity.
	If you want global-gravity, then put your viewpoint node at the scene level, or compute a 
	per-frame gravity for spherical worlds - see mainloop.c render_collisions.

	Implication: the user button LEVEL should level out/cancel/zero the bound-viewpoint's orientation field value.

	*/

	INITIATE_POSITION_ANTIPOSITION
	/* printf ("bind_OrthoViewpoint, pos %f %f %f antipos %f %f %f\n",Viewer.Pos.x, Viewer.Pos.y, Viewer.Pos.z, Viewer.AntiPos.x, Viewer.AntiPos.y, Viewer.AntiPos.z);
	*/

	viewer_lastP_clear();
	resolve_pos();
}

/* called from main, after the new viewpoint is setup */
int slerp_viewpoint(int itype)
{
	int iret;
	X3D_Viewer *viewer;
	ppViewer p = (ppViewer)gglobal()->Viewer.prv;
	viewer = Viewer();

	iret = 0; 
	if(viewer->SLERPing3 && itype==3){
		//navigation 'm' LOOKAT and 'g' EXPLORE non-vp-bind slerping comes through here
		// slerps: viewer->pos, .quat, .dist 
		double tickFrac;
		tickFrac = (TickTime() - viewer->startSLERPtime)/viewer->transitionTime;
		tickFrac = min(1.0,tickFrac); //clamp to max 1.0 otherwise a slow frame rate will overshoot
		quaternion_slerp(&viewer->Quat,&viewer->startSLERPQuat,&viewer->endSLERPQuat,tickFrac);
		point_XYZ_slerp(&viewer->Pos,&viewer->startSLERPPos,&viewer->endSLERPPos,tickFrac);
		general_slerp(&viewer->Dist,&viewer->startSLERPDist,&viewer->endSLERPDist,1,tickFrac);
		if(tickFrac >= 1.0) {
			viewer->SLERPing3 = 0;
			resolve_pos2(); //may not need this if examine etc do it
		}
		iret = 1;
		//now we let normal rendering use the viewer quat, pos, dist during rendering
	}else if(viewer->SLERPing2 && p->vp2rnSaved && itype==2) {
		//viewpoint slerp-on-bind comes through here
		if(viewer->SLERPing2justStarted)
		{
			//rn rootnode space, vpo/vpn old and new viewpoint space
			double vpo2rn[16];
            //double rn2vpo[16];
            double vpn2rn[16],rn2vpn[16];
            //double rn2rn[16];
            double diffrn[16];
			memcpy(vpo2rn,p->viewpoint2rootnode,sizeof(double)*16);
			if(viewer->LookatMode==3){
				memcpy(vpn2rn,p->viewpointnew2rootnode,sizeof(double)*16);
			}else{
				FW_GL_GETDOUBLEV(GL_MODELVIEW_MATRIX, p->viewpoint2rootnode);
				memcpy(vpn2rn,p->viewpoint2rootnode,sizeof(double)*16);
			}
			//matinverse(rn2vpo,vpo2rn);
			matinverseAFFINE(rn2vpn,vpn2rn);
			//this works a bit:
			// diff_RN[rn x rn] = vpo2rn[rn x vpo] * rn2vpn[vpn x rn]
			//printmatrix2(vpo2rn,"vpo2rn");
			//printmatrix2(rn2vpn,"rn2vpn");
			matmultiplyAFFINE(diffrn,vpo2rn,rn2vpn); 
			//printmatrix2(diffrn,"AFFINE diffrn");
			//matmultiplyFULL(diffrn,vpo2rn,rn2vpn);
			//printmatrix2(diffrn,"FULL diffrn");

			//slerping quat and point_XYZ
			matrix_to_quaternion(&p->sq,diffrn);
			quaternion_normalize(&p->sq);
			p->sp[0] = diffrn[12];
			p->sp[1] = diffrn[13];
			p->sp[2] = diffrn[14];

			viewer->SLERPing2justStarted = FALSE;
			//p->tickFrac = 0.0;
			//printf("in slerping2juststarted ");
		}
		//back transform by slerped amount
		{
			double tickFrac;
			Quaternion qdif,qzero;
			double vzero[3], vshift[3];

			tickFrac = (TickTime() - viewer->startSLERPtime)/viewer->transitionTime;
			/*
			if(0){ //debugging slowly
				p->tickFrac += .1;
				tickFrac = min(tickFrac,p->tickFrac);
			}*/
			tickFrac = DOUBLE_MIN(tickFrac,1.0);
			tickFrac = DOUBLE_MAX(tickFrac,0.0);
			//printf(" %4.1lf",tickFrac);
			//slerping quat and point
			vzero[0] = vzero[1] = vzero[2] = 0.0;
			vrmlrot_to_quaternion(&qzero, 0.0,1.0,0.0,0.0); //zero it
			quaternion_slerp(&qdif,&p->sq,&qzero,tickFrac);
			general_slerp(vshift,p->sp,vzero,3,tickFrac);
			if(1){
				FW_GL_TRANSLATE_D(vshift[0],vshift[1],vshift[2]);
				quaternion_togl(&qdif);
			}
			if(tickFrac > .99)
			{
				viewer->SLERPing2 = FALSE;
				//printf(" done\n");
			}
		}
		iret = 1;
	}
	return iret;
}
void setup_viewpoint_slerp(double* center, double pivot_radius, double vp_radius){
	/* when you don't have a  new viewpoint to bind to, but know where you want the viewer to go
		with a transform relative to the viewer, instead of bind_viewpoint call 
		setup_viewpoint_slerp(pointInEyespace, radiusOfShapeInEyespace)
		
	*/
	//GLDOUBLE matTargeti[16]; //, matTarget[16], mv[16];
	//double dradius; //distance, 
	double yaw, pitch; //, R1[16], R2[16], R3[16], R3i[16]; //, T[16], matQuat[16]; //, matAntiQuat[16];
	double C[3];
	//Quaternion sq;
	Quaternion q_i;
	Quaternion qyaw, qpitch, qtmp;
	struct point_XYZ PC;

	X3D_Viewer *viewer;
	ppViewer p = (ppViewer)gglobal()->Viewer.prv;

	double  pos[3] = {0.0,0.0,0.0}; //rpos[3],
	struct point_XYZ pp,qq;
	viewer = Viewer();

	veccopyd(pos,center);

	//dradius = max(viewer->Dist, radius + 5.0);
	//distance = veclengthd(pos);
	//distance = (distance - dradius)/distance;
	vecnormald(pos,pos);
	vecscaled(pos,pos,vp_radius); //distance);
	//dradius = veclengthd(pos);

	viewer->SLERPing3 = 1;

	//Dec 2014 another attempt at non-bind viewpoint slerping 
	//method: 
	// 1. snapshot the current viewer->quat, viewer->pos, viewer->dist as startSLERP 
	// 2. compute ending pos, quat, dist and set as endSLERP .Pos, .Quat .Dist
	// 3. in viewpoint_slerp(), slerp from starting to ending

	// 1. snapshot current viewer quat,pos,dist as startSLERP
	viewer->startSLERPPos = viewer->Pos;
	viewer->startSLERPQuat = viewer->Quat;
	viewer->startSLERPDist = viewer->Dist;
	viewer->startSLERPtime = TickTime();
		
	// 2. compute end pos,quat,dist as endSLERP
	// generally we have a vector center, and a pitch,roll 
	// end = start + center,pitch,roll
	// except we need to do proper transform concatonation:
	/*
	0.World
		1. viewpoint
			2. avatar position .Pos
				3. avatar orientation .Quat
					our center, pitch, yaw are observed here
	Order of transforms:
		.Pos += inverse(.Quat)*center
	*/
	viewer->endSLERPDist = vp_radius;

	// end = start + ...
	// endPos = startPos + inverse(startQuat)*center
	quaternion_normalize(&viewer->startSLERPQuat);
	quaternion_inverse( &q_i,&viewer->startSLERPQuat);
	vecdifd(pos,center,pos);
	double2pointxyz(&pp,pos);
	quaternion_rotation(&qq, &q_i, &pp);
	vecadd(&viewer->endSLERPPos,&viewer->startSLERPPos,&qq);

	// endQuat = startQuat*toQuat(yaw)*toQuat(pitch)
	//when you pick, your pickray and shape object isn't usually dead center in the viewport. In that case,
	//besides translating the viewer, you also want to turn the camera to look at the 
	//center of the shape (turning somewhat toward the pickray direction, but more precisely to the shape object ccenter)
	//compute yaw from our pickray
	veccopyd(C,pos);
	if( APPROX( vecnormald(C,C), 0.0) )
		C[2] = 1.0;
	//if we are too close, we don't want to turn 180 to move away, we just want to back up
	if(C[2] < 0.0) 
		vecscaled(C,C,-1.0);
	yaw = -atan2(C[0],C[2]);
	//apply the yaw to the pickray, so that all that's left of the pickray is the pitch part
	vrmlrot_to_quaternion(&qyaw, 0.0,1.0,0.0,yaw);
	double2pointxyz(&PC,C);
	quaternion_rotation(&PC,&qyaw,&PC);
	//compute pitch from yaw-transformed pickray
	pitch = atan2(PC.y,PC.z);
	vrmlrot_to_quaternion(&qpitch,1.0,0.0,0.0,pitch);
	//quatEnd = quatPitch*quatYaw*quatStart
	quaternion_multiply(&qtmp,&qyaw,&qpitch);
	quaternion_multiply(&viewer->endSLERPQuat,&qtmp,&viewer->startSLERPQuat);
	if(viewer->LookatMode == 3){
		if(viewer->type == VIEWER_LOOKAT)
			fwl_set_viewer_type(VIEWER_LOOKAT); //toggle off LOOKAT
		if(viewer->type == VIEWER_EXPLORE)
			fwl_set_viewer_type(VIEWER_EXPLORE); //toggle off LOOKAT
		viewer->LookatMode = 0; //VIEWER_EXPLORE
	}
	//viewer_lastP_clear(); //not sure I need this - its for wall penetration
}



/* We have a Viewpoint node being bound. (not a GeoViewpoint node) */
void bind_Viewpoint (struct X3D_Viewpoint *vp) {
	Quaternion q_i;
	float xd, yd,zd;
	X3D_Viewer *viewer;
	ppViewer p = (ppViewer)gglobal()->Viewer.prv;

	/* did bind_node tell us we could bind this guy? */
	if (!(vp->isBound)) return;

	/* SLERPing */
	/* record position BEFORE calculating new Viewpoint position */
	/*
		dug9 - viewpoint slerping: what I see as of July 12, 2011: 
			in the scene file if there's non-zero position 
			and orientation values in the fields of the first bindable viewpoint these values are 
			modified by slerping, during the initial bind. After the initial slerp, slerping code runs, 
			but accomplishes no effective slerping. 
			If the fields (.position, .orientation) are zero when the viewpoint first binds, 
			slerp code runs but no effective slerping.

		dug9 - my concept of how a viewpoint slerp should work, as of July 12, 2011:
			A smooth transition between current world pose and newly bound viewpoint world pose.
			definition of 'pose': 6 parameters consisting of 3 translations and 3 rotations in 3D space
				representing the position and direction of an assymetric object
			viewpoint pose: transform stack + (.Pos, .Quat) 
				- on initial binding without viewpoint slerping 
					(.Pos,.Quat) = (.position,.orientation)
				- on initial bind with viewpoint slerping 
					(.Pos,.Quat) = (.position,.orientation) - pose_difference
					pose_difference = (new viewpoint pose) - (last viewpoint pose)
			more detail...
		1. during a viewpoint bind the 'pose_difference' between the old and new viewpoint poses
		   is computed from their transform stacks
		   pose_difference = new_viewpoint_world_pose - old_viewpoint_world_pose
	    2. the new viewpoint is bound -as normally done without slerping- so it's at the new pose
		3. the new viewpoint's pose is multiplied by inverse(pose_difference) effectively 
			putting the camera part of the new viewpoint back to the old viewpoints camera pose. 
			This could be done by multiplying the position and orientation fields 
	    4. slerping is started to reduce pose_difference to zero at which point slerping stops
		   and the camera is at it's viewpoint's final pose
		there needs to be variables for the following:
			a) pose_difference - a translation and rotation
			b) original position and orientation fields
			c) modified position and orientation fields  
				modified_viewpoint_pose = inverse(pose_difference) * bound_viewpoint_pose
		the easy part is getting the position and orientation fields, which are simple properties
		of viewpoints. 
		pose_difference:
		The hard part: getting the pose_difference which is found by traversing
		the scenegraph to both viewpoints, at some point in time in the frame cycle. But when? 
		Options:
		A. as needed during a viewpoint bind, and with slerping on, call a function to
			traverse the scenegraph especially for getting the 2 viewpoint global transforms
	    B. every time a viewpoint is visited on a scenegraph traversal, store its global transform
			with it, so it's refreshed often, and becomes a property of the viewpoint which
			can be accessed immediately when slerping begins. And hope that's good enough, which
			during a very busy event cascade, it might not be.
		C. stagger the start of slerping to cover 2 frames
			- on the bind frame, we can call a function to invert the current modelview matrix, for 
			  the old viewpoint.
		    - on the next frame, ditto for the new viewpoint, then start slerping
			- problem: there's one frame where the camera jitters to the new pose, then on the
			  next frame back to the old pose where it starts slerping. 
			  solution: to avoid this, on the second
			  frame, before starting to draw, perhaps in prep_Viewpoint(), when we have the 
			  current modelview matrix for the new viewpoint, this is the point when we would
			  compute the pose_difference and the initial pose parameters.
		Current process: ==============================================
			prep_Viewpoint() in Component_Navigation.c L.80 
				- does the per-frame slerp increment
				- does the viewpoint field values of .orientation,.position 
			bind_Viewpoint() in Viewer.c L.1925 (here) - sets up the slerping values and flags
			viewer_togl() in Viewer.c L.515 - computes slerp increment, 
				- does part not done by prep_Viewpoint 
				- so net of that prep_Viewpoint + viewer_togl() combined 
					pose_difference = (new_world_pose) - (old_world_pose)
					pose_increment = slerpIncrement(pose_difference)
			    - turns off slerping flag when done. 

			Call stack:
			mainloop L.503  (before render geometry)
				startOfLoopNodeUpdates() in OpenGL_Utils L.3406
					bind_Viewpoint() (here)
			mainloop L.630 (before render geometry)
				render_pre()
					setup_viewpoint()
						viewer_togl()
						render_hier(rootnode,VF_Viewpoint)
							prep_Viewpoint() - only called on the current bound viewpoint
			mainloop L.647  (for render_hier(,VF_sensitive) after render geometry)
				setup_viewpoint() 
					ditto
			mainloop L.764
                SEND_BIND_IF_REQUIRED(tg->ProdCon.setViewpointBindInRender)
					prodcon L.604 send_bind_to(X3D_NODE(t->viewpointnodes[i]), 0); 
						send_bind_to() in Bindables.c L.267
							bind_viewpoint()
			generally setup_viewpoint() is called when needed before any 
			   non-VF_Viewpoint render_hier() call to update the current pose 
			   -and modelview matrix- of the camera
		Variables and what they mean:
			Viewer.
			.position    -viewpoint field, only changes through scripting
			.orientation -viewpoint field, only changes through scripting 
			- when you re-bind to a viewpoint later, these will be the originals or script modified
			- transform useage:
				ShapeCoordinates
					Transform stack shape2world (model part of modelview)
						WorldCoordinates (at scene root)
							Transform stack to CBV (view part of modelview)
								Currently Bound Viewpoint (CBV)
									.Pos (== .position after bind, then navigation changes it)
										viewpoint avatar
											.Quat (== .orientation after bind, then navigation changes it)
												viewpoint camera (so called eye coords)
			.Pos: on binding, it gets a fresh copy of the .position field of the CBV
				- and navigation changes it
			.Quat: on binding, it gets a fresh copy of the .orientation field of the CBV
				- and navigation changes it
				- LEVEL/viewer_level_to_bound() changes it
			.bindTimeQuat: == .orientation (of CBV) through lifecycle
					- used to un-rotate modelviewmatrix (which includes .Pos,.Quat)
					   to getcurrentPosInModel() (Q. should it be the whole .Quat?)

			.AntiPos
			.AntiQuat:  == inverse(.orientation)
			[.prepVPQuat == .orientation, used in prep_Viewpoint, which is wrong because it's not updated from scripting against .orientation]
			.currentPosInModel - used to calculate examine distance, GeoLOD range, debugging
								- starts as .position, updated in getCurrentPosInModel()
			No-slerping use of variables in prep_Viewpoint():
					rotate(prepVPQuat)
					translate(viewer->position)
		New process: =======================================================
		Goal: get both the old and new modelview matrices together, so pose_difference can be
			computed and applied to new viewpoint orientation/position before render() on the
			2nd loop.
		Proposed process:
			in bind_viewpoint, set a flag for prep_viewpoint saying its a newly bound viewpoint
				- save the current modelview matrix
			after prep_viewpoint in mainloop, call a new function:
			slerp_viewpoint():
				a) the first time in on a newly bound viewpoint
				- retrieve the last modelview stored by bind_viewpoint
			    - get the modelview matrix for the new viewpoint 
				- compute pose_difference between the old and new viewpoints
					pose_difference = last_modelviewmatrix*inverse(newModelViewMatrix)
				- modify the position and orientation fields of the new one
				  with pose_difference ie .Pos, .Orient += slerp(pose_difference)
				- setup the slerping numbers
				b) subsequent visits on the bound viewpoint
				- do slerping increment to reduce pose_difference gradually to zero
				- apply to .Pos,.Quat 
				- shut off slerping when done
	*/
	//INITIATE_SLERP
	//if(false){
	viewer = Viewer();
	if (viewer->transitionType != VIEWER_TRANSITION_TELEPORT && viewer->wasBound) { 
        viewer->SLERPing = FALSE; //TRUE; 
        viewer->startSLERPtime = TickTime(); 
        memcpy (&viewer->startSLERPPos, &viewer->Pos, sizeof (struct point_XYZ)); 
        memcpy (&viewer->startSLERPAntiPos, &viewer->AntiPos, sizeof (struct point_XYZ)); 
        memcpy (&viewer->startSLERPQuat, &viewer->Quat, sizeof (Quaternion)); 
        memcpy (&viewer->startSLERPAntiQuat, &viewer->AntiQuat, sizeof (Quaternion));  
        memcpy (&viewer->startSLERPbindTimeQuat, &viewer->bindTimeQuat, sizeof (Quaternion)); 
        memcpy (&viewer->startSLERPprepVPQuat, &viewer->prepVPQuat, sizeof (Quaternion)); 

		/* slerp Mark II */
		viewer->SLERPing2 = TRUE;
		viewer->SLERPing2justStarted = TRUE;
		//printf("binding\n");
		//save for future slerps
		p->vp2rnSaved = TRUE; //I probably don't need this flag, I always bind before prep_viewpoint()
		//printf("S");
		FW_GL_GETDOUBLEV(GL_MODELVIEW_MATRIX, p->viewpoint2rootnode);
		//printf("S");

	} else { 
		viewer->SLERPing = FALSE; 
		viewer->SLERPing2 = FALSE;
	}
	
	viewer->wasBound = TRUE;
	/* calculate distance between the node position and defined centerOfRotation */
	INITIATE_POSITION

	/* assume Perspective, unless Otrho set */
	viewer->ortho=FALSE;

	/* printf ("viewpoint binding distance %f\n",Viewer.Dist);  */

	/* since this is not a bind to a GeoViewpoint node... */
	viewer->GeoSpatialNode = NULL;

	/* set the examine mode rotation origin */
	INITIATE_ROTATION_ORIGIN

	/* set Viewer position and orientation */
	/* 
	
	From specs > abstract > architecture > 23.3.5 Viewpoint
	"When a Viewpoint node is at the top of the stack, the user's view is 
	conceptually re-parented as a child of the Viewpoint node. All subsequent changes to the Viewpoint node's
	coordinate system change the user's view (e.g., changes to any ancestor transformation nodes or to 
	the Viewpoint node's position or orientation fields)."
	
	"Navigation types (see 23.3.4 NavigationInfo) that require a definition of a down vector (e.g., terrain following) 
	shall use the negative Y-axis of the coordinate system of the currently bound Viewpoint node. 
	Likewise, navigation types that require a definition of an up vector shall use the positive Y-axis of the 
	coordinate system of the currently bound Viewpoint node. The orientation field of the Viewpoint node does 
	not affect the definition of the down or up vectors. This allows the author to separate the viewing direction 
	from the gravity direction."

	concept of transformations Jan3,2010:
world coords > [Transform stack] > bound Viewpoint > [Viewer.Pos,.Quat] > avatar 
"            < inverse[Transformstack] < "         < [AntiPos,AntiQuat] < avatar 
	gravity (according to specs): Y-down in the (bound Viewpoint local coords). 
	The viewpoint node orientation field doesn't count toward specs-gravity.
	If you want global-gravity, then put your viewpoint node at the scene level, or compute a 
	per-frame gravity for spherical worlds - see mainloop.c render_collisions.

	Implication: the user button LEVEL should level out/cancel/zero the bound-viewpoint's orientation field value.

	*/

	INITIATE_POSITION_ANTIPOSITION

	viewer_lastP_clear();
	resolve_pos();
}

int fwl_getAnaglyphSide(int whichSide) {
	ppViewer p = (ppViewer)gglobal()->Viewer.prv;
        //glColorMask(p->acMask[iside][0],p->acMask[iside][1],p->acMask[iside][2],t);

	if ((whichSide<0) || (whichSide>1)) {
		return 0;
	}

	/* should return 
		000 0 Black
		001 1 Blue
		010 2 Green
		011 3 Cyan
		100 4  Red
		101 5 Magenta
		110 6 Yellow
		111 7 White
	*/

	return (p->acMask[whichSide][0] << 2) | (p->acMask[whichSide][1] << 1) | (p->acMask[whichSide][2]);
}




// Android - we are loading in a new file while keeping the system sane.
void Android_reset_viewer_to_defaults() {
	//ConsoleMessage("********** Android_reset_viewer_to_defaults");
        // reset the viewer to initial mode.
	X3D_Viewer *viewer;
        ppViewer p = (ppViewer)gglobal()->Viewer.prv;
	viewer = Viewer();
        p->viewer_initialized = FALSE;

	viewer_default();
	viewer->SLERPing2 = FALSE;
	viewer->SLERPing = FALSE;
}

int viewer_iside(){
	return Viewer()->iside;
}