# Copyright (C) 1998 Tuomas J. Lukka, 1999 John Stewart CRC Canada
# DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
# See the GNU Library General Public License (file COPYING in the distribution)
# for conditions of use and redistribution.

# The following is POD documentation for the VRML::Browser module
# It will be automatically installed in a manual page upon make install.
# See the perlpod manpage for the I<>, C<> &c explanation

=head1 NAME

VRML::Browser -- perl module to implement a VRML97 browser

=head1 SYNOPSIS

Use the command-line interface (L<freewrl>), or
inside Perl:

	use VRML::Browser;

	$b = VRML::Browser->new();

	$b->load_file($url);
	$b->load_file($url,$base_url);
	$b->load_string("Shape { geometry ....", $base_url);

	$b->eventloop();

	# VRML Browser API
	$name = 	$b->getName();
	$version = 	$b->getVersion();
	$speed =	$b->getCurrentSpeed();
	...

	# The rest of the API may still change and is not documented
	# here. If you need to know, check the file Browser.pm

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

package VRML::Browser;
use File::Basename;
use strict vars;
use POSIX;

###############################################
#
# Public functions

sub new {
	my($type,$pars) = @_;
	my $this = bless {
		Verbose => delete $pars->{Verbose},
		BE => new VRML::GLBackEnd(@{$pars->{BackEnd} or []}),
		EV => new VRML::EventMachine(),
	}, $type;
	return $this;
}

sub clear_scene {
	my($this) = @_;
	delete $this->{Scene};
}

# Discards previous scene
sub load_file {
	my($this,$file,$url) = @_;
	$url = ($url || $file);

	# save this for getworldurl calls...
	$this->{URL} = $url ; 

	print "File: $file URL: $url\n" if $VRML::verbose::scene;
	my $t = VRML::URL::get_absolute($file);
	unless($t =~ /^#VRML V2.0/s) {
		if($t =~ /^#VRML V1.0/s) {
			print "Sorry, this file is according to VRML V1.0, I only know V2.0";
			exit (1);
		}
		warn("WARNING: file '$file' doesn't start with the '#VRML V2.0' header line");
	}
	$this->load_string($t,$url);
}

sub load_string {
	my($this,$string,$file) = @_;
	$this->clear_scene();
	$this->{Scene} = VRML::Scene->new($this->{EV},$file);
	$this->{Scene}->set_browser($this);
	VRML::Parser::parse($this->{Scene},$string);
        prepare ($this);
	# and, take care of keeping the viewpoints active...
	$this->{Scene}->register_vps($this);
}

sub get_scene {
	my($this) = @_;
	$this->{Scene} or ($this->{Scene} = VRML::Scene->new(
		$this->{EV}, "USER"));
}
sub get_eventmodel { return $_[0]->{EV} }

sub get_backend { return $_[0]{BE} }

sub eventloop {
	my($this) = @_;
	## my $seqcnt = 0;
	# print "eventloop ";
	while(!$this->{BE}->quitpressed) {
		$this->tick();
				# Skip 1st image, which may not be good
                if( $main::seq && $main::saving && ++$main::seqcnt )
                {
				# Too many images. Stop saving, do conversion
		  if ($main::seqcnt > $main::maximg){
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
		      
		    } elsif (open (O, ">$fn")){
		      print O $s2->[2] ;
		      close O ;
		      push @main::saved, [$fn, $s2->[0], $s2->[1], $main::seqcnt]; 
		    } else {
		      print STDERR "Can't open '$fn' for writing\n";
		    }
		  }
		}
	      }
}

sub prepare {
	my($this) = @_;

	$this->{Scene}->make_executable();
	$this->{Scene}->make_backend($this->{BE});
	$this->{Scene}->setup_routing($this->{EV}, $this->{BE});
	$this->{Scene}->init_routing($this->{EV},$this->{BE});
	$this->{EV}->print;
}

# prepare2 - re-do just the routing when adding/removing a child. This should
# NOT be this static. XXX

sub prepare2 {
	my($this) = @_;
	#JAS $this->{Scene}->make_executable();
	#JAS $this->{Scene}->make_backend($this->{BE});
	$this->{Scene}->setup_routing($this->{EV}, $this->{BE});
	$this->{Scene}->init_routing($this->{EV},$this->{BE});
}
sub tick {
	#
	my($this) = @_;
	my $time = get_timestamp();

	$this->{BE}->update_scene($time);

	$this->{EV}->propagate_events($time,$this->{BE},
		$this->{Scene});

	for(@{$this->{Periodic}}) {
		&$_();
	}
}

my $FPS = 0;

# Viewpoints are stored in the browser rather in the 
# individual scenes...

sub register_vp {
	my ($this, $scene, $node) = @_;
	VRML::NodeType::register_vp($scene, $node);
}

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

sub getName { return "FreeWRL VRML Browser" }
sub getVersion { return $VRML::Config{VERSION} } 
sub getCurrentSpeed { return 0.0 } # legal
sub getCurrentFrameRate { return $FPS }
sub getWorldURL { return $_[0]{URL} }
sub loadURL { print "Can't do loadURL yet\n"; exit (1) }
sub setDescription { print "Set description: ",
	(join '',reverse split '',$_[1]),"\n" } # Read the spec: 4.12.10.8 ;)

# Warning: due to the lack of soft references, all unreferenced nodes
# leak horribly. Perl 5.005 (to be out soon) will probably
# provide soft references. If not, we are going to make a temporary
# solution. For now, we leak.

sub replaceWorld {
	# make a new scene, this is very similar to load_string, found
	# in this file.

	my ($this,$string) = @_;
	my @newnodes = ();
	my $n;

	print "replaceWorld, string is $string\n";

	# lets go and create the new node array from the nodes as sent in.
	for $n (split(' ',$string)) {
		print "looking at element $n " , VRML::Handles::get($n), "\n";
		@newnodes = (@newnodes,VRML::Handles::get($n));
	}

        $this->clear_scene();
print "1\n";

        $this->{Scene} = VRML::Scene->new($this->{EV},"from replaceWorld");
print "2\n";

        $this->{Scene}->set_browser($this);
print "3\n";
	$this->{Scene}->topnodes(\@newnodes);
print "4\n";

        prepare ($this);

print "4.5\n";

	# go through the Bindables...
	for $n (@newnodes) {
		$this->{Scene}->replaceWorld_Bindable($n);
	}
print "5\n";

        # and, take care of keeping the viewpoints active...
        $this->{Scene}->register_vps($this);
print "6\n";
}


sub createVrmlFromString { 


	my ($this,$string) = @_;

	my $scene = VRML::Scene->new($this->{EV},"FROM A STRING, DUH");
	VRML::Parser::parse($scene, $string);
        $scene->make_executable();
	return $scene->mkbe_and_array($this->{BE});
}

sub createVrmlFromURL { 
	my ($this,$file,$url) = @_;

        # stage 1a - get the URL....
        $url = ($url || $file);

	# stage 1b - is this relative to the world URL base???
	if ($url !~ m/\//) {
		my $wurl = dirname(getWorldURL($this));
		$url = "$wurl\/$url";
	}


        print "File: $file URL: $url\n" if $VRML::verbose::scene;
        my $t = VRML::URL::get_absolute($url);
        unless($t =~ /^#VRML V2.0/s) {
                if($t =~ /^#VRML V1.0/s) {
                        print "Sorry, this file is according to VRML V1.0, I only know V2.0\n"; exit (1);
                }
                warn("WARNING: file '$file' doesn't start with the '#VRML V2.0' header line");        }

	# Stage 2 - load the string in....
        print "loading string, $this $t $url\n" if $VRML::verbose; 

	my $scene = VRML::Scene->new($this->{EV},$url);
	VRML::Parser::parse($scene, $t);
        $scene->make_executable();

	if ($VRML::verbose::eai) {
		print "createVRMLFromURL - nodes are ",  $scene->mkbe_and_array(), "\n";
	}

	return $scene->mkbe_and_array($this->{BE});
 }


sub addRoute {  print "No addroute yet\n"; exit(1) }
sub deleteRoute { print "No deleteroute yet"; exit (1) }

# EAI
sub api_beginUpdate { print "no beginupdate yet\n"; exit(1) }
sub api_endUpdate { print "no endupdate yet\n"; exit(1) }
sub api_getNode { 
	$_[0]->{Scene}->getNode($_[1]);
}
sub api__sendEvent { 
	my($this,$node,$field,$val) = @_;
	$this->{EV}->send_event_to($node,$field,$val);
}

# this one just registers real_node IS's.
my %IS = ();
sub api__register_IS_ALIAS {
	my ($node, $is, $as, $direction) = @_;
	# print "api__register_IS_ALIAS, node $node, is $is as $as direction $direction\n";
	# note, we don't care right now what the direction is... Should we?
	if(!defined $IS{$node}{$is}) {
		$IS{$node}{$is} = $as;
	}

}

sub api__find_IS_ALIAS {
	my ($node, $is) = @_;
	# print "api__find_IS_ALIAS, checking node $node for field $is\n";
        if (!defined $IS{$node}{$is}) {
		# print "api__find_IS_ALIAS returning FALSE \n";
		return FALSE;
	}
	# print "api__find_IS_ALIAS returning ",  $IS{$node}{$is},"\n";
	return $IS{$node}{$is};
}



sub api__registerListener { 
	my($this, $node, $field, $sub) = @_;
	$this->{EV}->register_listener($node, $field, $sub);
}

sub api__getFieldInfo {
	my($this,$node,$field) = @_;
	my($k,$t) = ($node->{Type}{FieldKinds}{$field},$node->{Type}{FieldTypes}{$field});
	return($k,$t);
}


sub add_periodic { push @{$_[0]{Periodic}}, $_[1]; }

# lets find the actual child transform that contains the field - PROTO stuff...
sub  find_transform {
        my ($this,$node, $field) = @_;
	return VRML::NodeType::find_transform ($this, $node, $field);
}

# is the child already present in the parent? If so, then return 1, if not, return 0
sub checkChildPresent {
	my ($this, $node, $child) = @_;
	# print "Browser.pm:checkChildPresent:: checking $node for child $child\n";
	return VRML::NodeType::checkChildPresent ($node, $child);
}


# EAI is asking to remove this child.
sub removeChild {
	my ($this, $node, $child) = @_;
	# print "Browser.pm:removeChild, removing $child from parent $node\n";

	my @av = VRML::NodeType::removeChild ($node, $child);
	# print "Browser.pm:removeChild, array now is @av\n";
	return @av;
}


## Get a snapshot from backend and save it
##
## Uses variables :
##
## $main::snapcnt
## $main::maximg
## $main::snapname
sub save_snapshot {
				# Get snapshot
  my ($this) = @_ ;
  my $s2 = $this->snapshot();
				# Save it
  $main::snapcnt++ ;
  return if $main::snapcnt > $main::maximg ;
  my $outname = sprintf("$main::snapname.%04d.ppm",$main::snapcnt);
  my $cmd = "$VRML::Browser::CONVERT -flip -size  $s2->[0]x$s2->[1] rgb:- $outname";
  print "Saving snapshot : Command is '$cmd'\n";

  if (open CONV, "|$cmd") {
    print CONV $s2->[2] ;
    close CONV ;
  } else {
    print STDERR "Can't open pipe for saving image\n";
    $main::snapcnt-- ;
  }
    
}

## Eventually convert a sequence of saved raw images. 
##
## Uses variables :
##
## @main::saved : list of images, with size and seq
##                number. @main::saved is reset. 
##
## $main::nogif : If true, save many ppm rather than a single gif file.
##
## $main::seqname :
sub convert_raw_sequence   {
  
  return unless @main::saved ;
  my $cmd ;
  # Convert to gif
  if (! $main::nogif){
    # size options for 'convert'
    my $sz = join " ",
    map {"-size $_->[1]x$_->[2] rgb:$_->[0]"} @main::saved ; 
    $cmd = "$VRML::Browser::CONVERT -flip ". 
      " -delay 35 $sz $main::seqname.gif";
    
  } else {	# Convert to ppm
    $cmd = join ";\n",
    map {"$VRML::Browser::CONVERT -flip ".
	   "-size $_->[1]x$_->[2] rgb:$_->[0] ".
	     "$main::seqtmp/$main::seqname.".sprintf("%04d",$_->[3]).".ppm"
	   } @main::saved ;
  }
  print "$$ : cmd is : \n-------\n$cmd\n------\n";

  # Fork so that system call won't hang browser
  my $p = fork (); 
  if (!defined($p)){
    print STDERR "could not fork to convert image sequence\n";
  } elsif ($p == 0) {
    my $nok = system $cmd;
    if ($nok) {
      print STDERR "convert failed. keeping raw images\n";
      @main::saved = ();	# Prevent END from trying again
      exit 1;
      
      # If all seems ok, remove raw images
    } else {
      print STDERR "convert successful. unlinking raw images\n";
      map {unlink $_->[0]} @main::saved;
      @main::saved = ();	# Prevent END from trying again
      exit 0;
    }
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
my $start = (POSIX::times())[0] / &POSIX::CLK_TCK;
my $add = time() - $start; $start += $add;
sub get_timestamp {
	my $ticks = (POSIX::times())[0] / &POSIX::CLK_TCK; # Get clock ticks
	$ticks += $add;
	print "TICK: $ticks\n"
		if $VRML::verbose;
	if(!$_[0]) {
		$ind++;;
		if($ind == 25) {
			$ind = 0;
			$FPS = 25/($ticks-$start);
			print "Fps: ",$FPS,"\n";
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
	if(defined $cur) {
		$h{$cur} += $t - $curt;
	}
	$cur = $name;
	$curt = $t;
}
sub pmeasures {
	return;
	my $s = 0;
	for(values %h) {$s += $_}
	print "TIMES NOW:\n";
	for(sort keys %h) {printf "$_\t%3.3f\n",$h{$_}/$s}
}
}
}



# No other nice place to put this so it's here...
# For explanation, see the file ARCHITECTURE
package VRML::Handles;

{
my %S = ();
my %ONSCREEN = ();

sub displayed {
        my($node) = @_;

        # print "HANDLES::displayed $node\n";
        # this child is displayed here.

        $ONSCREEN{$node} = "yes";
}


sub check_displayed {
        my ($node) = @_;

        # print "HANDLES::check_displayed $node\n";
        # is this child displayed yet?

        if (defined $ONSCREEN{$node}) {
                return 1;
        }
        return 0;
}

sub reserve {
	my($object) = @_;
	my $str = "$object";
	# print "Handle::reserve, reserving $str type ", ref($object), "\n";

	if(!defined $S{$str}) {
		$S{$str} = [$object, 0];
	}
	$S{$str}[1] ++;
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
		print "Nonexistent VRML Node Handle!\n";
		exit (1);
	}
	# print "handle get ", $S{$handle}[0], " ref ", ref($S{$handle}[0]), "\n";
	# print "       type ",$S{$handle}[0]{Type},"\n";
	# print "       typeName ",$S{$handle}[0]{TypeName},"\n";
	return $S{$handle}[0];
}
sub check {
	my($handle) = @_;
	return NULL if $handle eq "NULL";
	if(!exists $S{$handle}) {
		# print ("Handle::check $handle - Not a Node Handle!\n");
		return 0;
	}
	# print "Handle::check ", $S{$handle}[0], " ref ", ref($S{$handle}[0]), "\n";
	return 1;
}
}

1;
