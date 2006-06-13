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
#JAS require 'VRML/X3DParser.pm';

package VRML::Browser;
use File::Basename;
use strict vars;
use POSIX;

# threaded model requires config.
use Config;

my $globalBrowser = "";

# automatic output flushing
BEGIN {
    use IO::Handle;
    STDOUT->autoflush(1);
    STDERR->autoflush(1);
}

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


# set_backend_fields may require this one, if an SFNode comes in....
sub getBE { return $globalBrowser->{BE}; }
sub getEV {return $globalBrowser->{EV};}

# read in text, unzip if required.
sub getTextFromFile {
	my ($file,$parentURL) = @_;

	# read in data from a file; the file name has been verified
	# to exist before here by "C" functions (or, it is the name
	# of a file in the Browser cache). Read it in, and return

	#print "\ngetTextFromFile, file $file, parent $parentURL\n";
	my $ri = rindex ($parentURL,"/");
	my $ps = substr($parentURL,0,$ri);

	#print "reading file \n";
	my $text = VRML::VRMLFunc::readFile ($file,$ps);
	return $text;
}


# called from fw2init.pl.
sub prepare {
	my($this) = @_;

	$this->{Scene}->make_executable();
	my $bn = $this->{Scene}->make_backend($this->{BE});
	$this->{Scene}->setup_routing($this->{EV}, $this->{BE});

	# display this one

	#print "preparing ",
	#VRML::NodeIntern::dump_name($bn), ", ",
	#VRML::NodeIntern::dump_name($bn->{CNode}), ", ",
	#"\n";

	$this->{BE}->set_root($bn); # should eventually be removed
	# print "calling set_root, with a value of ",$bn->{CNode},"\n";
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
# Background (or TextureBackground)
# NavigationInfo
# Fog

my %vpn = (); my $vpcount=0;
my %bgd = (); my $bgcount=0;
my %nav=(); my $navcount=0;
my %fog=(); my $fogcount=0;


# keep track of the number of Shape nodes seen so far. For
# Occlusion culling and VisibilitySensors.
my $occNodeCount = 0;

# zero out bindables; called from C when we have a replaceWorld
# type of action - Anchor is one...

sub zeroBindables {
	%vpn=(); %bgd = (); %nav = (); %fog = ();
	$vpcount=0; $bgcount=0; $navcount=0; $fogcount=0;
	
	VRML::Handles::deleteAllHandles();

	$occNodeCount = 0;
}

sub NewOccludeNode {
	my ($node) = @_;
	# print "NewOccludeNode, orig is ", $node->{Fields}{__OccludeNumber}, "\n";
	$node->{Fields}{__OccludeNumber} = $occNodeCount;
	$occNodeCount ++;
}

# call this to keep binding in order; CNodes can come in at any
# point in time, this function just reserves the "order".
# look at the "__BGNumber" parameter of the node.
sub reserve_bind_space {
	my ($node) = @_;

	# print "reserve_bind_space, have a type of ". $node->{TypeName},"\n";

	if ($node->{TypeName} eq "Viewpoint") {
		if ($vpcount>900) {return;}
		$node->{Fields}{__BGNumber} = $vpcount;
		$vpcount++;

	} elsif ($node->{TypeName} eq "Background") {
		if ($bgcount>900) {return;}
		$node->{Fields}{__BGNumber} = $bgcount;
		$bgcount++;
	} elsif ($node->{TypeName} eq "TextureBackground") {
		if ($bgcount>900) {return;}
		$bgcount++;
		$node->{Fields}{__BGNumber} = $bgcount;

	} elsif ($node->{TypeName} eq "NavigationInfo") {
		if ($navcount>900) {return;}
		$node->{Fields}{__BGNumber} = $navcount;
		$navcount++;

	} elsif ($node->{TypeName} eq "Fog") {
		if ($fogcount>900) {return;}
		$node->{Fields}{__BGNumber} = $fogcount;
		$fogcount++;
	} else {
		print ("Browser:reserve_bind_space, unknown type ",$node->{TypeName});
	}
}

	
# Save this node pointer so that the C backend can get it.
# save a maximum of 900; this is to avoid any stack overflow.
sub register_bind {
	my ($node) = @_;

	# print "register_bind, have a ",$node->{TypeName}, " which is reserved as ", $node->{Fields}{__BGNumber},"\n";

        if (!defined ($node->{BackNode}{CNode})) {
                print "register_vp - no backend CNode node\n";
                return;
        }

	if ($node->{TypeName} eq "Viewpoint") {
		$vpn{$node->{Fields}{__BGNumber}} = $node->{BackNode}{CNode};
	} elsif ($node->{TypeName} eq "Background") {
		$bgd{$node->{Fields}{__BGNumber}} = $node->{BackNode}{CNode};
	} elsif ($node->{TypeName} eq "TextureBackground") {
		$bgd{$node->{Fields}{__BGNumber}} = $node->{BackNode}{CNode};
	} elsif ($node->{TypeName} eq "NavigationInfo") {
		$nav{$node->{Fields}{__BGNumber}} = $node->{BackNode}{CNode};
	} elsif ($node->{TypeName} eq "Fog") {
		$fog{$node->{Fields}{__BGNumber}} = $node->{BackNode}{CNode};
	} else {
		print ("Browser:register_bind, unknown type ",$node->{TypeName});
	}
}

# the C side wants a list of specific bindables.
sub getBindables {
	my ($ty) = @_;
	# check to see what bindable we wish
	if ($ty eq "Viewpoint") { return %vpn; }
	elsif ($ty eq "Background") { return %bgd; } #Background or TextureBackground
	elsif ($ty eq "NavigationInfo") { return %nav; }
	elsif ($ty eq "Fog") { return %fog; }
	else {
		print ("Browser::getBindables, invalid request $ty\n");
		return ();
	}
}

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
                $type = 3;
        } elsif($string =~ /^#VRML V1.0/s) {
                        VRML::VRMLFunc::ConsoleMessage( "VRML V1.0, I only know V2.0");
                        return;
        } elsif ($string =~/^<\?xml version/s) {
                $type = 4;
        } else {
                #warn("WARNING: file $file doesn't start with the '#VRML V2.0' header line");
                $type = 0;
        }

	#save this file.
	#open SAVER, ">/tmp/SAVED_FROM_FREEWRL.wrl"; print SAVER $string; close SAVER;

	# is this VRML or X3D? 
        if ($type == 4)  {
		if (!eval('require VRML::X3DParser')) {
			VRML::VRMLFunc::ConsoleMessage (
				"FreeWRL can not load the X3DParser perl module".
				" - check that the perl XML-Parser module is installed ".
				"\n - this is going to fail");
		} else {
		X3D::Parser::parse($scene, $string);
		}
        } else {
		# remove comments, etc:
		$string = VRML::VRMLFunc::sanitizeInput($string);
		VRML::Parser::parse($scene, $string);
	}

	# save the type of file, for EAI/SAI calls.
	VRML::VRMLFunc::SaveFileType($type);
	
	$scene->make_executable();
	$scene->setup_routing($this->{EV}, $this->{BE});
	$ret = $scene->mkbe_and_array($this->{BE}, $scene);
	$scene->dump(0) if $VRML::verbose::scenegraph;


	return $ret;
}

sub createVrmlFromString {
	my ($this, $string) = @_;

	my $wurl = $this->{Scene}->get_world_url();

	return $this->create_common (".",$wurl,$string);
}

sub createVrmlFromURL {
	my ($this, $file, $url) = @_;

	my $bind = 0;

	# stage 1a - get the URL....
	$url = ($url || $file);
	my $wurl = $url;

	print "File: $file URL: $url\n" if $VRML::verbose::scene;
	my $t = getTextFromFile($url,$wurl);


	# Stage 2 - load the string in....
	return $this->create_common($url,$wurl,$t);
}

sub EAI_Command {
	my ($dir, $str) = @_;
	my $rv;
	#print "EAI_Command in Browser,pm, dir $dir, str $str\n";

	# strip whitespace off around $str
        $str =~ s/^\s*//;
        $str =~ s/\s*$//;

	#commands handled: 
	#	EAIheaders.h:#define	UPDNAMEDNODE    'c'
	#	EAIheaders.h:#define	REMNAMEDNODE    'd'
	#

	#UPDNAMEDNODE REMNAMEDNODE
	if (($dir == 99) || ($dir == 100)) {
		#print "have NAMEDNODE code $dir\n";
		if ($dir == 99) {
			#print "have UPDATENAMEDNODE\n";
			my ($nodeName, $nodeCaddr) = split (" ",$str);
			#print "UPN, nwname $nodeName, nodeCaddr $nodeCaddr\n";

			my $node = VRML::Handles::def_check($nodeName);
			if ($node eq "") {
				#print "UPN, this is a new name\n";
				my $newdef = VRML::Handles::def_reserve ($nodeName);
				VRML::Handles::EAI_reserve("DEF$newdef", $nodeCaddr);
			} else {
				print "updateNamedNode, node $nodeName already exists\n";
				$rv = 1;
			}	
		} else {
			#print "have removeN amedNode\n";
			my ($nodename) = split (" ",$str);
			#print "UPN, nodename $nodename, $str\n";

			# is this a node created by Perl, or by the EAI?

			my $node = VRML::Handles::def_check($nodename);
			#print "def_check returns $node\n";

			if ($node eq "") {
				print "DELETENAMEDNODE, node $nodename does not exist\n";
				$rv = 1;
			} else {
				VRML::Handles::def_delete($nodename);
			}
		}

	# 101 (ascii 'e') - getProtoDeclaration, 102 (ascii 'f') updateProtoDeclaration
	# 103 (ascii 'g') - removeProtoDeclaration
	} elsif (($dir == 101) || ($dir == 102) || ($dir == 103)) {
		my ($newname, $nodename) = split (" ",$str);
		my $nn = VRML::Handles::return_EAI_name($newname);
		if (ref $nn eq "VRML::Scene") {
			my $field; 
			my $fo;
			my $ft;
			my $fv;
	
			# getProtoDeclaration
			if ($dir == 101) {
				my $retstr = "$newname SFNode ";
	
				# count keys in hash. It's friday, I'm going home soon, so here is the code.
				my $count = 0;
				foreach $field (keys %{$nn->{Pars} }) { $count++; }
				$retstr = $retstr . "$count";
	
				foreach $field (keys %{$nn->{Pars} }) {
					$fo = $nn->{Pars}{$field}[0];
					$ft = $nn->{Pars}{$field}[1];
					$fv = $nn->{Pars}{$field}[2];
	
					my $asciival = "VRML::Field::$ft"->as_string($fv);
					#print "asciival $asciival\n";
					$retstr = $retstr . " $fo $ft $field $asciival";
				}
				#print "returnstring $retstr\n";
				return $retstr;

			# updateProtoDeclaration
			} elsif ($dir == 102) {
				my $prstr = substr ($str, length($newname)+1);
				
				my $npd = VRML::Parser::parse_interfacedecl ($nn,1,1,"[".$prstr."]");
				$nn->{Pars} = $npd;
				#print "new proto declaration for $newname, is $npd\n";
			#removeProtoDeclaration
			} else {
				VRML::Handles::delete_EAI_name($newname);
			}
				
			
		} else {
			print "getProtoDeclaration, $newname is not a proto\n";
			$rv = 1;
			return $rv;
		}
	} elsif ($dir == 105) {
		# try and find a def node for this one 
		$rv = "nodeNotFound";

		#print "get defname for $str\n";
		my $n = VRML::Handles::get ("CNODE$str");
		#print "and n is $n  with a ref of ",ref $n,"\n";

		if ("VRML::NodeIntern" ne ref $n) {
			return $rv;
		}

		my $defname = VRML::Handles::findNodeDEFFromObject($n);
		#print "defname is $defname\n";

		if (substr($defname,0,3) ne "DEF") {
			return $rv;
		} 

		my $nodeName = VRML::Handles::findNodeFromDEF ($defname);
		#printf "real node name is $nodeName\n";
		$rv = $nodeName;

		

	} else {
		print "EAI_Command - invalid command $rv\n";
		$rv = 1; 
	}
	return $rv
}

################
# EAI Perl functions.

# EAI_GetNode returns "-1 0" for undefined node, or it returns the
# number of the node so that when the node is manipulated, it can be
# referenced as NODE42.
#
# It does this because, until a specific field is requested, we can't
# use the generic subroutines that are put in place for routing in Events.pm

my @EAIinfo;

sub EAI_GetNode {
	my ($nodetoget) = @_;
	my $id;
	my $CNode;

	#print "\n\nEAI_GetNode, getting $nodetoget\n";
	# now we change the node into a DEF name.

	my $node = VRML::Handles::def_check($nodetoget);
	if ($node eq "") {
		#print "EAI_GetNode: Node $nodetoget is not defined\n";
		return "-1 0";
	}

	my $node = VRML::Handles::return_def_name($nodetoget);


	#print "step 1, node is $node\n";

	# then change the DEF name into a VRML node pointer.
	$node = VRML::Handles::return_EAI_name($node);

	#print "step 2, node is $node, ref ",ref $node,"\n";

	# is this a node created by perl, or from EAI?
	if ("VRML::NodeIntern" eq ref $node) {
		$CNode = VRML::Handles::findCNodeFromObject($node);
		#print "step 3, my CNode is $CNode\n";


		$id = VRML::Handles::reserve($node);
	
		#print "step 4: handle is $id\n";
		$id =~ s/^NODE//;
		#print "step 5: node number is $id\n";

	} else {
		#print "this node is created by EAI\n";
		$CNode = VRML::Handles::return_EAI_name($node);
		#print "CNODE is $CNode\n";
		$id = 0;
	}
	#print "returning $id $CNode\n";
	return "$id $CNode";
}

sub EAI_GetViewpoint {
	my ($nodetoget) = @_;

	#print "\n\nEAI_GetViewpoint, getting $nodetoget\n";
	# now we change the node into a DEF name.
	my $node = VRML::Handles::return_def_name($nodetoget);

	if ($node eq $nodetoget) {
		print "Viewpoint $nodetoget is not defined\n";
		return 0;
	}

	#print "step 1, node is $node, ref ",ref $node,"\n";

	# then change the DEF name into a VRML node pointer.
	$node = VRML::Handles::return_EAI_name($node);

	#print "step 2, node is $node, ref ",ref $node,"\n";

	if (ref $node eq "") {
		print "Viewpoint $nodetoget is not defined\n";
		return 0;
	}

	if ($node->{TypeName} ne "Viewpoint") {
		print "Expected $nodetoget to be Viewpoint, is ",
			$node->{TypeName},"\n";
		return 0;
	}

	#ok, we have a Viewpoint node...
	my $bn = 0;
	if (exists $node->{BackNode}{CNode}) {
		$bn = $node->{BackNode}{CNode};
	} else {
		$bn = 0;
	}

	#print "node number is $bn\n";
	return $bn;
}

# get the value for a node; NOT used for EAI, only for .class (and other scripts)
# node is called EAI_... because it "fits" in with the other nodes.

sub EAI_GetValue {
	my ($nodenum, $fieldname) = @_;
	my $fval;
	my $typename;

	#chop $fieldname;
	#print "Browser.pm - EAI_GetValue on node $nodenum, field $fieldname\n";
	my $realele = VRML::Handles::get("NODE$nodenum");

	# strip off a "set_" or a "_changed" if we should.
	$fieldname = VRML::Parser::parse_exposedField($fieldname, $realele->{Type});

	# is this maybe a parameter to a PROTO??
	#print "GetValue - this is a ";
	if (!(exists $realele->{Type}{FieldTypes}{$fieldname})) {
		#print "parameter to a proto\n";
		my $pv = $realele->{Scene}{Pars}{$fieldname};
		my @xc = @{$pv};

		$typename = $xc[1];
		$fval = $realele->{Scene}{NodeParent}{Fields}{$fieldname};
	} else {
		#print "direct type\n";
		#print "value is ",$realele->{Fields}{$fieldname},"\n";
		if ("VRML::USE" eq ref $realele->{Fields}{$fieldname}) {
			#print "THIS IS A USE!!\n";
			$fval = $realele->{Fields}{$fieldname}->real_node();
		} else {
			$fval = $realele->{RFields}{$fieldname};
		}
		$typename = $realele->{Type}{FieldTypes}{$fieldname};
		#print "direct, fval $fval, typename $typename\n";
	}

	#print "after step 1, typename $typename fval $fval",
	#		@{$fval},"\n";

	# determine whether it is a MF or not. MF's have to return a count as first line.
	my $mf = substr ($typename, 0, 2);
	my $nod = substr ($typename, 2, 10);
	my $retval;

	if ("MF" eq $mf) {
		my $count = @{$fval};
		$retval = "".$count."\n"; # add the number of elements here....
		#print "it is an MF, retval is $retval\n";
	}

	#print "here in GetValue, fval is $fval\n";
	if ("ARRAY" eq ref $fval) {
		# this can be an array - eg, an SFvec3f is an array.
		my $val;
		my $id;
		my $bn;
		foreach $val (@{$fval}) {
			if ("VRML::USE" eq ref $val) {$val = $val->real_node();}
			if ("VRML::NodeIntern" eq ref $val) {
				#print "GetValue; have to convert $val into a NODE\n";
				#my $key; foreach $key (keys(%{$val})) {print "val key $key\n";}
				$id = VRML::Handles::reserve($val);
				if (exists $val->{BackNode}{CNode}) {
					$bn = $val->{BackNode}{CNode};
				} else {
					$bn = 0;
				}

				#print "handle is $id\n";
				$id =~ s/^NODE//;
				$retval = $retval.$id.":".$bn."\n";
			} else {
				$retval = $retval.$val."\n";
			}
		}
		#JAS - remove the last cr
		chop $retval;
	} else {
		if ("VRML::USE" eq ref $fval) {$fval = $fval->real_node();}
		if ("VRML::NodeIntern" eq ref $fval) {
			my $bn;
			#print "have to convert $fval into a NODE\n";
			my $id = VRML::Handles::reserve($fval);
			if (exists $fval->{BackNode}{CNode}) {
				$bn = $fval->{BackNode}{CNode};
			} else {
				$bn = 0;
			}

			#print "handle is $id\n";
			$id =~ s/^NODE//;
			$retval = "".$id.":".$bn;
		} else {
			$retval = "".$fval;
		}
	}
	#print "retval in Browser.pm is $retval\n";
	return $retval;
}

# get the name for a node; NOT used for EAI, only for .class (and other scripts)
# node is called EAI_... because it "fits" in with the other nodes.

sub EAI_GetTypeName {
	my ($nodenum) = @_;
	my $retval;

	#print "Browser.pm - EAI_GetTypeName on node $nodenum\n";
	my $realele = VRML::Handles::get("NODE$nodenum");
	$retval = "".$realele->{TypeName};
	#print "retval in Browser.pm is $retval\n";
	return $retval;
}



# get the type, return values used for direct manipulation in C, such as memory location, datasize, etc.

sub EAI_LocateNode {
	my ($nodenum, $fieldname, $direction) = @_;
	my $realele;


	#print "BROWSER:EAI_LocateNode params, $nodenum, $fieldname, $direction\n";

	# is "nodenum" an object passed in from Events.pm, or an integer
	# number passed in from the EAI code?
	if ("VRML::NodeIntern" ne ref $nodenum) {
		$realele = VRML::Handles::get("NODE$nodenum");
	} else {
		$realele = $nodenum;
	}

	#print "BROWSER:EAI_LocateNode, now $realele\n";

	#print "EAI_LocateNode fields of ".$realele->{TypeName}."are:\n";
	#foreach (keys %{$realele->{Fields}}) { print "     fields $_\n"; }


	# strip off a "set_" or a "_changed" if we should.
	$fieldname = VRML::Parser::parse_exposedField($fieldname, $realele->{Type});

	#print "BROWSER::EAI_LocateNode evin:",$realele->{Type}{EventIns};

	#print "BROWSER::EAI_LocateNode fieldname $fieldname, evin: ",
	#$realele->{Type}{EventIns}{$fieldname}," kinds ",
		#$realele->{Type}{FieldKinds}{$fieldname},"\n";

		#foreach (%{$realele->{Type}{Pars}}) {print "   .... ",@{$_}, " \n";}
	#print "Trying pars directly: ",@{$realele->{Type}{Pars}{$fieldname}} ,"\n";
	#print "\n\n\n";
	#print "BROWSER::EAI_LocateNode now $fieldname\n";

	if ((exists $realele->{Fields}{$fieldname}) && ($realele->{Fields}{$fieldname} ne "")) {
		#print "BROWSER:EAI - field $fieldname exists in node, it is ",
		#	$realele->{Fields}{$fieldname},"\n";
		return ($realele, $fieldname, $direction);
	}

	#print "BROWSER:EAI - field $fieldname DOES NOT exist in node\n";

	# try and see if this is a PROTO expansion.
	if (exists $realele->{Fields}{children}) {
		my $testnode = $realele->{Fields}{children}[0];
		if (ref $testnode eq "VRML::DEF") {$testnode = $testnode->node();}

		#print "my testnode is ",
		#VRML::NodeIntern::dump_name($testnode),"\n";

		if (exists $testnode->{Fields}{$fieldname}) {
			#print "field exists! making testnode realnode\n";
			$realele = $testnode;
		}
	}

	my $ms = $realele->{Scene};
	my ($xele, $sc, $in, $rn, $rf);

	# try to find this node/field within this scene.
	foreach $xele (@EAIinfo) {
		($sc, $rf, $rn, $in) = @{$xele};
		#print "in $in rf $rf fieldname $fieldname scene ",
		#	VRML::NodeIntern::dump_name($ms), " ",
		#	VRML::NodeIntern::dump_name($sc),"\n";
		if ($ms eq $sc) {  # same scene
			if ($fieldname eq $rf) {
				$realele = $rn;
				$fieldname = $in;
				#print "realele now is $realele, field $fieldname\n";
				goto FOUNDIT;
			}
		}
	}
	#goto NOFOUNDNOTHING;
	FOUNDIT:
#	print "------------------";
#	print "Was Found!";
	#NOFOUNDNOTHING:
	#print "------------------\n";

	#print "BROWSER:EAI_LocateNode, realele is ", VRML::NodeIntern::dump_name($realele)," field $fieldname\n";
	return ($realele, $fieldname, $direction);
}


sub EAI_GetType {
	my ($nn, $fn, $direct) = @_;
	my $outptr;
	my $outoffset;
	my $to_count;
	my $tonode_str;
	my $intptr;
	my $type;
	my $ok;
	my $datalen;
	my $retft;
	my $outoffset;
	my $fieldtype;
	my $fieldname;
	my $direction;
	my $realele;


	# pass in the nodenum, fieldname, and direction, and get a real
	# nodepointer, fieldname and direction back again.
	($realele, $fieldname, $direction) =
		EAI_LocateNode($nn, $fn, $direct);

		#print "EAI_GetType, locateNode returns $realele, $fieldname, $direction\n";

	# get info from FreeWRL internals.
	if ($direction =~/eventIn/i) {
        	($to_count, $tonode_str, $type, $ok, $intptr, $fieldtype) =
				$globalBrowser->{EV}->resolve_node_cnode($globalBrowser->{Scene},
					$realele, $fieldname, $direction);

		$datalen = 0; # we either know the length (eg, SFInt32), or if MF, it is the eventOut that
#		print "Browser.pm, tonodestr: $tonode_str\n";
			      # determines the exact length.
		($outptr, $outoffset) = split(/:/,$tonode_str,2);
	} else {
		($outptr, $outoffset, $type, $ok, $datalen, $fieldtype) = $globalBrowser->{EV}->resolve_node_cnode (
        		$globalBrowser->{Scene}, $realele, $fieldname, $direction);

	}

	#print "Browser, type $type, fieldtype $fieldtype, offset $outoffset\n";

	# return node pointer, offset, data length, type
	# - see the EAI C code (EAIServ.c) for definitions.
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

	# check for errors; both have to be zero, as the first script is "0"...
	# (JSparamnames will ensure that the first outoffset is 1)
	if (($scalaroutptr == 0) &&($outoffset == 0)) {
		print "FreeWRL:EAI: could not find field '$fn' of ";
		print "internal node 'NODE$nn'\n";
	}
	return ($scalaroutptr, $outoffset, $datalen, $retft, $type);
}

#EAI_CreateX3DNodeFromProto - make a node of this type, and return the node pointers.
sub EAI_CreateX3DNodeFromPROTO {
	my ($string) = @_;
	my $rv;

	# print "browser:EAI_CreateX3DNodeFromPROTO - node type $string\n";
	$rv = VRML::Handles::return_EAI_name($string);
	if ($rv eq "") {
		print "EAI_CreateX3DNodeFromPROTO, proto $string not found\n";
		return;
	}

	return EAI_CreateVrmlFromString ($string."{}");
}


#EAI_CreateX3DNodeFromString - make a node of this type, and return the node pointers.
sub EAI_CreateX3DNodeFromString {
	my ($string) = @_;
	my $rv;

	#print "browser:EAI_CreateX3DNodeFromString - node type $string\n";

	# format this, then just get EAI_CreateVrmlFromString to parse it
	
	# is this a defined node type?
	my $no = $VRML::Nodes{$string};
	if ($no ne "") {
		#print "EAI_CV, have a valid node type\n";
		return EAI_CreateVrmlFromString ($string . "{}");
	}

	# maybe it is a def name? 
	my $no = VRML::Handles::return_def_name($string);
	if ($no ne "") {
		my $realele = VRML::Handles::return_EAI_name(VRML::Handles::get($no));
		##print "EAI_CV, realele $realele type is ",$realele->{TypeName},"\n";	
		return EAI_CreateVrmlFromString ($realele->{TypeName}."{}");
	}
}

# EAI_CreateVrmlFromString - parse commands, and return a string of (node-number backnode) pairs.
sub EAI_CreateVrmlFromString {
	my ($string) = @_;

	#print "browser:EAI_CreateVrmlFromString, string $string\n";

	my $rv = createVrmlFromString ($globalBrowser,$string);

	my @rvarr = split (" ", $rv);
	my %retval = ();
	my $ele;
	my $realele;
	my $bn;

	foreach $ele (@rvarr) {
		$realele = VRML::Handles::get($ele);
		#print "Browser:CVS, ele $ele, real $realele has name ", $realele->{TypeName},"\n";
		#my $key;
		#foreach $key (keys(%{$realele})) {print "realele key $key\n";}

		# get the back nodes; but if this is a proto defn, skip it.
		if (exists $realele->{BackNode}{CNode}) {
			$bn = $realele->{BackNode}{CNode};
			$ele =~ s/^NODE//;
			$retval{$ele} = $bn;
			#print "EAI, have ele $ele, bn $bn\n";

			# reserve the CNODE as a node, because sometimes we do need to go
			# from CNode to node.
			VRML::Handles::CNodeLinkreserve("CNODE$bn",$realele);
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
			VRML::Handles::CNodeLinkreserve("CNODE$bn",$realele);
		} else {
			# print "warning, EAI_CreateVrmlFromURL - no backnode found for $ele\n";
		}
	}
	return %retval;
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


# Javascripting routing interface
sub JSRoute {
	my ($js, $dir, $route) = @_;

	my ($fn, $ff, $tn, $tf) = split (" ",$route);

	# print "JSRouting, from $fn, fromField $ff, to $tn, toField $tf\n";

	# check to see if any of these are simply CNode pointers; if so,
	# try to make them into a NODE(cnode), and re-do the checks
	my $nfn = VRML::Handles::check($fn);
	my $ntn = VRML::Handles::check($tn);
	# print "jspBrowser - check for them is $nfn, $ntn\n";

	if ($nfn == 0) {
		# print "from node is not checked ok\n";
		$fn = "NODE".$fn;
		$nfn = VRML::Handles::check($fn);
	}
	if ($ntn == 0) {
		# print "to node is not checked ok\n";
		$tn = "NODE".$tn;
		$ntn = VRML::Handles::check($tn);
	}

	if (($nfn==0) || ($ntn==0)) {
		print "jspBrowserAddRoute, can not find either of $nfn, $ntn\n";
		return;
	}

	$globalBrowser->{EV}->add_route($globalBrowser->{Scene},
				1,$fn,$ff,$tn,$tf);
}
#########################################################3
#
# Private stuff

# No other nice place to put this so it's here...
# For explanation, see the file ARCHITECTURE
package VRML::Handles;

my $handles_debug = 0;

{
my %S = ();
my %DEFNAMES = ();
my %EAINAMES = ();
my $LASTDEF  =1;

# on a replaceWorld call...
sub deleteAllHandles {
	%S=();
	%DEFNAMES = ();
	%EAINAMES = ();
	$LASTDEF  =1;
}

# keep a list of DEFined names and their real names around. Because
# a name can be used more than once, (eg, DEF MX ..... USE MX .... DEF MX
# USE MX...) we need to keep track of unique identifers.
#
# ALSO: for EAI, we need a way of keeping def names global, as EAI requires
# us to be able to get to Nodes in one scene from another.

## specifics for EAI. We care about node pointers outside of scene methods.

sub delete_EAI_name {
	my ($name) = @_;
	delete $EAINAMES{$name};
}

sub findNodeDEFFromObject {
	my ($object) = @_;

	foreach (keys %{%EAINAMES}) {
		#print "findCNodeDEFFromObject, have ".$EAINAMES{$_}.", comparing to $object.. \n";
		if ($EAINAMES{$_} eq $object) {
			return $_;
		}
	}
	return 0;
}

sub EAI_reserve {
	my ($name, $realnode) = @_;
	$EAINAMES{$name} = $realnode;
	print "reserving EAINAME $name ",
		ref $name, "is real $realnode\n\t",
		VRML::NodeIntern::dump_name($realnode),"\n\t",
		"ref ", ref $realnode,"\n"  if $handles_debug;
}
sub return_EAI_name {
	my ($name) = @_;
	if (!exists $EAINAMES{$name}) {

		print "return_EAI_name, looking for $name , it is not a EAI, returning $name\n"  if $handles_debug;
		#print "return_EAI_name - Name $name does not exist!\n";
		return $name;
	}
	print "return_EAI_name, looking for $name , it IS a EAI, returning ",
		$EAINAMES{$name},"\n" if $handles_debug;

	return $EAINAMES{$name};
}



## keep refs to DEFs instead??? vrml name kept in DEF instances...
sub def_reserve {
	my ($name) = @_;
	$DEFNAMES{$name} = "DEF$LASTDEF";
	print "reserving DEFNAME $name  as DEF$LASTDEF\n"  if $handles_debug;
	$LASTDEF ++;

}

sub findNodeFromDEF {
	my ($object) = @_;

	foreach (keys %{%DEFNAMES}) {
		#print "findCNodeDE have ".$EAINAMES{$_}.", comparing to $object.. \n";
		if ($DEFNAMES{$_} eq $object) {
			return $_;
		}
	}
	return 0;
}
sub return_def_name {
	my ($name) = @_;
	if (!exists $DEFNAMES{$name}) {

		print "return_def_name, looking for $name , it is not a def, returning $name\n"  if $handles_debug;
		print "return_def_name - Name $name does not exist!\n"  if $handles_debug;
		return $name;
	}
	print "return_def_name, looking for $name , it IS a def, returning ",
		$DEFNAMES{$name},"\n"  if $handles_debug;

	return $DEFNAMES{$name};
}


# is this DEF name already defined?
sub def_check {
	my ($name) = @_;
	return $DEFNAMES{$name};
}

sub def_delete {
	my ($name) = @_;
	delete $DEFNAMES{$name};

}

######

sub findCNodeFromObject {
	my ($object) = @_;
	my $ci = "$object";

	foreach (keys %{%S}) {
		#print "findCNodeFromObject, key $_ have ".$S{$_}[0].", comparing to $object.. \n";
		if ($S{$_}[0] eq $ci) {
			#print "checking... ". substr ($_,0,5)."\n";
			if (substr($_,0,5) eq "CNODE") {
				#print "FOUND IT! it is $_ returning ".  substr ($_,5)."\n";
				return substr ($_,5);
			}
			
		}
	}
	return 0;
}
sub CNodeLinkreserve {
	my($str,$object) = @_;
	print "Handle::CNodeLinkreserve, reserving $str for object $object type ", ref($object), "\n" if $handles_debug;

	if(!defined $S{$str}) {
		$S{$str} = [$object, 0];
	}
	$S{$str}[1]++;
	return $str;
}
sub reserve {
	my($object) = @_;
	my $str = VRML::NodeIntern::dump_name($object);
	print "Handle::reserve, reserving $str for object $object type ", ref($object), "\n"  if $handles_debug;

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
	print "Handle::get, looking for $handle\n"  if $handles_debug;
	return NULL if $handle eq "NULL";

	if(!exists $S{$handle}) {
		print "handle $handle does not exist\n"  if $handles_debug;
		return $handle;
	}
	print "returning ", $S{$handle}[0],"\n"  if $handles_debug;
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
