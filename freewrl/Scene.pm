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
				print "sk $sk ";
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

	# this was Nodes, but RootNode should contain all.
	if (defined $this->{RootNode}) {
		print "\n$padded(RootNode of ",VRML::NodeIntern::dump_name($this),")\n";
 		$this->{RootNode}->dump($level+1);
	} else {
		print "\n$padded(Nodes of ",VRML::NodeIntern::dump_name($this),")\n";
		for (@{$this->{Nodes}}) {
			$_->dump($level+1);
		}
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
					  RootNode => undef,
					  Nodes => undef,
					  NodeParent => undef,
					  Parent => undef,
					  Protos => undef,
					  Stack => undef,
					  DEF => undef
					 }, $type;
	print "VRML::Scene::new $this, $eventmodel, $url, $worldurl\n"
		if $VRML::verbose::scene;
	return $this;
	
}

# is this node a new "replaceWorld'd" node, and does it need to be put on the stack?
sub replaceWorld_Bindable {
    my ($this, $node) = @_;

    # Check if it is bindable and first -> bind to it later..

    if ($VRML::Nodes::bindable{$node->{TypeName}}) {
		# this should never happen...
		if (!defined $this->{Bindable}{$node->{TypeName}}) {
			$this->{Bindable}{$node->{TypeName}} = $node;
		}

		push @{$this->{Bindables}{$node->{TypeName}}}, $node;
    }
}

	
sub set_url {
    my ($this, $url) = @_;
    $this->{URL} = $url;
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

## Note: url handling needs to be fixed
sub newextp {
    my ($type, $pars, $parent, $name, $url) = @_;
    # XXX marijn: code copied from newp()

    my $this = $type->new();

    $this->{Pars} = $pars;
    $this->{Name} = $name;
    $this->{Parent} = $parent;

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
    # XXX marijn: fix this: only first url currently used
    if (ref $url) {
		$url = $url->[0];
    }

    print("EXTERNPROTO with URL: $url\n") if $VRML::verbose::parse;
    my ($protourl, $protoname) = split(/\#/, $url,2);
    # marijn: set the url for this proto
    $this->set_url($protourl);

    # XXX marijn: code copied from Browser->load_file()
    my $string = VRML::URL::get_relative($parent->{URL}, $protourl);

    # Required due to changes in VRML::URL::get_relative in URL.pm:
	## die or simply return?
	die "File $protourl was not found" if (!$string);

    if ($string =~/^<\?xml version/s) {
		eval 'require XML::LibXSLT';
		eval 'require XML::LibXML';

		my $parser = XML::LibXML->new();
		my $xslt = XML::LibXSLT->new();

		my $source = $parser->parse_string($string);
		my $style_doc = $parser->parse_file($VRML::Browser::X3DTOVRMLXSL);

		my $stylesheet = $xslt->parse_stylesheet($style_doc);

		my $results = $stylesheet->transform($source);

		$string = $stylesheet->output_string($results);
    }

	unless ($string =~ /^#VRML V2.0/s) {
		die("Sorry, this file is according to VRML V1.0, I only know V2.0")
			if ($string =~ /^#VRML V1.0/);
		warn("File $protourl doesn't start with the '#VRML V2.0' header line");
	}

	# XXX marijn: code copied from Browser->parse()
	my $po = pos $string;
    while ($string =~ /([#\"])/gsc) {
		(pos $string)--;
		if ($1 eq "#") {
			$string =~ s/#.*$//m;		
		} else {
			VRML::Field::SFString->parse($this, $string);
		}
    }
    (pos $string) = $po;

    # marijn: end of copying, now locate right PROTO
    my $succes = 0;
    while ($string =~ /[\s,^](PROTO\s+)($VRML::Parser::Word)/gsc ) {
		if (!$protoname) {
			$protoname = $2;
		}

		if ($2 eq $protoname) {
			(pos $string) -= ((length $1) + (length $2));
			VRML::Parser::parse_statement($this, $string);
			$succes = 1;
			last;
		}
    }

	VRML::Error::parsefail("no PROTO found", "$url") if (!$succes);

    # marijn: now create an instance of the PROTO, with all fields ISsed.
    # XXX marijn: What about FieldTypes/FieldKinds?

    my %fields = map {$_ => $this->new_is($_)} keys %{$this->{Pars}};
    my $n = $this->new_node($protoname, \%fields);
    my @node = ($n);

    # XXX marijn: code copied from Parser::parse_proto
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
	if ($type eq "Script") {
		# Special handling for Script which has an interface.
		my $t = "__script__".$cnt++;
		my %f = (
				 url => [MFString, [], exposedField],
				 directOutput => [SFBool, 0, field],
				 mustEvaluate => [SFBool, 0, field]
				);
		for (keys %$fields) {
			$f{$_} = [
				$fields->{$_}[1],
				$fields->{$_}[2],
				$fields->{$_}[0],
			];
		}
		my $type = VRML::NodeType->new($t, \%f, $VRML::Nodes{Script}{Actions});
		my $node = VRML::NodeIntern->new_script($this, $type, {}, $this->{EventModel});
		VRML::Handles::reserve($node);
		return $node;
	}

	my $node = VRML::NodeIntern->new($this, $type, $fields, $this->{EventModel});

	# Check if it is bindable and first -> bind to it later..
	if ($VRML::Nodes::bindable{$type}) {
		if (!defined $this->{Bindable}{$type}) {
			$this->{Bindable}{$type} = $node;
		}
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

sub delete_route {
	my ($this, $fn, $eo, $tn, $ei) = @_;
	my $route = "$fn"."$eo"."$tn"."$ei";

	print "VRML::Scene::delete_route: ", VRML::Debug::toString(\@_), "\n"
		if $VRML::verbose::scene;

	if (exists $this->{Routes}{$route}) {
		# make sure that the eventmodel knows that this route no longer exists
		push @{$this->{DelRoutes}}, $this->{Routes}{$route};
		# take it off the known routes for this node.
		delete $this->{Routes}{$route};
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

	return VRML::USE->new($name, $this->{DEF}{$name})
		if (defined $this->{DEF}{$name});

	return VRML::USE->new($name,
						  (VRML::Handles::return_def_name($name)));
}

sub new_is {
	my ($this, $name, $is) = @_;
	return VRML::IS->new($name, $is);
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

	my $p = $this->{Protos}{$name} = (ref $this)->newextp($pars, $this, $name, $url);
	return $p;
}

sub topnodes {
	my ($this, $nodes) = @_;

	$this->{Nodes} = $nodes;
	$this->{RootNode} = $this->new_node("Group",{children => $nodes});
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

	if (defined $this->{Scene}{DEF}) {
		%{$parentscene->{DEF}} = (%{$parentscene->{DEF}},%{$this->{Scene}{DEF}});
	}

	# Now, should we return the "RootNode" if it exists (which is a Group node)
	# or, a list of the "Node"s?
	# lets try returning the RootNode, as this way we always have a group to
	# add in the case of a proto.

	my $c;
	my $q = "";
	my $lastindex = $#{$this->{Nodes}};

	my $curindex = 0;
	while ($curindex <= $lastindex) {
		# PROTOS screw us up; the following line worked, but then
		# PROTO information was not available. So, store the PROTO
		# node definition, BUT generate the real node!

		$c = $this->{Nodes}[$curindex];

		if ("ARRAY" eq ref $c) {
			$c = @{$c};
		} elsif ("VRML::DEF" eq ref ($c)) {
			#AK - #$c = $c->real_node(1);
			$c = $c->node();
		}

		my $id;
		# lets make backend here, while we are having fun...
		if (!defined $c->{BackNode}) {
			my $rn = $c;
			if (defined $c->{IsProto}) {
				my $sf;
				foreach $sf (@{$c->{ProtoExp}{Nodes}}) {
					if ("VRML::DEF" eq ref $sf) {
						$sf = $sf->node();
					}
					$sf->{BackNode} = VRML::NodeIntern::make_backend($sf, $be);
					$id = VRML::NodeIntern::dump_name($sf);
					##my $id = VRML::Handles::reserve($sf);
					$q = "$q $id";
				}
			} else {
				$c->{BackNode} = VRML::NodeIntern::make_backend($c, $be);
			}
		} else {
			$id = VRML::NodeIntern::dump_name($c);
			##my $id = VRML::Handles::reserve($c);
			$q = "$q $id";
		}

		## XXX redundant code???
		$id = VRML::NodeIntern::dump_name($c);
		##my $id = VRML::Handles::reserve($c);
		$q = "$q $id";
		$curindex++;
	}


	# gather any DEFS. Protos will have new DEF nodes (expanded nodes)
	if (defined $this->{DEF}) {
		#print "\nmkbe_and_array: Gathering defs of ",VRML::NodeIntern::dump_name($this),
			#" parentscene ",VRML::NodeIntern::dump_name($parentscene),"\n";

		%{$parentscene->{DEF}} = (%{$parentscene->{DEF}}, %{$this->{DEF}});

		foreach ($this->{DEF}) {
			$parentscene->{DEF} = $_;
			#print "Gathering, found ";
			#foreach (keys %{$_}) {
			#	print "key $_ ";
			#}
			#print "\n";
		}
		#print "parentscene DEFS is now \n";
		#foreach ($parentscene->{DEF}) {
		#	foreach (keys %{$_}) {
		#		print $_;
		#	}
		#}
		#print "\nfinished $this defs\n";
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
	my $n = $this->{DEF}->{VRML::Handles::return_def_name($name)};
	if (!defined $n) {
		warn("Node $name is not defined");
		return undef;
	}

	#AK - #return $n->real_node(1); # Return proto enclosing node.
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

	$new->{Pars} = $this->{Pars};
	$new->{FieldTypes} = $this->{FieldTypes};

	$new->{Nodes} = [map { $_->copy($new) } @{$this->{Nodes}}];
	$new->{EventModel} = $this->{EventModel};

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
		": ($node->{TypeName} ", VRML::NodeIntern::dump_name($node),
			") $field IS ($this->{NodeParent}{TypeName} ",
				VRML::NodeIntern::dump_name($this->{NodeParent}), ") $is\n"
						if $VRML::verbose::scene;


	my $proto_ft = $this->{NodeParent}{Type}{FieldKinds}{$is} or
		die("Missing field type for $is from statement: $field IS $is");
	my $node_ft = $node->{Type}{FieldKinds}{$field} or
		die("Missing field type for $node->{TypeName} $field from statement: $field IS $is");

		die("Incompatible field or event types in statement: $field IS $is")
			if ($proto_ft ne $node_ft and $node_ft ne "exposedField");

	# If child's a field or exposedField, get initial value
	print "CK: $node_ft, PK: $proto_ft\n" 
		 if $VRML::verbose::scene;

	if ($node_ft =~ /[fF]ield$/ and $proto_ft =~ /[fF]ield$/) {
		print "SETV: $_ NP : '$this->{NodeParent}' '$this->{NodeParent}{Fields}{$_}'\n"
			if $VRML::verbose::scene;
		$retval=
		 "VRML::Field::$node->{Type}{FieldTypes}{$field}"->copy($this->{NodeParent}{Fields}{$is});
	} else {
		$retval = $node->{Type}{Defaults}{$_};
	}

	## add to event model
	if ($this->{NodeParent}{Type}{EventIns}{$is} and
		$node->{Type}{EventIns}{$field}) {
		$this->{EventModel}->add_is_in($this->{NodeParent}, $is, $node, $field);
	} elsif ($this->{NodeParent}{Type}{EventOuts}{$is} and
		$node->{Type}{EventOuts}{$field}) {
		$this->{EventModel}->add_is_out($this->{NodeParent}, $is, $node, $field);
	}

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

	if ($this->{RootNode}) {
		$this->{RootNode}->iterate_nodes($sub, $parent);
	} else {
		for (@{$this->{Nodes}}) {
			$_->iterate_nodes($sub, $parent);
		}
	}
}

sub iterate_nodes_all {
	my ($this, $subfoo) = @_;

	my @l;
	if ($this->{RootNode}) {
		@l = $this->{RootNode};
	} else {
		@l = @{$this->{Nodes}};
	}
	my $sub;
	for (@l) {
		$sub = sub {
			&$subfoo($_[0]);
			if (ref $_[0] eq "VRML::NodeIntern" and $_[0]->{ProtoExp}) {
				$_[0]->{ProtoExp}->iterate_nodes($sub);
			}
		};
		$_->iterate_nodes($sub);
		$sub = undef;
	}
}

sub set_parentnode {
	# NodeParent is used in IS's -
	# Parent is used for tracing back up the tree.

	my ($this, $nodeparent, $parent) = @_;

	$this->{NodeParent} = $nodeparent;
	$this->{Parent} = $parent;
}



# XXX This routine is too static - should be split and executed
# as the scene graph grows/shrinks.

# JAS XXX - I don't think that TouchSensors (maybe TimeSensors??) are 
# cought by this "sends" stuff....

{
	my %sends = map {($_ => 1)} qw/ TouchSensor TimeSensor /;

	sub make_executable {
		my ($this) = @_;

		# Step 1) Go through all nodes in "this" scene. Make executable
		# on them. Hopefully, all nodes are of type VRML::NodeIntern!

		for (@{$this->{Nodes}}) {
			$_->make_executable($this);
		}


		# Step 2) Give all ISs references to my data
		if (defined $this->{NodeParent}) {
			print "VRML::Scene::make_executable: ", VRML::NodeIntern::dump_name($this),
				" NodeParent ", VRML::NodeIntern::dump_name($this->{NodeParent})
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
\$this->{NodeParent}{Fields}{$this->{NodeParent}{Fields}{$node->{Fields}{$_}{Name}}}, $_
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

								 if (!defined $this->{DEF}{$node->{Name}}) {
									 $this->{DEF}{$node->{Name}} = $DEF{$node->{Name}};
								 }
							 });
	

		# Step 4) Update all USEs
		$this->iterate_nodes(sub {
								 my ($node) = @_;

								 return unless ref $node eq "VRML::USE";
								 print "VRML::Scene::make_executable: ",
									 VRML::Debug::toString($node), "\n"
											 if $VRML::verbose::scene;
								 $node->set_used($node->name(), $DEF{$node->name()});
							 });


		# Step 5) Collect all prototyped nodes from here
		# so we can call their events
		# JAS XXX I think that this is just BS, as Sensors, SubScenes are
		# not found anywhere else, and the ref is VRML::Scene in here...
		# Still, until I make sure that TouchSensors are ok from within
		# PROTOS...

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

	print "VRML::SCENE::make_backend ",VRML::NodeIntern::dump_name($this),
		" $be $parentbe \n"
			if $VRML::verbose::be;

	return $this->{BackNode} if ($this->{BackNode});

	my $bn;
	if ($this->{Parent}) {
		# I am a PROTO -- only my first node renders anything...
		print "\tScene: I'm a PROTO ",VRML::NodeIntern::dump_name($this),
			" $be $parentbe\n"
				if $VRML::verbose::be;
		$bn = $this->{Nodes}[0]->make_backend($be, $parentbe);
	} else {
		print "\tScene: I'm not PROTO $this $be $parentbe ($this->{IsInline})\n"
			if $VRML::verbose::be;

		$bn = $this->{RootNode}->make_backend($be, $parentbe);

		$be->set_root($bn) unless $this->{IsInline};

 		$be->set_vp_sub(
 			sub {
 				my $b = $this->get_browser();
 				my $vn = $b->get_vp_node();
 				my $vs = $b->get_vp_scene();
 				return if (!defined $vn);
 				$b->set_next_vp();
 				$vs->{EventModel}->send_event_to($vn, set_bind, 1);
 			}
 		);	
	}
	$this->{BackNode} = $bn;
	return $bn;
}

# Events are generally in the format
#  [$scene, $node, $name, $value]

# XXX This routine is too static - should be split and executed
# But also: we can simply redo this every time the scenegraph
# or routes change. Of course, that's a bit overkill.

sub setup_routing {
    my ($this, $eventmodel, $be) = @_;
	my ($fromNode, $eventOut, $toNode, $eventIn, $tmp);

	if ($VRML::verbose::scene) {
		my ($package, $filename, $line) = caller;
		print "VRML::Scene::setup_routing: ",
			VRML::NodeIntern::dump_name($this),
					", event model = $eventmodel, back end = $be from $package, $line\n";
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
								 $eventmodel->add_first($_[0]);
							 } else {
								 if ($_[0]->{ProtoExp}) {
									 # print "VRML::Scene::setup routing, this is a proto, calling protoexp setup_routing\n";
									 $_[0]->{ProtoExp}->setup_routing($eventmodel, $be);
								 }
							 }
							 # Look at child nodes
							 my $c;
							 if ($c = $VRML::Nodes::children{$_[0]->{TypeName}}) {
								 my $ref = $_[0]->{RFields}{$c};
								 print "\tCHILDFIELD: GOT @$ref FOR CHILDREN\n"
									 if $VRML::verbose::scene;

								 for (@$ref) {
									 # XXX Removing/moving sensors?!?!
									 my $n = $_->real_node();

									 print "\tREALNODE: $n $n->{TypeName}\n" if $VRML::verbose::scene;
									 if ($VRML::Nodes::siblingsensitive{$n->{TypeName}}) {
										 print "VRML::Scene sibling sensitive $n $n->{TypeName} bn ",
											 $_[0], "\n" if $VRML::verbose::scene;
										 $be->set_sensitive($_[0]->{BackNode},
															sub {
																$eventmodel->handle_touched($n, @_);
															});
									 }
								 }
							 }

							 if ($VRML::Nodes::sensitive{$_[0]->{TypeName}}) {
								 $be->set_sensitive($_[0]->{BackNode}, sub {},);
							 }
						 });

	print "\tDEFINED NODES in ", VRML::NodeIntern::dump_name($this),": ",
		(join ',', keys %{$this->{DEF}}),"\n"
			if $VRML::verbose::scene;

	# any routes to add?
	for (values %{$this->{Routes}}) {
		next if ($_->[4]);

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

		## exposedFields and eventOuts with some error checking
		$eventOut = $_->[1];
		if ($eventOut =~ /($VRML::Error::Word+)_changed$/) {
			$tmp = $1;
			if ($fromNode->{Type}{EventOuts}{$tmp} and
				$fromNode->{Type}{FieldKinds}{$tmp} =~ /^exposed/) {
				$eventOut = $tmp;
			}
		}

		if (!$fromNode->{Type}{EventOuts}{$eventOut}) {
			warn("Invalid eventOut $eventOut in route for $fromNode->{TypeName}");
			next;
		}

		## exposedFields and eventIns with some error checking
		$eventIn = $_->[3];
		if ($eventIn =~ /^set_($VRML::Error::Word+)/) {
			$tmp = $1;
			if ($toNode->{Type}{EventIns}{$tmp} and
				$toNode->{Type}{FieldKinds}{$tmp} =~ /^exposed/) {
				$eventIn = $tmp;
			}
		}

		if (!$toNode->{Type}{EventIns}{$eventIn}) {
			warn("Invalid eventIn $eventIn in route for $toNode->{TypeName}");
			next;
		}

		$_->[4] = $eventmodel->add_route($fromNode, $eventOut, $toNode, $eventIn);
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
		$eventOut = $_->[1];
		if ($eventOut =~ /($VRML::Error::Word+)_changed$/) {
			$tmp = $1;
			if ($fromNode->{Type}{EventOuts}{$tmp} and
				$fromNode->{Type}{FieldKinds}{$tmp} =~ /^exposed/) {
				$eventOut = $tmp;
			}
		}

		if (!$fromNode->{Type}{EventOuts}{$eventOut}) {
			warn("Invalid eventOut $eventOut in route for $fromNode->{TypeName}");
			next;
		}

		$eventIn = $_->[3];
		if ($eventIn =~ /^set_($VRML::Error::Word+)/) {
			$tmp = $1;
			if ($toNode->{Type}{EventIns}{$tmp} and
				$toNode->{Type}{FieldKinds}{$tmp} =~ /^exposed/) {
				$eventIn = $tmp;
			}
		}

		if (!$toNode->{Type}{EventIns}{$eventIn}) {
			warn("Invalid eventIn $eventIn in route for $toNode->{TypeName}");
			next;
		}

		$eventmodel->delete_route($fromNode, $eventOut, $toNode, $eventIn);
	}
	# ok, its processed; we don't want this route deleted from the events more than once
	$this->{DelRoutes} = [];

	print "VRML::Scene::setup_routing FINISHED for ",VRML::NodeIntern::dump_name($this),"\n"
		if $VRML::verbose::scene;
}


# Send initial events
sub init_routing {
	my ($this, $eventmodel, $backend, $no_bind) = @_;
	# XXX no_bind not used - I initialize all subnodes...
	my @e;

	print "VRML::Scene::init_routing\n" if $VRML::verbose::scene;

	$this->iterate_nodes_all(sub { push @e, $_[0]->initialize($this); });

	for (keys %{$this->{Bindable}}) {
		print "\tINIT Bindable '$_'\n" if $VRML::verbose::scene;
		$eventmodel->send_event_to($this->{Bindable}{$_}, set_bind, 1);
	}
	$eventmodel->put_events(\@e);
}


sub update_routing {
	my ($this, $node, $field) = @_;
	##my ($fn, $ff, $tn, $tf, $item);

	## for EAI:
	## add code to delete route if a child is removed if necessary...
	## what else???
}


sub set_bind {
    my ($this, $node, $value, $time) = @_;
    my $t = $node->{TypeName};
    my $s = ($this->{Stack}{$t} or $this->{Stack}{$t} = []);
    print "\nVRML::Scene::set_bind $this\n\t($node $t $value),\n\tSTACK #: $#$s\n"
		if $VRML::verbose::bind;
    if ($value) {
		print "\tScene: BINDING IT!\n"
			if $VRML::verbose::bind;
		if ($#$s != -1) {  # Do we have a stack?

			if ($node == $s->[-1]) {
				print("Scene: Bind node is top of stack...\n")
					if $VRML::verbose::bind;
				return; ## JAS - according to the book, do nothing.
			}

			my $i;
			for (0..$#$s) {
				if ($s->[$_] == $node) {$i = $_}
			}

			print "\tScene: WAS AS '$i'\n" if $VRML::verbose::bind;

			$s->[-1]->{RFields}{isBound} = 0;
			if ($s->[-1]->{Type}{Actions}{WhenUnBound}) {
				&{$s->[-1]->{Type}{Actions}{WhenUnBound}}($s->[-1], $this);
			}

			splice(@$s, $i, 1) if (defined $i);
		}

		$node->{RFields}{bindTime} = $time;
		$node->{RFields}{isBound} = 1;
		if ($node->{Type}{Actions}{WhenBound}) {
			&{$node->{Type}{Actions}{WhenBound}}($node, $this,0);
		}

		print "\tScene: PUSHING $node on @{$s}\n" if $VRML::verbose::bind;
		push @$s, $node;
		print "\tScene: ..... is @{$s}\n" if $VRML::verbose::bind;
    } else {
		# We're unbinding a node.
		print "\tScene: UNBINDING IT!\n" if $VRML::verbose::bind;
		if ($node == $s->[-1]) {
			print "\tScene: WAS ON TOP!\n" if $VRML::verbose::bind;
			$node->{RFields}{isBound} = 0;
			if ($node->{Type}{Actions}{WhenUnBound}) {
				&{$node->{Type}{Actions}{WhenUnBound}}($node, $this);
			}

			pop @$s;
			if (@$s) {
				$s->[-1]->{RFields}{isBound} = 1;
				$s->[-1]->{RFields}{bindTime} = $time;
				if ($s->[-1]->{Type}{Actions}{WhenBound}) {
					&{$s->[-1]->{Type}{Actions}{WhenBound}}($s->[-1], $this,1);
				}
			}
		} else {
			my $i;
			for (0..$#$s) {
				if ($s->[$_] == $node) {$i = $_}
			}
			print "\tScene: WAS AS '$i'\n" if $VRML::verbose::bind;
			splice(@$s, $i, 1) if (defined $i);
		}
		print "\n" if $VRML::verbose::bind;
    }
}

1;
