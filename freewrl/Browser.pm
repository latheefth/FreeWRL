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

sub tick {
	#
	# some timing print statements in here, will be
	# removed once timing is ok.
	#
	my($this) = @_;
	my $time = get_timestamp();
	#JAS print "tick 1 ",get_timestamp(),"\n";
	$this->{BE}->update_scene($time);
	#JAS print "tick 2 ",get_timestamp(),"\n";
	$this->{EV}->propagate_events($time,$this->{BE},
		$this->{Scene});
	#JAS print "tick 3 ",get_timestamp(),"\n";
	for(@{$this->{Periodic}}) {
		&$_();
	}
	#JAS print "tick 4 ",get_timestamp(),"\n";
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
sub replaceWorld { print "Can't do replaceworld yet\n"; exit (1) }
sub loadURL { print "Can't do loadURL yet\n"; exit (1) }
sub setDescription { print "Set description: ",
	(join '',reverse split '',$_[1]),"\n" } # Read the spec: 4.12.10.8 ;)

# Warning: due to the lack of soft references, all unreferenced nodes
# leak horribly. Perl 5.005 (to be out soon) will probably
# provide soft references. If not, we are going to make a temporary
# solution. For now, we leak.

sub createVrmlFromString { 


	my ($this,$string) = @_;

	my $scene = VRML::Scene->new($this->{EV},"FROM A STRING, DUH");
	VRML::Parser::parse($scene, $string);
        $scene->make_executable();

	if ($VRML::verbose::eai) {
		print "createVRMLFromString - nodes are ",  $scene->get_as_nodearraystring(), "\n";
	}
	return $scene->get_as_nodearraystring();
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
		print "createVRMLFromURL - nodes are ",  $scene->get_as_nodearraystring(), "\n";
	}

	return $scene->get_as_nodearraystring();
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
	#print "Browser.pm: api__sendEvent, sending $val to $node, field $field\n";
	$this->{EV}->send_event_to($node,$field,$val);
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

# is the child already present in the parent? If so, then return 1, if not, return 0
sub checkChildPresent {
	my ($this, $node, $child) = @_;
	print "Browser.pm:checkChildPresent:: checking $node for child $child\n";
	return VRML::NodeType::checkChildPresent ($node, $child);
}


# EAI is asking to remove this child.
sub removeChild {
	my ($this, $node, $child) = @_;
	print "Browser.pm:removeChild, removing $child from parent $node\n";

	my @av = VRML::NodeType::removeChild ($node, $child);
	print "Browser.pm:removeChild, array now is @av\n";
	return @av;
}

#JAS - NOT NEEDED for 0.27sub api__make_MFNode {
#JAS - NOT NEEDED for 0.27	my ($this,$SFNode) = @_;
#JAS - NOT NEEDED for 0.27	return VRML::NodeType::make_MFNode($SFNode);
#JAS - NOT NEEDED for 0.27}
#JAS - NOT NEEDED for 0.27
#JAS - NOT NEEDED for 0.27sub api__setMFNode {
#JAS - NOT NEEDED for 0.27	my($this,$node,$f,$v) = @_;
#JAS - NOT NEEDED for 0.27	
#JAS - NOT NEEDED for 0.27	# this = method
#JAS - NOT NEEDED for 0.27	# node = node to add to.
#JAS - NOT NEEDED for 0.27	# f    = field of node to add to.
#JAS - NOT NEEDED for 0.27	# v    = string id of node to add to field of object node.
#JAS - NOT NEEDED for 0.27
#JAS - NOT NEEDED for 0.27	# Step 1.need to sanitize $v... it probably has trailing newline...
#JAS - NOT NEEDED for 0.27	$v =~ s/^\s+//;
#JAS - NOT NEEDED for 0.27	$v =~ s/\s+$//;
#JAS - NOT NEEDED for 0.27
#JAS - NOT NEEDED for 0.27	# Is the parent displayed yet??? if not, then save it, and
#JAS - NOT NEEDED for 0.27	# add it in AFTER the parent has been added.
#JAS - NOT NEEDED for 0.27
#JAS - NOT NEEDED for 0.27	if (VRML::Handles::check_displayed ($node)) {
#JAS - NOT NEEDED for 0.27
#JAS - NOT NEEDED for 0.27print "set_MFNode - node $node is displayed.\n";
#JAS - NOT NEEDED for 0.27
#JAS - NOT NEEDED for 0.27		# Step 2. Add it to the scene.
#JAS - NOT NEEDED for 0.27		VRML::NodeType::add_MFNode ($node, $f, $v);
#JAS - NOT NEEDED for 0.27	
#JAS - NOT NEEDED for 0.27		# Step 3: Set up the new backend, routing, etc, etc...
#JAS - NOT NEEDED for 0.27		# (this is like prepare in the Browser.pm file, except that
#JAS - NOT NEEDED for 0.27		# make_executable was already done, and the rest need only to
#JAS - NOT NEEDED for 0.27		# be done on the node and lower...
#JAS - NOT NEEDED for 0.27
#JAS - NOT NEEDED for 0.27###JAS	        $this->{Scene}->make_backend($this->{BE});
#JAS - NOT NEEDED for 0.27
#JAS - NOT NEEDED for 0.27	        $this->{Scene}->setup_routing($this->{EV}, $this->{BE});
#JAS - NOT NEEDED for 0.27	        $this->{Scene}->init_routing($this->{EV},$this->{BE});
#JAS - NOT NEEDED for 0.27	        $this->api__sendEvent($node, $f, $node->{RFields}{$f});
#JAS - NOT NEEDED for 0.27
#JAS - NOT NEEDED for 0.27		# yes!!! register this node as being displayed.
#JAS - NOT NEEDED for 0.27		VRML::Handles::displayed ($v);
#JAS - NOT NEEDED for 0.27	
#JAS - NOT NEEDED for 0.27		# any children to add here, if it is a "delayed" add???
#JAS - NOT NEEDED for 0.27		# (see above, in the check_displayed function)
#JAS - NOT NEEDED for 0.27		my $unborn = VRML::Handles::unborn_children($v);
#JAS - NOT NEEDED for 0.27
#JAS - NOT NEEDED for 0.27		for (split /,/, $unborn) {
#JAS - NOT NEEDED for 0.27			print "doing the unborn for $_\n";
#JAS - NOT NEEDED for 0.27			my ($nf, $nv) = split " ",$_;
#JAS - NOT NEEDED for 0.27			print "doing the stuff for $nf, $nv\n";
#JAS - NOT NEEDED for 0.27			api__setMFNode($this, VRML::Handles::get($v), $nf, $nv);	
#JAS - NOT NEEDED for 0.27		};
#JAS - NOT NEEDED for 0.27		VRML::Handles::all_born($v);
#JAS - NOT NEEDED for 0.27print "now, the unborn are:";
#JAS - NOT NEEDED for 0.27		my $unborn = VRML::Handles::unborn_children($v);
#JAS - NOT NEEDED for 0.27print $unborn;
#JAS - NOT NEEDED for 0.27print "\n";
#JAS - NOT NEEDED for 0.27
#JAS - NOT NEEDED for 0.27#JAS		if($VRML::verbose::eai) {
#JAS - NOT NEEDED for 0.27		  print "setting field $f in node $node to $v\n";
#JAS - NOT NEEDED for 0.27		  my $s = " $node->{TypeName} { \n";
#JAS - NOT NEEDED for 0.27		  for(keys %{$node->{RFields}}) {
#JAS - NOT NEEDED for 0.27		          $s .= "\n $_ ";
#JAS - NOT NEEDED for 0.27		          if("VRML::IS" eq ref $node->{RFields}{$_}) {
#JAS - NOT NEEDED for 0.27		                  $s .= "(is) ";
#JAS - NOT NEEDED for 0.27	
#JAS - NOT NEEDED for 0.27		                  $s .= $node->{RFields}{$_}->as_string();
#JAS - NOT NEEDED for 0.27		          } else {
#JAS - NOT NEEDED for 0.27		                  $s .= "VRML::Field::$node->{Type}{FieldTypes}{$_}"->
#JAS - NOT NEEDED for 0.27		                          as_string($node->{RFields}{$_});
#JAS - NOT NEEDED for 0.27		          }
#JAS - NOT NEEDED for 0.27		  }
#JAS - NOT NEEDED for 0.27		  $s .= "}\n";
#JAS - NOT NEEDED for 0.27		  print $s;
#JAS - NOT NEEDED for 0.27		  print "do_add, print done\n\n";
#JAS - NOT NEEDED for 0.27		
#JAS - NOT NEEDED for 0.27#JAS		}
#JAS - NOT NEEDED for 0.27	} else {
#JAS - NOT NEEDED for 0.27print "set_MFNode - saving node $node for later display.\n";
#JAS - NOT NEEDED for 0.27		VRML::Handles::save_for_later ($node, $f, $v);
#JAS - NOT NEEDED for 0.27	}
#JAS - NOT NEEDED for 0.27}
#JAS - NOT NEEDED for 0.27
#JAS - NOT NEEDED for 0.27
#JAS - NOT NEEDED for 0.27sub api__unsetMFNode {
#JAS - NOT NEEDED for 0.27	my($this,$node,$f, $v) = @_;
#JAS - NOT NEEDED for 0.27	
#JAS - NOT NEEDED for 0.27	# this = method
#JAS - NOT NEEDED for 0.27	# node = node to add to.
#JAS - NOT NEEDED for 0.27	# f    = field of node to add to.
#JAS - NOT NEEDED for 0.27	# v    = string id of node to add to field of object node.
#JAS - NOT NEEDED for 0.27
#JAS - NOT NEEDED for 0.27	# need to sanitize $v... it probably has trailing newline...
#JAS - NOT NEEDED for 0.27	$v =~ s/^\s+//;
#JAS - NOT NEEDED for 0.27	$v =~ s/\s+$//;
#JAS - NOT NEEDED for 0.27	
#JAS - NOT NEEDED for 0.27	#print "api__remove_child - removing $v from $node\n";
#JAS - NOT NEEDED for 0.27
#JAS - NOT NEEDED for 0.27	# do it, and display it...
#JAS - NOT NEEDED for 0.27	VRML::NodeType::remove_MFNode ($node, $f, $v);
#JAS - NOT NEEDED for 0.27	$this->api__sendEvent($node, $f, $node->{RFields}{$f});
#JAS - NOT NEEDED for 0.27}

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
my %SEhash = ();

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

sub save_for_later {
	my ($node, $field, $child) = @_;

	$SEhash{$node} = "$SEhash{$node},$field $child";
	# take off the comma at the beginning, if there
	$SEhash{$node} =~s/^,//;

	print "HANDLES::save_for_later for $node is $SEhash{$node}\n";
}

sub unborn_children {
	my ($node) = @_;
	my $mm = $SEhash{$node};
	# print "HANDLES::unborn_children ifor $node are $mm\n";
	delete $SEhash{$node};
	return $mm;
}

sub all_born {
	my ($node) = @_;
	$SEhash{$node} = "";
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
