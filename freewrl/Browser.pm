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
		#IsThreaded => $Config{useithreads},
		IsThreaded => undef,
		ParseThread => undef,
		ImageThread => undef,
		toParseQueue =>undef,
		fromParseQueue => undef,
	 }, $type;

	if (defined $pars->{CocoaContext})
	{
		eval 'use CustomClass';
		eval 'use Foundation';
		eval 'use Foundation::Functions';
		eval 'use AppKit';
		eval 'use AppKit::Functions';
	}

	# create threads, if we are indeed threaded
	if (defined $this->{IsThreaded}) {
		eval 'threads';
		eval 'require Thread::Queue';

		# main to parser thread queues
		$this->{toParseQueue} = Thread::Queue->new;
		$this->{fromParseQueue} = Thread::Queue->new;

		#the parser thread
		$this->{ParseThread} = threads->new(\&load_file_threaded, $this);
	}
	# for x3d conversions. 
	$XSLTpath = $pars->{XSLTpath};

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

	# save this for getworldurl calls...
	$this->{URL} = $url;

	$this->clear_scene();
	$this->{Scene} = VRML::Scene->new($this->{EV}, $url, $url);

	$this->{Scene}->set_browser($this);

	#print "isThreaded is ",$this->{IsThreaded},"\n";
	if (!defined $this->{IsThreaded}) {
		$this->load_file_nothreads($url,undef);
        	prepare ($this);
		# and, take care of keeping the viewpoints active...
		# JAS $this->{Scene}->register_vps($this);
		# debugging scene graph call: 
		$this->{Scene}->dump(0) if $VRML::verbose::scenegraph;
	} else {
		$this->load_string("#VRML V2.0\n Group{}\n","initial null scene");
		prepare($this);

		# send the command and file name to the Parser thread	
		# it will be loaded into the scene graph in the event loop.
		$this->{toParseQueue}->enqueue("loadFile");
		$this->{toParseQueue}->enqueue($url)
	}
}

# parse a file or string in threaded mode
sub load_file_threaded {
	my($this) = @_;

	my $command;
	my $data;
	my $url;

print "load_file_threaded, this is $this\n";

	# lets wait 
	threads->yield;


	# Loop, get the command from the main thread.
	while ($command = $this->{toParseQueue}->dequeue) {
		$url= $this->{toParseQueue}->dequeue;
		print "load_file_threaded: command $command url $url\n" ;#JAS if $VRML::verbose::scene;

		# if command is "loadFile"
		$data = VRML::URL::get_absolute($url);

		print "load_file_threaded, string is $data\n";

		# Required due to changes in VRML::URL::get_absolute in URL.pm:
		if (!$data) { 
			print "\nFreeWRL Exiting -- File $url was not found.\n"; 
			print "stopping threads...\n";
		}

		$this->{URL} = $url;
		$this->clear_scene();
		$this->{Scene} = VRML::Scene->new($this->{EV}, $url, $url);

		$this->load_string($data, $url);

		$this->{Scene}->make_executable();

		my $ret = $this->{Scene}->mkbe_and_array($this->{BE}, $this->{Scene});
		print "VRML::Browser::createVrmlFromUrl: mkbe_and_array returned $ret\n"
			;#JAS if $VRML::verbose::scene;
		# debugging scene graph call
		$this->{Scene}->dump(0) ;#JAS if $VRML::verbose::scenegraph;
	


# DO THIS WHEN LOADED AS CHILD OF ROOT
	#prepare($this);
		# and, take care of keeping the viewpoints active...
		# JAS $this->{Scene}->register_vps($this);
		# debugging scene graph call: 
		#$this->{Scene}->dump(0) if $VRML::verbose::scenegraph;

		# push results back to browser
	}
	print "parse thread exiting\n";
}


sub load_file_nothreads {
	my($this,$url,$string) = @_;
	print "load_file_nothreads: URL: $url\n" if $VRML::verbose::scene;

	if (defined $url) {
		$string = VRML::URL::get_absolute($url);
	}

	# Required due to changes in VRML::URL::get_absolute in URL.pm:
	if (!$string) { print "\nFreeWRL Exiting -- File $url was not found.\n"; 
exit(1);}

	$this->load_string($string, $url);
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
			my @stgs;
			my $string;

			VRML::VRMLFunc::getAnchorBrowserAction($action); 

			# split the string up, if there are more items than one. 
			@stgs = split (" ",$action);
			foreach $try (@stgs) {
				$string = VRML::URL::get_absolute($try); 		
				if (defined $string) {
					$this->load_file_intro($try);
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
	$this->{Scene}->make_backend($this->{BE});
	$this->{Scene}->setup_routing($this->{EV}, $this->{BE});
	$this->{Scene}->init_events($this->{EV}, $this->{BE}, $bind);
	$this->{EV}->print;
}

# prepare2 - re-do just the routing when adding/removing a child. This should
# NOT be this static. XXX

sub prepare2 {
	my($this) = @_;
	my $bind = 0;

	$this->{Scene}->setup_routing($this->{EV}, $this->{BE});
	$this->{Scene}->init_events($this->{EV}, $this->{BE}, $bind);
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
	$this->{BE}->close_screen();
}


sub tick {
	my($this) = @_;
	my $time = get_timestamp();


	#handle app/os events.
	$this->{BE}->handle_events($time);

	#update viewer position (first draft)
	#AK - #$this->{BE}->{Viewer}->handle_tick($time);
	VRML::VRMLFunc::do_handle_tick($time);

	#setup projection.
	#activate proximity sensors.
	$this->{BE}->render_pre();

	$this->{EV}->propagate_events($time,$this->{BE},
		$this->{Scene});

	#do actual screen writing
	$this->{BE}->render();

	for(@{$this->{Periodic}}) {
		&$_();
	}
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

sub getName { return VRML::NodeType::getName(); }
sub getVersion { return $VRML::Config{VERSION}; }
sub getCurrentSpeed { return 0.0; } # legal
sub getCurrentFrameRate { return $FPS; }

sub setDescription {
	my ($this, $desc) = @_;
	$this->{Description} = $desc;
	print "Set description: $desc\n"; ## may do more later
} # Read the spec: 4.12.10.8 ;)

sub getWorldURL {
	my ($this) = @_;
	return $this->{URL};
}

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

	$this->clear_scene();
	$this->{Scene} = VRML::Scene->new($this->{EV}, "from replaceWorld");
	$this->{Scene}->set_browser($this);
	$this->{Scene}->topnodes(\@newnodes);
	prepare ($this);

	# go through the Bindables...
	for $n (@newnodes) {
		$this->{Scene}->replaceWorld_Bindable($n);
	}
	# and, take care of keeping the viewpoints active...
	# JAS $this->{Scene}->register_vps($this);
}

sub loadURL { print "Can't do loadURL yet\n"; }

sub createVrmlFromString {
	my ($this, $string) = @_;
	my $bind = 0;

	my $wurl = $this->{Scene}->get_world_url();
	my $scene = VRML::Scene->new($this->{EV}, "FROM A STRING", $wurl);

	VRML::Parser::parse($scene, $string);
	$scene->make_executable();
	$scene->setup_routing($this->{EV}, $this->{BE});
	$scene->init_events($this->{EV}, $this->{BE}, $bind);
	my $ret = $scene->mkbe_and_array($this->{BE}, $this->{Scene});

	$scene->dump(0) if $VRML::verbose::scenegraph;

	return $ret;
}

sub createVrmlFromURL {
	my ($this, $file, $url) = @_;
	my $bind = 0;

	# stage 1a - get the URL....
	$url = ($url || $file);
	my $wurl = getWorldURL($this);

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

	my $scene = VRML::Scene->new($this->{EV}, $url, $wurl);
	VRML::Parser::parse($scene, $t);
	$scene->make_executable();
	$scene->setup_routing($this->{EV}, $this->{BE});
	$scene->init_events($this->{EV}, $this->{BE}, $bind);

	my $ret = $scene->mkbe_and_array($this->{BE}, $this->{Scene});
	print "VRML::Browser::createVrmlFromUrl: mkbe_and_array returned $ret\n"
		if $VRML::verbose::scene;
	# debugging scene graph call
	$scene->dump(0) if $VRML::verbose::scenegraph;
	
	return $ret;
}

sub addRoute {
	my ($this, $fn, $ff, $tn, $tf) = @_;

	$this->{Scene}->new_route($fn, $ff, $tn, $tf);
	## not initializing Bindables -- necessary???
	$this->{Scene}->setup_routing($this->{EV}, $this->{BE});
	#AK - #$this->prepare2();
}

sub deleteRoute {
	my ($this, $fn, $ff, $tn, $tf) = @_;

	$this->{Scene}->delete_route($fn, $ff, $tn, $tf);
	#AK - #$this->prepare2();
	$this->{Scene}->setup_routing($this->{EV}, $this->{BE});
}

# EAI & Script node
sub api_beginUpdate { print "no beginupdate yet\n"; exit(1) }
sub api_endUpdate { print "no endupdate yet\n"; exit(1) }

sub api_getNode {
	$_[0]->{Scene}->getNode($_[1]);
}
sub api__sendEvent {
	my($this, $node, $field, $val) = @_;
	$this->{EV}->send_event_to($node, $field, $val);
}

sub api__registerListener {
	my($this, $node, $field, $sub) = @_;
	$this->{EV}->register_listener($node, $field, $sub);
}

sub api__getFieldInfo {
	my($this, $node, $field) = @_;

	my ($k, $t);

	($k,$t) = ($node->{Type}{FieldKinds}{$field},
			   $node->{Type}{FieldTypes}{$field});
	
	return($k,$t);
}

sub api__updateRouting {
	my ($this, $node, $field) = @_;

	$this->{Scene}->update_routing($node, $field);
	$this->prepare2();
}


sub add_periodic { push @{$_[0]{Periodic}}, $_[1]; }



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

#########################################################3
#
# Private stuff

{
	my $ind = 0; 
	#JAS - RH7.1 Perl does not have this routine defined off of the
	# CD. Every system that I have seen (SGI, Sun Linux) returns 100 for
	# the CLK_TCK, so I am just substituting this value here. IT only 
	# affects the FPS, from what I can see.
	#my $start = (POSIX::times())[0] / &POSIX::CLK_TCK;

	# BUG FIX: Tobias Hintze <th@hbs-solutions.de> found that the fps
	# calc could divide by zero on fast machines, thus the need for 
	# the check now.

	my $start = (POSIX::times())[0] / 100;
	my $add = time() - $start; $start += $add;
	sub get_timestamp {
		my $ticks = (POSIX::times())[0] / 100; # Get clock ticks
		$ticks += $add;
		if (!$_[0]) {
			$ind++;;
			if ($ind == 25) {
				$ind = 0;
				if ($ticks != $start) { 
					$FPS = 25/($ticks-$start);
				}
				#print "Fps: ",$FPS,"\n";
				pmeasures();
				$start = $ticks;
			}
		}
		return $ticks;
	}

	{
		my %h; my $cur; my $curt;
		sub tmeasure_single {
			my($name) = @_;
			my $t = get_timestamp(1);
			if (defined $cur) {
				$h{$cur} += $t - $curt;
			}
			$cur = $name;
			$curt = $t;
		}
		sub pmeasures {
			return;
			my $s = 0;
			for (values %h) {
				$s += $_;
			}
			print "TIMES NOW:\n";
			for (sort keys %h) {
				printf "$_\t%3.3f\n",$h{$_}/$s;
			}
		}
	}
}



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

sub reserve {
	my($object) = @_;
	my $str = VRML::NodeIntern::dump_name($object);
	#print "Handle::reserve, reserving $str for object $object type ", ref($object), "\n";

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

