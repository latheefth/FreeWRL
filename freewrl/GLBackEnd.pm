# Copyright (C) 1998 Tuomas J. Lukka, 1999 John Stewart CRC Canada.
# DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
# See the GNU Library General Public License (file COPYING in the distribution)
# for conditions of use and redistribution.


# Implement OpenGL backend using the C structs.

package VRML::GLBackEnd;

use VRML::OpenGL;
use VRML::VRMLFunc;
require 'VRML/VRMLCU.pm';

if($VRML::verbose::rend) {
	VRML::VRMLFunc::render_verbose(1);
}

if($VRML::verbose::collision) {
    VRML::VRMLFunc::render_verbose_collision(1);
}

use strict vars;

###############################################################
#
# Public backend API

# The cursor changes over sensitive nodes. 0 is the "normal"
# cursor.

my $cursortype = 0;  # pointer cursor
my $curcursor = 99;  # the last cursor change - force the cursor on startup

my $becollision = 1;	# collision detection turned on or off - 1 = on.

## from Viewer.h:
sub NONE  { return 0; }
sub EXAMINE { return 1; }
sub WALK { return 2; }
sub EXFLY { return 3; }
sub FLY { return 4; }

####
#
# set fast rendering - don't do smooth shading

sub set_fast {
	glShadeModel(&GL_FLAT);
}


####
#
# Take a snapshot of the world. Returns array ref, 
# first member: width, second: height and third is a raw string
# of RGB values. You can use e.g. rawtoppm to convert this

sub snapshot {
	my($this) = @_;
	my($w,$h) = @{$this}{qw/W H/};
	my $n = 3*$w*$h;
	my $str = pack("C$n");
	glPixelStorei(&GL_UNPACK_ALIGNMENT,1);
	glPixelStorei(&GL_PACK_ALIGNMENT,1);
	glReadPixels(0,0,$w,$h,&GL_RGB,&GL_UNSIGNED_BYTE, $str);
	return [$w,$h,$str];
}

sub setShutter {
	my ($this, $shutter) = @_;

	if ($shutter) {
		@{$this->{bufferarray}} = (&GL_BACK_LEFT, &GL_BACK_RIGHT);
	} else {
		@{$this->{bufferarray}} = (&GL_BACK);
	}
}

###############################################################
#
# Private functions, used by other browser modules below

if (0) {
	VRML::VRMLFunc::render_verbose(1);
	$VRML::verbose = 1;
}

sub new {
	my(
	   $type,
	   $cocoaContext, # SD reference to Aqua GL context
	   $fullscreen,
	   $shutter,
	   $eyedist,
	   $parent,
	   $screendist,
	   %pars
	  ) = @_;
	my $this = bless {}, $type;

	my($w,$h) = (450, 300);
	my $x = 0;
	my $y = 0;
	my $wintitle;
	my $pi = atan2(1,1) * 4;

	if (my $g = $pars{Geometry}) {
	    $g =~ /^(\d+)x(\d+)(?:([+-]\d+)([+-]\d+))?$/ 
			or die("Invalid geometry string '$g' given to GLBackend");
	    ($w, $h, $x, $y) = ($1, $2, $3, $4);
	    # print "GEOMETRY: $w $h $x $y\n";
	}

	if (defined $cocoaContext)
	{
		eval 'use Foundation';
		eval 'use Foundation::Functions';
		eval 'use AppKit';
		eval 'use AppKit::Functions';
	}

	$this->{W} = $w;
	$this->{H} = $h;
	$this->{CONTEXT} = $cocoaContext;
	$this->{CROSSCURSOR} = undef;
	$this->{ARROWCURSOR} = undef;
	
	if (!(defined $cocoaContext))
	{
	if ($shutter) {
		@{$this->{bufferarray}} = (&GL_BACK_LEFT, &GL_BACK_RIGHT);
	} else {
		@{$this->{bufferarray}} = (&GL_BACK);
	}
	}

	my @db = &GLX_DOUBLEBUFFER;

	if ($VRML::offline) {
		$x = -1; @db=();
	}

	# Window title. If the netscape option is on the command
	# line, use it's parameter for the window name
	if ($VRML::ENV{AS_PLUGIN}) {
		$wintitle = $VRML::ENV{AS_PLUGIN};
	} else {
		$wintitle = "FreeWRL";
	}

	print "Starting OpenGL\n" if $VRML::verbose;
	if (!(defined $cocoaContext))
	{
	glpOpenWindow(
				  attributes=>[],
				  mask => (KeyPressMask | &KeyReleaseMask | ButtonPressMask |
						   ButtonMotionMask | ButtonReleaseMask |
						   ExposureMask | StructureNotifyMask |
						   PointerMotionMask),
				  "x" => $x,
				  "y" => $y,
				  width => $w,
				  height => $h,
				  shutter=>$shutter,
				  parent=>$parent,
				  fs => $fullscreen,
				  wintitle => $wintitle,
				 );

	} else {
		$cocoaContext->makeCurrentContext;
	}

	# Initialize OpenGL functions/variables for our use.
	glpOpenGLInitialize();

	if (defined $cocoaContext)
	{
		$cocoaContext->flushBuffer;
	}

	print "STARTED OPENGL\n" if $VRML::verbose;

	if ($VRML::offline) {
		$this->doconfig($w,$h);
	}
	VRML::VRMLFunc::set_viewer_type(EXAMINE);

	VRML::VRMLFunc::do_set_eyehalf(
								   $eyedist/2.0,
								   atan2($eyedist/2.0,$screendist)*360.0/(2.0*$pi)
								  );
	return $this;
}

sub setEyeDist {
	my ($this, $eyedist, $screendist) = @_;
	my $pi = atan2(1,1) * 4;
	VRML::VRMLFunc::do_set_eyehalf(
								   $eyedist/2.0,
								   atan2($eyedist/2.0,$screendist)*360.0/(2.0*$pi)
								  );
}

# SD - adding call to shut down Open GL screen
sub close_screen {
	glXDestroyContext();
}

# Set the sub used to go forwards/backwards in VP list
sub set_vp_sub {
	$_[0]{VPSub} = $_[1];
}

sub quitpressed {
	return delete $_[0]{QuitPressed};
}

# process xevents in browser's event loop
sub handle_events {
	my($this, $time) = @_;
	
	if (!(defined $this->{CONTEXT})) {
		while (XPending()) {
			# print "UPDS: Xpend:",XPending(),"\n";
			my @e = &glpXNextEvent();
			# print "EVENT $e[0] $e[1] $e[2] !!!\n";
			if ($e[0] == &ConfigureNotify) {
				$this->{W} = $e[1];
				$this->{H} = $e[2];
			}
			$this->event($time, @e);
		}
		$this->finish_event();
	}
}

sub updateCoords {
	my ($this, $xcoor, $ycoor) = @_;

	$this->{W} = $xcoor;
	$this->{H} = $ycoor;
	$this->{CONTEXT}->update;
}

sub set_root { $_[0]{Root} = $_[1] }


#event: Handles external sensory events (keys, mouse, etc)
sub event {
	my $w;
	my($this, $time, $type, @args) = @_;
	my $code;
	my $but;

	# JAS - uncomment this to see all events, even mouse movenemts
	# print "EVENT $this $type $args[0] $args[1] $args[2]\n";

	#print "VRML::GLBackEnd::event: $time, $type, [ ", join(", ", @args)," ]\n";

	if ($type == &MotionNotify) {

		my $but;
		# print "MOT!\n";
		if (!(defined $this->{CONTEXT})) {
			if ($args[0] & (&Button1Mask)) {
				$but = 1;
			} elsif ($args[0] & (&Button2Mask)) {
				$but = 2;
			} elsif ($args[0] & (&Button3Mask)) {
				$but = 3;
			}
		} else {
			if ($args[0] == (&Button1)) {
				$but = 1;
			} elsif ($args[0] == (&Button2)) {
				$but = 2;
			} elsif ($args[0] == (&Button3)) {
				$but = 3;
			}
		}
		# print "BUT: $but\n";
		$this->{MX} = $args[1]; $this->{MY} = $args[2];
		$this->{BUT} = $but;
		$this->{SENSMOVE} = 1;
		push @{$this->{BUTEV}}, [MOVE, undef, $args[1], $args[2]];
		undef $this->{EDone} if ($but > 0);

    } elsif ($type == &ButtonPress) {
		# print "BP!\n";
		if ($args[0] == (&Button1)) {
			$but = 1;
		} elsif ($args[0] == (&Button2)) {
			$but = 2;
		} elsif ($args[0] == (&Button3)) {
			$but = 3;
		}
		$this->{MX} = $args[1]; $this->{MY} = $args[2];
		my $x = $args[1]/$this->{W};
		my $y = $args[2]/$this->{H};
		if ($but == 1 or $but == 3) {
			# print "BPRESS $but $x $y\n";

			# If this is not on a sensitive node....
			if ($cursortype == 0) {
				#AK - #$this->{Viewer}->handle("PRESS",$but,$x,$y, $args[5] & &ShiftMask, $args[5] & &ControlMask);
				VRML::VRMLFunc::do_handle("PRESS", $but, $x, $y);
			}
		}
		push @{$this->{BUTEV}}, [PRESS, $but, $args[1], $args[2]];
		$this->{BUT} = $but;
		$this->{SENSBUT} = $but;
		undef $this->{EDone};

    } elsif ($type == &ButtonRelease) {
		# print "BR\n";
		if ($args[0] == (&Button1)) {
			$but = 1;
		} elsif ($args[0] == (&Button2)) {
			$but = 2;
		} elsif ($args[0] == (&Button3)) {
			$but = 3;
		}
		push @{$this->{BUTEV}}, [RELEASE, $but, $args[1], $args[2]];
		$this->finish_event;
		if ($cursortype == 0) {			#ie, it is not a sensitive node being released
			VRML::VRMLFunc::do_handle("RELEASE", $but, 0, 0);
		}

		$this->{SENSBUTREL} = $but;
		undef $this->{BUT};

    } elsif ($type == &KeyPress) {
		#print "KEY: $args[0] $args[1] $args[2] $args[3]\n";
		my $key = lc $args[0];

		if ($key eq "e") {
			VRML::VRMLFunc::set_viewer_type(EXAMINE);
		} elsif ($key eq "w") {
			VRML::VRMLFunc::set_viewer_type(WALK);
		} elsif ($key eq "d") {
			VRML::VRMLFunc::set_viewer_type(FLY);
		} elsif ($key eq "f") {
			VRML::VRMLFunc::set_viewer_type(EXFLY);
		} elsif ($key eq "h") {
			VRML::VRMLFunc::do_toggle_headlight();
		} elsif ($key eq "/") {
			VRML::VRMLFunc::do_print_viewer();

			# Sequence / Single Image saving ###########
		} elsif ($key eq "s") {

			# Sequence saving ##########################
			if ($main::seq) {
				$main::saving = ! $main::saving ;
				print "Saving ",$main::saving ? "on" : "off","\n" ;

				# At end of sequence, convert raw
				# images to a gif or ppm's 
				if (! $main::saving) {
					VRML::Browser::convert_raw_sequence();
				} else {				# Start new sequence
					@main::saved = ();	# Reset list of images
					$main::seqcnt = 0;
				}
				# Single image
			} else {
				print "Saving snapshot\n";
				VRML::Browser::save_snapshot($this);
			}
		} elsif ($key eq "q") {
			# if in netscape, don't do the quitpressed!
			if (!$VRML::ENV{AS_PLUGIN}) {
				$this->{QuitPressed} = 1;
			}

		} elsif ($key eq "v") {
			# print "NEXT VP\n";
			if ($this->{VPSub}) {
				$this->{VPSub}->(1);
			} else {
				die("No VPSUB");
			}
		} elsif ($key eq "c") { # toggle collision detection on/off
			if ($becollision == 1) {
				$becollision = 0;
			} else {
				$becollision = 1;
			}
		} elsif (!VRML::VRMLFunc::use_keys()) {
			if ($key eq "k") {
				#AK - #$this->{Viewer}->handle("PRESS", 1, 0.5, 0.5);
				#AK - #$this->{Viewer}->handle("DRAG", 1, 0.5, 0.4);
				VRML::VRMLFunc::do_handle("PRESS", 1, 0.5, 0.5);
				VRML::VRMLFunc::do_handle("DRAG", 1, 0.5, 0.4);
			} elsif ($key eq "j") {
				#AK - #$this->{Viewer}->handle("PRESS", 1, 0.5, 0.5);
				#AK - #$this->{Viewer}->handle("DRAG", 1, 0.5, 0.6);
				VRML::VRMLFunc::do_handle("PRESS", 1, 0.5, 0.5);
				VRML::VRMLFunc::do_handle("DRAG", 1, 0.5, 0.6);
			} elsif ($key eq "l") {
				#AK - #$this->{Viewer}->handle("PRESS", 1, 0.5, 0.5);
				#AK - #$this->{Viewer}->handle("DRAG", 1, 0.6, 0.5);
				VRML::VRMLFunc::do_handle("PRESS", 1, 0.5, 0.5);
				VRML::VRMLFunc::do_handle("DRAG", 1, 0.6, 0.5);
			} elsif ($key eq "h") {
				#AK - #$this->{Viewer}->handle("PRESS", 1, 0.5, 0.5);
				#AK - #$this->{Viewer}->handle("DRAG", 1, 0.4, 0.5);
				VRML::VRMLFunc::do_handle("PRESS", 1, 0.5, 0.5);
				VRML::VRMLFunc::do_handle("DRAG", 1, 0.4, 0.5);
			}
		} else {
			VRML::VRMLFunc::do_handle_key($args[0]);
		}
	} elsif ($type == &KeyRelease) {
		if (VRML::VRMLFunc::use_keys()) {
			VRML::VRMLFunc::do_handle_keyrelease($args[0]);
		}
    }
}
	
sub finish_event {
	my($this) = @_;
	
	return if $this->{EDone};
	my $x = $this->{MX} / $this->{W};
	my $y = $this->{MY} / $this->{H};
	my $but = $this->{BUT};
	if(($but == 1 or $but == 3) and $cursortype==0) {
	    #AK - #$this->{Viewer}->handle("DRAG", $but, $x, $y);
		VRML::VRMLFunc::do_handle("DRAG", $but, $x, $y);
	    # print "FE: $but $x $y\n";
	} elsif($but == 2) {
	    $this->{MCLICK} = 1;
	    $this->{MCLICKO} = 1;
	    $this->{MOX} = $this->{MX}; $this->{MOY} = $this->{MY}
	}
	$this->{EDone} = 1;
}


sub new_node {
	my($this,$type,$fields) = @_;
	
	my $node = {
		Type => $type,
		CNode => VRML::CU::alloc_struct_be($type),
	};

	$this->set_fields($node,$fields);
	return $node;
}

sub set_fields {
	my($this,$node,$fields) = @_;
	for (keys %$fields) {
		my $value = $fields->{$_};
		VRML::CU::set_field_be($node->{CNode}, $node->{Type}, $_, $fields->{$_});
    }
}

sub set_sensitive {
	my($this,$node,$sub) = @_;

	print "\nBE SET SENS Node: $node, Sub: $sub for this $this\n"
	  if $VRML::verbose::glsens;

	push @{$this->{Sens}}, [$node, $sub];
	$this->{SensC}{$node->{CNode}} = $node;
	$this->{SensR}{$node->{CNode}} = $sub;
}

sub delete_node {
	my($this,$node) = @_;

	VRML::CU::free_struct_be($node->{CNode}, $node->{Type});
}

sub setup_projection {
	my($this,$pick,$x,$y) = @_;
	my $i = pack ("i",0);
	
	glMatrixMode(&GL_PROJECTION);
	
	glViewport(0,0,$this->{W},$this->{H});
	glLoadIdentity();
	# print "SVP: $this->{W} $this->{H}\n";
	if($pick) 
	  { # We are picking for mouse events ..
	    my $vp = pack("i4",0,0,0,0);
	    glGetIntegerv(&GL_VIEWPORT, $vp);
	    # print "VPORT: ",(join ",",unpack"i*",$vp),"\n";
	    # print "PM: $this->{MX} $this->{MY} 3 3\n";
	    my @vp = unpack("i*",$vp);
	    print "Pick".(join ', ',$this->{MX}, $vp[3]-$this->{MY}, 3, 3, @vp)."\n"
	      if $VRML::verbose::glsens;
	    glupPickMatrix($x, $vp[3]-$y, 3, 3, @vp);
	  }

	VRML::VRMLFunc::setup_projection ($this->{H} != 0 ? $this->{W}/$this->{H} : $this->{W});
	glPrintError("GLBackEnd::setup_projection");
}

sub setup_viewpoint {
	my($this,$node) = @_;
	my $viewpoint = 0;

	glMatrixMode(&GL_MODELVIEW); # this should be assumed , here for safety.
	glLoadIdentity();

	# Make viewpoint, adds offset in stereo mode.
	# FIXME: I think it also adds offset of left eye in mono mode.

	VRML::VRMLFunc::do_viewer_togl();

	VRML::VRMLFunc::render_hier($node, 	# Node
				    &VF_Viewpoint,# render view point
				    $viewpoint);# what view point
	glPrintError("GLBackEnd::setup_viewpoint");

}



sub render_pre {
	my ($this) = @_;

	my ($node,$viewpoint) = @{$this}{Root, Viewpoint};
	$node = $node->{CNode};

	# 1. Set up projection
	$this->setup_projection();


	# 2. Headlight, initialized here where we have the modelview matrix to Identity.
	# FIXME: position of light sould actually be offset a little (towards the center)
	# when in stereo mode.
	glLoadIdentity();

	if (VRML::VRMLFunc::do_get_headlight()) {
		BackEndHeadlightOn();
	}


	# 3. Viewpoint
	$this->setup_viewpoint($node); #need this to render collisions correctly

	# 4. Collisions
	if ($becollision == 1) {
		$this->render_collisions();
		$this->setup_viewpoint($node); #update viewer position after collision, to 
						#give accurate info to Proximity sensors.
	}

	# 5. render hierarchy - proximity
	VRML::VRMLFunc::render_hier($node,  # Node
				&VF_Proximity, 
				0); # what view point

	glPrintError("GLBackend::render_pre");
}


sub render_collisions {
    my ($this) = @_;
    my ($node, $viewpoint) = @{$this}{Root, Viewpoint};
    $node = $node->{CNode};
	VRML::VRMLFunc::do_render_collisions($node);

    #AK - #VRML::VRMLFunc::reset_collisionoffset();

    #AK - #VRML::VRMLFunc::render_hier($node,  # Node
	#AK - #			&VF_Collision, 
	#AK - #			0); # what view point

    #AK - #my($x,$y,$z);
    #AK - #VRML::VRMLFunc::get_collisionoffset($x,$y,$z);
#    print "$x,$y,$z";

	#AK - replace this!!! XXX
    #AK - #my $nv = $this->{Viewer}->{Quat}->invert->rotate([$x,$y,$z]);
    #AK - #for(0..2) {$this->{Viewer}->{Pos}[$_] += $nv->[$_]}
}

# Given root node of scene, render it all
sub render {
    my ($this) = @_;
    my ($node,$viewpoint) = @{$this}{Root, Viewpoint};
    my ($i);
    $node = $node->{CNode};
    $viewpoint = $viewpoint->{CNode};

    print "Render: root $node\n" if ($VRML::verbose::be);
	
    foreach $i (@{$this->{bufferarray}}) {

		VRML::VRMLFunc::do_set_buffer($i);

		glDrawBuffer(VRML::VRMLFunc::do_get_buffer());

		# turn lights off, and clear buffer bits
		BackEndClearBuffer();
		BackEndLightsOff();

		# turn light #0 off only if it is not a headlight.
		if (! VRML::VRMLFunc::do_get_headlight()) {
			BackEndHeadlightOff();
		}

        # Correct Viewpoint, only needed when in stereo mode.
		$this->setup_viewpoint($node) if @{$this->{bufferarray}} != 1;

		# Other lights
		glPrintError("GLBackEnd::render, before render_hier");

		VRML::VRMLFunc::render_hier($node, # Node
									&VF_Lights,	# render lights
									0); # what view point
		glPrintError("GLBackEnd::render, VRML::VRMLFUNC::render_hier(VF_Lights)");

		# 4. Nodes (not the blended ones)

		VRML::VRMLFunc::render_hier($node, # Node
									&VF_Geom, # render geoms
									0);	# what view point
		glPrintError("GLBackEnd::render, VRML::VRMLFUNC::render_hier(VF_Geom)");
    }

    if (defined $this->{CONTEXT}) {
		$this->{CONTEXT}->flushBuffer;
    } else {
		glXSwapBuffers();
    }

    # Do selection
    if (@{$this->{Sens}}) {
		# print "SENS: $this->{MOUSEGRABBED}\n";
		# my $b = delete $this->{SENSBUT};
		# my $sb = delete $this->{SENSBUTREL};
		# my $m = delete $this->{SENSMOVE};
		print "SENSING\n" if $VRML::verbose::glsens;

		for (my $i = 1; $i < scalar @{$this->{BUTEV}}; $i ++) {
			if ($this->{BUTEV}[$i-1][0] eq "MOVE" and
				$this->{BUTEV}[$i][0] eq "MOVE") {
				splice @{$this->{BUTEV}}, $i-1, 1;
				$i --;
			}
		}

		for (@{$this->{BUTEV}}) {
			print "BUTEV: $_->[0]\n" if $VRML::verbose::glsens;
	
			# loop through all the "Sensor" nodes.	
			for (@{$this->{Sens}}) {
				if ((defined $_->[0]) && (defined $_->[1])) {
					# print "GLBackEnd.pm, going throuh Sens for $_ : \n";

					VRML::VRMLFunc::zero_hits($_->[0]{CNode});
					VRML::VRMLFunc::set_sensitive($_->[0]{CNode},1);
				} else {
					print "BUTEV - should remove this one from the list\n"
						if $VRML::verbose::glsens;
				}
			}
			if ($this->{MOUSEGRABBED}) {
				VRML::VRMLFunc::set_hypersensitive($this->{MOUSEGRABBED});
			} else {
				VRML::VRMLFunc::set_hypersensitive(0);
			}

			my $nints = 100;
			my $s = pack("i$nints");
		

			$this->setup_projection(1, $_->[2], $_->[3]);
			$this->setup_viewpoint($node);
	
			# sensitive nodes. Note, the routing no longer handles values,
			# only events; the pos, hyperhits, are stored in C structures
			# to save saving/setting from Perl. JAS.
	
			VRML::VRMLFunc::render_hier($node, # Node
										&VF_Sensitive, # render sensitive
										0);	# what view point

			# print "SENS_BR: $b\n" if $VRML::verbose::glsens;

			my $p = VRML::VRMLFunc::get_rayhit();
			if ($this->{MOUSEGRABBED}) {
				if ($_->[0] eq "RELEASE") {
					# XXX The position at release is garbage :(
					my $rout = $this->{SensR}{$this->{MOUSEGRABBED}};
					$rout->("RELEASE", 0, 1);
					undef $this->{MOUSEGRABBED};
				} elsif ($_->[0] eq "MOVE") {
					my $over = ($this->{MOUSEGRABBED} == $p);
					my $rout = $this->{SensR}{$this->{MOUSEGRABBED}};
					$rout->("",1,$over);
	
					die("No hyperhit??? REPORT BUG!")
						if (!VRML::VRMLFunc::get_hyperhit());
			
					my $rout = $this->{SensR}{$this->{MOUSEGRABBED}};
					$rout->("DRAG", 1, $over);
				}
			} else {
				if (defined $this->{SensC}{$p}) {
					$cursortype = 1;

					my $rout = $this->{SensR}{$p};
			
					if ($_->[0] eq "MOVE") {
						$rout->("",1,1);
					}
			
					if ($_->[0] eq "PRESS") {
						$rout->("PRESS",0,1);
						$this->{MOUSEGRABBED} = $p;
					}
					# if($sb) {
					# 	$rout->("RELEASE",0,1);
					# }
				} else {	
					$cursortype=0;
					print "No hit: $p\n"
						if $VRML::verbose::glsens;
				}

				print "MOUSOVER: $this->{MOUSOVER}, p: $p\n"
					if $VRML::verbose::glsens;

				if (defined $this->{MOUSOVER} and $this->{MOUSOVER}
					!= $p and defined($this->{SensC}{$this->{MOUSOVER}})) {
					# print "in final MOUSOVER code\n";
					my $rout = $this->{SensR}{$this->{MOUSOVER}};
					$rout->("",1,0);
				}
				$this->{MOUSOVER} = $p;
			}
		}
    }
	
    $#{$this->{BUTEV}} = -1;

    # determine whether cursor should be "sensor".
    if ($cursortype != $curcursor) {
		$curcursor = $cursortype;
		if ($cursortype == 0) {
			if (!(defined $this->{CONTEXT})) {
				arrow_cursor();
			} else {
				$this->{ARROWCURSOR}->set();
			}
		} else {
			if (!(defined $this->{CONTEXT})) {
				sensor_cursor();
			} else {
				$this->{CROSSCURSOR}->set();
			}
		}
    }
    glPrintError("GLBackEnd::render");
}

1;



