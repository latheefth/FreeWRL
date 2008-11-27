/*
=INSERT_TEMPLATE_HERE=

$Id$

CProto ???

*/

#include <config.h>
#include <system.h>
#include <display.h>
#include <internal.h>

#include <libFreeX3D.h>

#include "../vrml_parser/Structs.h"
#include "../main/headers.h"

#include "quaternion.h"
#include "Viewer.h"


static int viewer_type = NONE;
int viewer_initialized = FALSE;
static X3D_Viewer_Walk viewer_walk = { 0, 0, 0, 0, 0, 0 };
static X3D_Viewer_Examine viewer_examine = { { 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, 0, 0 };
static X3D_Viewer_Fly viewer_fly = { { 0, 0, 0 }, { 0, 0, 0 }, KEYMAP, KEYMAP, -1 };

static int translate[COORD_SYS] = { 0, 0, 0 }, rotate[COORD_SYS] = { 0, 0, 0 };

static int movedPosition = FALSE;
static int walkMotion = FALSE;

static FILE *exfly_in_file;

struct point_XYZ VPvelocity;

double nearPlane=DEFAULT_NEARPLANE;                     /* near Clip plane - MAKE SURE that statusbar is not in front of this!! */
double farPlane=DEFAULT_FARPLANE;                       /* a good default value */
double screenRatio=1.5;
double fieldofview=45.0;
double calculatedNearPlane = 0.0;
double calculatedFarPlane = 0.0;


void print_viewer(void);
unsigned int get_buffer(void);
int get_headlight(void);
void toggle_headlight(void);
void handle_tick_walk(void);
void handle_tick_fly(void);
void handle_tick_exfly(void);
void handle_tick(void);

/* used for EAI calls to get the current speed. Not used for general calcs */
/* we DO NOT return as a float, as some gccs have trouble with this causing segfaults */
void getCurrentSpeed() {
	BrowserSpeed = BrowserFPS * (fabs(VPvelocity.x) + fabs(VPvelocity.y) + fabs(VPvelocity.z));
}

void viewer_default() {
	Quaternion q_i;

	fieldofview = 45.0;

	VPvelocity.x = 0.0; VPvelocity.y = 0.0; VPvelocity.z = 0.0; 
	(Viewer.Pos).x = 0;
	(Viewer.Pos).y = 0;
	(Viewer.Pos).z = 10;

	(Viewer.AntiPos).x = 0;
	(Viewer.AntiPos).y = 0;
	(Viewer.AntiPos).z = 0;
	vrmlrot_to_quaternion (&Viewer.Quat,1.0,0.0,0.0,0.0);
	vrmlrot_to_quaternion (&q_i,1.0,0.0,0.0,0.0);
	inverse(&(Viewer.AntiQuat),&q_i);

	Viewer.headlight = TRUE;
	/* tell the menu buttons of the state of this headlight */
	setMenuButton_headlight(Viewer.headlight);
	Viewer.speed = 1.0;
	Viewer.Dist = 10.0;
	Viewer.walk = &viewer_walk;
	Viewer.examine = &viewer_examine;
	Viewer.fly = &viewer_fly;

	set_viewer_type(1);

	set_eyehalf( eyedist/2.0,
		atan2(eyedist/2.0,screendist)*360.0/(2.0*3.1415926));

	/* assume we are not bound to a GeoViewpoint */
	Viewer.GeoSpatialNode = NULL;

#ifndef AQUA
	if (shutterGlasses)
		XEventStereo();
#endif
}

void viewer_init (X3D_Viewer *viewer, int type) {
	Quaternion q_i;

	/* what type are we? used for handle events below */
	viewer_type = type;

	/* if we are brand new, set up our defaults */
	if (!viewer_initialized) {
		viewer_initialized = TRUE;

		(viewer->Pos).x = 0;
		(viewer->Pos).y = 0;
		(viewer->Pos).z = 10;

		(viewer->AntiPos).x = 0;
		(viewer->AntiPos).y = 0;
		(viewer->AntiPos).z = 0;


		vrmlrot_to_quaternion (&Viewer.Quat,1.0,0.0,0.0,0.0);
		vrmlrot_to_quaternion (&q_i,1.0,0.0,0.0,0.0);
		inverse(&(Viewer.AntiQuat),&q_i);

		viewer->headlight = TRUE;
		/* tell the menu buttons of the state of this headlight */
		setMenuButton_headlight(viewer->headlight);
		viewer->speed = 1.0;
		viewer->Dist = 10.0;
		viewer->walk = &viewer_walk;
		viewer->examine = &viewer_examine;
		viewer->fly = &viewer_fly;
	}
	resolve_pos();
}


void
print_viewer()
{
	struct orient ori;
	quaternion_to_vrmlrot(&(Viewer.Quat), &(ori.x),&(ori.y),&(ori.z), &(ori.a));
	printf("Viewer {\n\tPosition [ %.4g, %.4g, %.4g ]\n\tQuaternion [ %.4g, %.4g, %.4g, %.4g ]\n\tOrientation [ %.4g, %.4g, %.4g, %.4g ]\n}\n", (Viewer.Pos).x, (Viewer.Pos).y, (Viewer.Pos).z, (Viewer.Quat).w, (Viewer.Quat).x, (Viewer.Quat).y, (Viewer.Quat).z, ori.x, ori.y, ori.z, ori.a);

}

unsigned int
get_buffer()
{
	return(Viewer.buffer);
}

void
set_buffer(const unsigned int buffer)
{
	Viewer.buffer = buffer;
}

int get_headlight() {
	return(Viewer.headlight);
}

void toggle_headlight() {
	if (Viewer.headlight == TRUE) {
		Viewer.headlight = FALSE;
	} else {
		Viewer.headlight = TRUE;
	}
	/* tell the menu buttons of the state of this headlight */
	setMenuButton_headlight(Viewer.headlight);

}

void set_eyehalf(const double eyehalf, const double eyehalfangle) {
	Viewer.eyehalf = eyehalf;
	Viewer.eyehalfangle = eyehalfangle;
}

void set_viewer_type(const int type) {

	/* set velocity array to zero again - used only for EAI */
	VPvelocity.x=0.0; VPvelocity.y=0.0; VPvelocity.z=0.0;

	/* can the currently bound viewer type handle this */
	/* if there is no bound viewer, just ignore (happens on initialization) */
	if (navi_tos != -1)
		if (Viewer.oktypes[type]==FALSE) {
			setMenuButton_navModes(viewer_type);
			return;
		}

	viewer_init(&Viewer,type);

	/* tell the window menu what we are */
	setMenuButton_navModes(viewer_type);

	switch(type) {
	case NONE:
	case EXAMINE:
	case WALK:
	case EXFLY:
	case FLY:
		viewer_type = type;
		break;
	default:
		fprintf(stderr, "Viewer type %d is not supported. See Viewer.h.\n", type);
		viewer_type = NONE;
		break;
	}
}


int use_keys() {
	if (viewer_type == FLY) {
		return TRUE;
	}
	return FALSE;
}

void resolve_pos(void) {
	/* my($this) = @_; */
	struct point_XYZ rot, z_axis = { 0, 0, 1 };
	Quaternion q_inv;

	double dist = 0;

	X3D_Viewer_Examine *examine = Viewer.examine;

	if (viewer_type == EXAMINE) {
		/* my $z = $this->{Quat}->invert->rotate([0,0,1]); */
		inverse(&q_inv, &(Viewer.Quat));
		rotation(&rot, &q_inv, &z_axis);

		if (Viewer.GeoSpatialNode != NULL) {
			/* printf ("resolve_pos, a Geospatial Viewpoint\n"); */
			/* Geospatial Viewpoint - */
			/* my $d = 0; for(0..2) {$d += $this->{Pos}[$_] * $z->[$_]} */
			dist = VECPT(Viewer.Pos, rot);
	
			/*
			 * Fix the rotation point to be 10m in front of the user (dist = 10.0)
			 * or, try for the origin. Preferential treatment would be to choose
			 * the shape within the center of the viewpoint. This information is
			 * found in the matrix, and is used for collision calculations - we
			 * need to better store it.
			 */
	
			/* $d = abs($d); $this->{Dist} = $d; */
			Viewer.Dist = fabs(dist);
		}

		/* $this->{Origin} = [ map {$this->{Pos}[$_] - $d * $z->[$_]} 0..2 ]; */
		(examine->Origin).x = (Viewer.Pos).x - Viewer.Dist * rot.x;
		(examine->Origin).y = (Viewer.Pos).y - Viewer.Dist * rot.y;
		(examine->Origin).z = (Viewer.Pos).z - Viewer.Dist * rot.z;

		/* printf ("examine origin = %f %f %f\n",examine.Origin.x,examine.Origin.y,examine.Origin.z); */
	}
}

void viewer_togl(double fieldofview) {

	/* GLdouble modelMatrix[16];

        fwGetDoublev(GL_MODELVIEW_MATRIX, modelMatrix);
	printf ("Viewer_togl Matrix: \n\t%f %f %f %f\n\t%f %f %f %f\n\t%f %f %f %f\n\t%f %f %f %f\n",
                modelMatrix[0],  modelMatrix[4],  modelMatrix[ 8],  modelMatrix[12],
                modelMatrix[1],  modelMatrix[5],  modelMatrix[ 9],  modelMatrix[13],
                modelMatrix[2],  modelMatrix[6],  modelMatrix[10],  modelMatrix[14],
                modelMatrix[3],  modelMatrix[7],  modelMatrix[11],  modelMatrix[15]);
	*/


	if (Viewer.buffer != GL_BACK) {
		set_stereo_offset(Viewer.buffer, Viewer.eyehalf, Viewer.eyehalfangle, fieldofview);
	}

	togl(&(Viewer.Quat));
	/* printf ("viewer togl first trans %lf %lf %lf\n", -(Viewer.Pos).x, -(Viewer.Pos).y, -(Viewer.Pos).z);
	printf ("       togl secnd trans %lf %lf %lf\n",(Viewer.AntiPos).x, (Viewer.AntiPos).y, (Viewer.AntiPos).z);
	*/


	glTranslated(-(Viewer.Pos).x, -(Viewer.Pos).y, -(Viewer.Pos).z);
	glTranslated((Viewer.AntiPos).x, (Viewer.AntiPos).y, (Viewer.AntiPos).z);
	togl(&(Viewer.AntiQuat));

        /* fwGetDoublev(GL_MODELVIEW_MATRIX, modelMatrix);
       printf ("Viewer end _togl Matrix: \n\t%f %f %f %f\n\t%f %f %f %f\n\t%f %f %f %f\n\t%f %f %f %f\n",
                modelMatrix[0],  modelMatrix[4],  modelMatrix[ 8],  modelMatrix[12],
                modelMatrix[1],  modelMatrix[5],  modelMatrix[ 9],  modelMatrix[13],
                modelMatrix[2],  modelMatrix[6],  modelMatrix[10],  modelMatrix[14],
                modelMatrix[3],  modelMatrix[7],  modelMatrix[11],  modelMatrix[15]);
	*/

}


void handle_walk(const int mev, const unsigned int button, const float x, const float y) {
	X3D_Viewer_Walk *walk = Viewer.walk;

	walkMotion = FALSE;	/* assume no motion right now */
	if (mev == ButtonPress ) {
		walk->SY = y;
		walk->SX = x;
	} else if (mev == MotionNotify) {
		/* ok, we HAVE done something here */
		walkMotion = TRUE;

		if (button == 1) {
			walk->ZD = (y - walk->SY) * Viewer.speed;
			walk->RD = (x - walk->SX) * 0.1;
		} else if (button == 3) {
			walk->XD = (x - walk->SX) * Viewer.speed;
			walk->YD = -(y - walk->SY) * Viewer.speed;
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


void handle_examine(const int mev, const unsigned int button, float x, float y) {
	Quaternion q, q_i, arc;
	struct point_XYZ p = { 0, 0, 0};
	X3D_Viewer_Examine *examine = Viewer.examine;
	double squat_norm;

	p.z=Viewer.Dist;

	if (mev == ButtonPress) {
		if (button == 1) {
			xy2qua(&(examine->SQuat), x, y);
			set(&(examine->OQuat), &(Viewer.Quat));
		} else if (button == 3) {
			examine->SY = y;
			examine->ODist = Viewer.Dist;
		}
	} else if (mev == MotionNotify) {
		/* ok, we HAVE done something here */
		movedPosition = TRUE;

		if (button == 1) {
			squat_norm = norm(&(examine->SQuat));
			/* we have missed the press */
			if (APPROX(squat_norm, 0)) {
				fprintf(stderr, "Viewer handle_examine: mouse event DRAG - missed press\n");
				/* 			$this->{SQuat} = $this->xy2qua($mx,$my); */
				xy2qua(&(examine->SQuat), x, y);
				/* 			$this->{OQuat} = $this->{Quat}; */
				set(&(examine->OQuat), &(Viewer.Quat));
			} else {
				/* my $q = $this->xy2qua($mx,$my); */
				xy2qua(&q, x, y);
				/* my $arc = $q->multiply($this->{SQuat}->invert()); */
				inverse(&q_i, &(examine->SQuat));
				multiply(&arc, &q, &q_i);


				/* $this->{Quat} = $arc->multiply($this->{OQuat}); */
				multiply(&(Viewer.Quat), &arc, &(examine->OQuat));
			}
		} else if (button == 3) {
			Viewer.Dist = examine->ODist * exp(examine->SY - y);

		}
 	}
	inverse(&q_i, &(Viewer.Quat));
	rotation(&(Viewer.Pos), &q_i, &p);

	Viewer.Pos.x += (examine->Origin).x;
	Viewer.Pos.y += (examine->Origin).y;
	Viewer.Pos.z += (examine->Origin).z;
}


void handle(const int mev, const unsigned int button, const float x, const float y)
{

	/* printf("Viewer handle: viewer_type %s, mouse event %d, button %u, x %f, y %f\n",
	   VIEWER_STRING(viewer_type), mev, button, x, y);  */

	if (button == 2) {
		return;
	}

	switch(viewer_type) {
	case NONE:
		break;
	case EXAMINE:
		handle_examine(mev, button, ((float) x), ((float) y));
		break;
	case WALK:
		handle_walk(mev, button, ((float) x), ((float) y));
		break;
	case EXFLY:
		break;
	case FLY:
		break;
	default:
		break;
	}
}


void
handle_key(const char key)
{
	X3D_Viewer_Fly *fly = Viewer.fly;
	char _key;
	int i;

	if (viewer_type == FLY) {
		/* $key = lc $key; */
		_key = (char) tolower((int) key);

		for (i = 0; i < KEYS_HANDLED; i++) {
			if ((fly->Down[i]).key  == _key) {
				/* $this->{Down}{$key} = 1; */
				(fly->Down[i]).hit = 1;
			}
		}
	}
}


void
handle_keyrelease(const char key)
{
	/* my($this,$time,$key) = @_; */
	X3D_Viewer_Fly *fly = Viewer.fly;
	char _key;
	int i;

	if (viewer_type == FLY) {
		/* $key = lc $key; */
		_key = (char) tolower((int) key);

		for (i = 0; i < KEYS_HANDLED; i++) {
			if ((fly->Down[i]).key  == _key) {
				/* $this->{WasDown}{$key} += $this->{Down}{$key}; */
				(fly->WasDown[i]).hit += (fly->Down[i]).hit;
				/* delete $this->{Down}{$key}; */
				(fly->Down[i]).hit = 0;
			}
		}
	}
}


/*
 * handle_tick_walk: called once per frame.
 *
 * Sets viewer to next expected position.
 * This should be called before position sensor calculations
 * (and event triggering) take place.
 * Position dictated by this routine is NOT final, and is likely to
 * change if the viewer is left in a state of collision. (ncoder)
 */

void
handle_tick_walk()
{
	X3D_Viewer_Walk *walk = Viewer.walk;
	Quaternion q, nq;
	struct point_XYZ p;

	
	if (!walkMotion) return;
	movedPosition = TRUE;

	p.x = 0.15 * walk->XD;
	p.y = 0.15 * walk->YD;
	p.z = 0.15 * walk->ZD;
	q.w = (Viewer.Quat).w;
	q.x = (Viewer.Quat).x;
	q.y = (Viewer.Quat).y;
	q.z = (Viewer.Quat).z;
	nq.w = 1 - 0.2 * walk->RD;
	nq.x = 0.0l;
	nq.y = 0.2 * walk->RD;
	nq.z = 0.0;

	increment_pos(&p);

	normalize(&nq);
	multiply(&(Viewer.Quat), &nq, &q);
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

void
handle_tick_exfly()
{
	size_t len = 0;
	char string[STRING_SIZE];
	float px,py,pz,q1,q2,q3,q4;
	size_t rv; /* unused, but here for compile warnings */

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
	if ((exfly_in_file = fopen(IN_FILE, "r")) == NULL) {
		fprintf(stderr,
				"Viewer handle_tick_exfly: could not open %s for read, returning to EXAMINE mode.\nSee the FreeWRL man page for further details on the usage of Fly - External Sensor input mode.\n",
				IN_FILE);

		/* allow the user to continue in default Viewer mode */
		viewer_type = EXAMINE;
		setMenuButton_navModes(viewer_type);
		return;
	}
	rv = fread(string, sizeof(char), IN_FILE_BYTES, exfly_in_file);
	if (ferror(exfly_in_file)) {
		fprintf(stderr,
				"Viewer handle_tick_exfly: error reading from file %s.",
				IN_FILE);
		fclose(exfly_in_file);
		return;
	}
	fclose(exfly_in_file);

/* 	if (length($string)>0) */
	if ((len = strlen(string)) > 0) {
		len = sscanf (string, "%f %f %f %f %f %f %f",&px,&py,&pz,
			&q1,&q2,&q3,&q4);

		/* read error? */
		if (len != 7) return;

		(Viewer.Pos).x = px;
		(Viewer.Pos).y = py;
		(Viewer.Pos).z = pz;

		(Viewer.Quat).w = q1;
		(Viewer.Quat).x = q2;
		(Viewer.Quat).y = q3;
		(Viewer.Quat).z = q4;
	}
}


void
set_action(char *key)
{
	switch(*key) {
	case 'a':
		translate[Z_AXIS] -= 1;
		break;
	case 'z':
		translate[Z_AXIS] += 1;
		break;
	case 'j':
		translate[X_AXIS] -= 1;
		break;
	case 'l':
		translate[X_AXIS] += 1;
		break;
	case 'p':
		translate[Y_AXIS] -= 1;
		break;
	case ';':
		translate[Y_AXIS] += 1;
		break;
	case '8':
		rotate[X_AXIS] += 1;
		break;
	case 'k':
		rotate[X_AXIS] -= 1;
		break;
	case 'u':
		rotate[Y_AXIS] -= 1;
		break;
	case 'o':
		rotate[Y_AXIS] += 1;
		break;
	case '7':
		rotate[Z_AXIS] -= 1;
		break;
	case '9':
		rotate[Z_AXIS] += 1;
		break;
	default:
		break;
	}
}

void
handle_tick_fly()
{
	X3D_Viewer_Fly *fly = Viewer.fly;
	Key ps[KEYS_HANDLED] = KEYMAP;
	Quaternion q_v, nq = { 1, 0, 0, 0 };
	struct point_XYZ v;
	double changed = 0, time_diff = -1;
	int i;

	if (fly->lasttime < 0) {
		fly->lasttime = TickTime;
		return;
	} else {
		time_diff = TickTime - fly->lasttime;
		fly->lasttime = TickTime;
		if (APPROX(time_diff, 0)) {
			return;
		}
	}

	/* first, get all the keypresses since the last time */
	for (i = 0; i < KEYS_HANDLED; i++) {
		(ps[i]).hit += (fly->Down[i]).hit;
	}

	for (i = 0; i < KEYS_HANDLED; i++) {
		(ps[i]).hit += (fly->WasDown[i]).hit;
		(fly->WasDown[i]).hit = 0;
	} 

	memset(translate, 0, sizeof(int) * COORD_SYS);
	memset(rotate, 0, sizeof(int) * COORD_SYS);

	for (i = 0; i < KEYS_HANDLED; i++) {
		if ((ps[i]).hit) {
			set_action(&(ps[i]).key);
		}
	}

	/* has anything changed? if so, then re-render */

	/* linear movement */
	for (i = 0; i < COORD_SYS; i++) {
		fly->Velocity[i] *= pow(0.06, time_diff);

		fly->Velocity[i] += time_diff * translate[i] * 14.5 * Viewer.speed;
		changed += fly->Velocity[i];
		/* printf ("vel %d %f\n",i,fly->Velocity[i]); */
	}

	/* if we do NOT have a GeoViewpoint node, constrain all 3 axis */
	if (Viewer.GeoSpatialNode == NULL) 
		for (i = 0; i < COORD_SYS; i++) {
			if (fabs(fly->Velocity[i]) >9.0) fly->Velocity[i] /= (fabs(fly->Velocity[i]) /9.0);
		}

	/* angular movement */
	for (i = 0; i < COORD_SYS; i++) {
		fly->AVelocity[i] *= pow(0.04, time_diff);
		fly->AVelocity[i] += time_diff * rotate[i] * 0.025;

		if (fabs(fly->AVelocity[i]) > 0.8) {
			fly->AVelocity[i] /= (fabs(fly->AVelocity[i]) / 0.8);
		}
		changed += fly->AVelocity[i];
		/* printf ("avel %d %f\n",i,fly->AVelocity[i]); */
	}

	/* have we done anything here? */
	if (APPROX(changed,0.0)) return;

	/* Yes, something has changed, as we are still here */
	movedPosition = TRUE;

	v.x = fly->Velocity[0] * time_diff;
	v.y = fly->Velocity[1] * time_diff;
	v.z = fly->Velocity[2] * time_diff;
	increment_pos(&v);


	nq.x = fly->AVelocity[0];
	nq.y = fly->AVelocity[1];
	nq.z = fly->AVelocity[2];
	normalize(&nq);

	set(&q_v, &(Viewer.Quat));
	multiply(&(Viewer.Quat), &nq, &q_v);
}

void
handle_tick()
{

	switch(viewer_type) {
	case NONE:
		break;
	case EXAMINE:
		break;
	case WALK:
		handle_tick_walk();
		break;
	case EXFLY:
		handle_tick_exfly();
		break;
	case FLY:
		handle_tick_fly();
		break;
	default:
		break;
	}

	/* have we moved? */
	if (movedPosition) {
		if (Viewer.GeoSpatialNode) 
			viewer_calculate_speed();
		movedPosition = FALSE;
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
/* 	my($this, $x, $y) = @_; */
/* 	$x -= 0.5; $y -= 0.5; $x *= 2; $y *= 2; */
/* 	$y = -$y; */
	double _x = x - 0.5, _y = y - 0.5, _z, dist;
	_x *= 2;
	_y *= -2;

/* 	my $dist = sqrt($x**2 + $y**2); */
	dist = sqrt((_x * _x) + (_y * _y));

/* 	if($dist > 1.0) {$x /= $dist; $y /= $dist; $dist = 1.0} */
	if (dist > 1.0) {
		_x /= dist;
		_y /= dist;
		dist = 1.0;
	}
/* 	my $z = 1-$dist; */
	_z = 1 - dist;

/* 	my $qua = VRML::Quaternion->new(0,$x,$y,$z); */
	ret->w = 0;
	ret->x = _x;
	ret->y = _y;
	ret->z = _z;
/* 	$qua->normalize_this(); */
	normalize(ret);
/* 	return $qua; */
}

float stereoParameter = 0.4;

void setStereoParameter (const char *optArg) {
	int i;
	i = sscanf(optArg,"%f",&stereoParameter);
	if (i==0) printf ("warning, command line stereo parameter incorrect - was %s\n",optArg);
}

void
set_stereo_offset(unsigned int buffer, const double eyehalf, const double eyehalfangle, const double fieldofview)
{
      double x = 0.0, angle = 0.0;
      UNUSED (fieldofview);

      if (buffer == GL_BACK_LEFT) {
              x = eyehalf;
              angle = eyehalfangle * stereoParameter;
      } else if (buffer == GL_BACK_RIGHT) {
              x = -eyehalf;
              angle = -eyehalfangle * stereoParameter;
      }
      glTranslated(x, 0.0, 0.0);
      glRotated(angle, 0.0, 1.0, 0.0);
}


/* used to move, in WALK, FLY modes. */
void increment_pos(struct point_XYZ *vec) {
	struct point_XYZ nv;
	Quaternion q_i;

	inverse(&q_i, &(Viewer.Quat));
	rotation(&nv, &q_i, vec);

	/* save velocity calculations for this mode; used for EAI calls only */
	VPvelocity.x = nv.x; VPvelocity.y = nv.y; VPvelocity.z = nv.z;

	/* and, act on this change of location. */
	(Viewer.Pos).x += nv.x;
	(Viewer.Pos).y += nv.y;
	(Viewer.Pos).z += nv.z;
}


/* We have a Viewpoint node being bound. (not a GeoViewpoint node) */
void bind_viewpoint (struct X3D_Viewpoint *vp) {
	Quaternion q_i;
	float xd, yd,zd;
	

	/* calculate distance between the node position and defined centerOfRotation */
	xd = vp->position.c[0]-vp->centerOfRotation.c[0];
	yd = vp->position.c[1]-vp->centerOfRotation.c[1];
	zd = vp->position.c[2]-vp->centerOfRotation.c[2];
	Viewer.Dist = sqrt (xd*xd+yd*yd+zd*zd);

	/* printf ("viewpoint rotate distance %f\n",Viewer.Dist); */

	/* since this is not a bind to a GeoViewpoint node... */
	Viewer.GeoSpatialNode = NULL;

	/* set Viewer position and orientation */

	/* printf ("bind_viewpoint, setting Viewer to %f %f %f orient %f %f %f %f\n",vp->position.c[0],vp->position.c[1],
	vp->position.c[2],vp->orientation.r[0],vp->orientation.r[1],vp->orientation.r[2],
	vp->orientation.r[3]);
	printf ("	node %d fieldOfView %f\n",vp,vp->fieldOfView); */
	
	Viewer.Pos.x = vp->position.c[0];
	Viewer.Pos.y = vp->position.c[1];
	Viewer.Pos.z = vp->position.c[2];
	Viewer.AntiPos.x = vp->position.c[0];
	Viewer.AntiPos.y = vp->position.c[1];
	Viewer.AntiPos.z = vp->position.c[2];

	vrmlrot_to_quaternion (&Viewer.Quat,vp->orientation.r[0],
		vp->orientation.r[1],vp->orientation.r[2],vp->orientation.r[3]);

	vrmlrot_to_quaternion (&q_i,vp->orientation.r[0],
		vp->orientation.r[1],vp->orientation.r[2],vp->orientation.r[3]);
	inverse(&(Viewer.AntiQuat),&q_i);

	resolve_pos();
}
