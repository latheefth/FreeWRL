/*


Viewer ???

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


#ifndef __FREEWRL_VIEWER_H__
#define __FREEWRL_VIEWER_H__

#include "quaternion.h"
void fwl_set_viewer_type(const int type);
int fwl_setNavMode(char *mode);

#define PRESS "PRESS"
#define PRESS_LEN 5

#define DRAG "DRAG"
#define DRAG_LEN 4

#define RELEASE "RELEASE"
#define RELEASE_LEN 7

#define KEYS_HANDLED 12


#define VIEWER_STEREO_OFF 0
#define VIEWER_STEREO_SHUTTERGLASSES 1
#define VIEWER_STEREO_SIDEBYSIDE 2
#define VIEWER_STEREO_ANAGLYPH 3
#define VIEWER_STEREO_UPDOWN 4


#define X_AXIS 0
#define Y_AXIS 1
#define Z_AXIS 2

#define STRING_SIZE 256

#ifdef _MSC_VER
#define IN_FILE "C:/tmp/inpdev.txt"
#else
#define IN_FILE "/tmp/inpdev"
#endif
#define IN_FILE_BYTES 100
#define INPUT_LEN 9
#define INPUT_LEN_Z 8
#define X_OFFSET 8
#define Y_OFFSET 17
#define Z_OFFSET 0
#define QUAT_W_OFFSET 26
#define QUAT_X_OFFSET 35
#define QUAT_Y_OFFSET 44
#define QUAT_Z_OFFSET 53


#define VIEWER_TRANSITION_TELEPORT 0
#define VIEWER_TRANSITION_LINEAR 1
#define VIEWER_TRANSITION_ANIMATE 2


#define CALCULATE_EXAMINE_DISTANCE \
	{ \
        	float xd, yd,zd; \
		double test; \
	        /* calculate distance between the node position and defined centerOfRotation */ \
	        xd = (float) p->Viewer.currentPosInModel.x; \
	        yd = (float) p->Viewer.currentPosInModel.y; \
	        zd = (float) p->Viewer.currentPosInModel.z; \
	        test = sqrt (xd*xd+yd*yd+zd*zd); \
		/* printf ("htw; cur Dist %4.2f, calculated %4.2f at %lf\n", Viewer.Dist, test,TickTime());  */\
		p->Viewer.Dist = test; \
	}

#define INITIATE_SLERP \
	if (p->Viewer.transitionType != VIEWER_TRANSITION_TELEPORT) { \
        p->Viewer.SLERPing = TRUE; \
        p->Viewer.startSLERPtime = TickTime(); \
        memcpy (&p->Viewer.startSLERPPos, &p->Viewer.Pos, sizeof (struct point_XYZ)); \
        memcpy (&p->Viewer.startSLERPAntiPos, &p->Viewer.AntiPos, sizeof (struct point_XYZ)); \
        memcpy (&p->Viewer.startSLERPQuat, &p->Viewer.Quat, sizeof (Quaternion)); \
        memcpy (&p->Viewer.startSLERPAntiQuat, &p->Viewer.AntiQuat, sizeof (Quaternion));  \
        memcpy (&p->Viewer.startSLERPbindTimeQuat, &p->Viewer.bindTimeQuat, sizeof (Quaternion)); \
        memcpy (&p->Viewer.startSLERPprepVPQuat, &p->Viewer.prepVPQuat, sizeof (Quaternion)); \
	} else { p->Viewer.SLERPing = FALSE; }


#define INITIATE_POSITION \
        xd = vp->position.c[0]-vp->centerOfRotation.c[0]; \
        yd = vp->position.c[1]-vp->centerOfRotation.c[1]; \
        zd = vp->position.c[2]-vp->centerOfRotation.c[2]; \
        p->Viewer.Dist = sqrt (xd*xd+yd*yd+zd*zd);

#define INITIATE_ROTATION_ORIGIN \
        p->Viewer.examine.Origin.x = vp->centerOfRotation.c[0]; \
        p->Viewer.examine.Origin.y = vp->centerOfRotation.c[1]; \
	p->Viewer.examine.Origin.z = vp->centerOfRotation.c[2];

#define INITIATE_POSITION_ANTIPOSITION \
        p->Viewer.Pos.x = vp->position.c[0]; \
        p->Viewer.Pos.y = vp->position.c[1]; \
        p->Viewer.Pos.z = vp->position.c[2]; \
        p->Viewer.AntiPos.x = vp->position.c[0]; \
        p->Viewer.AntiPos.y = vp->position.c[1]; \
        p->Viewer.AntiPos.z = vp->position.c[2]; \
        p->Viewer.currentPosInModel.x = vp->position.c[0]; \
        p->Viewer.currentPosInModel.y = vp->position.c[1]; \
        p->Viewer.currentPosInModel.z = vp->position.c[2]; \
        vrmlrot_to_quaternion (&p->Viewer.Quat,vp->orientation.c[0], \
                vp->orientation.c[1],vp->orientation.c[2],-vp->orientation.c[3]); /* dug9 sign change on orientation Jan 18,2010 to accomodate level_to_bound() */ \
        vrmlrot_to_quaternion (&p->Viewer.bindTimeQuat,vp->orientation.c[0], \
                vp->orientation.c[1],vp->orientation.c[2],-vp->orientation.c[3]); /* '' */ \
        vrmlrot_to_quaternion (&q_i,vp->orientation.c[0], \
                vp->orientation.c[1],vp->orientation.c[2],-vp->orientation.c[3]); /* '' */ \
        quaternion_inverse(&(p->Viewer.AntiQuat),&q_i);  \
	vrmlrot_to_quaternion(&p->Viewer.prepVPQuat,vp->orientation.c[0],vp->orientation.c[1],vp->orientation.c[2],-vp->orientation.c[3]);


/* extern struct point_XYZ ViewerPosition; */
/* extern struct orient ViewerOrientation; */


typedef struct viewer_walk {
	double SX;
	double SY;
	double XD;
	double YD;
	double ZD;
	double RD;
} X3D_Viewer_Walk;


typedef struct viewer_examine {
        struct point_XYZ Origin;
        Quaternion OQuat;
        Quaternion SQuat;
        double ODist;
        double SY;
} X3D_Viewer_Examine;

typedef struct viewer_ypz {
	double ypz0[3];
	double ypz[3];
	float x,y;
} X3D_Viewer_Spherical;

typedef struct viewer_inplane {
	double x,y;
	double xx,yy;
	int on;
	int ibut;
} X3D_Viewer_InPlane;

typedef struct key {
	char key;
	unsigned int hit;
} Key;
typedef struct keyHit {
	int direction;
	double epoch; //original keydown time
	double era; //keydown time not yet used by handle_tick
	int once; //flag for handle_tick to tell if its used this keyHit already
} KeyHit;


/* Modeled after Descent(tm) ;) */
typedef struct viewer_fly {
	double Velocity[2][3];
	KeyHit down[2][3]; //
	int ndown[2][3]; //number of clicks queued per axis motion
	KeyHit wasDown[2][3][10]; //up to 10 chars per axis motion are queued for fly_tick
	double lasttime;
} X3D_Viewer_Fly;


typedef struct viewer {
	struct point_XYZ Pos;
	struct point_XYZ AntiPos;
	struct point_XYZ currentPosInModel;
	Quaternion Quat;
	Quaternion AntiQuat;
	Quaternion bindTimeQuat;
	int headlight;
	int collision; //added July 7, 2012
	double speed;
	double Dist; //examine dist
	//double exploreDist; //explore dist
	/*stereovision...*/
	int isStereo; /*=1 stereovision of any type (all types require viewpoint to shift left and right in scene) */
	int iside;    /* rendering buffer index Left=0 Right=1 */
	int sidebyside; /*=1 if 2 viewport method*/
	int updown; /*=1 if 2 viewport method*/
	int shutterGlasses;
	int haveQuadbuffer;
	int anaglyph; /* = 1 if analglyph is turned on */
	int dominantEye; /* 2D screen cursor picks in which viewport? 0=Left 1=Right */
	double stereoParameter;
	double eyehalf;
	double eyehalfangle;
	double screendist;
	double eyedist;
	
	int iprog[2]; /*anaglyph R=0,GBACM per side */
	unsigned int buffer;
	int oktypes[16];		/* boolean for types being acceptable. */
	X3D_Viewer_Walk walk;
	X3D_Viewer_Examine examine;
	X3D_Viewer_Fly fly;
	X3D_Viewer_Spherical ypz;
	X3D_Viewer_InPlane inplane;

	struct point_XYZ VPvelocity;

	int SLERPing2;
	int SLERPing2justStarted;

	int SLERPing;
	double startSLERPtime;

	int SLERPing3; 

	int type; 	/* eg, VIEWER_EXAMINE, etc */
	int lastType; /* LOOKAT saves previous type, and recovers it when done */
	int LookatMode; //0 = not, 1= mainloop should do a node pick operation then set this back to 0 */
	int transitionType;   	/* going from one viewpoint to another */
	double transitionTime;
	double lasttime;

	struct point_XYZ startSLERPPos;
	struct point_XYZ startSLERPAntiPos;
	Quaternion startSLERPQuat;
	Quaternion startSLERPAntiQuat;
	Quaternion startSLERPbindTimeQuat;
	Quaternion prepVPQuat;
	Quaternion startSLERPprepVPQuat;

	double startSLERPDist, endSLERPDist;
	struct point_XYZ endSLERPPos;
	Quaternion endSLERPQuat;


	struct X3D_GeoViewpoint *GeoSpatialNode; /* NULL, unless we are a GeoViewpoint */

	int doExamineModeDistanceCalculations;	

	/* are we perspective or ortho? */
	int ortho;
	double orthoField[4];

	/* are we normal, or rotated? (makes sense only for mobile devices) */
	int screenOrientation;

	double nearPlane;
	double farPlane;
	double backgroundPlane ;
	GLDOUBLE fieldofview;
	GLDOUBLE fovZoom ;
	int wasBound; /* 0 for default viewpoint, 1 thereafter (for no-slerp startup) */

} X3D_Viewer;

void Viewer_anaglyph_setSide(int iside);
void Viewer_anaglyph_clearSides();
void fwl_init_StereoDefaults(void);

void viewer_postGLinit_init(void);

void viewer_init(X3D_Viewer *viewer, int type);

void print_viewer();
int fwl_get_headlight();
void fwl_toggle_headlight();
//int use_keys(void);

void set_eyehalf( const double eyehalf,	const double eyehalfangle);
void resolve_pos(void);
void getViewpointExamineDistance(void);

void xy2qua(Quaternion *ret,
	   const double x,
	   const double y);

void viewer_togl( double fieldofview);

void handle(const int mev, const unsigned int button, const float x, const float y);
void handle_key(const char key, double keytime);
void handle_keyrelease (const char key, double keytime);
void handle_tick();
void set_stereo_offset0(); /*int iside, double eyehalf, double eyehalfangle);*/
/*
void
set_stereo_offset(unsigned int buffer,
				  const double eyehalf,
				  const double eyehalfangle,
				  double fieldofview);
*/
void increment_pos( struct point_XYZ *vec);

void bind_Viewpoint(struct X3D_Viewpoint *node);
void bind_OrthoViewpoint(struct X3D_OrthoViewpoint *node);
void bind_GeoViewpoint(struct X3D_GeoViewpoint *node);

//extern X3D_Viewer Viewer; /* in VRMLC.pm */
X3D_Viewer *Viewer();

void viewer_default(void);

extern float eyedist;
extern float screendist;

void getCurrentSpeed(void);
void getCurrentPosInModel (int addInAntiPos);

void toggle_collision(void);
void viewer_lastP_clear(void);
void avatar2BoundViewpointVerticalAvatar(GLDOUBLE *matA2BVVA, GLDOUBLE *matBVVA2A);

void toggleOrSetStereo(int type);
void setAnaglyphSideColor(char val, int iside);
void updateEyehalf(void);
void viewer_level_to_bound(void);

int getAnaglyphPrimarySide(int primary, int iside);
void setAnaglyphPrimarySide(int primary, int iside);
int viewer_getKeyChord();
void viewer_setKeyChord(int chord);

#endif /* __FREEWRL_VIEWER_H__ */
