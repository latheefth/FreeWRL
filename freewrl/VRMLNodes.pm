#
# $Id$
#
# Copyright (C) 1998 Tuomas J. Lukka 1999 John Stewart CRC Canada.
# DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
# See the GNU Library General Public License (file COPYING in the distribution)
# for conditions of use and redistribution.

# The event subs:
#  Initialize($node,$fields,$time,$scene): called when node created in world
#  EventsProcessed($node,$fields,$time): called when events have been received
#					and processed by other routines 
#  field/eventname($node,$fields,$value,$time) 
#	(if field/exposedField, the name, otherwise exact eventname)
#		: called when eventIn received in that event.
#  ClockTick($node,$fields,$time): called at each clocktick if exists 
#	(only timesensor-type nodes)
#
# default field/eventname: $t->set_field(...,...), if event received,
#  field is not set so that it can be ignored (e.g. TimeSensor)
#  set_field returns the eventout to send also...!!
#
#  all these can return lists of eventOuts to send.
#
# XXXXXXXXXXXX
#  Problem: Interpolators send an event at startup and they shouldn't...
#  TouchSensor: how to get hitpoint, normal and texcoords without 
#   		spending an ungodly amount of time at it?

package VRML::NodeType; # Same for internal and external!

my @vps;	# viewpoint Scenes.
my @vpn;	# viewpoint Nodes.
my $vpno = 1;
my $globalAudioSource = 0;  # count of audio sources
my $globalDuration = -1.0;	# have problems setting duration variable from C for AudioClips.
my $SoundMaterial;	    # is the parent a Sound or not? (MovieTextures...)

# Viewpoints are stored in the browser rather in the 
# individual scenes...

sub register_vp {
	my ($scene, $node) = @_;
	# print "Node::register_vp $scene $node\n";
	push @vpn, $node;
	push @vps, $scene;
        # print "VRML::NodeIntern::register_vp, viewpoint is ",$node->{Fields}{description},"\n";
         #         print "ref t ", ref $node,"\n";
         #         print "ref t backend ", ref $node->{BackEnd},"\n";
         #         print "t backend ", $node->{BackEnd},"\n";
         #         print "ref t backnode ", ref $node->{BackNode},"\n";
         #         print "t backNode ", $node->{BackNode},"\n";
         #         print "ref t protoexp ", ref $node->{ProtoExp},"\n";
         #         print "t protoexp ", $node->{ProtoExp},"\n";
         #         if (defined $node->{IsProto}) print "t isproto ", $node->{IsProto} ,"\n";
# 		print "VRML::NODE::end\n";

}

sub set_next_vp {
	$vpno++;
	if ($vpno > $#vpn) {$vpno = 0;}
	# print "set_next_vp, next vp to get is number $vpno\n";
}

sub get_vp_node { # print "get_vp_node, returning ", $vpn[$vpno],"\n";
return $vpn[$vpno];}

sub get_vp_scene {# print "get_vp_scene, returning ", $vps[$vpno],"\n";
return $vps[$vpno];}



#########################################################
# The routines below implement the browser object interface.

sub getName { return "FreeWRL VRML Browser" }

# EG Die unless perl scripts are enabled
sub check_perl_script {
 	if(!$VRML::DO_PERL) {
 		die <<EOF ;

Perl scripts are currently unsafe as they are trusted and run in the
main interpreter. If you are sure of your code (i.e. you have written
it or know the writer), use freewrl's "-ps" (perl scripts) option.

FreeWRL should soon be modified to run untrusted Perl code in Safe
compartments. Until that, Perl scripts are disabled by default.

EOF
 	}
 }

## EG
## perl_script_output ( $on )
##
## If a script output file has been specified with the "-psout" option, and
##   If $on is true,  then
##     the filehandle corresponding to the script output file is selected.
##   If $on is false, then
##     the previously selected filehandle is selected.
##
## The following VRML:: variables are used
BEGIN { 
								# Script output file name (set with -psout)
  $VRML::script_out_file = "" unless
	defined $VRML::script_out_file ;
  $VRML::script_out_open = 0;	# True if output file is open
								# True if script output file is
								# currently selected
  $VRML::script_out_selected = 0 ;
  $VRML::LAST_HANDLE = 0 ;		# Filehandle in use before script call
}

sub perl_script_output {

  my $on = shift ;
  my $v = 0 ;					# Local verbose option

  if ($VRML::script_out_file) {	# If output is re-routed

								# I've already opened the file ....
    if ($VRML::script_out_open) {
								# ... but has someone deleted it?
	  if (! -f $VRML::script_out_file) {
		print "Script output file disappeared! (no problem)\n" if $v;
		close VRML_SCRIPT_OUT;
		$VRML::script_out_open = 0;
	  }
	}
								# Eventually open the file
    if (!$VRML::script_out_open) {
      open VRML_SCRIPT_OUT, ">$VRML::script_out_file"
		or die "Can't open script output file '$VRML::script_out_file'\n";
      $VRML::script_out_open = 1;
      print "Opened script output file '$VRML::script_out_file'\n" if $v;
    }

    if ($on) {					# select script output filehandle
      if (!$VRML::script_out_selected){
		$VRML::LAST_HANDLE = select VRML_SCRIPT_OUT;
		$| = 1;
		$VRML::script_out_selected = 1;
		print $VRML::LAST_HANDLE "VRML_SCRIPT_OUT selected\n" if $v;
      } else {
		print $VRML::LAST_HANDLE "VRML_SCRIPT_OUT already selected\n" if $v;
      }
    } else {					# select previous output filehandle
      select $VRML::LAST_HANDLE;
      $VRML::script_out_selected = 0;
      print "Selected previous filehandle '$VRML::LAST_HANDLE'\n" if $v;
    }
  } else {
    print "No script output file specified\n" if $v;
  }
}								# End perl_script_output

## EG : Just a little debugging function
sub show_stack {
  my $n = @_ ? shift : 1 ;
  for $i (2..$n+1) {
	my @a = caller ($i);
	if (@a) {
	  $a[1] =~ s{^.*/}{};
	  print "package $a[0], file $a[1]:$a[2], sub $a[3], wantarray $a[5]\n" ;
	}
  }
}
use Data::Dumper ;

# EG : returns the list of names of fields that are visible in the
# script
sub script_variables {
  ## my $fields = shift ;
  ## my @res = grep { ! /mustEvaluate|directOutput/ } sort keys %$fields ;
  ## map {print "$_ has value ", Dumper ($fields->{$_}) } @res ;
  ## @res;
  grep { ! /mustEvaluate|directOutput/ } sort keys %{shift()};
}

package MTS ;			# My tied Scalar : Will be used to tie scalars
				# to fields and eventIn for perl scripting.

use Exporter ;
@ISA = qw(Exporter);
@EXPORT = qw( with ) ;

require Tie::Scalar;

sub TIESCALAR {
  ### my $class = shift;
  ## my $self = shift;
  ## bless $self;
  ### bless shift ;
  bless $_[1];
}

sub FETCH {
  ## my $self = shift;
  ## $$self ;
  ### ${shift()};
  ${$_[0]};
}

sub STORE {
  ## my $self = shift;
  ## $$self = shift ;
  ${$_[0]} = $_[1];
}


package VRML::NodeType;

# JAS - many nodes use the same type of algorithm for ClockTicks and each had 
# JAS - the same code (kind of) Make it all happen here. Refer to the VRML
# JAS - spec 4.6.9- Time Dependent Nodes.


sub ClockTick_TimeDepNodes {
	my($t,$f,$tick) = @_;

	# print "CT in ClockTick_TimeDepNodes - $t $f $tick\n";
	# can we possibly have started yet?
	if($tick < $f->{startTime}) {
		return();
	}

	my $oldstatus = $f->{isActive};
	my @e;

	my $time;
	my $duration = 1.0;
	my $speed = 1.0;	# speed, pitch, depending on calling node syntax

	# used only for MovieTexture Video mode only
	my $frac = 1;
	my $highest = 1;
	my $lowest = 0;


	if ($f->{__type} == 0) {
		# MovieTexture - Texture mode
		$speed = $f->{speed};
		$frac = $f->{__ctex};
		$lowest = $f->{__texture0_};
		$highest = $f->{__texture1_};

		# sanity check - avoids divide by zero problems below
		if ($lowest >= $highest) {
			$lowest = $highest-1;
		}	
		$duration = ($highest - $lowest)/30;

	} elsif ($f->{__type} == 1) {
		# AudioClip
		$speed = $f->{pitch};
		if ($f->{__duration} < 0.0) {
			# lets see if it has finally been registered yet.
			$f->{__duration} = VRML::VRMLFunc::return_Duration($f->{__sourceNumber});
		}
		$duration = $f->{__duration};
		# duration not available from clip
		if ($duration <=0.0) {$duration = 1.0;}

	} elsif ($f->{__type} == 2) {
		# TimeSensor
		$duration = $f->{cycleInterval};
		
		
	}

	print "ct, start ",$f->{startTime}," stop ",$f->{stopTime},"tick $tick status ",
		$f->{isActive}," initTime ",$f->{__inittime},"\n"
			if $VRML::verbose::timesens;

	# what we do now depends on whether we are active or not

	if ($f->{isActive} == 1) {   # active - should we stop?

		if ($tick > $f->{stopTime}) {
			if ($f->{startTime} >= $f->{stopTime}) { 
				# cases 1 and 2
				if (!($f->{loop})) {
					if ($speed != 0) {
					    if ($tick >= ($f->{startTime} + 
							abs($duration/$speed))) {
						#print "stopping case x\n";
						$f->{isActive} = 0;
						$f->{stopTime} = $tick;
					    }
					}
				} else {
				#	print "stopping case y\n";
				#	$f->{isActive} = 0;
				#	$f->{stopTime} = $tick;
				}
			} else {
				#print "stopping case z\n";
				$f->{isActive} = 0;
				$f->{stopTime} = $tick;
			}
		}
	}

	# immediately process start events; as per spec. 
	if ($f->{isActive} == 0) {   # active - should we start?
		if ($tick >= $f->{startTime}) {
			# We just might need to start running

			if ($tick >= $f->{stopTime}) {
				# lets look at the initial conditions; have not had a stoptime
				# event (yet)

				if ($f->{loop}) {
					if ($f->{startTime} >= $f->{stopTime}) {
						# VRML standards, table 4.2 case 2 
						$f->{startTime} = $tick;
						$f->{isActive} = 1;
						#print "case 2 here\n";
					}
				} elsif ($f->{startTime} >= $f->{stopTime}) {
					if ($f->{startTime} > $f->{__inittime}) { #ie, we have an event
						#print "case 1 here\n";
						# we should be running 
						# VRML standards, table 4.2 case 1 
						$f->{startTime} = $tick;
						$f->{isActive} = 1;
					}
				}
			} else {
				#print "case 3 here\n";
				# we should be running -  
				# VRML standards, table 4.2 cases 1 and 2 and 3
				$f->{startTime} = $tick;
				$f->{isActive} = 1;
			}
		}

		# if we have gone active, make sure that the first image is displayed
		# for MovieTextures
		if (($f->{isActive} == 1) && ($f->{__type} == 0)) { $f->{__ctex} = -1; }

		# TimeSensor cycleTime - event at start or once per cycle.
		if (($f->{isActive} == 1)  && ($f->{__type} == 2)) {
			$f->{__ctflag} = 10;  # force code below to generate event
		}

	}
	if ($oldstatus != $f->{isActive}) {
		push @e, [$t, "isActive", $f->{isActive}];
		if ($f->{__type} == 1) {
			# tell SoundEngine that this source has changed. 
			VRML::VRMLFunc::SetAudioActive($f->{__sourceNumber},$f->{isActive});
		}
	}

	if($f->{isActive} == 1) {
		if ($f->{__type} == 0) {
			# MovieTextures
			# calculate what fraction we should be 
	 		$time = ($tick - $f->{startTime}) * $speed / $duration;
			$frac = $time - int $time;
	
			# negative speed? - only MovieTextures will vary this.
			if ($speed < 0) {
				$frac = 1+$frac; # frac will be *negative*
			} elsif ($speed == 0) {
				$frac = 0;
			}
	
			# frac will tell us what texture frame we should apply...
			$frac = int ($frac*($highest-$lowest+1) + $lowest);
	
			# verify parameters
			if ($frac < $lowest){ 
				#print "frac $frac lowest $lowest\n"; 
				$frac = $lowest
			}
			if ($frac > $highest){ 
				#print "frac $frac highest $highest\n";
				$frac = $highest
			}
	
			if ($f->{__ctex} != $frac) {
				$f->{__ctex} = $frac;
				# print "pushing image $frac of $lowest $highest\n";
				push @e, [$t, "mytexfrac", $f->{__ctex}];
			}
		} elsif ($f->{__type} == 1) {
			# make sure that the SoundEngine gets updates, even if no geometry nodes
			# require it.
			VRML::OpenGL::set_render_frame();
		} elsif ($f->{__type} == 2) {
			# TimeSensors
			# calculate what fraction we should be 
	 		$time = ($tick - $f->{startTime}) * $speed / $duration;

			if ($f->{loop}) {
				$frac = $time - int $time;
			} else {
				$frac = ($time > 1 ? 1 : $time);
			}

			# cycleTime events once at start, and once every loop.
			if ($frac < $f->{__ctflag}) {
				# print "cycleTime event\n";
                               	push @e, [$t, cycleTime, $tick];
			}
			$f->{__ctflag} = $frac;
	
			# time  and fraction_changed events
			push @e, [$t, "time", $tick];
			push @e, [$t, fraction_changed, $frac];
		}
	}
	return @e;
}


# AK - Grouping nodes (see VRML97 4.6.5) that have children use essentially
# AK - the same code to add & remove child nodes.
sub addChildren_GroupingNodes {
	my ($node, $fields, $value, $time) = @_;

	# debug:
	#print "VRML::NodeType::addChildren_GroupingNodes: ", VRML::Debug::toString(\@_), "\n";

	my %children = map { $_ => 1 } @{$node->{RFields}{children}};
	for (@{$value}) {
		if (!$children{$_}) {
			push @{$node->{Fields}{children}}, $_;
		}
	}
	$node->{RFields}{children} = $node->{Fields}{children};

	@{$node->{Fields}{addChildren}} = ();
}

sub removeChildren_GroupingNodes {
	my ($node, $fields, $value, $time) = @_;

	# debug:
	#print "VRML::NodeType::removeChildren_GroupingNodes: ", VRML::Debug::toString(\@_), "\n";

	my %toremove = map { $_ => 1 } @{$value};
	my @children = grep { !$toremove{$_} } @{$node->{RFields}{children}};

	@{$node->{Fields}{children}} = ();

	push @{$node->{Fields}{children}}, @children;

	$node->{RFields}{children} = $node->{Fields}{children};

	@{$node->{Fields}{removeChildren}} = ();
}


########################################################################
#
# Image loading.
#

my %image_same_url = ();



# pass in a scene, and an MFString of urls, returns a local name to the file.
sub getURLfromMFString {
	my ($scene,$urls) = @_;

    	my $purl = $t->{PURL} = $scene->get_url;
	my $wurl = $scene->get_world_url;
	my $file;
	my $suffix;
 
	URL: for $u (@$urls) {
		next unless $u =~ /\.(\w*)$/;
		$suffix = $1;
		
		if (defined $wurl) { $file = VRML::URL::get_relative($wurl, $u, 1);
		} else { $file = VRML::URL::get_relative($purl, $u, 1); }

		# Required due to changes in VRML::URL::get_relative in URL.pm:
		if (!$file) { warn "Could not retrieve $u"; next URL; }

	}
	print "VRML::Nodes::getURLfromMFString got: $file Suffix: $suffix\n"
			if $VRML::verbose;
	return ($file,$suffix);
}


# picture image
sub init_image {
	my($name, $urlname, $t, $f, $scene, $flip) = @_;
	my $urls = $f->{$urlname};

	if ($#{$urls} == -1) { goto NO_TEXTURE; }

	my $file;
	my $suffix;
	($file,$suffix)  = getURLfromMFString($scene,$urls);

	my ($hei,$wi,$dep);
	my $tempfile = $file;

	$f->{__istemporary.$name} = 0;

        if(exists $image_same_url{$file}) {
		# we have already seen this image
		$f->{__texture.$name} = $image_same_url{$file};
		return;
	}


	$f->{__texture.$name} = VRML::OpenGL::glGenTexture();

	# save the texture number for later
	$image_same_url{$file} = $f->{__texture.$name};

	if (!($suffix  =~ /png/i || $suffix =~ /jpg/i)) {
		# Lets convert to a png, and go from there...
		# Use Imagemagick to do the conversion, and flipping.
	
		# Simply make a default user specific file by
		# attaching the username (LOGNAME from environment).

		my $lgname = $ENV{LOGNAME};
		my $tempfile_name = "/tmp/freewrl_";
		$tempfile = join '', $tempfile_name,$lgname,
			$f->{__texture.$name},".png";

		my $cmd = "$VRML::Browser::CONVERT $file $tempfile";
		my $status = system ($cmd);
		warn "$image conversion problem: '$cmd' returns $?"
			unless $status == 0;

		# tell bind_texture to remove this one
		$f->{__istemporary.$name} = 1;
	}

	$f->{__locfile.$name} = $tempfile; # store the name for later processing
		return;

	NO_TEXTURE:
	$f->{__locfile.$name} = "";
	$f->{__texture.$name} = 0;
	$f->{__istemporary.$name} = 0;
	return;
}


sub init_pixel_image {
    my($imagename, $t, $f, $scene) = @_;
    my $sfimage = $f->{$imagename};

    # now, $sfimage contains the "image" field of the node.

    if (!defined $sfimage) {
	$f->{__depth} = 0;
	$f->{__x} = 0;
	$f->{__y} = 0;
	$f->{___istemporary} = 0;
	$f->{___texture} = 0;
	$f->{__locfile} = "";
    } else {
	$sfimage =~ /\s*([0-9]+)\s+([0-9]+)\s+([0-9]+)/ogcs
	    or parsefail($_[2], "didn't match width/height/depth of SFImage");

	# should we just store the string here, or should we put it to a file?
	# Some of these textures are large - in the order of a meg. For now,
	# they are written to a file, which allows handling equivalent to
	# what happens for other images.

	$f->{__depth} = $3;
	$f->{__x} = $1;
	$f->{__y} = $2;
	$f->{__istemporary} = 1;
    	$f->{__texture} = VRML::OpenGL::glGenTexture();

	my $lgname = $ENV{LOGNAME};
	my $tempfile_name = "/tmp/freewrl_";
	my $tempfile = join '', $tempfile_name,$lgname,
			$f->{__texture},".pixtex";

    	$f->{__locfile} = $tempfile;
	
	# write ascii pixeltexture data to file
	my $fh;
	open ($fh, "> $tempfile");
	print $fh $sfimage;
	close ($fh);

    }
    return;
}



# MPEG picture image
sub init_movie_image {
	my($name, $urlname, $t, $f, $scene) = @_;
	# print "init_movie_image, name $name, urlname $urlname t $t f $f \n";

	my $purl = $t->{PURL} = $scene->get_url;
	my $wurl = $scene->get_world_url;
	my $urls = $f->{$urlname};
	if ($#{$urls} == -1) { goto NO_TEXTURE; }

	my $file;
	my $suffix;
	($file,$suffix)  = getURLfromMFString($scene,$urls);

	my $init_tex = VRML::OpenGL::glGenTexture();
	$f->{__texture0_} = $init_tex;
	$f->{__texture1_} =  VRML::VRMLFunc::read_mpg_file ($init_tex,
		$file,$f->{repeatS},$f->{repeatT});
	$f->{__locfile} = ();
	#print "init_movie, for $f, first texture is ",$f->{__texture0_},"\n";
	#print "init_movie, for $f, last texture is ",$f->{__texture1_},"\n";
	return;

 NO_TEXTURE:
    $f->{__locfile} = ();
    $f->{__texture0_} = 0;
    $f->{__texture1_} = 0;
    return;
}

# AudioClip WAV/MIDI sound file
sub init_sound {
	my($name, $urlname, $t, $f, $scene) = @_;
	#print "init_sound_image, name $name, urlname $urlname t $t f $f \n";

	my $purl = $t->{PURL} = $scene->get_url;
	my $wurl = $scene->get_world_url;
	my $urls = $f->{$urlname};
	if ($#{$urls} == -1) { goto NO_TEXTURE; }

	my $file;
	my $suffix;
	($file,$suffix)  = getURLfromMFString($scene,$urls);

	$f->{__localFileName} = $file;

	return;

NO_TEXTURE:
	return;
}


########################################################################


my $protono;

{
    # XXX When this changes, change Scene.pm: VRML::Scene::newp too --
    # the members must correspond.
    sub new {
		my($type, $name, $fields, $eventsubs) = @_;
		my $this = bless {
						  Name => $name,
						  Actions => $eventsubs,
						  Defaults => {},
						  EventOuts => {},
						  EventIns => {},
						  Fields => {},
						  FieldKinds => {},
						  FieldTypes => {}
						 },$type;

		my $t;
		for (keys %$fields) {
			if (ref $fields->{$_}[1] eq "ARRAY") {
				push @{$this->{Defaults}{$_}}, @{$fields->{$_}[1]};
			} else {
				$this->{Defaults}{$_} = $fields->{$_}[1];
			}
			$this->{FieldTypes}{$_} = $fields->{$_}[0];
			$t = $fields->{$_}[2];
			if (!defined $t) {
				die("Missing field or event type for $_ in $name");
			}

			if ($t =~ /in$/i) {
				$this->{EventIns}{$_} = $_;
			} elsif ($t =~ /out$/i) {
				$this->{EventOuts}{$_} = $_;
			} elsif ($t =~ /^exposed/) {
				## in case the 'set_' prefix or '_changed' suffix isn't
				## used for exposedFields in routes
				$this->{EventIns}{$_} = $_;
				$this->{EventOuts}{$_} = $_;
			}
			$this->{FieldKinds}{$_} = $t;
		}
		return $this;
    }
}


%VRML::Nodes::bindable = map {($_,1)} qw/
 Viewpoint
 Background
 NavigationInfo
 Fog
/;

%VRML::Nodes::initevents = map {($_,1)} qw/
 TimeSensor
 TouchSensor
 PlaneSensor
 CylinderSensor
 SphereSensor
 ProximitySensor
 VisibilitySensor
 PixelTexture

 MovieTexture
 AudioClip
 Sound
/;

# What are the transformation-hierarchy child nodes?
%VRML::Nodes::children = qw(
 Transform	children
 Group		children
 Billboard	children
 Anchor		children
 Collision	children
);

%VRML::Nodes::siblingsensitive = map {($_, 1)} qw/
 TouchSensor
 PlaneSensor
 CylinderSensor
 SphereSensor
/;

%VRML::Nodes::sensitive = map {($_, 1)} qw/
 ProximitySensor
/;

%VRML::Nodes = (
	# Internal structures, to store def and use in the right way
	DEF =>
	new VRML::NodeType("DEF",
					   { node => [SFNode, NULL, exposedField] },
					   id => [SFString, ""]
					  ),

	USE =>
	new VRML::NodeType("USE",
					   { node => [SFNode, NULL, exposedField] },
					   id => [SFString, ""]
					  ),

	Shape =>
	new VRML::NodeType ("Shape",
						{
						 appearance => [SFNode, NULL, exposedField],
						 geometry => [SFNode, NULL, exposedField]
						}
					   ),

	# Complete
	Appearance =>
	new VRML::NodeType ("Appearance",
						{
						 material => [SFNode, NULL, exposedField],
						 texture => [SFNode, NULL, exposedField],
						 textureTransform => [SFNode, NULL, exposedField]
						}
					   ),

	# Complete
	Material =>
	new VRML::NodeType ("Material",
						{
						 ambientIntensity => [SFFloat, 0.2, exposedField],
						 diffuseColor => [SFColor, [0.8, 0.8, 0.8], exposedField],
						 emissiveColor => [SFColor, [0, 0, 0], exposedField],
						 shininess => [SFFloat, 0.2, exposedField],
						 specularColor => [SFColor, [0, 0, 0], exposedField],
						 transparency => [SFFloat, 0, exposedField]
						}
					   ),

	ImageTexture =>
	new VRML::NodeType("ImageTexture",
					   {
						# original URL from VRML file
						url => [MFString, [], exposedField],
						# VRML repeatS field
						repeatS => [SFBool, 1, field],
						# VRML repeatT field
						repeatT => [SFBool, 1, field],
						# where on the local file system texture resides
						__locfile => [SFString, "", field],
						# OpenGL texture number
						__texture => [SFInt32, 0, field],
						# if we have to remove this after processing
						__istemporary =>[SFInt32, 0, field]
					   },
					   {
						Initialize => sub {
							my ($t,$f,$time,$scene) = @_;
							init_image("","url",$t,$f,$scene,1);
							return ();
						}
					   }
					  ),


# From Remi Cohen-Scali

	PixelTexture =>
	new VRML::NodeType("PixelTexture",
					   {
						# pixeltexture value, uncompiled.
						image => [SFImage, [0, 0, 0], exposedField],
						repeatS => [SFBool, 1, field],
						repeatT => [SFBool, 1, field],
						# OpenGL texture number
						__texture => [SFInt32, 0, field],
						# depth, from PixelTexture
						__depth => [SFInt32, 1, field],
						# if we have to remove the data file after processing
						__istemporary =>[SFInt32, 0, field],
						# x size, from PixelTexture
						__x => [SFInt32, 0, field],
						# y size, from PixelTexture
						__y => [SFInt32, 0, field],
						# the name that the PixelTexture data (ascii) is
						__locfile => [SFString, "", field]
						# stored in to allow C functions to parse it.
					   },
					   {
						Initialize => sub {
							my($t,$f,$time,$scene) = @_;
							init_pixel_image("image",$t,$f,$scene);
							return ();
						}
					   }
					  ),


	MovieTexture =>
	new VRML::NodeType ("MovieTexture",
						{
						 loop => [SFBool, 0, exposedField],
						 speed => [SFFloat, 1.0, exposedField],
						 startTime => [SFTime, 0, exposedField],
						 stopTime => [SFTime, 0, exposedField],
						 url => [MFString, [""], exposedField],
						 repeatS => [SFBool, 1, field],
						 repeatT => [SFBool, 1, field],
						 duration_changed => [SFTime, -1, eventOut],
						 isActive => [SFBool, 0, eventOut],
						 __locfile => [MFString, [], field],
						 # initial texture number
						 __texture0_ => [SFInt32, 0, field],
						 # last texture number
						 __texture1_ => [SFInt32, 0, field],
						 # which texture number is used
						 __ctex => [SFInt32, 0, field],
						 # time that we were initialized at
						 __inittime => [SFInt32, 0, field],
						 # internal sequence number
						 __sourceNumber => [SFInt32, 0, field],
						 # local name, as received on system
						 __localFileName => [SFString, "", exposedField],
						 # 0:MovTex Vid 1:AudioClip 2:TimeSensor 3:MT Audio
						 __type => [SFInt32, 0, exposedField]
						},
						@x = {
							  Initialize => sub {
								  my ($t,$f,$time,$scene) = @_;

								  if ($SoundMaterial eq "Sound") {
									  # Assign a source number to this source
									  $f->{__sourceNumber} = $globalAudioSource++;
									  $f->{__type} = 4;  

									  # get the file
									  init_sound("","url",$t,$f,$scene,1);
								  } else {
									  init_movie_image("","url",$t,$f,$scene);

									  # which frame to start with?
									  if ($f->{speed} >= 0) {
										  $f->{__ctex} = $f->{__texture0_};
									  } else {
										  $f->{__ctex} = $f->{__texture1_};
									  }
									  $f->{isActive} = 0; # inactive
									  $f->{__inittime} = $time;

									  #print "mt init time is $time\n";
								  }
								  # this will only be reset the next time a Sound node gets hit
								  $SoundMaterial = "unknown";
								  return ();
							  },
							  startTime => sub {
								  my($t,$f,$val) = @_;
								  #print "MT StartTime $val\n";
								  $f->{startTime} = $val;
							  },
							  # Ignore if less than startTime
							  stopTime => sub {
								  my($t,$f,$val) = @_;
								  #print "MT StopTime $val\n";
								  $f->{stopTime} = $val;
							  },

							  ClockTick => sub {
								  return ClockTick_TimeDepNodes (@_);
							  },
							 }
					   ),


	Box =>
	new VRML::NodeType("Box",
					   { size => [SFVec3f, [2, 2, 2], field] }
					  ),

	# Complete
	Cylinder =>
	new VRML::NodeType ("Cylinder",
						{
						 bottom => [SFBool, 1, field],
						 height => [SFFloat, 2.0, field],
						 radius => [SFFloat, 1.0, field],
						 side => [SFBool, 1, field],
						 top => [SFBool, 1, field]
						},
					   ),

	# Complete
	Cone =>
	new VRML::NodeType ("Cone",
						{
						 bottomRadius => [SFFloat, 1.0, field],
						 height => [SFFloat, 2.0, field],
						 side => [SFBool, 1, field],
						 bottom => [SFBool, 1, field]
						},
					   ),

	# Complete
	Coordinate =>
	new VRML::NodeType("Coordinate",
					   { point => [MFVec3f, [], exposedField] }
					  ),

	Color =>
	new VRML::NodeType("Color",
					   { color => [MFColor, [], exposedField] }
					  ),

	Normal =>
	new VRML::NodeType("Normal",
					   { vector => [MFVec3f, [], exposedField] }
					  ),

	TextureCoordinate =>
	new VRML::NodeType("TextureCoordinate",
					   { point => [MFVec2f, [], exposedField] }
					  ),


	ElevationGrid =>
	new VRML::NodeType("ElevationGrid",
					   {
						set_height => [MFFloat, undef, eventIn],
						color => [SFNode, NULL, exposedField],
						normal => [SFNode, NULL, exposedField],
						texCoord => [SFNode, NULL, exposedField],
						height => [MFFloat, [], field],
						ccw => [SFBool, 1, field],
						colorPerVertex => [SFBool, 1, field],
						creaseAngle => [SFFloat, 0, field],
						normalPerVertex => [SFBool, 1, field],
						solid => [SFBool, 1, field],
						xDimension => [SFInt32, 0, field],
						xSpacing => [SFFloat, 1.0, field],
						zDimension => [SFInt32, 0, field],
						zSpacing => [SFFloat, 1.0, field]
					   }
					  ),

	Extrusion =>
	new VRML::NodeType("Extrusion",
					   {
						set_crossSection => [MFVec2f, undef, eventIn],
						set_orientation => [MFRotation, undef, eventIn],
						set_scale => [MFVec2f, undef, eventIn],
						set_spine => [MFVec3f, undef, eventIn],
						beginCap => [SFBool, 1, field],
						ccw => [SFBool, 1, field],
						convex => [SFBool, 1, field],
						creaseAngle => [SFFloat, 0, field],
						crossSection => [MFVec2f, [[1, 1],[1, -1],[-1, -1],
												   [-1, 1],[1, 1]], field],
						endCap => [SFBool, 1, field],
						orientation => [MFRotation, [[0, 0, 1, 0]], field],
						scale => [MFVec2f, [[1, 1]], field],
						solid => [SFBool, 1, field],
						spine => [MFVec3f, [[0, 0, 0],[0, 1, 0]], field]
					   }
					  ),

	# Complete
	Sphere =>
	new VRML::NodeType("Sphere",
					   { radius => [SFFloat, 1.0, field] }
					  ),

	 # normalPerVertex does not work.
	IndexedFaceSet =>
	new VRML::NodeType("IndexedFaceSet",
					   {
						set_colorIndex => [MFInt32, undef, eventIn],
						set_coordIndex => [MFInt32, undef, eventIn],
						set_normalIndex => [MFInt32, undef, eventIn],
						set_texCoordIndex => [MFInt32, undef, eventIn],
						color => [SFNode, NULL, exposedField],
						coord => [SFNode, NULL, exposedField],
						normal => [SFNode, NULL, exposedField],
						texCoord => [SFNode, NULL, exposedField],
						ccw => [SFBool, 1, field],
						colorIndex => [MFInt32, [], field],
						colorPerVertex => [SFBool, 1, field],
						convex => [SFBool, 1, field],
						coordIndex => [MFInt32, [], field],
						creaseAngle => [SFFloat, 0, field],
						normalIndex => [MFInt32, [], field],
						normalPerVertex => [SFBool, 1, field],
						solid => [SFBool, 1, field],
						texCoordIndex => [MFInt32, [], field]
					   }
					  ),

	# Complete
	IndexedLineSet =>
	new VRML::NodeType("IndexedLineSet",
					   {
						set_colorIndex => [MFInt32, undef, eventIn],
						set_coordIndex => [MFInt32, undef, eventIn],
						color => [SFNode, NULL, exposedField],
						coord => [SFNode, NULL, exposedField],
						colorIndex => [MFInt32, [], field],
						colorPerVertex => [SFBool, 1, field],
						coordIndex => [MFInt32, [], field]
					   }
					  ),


	PointSet =>
	new VRML::NodeType("PointSet",
					   {
						color => [SFNode, NULL, exposedField],
						coord => [SFNode, NULL, exposedField]
					   }
					  ),

	Text =>
	new VRML::NodeType ("Text",
						{
						 string => [MFString, [], exposedField],
						 fontStyle => [SFNode, NULL, exposedField],
						 length => [MFFloat, [], exposedField],
						 maxExtent => [SFFloat, 0, exposedField],
						 __rendersub => [SFInt32, 0, exposedField] # Function ptr hack
						}
					   ),

	FontStyle =>
	new VRML::NodeType("FontStyle",
					   {
						family => [MFString, ["SERIF"], field],
						horizontal => [SFBool, 1, field],
						justify => [MFString, ["BEGIN"], field],
						language => [SFString, "", field],
						leftToRight => [SFBool, 1, field],
						size => [SFFloat, 1.0, field],
						spacing => [SFFloat, 1.0, field],
						style => [SFString, "PLAIN", field],
						topToBottom => [SFBool, 1, field]
					   }
					  ),

	AudioClip =>
	new VRML::NodeType("AudioClip",
					   {
						description => [SFString, "", exposedField],
						loop =>	[SFBool, 0, exposedField],
						pitch => [SFFloat, 1.0, exposedField],
						startTime => [SFTime, 0, exposedField],
						stopTime => [SFTime, 0, exposedField],
						url => [MFString, [], exposedField],
						duration_changed => [SFTime, -1, eventOut],
						isActive => [SFBool, 0, eventOut],

						# internal sequence number
						__sourceNumber => [SFInt32, 0, field],
						# local name, as received on system
						__localFileName => [SFString, "", exposedField],
						# time that we were initialized at
						__inittime => [SFInt32, 0, field],
						# 0:MovTex Vid 1:AudioClip 2:TimeSensor 3:MT Audio
						__type => [SFInt32, 1, exposedField],
						# duration assuming pitch=1 - note, its a string
						# that contains a floating point value.
						__duration =>[SFString, "-1", exposedField]
					   },
					   @x = {
							Initialize => sub {
								my ($t,$f,$time,$scene) = @_;
								# Assign a source number to this source
								$f->{__sourceNumber} = $globalAudioSource++;
								# get the file
								init_sound("","url",$t,$f,$scene,1);
								# we are done with this Sound...
								$SoundMaterial = "unknown";
								return ();
							},

							startTime => sub {
								my($t,$f,$val) = @_;
								#print "MT StartTime $val\n";
								$f->{startTime} = $val;
							},
							# Ignore if less than startTime
							stopTime => sub {
								my($t,$f,$val) = @_;
								#print "MT StopTime $val\n";
								$f->{stopTime} = $val;
							},

							ClockTick => sub {
								return ClockTick_TimeDepNodes (@_);
							},
						   },
					  ),


	Sound =>
	new VRML::NodeType("Sound",
					   {
						direction => [SFVec3f, [0, 0, 1], exposedField],
						intensity => [SFFloat, 1.0, exposedField],
						location => [SFVec3f, [0, 0, 0], exposedField],
						maxBack => [SFFloat, 10.0, exposedField],
						maxFront => [SFFloat, 10.0, exposedField],
						minBack => [SFFloat, 1.0, exposedField],
						minFront => [SFFloat, 1.0, exposedField],
						priority => [SFFloat, 0, exposedField],
						source => [SFNode, NULL, exposedField],
						spatialize => [SFBool,1, field]
					   },
					   {
						Initialize => sub {
							$SoundMaterial = "Sound";
							return ();
						}
					   }
					  ),

	Switch =>
	new VRML::NodeType("Switch",
					   {
						choice => [MFNode, [], exposedField],
						whichChoice => [SFInt32, -1, exposedField]
					   }
					  ),

	LOD =>
	new VRML::NodeType("LOD",
					   {
						level => [MFNode, [], exposedField],
						center => [SFVec3f, [0, 0, 0],  field],
						range => [MFFloat, [], field]
					   }
					  ),

	Transform =>
	new VRML::NodeType ("Transform",
						{
						 addChildren => [MFNode, undef, eventIn],
						 removeChildren => [MFNode, undef, eventIn],
						 center => [SFVec3f, [0, 0, 0], exposedField],
						 children => [MFNode, [], exposedField],
						 rotation => [SFRotation, [0, 0, 1, 0], exposedField],
						 scale => [SFVec3f, [1, 1, 1], exposedField],
						 scaleOrientation => [SFRotation, [0, 0, 1, 0], exposedField],
						 translation => [SFVec3f, [0, 0, 0], exposedField],
						 bboxCenter => [SFVec3f, [0, 0, 0], field],
						 bboxSize => [SFVec3f, [-1, -1, -1], field]
						},
						{
						 addChildren => sub {
							 return addChildren_GroupingNodes(@_);
						 },

						 removeChildren => sub {
							 return removeChildren_GroupingNodes(@_);
						 },

						 EventsProcessed => sub {
							 #my($node,$fields,$time) = @_;
							 ##my $ac = $fields->{addChildren};
							 #print("Transform:EventsProcessed $node $fields\n");
							 ##$node->{BackEnd}->update_scene($time);
							 ##add_MFNode($t,"children",$ac->[0], 1);
							 ##$node->receive_event("addChildren", $ac, $time);
							 return ();
						 }
						},
					   ),

	TextureTransform =>
	new VRML::NodeType ("TextureTransform",
						{
						 center => [SFVec2f, [0, 0], exposedField],
						 rotation => [SFFloat, 0, exposedField],
						 scale => [SFVec2f, [1, 1], exposedField],
						 translation => [SFVec2f, [0, 0], exposedField]
						}
					   ),

	# Complete
	Group =>
	new VRML::NodeType("Group",
					   {
						addChildren => [MFNode, undef, eventIn],
						removeChildren => [MFNode, undef, eventIn],
						children => [MFNode, [], exposedField],
						bboxCenter => [SFVec3f, [0, 0, 0], field],
						bboxSize => [SFVec3f, [-1, -1, -1], field]
					   },
					   {
						addChildren => sub {
							return addChildren_GroupingNodes(@_);
						},

						removeChildren => sub {
							return removeChildren_GroupingNodes(@_);
						},

						EventsProcessed => sub {
							# my($node,$fields,$time) = @_;
							# print ("Group, EP, $node $fields\n");
							return();
						}
					   }
					  ),

	Anchor =>
	new VRML::NodeType("Anchor",
					   {
						addChildren => [MFNode, undef, eventIn],
						removeChildren => [MFNode, undef, eventIn],
						children => [MFNode, [], exposedField],
						description => [SFString, "", exposedField],
						parameter => [MFString, [], exposedField],
						url => [MFString, [], exposedField],
						bboxCenter => [SFVec3f, [0, 0, 0], field],
						bboxSize => [SFVec3f, [-1, -1, -1], field]
					   },
					   {
						addChildren => sub {
							return addChildren_GroupingNodes(@_);
						},

						removeChildren => sub {
							return removeChildren_GroupingNodes(@_);
						}
					   }
					  ),

	Billboard =>
	new VRML::NodeType("Billboard",
					   {
						addChildren => [MFNode, undef, eventIn],
						removeChildren => [MFNode, undef, eventIn],
						axisOfRotation => [SFVec3f, [0, 1, 0], exposedField],
						children => [MFNode, [], exposedField],
						bboxCenter => [SFVec3f, [0, 0, 0], field],
						bboxSize => [SFVec3f, [-1, -1, -1], field]
					   },
					   {
						addChildren => sub {
							return addChildren_GroupingNodes(@_);
						},

						removeChildren => sub {
							return removeChildren_GroupingNodes(@_);
						}
					   }
					  ),

	# Complete
	WorldInfo =>
	new VRML::NodeType("WorldInfo",
					   {
						info => [MFString, [], field],
						title => [SFString, "", field]
					   }
					  ),

	# Complete
	# XXX "getting value if no events should give keyValue[0]!!!"
	ScalarInterpolator =>
	new VRML::NodeType("ScalarInterpolator",
					   {
						set_fraction => [SFFloat, undef, eventIn],
						key => [MFFloat, [], exposedField],
						keyValue => [MFFloat, [], exposedField],
						value_changed => [SFFloat, 0.0, eventOut]
					   },
					   {
						Initialize => sub {
							my($t,$f) = @_;
							$t->{Fields}->{value_changed} =
								(defined $f->{keyValue}[0] ?
								 $f->{keyValue}[0] :
								 0);
							return [$t, value_changed, $f->{keyValue}[0]];
						},

						EventsProcessed => sub {
							my($t, $f) = @_;
							my $k = $f->{key};
							my $kv = $f->{keyValue};
							my $fr = $f->{set_fraction};
							my $v;
							if ($f->{set_fraction} <= $k->[0]) {
								$v = $kv->[0]
							} elsif ($f->{set_fraction} >= $k->[-1]) {
								$v = $kv->[-1]
							} else {
								my $i;
								for ($i=1; $i<=$#$k; $i++) {
									if ($f->{set_fraction} < $k->[$i]) {
										print "SCALARX: $i\n"
											if $VRML::verbose;
										$v = ($f->{set_fraction} - $k->[$i-1]) /
											($k->[$i] - $k->[$i-1]) *
												($kv->[$i] - $kv->[$i-1]) +
													$kv->[$i-1];
										last;
									}
								}
							}
							print "SCALAR: NEW_VALUE $v ($k $kv $f->{set_fraction}, $k->[0] $k->[1] $k->[2] $kv->[0] $kv->[1] $kv->[2])\n"
								if $VRML::verbose;
							return [$t, value_changed, $v];
						}
					   }
					  ),

	OrientationInterpolator =>
	new VRML::NodeType("OrientationInterpolator",
					   {
						set_fraction => [SFFloat, undef, eventIn],
						key => [MFFloat, [], exposedField],
						keyValue => [MFRotation, [], exposedField],
						value_changed => [SFRotation, [0, 0, 1, 0], eventOut]
					   },
					   {
						Initialize => sub {
							my($t,$f) = @_;
							$t->{Fields}{value_changed} =
								($f->{keyValue}[0] or [0, 0, 1, 0]);
							return ();
						},

						EventsProcessed => sub {
							my($t, $f) = @_;
							my $k = $f->{key};
							my $kv = $f->{keyValue};
							my $fr = $f->{set_fraction};
							my $v;

							if ($f->{set_fraction} <= $k->[0]) {
								$v = $kv->[0]
							} elsif ($f->{set_fraction} >= $k->[-1]) {
								$v = $kv->[-1]
							} else {
								my $i;
								for ($i=1; $i<=$#$k; $i++) {
									if ($f->{set_fraction} < $k->[$i]) {
										print "ORIENTX: $i ($#$k) $k->[$i] $k->[$i-1] @{$kv->[$i]} | @{$kv->[$i-1]}\n"
											if $VRML::verbose::oint;

										# get the rotation fraction, starting and ending rotations
										my $f = ($f->{set_fraction} -
												 $k->[$i-1]) /
													 ($k->[$i] - $k->[$i-1]);
										my $sr = [@{$kv->[$i-1]}];
										my $er = [@{$kv->[$i]}];


										# THE FOLLOWING LINES COULD BE OPTIMIZED.
										# are the rotations changed? Jeff Sonsteins blimp
										# does this...
										# eg, [0 1 0 0.5, 0 -1 0 0.0]
										# and, we don't care if a starting/ending rotation has
										# an angle of 0, because then the axis does not matter.
										# Nancy in Jump mode has this "problem". Comment out the
										# following if/elsif and run nancy to see what is up.
										if (abs($sr->[3]) < 0.0001) {
											$sr->[0] = $er->[0];
											$sr->[1] = $er->[1];
											$sr->[2] = $er->[2];
										} elsif (abs($er->[3]) < 0.0001) {
											$er->[0] = $sr->[0];
											$er->[1] = $sr->[1];
											$er->[2] = $sr->[2];
										}

										if (($sr->[0]*$er->[0] + $sr->[1]*$er->[1] +
											 $sr->[2]*$er->[2]) >= 0) {
											$v->[0] = $sr->[0] +
												($f * ($er->[0] - $sr->[0]));
											$v->[1] = $sr->[1] +
												($f * ($er->[1] - $sr->[1]));
											$v->[2] = $sr->[2] +
												($f * ($er->[2] - $sr->[2]));
										} else {
											$v->[0] = $sr->[0] +
												($f * (-$er->[0] - $sr->[0]));
											$v->[1] = $sr->[1] +
												($f * (-$er->[1] - $sr->[1]));
											$v->[2] = $sr->[2] +
												($f * (-$er->[2] - $sr->[2]));
											$er->[3] = -$er->[3];
										}

										# now, the spec, section 6.32 says:
										# If two consecutive keyValue values exist such that the
										# arc length between them is greater than PI, the
										# interpolation will take place on the arc complement.

										if (abs($er->[3] - $sr->[3]) > 3.1415926) {
											if ($sr->[3] >= $er->[3]) {
												$er->[3] += 6.283;
											} else {
												$sr->[3] += 6.283;
											}
										}

										# now calculate the angle
										$v->[3] = $sr->[3] +
											($f * ($er->[3] - $sr->[3]));

										# Bounds check; angle between 0 and 2PI
										if ($v->[3] < 0.0) {
											$v->[3] += 6.283;
										} else {
											if ($v->[3] > 6.283) {
												$v->[3] -=6.283;
											}
										}

										print "V: ", (join ',  ',@$v), "\n"
											if $VRML::verbose::oint;
										last;
									}
								}
							}
							print "OR: NEW_VALUE $v ($k $kv $f->{set_fraction}, $k->[0] $k->[1] $k->[2] $kv->[0] $kv->[1] $kv->[2])\n"
								if $VRML::verbose::oint;
							return [$t, value_changed, $v];
						}
					   }
					  ),

# Complete
# XXX "getting value if no events should give keyValue[0]!!!"
	ColorInterpolator =>
	new VRML::NodeType("ColorInterpolator",
					   {
						set_fraction => [SFFloat, undef, eventIn],
						key => [MFFloat, [], exposedField],
						keyValue => [MFColor, [], exposedField],
						value_changed => [SFColor, [0, 0, 0], eventOut]
					   },
					   @x = {
						Initialize => sub {
							my($t,$f) = @_;
							$t->{Fields}->{value_changed} = ($f->{keyValue}[0] or [0, 0, 0]);
							return ();
						},
						EventsProcessed => sub {
							my($t, $f) = @_;
							my $k = $f->{key};
							my $kv = $f->{keyValue};
							# print "K,KV: $k, $kv->[0][0], $kv->[0][1], $kv->[0][2],
							# 	$kv->[1][0], $kv->[1][1], $kv->[1][2]\n";
							my $fr = $f->{set_fraction};
							my $v;
							if ($f->{set_fraction} <= $k->[0]) {
								$v = $kv->[0]
							} elsif ($f->{set_fraction} >= $k->[-1]) {
								$v = $kv->[-1]
							} else {
								my $i;
								for ($i = 1; $i <= $#{$k}; $i++) {
									if ($f->{set_fraction} < $k->[$i]) {
										print "COLORX: $i\n"
											if $VRML::verbose or
												$VRML::sverbose =~ /\binterp\b/;
										for (0..2) {
											$v->[$_] =
												($f->{set_fraction} - $k->[$i-1]) /
												($k->[$i] - $k->[$i-1]) *
													($kv->[$i][$_] - $kv->[$i-1][$_]) +
														$kv->[$i-1][$_];
										}
										last;
									}
								}
							}
							print "SCALAR: NEW_VALUE $v ($k $kv $f->{set_fraction}, $k->[0] $k->[1] $k->[2] $kv->[0] $kv->[1] $kv->[2])\n"
								if $VRML::verbose or
									$VRML::sverbose =~ /\binterp\b/;
							return [$t, value_changed, $v];
						}
					   }
					  ),

	PositionInterpolator =>
	new VRML::NodeType("PositionInterpolator",
					   {
						set_fraction => [SFFloat, undef, eventIn],
						key => [MFFloat, [], exposedField],
						keyValue => [MFVec3f, [], exposedField],
						value_changed => [SFVec3f, [0, 0, 0], eventOut]
					   },
					   @x
					  ),
	
	# Complete
	# XXX "getting value if no events should give keyValue[0]!!!"
	CoordinateInterpolator =>
	new VRML::NodeType("CoordinateInterpolator",
					   {
						set_fraction => [SFFloat, undef, eventIn],
						key => [MFFloat, [], exposedField],
						keyValue => [MFVec3f, [], exposedField],
						value_changed => [MFVec3f, [], eventOut]
					   },
					   @x = {
							 Initialize => sub {
								 my($t,$f) = @_;
								 # Can't do $f->{} = .. because that sends an event.
								 my $nkv =
									 scalar(@{$f->{keyValue}}) /
									 scalar(@{$f->{key}});
								 $t->{Fields}->{value_changed} =
									 ([@{$f->{keyValue}}[0..$nkv-1]] or []);
								 return ();
								 # XXX DON'T DO THIS!
								 # return [$t, value_changed, $f->{keyValue}[0]];
							 },

							 EventsProcessed => sub {
								 my($t, $f) = @_;
								 my $k = $f->{key};
								 my $kv = $f->{keyValue};
								 my $n = scalar(@{$f->{keyValue}});
								 my $nkv = scalar(@{$f->{keyValue}}) /
									 scalar(@{$f->{key}});
								 # print "K,KV: $k, $kv->[0][0], $kv->[0][1], $kv->[0][2],
								 # 	$kv->[1][0], $kv->[1][1], $kv->[1][2]\n";
								 my $fr = $f->{set_fraction};
								 my $v;
								 if ($f->{set_fraction} <= $k->[0]) {
									 $v = [@{$kv}[0..$nkv-1]];
								 } elsif ($f->{set_fraction} >= $k->[-1]) {
									 $v = [@{$kv}[$n-$nkv .. $n-1]];
								 } else {
									 my $i;
									 for ($i = 1; $i <= $#{$k}; $i++) {
										 if ($f->{set_fraction} < $k->[$i]) {
											 print "CoordinateInterpolator: $f->{set_fraction} $i $k->[$i] - $k->[$i-1]\n"
												 if $VRML::verbose::interp;
											 $v = [];
											 my $o = $i * $nkv;
											 for my $kn (0..$nkv-1) {
												 for (0..2) {
													 $v->[$kn][$_] =
														 ($f->{set_fraction} - $k->[$i-1]) /
															 ($k->[$i] - $k->[$i-1]) *
																 ($kv->[$o+$kn][$_] -
																  $kv->[$o+$kn-$nkv][$_]) +
																	  $kv->[$o+$kn-$nkv][$_];
												 }
											 }
											 last;
										 }
									 }
								 }
								 print "CoordinateInterpolator: NEW_VALUE $v ($k $kv $f->{set_fraction}, $k->[0] $k->[1] $k->[2] $kv->[0] $kv->[1] $kv->[2])\n"
									 if $VRML::verbose::interp;
								 return [$t, value_changed, $v];
							 }
							}
					  ),



	NormalInterpolator =>
	new VRML::NodeType("NormalInterpolator",
					   {
						set_fraction => [SFFloat, undef, eventIn],
						key => [MFFloat, [], exposedField],
						keyValue => [MFVec3f, [], exposedField],
						value_changed => [MFVec3f, [], eventOut]
					   },
					   @x = {
							 Initialize => sub {
								 my($t,$f) = @_;
								 # Can't do $f->{} = .. because that sends an event.
								 my $nkv =
									 scalar(@{$f->{keyValue}}) /
									 scalar(@{$f->{key}});
								 $t->{Fields}->{value_changed} =
									 ([@{$f->{keyValue}}[0..$nkv-1]] or []);
								 return ();
								 # XXX DON'T DO THIS!
								 # return [$t, value_changed, $f->{keyValue}[0]];
							 },
							 EventsProcessed => sub {
								 my($t, $f) = @_;
								 my $k = $f->{key};
								 my $kv = $f->{keyValue};
								 my $n = scalar(@{$f->{keyValue}});
								 my $nkv =
									 scalar(@{$f->{keyValue}}) /
									 scalar(@{$f->{key}});
								 # print "K,KV: $k, $kv->[0][0], $kv->[0][1], $kv->[0][2],
								 # 	$kv->[1][0], $kv->[1][1], $kv->[1][2]\n";
								 my $fr = $f->{set_fraction};
								 my $v;
								 if ($f->{set_fraction} <= $k->[0]) {
									 $v = [@{$kv}[0..$nkv-1]];
								 } elsif ($f->{set_fraction} >= $k->[-1]) {
									 $v = [@{$kv}[$n-$nkv .. $n-1]];
								 } else {
									 my $i;
									 for ($i=1; $i<=$#$k; $i++) {
										 if ($f->{set_fraction} < $k->[$i]) {
											 print "COLORX: $f->{set_fraction} $i $k->[$i] - $k->[$i-1]\n"
												 if $VRML::verbose::interp;
											 $v = [];
											 my $o = $i * $nkv;
											 for my $kn (0..$nkv-1) {
												 for (0..2) {
													 $v->[$kn][$_] =
														 ($f->{set_fraction} -
														  $k->[$i-1]) /
														 ($k->[$i] - $k->[$i-1]) *
															 ($kv->[$o+$kn][$_] -
															  $kv->[$o+$kn-$nkv][$_]) +
																 $kv->[$o+$kn-$nkv][$_];
												 }
											 }
											 last;
										 }
									 }
								 }
								 print "NormalI: NEW_VALUE $v ($k $kv $f->{set_fraction}, $k->[0] $k->[1] $k->[2] $kv->[0] $kv->[1] $kv->[2])\n"
									 if $VRML::verbose::interp;
								 return [$t, value_changed, $v];
							 }
							}
					  ),


	TimeSensor =>
	new VRML::NodeType("TimeSensor",
					   {
						cycleInterval => [SFTime, 1, exposedField],
						enabled => [SFBool, 1, exposedField],
						loop => [SFBool, 0, exposedField],
						startTime => [SFTime, 0, exposedField],
						stopTime => [SFTime, 0, exposedField],
						cycleTime => [SFTime, -1, eventOut],
						fraction_changed => [SFFloat, 0.0, eventOut],
						isActive => [SFBool, 0, eventOut],
						time => [SFTime, -1, eventOut],
						# 0:MovTex Vid 1:AudioClip 2:TimeSensor 3:MT Audio
						__type => [SFInt32, 2, exposedField],
						# cycleTimer flag.
						__ctflag =>[SFTime, 0, exposedField]
					   },
					   {
						Initialize => sub {
							my($t,$f) = @_;

							# force an event at startup
							$f->{__ctflag} = 10.0;
							# print "TS init\n";
							return ();
						},

						EventsProcessed => sub {
							# print "TS EV\n";
							return ();
						},

						#
						#  Ignore startTime and cycleInterval when active..
						#
						startTime => sub {
							my($t,$f,$val) = @_;
							# print "TS ST\n";
							if ($t->{Priv}{active}) {
							} else {
								# $f->{startTime} = $val;
							}
						},

						cycleInterval => sub {
							my($t,$f,$val) = @_;
							# print "TS CI\n";
							if ($t->{Priv}{active}) {
							} else {
								# $f->{cycleInterval} = $val;
							}
						},

						# Ignore if less than startTime
						stopTime => sub {
							my($t,$f,$val) = @_;
							# print "TS ST\n";
							if ($t->{Priv}{active} and $val < $f->{startTime}) {
							} else {
								# return $t->set_field(stopTime,$val);
							}
						},

						ClockTick => sub {
							my($t,$f,$tick) = @_;
							my @e;
							my $act = 0; 
							#print "TS CT\n";

							# are we enabled? If not, make sure we are not marked active.
							if (!$f->{enabled}) {
								if ($f->{isActive}) {
									push @e, [$t, "isActive", 0];
								}
								return @e;
							}

							# ok, we ARE enabled. Process
							return ClockTick_TimeDepNodes (@_);
						},
					   }
					  ),


# new touchsensor rules; the old ones were broken. JAS.
# if we are on a touchsensor, and the button is "PRESS",
# then it is pressed. Period.

# this mimics Cosmoplayer, except that we don't (yet) change
# the cursor type
#
# JAS.

	TouchSensor =>
	new VRML::NodeType("TouchSensor",
					   {
						enabled => [SFBool, 1, exposedField],
						hitNormal_changed => [SFVec3f, [0, 0, 0], eventOut],
						hitPoint_changed => [SFVec3f, [0, 0, 0], eventOut],
						hitTexCoord_changed => [SFVec2f, [0, 0], eventOut],
						isActive => [SFBool, 0, eventOut],
						isOver => [SFBool, 0, eventOut],
						touchTime => [SFTime, -1, eventOut]
					   },
					   {
						__mouse__ => sub {
							my($t, $f, $time, $moved, $button, $over, $pos,
							   $norm, $texc) = @_;
							print "MOUSE: over $over but $button moved $moved\n"
								if $VRML::verbose::timesens;
							if ($button ne "PRESS") {
								return;
							}

							#ok, we are here, and we have a button press.
							# Don't know how many of these thingies we need, nor
							# what they all do, but, this seems to work, at least
							# for simple data... JAS.

							$f->{isOver} = $over;
							$f->{hitPoint_changed} = $pos;
							$f->{hitNormal_changed} = $norm;
							$f->{isActive} = 1;
							$f->{touchTime} = $time;
						}
					   }
					  ),


	PlaneSensor =>
	new VRML::NodeType("PlaneSensor",
					   {
						autoOffset => [SFBool, 1, exposedField],
						enabled => [SFBool, 1, exposedField],
						maxPosition => [SFVec2f, [-1, -1], exposedField],
						minPosition => [SFVec2f, [0, 0], exposedField],
						offset => [SFVec3f, [0, 0, 0], exposedField],
						isActive => [SFBool, 0, eventOut],
						trackPoint_changed => [SFVec3f, [0, 0, 0], eventOut],
						translation_changed => [SFVec3f, [0, 0, 0], eventOut]
					   },
					   {
						__mouse__ => sub {
							my($t, $f, $time, $moved, $button, $over, $pos,
							   $norm, $texc) = @_;
							# print "PS_MOUSE: $moved $button $over @$pos @$norm\n";
							if ($button eq "PRESS") {
								$t->{OrigPoint} = $pos;
								$f->{isActive} = 1;
							} elsif ($button eq "RELEASE") {
								# print "PLREL!\n";
								undef $t->{OrigPoint};
								$t->{isActive} = 0;
								if ($f->{autoOffset}) {
									$f->{offset} =
										$f->{translation_changed};
								}
							} elsif ($button eq "DRAG") {
								# 1. get the point on the plane
								my $op = $t->{OrigPoint};
								my $of = $f->{offset};
								my $mult =
									($op->[2] - $pos->[2]) /
									($norm->[2] - $pos->[2]);
								my $nx = $pos->[0] + $mult *
									($norm->[0] - $pos->[0]);
								my $ny = $pos->[1] + $mult *
									($norm->[1] - $pos->[1]);
								# print "Now: ($mult $op->[2]) $nx $ny $op->[0] $op->[1] $of->[0] $of->[1]\n";
								$f->{trackPoint_changed} =
									[$nx,$ny,$op->[2]];
								my $tr = [$nx - $op->[0] + $of->[0],
										  $ny - $op->[1] + $of->[1],
										  0 + $of->[2]];
								# print "TR: @$tr\n";
								for (0..1) {
									# Clamp
									if ($f->{maxPosition}[$_] >=
										$f->{minPosition}[$_]) {
										if ($tr->[$_] < $f->{minPosition}[$_]) {
											$tr->[$_] = $f->{minPosition}[$_];
										} elsif ($tr->[$_] > $f->{maxPosition}[$_]) {
											$tr->[$_] = $f->{maxPosition}[$_];
										}
									}
								}
								# print "TRC: (@$tr) (@$pos)\n";
								$f->{translation_changed} = $tr;
							}
						}
					   }
					  ),

	SphereSensor =>
	new VRML::NodeType("SphereSensor",
					   {
						autoOffset => [SFBool, 1, exposedField],
						enabled => [SFBool, 1, exposedField],
						offset => [SFRotation, [0, 1, 0, 0], exposedField],
						isActive => [SFBool, 0, eventOut],
						rotation_changed => [SFRotation, [0, 0, 1, 0], eventOut],
						trackPoint_changed => [SFVec3f, [0, 0, 0], eventOut]
					   },
					   {
						__mouse__ => sub {
							my($t, $f, $time, $moved, $button, $over, $pos,
							   $norm, $texc) = @_;
							# print "PS_MOUSE: $moved $button $over @$pos @$norm\n";
							if ($button eq "PRESS") {
								$t->{OrigPoint} = $pos;
								$t->{Radius} = $pos->[0] ** 2 +
									$pos->[1] ** 2 + $pos->[2] ** 2;
								$f->{isActive} = 1;
							} elsif ($button eq "RELEASE") {
								undef $t->{OrigPoint};
								$t->{isActive} = 0;
								if ($f->{autoOffset}) {
									$f->{offset} = $f->{rotation_changed};
								}
							} elsif ($button eq "DRAG") {
								# 1. get the point on the plane
								my $op = $t->{OrigPoint};
								my $r = $t->{Radius};
								my $of = $f->{offset};

								my $tr1sq =
									$pos->[0]**2 +
									$pos->[1]**2 +
									$pos->[2]**2;
								my $tr2sq =
									$norm->[0]**2 +
									$norm->[1]**2 +
									$norm->[2]**2;
								my $tr1tr2 =
									$pos->[0]*$norm->[0] +
									$pos->[1]*$norm->[1] +
									$pos->[2]*$norm->[2];
								my @d = map {
									$norm->[$_] - $pos->[$_]
								} 0..2;
								my $dlen =
									$d[0]**2 +
									$d[1]**2 +
									$d[2]**2;

								my $a = $dlen;
								my $b = 2*($d[0]*$pos->[0] +
										   $d[1]*$pos->[1] +
										   $d[2]*$pos->[2]);
								my $c = $tr1sq - $r*$r;
								$b /= $a;
								$c /= $a;

								my $und = $b*$b - 4*$c;
								if ($und >= 0) {
									my $sol;
									if ($b >= 0) {
										$sol = (-$b + sqrt($und)) / 2;
									} else {
										$sol = (-$b - sqrt($und)) / 2;
									}
									my @r = map {
										$pos->[$_] + $sol * ($norm->[$_] - $pos->[$_])
									} 0..2;
									# Ok, now we have the two vectors op
									# and r, find out the rotation to take 
									# one to the other.
									my @cp = (
											  $r[1] * $op->[2] - $op->[1] * $r[2],
											  $r[2] * $op->[0] - $op->[2] * $r[0],
											  $r[0] * $op->[1] - $op->[0] * $r[1],
											 );
									my @dot = (
											   $r[0] * $op->[0],
											   $r[1] * $op->[1],
											   $r[2] * $op->[2]
											  );
									my $an = atan2((my $cl =
													$cp[0]**2 +
													$cp[1]**2 +
													$cp[2]**2),
												   $dot[0]**2 +
												   $dot[1]**2 +
												   $dot[2]**2);
									for (@cp) {
										$_ /= $cl;
									}
									$f->{trackPoint_changed} = [@r];
				
									# print "QNEW: @cp, $an (R: $r)\n";
									my $q =
										VRML::Quaternion->new_vrmlrot(@cp, -$an);
									# print "QNEW2: @$of\n";
									my $q2 =
										VRML::Quaternion->new_vrmlrot(@$of);

									$f->{rotation_changed} =
										$q->multiply($q2)->to_vrmlrot();
								}
							}
						}
					   }
					  ),

	CylinderSensor =>
	new VRML::NodeType("CylinderSensor",
					   {
						autoOffset => [SFBool, 1, exposedField],
						diskAngle => [SFFloat, 0.262, exposedField],
						enabled => [SFBool, 1, exposedField],
						maxAngle => [SFFloat, -1.0, exposedField],
						minAngle => [SFFloat, 0, exposedField],
						offset => [SFFloat, 0, exposedField],
						isActive => [SFBool, 0, eventOut],
						rotation_changed => [SFRotation, [0, 0, 1, 0], eventOut],	
						trackPoint_changed => [SFVec3f, [0, 0, 0], eventOut]
					   },
					   {
						__mouse__ => sub {
							## until there's code, need empty method to prevent
							## the browser from crashing
						}
					   }
					  ),


	# This just about the minimal visibilitysensor allowed by the spec.
	VisibilitySensor =>
	new VRML::NodeType("VisibilitySensor",
					   {
						center => [SFVec3f, [0, 0, 0], exposedField],
						enabled => [SFBool, 1, exposedField],
						size => [SFVec3f, [0, 0, 0], exposedField],
						enterTime => [SFTime, -1, eventOut],
						exitTime => [SFTime, -1, eventOut],
						isActive => [SFBool, 0, eventOut]
					   },
					   {
						Initialize => sub {
							my($t,$f,$time,$scene) = @_;
							if ($f->{enabled}) {
								return ([$t, enterTime, $time],
										[$t, isActive, 1]);
							}
							return ();
						}
					   }
					  ),

	ProximitySensor =>
	new VRML::NodeType("ProximitySensor",
					   {
						center => [SFVec3f, [0, 0, 0], exposedField],
						size => [SFVec3f, [0, 0, 0], exposedField],
						enabled => [SFBool, 1, exposedField],
						isActive => [SFBool, 0, eventOut],
						position_changed => [SFVec3f, [0, 0, 0], eventOut],
						orientation_changed => [SFRotation, [0, 0, 1, 0], eventOut],
						enterTime => [SFTime, -1, eventOut],
						exitTime => [SFTime, -1, eventOut],
						# These fields are used for the info.
						__hit => [SFInt32, 0, exposedField],
						__t1 => [SFVec3f, [10000000, 0, 0], exposedField],
						__t2 => [SFRotation, [0, 1, 0, 0], exposedField]
					   },
					   {
						ClockTick => sub {
							my($t,$f,$tick) = @_;
							return if !$f->{enabled};
							return if !$t->{BackEnd};
							my($hit, $x1, $y1, $z1, $x2, $y2, $z2, $q2);

							VRML::VRMLFunc::get_proximitysensor_vecs($t->{BackNode}->{CNode},
																	 $hit,
																	 $x1,
																	 $y1,
																	 $z1,
																	 $x2,
																	 $y2,
																	 $z2,
																	 $q2);

							if ($VRML::verbose::prox) {
								print "PROX: $r->[0] ($r->[1][0] $r->[1][1] $r->[1][2]) ($r->[2][0] $r->[2][1] $r->[2][2] $r->[2][3])\n";
							}
							if ($hit) {
								if (!$f->{isActive}) {
									#print "PROX - initial defaults\n";
									$f->{isActive} = 1;
									$f->{enterTime} = $tick;
									$f->{position_changed} = [$x1,$y1,$z1];
									$f->{orientation_changed} = [$x2,$y2,$z2,$q2];
								}
			
								# now, has anything changed?
								my $ch = 0;
								if (($x1 != $f->{position_changed}[0]) ||
									($y1 != $f->{position_changed}[1]) ||
									($z1 != $f->{position_changed}[2])) {
									#print "PROX - position changed!!! \n";
									$f->{position_changed} = [$x1,$y1,$z1];
									$ch = 1;
								}
								if (($x2 != $f->{orientation_changed}[0]) ||
									($y2 != $f->{orientation_changed}[1]) ||
									($z2 != $f->{orientation_changed}[2]) ||
									($q2 != $f->{orientation_changed}[3])) {
									#print "PROX - orientation changed!!! ";
									#print $r->[2][0]," ", $r->[2][1], " ",$r->[2][2]," ",$r->[2][3],"\n";
				
									$f->{orientation_changed} = [$x2,$y2,$z2,$q2];
									$ch = 1;
								}
								# return if !$ch;
								return 1 if !$ch; #ncoder: added 1. seemed Scene.pm needed a return value. mustcheck.
							} else {
								if ($f->{isActive}) {
									$f->{isActive} = 0;
									$f->{exitTime} = $tick;
								}
							}
						}
					   }
					  ),


	DirectionalLight =>
	new VRML::NodeType("DirectionalLight",
					   {
						ambientIntensity => [SFFloat, 0, exposedField],
						color => [SFColor, [1, 1, 1], exposedField],
						direction => [SFVec3f, [0, 0, -1], exposedField],
						intensity => [SFFloat, 1.0, exposedField],
						on => [SFBool, 1, exposedField]
					   }
					  ),

	PointLight =>
	new VRML::NodeType("PointLight",
					   {
						ambientIntensity => [SFFloat, 0, exposedField],
						attenuation => [SFVec3f, [1, 0, 0], exposedField],
						color => [SFColor, [1, 1, 1], exposedField],
						intensity => [SFFloat, 1.0, exposedField],
						location => [SFVec3f, [0, 0, 0], exposedField],
						on => [SFBool, 1, exposedField],
						radius => [SFFloat, 100.0, exposedField],
						##not in the spec
						direction => [SFVec3f, [0, 0, -1.0], exposedField]
					   }
					  ),

	SpotLight =>
	new VRML::NodeType("SpotLight",
					   {
						ambientIntensity => [SFFloat, 0, exposedField],
						attenuation => [SFVec3f, [1, 0, 0], exposedField],
						beamWidth => [SFFloat, 1.570796, exposedField],
						color => [SFColor, [1, 1, 1], exposedField],
						cutOffAngle => [SFFloat, 0.785398, exposedField],
						direction => [SFVec3f, [0, 0, -1], exposedField],
						intensity => [SFFloat, 1.0, exposedField],
						location => [SFVec3f, [0, 0, 0], exposedField],
						on => [SFBool, 1, exposedField],
						radius => [SFFloat, 100.0, exposedField]
					   }
					  ),

	Fog =>
	new VRML::NodeType("Fog",
					   {
						set_bind => [SFBool, undef, eventIn],
						color => [SFColor, [1, 1, 1], exposedField],
						fogType => [SFString, "LINEAR", exposedField],
						visibilityRange => [SFFloat, 0, exposedField],
						isBound => [SFBool, 0, eventOut]
						# AK - not in spec #bindTime => [SFTime, undef, eventOut]
					   }
					  ),

	Background =>
	new VRML::NodeType("Background",
					   {
						set_bind => [SFBool, undef, eventIn],
						groundAngle => [MFFloat, [], exposedField],
						groundColor => [MFColor, [], exposedField],
						skyAngle => [MFFloat, [], exposedField],
						skyColor => [MFColor, [[0, 0, 0]], exposedField],
						isBound => [SFBool, 0, eventOut],
						(map {(
							   $_.Url => [MFString, [], exposedField],
							   # local or temp file name
							   __locfile.$_ => [SFString, "", exposedField],
							   # OpenGL texture number
							   __texture.$_ => [SFInt32, 0, exposedField],
							   # is this a temp file?
							   __istemporary.$_ => [SFInt32, 0, exposedField]
							  )} qw/back front top bottom left right/),
						# AK - not in spec #bindTime => [SFTime, undef, eventOut]
					   },
					   {
						Initialize => sub {
							my($t,$f,$time,$scene) = @_;
							for (qw/back front top bottom left right/) {
								init_image("$_","${_}Url",$t,$f,$scene,0);
							}
							return ();
						}
					   }
					  ),

	Viewpoint =>
	new VRML::NodeType("Viewpoint",
					   {
						set_bind => [SFBool, undef, eventIn],
						fieldOfView => [SFFloat, 0.785398, exposedField],
						jump => [SFBool, 1, exposedField],
						orientation => [SFRotation, [0, 0, 1, 0], exposedField],
						position => [SFVec3f,[0, 0, 10], exposedField],
						description => [SFString, "", field],
						bindTime => [SFTime, -1, eventOut],
						isBound => [SFBool, 0, eventOut]
					   },
					   {

						Initialize => sub {
							my($t,$f) = @_;
							# print "Initialize - viewpoint\n";
							return ();
						},

						WhenBound => sub {
							my($t,$scene,$revealed) = @_;
							# print "\nstart of WhenBound\n";
							#  print "VP_BOUND t $t scene $scene revealed $revealed\n";
							#  print "VP_BOUND!\n" if $VRML::verbose::bind;
							#  print "ref t ", ref $t,"\n";
							#  print "ref t backend ", ref $t->{BackEnd},"\n";
							#  print "t backend ", $t->{BackEnd},"\n";
							#  print "ref t backnode ", ref $t->{BackNode},"\n";
							#  print "t backNode ", $t->{BackNode},"\n";
							#  print "ref t protoexp ", ref $t->{ProtoExp},"\n";
							#  print "t protoexp ", $t->{ProtoExp},"\n";
							# print "END of WhenBound\n";

							$t->{BackEnd}->bind_viewpoint($t,
														  ($revealed ?
														   $t->{VP_Info} :
														   undef));
							VRML::VRMLFunc::reset_upvector();
						},

						WhenUnBound => sub {
							my($t,$scene) = @_;
							print "VP_UNBOUND!\n" if $VRML::verbose::bind;
							$t->{VP_Info} =
								$t->{BackEnd}->unbind_viewpoint($t);
						}
					   }
					  ),


	NavigationInfo =>
	new VRML::NodeType("NavigationInfo",
					   {
						set_bind => [SFBool, undef, eventIn],
						avatarSize => [MFFloat, [0.25, 1.6, 0.75], exposedField],
						headlight => [SFBool, 1, exposedField],
						speed => [SFFloat, 1.0, exposedField],
						type => [MFString, ["WALK", "ANY"], exposedField],
						visibilityLimit => [SFFloat, 0, exposedField],
						isBound => [SFBool, 0, eventOut]
						# AK - not in spec #bindTime => [SFTime, undef, eventOut]
					   },
					   {
						Initialize => sub { return (); },

						WhenBound => sub {
							my ($t) = @_;
							$t->{BackEnd}->bind_navi_info($t);
							VRML::VRMLFunc::set_naviinfo($t->{Fields}{avatarSize}[0],
														 $t->{Fields}{avatarSize}[1],
														 $t->{Fields}{avatarSize}[2]);
						},
					   },
					  ),

	# Complete
	# fields, eventins and outs parsed in Parse.pm by special switch :(
	# JAS took out perl script, because it is not standard.


	## directOutput && mustEvaluate work???

	Script =>
	new VRML::NodeType("Script",
					   {
						url => [MFString, [], exposedField],
						directOutput => [SFBool, 0, field],
						mustEvaluate => [SFBool, 0, field]
					   },
					   {
						Initialize => sub {
							#JAS $VRML::verbose::script = 1;
							my($t,$f,$time,$scene) = @_;

							print "ScriptInit t ",
								VRML::NodeIntern::dump_name($t),
										" f $f time $time scene ",
											VRML::NodeIntern::dump_name($scene), "\n"
													if $VRML::verbose::script;

							my $h;
							my $Browser = $scene->get_browser();
							for (@{$f->{url}}) {
								# is this already made???
								print "Working on $_\n" if $VRML::verbose::script;
								if (defined  $t->{J}) {
									print "...{J} already defined, skipping\n"
										if $VRML::verbose::script;
									last;
								}

								my $str = $_;
								print "TRY $str\n" if $VRML::verbose::script;
								if (s/^perl(_tjl_xxx1)?://) {
									{
										print "XXX1 script\n" if $VRML::verbose::script;
										check_perl_script();

										# See about RFields in file ARCHITECTURE and in
										# Scene.pm's VRML::FieldHash package
										my $u = $t->{Fields};

										my $t = $t->{RFields};

										# This string ties scalars
										my $tie = join("", map {
											"tie \$$_, 'MTS',  \\\$t->{$_};"
										} script_variables($u));
										## print "tie = $tie\n";
										## $h = eval "$tie ({$_})";
										$h = eval "({$_})";
										# Wrap up each sub in the script node
										foreach (keys %$h) {
											my $tmp = $h->{$_};
											my $src = join ("\n",
															"sub {",
															"  $tie",
															"  \&\$tmp (\@_)",
															"}");
											## print "---- src ----$src\n--------------",
											$h->{$_} = eval $src ;
										}

										## $h = eval "({$_})";

										print "Evaled: $h\n",
											"-- h = $h --\n",
												(map {"$_ => $h->{$_}\n"}
												 keys %$h),
													"-- u = $u --\n",
														(map {
															"$_ => $u->{$_}\n"
														} keys %$u),
															"-- t = $t --\n",
																(map {
																	"$_ => $t->{$_}\n"
																} keys %$t)
																	if $VRML::verbose::script;
										if ($@) {
											die "Invalid script '$@'"
										}
									}
									last;
								} elsif (/\.class$/) {
									my $wurl = $scene->get_world_url;
									$t->{PURL} = $scene->get_url;
									if (!defined $VRML::J) {
										eval('require "VRML/VRMLJava.pm"');
										if ($@) {
											die $@;
										}
										$VRML::J =
											VRML::JavaCom->new($scene->get_browser);
									}
									if (defined $wurl) {
										$VRML::J->newscript($wurl,$_,$t);
									} else {
										$VRML::J->newscript($t->{PURL},$_,$t);
									}

									$t->{J} = $VRML::J;
									last;
								} elsif (/\.js/) {
									# New js url handling
									my $purl = $t->{PURL} = $scene->get_url;
									my $wurl = $scene->get_world_url;
									my $file;

									if (defined $wurl) {
										$file =
											VRML::URL::get_relative($wurl, $_, 1);
									} else {
										$file =
											VRML::URL::get_relative($purl, $_, 1);
									}

									print "JS url: file = $file\n"
										if $VRML::verbose::script;
									open (SCRIPT_CODE, "< $file") ||
										die ("Couldn't retrieve javascript url $_ !");
									my $code = "";
									while (<SCRIPT_CODE>) {
										$code .= $_;
									}
									close(SCRIPT_CODE);
									print "JS url: code = $code\n"
										if $VRML::verbose::script;
									eval('require VRML::JS;');
									if ($@) {
										die $@;
									}
									$t->{J} = VRML::JS->new($code, $t, $Browser);
									last;
								} elsif (s/^(java|vrml)script://) {
									eval('require VRML::JS;');
									if ($@) {
										die $@;
									}
									$t->{J} = VRML::JS->new($_, $t, $Browser);
									last;
								} else {
									warn("unknown script: $_");
								}
							}

							if (!defined $h and !defined $t->{J}) {
								die "Didn't find a valid perl(_tjl_xxx)? or java script";
							}
							print "Script got: ", (join ',',keys %$h), "\n"
								if $VRML::verbose::script;
							$t->{ScriptScript} = $h;
							my $s;
							if (($s = $t->{ScriptScript}{"initialize"})) {
								print "CALL $s\n if $VRML::verbose::script"
									if $VRML::verbose::script;
								perl_script_output(1);
								my @res = &{$s}();
								perl_script_output(0);
								return @res;
							} elsif ($t->{J}) {
								return $t->{J}->initialize($scene, $t);
							}
							return ();
						},
						url => sub {
							print "ScriptURL $_[0] $_[1]!!\n"
								if $VRML::verbose::script;
							die "URL setting not enabled";
						},
						__any__ => sub {
							my($t,$f,$v,$time,$ev) = @_;
							print "ScriptANY ",VRML::NodeIntern::dump_name($t),
								" $_[1] $_[2] $_[3] $_[4]\n"

									if $VRML::verbose::script;
							my $s;


							if (($s = $t->{ScriptScript}{$ev})) {
								print "CALL $s\n"
									if $VRML::verbose::script;
								##EG show_stack (5);
								##EG return &{$s}();
								perl_script_output (1);
								my @res = &{$s}();
								perl_script_output (0);
								return @res ;
							} elsif ($t->{J}) {

								#EG			if($t->{J}) {
								return $t->{J}->sendevent($t, $ev, $v, $time);
							}
							return ();
						},
						EventsProcessed => sub {
							my($t,$f) = @_;
							print "ScriptEP $_[0] $_[1]!!\n"
								if $VRML::verbose::script;
							if ($t->{J}) {
								return $t->{J}->sendeventsproc($t);
							}
							return ();
						},
					   }
					  ),

	Collision =>
	new VRML::NodeType("Collision",
					   {
						addChildren => [MFNode, undef, eventIn],
						removeChildren => [MFNode, undef, eventIn],
						children => [MFNode, [], exposedField],
						collide => [SFBool, 1, exposedField],
						bboxCenter => [SFVec3f, [0, 0, 0], field],
						bboxSize => [SFVec3f, [-1, -1, -1], field],
						proxy => [SFNode, NULL, field],
						collideTime => [SFTime, -1, eventOut],
						# return info for collisions
						# bit 0 : collision or not
						# bit 1: changed from previous of not
						__hit => [SFInt32, 0, exposedField]
					   },
					   {
						addChildren => sub {
							return addChildren_GroupingNodes(@_);
						},

						removeChildren => sub {
							return removeChildren_GroupingNodes(@_);
						},

						ClockTick => sub {
							my($t,$f,$tick) = @_;
							return if !$t->{BackEnd};
							my $hit;

							VRML::VRMLFunc::get_collision_info($t->{BackNode}->{CNode},
															   $hit);

							if ($hit == 3) { #collision occured and state changed
								if ($VRML::verbose::collision) {
									print "COLLISION: $t, time=$tick\n";
								}
								$f->{collideTime} = $tick;
							}
						}
					   }
					  ),

	Inline =>
	new VRML::NodeType("Inline",
					   {
						url => [MFString, [], exposedField],
						bboxCenter => [SFVec3f, [0, 0, 0], field],
						bboxSize => [SFVec3f, [-1, -1, -1], field]
					   },
					   {
						Initialize => sub {
							my($t, $f, $time, $scene) = @_;
							# XXXXXX!!
							#print "VRMLNode::Inline\n\tt $t\n\tf $f\n\ttime $time\n\tscene $scene\n";

							my ($text, $url);
							my $purl = $scene->get_url();
							my $wurl = $scene->get_world_url;
							my $urls = $f->{url};
							$p = $scene->new_proto("__proto".$protono++);

							my $valid = 0;

						URL:
							for $u (@$urls) {
								if (defined $wurl) {
									($text, $url) = VRML::URL::get_relative($wurl, $u);
								} else {
									($text, $url) = VRML::URL::get_relative($purl, $u);
								}

								if (!$text) {
									warn "Warning: could not retrieve $u";
									next URL;
								}

								$p->set_url($url);
								VRML::Parser::parse($p, $text);
								if (!defined $p) {
									die("Inline not found");
								}

								$t->{ProtoExp} = $p;
								$t->{ProtoExp}->set_parentnode($t);
								$t->{ProtoExp}->make_executable();
								$t->{ProtoExp}{IsInline} = 1;
								$t->{IsProto} = 1;

								$valid = 1;
							} # for $u (@$urls)

							if (!$valid) {
								die "Unable to loacte a valid url";
							}
							return ();
						}
					   }
					  ),

); ##%VRML::Nodes


1;

