# Copyright (C) 1998 Tuomas J. Lukka, 1999 John Stewart CRC Canada
# DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
# See the GNU Library General Public License (file COPYING in the distribution)
# for conditions of use and redistribution.

# The following is POD documentation for the VRML::Browser module
# It will be automatically installed in a manual page upon make install.
# See the perlpod manpage for the I<>, C<> &c explanation

=head1 NAME

VRML::Browser -- perl module to implement a VRML97 and X3D browser

=head1 SYNOPSIS

Use the command-line interface (L<freewrl>), or from within a browser.

=head1 DESCRIPTION

This module implements a VRML browser. The actual module
is of interest only if you are planning to use the code from Perl.

For information on the user interface, see L<VRML::Viewer>.

=head1 AUTHOR

See L<freewrl>.

=cut

require 'VRML/GLBackEnd.pm';
require 'VRML/Parser.pm';
require 'VRML/Scene.pm';
require 'VRML/Events.pm';
require 'VRML/Config.pm';
require 'VRML/URL.pm';

if ($VRML::ENV{AS_PLUGIN}) { require 'VRML/PluginGlue.pm'; }

package VRML::Browser;
use File::Basename;
use strict vars;
use POSIX;

# threaded model requires config.
use Config;

# path for x3d conversion template

my $XSLTpath = "";
my $globalBrowser = "";


###############################################
#
# Public functions

sub new {
	my($type, $pars) = @_;

	my $this = bless {
		Verbose => delete $pars->{Verbose},
		BE => new VRML::GLBackEnd($pars->{CocoaContext},
			$pars->{FullScreen},
			$pars->{Shutter},
			$pars->{EyeDist},
			$pars->{Parent},
			$pars->{ScreenDist},
			@{$pars->{BackEnd} or []}),
		Description => "",
		EV => new VRML::EventMachine(),
		Scene => undef,
		URL => undef,
		JSCleanup => undef,
		CONTEXT => $pars->{CocoaContext},
	 }, $type;

	if (defined $pars->{CocoaContext})
	{
		eval 'use CustomClass';
		eval 'use Foundation';
		eval 'use Foundation::Functions';
		eval 'use AppKit';
		eval 'use AppKit::Functions';
	}

	# for x3d conversions. 
	$XSLTpath = $pars->{XSLTpath};

	# save browser version
	VRML::VRMLFunc::SaveVersion($VRML::Config::vrml_config{VERSION});


	# save this - there is only one browser method EVER, and this makes
	# calling functions from C for EAI easier.

	$globalBrowser = $this;

	return $this;
}

# SD Functions used to set values from Aqua interface, and functions used to pass events from Aqua interface to perl backend
sub setLogFile
{
	my($this, $logfile) = @_;
	open STDOUT, ">$logfile" || die("Can't redirect stdout to $logfile: $!");
	open STDERR, ">&STDOUT" || die("Can't redirect stderr to stdout: $!");
}

sub doScripts 
{
	#\$VRML::DO_PERL = 1;
}

sub setSaveDir
{
	my($this, $savedir) = @_;
	#\$VRML::ENV{FREEWRL_SAVE} = $savedir;
}
	
sub setZbuff
{
	my ($this, $zbuff) = @_;
	VRML::VRMLFunc::set_vpdist($zbuff);
}

sub setSeq
{
	$main::seq = 1;
}

sub setSeqFile
{
	my ($this, $file) = @_;
	$main::seqname = $file;
}

sub setTempSeqFile
{
	my ($this, $file) = @_;
	$main::seqtmp = $file;
}

sub setSnapFile
{
	my ($this, $file) = @_;
	$main::snapname = $file;
}

sub setMaxImg
{
	my ($this, $maxNum) = @_;
	$main::maximg = $maxNum;
}

sub setShutter
{
	my ($this, $shutter) = @_;
	$this->{BE}->setShutter($shutter);
}

sub keyUpWithKey
{
	my ($this, $key) = @_;
	$this->{BE}->event(0.0, 3, $key);
	#print "Got key up event with key $key\n";
}

sub keyDownWithKey
{
	my ($this, $key) = @_;
	$this->{BE}->event(0.0, 2, $key);
	#print "Got key up event with key $key\n";
}

sub setArrowCursor
{
	my($this, $cursor) = @_;
	$this->{BE}->{ARROWCURSOR} = $cursor;
}

sub setCrossCursor
{
	my ($this, $cursor) = @_;
	$this->{BE}->{CROSSCURSOR} = $cursor;
}

sub setEyeDist
{
	my ($this, $eyes) = @_;
	my @args = split(/ /, $eyes);
	$this->{BE}->setEyeDist($args[0], $args[1]);
}

sub mouseDownAt
{
	my ($this, $xvalue) = @_;
        my @args = split(/ /, $xvalue);
	#print "Got mouse down for button $args[2] at $args[0] $args[1]\n";
	$this->{BE}->event(0.0, 4, $args[2], $args[0], $args[1]);
	$this->{BE}->finish_event;
}

sub mouseUpAt
{
	my ($this, $xvalue) = @_;
        my @args = split(/ /, $xvalue);
	#print "Got mouse up for button $args[2] at $args[0] $args[1]\n";
	$this->{BE}->event(0.0, 5, $args[2], $args[0], $args[1]); 
	$this->{BE}->finish_event;
}

sub mouseMovedAt
{
	my ($this, $xvalue) = @_;
        my @args = split(/ /, $xvalue);
	#print "Got mouse moved for button $args[2] at $args[0] $args[1]\n";
	$this->{BE}->event(0.0, 6, $args[2], $args[0], $args[1]);
	$this->{BE}->finish_event;
}
sub rectChanged
{
	my ($this) = @_;
	my $customClass = CustomClass->alloc->init;
	$customClass->setView($this->{CONTEXT}->view);
	my $ycoor = $customClass->getWidth;
	my $xcoor = $customClass->getHeight;
	$this->{BE}->updateCoords($xcoor, $ycoor);
	$this->{BE}->finish_event;
}
# End of Aqua interface functions


########################################################################
#
# load the initial file at startup
#
# Discards previous scene
#
#########################################################################
sub load_file_intro {
	my($this, $url) = @_;
	my $string;

	# save this for getworldurl calls...
	$this->{URL} = $url;

	VRML::VRMLFunc::SaveURL($url);

	$this->clear_scene();
	$this->{Scene} = VRML::Scene->new($this->{EV}, $url, $url);

	$this->{Scene}->set_browser($this);

	if (defined $url) {
		$string = VRML::URL::get_absolute($url);
	}

	# Required due to changes in VRML::URL::get_absolute in URL.pm:
	if (!$string) { print "\nFreeWRL Exiting -- File $url was not found.\n"; 
		exit(1);}

	$this->load_string($string, $url);

       	prepare ($this);

	# debugging scene graph call: 
	$this->{Scene}->dump(0) if $VRML::verbose::scenegraph;
}

########################################################################

# actually load the file and parse it.
sub load_string {
	my($this,$string,$file) = @_;

	my $type = 0;

	#print "load_string, string is $string\nload_string file is $file\n";
	# type is 0 for VRML v2, 1 for xml

	if ($string =~ /^#VRML V2.0/s) {
		$type = 0;
	} elsif($string =~ /^#VRML V1.0/s) {
			print "VRML V1.0, I only know V2.0";
			return;
	} elsif ($string =~/^<\?xml version/s) {
		$type = 1;
	} else {
		#warn("WARNING: file $file doesn't start with the '#VRML V2.0' header line");
		$type = 0;
	}
	if ($type == 1)  {
               $string = convertX3D($string);
	}
	VRML::Parser::parse($this->{Scene},$string);
}




########################################################################
sub clear_scene {
	my($this) = @_;
	delete $this->{Scene};
}

sub get_scene {
	my($this) = @_;
	$this->{Scene} or ($this->{Scene} = VRML::Scene->new(
		$this->{EV}, "USER"));
}
sub get_eventmodel { return $_[0]->{EV} }

sub get_backend { return $_[0]{BE} }


########################################################################
sub eventloop {
	my($this) = @_;
	## my $seqcnt = 0;
	while (!$this->{BE}->quitpressed) {
		# print "eventloop\n";

		# Events from within C; do we have a replaceWorld or goto Viewpoint action?

		if (VRML::VRMLFunc::BrowserAction()) {
			my $action;
			my $try;
			my $url;
			my @stgs;
			my $string;

			VRML::VRMLFunc::getAnchorBrowserAction($action); 

			# split the string up, if there are more items than one. 
			@stgs = split (" ",$action);
			foreach $try (@stgs) {
				($string, $url) = VRML::NodeType::getTextFromURLs($this->{Scene}, $try);
				if (defined $string) {
					$this->load_file_intro($url);
				}
			}
		}

		$this->tick();
		# Skip 1st image, which may not be good
		if ( $main::seq && $main::saving && ++$main::seqcnt ) {
			# Too many images. Stop saving, do conversion
			if ($main::seqcnt > $main::maximg) {
				print "Saving off : sequence too long (max is $main::maximg)\n" ;
				VRML::Browser::convert_raw_sequence();
				# @main::saved = (); # Reset list of images
				# $main::seqcnt = 1;
				# $main::saving = 0;

			} else {
				## print " this : $this\n";
				## print " BE   : $this->{BE}\n";
				my $s2 = $this->{BE}->snapshot();
				my $fn = "$main::seqtmp/$main::seqname" . 
					sprintf("%04d",$main::seqcnt) . 
						".$s2->[0].$s2->[1].raw" ;

				# Check temp dir
				if ( ! (-d $main::seqtmp) && ! mkdir($main::seqtmp,0755) ) {
					print (STDERR "Can't create $main::seqtmp,",
						   " so can't save sequence\n");
					$main::seqcnt = 0;
					$main::saving = 0;
					print "Saving off : Can't save temp files\n";

					# Refuse to crush files. Maybe "convert" has not
					# finished its job. 
				} elsif (-f $fn) {

					print (STDERR "File '$fn' already exists.\n",
						   "  Maybe previous sequence has not been converted\n".
						   "  Maybe you should remove it by hand\n") ;
					$main::seqcnt = 0;
					$main::saving = 0;
					print "Saving off : Won't crush file\n";

				} elsif (open (O, ">$fn")) {
					print O $s2->[2] ;
					close O ;
					push @main::saved, [$fn, $s2->[0], $s2->[1], $main::seqcnt]; 
				} else {
					print STDERR "Can't open '$fn' for writing\n";
				}
			}
		}
	}
	$this->shut();
}

sub prepare {
	my($this) = @_;
	my $bind = 1;

	$this->{Scene}->make_executable();
	my $bn = $this->{Scene}->make_backend($this->{BE});
	$this->{Scene}->setup_routing($this->{EV}, $this->{BE});
	$this->{Scene}->init_events($this->{EV}, $this->{BE}, $bind);
	
	# display this one
	$this->{BE}->set_root($bn);

	$this->{EV}->print;
}

sub shut {
	my($this) = @_;

	if ($VRML::ENV{AS_PLUGIN}) {
		VRML::PluginGlue::closeFileDesc($VRML::PluginGlue::globals{pluginSock});
	}

	if ($this->{JSCleanup}) {
		&{$this->{JSCleanup}}();
	}

	VRML::VRMLFunc::do_CRoutes_free();
	VRML::VRMLFunc::do_EAI_shutdown();
	$this->{BE}->close_screen();
}


sub tick {
	my($this) = @_;
	my $time = VRML::VRMLFunc::get_timestamp();

	#handle app/os events.
	$this->{BE}->handle_events($time);

	#update viewer position (first draft)
	VRML::VRMLFunc::do_handle_tick();

	#setup projection.
	#activate proximity sensors.
	$this->{BE}->render_pre();

	$this->{EV}->propagate_events($time, $this->{BE}, $this->{Scene});

	#do actual screen writing
	$this->{BE}->render();

	#handle EAI.
	VRML::VRMLFunc::do_handle_EAI();
}

my $FPS = 0;

# Viewpoints are stored in the browser rather in the 
# individual scenes...

#sub register_vp {
#	my ($this, $scene, $node) = @_;
#	VRML::NodeType::register_vp($scene, $node);
#}

sub get_vp_node {
	return VRML::NodeType::get_vp_node();
}
sub get_vp_scene {
	return VRML::NodeType::get_vp_scene();
}
sub set_next_vp {
	VRML::NodeType::set_next_vp();
}



# The routines below implement the browser object interface.

sub getCurrentSpeed { return 0.0; } # legal
sub getCurrentFrameRate { return $FPS; }

sub setDescription {
	my ($this, $desc) = @_;
	$this->{Description} = $desc;
	print "Set description: $desc\n"; ## may do more later
} # Read the spec: 4.12.10.8 ;)

sub replaceWorld {
	# make a new scene, this is very similar to load_string, found
	# in this file.

	my ($this, $string) = @_;
	my @newnodes = ();
	my $n;

	print "replaceWorld, string is $string\n";

	# lets go and create the new node array from the nodes as sent in.
	for $n (split(' ',$string)) {
		print "looking at element $n " , VRML::Handles::get($n), "\n";
		@newnodes = (@newnodes,VRML::Handles::get($n));
	}

	$this->{Scene}->topnodes(\@newnodes);
	##JAS $this->clear_scene();
	##JAS $this->{Scene} = VRML::Scene->new($this->{EV}, "from replaceWorld");
	##JAS $this->{Scene}->set_browser($this);
	##JAS $this->{Scene}->topnodes(\@newnodes);
	##JAS prepare ($this);

	# go through the Bindables...
	##JAS for $n (@newnodes) {
	##JAS 	$this->{Scene}->replaceWorld_Bindable($n);
	##JAS }
}

sub loadURL { print "Can't do loadURL yet\n"; }


#createVrml common stuff
sub create_common {
	my ($this,$f1,$f2,$string) = @_;
	my $bind = 0;
	my $ret;

	my $scene = VRML::Scene->new($this->{EV}, $f1,$f2);
	#$scene->set_browser($this);

	VRML::Parser::parse($scene, $string);
	$scene->make_executable();

	# need make_backend, so that Sensors work.
	$scene->make_backend($this->{BE});

	$scene->setup_routing($this->{EV}, $this->{BE});
	$scene->init_events($this->{EV}, $this->{BE}, $bind);
	$ret = $scene->mkbe_and_array($this->{BE}, $scene);

	$scene->dump(0) if $VRML::verbose::scenegraph;

	return $ret;
}

sub createVrmlFromString {
	my ($this, $string) = @_;

	my $wurl = $this->{Scene}->get_world_url();
	return $this->create_common ("FROM A STRING",$wurl,$string);
}

sub createVrmlFromURL {
	my ($this, $file, $url) = @_;

	my $bind = 0;

	# stage 1a - get the URL....
	$url = ($url || $file);
	my $wurl = $this->{Scene}->get_world_url();

	print "File: $file URL: $url\n" if $VRML::verbose::scene;
	my $t = VRML::NodeType::getTextFromURLs($this->{Scene}, $url);

	# Required due to changes in VRML::URL::get_absolute in URL.pm:
	if (!$t) {
		warn("File $file was not found");
		return "";
	}

	unless($t =~ /^#VRML V2.0/s) {
		die("Sorry, this file is according to VRML V1.0. I only know V2.0")
		   if ($t =~ /^#VRML V1.0/s);
		warn("File $file doesn't start with the '#VRML V2.0' header line");
	}

	# Stage 2 - load the string in....
	return $this->create_common($url,$wurl,$t);
}

sub addRoute {
	my ($this, $fn, $ff, $tn, $tf) = @_;

	$this->{Scene}->new_route($fn, $ff, $tn, $tf);
	## not initializing Bindables -- necessary???
	$this->{Scene}->setup_routing($this->{EV}, $this->{BE});
}

sub deleteRoute {
	my ($this, $fn, $ff, $tn, $tf) = @_;

	$this->{Scene}->delete_route($fn, $ff, $tn, $tf);
	$this->{Scene}->setup_routing($this->{EV}, $this->{BE});
}

#######################################################################
#
# X3D Conversion routines.
#
#######################################################################
sub convertX3D {
	my ($string) = @_;

	# x3d - convert this to VRML.
        my $lgname = $ENV{LOGNAME};
        my $tempfile_name = "/tmp/freewrl_xmlConversionFile__";
        my $tempfile1 = join '', $tempfile_name,$lgname, ,".in";
        my $tempfile2 = join '', $tempfile_name,$lgname, ,".out";

	# write string to a file for conversion (inefficient, but...)
	open(fileOUT, ">$tempfile1") or warn("Can't open xml conversion file for writing: $!");
	print fileOUT "$string\n";
	close(fileOUT);

	# do the conversion
	my $cmd = "$VRML::Browser::XSLTPROC -o $tempfile2 $XSLTpath $tempfile1";

        my $status = system ($cmd);
        warn "X3D conversion problem: $?"
                        unless $status == 0;

	# read the VRML in.
	$string = `cat $tempfile2`;  

	# remove the two temporary files
	unlink ($tempfile1);
	unlink ($tempfile2);

	return $string;
}

#######################################################################
sub save_snapshot {
				# Get snapshot
  my ($this) = @_ ;
  my $s2 = $this->snapshot();
				# Save it
  $main::snapcnt++ ;
  return if $main::snapcnt > $main::maximg ;
  my $outname = sprintf("$main::snapname.%04d.ppm",$main::snapcnt);
  my $cmd = "$VRML::Browser::CONVERT -depth 8 -flip -size  $s2->[0]x$s2->[1] rgb:- $outname";
  print "Saving snapshot : Command is '$cmd'\n";

  if (open CONV, "|$cmd") {
    print CONV $s2->[2] ;
    close CONV ;
  } else {
    print STDERR "Can't open pipe for saving image\n";
    $main::snapcnt-- ;
  }
}



# use Perl to save the snapshot sequences. From: Aleksandar Donev <adonev@Princeton.EDU>

#sub save_snapshot_aleksandar {
#	# Get snapshot
#	my ($this) = @_ ;
#	# print "This=$this\n";
#	my $s2 = $this->snapshot();
#	$main::snapcnt++ ;
#	return if $main::snapcnt > $main::maximg ;
#
#	my $outname = sprintf("$main::snapname.%04d.ppm",$main::snapcnt);
#	my $cmd = "$VRML::Browser::CONVERT -flip -depth 8 -size $s2->[0]x$s2->[1] rgb:- $outname";
#	# my $cmd = "rawtoppm $s2->[0] $s2->[1] | pnmflip -tb > $outname.raw.ppm";
#	# my $cmd = "cat > $outname.rgb";
#	# print "Saving snapshot : Command is '$cmd'\n";
#
#	# Use ImageMagick directly
#	use Image::Magick;
#	my($x,$image);
#	$image=Image::Magick->new(magick=>'rgb', depth=>8, size=>'300x300', quality=>75);
#	$x=$image->BlobToImage($s2->[2]);
#	warn "$x\n" if "$x";
#	$x=$image->Flip;
#	$x=$image->Write($outname);
#	warn "$x\n" if "$x";
#} # A. Donev



## Eventually convert a sequence of saved raw images. 
##
## Uses variables :
##
## @main::saved : list of images, with size and seq
##                number. @main::saved is reset. 
##
## $main::convtompg : If true, save as mpg rather than a single gif file.
##
## $main::seqname :
sub convert_raw_sequence   {

	return unless @main::saved ;

	my $cmd ;
	my $outfile;

	if (! $main::convtompg){
		$outfile = "$main::seqname.mpg";
	} else {
		$outfile = "$main::seqname.gif";
	}	

	my $sz = join " ",
		map {"-flip -size $_->[1]x$_->[2] rgb:$_->[0]"} @main::saved ;
	$cmd = "$VRML::Browser::CONVERT -depth 8 ".
			" -delay 70 $sz $outfile";
	#} else {	# Convert to ppm
	#	$cmd = join ";\n",
	#		map {"$VRML::Browser::CONVERT -depth 8 ".
	#		"-size $_->[1]x$_->[2] rgb:$_->[0] ".
	#		"-flip $main::seqtmp/$main::seqname.".sprintf("%04d",$_- >[3]).".ppm"
	#		} @main::saved ;
	#}
	#JAS print "$$ : cmd is : \n-------\n$cmd\n------\n";

	# Fork so that system call won't hang browser
	my $p = fork (); 
	if (!defined($p)){
		print STDERR "could not fork to convert image sequence\n";
	} elsif ($p == 0) {
		my $nok = system $cmd;
		# print "nok is $nok\n";
		# JAS - convert to mpg format returns an error, at least in RH 7.3
		# JAS - even if it is successful. Figure that one out...
		#JAS if ($nok) {
		#JAS 	print STDERR "convert failed. keeping raw images\n";
		#JAS 	@main::saved = ();	# Prevent END from trying again
		#JAS 	exit 1;
		# If all seems ok, remove raw images
		#JAS } else {
			print STDERR "convert successful. unlinking raw images\n";
			map {unlink $_->[0]} @main::saved;
			@main::saved = ();	# Prevent END from trying again
			exit 0;
		#JAS }
	}			
				# Parent process #####################
	@main::saved = ();
	$main::saving = 0;
	$main::seqcnt = 0;
}  # End of conversion of raw images

## If freewrl exits while saving, convert whatever has been saved
END { 
  print "Please wait while sequence is converted\n" if @main::saved ;
  convert_raw_sequence() 
}


################
# EAI Perl functions.

# EAI_GetNode returns "UNDEFINED" for undefined node, or it returns the 
# number of the node so that when the node is manipulated, it can be
# referenced as NODE42. 
#
# It does this because, until a specific field is requested, we can't
# use the generic subroutines that are put in place for routing in Events.pm

my @EAIinfo;

sub EAI_GetNode {
	my ($nodetoget) = @_;

	my $node = $globalBrowser->{Scene}->getNode($nodetoget);

	if (!defined $node) {
		warn("Node $nodetoget is not defined");
		return 0;
	}

	if ("VRML::DEF" eq ref $node) {
		$node = $globalBrowser->{Scene}->getNode(VRML::Handles::return_def_name($nodetoget));
		if (!defined $node) {
			warn("DEF node $nodetoget is not defined");
			return 0;
		}
	}

	my $id = VRML::Handles::reserve($node);

	# print "handle is $id\n";
	$id =~ s/^NODE//;
	# print "node number is $id\n";

	# remember this - this node is displayed already
	#VRML::Handles::displayed($node);

	
	return $id;
}

# get the type, return values used for direct manipulation in C, such as memory location, datasize, etc.
sub EAI_GetType {
	my ($nodenum, $fieldname, $direction) = @_;

	my $outptr;
	my $outoffset;
	my $fieldtype;
	my $retft;

	my $ok;
	my $datalen;
	my $type;		# used for routing to scripts, etc.
	my $intptr;
	my $to_count;
	my $tonode_str;


	# print "BROWSER:EAI_GetType, $nodenum, $fieldname, $direction\n";

	# is this an IS'd field?
	my $realele = VRML::Handles::get("NODE$nodenum");

	# strip off a "set_" or a "_changed" if we should.
	$fieldname = VRML::Parser::parse_exposedField($fieldname, $realele->{Type});

	#print "\nBrowser.pm, fieldname $fieldname, evin: ",
	#	$realele->{Type}{EventIns}{$fieldname}," kinds ",
	#	$realele->{Type}{FieldKinds}{$fieldname},"\n";
	
	#foreach (%{$realele->{Type}{Pars}}) {print "	.... ",@{$_}, " \n";}
	#print "Trying pars directly: ",@{$realele->{Type}{Pars}{$fieldname}} ,"\n";
	#print "\n\n\n";


	# print "BROWSER::EAI_GetType now $fieldname\n";

	if (exists $realele->{Fields}{$fieldname}) {
		# print "BROWSER:EAI - field $fieldname exists in node, it is ",
		# 	$realele->{Fields}{$fieldname},"\n";
	} else {
		# print "BROWSER:EAI - field $fieldname DOES NOT exist in node\n";
		my $ms = $realele->{Scene};		
		my ($xele, $sc, $in, $rn, $rf);

		# try to find this node/field within this scene.
		foreach $xele (@EAIinfo) {
			($sc, $rf, $rn, $in) = @{$xele};
			#print "in $in rf $rf fieldname $fieldname\n";
			if ($ms eq $sc) {  # same scene
				if ($fieldname eq $rf) {
					$realele = $rn;
					$fieldname = $in;
					# print "realele now is $realele, field $fieldname\n";
					goto FOUNDIT;
				}
			}
		}
		FOUNDIT:
	}

	# print "BROWSER:EAI_GetType, realele is ", VRML::NodeIntern::dump_name($realele)," field $fieldname\n";

	
	
	# get info from FreeWRL internals.
	if ($direction eq "eventOut") {
		($outptr, $outoffset, $type, $ok, $datalen, $fieldtype) = $globalBrowser->{EV}->resolve_node_cnode (
        		$globalBrowser->{Scene}, $realele, $fieldname, $direction);

	} else {
        	($to_count, $tonode_str, $type, $ok, $intptr, $fieldtype) = 
				$globalBrowser->{EV}->resolve_node_cnode($globalBrowser->{Scene}, 
					$realele, $fieldname, $direction);

		$datalen = 0; # we either know the length (eg, SFInt32), or if MF, it is the eventOut that
			      # determines the exact length.

		($outptr, $outoffset) = split(/:/,$tonode_str,2); 
		
	}

	#print "Browser, type $type\n";

	# return node pointer, offset, data length, type - see the EAI C code (EAIServ.c) for definitions.
	$retft = 97; 	#SFUNKNOWN
	if ($fieldtype eq "SFBool") {$retft = 98;}
	elsif ($fieldtype eq "SFVec3f") {$retft = 117;}
	elsif ($fieldtype eq "SFColor") {$retft = 99; }# color and vec3f are identical
	elsif ($fieldtype eq "SFFloat") {$retft = 100;}
	elsif ($fieldtype eq "SFTime") {$retft = 101;}
	elsif ($fieldtype eq "SFInt32") {$retft = 102;}
	elsif ($fieldtype eq "SFString") {$retft = 103;}
	elsif ($fieldtype eq "SFNode") {$retft = 104;}
	elsif ($fieldtype eq "SFRotation") {$retft = 105;}
	elsif ($fieldtype eq "SFVec2f") {$retft = 106;}
	elsif ($fieldtype eq "SFImage") {$retft = 107;}
	elsif ($fieldtype eq "MFColor") {$retft = 108;}
	elsif ($fieldtype eq "MFFloat") {$retft = 109;}
	elsif ($fieldtype eq "MFTime") {$retft = 110;}
	elsif ($fieldtype eq "MFInt32") {$retft = 111;}
	elsif ($fieldtype eq "MFString") {$retft = 112;}
	elsif ($fieldtype eq "MFNode") {$retft = 113;}
	elsif ($fieldtype eq "MFRotation") {$retft = 114;}
	elsif ($fieldtype eq "MFVec2f") {$retft = 115;}
	elsif ($fieldtype eq "MFVec3f") {$retft = 116;}
		
	# print "Browser.pm: outptr $outptr offset $outoffset datalen $datalen retft $retft\n";

	return ($outptr, $outoffset, $datalen, $retft, $type); 

}

# EAI_CreateVrmlFromString - parse commands, and return a string of (node-number backnode) pairs.
sub EAI_CreateVrmlFromString {
	my ($string) = @_;

	my $rv = createVrmlFromString ($globalBrowser,$string);

	my @rvarr = split (" ", $rv);
	my %retval = ();
	my $ele;
	my $realele;
	my $bn;

	foreach $ele (@rvarr) {
		$realele = VRML::Handles::get($ele);

		# get the back nodes; but if this is a proto defn, skip it.
		if (exists $realele->{BackNode}{CNode}) {
			$bn = $realele->{BackNode}{CNode};
			$ele =~ s/^NODE//;
			$retval{$ele} = $bn;

			# reserve the CNODE as a node, because sometimes we do need to go
			# from CNode to node.
			VRML::Handles::CNodeLinkreserve("NODE$bn",$realele);
		} else {
			# print "warning, EAI_CreateVrmlFromString - no backnode found for $ele\n";
		}
	}
	return %retval;
}

sub EAI_CreateVrmlFromURL {
	my ($string) = @_;

	my $rv = createVrmlFromURL ($globalBrowser,$string, $string);

	my @rvarr = split (" ", $rv);
	my %retval = ();
	my $ele;
	my $realele;
	my $bn;

	foreach $ele (@rvarr) {
		$realele = VRML::Handles::get($ele);

		# get the back nodes; but if this is a proto defn, skip it.
		if (exists $realele->{BackNode}{CNode}) {
			$bn = $realele->{BackNode}{CNode};
			$ele =~ s/^NODE//;
			$retval{$ele} = $bn;

			# reserve the CNODE as a node, because sometimes we do need to go
			# from CNode to node.
			VRML::Handles::CNodeLinkreserve("NODE$bn",$realele);
		} else {
			# print "warning, EAI_CreateVrmlFromURL - no backnode found for $ele\n";
		}
	}
	return %retval;
}

sub EAI_replaceWorld {
	my ($string) = @_;

	print "BROWSER: replace world with $string\n";
	replaceWorld ($globalBrowser,$string);
}


#######################################################
#
# for protos, we need to know the real node and real field
# for each scene, input node
#
#######################################################
sub save_EAI_info {
	my ($scene, $node, $rn, $in) = @_;

	#print "Browser::save_EAI_info, scene ", VRML::NodeIntern::dump_name ($scene), 
	#	" node:",VRML::NodeIntern::dump_name($node), " real $rn  ISN $in\n";

	push @EAIinfo, [$scene,$in,$node,$rn];
}
#########################################################3
#
# Private stuff

# No other nice place to put this so it's here...
# For explanation, see the file ARCHITECTURE
package VRML::Handles;

{
my %S = ();
my %ONSCREEN = ();
## delete %RP???
my %RP = ();
my %DEFNAMES = ();


# keep a list of DEFined names and their real names around. Because
# a name can be used more than once, (eg, DEF MX ..... USE MX .... DEF MX
# USE MX...) we need to keep track of unique identifers. 
# 
# ALSO: for EAI, we need a way of keeping def names global, as EAI requires
# us to be able to get to Nodes in one scene from another.


## keep refs to DEFs instead??? vrml name kept in DEF instances...

sub def_reserve {
	my ($name, $realnode) = @_;
	$DEFNAMES{$name} = $realnode;
	# print "reserving DEFNAME $name ", ref $name, "is real $realnode, ref ", ref $realnode,"\n";

}
sub return_def_name {
	my ($name) = @_;
	# print "return_def_name, looking for $name , it is ";
	if (!exists $DEFNAMES{$name}) {
		#print "return_def_name - Name $name does not exist!\n";
		return $name;
	}
	return $DEFNAMES{$name};
}

sub displayed {
	my($node) = @_;

	# print "HANDLES::displayed $node\n";
	# this child is displayed here.

	$ONSCREEN{$node} = "yes";
}


sub check_displayed {
	my ($node) = @_;

	# is this child displayed yet?
	if (defined $ONSCREEN{$node}) {
		return 1;
	}
	return 0;
}

## delete these???
# when sending children in EAI, we get the "real nodes". We actually want to
# keep the backwards link, so that real nodes can point to their masters, in
# cases of PROTOS.
sub front_end_child_reserve {
	my ($child, $real) = @_;
	$RP{$child} = $real;
	# print "front_end_child_reserve, reserving for child $child ", ref $child,
	# 	VRML::NodeIntern::dump_name($child),  " real $real ", ref $real,
	# 	VRML::NodeIntern::dump_name($real), "\n";

}

sub front_end_child_get {
	my ($handle) = @_;
	# print "front_end_child_get looking for $handle ",ref $handle,"\n";
	if (!exists $RP{$handle}) {
		# print "front_end_child_get Nonexistent parent for child !\n";
		return $handle;
	}
	# print "front_end_child_get, returning for $handle ", $RP{$handle},"\n";
	return $RP{$handle};
}
######

sub CNodeLinkreserve {
	my($str,$object) = @_;
	# print "Handle::CNodeLinkreserve, reserving $str for object $object type ", ref($object), "\n";

	if(!defined $S{$str}) {
		$S{$str} = [$object, 0];
	}
	$S{$str}[1]++;
	return $str;
}
sub reserve {
	my($object) = @_;
	my $str = VRML::NodeIntern::dump_name($object);
	# print "Handle::reserve, reserving $str for object $object type ", ref($object), "\n";

	if(!defined $S{$str}) {
		$S{$str} = [$object, 0];
	}
	$S{$str}[1]++;
	return $str;
}

sub release {
	my($object) = @_;
	if(--$S{"$object"}[1] <= 0) {
		delete $S{"$object"};
	}
}

sub get {
	my($handle) = @_;
	return NULL if $handle eq "NULL";

	if(!exists $S{$handle}) {
		return $handle;
	}
	return $S{$handle}[0];
}
sub check {
	my($handle) = @_;
	return NULL if $handle eq "NULL";
	if(!exists $S{$handle}) {
		return 0;
	}
	return 1;
}
}

1;
