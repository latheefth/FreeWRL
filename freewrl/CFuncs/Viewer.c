/*******************************************************************
 Copyright (C) 1998 Tuomas J. Lukka 2003 John Stewart, Ayla Khan CRC Canada
 Portions Copyright (C) 1998 John Breen
 DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
 See the GNU Library General Public License (file COPYING in the distribution)
 for conditions of use and redistribution.
*********************************************************************/

/*
 * $Id$
 *
 */

#include "Viewer.h"


static int viewer_type = NONE;
static int viewer_initialized = FALSE;
static VRML_Viewer_Walk viewer_walk = { 0, 0, 0, 0, 0, 0 };
static VRML_Viewer_Examine viewer_examine = { { 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, 0, 0 };
static VRML_Viewer_Fly viewer_fly = { { 0, 0, 0 }, { 0, 0, 0 }, KEYMAP, KEYMAP, -1 };

static int translate[COORD_SYS] = { 0, 0, 0 }, rotate[COORD_SYS] = { 0, 0, 0 };

static FILE *exfly_in_file;

void viewer_init (VRML_Viewer *viewer, int type) {

	/* what type are we? used for handle events below */
	viewer_type = type;

	/* if we are brand new, set up our defaults */
	if (!viewer_initialized) {	
		viewer_initialized = TRUE;

/* 		Pos => [0,0,10]; */
		(viewer->Pos).x = 0;
		(viewer->Pos).y = 0;
		(viewer->Pos).z = 10;

/* 		# The viewpoint node at the time of the binding -- we have */
/* 		# to counteract it. */
/* 		# AntiPos is the real position, AntiQuat is inverted ;) */
/* 		AntiPos => [0,0,0], */
		(viewer->AntiPos).x = 0;
		(viewer->AntiPos).y = 0;
		(viewer->AntiPos).z = 0;

/* 		Quat => new VRML::Quaternion(1,0,0,0), */
		(viewer->Quat).w = 1;
		(viewer->Quat).x = 0;
		(viewer->Quat).y = 0;
		(viewer->Quat).z = 0;

/* 		AntiQuat => new VRML::Quaternion(1,0,0,0), */
		(viewer->AntiQuat).w = 1;
		(viewer->AntiQuat).x = 0;
		(viewer->AntiQuat).y = 0;
		(viewer->AntiQuat).z = 0;

/* 		Navi => undef, */
/* 		$this->{Navi} = VRML::Scene->new_node("NavigationInfo", */
/* 							VRML::Nodes->{NavigationInfo}{Defaults}); */
		/* speed & headlight are the only NavigationInfo fields used */
		viewer->headlight = TRUE;
		viewer->speed = 0.0;
/* 		Dist = 10.0; */
		viewer->Dist = 10.0;
		viewer->eyehalf = 0.0;
		viewer->eyehalfangle = 0.0;
		viewer->buffer = 0;

		viewer->walk = &viewer_walk;
		viewer->examine = &viewer_examine;
		viewer->fly = &viewer_fly;
	}
/* 	$this->resolve_pos(); */
	resolve_pos(viewer);
}


unsigned int
get_buffer(VRML_Viewer *viewer)
{
	return(viewer->buffer);
}

void
set_buffer(VRML_Viewer *viewer, const unsigned int buffer)
{
	viewer->buffer = buffer;
}

int
get_headlight(VRML_Viewer *viewer)
{
	return(viewer->headlight);
}

void
toggle_headlight(VRML_Viewer *viewer)
{
	if (viewer->headlight == TRUE) {
		viewer->headlight = FALSE;
	} else {
		viewer->headlight = TRUE;
	}
}

void
set_eyehalf(VRML_Viewer *viewer, const double eyehalf, const double eyehalfangle)
{
	viewer->eyehalf = eyehalf;
	viewer->eyehalfangle = eyehalfangle;
}

void
set_viewer_type(const int type)
{
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


int
use_keys()
{ 	
	if (viewer_type == FLY) {
		return TRUE;
	}
	return FALSE;
}


void
resolve_pos(VRML_Viewer *viewer)
{ 
	/* my($this) = @_; */
	struct pt rot, z_axis = { 0, 0, 1 };
	Quaternion q_inv;
	double dist = 0;
	VRML_Viewer_Examine *examine = viewer->examine;

	if (viewer_type == EXAMINE) {
		/* my $z = $this->{Quat}->invert->rotate([0,0,1]); */
		inverse(&q_inv, &(viewer->Quat));
		rotation(&rot, &q_inv, &z_axis);

		/* my $d = 0; for(0..2) {$d += $this->{Pos}[$_] * $z->[$_]} */
		dist = VECPT(viewer->Pos, rot);

		/*
		 * Fix the rotation point to be 10m in front of the user (dist = 10.0)  
		 * or, try for the origin. Preferential treatment would be to choose
		 * the shape within the center of the viewpoint. This information is
		 * found in the matrix, and is used for collision calculations - we
		 * need to better store it.
		 */

		/* $d = abs($d); $this->{Dist} = $d; */
		viewer->Dist = fabs(dist);

		/* $this->{Origin} = [ map {$this->{Pos}[$_] - $d * $z->[$_]} 0..2 ]; */
		(examine->Origin).x = (viewer->Pos).x - viewer->Dist * rot.x;
		(examine->Origin).y = (viewer->Pos).y - viewer->Dist * rot.y;
		(examine->Origin).z = (viewer->Pos).z - viewer->Dist * rot.z;
	}
}

void
viewer_togl(VRML_Viewer *viewer, double fieldofview)
{
/* 	my($this) = @_; */

/* 	if ($this->{buffer}!=&VRML::OpenGL::GL_BACK) */
	if (viewer->buffer != GL_BACK) {
		set_stereo_offset(viewer->buffer, viewer->eyehalf, viewer->eyehalfangle, fieldofview);
/* 		VRML::VRMLFunc::set_stereo_offset ($this->{buffer}, $this->{eyehalf},$this->{eyehalfangle}); */
	}

/* 	$this->{Quat}->togl(); */
	togl(&(viewer->Quat));
/* 	VRML::OpenGL::glTranslatef(map {-$_} @{$this->{Pos}}); */
	glTranslated(-(viewer->Pos).x,
				 -(viewer->Pos).y,
				 -(viewer->Pos).z);
/* 	VRML::OpenGL::glTranslatef(@{$this->{AntiPos}}); */
	glTranslated((viewer->AntiPos).x,
				 (viewer->AntiPos).y,
				 (viewer->AntiPos).z);
/* 	$this->{AntiQuat}->togl(); */
	togl(&(viewer->AntiQuat));
}


void
handle_walk(VRML_Viewer *viewer, const char *mev, const unsigned int button, const double x, const double y)
{
/* 	my($this, $mev, $but, $mx, $my) = @_; */
	VRML_Viewer_Walk *walk = viewer->walk;

/* 	if($mev eq "PRESS" and $but == 1) { */
	if (strncmp(mev, PRESS, PRESS_LEN) == 0) {
/* 		$this->{SY} = $my; */
/* 		$this->{SX} = $mx; */
/* 	} elsif($mev eq "PRESS" and $but == 3) { */
/* 		$this->{SY} = $my; */
/* 		$this->{SX} = $mx; */
		walk->SY = y;
		walk->SX = x;
/* 	} elsif($mev eq "DRAG" and $but == 1) { */
	} else if (strncmp(mev, DRAG, DRAG_LEN) == 0) {
		if (button == 1) {
/* 		$this->{ZD} = ($my - $this->{SY}) * $this->{Navi}{Fields}{speed}; */
			walk->ZD = (y - walk->SY) * viewer->speed;
/* 		$this->{RD} = ($mx - $this->{SX}) * 0.1; */
			walk->RD = (x - walk->SX) * viewer->speed;
/* 	} elsif($mev eq "DRAG" and $but == 3) { */
		} else if (button == 3) {
/* 		$this->{XD} = ($mx - $this->{SX}) * $this->{Navi}{Fields}{speed}; */
			walk->XD = (x - walk->SX) * viewer->speed;
/* 		$this->{YD} = -($my - $this->{SY}) * $this->{Navi}{Fields}{speed}; */
			walk->YD = (y - walk->SY) * viewer->speed;
		}
/* 	} elsif ($mev eq "RELEASE") { */
	} else if (strncmp(mev, RELEASE, RELEASE_LEN) == 0) {
		if (button == 1) {
/* 			$this->{ZD} = 0; */
			walk->ZD = 0;
/* 			$this->{RD} = 0; */
			walk->RD = 0;
		} else if (button == 3) {
/* 			$this->{XD} = 0; */
			walk->XD = 0;
/* 			$this->{YD} = 0; */
			walk->YD = 0;
		}
	}
}


void
handle_examine(VRML_Viewer *viewer, const char *mev, const unsigned int button, const double x, const double y)
{
/* 	my($this, $mev, $but, $mx, $my) = @_; */
	Quaternion q, q_i, arc;
	struct pt p = { 0, 0, viewer->Dist };
	VRML_Viewer_Examine *examine = viewer->examine;

/* 	if($mev eq "PRESS" and $but == 1) { */
	if (strncmp(mev, PRESS, PRESS_LEN) == 0) {
		if (button == 1) {
	/* 		$this->{SQuat} = $this->xy2qua($mx,$my); */
			xy2qua(&(examine->SQuat), x, y);
	/* 		$this->{OQuat} = $this->{Quat}; */
			examine->OQuat = viewer->Quat;
	/* 	} elsif($mev eq "PRESS" and $but == 3) { */
		} else if (button == 3) {
	/* 		$this->{SY} = $my; */
			examine->SY = y;
	/* 		$this->{ODist} = $this->{Dist}; */
			examine->ODist = viewer->Dist;
		}
/* 	} elsif($mev eq "DRAG" and $but == 1) { */
	} else if (strncmp(mev, DRAG, DRAG_LEN) == 0) {
		if (button == 1) {
			/* 		if (!defined $this->{SQuat}) {  */
			/* we have missed the press */
			if (norm(&(examine->SQuat)) == 0) {
				printf("Viewer handle_examine: mouse event DRAG - missed press\n");
				/* 			$this->{SQuat} = $this->xy2qua($mx,$my); */
				xy2qua(&(examine->SQuat), x, y);
				/* 			$this->{OQuat} = $this->{Quat}; */
				examine->OQuat = viewer->Quat;
			} else {
				/* 			my $q = $this->xy2qua($mx,$my); */
				xy2qua(&q, x, y);
				/* 			my $arc = $q->multiply($this->{SQuat}->invert()); */
				inverse(&q_i, &(examine->SQuat));
				multiply(&arc, &q, &q_i);
				/* 			$this->{Quat} = $arc->multiply($this->{OQuat}); */
				multiply(&(viewer->Quat), &arc, &(examine->OQuat));
			}
			/* 	} elsif($mev eq "DRAG" and $but == 3) { */
		} else if (button == 3) {
			/* 		$this->{Dist} = $this->{ODist} * exp($this->{SY} - $my); */
			viewer->Dist = examine->ODist * exp(examine->SY - y);
		}
 	}
/* 	$this->{Pos} = $this->{Quat}->invert->rotate([0,0,$this->{Dist}]); */
	inverse(&q_i, &(viewer->Quat));
	rotation(&(viewer->Pos), &q_i, &p);
/* 	for(0..2) {$this->{Pos}[$_] += $this->{Origin}[$_]} */
	(viewer->Pos).x += (examine->Origin).x;
	(viewer->Pos).y += (examine->Origin).y;
	(viewer->Pos).z += (examine->Origin).z;
}


void handle(VRML_Viewer *viewer, const char *mev, const unsigned int button, const double x, const double y)
{
	if (button == 2) {
		return;
	}

	switch(viewer_type) {
	case NONE:
		break;
	case EXAMINE:
		handle_examine(viewer, mev, button, x, y);
		break;
	case WALK:
		handle_walk(viewer, mev, button, x, y);
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
handle_key(VRML_Viewer *viewer, const double time, const char key)
{
	/* my($this,$time,$key) = @_; */
	VRML_Viewer_Fly *fly = viewer->fly;
	char _key;
	int i;

	UNUSED(time);

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
handle_keyrelease(VRML_Viewer *viewer, const double time, const char key)
{
	/* my($this,$time,$key) = @_; */
	VRML_Viewer_Fly *fly = viewer->fly;
	char _key;
	int i;

	UNUSED(time);

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
handle_tick_walk(VRML_Viewer *viewer, const double time)
{
/* 	my($this, $time) = @_; */
	VRML_Viewer_Walk *walk = viewer->walk;
	Quaternion q_i, q = { (viewer->Quat).w,
						  (viewer->Quat).x,
						  (viewer->Quat).y,
						  (viewer->Quat).z },
		/* my $nq = new VRML::Quaternion(1-0.2*$this->{RD},0,0.2*$this->{RD},0); */
		nq = { 1 - 0.2 * walk->RD,
			   0,
			   0.2 * walk->RD,
			   0 };
	struct pt nv, p = { 0.15 * walk->XD, 0.15 * walk->YD, 0.15 * walk->ZD };

	UNUSED(time);

	/* my $nv = $this->{Quat}->invert->rotate([0.15*$this->{XD},0.15*$this->{YD},0.15*$this->{ZD}]); */
	inverse(&q_i, &(viewer->Quat));
	rotation(&nv, &q_i, &p);

	/* for(0..2) {$this->{Pos}[$_] += $nv->[$_]} */
	(viewer->Pos).x += nv.x;
	(viewer->Pos).y += nv.y;
	(viewer->Pos).z += nv.z;

	/* $nq->normalize_this; */
	normalize(&nq);
	/* $this->{Quat} = $nq->multiply($this->{Quat}); */
	multiply(&(viewer->Quat), &nq, &q);
    
	/* info passed to Collision routines */
	/* VRML::VRMLFunc::set_viewer_delta($this->{XD},
	 * $this->{YD},$this->{ZD}); #interresting idea, but not quite.
	 */

	/* any movement? if so, lets render it. */
 	if ((fabs(walk->RD) > 0.000001) ||
		(fabs(walk->YD) > 0.000001) ||
		(fabs(walk->ZD) > 0.000001)) {
		set_render_frame();
	}
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
handle_tick_exfly(VRML_Viewer *viewer, const double time)
{
/* 	my($this, $time) = @_; */
	size_t len = 0;
	unsigned int i = 0;
	char string[STRING_SIZE];
	char x_str[INPUT_LEN], y_str[INPUT_LEN], z_str[INPUT_LEN];
	char quat_w_str[INPUT_LEN], quat_x_str[INPUT_LEN], quat_y_str[INPUT_LEN], quat_z_str[INPUT_LEN];

	UNUSED(time);

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
				"Viewer handle_tick_exfly: could not open %s for read.\n",
				IN_FILE);
		return;
	}
	fread(string, sizeof(char), IN_FILE_BYTES, exfly_in_file);
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
		while (i < len) {
			if (i < INPUT_LEN_Z) { /* substr ($string,0,8); */
				z_str[i] = string[i];
			} else if (i >= X_OFFSET &&
					   i < INPUT_LEN) { /* substr ($string,8,9); */
				x_str[i] = string[i];
			} else if (i >= Y_OFFSET &&
					   i < INPUT_LEN) { /* substr ($string,17,9); */
				y_str[i] = string[i];
			} else if (i >= QUAT_W_OFFSET &&
					   i < INPUT_LEN) { /* substr($string,26,9) */
				quat_w_str[i] = string[i];
			} else if (i >= QUAT_X_OFFSET &&
					   i < INPUT_LEN) { /* substr($string,35,9) */
				quat_x_str[i] = string[i];
			} else if (i >= QUAT_Y_OFFSET &&
					   i < INPUT_LEN) { /* substr($string,44,9) */
				quat_y_str[i] = string[i];
			} else if (i >= QUAT_Z_OFFSET &&
					   i < INPUT_LEN) { /* substr($string,53,9) */
				quat_z_str[i] = string[i];
			} else {
				break;
			}
			i++;
		}

		/* $this->{Pos}[0] = substr ($string,8,9); */
		(viewer->Pos).x = atof((const char *) x_str);
		/* $this->{Pos}[1] = substr ($string,17,9); */
		(viewer->Pos).y = atof((const char *) y_str);
		/* $this->{Pos}[2] = substr ($string,0,8); */
		(viewer->Pos).z = atof((const char *) z_str);

/* 		$this->{Quat} = new VRML::Quaternion(substr ($string,26,9),  */
/* 			substr ($string,35,9), substr ($string,44,9), */
/* 			substr ($string,53,9)); */
		(viewer->Quat).w =  atof((const char *) quat_w_str);
		(viewer->Quat).x =  atof((const char *) quat_x_str);
		(viewer->Quat).y =  atof((const char *) quat_y_str);
		(viewer->Quat).z =  atof((const char *) quat_z_str);

		set_render_frame();
	}
}


/* $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$ */

/* package VRML::Viewer::Fly; # Modeled after Descent(tm) ;) */
/* @VRML::Viewer::Fly::ISA=VRML::Viewer; */
/* # */
/* # Members: */
/* #  Velocity - current velocity as 3-vector */
/* #   */

/* # Do nothing for the mouse */


/* my %actions = ( */
/* 	a => sub {$aadd[2] -= $_[0]}, */
/* 	z => sub {$aadd[2] += $_[0]}, */
/* 	j => sub {$aadd[0] -= $_[0]}, */
/* 	l => sub {$aadd[0] += $_[0]}, */
/* 	p => sub {$aadd[1] += $_[0]}, */
/* 	';' => sub {$aadd[1] -= $_[0]}, */

/* 	8 => sub {$radd[0] += $_[0]}, */
/* 	k => sub {$radd[0] -= $_[0]}, */
/* 	u => sub {$radd[1] -= $_[0]}, */
/* 	o => sub {$radd[1] += $_[0]}, */
/* 	7 => sub {$radd[2] -= $_[0]}, */
/* 	9 => sub {$radd[2] += $_[0]}, */
/* ); */

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
		translate[Y_AXIS] += 1;
		break;
	case ';':
		translate[Y_AXIS] -= 1;
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
handle_tick_fly(VRML_Viewer *viewer, const double time)
{
/* 	my($this, $time) = @_; */

	VRML_Viewer_Fly *fly = viewer->fly;
	Key ps[KEYS_HANDLED] = KEYMAP;
	Quaternion q_i, q_v, nq = { 1, 0, 0, 0 };
	struct pt nv, v;
	double changed = 0, time_diff = -1;
	int i;

	if (fly->lasttime < 0) {
		fly->lasttime = time;
		return;
	} else {
		time_diff = time - fly->lasttime;
		fly->lasttime = time;
		if (APPROX(time_diff, 0)) {
			return;
		}
	}
	
	/* first, get all the keypresses since the last time */
/* 	for(keys %{$this->{Down}}) */
	for (i = 0; i < KEYS_HANDLED; i++) {
/* 		$ps{$_} += $this->{Down}{$_}; */
		(ps[i]).hit += (fly->Down[i]).hit;
	}

/* 	for(keys %{$this->{WasDown}}) */
	for (i = 0; i < KEYS_HANDLED; i++) {
/* 		$ps{$_} += delete $this->{WasDown}{$_}; */
		(ps[i]).hit += (fly->WasDown[i]).hit;
		(fly->WasDown[i]).hit = 0;
	}

/* 	undef @aadd; */
/* 	undef @radd; */
	memset(translate, 0, sizeof(int) * COORD_SYS);
	memset(rotate, 0, sizeof(int) * COORD_SYS);

/* 	for(keys %ps) */
	for (i = 0; i < KEYS_HANDLED; i++) {
/* 		if(exists $actions{$_}) { */
/* 			$actions{$_}->($ps{$_}?1:0); */
/* 		}  */
		if ((ps[i]).hit) {
			set_action(&(ps[i]).key);
		}
	}

	/* has anything changed? if so, then re-render */
/* 	for(@$v) */
	for (i = 0; i < COORD_SYS; i++) {
/* 		$_ *= 0.06 ** ($dt); */
		fly->Velocity[i] *= pow(0.06, time_diff);

/* 		$_ += $dt * $aadd[$ind++] * 14.5; */
		fly->Velocity[i] += time_diff * translate[i] * 14.5;

/* 		if(abs($_) > 9.0) {$_ /= abs($_)/9.0} */
		if (fabs(fly->Velocity[i]) >9.0) {
			fly->Velocity[i] /= (fabs(fly->Velocity[i]) /9.0);
		}
/* 		$changed += $_; */
		changed += fly->Velocity[i];
	}

/* 	my $nv = $this->{Quat}->invert->rotate( */
/* 		[map {$_ * $dt} @{$this->{Velocity}}] */
/* 	); */
	v.x = fly->Velocity[0] * time_diff;
	v.y = fly->Velocity[1] * time_diff;
	v.z = fly->Velocity[2] * time_diff;

	inverse(&q_i, &(viewer->Quat));
	rotation(&nv, &q_i, &v);

/* 	for(0..2) {$this->{Pos}[$_] += $nv->[$_]} */
	(viewer->Pos).x += nv.x;
	(viewer->Pos).y += nv.y;
	(viewer->Pos).z += nv.z;

/* 	for(@$av) */
	for (i = 0; i < COORD_SYS; i++) {
/* 		$_ *= 0.04 ** ($dt); */
		fly->AVelocity[i] *= pow(0.04, time_diff);
/* 		$_ += $dt * $radd[$ind++] * 0.1; */
		fly->AVelocity[i] += time_diff * rotate[i] * 0.1;

/* 		if(abs($_) > 0.8) {$_ /= abs($_)/0.8;} */
		if (fabs(fly->AVelocity[i]) > 0.8) {
			fly->AVelocity[i] /= (fabs(fly->AVelocity[i]) / 0.8);
		}
/* 		$sq += $_*$_; */
/* 		$changed += $_; */
		changed += fly->AVelocity[i];
	}
	
/* 	my $nq = new VRML::Quaternion(1,@$av); */
	nq.x = fly->AVelocity[0];
	nq.y = fly->AVelocity[1];
	nq.z = fly->AVelocity[2];
/* 	$nq->normalize_this; */
	normalize(&nq);

/* 	$this->{Quat} = $nq->multiply($this->{Quat}); */
	set(&q_v, &(viewer->Quat));
	multiply(&(viewer->Quat), &nq, &q_v);

	/* any movement? if so, lets render it */
	if (fabs(changed) > 0.000001) {
		set_render_frame();
	}
}


void
handle_tick(VRML_Viewer *viewer, const double time)
{
	switch(viewer_type) {
	case NONE:
		break;
	case EXAMINE:
		break;
	case WALK:
		handle_tick_walk(viewer, time);
		break;
	case EXFLY:
		handle_tick_exfly(viewer, time);
		break;
	case FLY:
		handle_tick_fly(viewer, time);
		break;
	default:
		break;
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

void
set_stereo_offset(unsigned int buffer, const double eyehalf, const double eyehalfangle, const double fieldofview)
{
      double x = 0.0, correction, angle = 0.0;

      /*
	   * correction for fieldofview
       * 18.0: builtin fieldOfView of human eye
       * 45.0: default fieldOfView of VRML97 viewpoint
       */

      correction = 18.0 / fieldofview;
      if (buffer == GL_BACK_LEFT) {
              x = eyehalf;
              angle = eyehalfangle * correction;
      } else if (buffer == GL_BACK_RIGHT) {
              x = -eyehalf;
              angle = -eyehalfangle * correction;
      }
      glTranslated(x, 0.0, 0.0);
      glRotated(angle, 0.0, 1.0, 0.0);
}
