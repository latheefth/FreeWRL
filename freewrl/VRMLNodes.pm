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
my $vpno = 0;
my $globalAudioSource = 0;  # count of audio sources
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

sub get_vp_node { 
	#print "get_vp_node, returning vp $vpno " , $vpn[$vpno],"\n";
return $vpn[$vpno];}

sub get_vp_scene {
	#print "get_vp_scene, returning ", $vps[$vpno],"\n";
return $vps[$vpno];}



#########################################################
# The routines below implement the browser object interface.

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


# pass in a scene, and an MFString of urls, returns a local name to the file.
sub getFileFromUrls {
	my ($scene, $urls, $node) = @_;

	my $purl = $scene->get_url();
	if ($node) {
		$node->{PURL} = $purl;
	}
	my $wurl = $scene->get_world_url();
	my $file;
	my $suffix;
	my $u;

	for $u (@{$urls}) {
		next unless $u =~ /\.(\w*)$/;
		$suffix = $1;
		
		if (defined $wurl) {
			$file = VRML::URL::get_relative($wurl, $u, 1);
		} else {
			$file = VRML::URL::get_relative($purl, $u, 1);
		}

		if ($file) {
			last;
		} else {
			warn("Could not retrieve $u");
		}
	}
	print "VRML::Nodes::getFileFromUrls got: $file, $suffix\n"
		if $VRML::verbose;
	return (wantarray ? ($file, $suffix) : $file);
}

sub getTextFromURLs {
	my ($scene, $url, $node) = @_;

	my $purl = $scene->get_url();
	if ($node) {
		$node->{PURL} = $purl;
	}
	my $wurl = $scene->get_world_url();
	my $text;
	my $actual_url;

	if (ref $url eq "ARRAY") {
		for my $u (@{$url}) {
			if (defined $wurl) {
				($text, $actual_url) = VRML::URL::get_relative($wurl, $u);
			} else {
				($text, $actual_url) = VRML::URL::get_relative($purl, $u);
			}

			if ($text) {
				last;
			} else {
				warn("Could not retrieve $u from ".VRML::Debug::toString($url));
			}
		}
	} else {
		if (defined $wurl) {
			($text, $actual_url) = VRML::URL::get_relative($wurl, $url);
		} else {
			($text, $actual_url) = VRML::URL::get_relative($purl, $url);
		}

		if (!$text) {
			warn("Could not retrieve $u from $url");
		}
	}

	print "VRML::Nodes::getTextFromUrls got: $text, $actual_url\n"
		if $VRML::verbose;
	return (wantarray ? ($text, $actual_url) : $text);
}

########################################################################
#
# Image loading.
#

my %image_same_url = ();



# picture image
sub init_image {
	my($name, $urlname, $t, $f, $scene, $flip) = @_;
	my $urls = $f->{$urlname};

	if ($#{$urls} == -1) { goto NO_TEXTURE; }

	my $file;
	my $suffix;
	($file, $suffix) = getFileFromUrls($scene, $urls, $t);

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

	my $urls = $f->{$urlname};
	if ($#{$urls} == -1) { goto NO_TEXTURE; }

	my $file;
	my $suffix;
	($file, $suffix) = getFileFromUrls($scene, $urls, $t);

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

	my $urls = $f->{$urlname};
	if ($#{$urls} == -1) { goto NO_TEXTURE; }

	my $file;
	my $suffix;
	($file, $suffix) = getFileFromUrls($scene, $urls, $t);

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
 Collision
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
 Anchor
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
						 __inittime => [SFTime, 0, field],
						 # internal sequence number
						 __sourceNumber => [SFInt32, 0, field],
						 # local name, as received on system
						 __localFileName => [SFString, "", exposedField],
						},
						@x = {
							  Initialize => sub {
								  my ($t,$f,$time,$scene) = @_;

								  if ($SoundMaterial eq "Sound") {
									  # Assign a source number to this source
									  $f->{__sourceNumber} = $globalAudioSource++;

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
								  $f->{startTime} = $val;
							  },
							  # Ignore if less than startTime
							  stopTime => sub {
								  my($t,$f,$val) = @_;
								  $f->{stopTime} = $val;
							  },

							ClockTick => sub {
								my($t) = @_;
								VRML::VRMLFunc::MovieTextureClockTick(
									$t->{BackNode}->{CNode});
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
						orientation => [MFRotation, [[0, 0, 1, 0]],field],

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
						__localFileName => [SFString, "",field],
						# time that we were initialized at
						__inittime => [SFTime, 0, field],
					   },
						{
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
								$f->{startTime} = $val;
							},
							# Ignore if less than startTime
							stopTime => sub {
								my($t,$f,$val) = @_;
								$f->{stopTime} = $val;
							},

							ClockTick => sub {
								my($t) = @_;

								VRML::VRMLFunc::AudioClockTick(
									$t->{BackNode}->{CNode});
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
						 bboxSize => [SFVec3f, [-1, -1, -1], field],

						 # fields for reducing redundant calls
						 __do_center => [SFInt32, 0, field],
						 __do_trans => [SFInt32, 0, field],
						 __do_rotation => [SFInt32, 0, field],
						 __do_scaleO => [SFInt32, 0, field],
						 __do_scale => [SFInt32, 0, field],
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
					  ),

	# Complete
	WorldInfo =>
	new VRML::NodeType("WorldInfo",
					   {
						info => [MFString, [], field],
						title => [SFString, "", field]
					   }
					  ),

	#############################################################################################
	# interpolators
	#

	ScalarInterpolator =>
	new VRML::NodeType("ScalarInterpolator",
		   {
			set_fraction => [SFFloat, undef, eventIn],
			key => [MFFloat, [], exposedField],
			keyValue => [MFFloat, [], exposedField],
			value_changed => [SFFloat, 0.0, eventOut]
		   },
		  ),

	OrientationInterpolator =>
	new VRML::NodeType("OrientationInterpolator",
		   {
			set_fraction => [SFFloat, undef, eventIn],
			key => [MFFloat, [], exposedField],
			keyValue => [MFRotation, [], exposedField],
			value_changed => [SFRotation, [0, 0, 1, 0], eventOut]
		   },
		  ),

	###################################################################################
	# PositionInterpolator,ColorInterpolator use same code
	# MAKE SURE FIELD DEFS ARE SAME AND IN SAME ORDER

	ColorInterpolator =>
	new VRML::NodeType("ColorInterpolator",
		   {
			set_fraction => [SFFloat, undef, eventIn],
			key => [MFFloat, [], exposedField],
			keyValue => [MFColor, [], exposedField],
			value_changed => [SFColor, [0, 0, 0], eventOut],
		   },
		  ),

	PositionInterpolator =>
	new VRML::NodeType("PositionInterpolator",
		   {
			set_fraction => [SFFloat, undef, eventIn],
			key => [MFFloat, [], exposedField],
			keyValue => [MFVec3f, [], exposedField],
			value_changed => [SFVec3f, [0, 0, 0], eventOut],
		   },
		  ),

	############################################################################33
	# CoordinateInterpolators and NormalInterpolators use almost the same thing;
	# look at the _type field.

	CoordinateInterpolator =>
	new VRML::NodeType("CoordinateInterpolator",
		{
			set_fraction => [SFFloat, undef, eventIn],
			key => [MFFloat, [], exposedField],
			keyValue => [MFVec3f, [], exposedField],
			value_changed => [MFVec3f, [], eventOut],
			_type => [SFInt32, 0, exposedField], #1 means dont normalize
		},
	  ),



	NormalInterpolator =>
	new VRML::NodeType("NormalInterpolator",
		{
			set_fraction => [SFFloat, undef, eventIn],
			key => [MFFloat, [], exposedField],
			keyValue => [MFVec3f, [], exposedField],
			value_changed => [MFVec3f, [], eventOut],
			_type => [SFInt32, 1, exposedField], #1 means normalize
		},
	  ),

	###################################################################################


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
						 # time that we were initialized at
						 __inittime => [SFTime, 0, field],
						# cycleTimer flag.
						__ctflag =>[SFTime, 10, exposedField]
					   },
					   {
						ClockTick => sub {
							my($t) = @_;

							VRML::VRMLFunc::TimeSensorClockTick(
								$t->{BackNode}->{CNode});
						},
					   }
					  ),


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
						translation_changed => [SFVec3f, [0, 0, 0], eventOut],
						# where we are at a press...
						_origPoint => [SFVec3f, [0, 0, 0], field],
					   },
					  ),

	SphereSensor =>
	new VRML::NodeType("SphereSensor",
					   {
						autoOffset => [SFBool, 1, exposedField],
						enabled => [SFBool, 1, exposedField],
						offset => [SFRotation, [0, 1, 0, 0], exposedField],
						isActive => [SFBool, 0, eventOut],
						rotation_changed => [SFRotation, [0, 0, 1, 0], eventOut],
						trackPoint_changed => [SFVec3f, [0, 0, 0], eventOut],
						# where we are at a press...
						_origPoint => [SFVec3f, [0, 0, 0], field],
						_radius => [SFFloat, 0, field],
					   },
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
					  ),


	VisibilitySensor =>
	new VRML::NodeType("VisibilitySensor",
					   {
						center => [SFVec3f, [0, 0, 0], exposedField],
						enabled => [SFBool, 1, exposedField],
						size => [SFVec3f, [0, 0, 0], exposedField],
						enterTime => [SFTime, -1, eventOut],
						exitTime => [SFTime, -1, eventOut],
						isActive => [SFBool, 0, eventOut]
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
							my($t) = @_;

							VRML::VRMLFunc::ProximitySensorClockTick(
								$t->{BackNode}->{CNode});
						},
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
					   },
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
						ClockTick => sub {
							my($t) = @_;

							VRML::VRMLFunc::CollisionClockTick(
								$t->{BackNode}->{CNode});
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
							my($node, $f, $time, $scene) = @_;

							my $urls = $f->{url};
							my $proto;

							my ($text, $url) = getTextFromURLs($scene, $urls, $node);
							if ($text) {
								$proto = $scene->new_proto("__proto".$protono++);
								$node->{ProtoExp} = $proto;
								$node->{ProtoExp}->set_parentnode($node);
								$node->{IsProto} = 1;
								$node->{ProtoExp}{IsInline} = 1;

								$proto->set_url($url);
								$proto->set_world_url($url);

								VRML::Parser::parse($proto, $text);
								$node->{ProtoExp}->make_executable();
							} else {
								warn("Unable to locate a valid url from ".VRML::Debug::toString($urls));
							}

							return ();
						}
					   }
					  ),

); ##%VRML::Nodes


1;

