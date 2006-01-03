# Copyright (C) 1998 Tuomas J. Lukka 1999 John Stewart CRC Canada
# DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
# See the GNU Library General Public License (file COPYING in the distribution)
# for conditions of use and redistribution.
#
# $Id$
#
# Implement a scene model, with the specified parser interface.
# At some point, this file should be redone so that it uses softrefs
# for circular data structures.

use strict vars;

require 'VRML/DEF.pm';
require 'VRML/FieldHash.pm';
require 'VRML/IS.pm';
require 'VRML/NodeIntern.pm';
require 'VRML/Parser.pm';
require 'VRML/USE.pm';


#################################################################################################

## temporary: for debugging only, do not add to project!!!
require 'VRML/Debug.pm';

#################################################################################################


#######################################################################
#
# VRML::Scene
#  this package represents a scene or a prototype definition/copy of it.
#

package VRML::Scene;
#
# Pars - parameters for proto, hashref
# Nodes - arrayref of toplevel nodes
# Protos - hashref of prototypes
# Routes - arrayref of routes [from,fromf,to,tof]
#
# Expansions:
#  - expand_protos = creates actual copied nodes for all prototypes
#    the copied nodes are stored in the field ProtoExp of the Node
#
#  - expand_usedef


sub dump {
	my ($this, $level) = @_;

	if ($level > 20) {
		print "level too deep, returning\n";
		return();
	}
	my $lp = $level*2+2;
	my $padded = pack("A$lp","$level ");


	# print out the Scene graph, to help in debugging.
	print $padded,"SCENE DUMP ",
		VRML::NodeIntern::dump_name($this),"\t", $this->{URL},"\n\n";
	foreach (keys %{$this}) {
		print $padded,"$_\t";

		# lets do something special for Routes
		if ("Routes" eq $_) {
       		 for (values %{$this->{$_}}) {
                	my ($fnam, $ff, $tnam, $tf) = @{$_};
                	print "$padded    Route from $fnam field $ff to $tnam field $tf\n";
        	 }

		} elsif ("DEF" eq $_) {
		  my $nk;
		  foreach $nk (keys %{$this->{$_}}) {
			print "(key $nk ";
			my $sk;
			foreach $sk (keys %{$this->{$_}{$nk}}) {
				print "sk $sk : ",$this->{$_}{$nk}{$sk},"; ";
			}
			print ") ";
		  }
		} elsif ("ARRAY" eq ref $this->{$_}) {
			print "(array) ";
			foreach (@{$this->{$_}}) {
				print VRML::NodeIntern::dump_name($_->real_node());
				print " ";
			}
		} elsif ("HASH" eq ref $this->{$_}) {
			print "(hash) ";
			foreach (keys %{ $this->{$_}}) {
			  print "$_ ";
			}
		} else {
		 	print VRML::NodeIntern::dump_name($this->{$_});
		}
		print "\n";
	}

	print "\n$padded(Nodes of ",VRML::NodeIntern::dump_name($this),")\n";
	for (@{$this->{Nodes}}) {
		$_->dump($level+1);
	}

	if (defined $this->{SubScenes}) {
		print "\n$padded(SubScenes of ",VRML::NodeIntern::dump_name($this),")\n";
		my $sk;
		foreach $sk (@{{$this}->{SubScenes}}) {
			print "SUBSCENE of ",VRML::NodeIntern::dump_name($this);
			$sk->dump($level+1);
		}
	}

	print "$level END OF SCENE DUMP for ",VRML::NodeIntern::dump_name($this),"\n\n";
}




sub new {
	my ($type, $eventmodel, $url, $worldurl) = @_;
	my $this = bless {
					  EventModel => $eventmodel,
					  URL => $url,
					  WorldURL => $worldurl,
					  SubScenes => undef,
					  Bindable => undef,
					  Bindables => undef,
					  Routes => undef,
					  DelRoutes => undef,
					  Nodes => undef,
					  NodeParent => undef,
					  Parent => undef,
					  Protos => undef,
					  DEF => undef
					 }, $type;
	print "VRML::Scene::new: ", VRML::Debug::toString(\@_), "\n"
		if $VRML::verbose::scene;
	return $this;

}

sub set_url {
    my ($this, $url, $parentURL) = @_;
    my $ri = rindex ($parentURL,"/");
    my $ps = substr($parentURL,0,$ri);

    $this->{URL} = $url;
    $this->{WorldURL} = $ps;
}

sub newp {
    my ($type, $pars, $parent, $name) = @_;
    # newp - ...
    # actually creates a new prototype.
    #
    # parameters:

    # type: probably always VRML::Scene
    # pars: not yet decoded. prints as HASH(0x83e45f0)
    # look below for its usage in extracting fields.
    # parent:which invocationo of scene this is in.  SCENE_2
    # name: VRML name, eg, dirigible

    #print "newp, type $type, name $name\n";
    my $this = $type->new();

    $this->{Pars} = $pars;
    $this->{Name} = $name;
    $this->{Parent} = $parent;

    # Extract the field types
    $this->{FieldTypes} = {map {$_ => $this->{Pars}{$_}[1]} keys %{$this->{Pars}}};
    $this->{FieldKinds} = {map {$_ => $this->{Pars}{$_}[0]} keys %{$this->{Pars}}};

    $this->{EventModel} = $parent->{EventModel};
    $this->{Defaults} = {map {$_ => $this->{Pars}{$_}[2]} keys %{$this->{Pars}}};

	my $k;

    for (keys %{$this->{FieldKinds}}) {
		$k = $this->{FieldKinds}{$_};
		if ($k eq "exposedField") {
			$this->{EventIns}{$_} = $_;
			$this->{EventOuts}{$_} = $_;
		} elsif ($k eq "eventIn") {
			$this->{EventIns}{$_} = $_;
		} elsif ($k eq "eventOut") {
			$this->{EventOuts}{$_} = $_;
		} elsif ($k ne "field") {
			die("$k is an invalid kind of field or event from in $name");
		}
    }

    return $this;
}


sub newextp {
    my ($type, $pars, $parent, $name, $url, $parentURL) = @_;
    # XXX marijn: code copied from newp()

    #print "newextp, calling new for $name,",@{$url},$this->{URL},$this->{WorldURL},"\n ";
    my $this = $type->new();
    $this->{Pars} = $pars;
    $this->{Name} = $name;
    $this->{Parent} = $parent;
    $this->{URL} = $url;
    $this->{WorldURL} = $parentURL;

    #print "newextp, url ",@{$url},", parenturl $parentURL\n";

    # is this an EXTERNPROTO invoked from another EXTERNPROTO? If so,
    # we may not have the correct parent url passed in.
    # this is not quite correct, but should work for 99% of the time.
    # we should go to the url for the parent EXTERNPROTO, rather than
    # the parent for the whole scene, but, this is not stored in Perl
    # anywhere.

    if (!$parentURL) {
	    $parentURL = VRML::VRMLFunc::GetBrowserFullPath();
	    $this->{WorldURL} = $parentURL;
	  }


    # Extract the field types
    $this->{FieldTypes} = {map {$_ => $this->{Pars}{$_}[1]} keys %{$this->{Pars}}};
    $this->{FieldKinds} = {map {$_ => $this->{Pars}{$_}[0]} keys %{$this->{Pars}}};
    $this->{EventModel} = $parent->{EventModel};

	my $k;

    for (keys %{$this->{FieldKinds}}) {
		$k = $this->{FieldKinds}{$_};
		if ($k eq "exposedField") {
			$this->{EventIns}{$_} = $_;
			$this->{EventOuts}{$_} = $_;
		} elsif ($k eq "eventIn") {
			$this->{EventIns}{$_} = $_;
		} elsif ($k eq "eventOut") {
			$this->{EventOuts}{$_} = $_;
		} elsif ($k ne "field") {
			die("$k is an invalid kind of field or event from in $name");
		}
    }

    print "EXTERNPROTO with URL: ", VRML::Debug::toString($url), "\n"
		if $VRML::verbose::parse;
	my ($string, $protourl, $protoname, $brow, $po);
	my $success = 0;

	for (@{$url}) {
		($protourl, $protoname) = split(/#/, $_, 2);
		$string = VRML::Browser::getTextFromFile($protourl,$parentURL);
		next if (!$string);


		my $newurl = VRML::VRMLFunc::GetLastReadFile();
		#print "Scene.pm - this file is $newurl\n";
		# marijn: set the url for this proto
		$this->set_url($newurl,$parentURL);

		unless ($string =~ /^#VRML V2.0/s) {
			die("Sorry, this file is according to VRML V1.0, I only know V2.0")
				if ($string =~ /^#VRML V1.0/);
			#warn("File $protourl doesn't start with the '#VRML V2.0' header line");
		}

		# remove comments, etc...
		$string = VRML::VRMLFunc::sanitizeInput($string);

		# XXX marijn: code copied from Browser->parse()
		$po = pos $string;
		while ($string =~ /([\#\"])/gsc) {
			(pos $string)--;
			if ($1 eq "#") {
				$string =~ s/#.*$//m;
			} else {
				VRML::Field::SFString->parse($this, $string);
			}
		}
		(pos $string) = $po;

		# strip off any whitespace at the beginning of the string.
                $string =~ s/^\s+//g;

		# go through all PROTOS in this EXTERNPROTO; stop when
		# we have our PROTO. (parse and use all protos up to this
		# point
		while ($string =~ /(PROTO\s+)($VRML::Error::Word)/gsc ) {
			if (!$protoname) {
				$protoname = $2;
			}

			#print "requested proto is $protoname, found $2; d1 is $1\n";
			if ($2 eq $protoname) {
				# found our proto
				$success = 1;
			}
	
			# back up; put the "PROTO name" back on the front
			(pos $string) -= ((length $1) + (length $2));

			# parse this PROTO
			VRML::Parser::parse_statement($this, $string,0,"protoTop");

			# strip off any whitespace at the beginning of the string.
                	$string =~ s/^\s+//g;

		}
		last if ($success);
	}

	VRML::Error::parsefail($string,"looking for PROTO $protoname, not found in EXTERNPROTO file")
		if (!$success);

    # marijn: now create an instance of the PROTO, with all fields IS'd.
    my %fields = map { $_ => $this->new_is($_) } keys %{$this->{Pars}};
    my $n = $this->new_node($protoname, \%fields);
    my @node = ($n);


	$this->topnodes(\@node);

    # marijn: copy defaults from PROTO
	$this->{Defaults} = {
						 map {$_ => $this->{Protos}{$protoname}->{Defaults}{$_}}
						 keys %{$this->{Protos}{$protoname}->{Defaults}}
						};
	return $this;
}

##################################################################
#
# This is the public API for use of the parser or whoever..

{

my $cnt;
sub new_node {
	my ($this, $type, $fields) = @_;

	# VRML scripting.
	if ($type eq "Script") {
		#print "Scene.pm - new script node, cnt $cnt\n ";
		# Special handling for Script which has an interface.
		my $t = "__script__".$cnt++;
		 #print " name is $t ";
		my %f = (
				 url => [MFString, [], exposedField],
				 directOutput => [SFBool, 0, field],
				 mustEvaluate => [SFBool, 0, field]
				);
		for (keys %$fields) {
			#print "Scene.pm - working on field $_\n";
			#print "Scene.pm - the fields are ",$fields->{$_}[0],", ",
			#		$fields->{$_}[1],", ",$fields->{$_}[2],"\n";
			$f{$_} = [
				$fields->{$_}[1], #eg, SFFloat
				$fields->{$_}[2], #eg, value
				$fields->{$_}[0], #eg, eventOut
			];
		}
		my $type = VRML::NodeType->new($t, \%f);
		my $node = VRML::NodeIntern->new($this, $type, {}, $this->{EventModel});
		VRML::Handles::reserve($node);
		#print "handle is $node, ",VRML::NodeIntern::dump_name($node),"\n";
		return $node;
	}

	# X3D scripting.
	if ($type eq "X3DScript") {
		#print "Scene.pm - new X3Dscript node, cnt $cnt\n ";
		# Special handling for X3DScript which has an interface.
		my $internalScriptName = "__script__".$cnt++;
		# print " name is $internalScriptName \n";

		# create fields for this script. They should be like (from NodeType->new()):
		#VRML::NodeType  - new of type VRML::NodeType, name Script
		#        field url, ref fields[1] - ARRAY
		#                0:MFString, 1:ARRAY(0x194eaac), 2:exposedField
		#        field url has t of exposedField, 0 MFString, 1 ARRAY(0x194eaac), 2 exposedField
		#        field mustEvaluate, ref fields[1] -
		#                0:SFBool, 1:0, 2:field
		#        field mustEvaluate has t of field, 0 SFBool, 1 0, 2 field
		#        field directOutput, ref fields[1] -
		#                0:SFBool, 1:0, 2:field
		#        field directOutput has t of field, 0 SFBool, 1 0, 2 field

		my %f = ();

		# work on the URL.
		my @arr;
		push @arr, "[\"".$fields->{ScriptBody}."\"]";
		$f{url} = ["MFString", "VRML::Field::MFString"->parse($this,@arr), "exposedField"];

		# work on everything else BUT the url .-|
		for my $fk (keys %{$fields->{ScriptInterface}}) {
			# print "X3DScript in Scene, key $fk\n";
			if (($fk eq "directOutput") || ($fk eq "mustEvaluate")) {
				#print "found $fk\n";
				my $iv = 0;
				if ($fields->{ScriptInterface}{$fk} eq "true") {$iv = 1;}
				$f{$fk} = ["SFBool",$iv,"field"];
			} elsif ($fk eq "containerField") {
				# print "X3DScript - got containerField...\n";
				
			} elsif ($fk eq "field") {
				#print "found a field\n";
				for my $sfk (keys %{$fields->{ScriptInterface}{$fk}}) {
					#print "subfield $sfk\n";
					my $sfkptr = $fields->{ScriptInterface}{$fk}{$sfk};
					my $akt = $fields->{ScriptInterface}{$fk}{$sfk}{accessType};
					my $val = $fields->{ScriptInterface}{$fk}{$sfk}{value};
					my $type = $fields->{ScriptInterface}{$fk}{$sfk}{type};

					if ($akt eq "initializeOnly") {$akt = "field";}
					elsif ($akt eq "inputOnly") {$akt = "eventIn";}
					elsif ($akt eq "outputOnly") {$akt = "eventOut";}
					elsif ($akt eq "inputOutput") {
						$akt = "eventOut";
						print "Warning, field $sfk assumed to be eventOut, not inputOutput\n";
					} else {
						print "Huh? field $sfk has a field of $akt - what is this?\n";
						$akt = "eventOut";
					}
					#print "field $sfk type $type val $val atk $akt\n";
					$f{$sfk} = [$type,$val,$akt];
				}
			} elsif ($fk ne "url") {
				print "Scene:new_node, unknown field $fk for X3DScript\n";
			}
		}


		# create new type for this script node.
		my $type = VRML::NodeType->new($internalScriptName, \%f);
		my $node = VRML::NodeIntern->new($this, $type, {}, $this->{EventModel});
		VRML::Handles::reserve($node);
		#print "handle is $node, ",VRML::NodeIntern::dump_name($node),"\n";
		return $node;
	}

	my $node = VRML::NodeIntern->new($this, $type, $fields, $this->{EventModel});

	# Check if it is bindable and first -> bind to it later..
	if ($VRML::Nodes::bindable{$type}) {
		if (!defined $this->{Bindable}{$type}) {
			$this->{Bindable}{$type} = $node;
		}

		# GeoViewpoint and Viewpoint are handled the same, and on the same stack
		if ($type eq "GeoViewpoint") {$type = "Viewpoint"};
		push @{$this->{Bindables}{$type}}, $node;
	}
	VRML::Handles::reserve($node);
	return $node;
}

}

sub new_route {
	my ($this, $fn, $eo, $tn, $ei) = @_;
	my $route = "$fn"."$eo"."$tn"."$ei";
	my $isProcessed = 0;

	print "VRML::Scene::new_route: ", VRML::Debug::toString(\@_), "\n"
		if $VRML::verbose::scene;

	if (!exists $this->{Routes}{$route} or !$this->{Routes}{$route}->[4]) {
		$this->{Routes}{$route} = [$fn, $eo, $tn, $ei, $isProcessed];
	}
}

sub new_def {
	my ($this, $name, $node, $vrmlname) = @_;

	print "VRML::Scene::new_def ", VRML::Debug::toString(\@_), "\n"
		if $VRML::verbose::scene;

	my $def = VRML::DEF->new($name, $node, $vrmlname);
	$this->{DEF}{$name} = $def;

	return $def;
}

sub new_use {
	my ($this, $name) = @_;

	#print "Scene, new_use for name $name, DEFname ",$this->{DEF}{$name}," ref ",
	#ref $this->{DEF}{$name},"\n";
	return VRML::USE->new($name, $this->{DEF}{$name})
		if (defined $this->{DEF}{$name});

	return VRML::USE->new($name,
	  (VRML::Handles::return_def_name($name)));
}

sub new_is {
	my ($this, $name, $is) = @_;
	my $newObj = VRML::IS->new($name, $is);
	return $newObj;
}

sub new_proto {
	my ($this, $name, $pars) = @_;

	print "VRML::Scene::new_proto: ", VRML::Debug::toString(\@_), "\n"
		if $VRML::verbose::scene;

	my $p = $this->{Protos}{$name} = (ref $this)->newp($pars, $this, $name);
	print "Scene:new_proto, returning $p \n" if $VRML::verbose::scene;
	return $p;
}

sub new_externproto {
	my ($this, $name, $pars, $url) = @_;

	print "VRML::Scene::new_externproto: ", VRML::Debug::toString(\@_), "\n"
		if $VRML::verbose::scene;

	# pass in url to allow finding of files relative to url.
	my $p = $this->{Protos}{$name} = (ref $this)->newextp($pars, $this, $name,
			$url,$this->{URL});
	return $p;
}


# prototopnodes - do a topnode, but put ALL proto children within a Group{} construct
# to help with displaying geometry of only first child.

sub prototopnodes {
	my ($this, $nodes) = @_;

	#my ($package, $filename, $line) = caller;
	#print "\nprototopnodes, this ",VRML::NodeIntern::dump_name($this)," called from $package, $line\n";

	# encase the proto nodes in a Group
	 my $ntn = ($this->new_node("Group",{children => $nodes}));

	# tell the rendering engine that this is a PROTO - render only first child
	$ntn->{Fields}{__isProto} = 1;
	push (@{$this->{Nodes}}, $ntn);
}

# topnodes for non-PROTO Scenes.
sub topnodes {
	my ($this, $nodes) = @_;
	$this->{Nodes} = $nodes;

}

sub get_proto {
	my ($this, $name) = @_;
	print "VRML::Scene::get_proto: ", VRML::Debug::toString(\@_), "\n"
		if $VRML::verbose::scene;

	return $this->{Protos}{$name}
		if ($this->{Protos}{$name});

	return $this->{Parent}->get_proto($name)
		if ($this->{Parent});

	print "VRML::Scene::get_proto: $name is not defined\n" if $VRML::verbose::scene;
	return undef;
}

sub get_url {
	my ($this) = @_;
	return $this->{URL} if (defined $this->{URL});
	return $this->{Parent}->get_url() if ($this->{Parent});
	die("Undefined URL tree");
}

sub set_world_url {
	my ($this, $url) = @_;
	print "VRML::Scene::set_world_url: ", VRML::Debug::toString(\@_), "\n"
		if $VRML::verbose::scene;

	$this->{WorldURL} = $url;
}

sub get_world_url {
	my ($this) = @_;
	return $this->{WorldURL} if (defined $this->{WorldURL});
	return $this->{Parent}->get_world_url() if ($this->{Parent});
	return undef;
}

sub get_scene {
	my ($this) = @_;
	return $this->{Parent}->get_scene() if ($this->{Parent});
	return $this;
}

sub set_browser { $_[0]{Browser} = $_[1]; }

sub get_browser {
	my ($this) =@_;
	return $this->{Parent}->get_browser() if ($this->{Parent});
	return $this->{Browser};
}

########################################################
#
# Private functions again.


sub get_as_mfnode {
	return VRML::Handles::reserve($_[0]{Nodes});
}

sub mkbe_and_array {
	my ($this, $be, $parentscene) = @_;
	# lets get the number of items in there...

	# copy over any generic scene DEFs. Proto specific DEFS below
	# note that the DEFS are part of a Scene object. Routes are attached
	# to a node.
	# my ($package, $filename, $line) = caller;
	# print "\nmkbe_and_array, this ",VRML::NodeIntern::dump_name($this)," called from $package, $line\n";

	if (defined $this->{Scene}{DEF}) {
		%{$parentscene->{DEF}} = (%{$parentscene->{DEF}}, %{$this->{Scene}{DEF}});
	}

	my $c;
	my $ref;
	my $q = "";
	my $lastindex = $#{$this->{Nodes}};

	my $curindex = 0;
	while ($curindex <= $lastindex) {
		# PROTOS screw us up; the following line worked, but then
		# PROTO information was not available. So, store the PROTO
		# node definition, BUT generate the real node!

		$c = $this->{Nodes}[$curindex];
		$ref = ref $c;
		#print "mkbe_and_array, working on node $curindex of $lastindex, it is ", VRML::NodeIntern::dump_name($c), " ref $ref\n";

		if ($ref eq "ARRAY") {
			$c = @{$c};
		} elsif ($ref eq "VRML::DEF") {
			$c = $c->node();
		}
		my $id;
		# lets make backend here, while we are having fun...
		if (!defined $c->{BackNode}) {
			#print "mkbe_and_array, node ",VRML::NodeIntern::dump_name($c), " does not have a backend\n";
			my $rn = $c;
			if (defined $c->{IsProto}) {
				#print "mkbe_and_array, node ",VRML::NodeIntern::dump_name($c), " IS a proto\n";
				my $sf;
				foreach $sf (@{$c->{ProtoExp}{Nodes}}) {
					if (ref $sf eq "VRML::DEF") {
						$sf = $sf->node();
					}
					$sf->{BackNode} = VRML::NodeIntern::make_backend($sf, $be);
					$id = VRML::NodeIntern::dump_name($sf);
					$q = "$q $id";
				}
			} else {
				#print "mkbe_and_array, node ",VRML::NodeIntern::dump_name($c), " IS NOT proto\n";
				$c->{BackNode} = VRML::NodeIntern::make_backend($c, $be);
				$id = VRML::NodeIntern::dump_name($c);
				$q = "$q $id";
			}
		} else {
			#print "mkbe_and_array, node ",VRML::NodeIntern::dump_name($c), " already has a backend\n";
			$id = VRML::NodeIntern::dump_name($c);
			$q = "$q $id";
		}

		## XXX redundant code???
		$id = VRML::NodeIntern::dump_name($c);
		#$q = "$q $id";
		$curindex++;
	}
	#print "mkbe_and_array at end, q is $q\n\n";


	# gather any DEFS. Protos will have new DEF nodes (expanded nodes)
	if (defined $this->{DEF}) {
		%{$parentscene->{DEF}} = (%{$parentscene->{DEF}}, %{$this->{DEF}});

		foreach ($this->{DEF}) {
			$parentscene->{DEF} = $_;
		}
	}
	if (defined $this->{Nodes}) {
		foreach (@{$this->{Nodes}}) {
			$_->gather_defs($parentscene);

			# reserve the Node so that EAI can get it, if necessary
			VRML::NodeIntern::dump_name($_->real_node());
		}
	}

	# need to sanitize ... it probably has trailing newline...
	$q =~ s/^\s+//;
	$q =~ s/\s+$//;
	return $q
}

## used for DEF'd nodes
sub getNode {
	my ($this, $name) = @_;
	#print "Scene::getNode, looking for $name in $this\n";

	my $n = $this->{DEF}->{VRML::Handles::return_def_name($name)};
	if (!defined $n) {
		#warn("Node $name is not defined");
		return undef;
	}
	return $n->node();
}

sub as_string {
	my ($this) = @_;
	join "\n ($this) ",map {$_->as_string()} @{$this->{Nodes}};
}

# Construct a full copy of this scene -- used for protos.
# Note: much data is shared - problems?
sub get_copy {
	my ($this, $name) = @_;
	my $new = bless {}, ref $this;

	if (defined $this->{URL}) {
		$new->{URL} = $this->{URL};
	}

	if (defined $this->{WorldURL}) {
		$new->{WorldURL} = $this->{WorldURL};
	}

	$new->{Name} = $this->{Name};
	$new->{Pars} = $this->{Pars};
	$new->{FieldTypes} = $this->{FieldTypes};

	$new->{Nodes} = [map { $_->copy($new) } @{$this->{Nodes}}];
	$new->{EventModel} = $this->{EventModel};

	my $key;
	foreach $key (keys(%{$this->{Protos}})) {
		#print "Scene.pm::get_copy: $key, $this->{Protos}{$key}\n";
		# try shallow copy first
		$new->{Protos}{$key} = $this->{Protos}{$key};
	}

	my ($route, $arrayRef);
	while (($route, $arrayRef) = each %{$this->{Routes}}) {
		$new->{Routes}{$route} = [$arrayRef->[0], $arrayRef->[1], $arrayRef->[2], $arrayRef->[3], 0];
	}

	return $new;
}

sub make_is {
	my ($this, $node, $field, $is) = @_;
	my $retval;

	print "VRML::Scene::make_is ", VRML::NodeIntern::dump_name($this),
		": ($node->{TypeName} ", VRML::Debug::toString($node),
			") $field IS ($this->{NodeParent}{TypeName} ",
				VRML::Debug::toString($this->{NodeParent}), ") $is\n"
						if $VRML::verbose::scene;

	my $proto_ft = $this->{NodeParent}{Type}{FieldKinds}{$is} or
		die("Missing field type for $is from statement: $field IS $is");
	my $node_ft = $node->{Type}{FieldKinds}{$field} or
		die("Missing field type for $node->{TypeName} $field from statement: $field IS $is");

		die("Incompatible field or event types in statement: $field IS $is")
			if ($proto_ft ne $node_ft and $node_ft ne "exposedField");

	# If child's a field or exposedField, get initial value
	print "VRML::Scene::make_is: $node_ft $field, NodeParent $proto_ft $is\n"
		 if $VRML::verbose::scene;

	my $ftype = "VRML::Field::"."$node->{Type}{FieldTypes}{$field}";

	if ($node_ft =~ /[fF]ield$/ and $proto_ft =~ /[fF]ield$/) {
		print "VRML::Scene::make_is: returning $this->{NodeParent}{Fields}{$is}\n"
			if $VRML::verbose::scene;
		$retval= $ftype->copy($this->{NodeParent}{Fields}{$is});
	} else {
		$retval = $node->{Type}{Defaults}{$is};
	}

	## add to event model
	$this->{EventModel}->add_is($this->{NodeParent}, $is, $node, $field);

	return $retval;
}

#
# Here come the expansions:
#  - make executable: create copies of all prototypes.
#

sub iterate_nodes {
	my ($this, $sub, $parent) = @_;

	print "VRML::Scene::iterate_nodes - PROTO expansion\n"
		 if $VRML::verbose::scene;

	for (@{$this->{Nodes}}) {
		$_->iterate_nodes($sub, $parent);
	}
}

sub set_parentnode {
	# NodeParent is used in IS's -
	# Parent is used for tracing back up the tree.

	my ($this, $nodeparent, $parent) = @_;

	$this->{NodeParent} = $nodeparent;
	$this->{Parent} = $parent;

	# keep Parent's's Proto hash up-to-date
	$parent->{Protos}{$this->{Name}} = $this;
}



# XXX This routine is too static - should be split and executed
# as the scene graph grows/shrinks.

{
	my %sends = map {($_ => 1)} qw/ Anchor TouchSensor TimeSensor GeoTouchSensor /;

	sub make_executable {
		my ($this) = @_;

		# Step 1) Go through all nodes in "this" scene. Make executable
		# on them. Hopefully, all nodes are of type VRML::NodeIntern!

		for (@{$this->{Nodes}}) {
			$_->make_executable($this);
		}


		# Step 2) Give all ISs references to my data
		if (defined $this->{NodeParent}) {
			print "VRML::Scene::make_executable: ", VRML::Debug::toString($this),
				" NodeParent ", VRML::Debug::toString($this->{NodeParent}), "\n"
					 if $VRML::verbose::scene;

			$this->iterate_nodes(sub {
			my ($node) = @_;

			print "MENID  ref node ", ref $node,"\n" if $VRML::verbose::scene;
			return unless (ref $node eq "VRML::NodeIntern");

			for (keys %{$node->{Fields}}) {
				print "MENIDF $_\n" if $VRML::verbose::scene;

				next unless (ref $node->{Fields}{$_} eq "VRML::IS");
				print "MENIDFSET $_\n" if $VRML::verbose::scene;
				$node->{Fields}{$_}->set_ref(
\$this->{NodeParent}{Fields}{$this->{NodeParent}{Fields}{$node->{Fields}{$_}{Name}}}
																	 );
									 }
								 });
		}


		# Step 3) Gather all 'DEF' statements and update
		my %DEF;
		$this->iterate_nodes(sub {
			 my ($node) = @_;

			 return unless (ref $node eq "VRML::DEF");

			 print "VRML::Scene::make_executable: ",
				 VRML::Debug::toString($node), "\n"
					 if $VRML::verbose::scene;
			 $DEF{$node->{Name}} = $node;

			# for EAI, reserve this name and node
			VRML::Handles::EAI_reserve ($node->{Name},$node->real_node());

			 if (!defined $this->{DEF}{$node->{Name}}) {
				 $this->{DEF}{$node->{Name}} = $DEF{$node->{Name}};
			 }
		 });


		# Step 4) Update all USEs
		# it is supposed to make a DEF ref into a NodeIntern...
		$this->iterate_nodes(sub {
			 my ($node) = @_;

			 return unless ref $node eq "VRML::USE";
			 print "VRML::Scene::make_executable: ",
				 VRML::Debug::toString($node), "\n"
						 if $VRML::verbose::scene;
			 # has this one been defined yet? if not, defer and pray...
			if (defined $DEF{$node->name()}) {
				#print "it is defined, lets do it!\n";
			 	$node->set_used($node->name(), $DEF{$node->name()});
			} else {
				#print "step 4, not defined yet, lets defer...\n";
			}
			 });


		# Step 5) Collect all prototyped nodes from here
		# so we can call their events

		$this->iterate_nodes(sub {
			 my ($node) = @_;

			 return unless ref $node eq "VRML::NodeIntern";
			 push @{$node->{SubScenes}}, $node
				 if $node->{ProtoExp};
			 push @{$node->{Sensors}}, $node
				 if $sends{$node};
		 });

	}
}

############################################################3
#
# SCENE::make_backend
#
############################################################

sub make_backend {
	my ($this, $be, $parentbe) = @_;
	my $vrml97_msg = "VRML97 4.8.3: 'A prototype definition consists of one or more nodes, nested PROTO statements, and ROUTE statements.'";

		my ($package, $filename, $line) = caller;
	print "VRML::Scene::make_backend ",VRML::NodeIntern::dump_name($this),
		" $be $parentbe caller $package :$line \n"
			if $VRML::verbose::be;

	return $this->{BackNode} if ($this->{BackNode}{CNode});

	# print "make_backend - node has has node $#{$this->{Nodes}}\n";
	my $bn;
	if ($this->{Parent}) {
		# I am a PROTO -- only my first node renders anything, but other nodes
		# need the CNode to be there for data storage (eg, scripts, interpolators...)

		print "VRML::Scene::make_be: PROTO ",
			VRML::NodeIntern::dump_name($this),
					", Nodes ", VRML::Debug::toString($this->{Nodes}),
						" $be $parentbe\n"
				if $VRML::verbose::be;

		#print "   has node $#{$this->{Nodes}}\n";

		# this is the first node; make it no matter what kind it is.
		if (! defined $this->{Nodes}[0]) {
			die("$vrml97_msg Empty prototype definition for $this->{NodeParent}{TypeName}");
		}
		$bn = $this->{Nodes}[0]->make_backend($be, $parentbe);
	} else {
		$bn = $this->{Nodes}[0]->make_backend($be, $parentbe);
		$this->{BackNode} = $bn;
	}
	return $bn;
}

# Events are generally in the format
#  [$scene, $node, $name, $value]

sub setup_routing {
    my ($this, $eventmodel, $be) = @_;
	my ($fromNode, $eventOut, $toNode, $eventIn, $tmp);

	if ($VRML::verbose::scene) {
		my ($package, $filename, $line) = caller;
		print "VRML::Scene::setup_routing: ",
			VRML::Debug::toString($this),
					", event model = $eventmodel, back end = $be from $package, $line";
	}

    $this->iterate_nodes(sub {
		 print "\tITNOREF: ",VRML::NodeIntern::dump_name($_[0]),"\n"
		  if $VRML::verbose::scene;

		 return unless "VRML::NodeIntern" eq ref $_[0];
		 print "\t\tITNO: $_[0]->{TypeName} ($VRML::Nodes::initevents{$_[0]->{TypeName}})\n"
			 if $VRML::verbose::scene;
		 if ($VRML::Nodes::initevents{$_[0]->{TypeName}}) {
			 print "\tITNO:is member of initevents\n"
				if $VRML::verbose::scene;


			# is this a proto expansion SFNode field?
			# if so, the backnode->{CNode} will need to be created
			if (!defined $_[0]->{BackNode}) {
				#print "backnode not defined\n";

				$_[0]->{BackNode} =
					VRML::NodeIntern::make_backend($_[0], $be);
			}

			VRML::VRMLFunc::add_first($_[0]->{TypeName}, $_[0]->{BackNode}->{CNode});
		 } else {
			 if ($_[0]->{ProtoExp}) {
				 #print "VRML::Scene::setup_routing, this $this is a proto, calling protoexp setup_routing\n";
				 $_[0]->{ProtoExp}->setup_routing($eventmodel, $be);
			 }
		 }
		 # Look at child nodes
		 my $c;
		 if ($c = $VRML::Nodes::Transchildren{$_[0]->{TypeName}}) {
			 my $ref = $_[0]->{RFields}{$c};
			 print "\tCHILDFIELD: GOT @$ref FOR CHILDREN\n"
				 if $VRML::verbose::scene;

			 for (@$ref) {
				 # XXX Removing/moving sensors?!?!
				 my $n = $_->real_node();

				 print "\tREALNODE: $n $n->{TypeName}\n" if $VRML::verbose::scene;
				 if ($VRML::Nodes::siblingsensitive{$n->{TypeName}}) {
					 print "VRML::Scene sibling sensitive $n, $n->{TypeName}, bn, ",
						 $_[0], "\n" if $VRML::verbose::scene;

					# print "going to set_sensitive. this node $_[0] \n";
					if (!defined $_[0]->{BackNode}) {
						# print "backNode not defined, going to make it here. \n";
						$_[0]->{BackNode} = VRML::NodeIntern::make_backend($_[0], $be);
					}

					VRML::VRMLFunc::set_sensitive ($_[0]->{BackNode}{CNode},
							$n->{BackNode}{CNode},$n->{TypeName});
				 }
			 }
		 }

		# Anchors, etc.
		 if ($VRML::Nodes::sensitive{$_[0]->{TypeName}}) {
			print "sesens call for ", $_[0]->{TypeName},"\n";
			my $n = $_->real_node();
			# print "going to set_sensitive. this node $_[0] \n";
			if (!defined $_[0]->{BackNode}) {
				# print "backNode not defined, going to make it here. \n";
				$_[0]->{BackNode} = VRML::NodeIntern::make_backend($_[0], $be);
			}

			VRML::VRMLFunc::set_sensitive ($_[0]->{BackNode}{CNode},$n->{BackNode}{CNode},$n->{TypeName});
		 }
	 });

	print "\tDEFINED NODES in ", VRML::NodeIntern::dump_name($this),": ",
		(join ',', keys %{$this->{DEF}}),"\n"
			if $VRML::verbose::scene;

	# any routes to add?
	for (values %{$this->{Routes}}) {
		next if ($_->[4]);
		#print "setup_routing calling add_route, ",VRML::Debug::toString($this),"\n";
		$_->[4] = $eventmodel->add_route($this, 1, $_->[0], $_->[1], $_->[2], $_->[3]);
	}

	# Any routes to delete? Maybe from the EAI...
	for (@{$this->{DelRoutes}}) {
		$tmp = VRML::Handles::get($_->[0]);
		if (ref $tmp eq "VRML::NodeIntern") {
			$fromNode = $tmp;
		} else {
			$fromNode = $this->getNode($tmp);
			if (!defined $fromNode) {
				warn("DEF node $tmp is not defined");
				next;
			}
		}

		$tmp = VRML::Handles::get($_->[2]);
		if (ref $tmp eq "VRML::NodeIntern") {
			$toNode = $tmp;
		} else {
			$toNode = $this->getNode($tmp);
			if (!defined $toNode) {
				warn("DEF node $tmp is not defined");
				next;
			}
		}
		$eventOut = VRML::Parser::parse_exposedField($_->[1], $fromNode->{Type});

		if (!$fromNode->{Type}{EventOuts}{$eventOut}) {
			warn("Invalid eventOut $eventOut in route for $fromNode->{TypeName}");
			next;
		}

		$eventIn = VRML::Parser::parse_exposedField($_->[3], $toNode->{Type});

		if (!$toNode->{Type}{EventIns}{$eventIn}) {
			warn("Invalid eventIn $eventIn in route for $toNode->{TypeName}");
			next;
		}

		# to delete, add_route with 2nd param of 0
		$eventmodel->add_route($this, 0, $fromNode, $eventOut, $toNode, $eventIn);
	}
	# ok, its processed; we don't want this route deleted from the events more than once
	$this->{DelRoutes} = [];

	print "VRML::Scene::setup_routing FINISHED for ",VRML::NodeIntern::dump_name($this),"\n"
		if $VRML::verbose::scene;
}

1;
