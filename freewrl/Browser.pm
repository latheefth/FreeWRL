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

require DynaLoader;
require POSIX;

require 'VRML/GLBackEnd.pm';
require 'VRML/Parser.pm';
require 'VRML/Scene.pm';
require 'VRML/Events.pm';
require 'VRML/Config.pm';

if ($VRML::ENV{AS_PLUGIN}) { require 'VRML/PluginGlue.pm'; }

package VRML::Browser;
use File::Basename;
use strict vars;
use POSIX;

# threaded model requires config.
use Config;

# path for x3d conversion template

my $XSLTpath = "$VRML::ENV{FREEWRL_BUILDDIR}/x3d/X3dToVrml97.xsl";
my $globalBrowser = "";

###############################################
#
# Public functions

sub new {
	my($type, $pars) = @_;

	my $this = bless {
		Verbose => delete $pars->{Verbose},
		BE => new VRML::GLBackEnd(
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
	 }, $type;

	# save browser version
	VRML::VRMLFunc::SaveVersion($VRML::Config::vrml_config{VERSION});


	# save this - there is only one browser method EVER, and this makes
	# calling functions from C for EAI easier.

	$globalBrowser = $this;

	return $this;
}


##############################################################
#
# Much of the VRML content is gzipped -- we have to recognize
# it in the run.

use IO::File;
use Fcntl;
my $temp_dir = -d '/tmp' ? '/tmp' : $ENV{TMP} || $ENV{TEMP};
my $base_name = sprintf("%s/freewrl-%d-%d-00000", $temp_dir, $$, time());

sub temp_file {
	my $fh = undef;
	my $count = 0;
	until (defined($fh) || $count > 100) {
		$base_name =~ s/-(\d+)$/"-" . (1 + $1)/e;
		$fh = IO::File->new($base_name, O_WRONLY|O_EXCL|O_CREAT,0644)
	}
	if (defined($fh)) {
		undef $fh;
		unlink $base_name;
		return $base_name;
	} else {
		die("Couldn't make temp file");
	}
}

sub is_gzip {
	if($_[0] =~ /^\037\213/) {
		# warn "GZIPPED content -- trying to ungzip\n";
		return 1;
	}
	return 0;
}

sub ungzip_file {
	my($file) = @_;
	if($file !~ /^[-\w~\.,\/]+$/) {
	 warn("Suspicious file name '$file' -- not gunzipping");
	 return $file;
	}
	open URLFOO,"<$file";
	my $a;
	read URLFOO, $a, 10;
	if(is_gzip($a)) {
		print "Seems to be gzipped - ungzipping\n" if $VRML::verbose::url;
		my $f = temp_file();
		system("gunzip <$file >$f") == 0
		 or die("Gunzip failed: $?");
		return $f;
	} else {
		return $file;
	}
	close URLFOO;
}


sub getTextFromFile {
	my ($file,$parentURL) = @_;
	
	# read in data from a file; the file name has been verified
	# to exist before here by "C" functions (or, it is the name
	# of a file in the Browser cache). Read it in, and return

	#print "getTextFromFile, file $file, parent $parentURL\n";
	if (!(-e $file)) {
		# try appending file to parenturl
		my $ri = rindex ($parentURL,"/");
		my $ps = substr ($parentURL,0,$ri+1);
		$file = "$ps$file";
	}
	
	my $nfile = ungzip_file($file);

	open (INPUT, "<$nfile");
	my @lines = (<INPUT>);
	close (INPUT); 

	my $text = "@lines";
	# print "Browser:getTextFromFile, got $text\n";
	if ($nfile ne $file) {
		# unlink this temporary file
		unlink $nfile;
	}

	return $text;
}

sub prepare {
	my($this) = @_;

	$this->{Scene}->make_executable();
	my $bn = $this->{Scene}->make_backend($this->{BE});
	$this->{Scene}->setup_routing($this->{EV}, $this->{BE});
	
	# display this one

	$this->{BE}->set_root($bn); # should eventually be removed
	VRML::VRMLFunc::set_root($bn->{CNode});
	
	

	$this->{EV}->print;
}

sub shut {
	my($this) = @_;

	if ($this->{JSCleanup}) {
		&{$this->{JSCleanup}}();
	}

	VRML::VRMLFunc::do_CRoutes_free();
	VRML::VRMLFunc::do_EAI_shutdown();
	$this->{BE}->close_screen();
}




# Bindable return values.
# Viewpoint (or GeoViewpoint)
# Background
# NavigationInfo
# Fog

my %vpn = (); my $vpcount=0;
my %bgd = (); my $bgcount=0;
my %nav=(); my $navcount=0;
my %fog=(); my $fogcount=0;


# Save this node pointer so that the C backend can get it.
# save a maximum of 900; this is to avoid any stack overflow.
sub register_bind {
	my ($node) = @_;
        if (!defined ($node->{BackNode}{CNode})) {
                print "register_vp - no backend CNode node\n";
                return;
        }

	if ($node->{TypeName} eq "Viewpoint") {
		if ($vpcount>900) {return;}
		$vpn{$vpcount} = $node->{BackNode}{CNode}; $vpcount++;
	} elsif ($node->{TypeName} eq "Background") {
		if ($bgcount>900) {return;}
		$bgd{$bgcount} = $node->{BackNode}{CNode}; $bgcount++;
	} elsif ($node->{TypeName} eq "NavigationInfo") {
		if ($navcount>900) {return;}
		$nav{$navcount} = $node->{BackNode}{CNode}; $navcount++;
	} elsif ($node->{TypeName} eq "Fog") {
		if ($fogcount>900) {return;}
		$fog{$fogcount} = $node->{BackNode}{CNode}; $fogcount++;
	} else {
		print ("Browser:register_bind, unknown type ",$node->{TypeName});
	}
}

# the C side wants a list of specific bindables.
sub getBindables {
	my ($ty) = @_;
	# check to see what bindable we wish
	if ($ty eq "Viewpoint") { return %vpn; }
	elsif ($ty eq "Background") { return %bgd; }
	elsif ($ty eq "NavigationInfo") { return %nav; }
	elsif ($ty eq "Fog") { return %fog; }
	else {
		print ("Browser::getBindables, invalid request $ty\n");
		return ();
	}
}


# The routines below implement the browser object interface.

sub setDescription {
	my ($this, $desc) = @_;
	$this->{Description} = $desc;
	print "Set description: $desc\n"; ## may do more later
} # Read the spec: 4.12.10.8 ;)

#JASsub replaceWorld {
#JAS	# make a new scene, this is very similar to load_string, found
#JAS	# in this file.
#JAS
#JAS	my ($this, $string) = @_;
#JAS	my @newnodes = ();
#JAS	my $n;
#JAS
#JAS	print "replaceWorld, string is $string\n";
#JAS
#JAS	# lets go and create the new node array from the nodes as sent in.
#JAS	for $n (split(' ',$string)) {
#JAS		print "looking at element $n " , VRML::Handles::get($n), "\n";
#JAS		@newnodes = (@newnodes,VRML::Handles::get($n));
#JAS	}
#JAS
#JAS	$this->{Scene}->topnodes(\@newnodes);
#JAS}

sub loadURL { print "Can't do loadURL yet\n"; }


#createVrml common stuff
sub create_common {
	my ($this,$f1,$f2,$string) = @_;
	my $ret;

	my $scene = VRML::Scene->new($this->{EV}, $f1,$f2);
	$scene->set_browser($this);

	# is this an X3D string?
        my $type = 0;
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

	# end of X3D conversion.

	VRML::Parser::parse($scene, $string);
	$scene->make_executable();
	$scene->make_backend($this->{BE});
	$scene->setup_routing($this->{EV}, $this->{BE});
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
	my $wurl = $url;

	print "File: $file URL: $url\n" if $VRML::verbose::scene;
	my $t = getTextFromFile($url);


	# Stage 2 - load the string in....
	return $this->create_common($url,$wurl,$t);
}

sub EAI_Route {
	my ($dir, $str) = @_;
	#print "EAI_Route in Browser,pm, dir $dir, str $str\n";

	my ($fn, $ff, $tn, $tf) = split (" ",$str);
	my $ar = 0;

	$ff = VRML::Parser::parse_exposedField($ff, VRML::Handles::get($fn)->{Type});
	$tf = VRML::Parser::parse_exposedField($tf, VRML::Handles::get($tn)->{Type});

	# the direction is "72" for an add; it has no specific meaning.
	if ($dir == 72) {$ar = 1;}

	$globalBrowser->{EV}->add_route($globalBrowser->{Scene},
			$ar , $fn, $ff, $tn, $tf);
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

	# print "\n\nEAI_GetNode, getting $nodetoget\n";
	# now we change the node into a DEF name.	
	my $node = VRML::Handles::return_def_name($nodetoget);

	#print "step 1, node is $node, ref ",ref $node,"\n";

	# then change the DEF name into a VRML node pointer.
	$node = VRML::Handles::return_EAI_name($node);

	#print "step 2, node is $node, ref ",ref $node,"\n";

	if (!defined $node) {
		warn("Node $nodetoget is not defined");
		return 0;
	}

	my $id = VRML::Handles::reserve($node);

	# print "handle is $id\n";
	$id =~ s/^NODE//;
	# print "node number is $id\n";
	return $id;
}

# get the value for a node; NOT used for EAI, only for .class (and other scripts)
# node is called EAI_... because it "fits" in with the other nodes.

sub EAI_GetValue {
	my ($nodenum, $fieldname) = @_;

	#chop $fieldname;
	print "Browser.pm - EAI_GetValue on node $nodenum, field $fieldname\n";
	my $realele = VRML::Handles::get("NODE$nodenum");
	# strip off a "set_" or a "_changed" if we should.
	$fieldname = VRML::Parser::parse_exposedField($fieldname, $realele->{Type});

	#print "Browser.pm now node $realele, field :$fieldname:\n";
	print "exactvalue is ",$realele->{Fields}{$fieldname},"\n";
	#print "exactvalue2 is ",$realele->{RFields}{$fieldname},"\n";

	#my $key;
	#foreach $key (keys(%{$realele})) {print "key $key\n";}
	#foreach $key (keys(%{$realele->{Fields}})) {
	#	print "Field $key, ",$realele->{Fields}{$key},"\n";}

	my $retval;
	if ("ARRAY" eq ref $realele->{RFields}{$fieldname}) {
		$retval = "[";
		foreach (@{$realele->{RFields}{$fieldname}}) {
			$retval = $retval.$_." ";
		}
		$retval = $retval."]";
	} else {
		$retval = "".$realele->{RFields}{$fieldname};
	}
	print "retval in Browser.pm is $retval\n";
	return $retval;
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


	#print "BROWSER:EAI_GetType, $nodenum, $fieldname, $direction\n";

	# is this an IS'd field?
	my $realele = VRML::Handles::get("NODE$nodenum");

	#print "BROWSER::EAI_GetType ", $realele, " - " , $realele->{Type};

	# strip off a "set_" or a "_changed" if we should.
	$fieldname = VRML::Parser::parse_exposedField($fieldname, $realele->{Type});

	#print "BROWSER::EAI_GetType evin:",$realele->{Type}{EventIns};

	#print "\nBrowser.pm, fieldname $fieldname, evin: ",
	#	$realele->{Type}{EventIns}{$fieldname}," kinds ",
	#	$realele->{Type}{FieldKinds}{$fieldname},"\n";
	
	#foreach (%{$realele->{Type}{Pars}}) {print "   .... ",@{$_}, " \n";}
	#print "Trying pars directly: ",@{$realele->{Type}{Pars}{$fieldname}} ,"\n";
	#print "\n\n\n";


	#print "BROWSER::EAI_GetType now $fieldname\n";

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
		#goto NOFOUNDNOTHING;
		FOUNDIT:
		#print "------------------";
		#print "Was Found!";
		#NOFOUNDNOTHING:
		#print "------------------\n";
	}

	#print "BROWSER:EAI_GetType, realele is ", VRML::NodeIntern::dump_name($realele)," field $fieldname\n";

	
	
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

#		print "BROWSER:EAI_GetType to_count:",$to_count,
#		" tonode_str:",$tonode_str,
#		" type:", $type,
#		" ok:", $ok,
#		" intptr:", $intptr,
#		" fieldtype:", $fieldtype,
#		"\n";
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
		
	#print "Browser.pm: EAI_GetType outptr $outptr offset $outoffset datalen $datalen retft $retft type $type\n";
	my $scalaroutptr = $outptr;
	return ($scalaroutptr, $outoffset, $datalen, $retft, $type); 
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
	$globalBrowser->{URL} = $string;

	# print "Browser, EAI_CreateVrmlFromURL, $string\n";
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

#JAS sub EAI_replaceWorld {
#JAS 	my ($string) = @_;
#JAS 
#JAS 	print "BROWSER: replace world with $string\n";
#JAS 	replaceWorld ($globalBrowser,$string);
#JAS }


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
my %EAINAMES = ();


# keep a list of DEFined names and their real names around. Because
# a name can be used more than once, (eg, DEF MX ..... USE MX .... DEF MX
# USE MX...) we need to keep track of unique identifers. 
# 
# ALSO: for EAI, we need a way of keeping def names global, as EAI requires
# us to be able to get to Nodes in one scene from another.

## specifics for EAI. We care about node pointers outside of scene methods.
sub EAI_reserve {
	my ($name, $realnode) = @_;
	$EAINAMES{$name} = $realnode;
	#print "reserving EAINAME $name ", ref $name, "is real $realnode, ref ", ref $realnode,"\n";
}
sub return_EAI_name {
	my ($name) = @_;
	if (!exists $EAINAMES{$name}) {
	
		#print "return_EAI_name, looking for $name , it is not a EAI, returning $name\n";
		#print "return_EAI_name - Name $name does not exist!\n";
		return $name;
	}
	#print "return_EAI_name, looking for $name , it IS a EAI, returning ",
	#	$EAINAMES{$name},"\n";

	return $EAINAMES{$name};
}



## keep refs to DEFs instead??? vrml name kept in DEF instances...

sub def_reserve {
	my ($name, $realnode) = @_;
	$DEFNAMES{$name} = $realnode;
	#print "reserving DEFNAME $name ", ref $name, "is real $realnode, ref ", ref $realnode,"\n";

}
sub return_def_name {
	my ($name) = @_;
	if (!exists $DEFNAMES{$name}) {
	
		#print "return_def_name, looking for $name , it is not a def, returning $name\n";
		#print "return_def_name - Name $name does not exist!\n";
		return $name;
	}
	#print "return_def_name, looking for $name , it IS a def, returning ",
	#	$DEFNAMES{$name},"\n";

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
	#print "Handle::get, looking for $handle\n";
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
