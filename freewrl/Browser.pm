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
	my $imgcnt = 0;
	# print "eventloop ";
	while(!$this->{BE}->quitpressed) {
		$this->tick();
                if( $main::save && $imgcnt++ ) # Skip 1st image, which may not be good
                {
                    my $s2 = $this->{BE}->snapshot();
                    my $fn = "seqs/seq" . sprintf("%04d",$imgcnt) . 
                        ".$s2->[0].$s2->[1].raw" ;
                    if( $imgcnt<100 && 
                        open( O, ">$fn" ))
                    {
                        $imgcnt++ ;
                        print O $s2->[2] ;
                        close O ;
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
	# print "api__sendEvent, sending $val to $node, field $field\n";
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

sub api__make_MFNode {
	my ($this,$SFNode) = @_;
	return VRML::NodeType::make_MFNode($SFNode);
}

sub api__setMFNode {
	my($this,$node,$f,$v,$flag) = @_;
	
	# this = method
	# node = node to add to.
	# f    = field of node to add to.
	# v    = string id of node to add to field of object node.
	# flag = if 1, this is an add or remove, else absolute set.

	# Step 1.need to sanitize $v... it probably has trailing newline...
	$v =~ s/^\s+//;
	$v =~ s/\s+$//;

	# Is the parent displayed yet??? if not, then save it, and
	# add it in AFTER the parent has been added.

	if (VRML::Handles::check_displayed ($node)) {

		# Step 2. Add it to the scene.
		VRML::NodeType::add_MFNode ($node, $f, $v, $flag);
	
		# Step 3: Set up the new backend, routing, etc, etc...
		# (this is like prepare in the Browser.pm file, except that
		# make_executable was already done, and the rest need only to
		# be done on the node and lower...

##JAS	        $this->{Scene}->make_backend($this->{BE});
	        $this->{Scene}->setup_routing($this->{EV}, $this->{BE});
	        $this->{Scene}->init_routing($this->{EV},$this->{BE});
	        $this->api__sendEvent($node, $f, $node->{Fields}{$f});

		# yes!!! register this node as being displayed.
		VRML::Handles::displayed ($v);
	
		# any children to add here, if it is a "delayed" add???
		# (see above, in the check_displayed function)
		my $unborn = VRML::Handles::unborn_children($v);

		for (split /,/, $unborn) {
			# print "doing the unborn for $_\n";
			my ($nf, $nv) = split " ",$_;
			# print "doing the stuff for $nf, $nv\n";
			api__setMFNode($this, VRML::Handles::get($v), $nf, $nv, $flag);	
		};

		if($VRML::verbose::eai) {
		  print "setting field $f in node $node to $v\n";
		  my $s = " $node->{TypeName} { \n";
		  for(keys %{$node->{Fields}}) {
		          $s .= "\n $_ ";
		          if("VRML::IS" eq ref $node->{Fields}{$_}) {
		                  $s .= "(is) ";
	
		                  $s .= $node->{Fields}{$_}->as_string();
		          } else {
		                  $s .= "VRML::Field::$node->{Type}{FieldTypes}{$_}"->
		                          as_string($node->{Fields}{$_});
		          }
		  }
		  $s .= "}\n";
		  print $s;
		  print "do_add, print done\n\n";
		
		}
	} else {
		VRML::Handles::save_for_later ($node, $f, $v);
	}
}


sub api__unsetMFNode {
	my($this,$node,$f, $v) = @_;
	
	# this = method
	# node = node to add to.
	# f    = field of node to add to.
	# v    = string id of node to add to field of object node.

	# need to sanitize $v... it probably has trailing newline...
	$v =~ s/^\s+//;
	$v =~ s/\s+$//;
	
	#print "api__remove_child - removing $v from $node\n";

	# do it, and display it...
	VRML::NodeType::remove_MFNode ($node, $f, $v);
	$this->api__sendEvent($node, $f, $node->{Fields}{$f});
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

	# print "HANDLES::save_for_later for $node is $SEhash{$node}\n";
}

sub unborn_children {
	my ($node) = @_;
	my $mm = $SEhash{$node};
	# print "HANDLES::unborn_children ifor $node are $mm\n";
	delete $SEhash{$node};
	return $mm;
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
