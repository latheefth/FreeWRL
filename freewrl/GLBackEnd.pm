# Copyright (C) 1998 Tuomas J. Lukka, 1999 John Stewart CRC Canada.
# DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
# See the GNU Library General Public License (file COPYING in the distribution)
# for conditions of use and redistribution.


# Implement OpenGL backend using the C structs.

package VRML::GLBackEnd;

use VRML::VRMLFunc;
require "VRML/VRMLCU.pm";

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
	$this->{W} = $w;
	$this->{H} = $h;
	$this->{CONTEXT} = $cocoaContext;
	$this->{CROSSCURSOR} = undef;
	$this->{ARROWCURSOR} = undef;
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

#JAS # SD - adding call to shut down Open GL screen
#JAS 
#JAS # Set the sub used to go forwards/backwards in VP list
#JAS sub set_vp_sub {
#JAS 	$_[0]{VPSub} = $_[1];
#JAS }
#JAS
#JASsub quitpressed {
#JAS	return delete $_[0]{QuitPressed};
#JAS}

sub updateCoords {
	my ($this, $xcoor, $ycoor) = @_;

	$this->{W} = $xcoor;
	$this->{H} = $ycoor;
	$this->{CONTEXT}->update;
}

sub set_root { $_[0]{Root} = $_[1] }

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

sub delete_node {
	my($this,$node) = @_;

	VRML::CU::free_struct_be($node->{CNode}, $node->{Type});
}

1;

