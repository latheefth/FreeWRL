# Copyright (C) 1998 Tuomas J. Lukka 1999 John Stewart CRC Canada
# DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
# See the GNU Library General Public License (file COPYING in the distribution)
# for conditions of use and redistribution.

# Scene.pm
#
# Implement a scene model, with the specified parser interface.


# The idea here has been to try to preserve as much as possible
# of the original file structure -- that may not be the best approach
# in the end, but all dependencies on that decision should be in this file.
# It would be pretty easy, therefore, to write a new version that would
# just discard the original structure (USE, DEF, IS).
#
# At some point, this file should be redone so that it uses softrefs
# for circular data structures.

use strict vars;
use VRML::Parser;

#######################################################
#
# The FieldHash
#
# This is the object behind the "RFields" hash member of
# the object VRML::Node. It allows you to send an event by
# simply saying "$node->{RFields}{xyz} = [3,4,5]" for which 
# calls the STORE method here which then queues the event.
#
# XXX This needs to be separated into eventins and eventouts --
# assigning has different meanings.

package VRML::FieldHash;
@VRML::FieldHash::ISA=Tie::StdHash;

sub TIEHASH {
	my($type,$node) = @_;
	bless \$node, $type;
}

{my %DEREF = map {($_=>1)} qw/VRML::IS/;
my %REALN = map {($_=>1)} qw/VRML::DEF VRML::USE/;
sub FETCH {
	my($this, $k) = @_;
	my $node = $$this;
	my $v = $node->{Fields}{$k};

# print "FETCH, this $this, node ",VRML::Node::dump_name($node), " v $v k $k\n";

	if($VRML::verbose::tief) {
		print "TIEH: FETCH $k $v\n" ;
		if("ARRAY" eq ref $v) {
			print "TIEHARRVAL: @$v\n";
		}
	}
	# while($DEREF{ref $v}) {
	#	$v = ${$v->get_ref};
	#	print "DEREF: $v\n" if $VRML::verbose::tief;
	#}

	while($REALN{ref $v}) {
		$v = $v->real_node;
		print "TIEH: MOVED TO REAL NODE: $v\n"
			if $VRML::verbose::tief;
	}
	if(ref $v eq "VRML::IS") {
		print "Is should've been dereferenced by now -- something's cuckoo\n";
		exit (1);
	}
	return $v;
}

sub STORE {
	my($this, $k, $value) = @_;
	if($VRML::verbose::tief) {
		print "TIEH: STORE Node $this K $k value $value\n" ;
		if("ARRAY" eq ref $value) {
			print "TIEHARRVAL: @$value\n";
		}
	}
	my $node = $$this;
	my $v = \$node->{Fields}{$k};
	# while($DEREF{ref $$v}) {
	# 	$v = ${$v}->get_ref;
	# 	print "DEREF: $v\n" if $VRML::verbose::tief;
	# }
	$$v = $value;
	if ($VRML::verbose::events) {
		if ($k eq "__data") {
			print "STORE, $node, $k, BINARY DATA\n";
		} else { 
			print "STORE, $node, $k, $value\n";
		}
	}

	if (defined $node->{EventModel}){
	  print "STORE, defined eventmodel\n" if $VRML::verbose::events;
	  $node->{EventModel}->put_event($node, $k, $value);
	  if(defined $node->{BackNode}) { 
		print "STORE, BackNode defined\n"  if $VRML::verbose::events;
		$node->set_backend_fields($k);
	  }
	}
}
}

sub FIRSTKEY {
	return undef
}

#####################################################
#
# IS, DEF, USE, NULL
#
# The following packages implement some of the possible 
# structures in the VRML file.
#
# The implementation here may change for efficiency later.
# 
# However, the fact that we go through these does not usually
# make performance too bad since it only affects us when there
# are changes of the scene graph or IS'ed field values.
#


###################
 
package VRML::IS;

###################
 
sub new {bless [$_[1]],$_[0]}
sub copy {my $a = $_[0][0]; bless [$a], ref $_[0]}
sub make_executable {

	my($this,$scene,$node,$field) = @_;
	# print "make executable in IS, node $node field $field\n";
}
sub iterate_nodes {
	my($this,$sub) = @_;
	# print "iterate_nodes in package VRML::IS, sub is $sub\n";
	&$sub($this);
	# print "iterate_nodes in package VRML::IS, sub is $sub DONE\n";
}
sub name { $_[0][0] }
sub set_ref { $_[0][1] = $_[1] }
sub get_ref { if(!defined $_[0][1]) {print "IS not def!\n"; exit(1)} $_[0][1] }
sub initialize {()}

sub as_string {" ($_) IS $_[0][0] "}

sub gather_defs { # print "VRML::IS::gather_defs called - null procedure\n";
		}

sub dump { 
	my ($this,$level) = @_;
	print "$level node $this is ",$this->name(),"\n";
}

###################
 
package VRML::DEF;

###################
 
sub new {
	# parameters are:
	# 0 - the object ref type (VRML::DEF)
	# 1 - the name of the object
	# 2 - the node of the object
	# print "VRML::DEF, blessing ",$_[1], " and ",$_[2],"\n";
	bless [$_[1],$_[2]],$_[0];

}

sub copy {
	(ref $_[0])->new($_[0][0], $_[0][1]->copy($_[1]))
}

sub make_executable {
	# print "make executable in DEF  \n";
	$_[0][1]->make_executable($_[1]);
}

sub make_backend {
	print "DEF::make_backend $_[0] $_[0][0] ", VRML::Node::dump_name($_[0][1]),"\n" 
			if $VRML::verbose::be;
	return $_[0][1]->make_backend($_[1],$_[2]);
}

sub iterate_nodes {
	my($this,$sub,$parent) = @_;
	# print "iterate_nodes in package VRML::DEF, sub is $sub \n";
	&$sub($this,$parent);
	$this->[1]->iterate_nodes($sub,$parent);
	# print "iterate_nodes in package VRML::DEF, sub is $sub DONE\n";
}

sub name { return $_[0][0]; }

sub def { return $_[0][1]; }

sub get_ref { $_[0][1] }

sub real_node { 
        # print "(a)in real_node $_ in VRML::DEF, ", $_[0],"\n";
        # print "(Name)                       ", $_[0][0], "\n";
        # print "(Node)                       ", $_[0][1], "\n";
        # print "ref of Node is               ", ref($_[0][1]), "\n";

        return $_[0][1]->real_node($_[1]); 
}

sub initialize {()}

sub as_string {" ($_) DEF Name: $_[0][0] Node: $_[0][1] ".$_[0][1]->as_string}

sub gather_defs {
	my ($this,$parentnode) = @_;

	# print "VRML::DEF - gather defs, this is ", VRML::Node::dump_name($this), "name is ",
	# 			$this->name(), " ref is ", $this->get_ref(),
	# 			"  parent is ", VRML::Node::dump_name($parentnode),"\n";

	$parentnode->{DEF}{$this->name()} = $this->get_ref();
	my $real = $this->get_ref();
	# print "		real node is ",VRML::Node::dump_name($real),")\n";
	$real->gather_defs($parentnode);
}
	
sub dump {
	my ($this, $level) = @_;


	print "VRML::DEF, name is ",$this->name(),"\n";
	my $real =  $this->get_ref();
	$real->dump($level+1);
}

####################

package VRML::USE;

###################
 
sub new {
	my($objtype,$defname,$defdef) = @_;
        my $arr = [$defname,$defdef];
	bless $arr,$objtype;
	return $arr;
}

sub copy {(ref $_[0])->new(@{$_[0]})}

sub make_executable {
	# print "make executable in USE\n";
}

sub set_used {
	my($this, $node) = @_;
	$this->[1] = $node;
}

sub make_backend {
	print "USE::make_backend $_[0] $_[0][0] $_[0][1]\n" if $VRML::verbose::be;
	return $_[0][1]->make_backend($_[1], $_[2]);
}

sub iterate_nodes {
	my($this,$sub,$parent) = @_;
	# print "iterate_nodes in package VRML::USE, sub is $sub\n";
	&$sub($this,$parent);
	# print "iterate_nodes in package VRML::USE, sub is $sub DONE\n";
}

sub name { return $_[0][0]; }

sub real_node { 
	# ok - the following conventions/params are here...
	#
	# appearance DEF Grey Appearance 
	#	{ material Material 
	#		{ emissiveColor .2 .2 .2 specularColor 1.0 0 0 } 
	#	}
	#
	# $_ is the "name" of the DEF... eg, "appearance".
	# $_[0] is a "use" array, defined and blessed. It contains...
	# $[0][0] name. In this case, "Grey".
	# $[0][1] the node for the definition. 
	# $[0][2] the original scene for the definition.

	# print "(a)in real_node $_ in VRML::USE, ", $_[0],$_[0]->as_string(),"\n";
	# print "(Name)                       ", $_[0][0], "\n";
	# print "(Node)                       ", $_[0][1], "\n";
	# print "ref of Node is               ", ref($_[0][1]), "\n";

	return $_[0][1]->real_node($_[1]); 
}


sub get_ref { $_[0][1] }

sub initialize {()}

sub as_string {" ($_) USE Name: $_[0][0]  Node: $_[0][1] "}

sub gather_defs { 
		#print "VRML::USE::gather_defs called - null procedure\n";
		}


sub dump { 
	my ($this,$level) = @_;
	print "$level node VRML::USE, $this is ",$this->name(),"\n";
}


###################
 
package NULL; # ;)

###################
 
sub make_backend {return ()}
sub make_executable {}
sub iterate_nodes {

	# print "iterate_nodes in package VRML::NULL\n";
}
sub as_string {" ($_) NULL"}

sub gather_defs { 
	# print "VRML::NULL::gather_defs called - null procedure\n";
	}
sub dump {}

###############################################################
#
# This is VRML::Node, the internal representation for a single
# node.

package VRML::Node;

my %DUMPNAMES = ();
my $DNINDEX = 1;
my $SCINDEX = 1;

sub dump_name {
        my ($name) = @_;

	my $nr = ref $name;

        if (!exists $DUMPNAMES{$name}) {
		if ("VRML::Node" eq $nr) {
			$DUMPNAMES{$name} = "NODE$DNINDEX";
			$DNINDEX++;
		} elsif ("VRML::Scene" eq $nr) {
			$DUMPNAMES{$name} = "SCENE$SCINDEX";
			$SCINDEX ++;
		} elsif ("VRML::NodeType" eq $nr) {
			$DUMPNAMES{$name} = $name->{Name};
		} else {
			# dont map this one
			return $name;
		}
		
	}
	return $DUMPNAMES{$name};
}

sub gather_defs {
	my ($this,$parentnode) = @_;

	# print "Scene::VRML::Node gather_defs, this ",dump_name($this), "   parentnode ",
	# 	dump_name($parentnode),"\n";

	if (defined $this->{IsProto}) {
		# possibly this is a Scene PROTO Expansion
		foreach (@{$this->{ProtoExp}{Nodes}}) {
			# print "VRML::Node in ",dump_name($this)," we are going to look at ",
			# 	dump_name($_),"\n";
			$_->gather_defs($parentnode);
		}
	} elsif (defined $this->{Fields}) {
                # print "VRML::Node gather_defs (children of ",dump_name($this),")\n";

		my $fld;
		for $fld (keys %{$this->{Fields}}) {
			if (ref $this->{Fields}{$fld} eq "ARRAY") {
				# first level of an array...

				foreach (@{$this->{Fields}{$fld}}) { 
					if (ref $_ ne "") { 
						if (ref $_ eq "ARRAY") {
							# two dimensional array

							my $ae;
							foreach $ae (@{$_}) {
								if (ref $ae ne "") {
									$ae->gather_defs($parentnode);
								}
							}
						} else {
							$_->gather_defs($parentnode);
						}
					}
				}
			} else {
				if (ref $this->{Fields}{$fld} ne "") {
					$this->{Fields}{$fld}->gather_defs($parentnode);
				}
			}
		}
        }
}


sub dump {
	my ($this,$level) = @_;

	if ($level > 20) {
		print "level too deep, returning\n";
		return();
	}
	my $lp = $level*2+2;
	my $padded = pack("A$lp","$level ");

	# Debugging - print out this node.
	print $padded,"NODE DUMP OF: ",dump_name($this),"\n";
	foreach (keys %{$this}) {
		print $padded,"$_\t";

		# lets do something special for Routes
		if ("Routes" eq $_) {
       		 for (@{$this->{$_}}) {
                	my($fnam, $ff, $tnam, $tf) = @$_;
                	print "$padded    Route from $fnam field $ff to $tnam field $tf\n";
        	 }
					
		} elsif ("ARRAY" eq ref $this->{$_}) {
			print "(array) ";
			foreach (@{$this->{$_}}) {
				print dump_name($_);
				print " ";
			}
		} elsif ("HASH" eq ref $this->{$_}) {
			print "(hash) ";
			foreach (keys %{ $this->{$_}}) {
			  print "$_ ";
			}
		} else {
		 	print dump_name($this->{$_});
		}
		print "\n";
	}

	if (defined $this->{SceneRoutes}) {
        	print "\n$padded(SceneRoutes of ",dump_name($this),")\n";
	        my $sk;
	        foreach $sk (@{{$this}->{SceneRoutes}}) {
	                print "SceneRoutes of ",dump_name($this),") ";
			print "$sk\n";
	                $sk->dump($level+1);
	        }
	}

	if (defined $this->{ProtoExp}) {
		print "\n$padded(ProtoExp of ",dump_name($this),"\n";
		$this->{ProtoExp}->dump($level+1);
	}

	if (defined $this->{Type}) {
		print "\n$padded(Type of ",dump_name($this),")\n";
		if ("VRML::NodeType" ne ref $this->{Type}) {
			$this->{Type}->dump($level+1);
		}
	}

	if (defined $this->{Fields}) {
		my $fld;
		for $fld (keys %{$this->{Fields}}) {
			print "\n$padded(Fields $fld of ",dump_name($this),")\n";

			if (ref $this->{Fields}{$fld} eq "ARRAY") {
				# first level of an array...

				foreach (@{$this->{Fields}{$fld}}) { 
					if (ref $_ ne "") { 
						if (ref $_ eq "ARRAY") {
							# two dimensional array

							my $ae;
							foreach $ae (@{$_}) {
								if (ref $ae ne "") {
									$ae->dump(level+1);
								} else {
									print "$padded(DATA) $ae\n";
								}
							}
						} else {
							$_->dump($level+1);
						}
					} else {
						print "$padded(DATA) $_\n";
					}
				}
			} else {
				if (ref $this->{Fields}{$fld} ne "") {
					$this->{Fields}{$fld}->dump($level+1);
				} else {
					print "$padded(DATA) $_\n";
				}
			}
		}
	}
}


sub new {
	my($type, $scene, $ntype, $fields,$eventmodel) = @_;
	print "new Node (sub new): $ntype\n" if $VRML::verbose::nodec;
	my %rf;
	my $this = bless {
		TypeName => $ntype,
		Fields => $fields,
		EventModel => $eventmodel,
		Scene => $scene,
	}, $type;
	tie %rf, VRML::FieldHash, $this;
	$this->{RFields} = \%rf;
	my $t;
	if(!defined ($t = $VRML::Nodes{$this->{TypeName}})) {
		# PROTO
		$this->{IsProto} = 1;
		$this->{Type} = $scene->get_proto($this->{TypeName});
		print "new Node ", VRML::Node::dump_name($this), 
			"of type $ntype is a proto, type is ",VRML::Node::dump_name($this->{Type}),"\n"
			 if $VRML::verbose::nodec;
	} else {
		# REGULAR
		$this->{Type} = $t;
		print "new Node of type $ntype is regular, type is ",$this->{Type},"\n"
			 if $VRML::verbose::nodec;
	}
	$this->do_defaults();
	return $this;
}

# Construct a new Script node -- the Type argument is different
# and there is no way of this being a proto.
sub new_script {
	my($type, $scene, $stype, $fields, $eventmodel) = @_;
	print "new Node (new_script): $stype->{Name}\n" if $VRML::verbose::nodec;
	my %rf;
	my $this = bless {
		TypeName => $stype->{Name},
		Type => $stype,
		Fields => $fields,
		EventModel => $eventmodel,
		Scene => $scene,
	}, $type;
	tie %rf, VRML::FieldHash, $this;
	$this->{RFields} = \%rf;
	$this->do_defaults();
	return $this;
}

# Fill in nonexisting field values by the default values.
# XXX Maybe should copy?
sub do_defaults {
	my($this) = @_;
	# print "do_defaults  - this is $this\n";

	for(keys %{$this->{Type}{Defaults}}) {
		# print "  $_\n";
		if(!exists $this->{Fields}{$_}) {
			$this->{Fields}{$_} = $this->{Type}{Defaults}{$_};
			# print "      (created!)\n";
		}
	}
}

sub as_string {
	my($this) = @_;
	my $s = " ($this) $this->{TypeName} { \n";
	for(keys %{$this->{Fields}}) {
		$s .= "\n $_ ";
		if("VRML::IS" eq ref $this->{Fields}{$_}) {
			$s .= $this->{Fields}{$_}->as_string();
		} else {
			$s .= "VRML::Field::$this->{Type}{FieldTypes}{$_}"->
				as_string($this->{Fields}{$_});
		}
	}
	$s .= "}\n";
	return $s;
}

# If this is a PROTO expansion, return the actual "physical" VRML
# node under it.
sub real_node {
	my($this, $proto) = @_;
	# print "Scene.pm - real node in PROTO, $this, $proto\n";

	if(!$proto and defined $this->{IsProto}) {
		# print "Scene.pm - PROTO real node returning the proto expansion...\n";
		# print "	{protoExp}{Nodes}[0] is ", $this->{ProtoExp}{Nodes}[0],"\n";
		# print "	{protoExp}{Nodes}[1] is ", $this->{ProtoExp}{Nodes}[1],"\n";
		# print "	{protoExp}{Nodes}[2] is ", $this->{ProtoExp}{Nodes}[2],"\n";
	
		VRML::Handles::front_end_child_reserve ($this->{ProtoExp}{Nodes}[0]->real_node(),$this);	
		return $this->{ProtoExp}{Nodes}[0]->real_node;
	} else {
		# print "Scene.pm - PROTO real node returning just $this\n";
		return $this;
	}
}

# Return the initial events returned by this node.
sub get_firstevent {
	my($this,$timestamp) = @_;
	print "GFE $this $this->{TypeName} $timestamp\n" if $VRML::verbose;
	if($this->{Type}{Actions}{ClockTick}) {
		print "ACT!\n" if $VRML::verbose;
		my @ev = &{$this->{Type}{Actions}{ClockTick}}($this, $this->{RFields},
			$timestamp);
		for(@ev) {
			$this->{Fields}{$_->[1]} = $_->[2];
		}
		return @ev;
	}
	return ();
}

sub receive_event {
	my($this,$field,$value,$timestamp) = @_;

	if(!exists $this->{Fields}{$field}) {
		die("Invalid event received: $this->{TypeName} $field")
		unless($field =~ s/^set_// and
		       exists($this->{Fields}{$field})) ;
	}
	print "REC $this $this->{TypeName} $field $timestamp $value : ",
		("ARRAY" eq ref $value? (join ', ',@$value):$value),"\n" 
		if $VRML::verbose::events;
	$this->{RFields}{$field} = $value;

	if($this->{Type}{Actions}{$field}) {
		print "RACT!\n" if $VRML::verbose::events;
		my @ev = &{$this->{Type}{Actions}{$field}}($this,$this->{RFields},
			$value,$timestamp);
		for(@ev) {
			$this->{Fields}{$_->[1]} = $_->[2];
		}
		return @ev;
	}  elsif($this->{Type}{Actions}{__any__}) {
		my @ev = &{$this->{Type}{Actions}{__any__}}(
			$this,
			$this->{RFields},
			$value,
			$timestamp,
			$field,
		);
		# XXXX!!!???
		for(@ev) {
			$this->{Fields}{$_->[1]} = $_->[2];
		}
		return @ev;
	} elsif ($VRML::Nodes::bindable{$this->{TypeName}} 
			and $field eq "set_bind") {
		my $scene = $this->get_global_scene();
		$scene->set_bind($this, $value, $timestamp);
		return ();
	} else {
		# Ignore event
	}
}

# Get the outermost scene we are in
sub get_global_scene {
	my($this) = @_;
	return $this->{Scene}->get_scene();
}

sub events_processed {
	my($this,$timestamp,$be) = @_;
	print "EP: $this $this->{TypeName} $timestamp $be\n" if $VRML::verbose;
	if($this->{Type}{Actions}{EventsProcessed}) {
		print "PACT!\n" if $VRML::verbose;
		return &{$this->{Type}{Actions}{EventsProcessed}}($this, 
			$this->{RFields},
			$timestamp);
	}

	# JAS - this seemed to be a spurrious call - this simply resets ALL
	# fields.
	# if($this->{BackNode}) {
	# 	$this->set_backend_fields();
	# }
}

# Copy a deeper struct
sub ccopy {
	my($v,$scene) = @_;
	if(!ref $v) { return $v }
	elsif("ARRAY" eq ref $v) { return [map {ccopy($_,$scene)} @$v] }
	else { return $v->copy($scene) }
}

# Copy me
sub copy {
	my($this, $scene) = @_;
	my $new = {};
	$new->{Type} = $this->{Type};
	$new->{TypeName} = $this->{TypeName};
	$new->{EventModel} = $this->{EventModel} ;
	my %rf;
	if (defined $this->{IsProto}) {$new->{IsProto} = $this->{IsProto};};

	tie %rf, VRML::FieldHash, $new;
	$new->{RFields} = \%rf;
	for(keys %{$this->{Fields}}) {
		my $v = $this->{Fields}{$_};
		$new->{Fields}{$_} = ccopy($v,$scene);
	}
	$new->{Scene} = $scene;
	return bless $new,ref $this;
}

sub iterate_nodes {
	my($this, $sub,$parent) = @_;
	# print "iterate_nodes in package VRML::Node, sub is $sub \n";
	&$sub($this,$parent);
	# print "after sub call in ITERATE_NODES 2\n";

	for(keys %{$this->{Fields}}) {
		if($this->{Type}{FieldTypes}{$_} =~ /SFNode$/) {
			print "FIELDI: SFNode\n" if $VRML::verbose::scene;
			$this->{Fields}{$_}->iterate_nodes($sub,$this);
		} elsif($this->{Type}{FieldTypes}{$_} =~ /MFNode$/) {
			print "FIELDT: MFNode\n" if $VRML::verbose::scene;
			my $ref = $this->{RFields}{$_};
			for(@$ref) {
				print "ITERATE_NODES 3 going down... $_\n" if $VRML::verbose::scene;
				$_->iterate_nodes($sub,$this);
			}
		} else {
		}
	}
	# print "iterate_nodes in package VRML::Node, sub is $sub DONE\n";
}

sub make_executable {
	my($this,$scene) = @_;
	print "MKEXE $this->{TypeName}\n" if $VRML::verbose::scene;

	# loop through all the fields for this node type.

	for(keys %{$this->{Fields}}) {

		print "MKEXE - key $_\n" if $VRML::verbose::scene;
		# print "    MKEXE - key $_ ", ref $this->{Fields}{$_}, "\n";

		# First, get ISes values
		if( ref $this->{Fields}{$_} eq "VRML::IS" ) {
			print "MKEXE - its an IS!!!\n" if $VRML::verbose::scene;
			my $n = $this->{Fields}{$_}->name;
			$this->{Fields}{$_} =
				$scene->make_is($this, $_, $n);
		} 
		# Then, make the elements executable. Note that 
		# we do two things; the first is for non-arrays, the
		# second for arrays.

		if(ref $this->{Fields}{$_} and
		   "ARRAY" ne ref $this->{Fields}{$_}) {
			print "EFIELDT: SFReference\n" if $VRML::verbose::scene;
			$this->{Fields}{$_}->make_executable($scene,
				$this, $_);

		} elsif( $this->{Type}{FieldTypes}{$_} =~ /^MF/) {
			print "EFIELDT: MF\n" if $VRML::verbose::scene;
			my $ref = $this->{RFields}{$_};
			for (@$ref)
			 {
				# print "EFIELDT, field $_, ",ref $_,"\n";
			 	$_->make_executable($scene)
				 if(ref $_ and "ARRAY" ne ref $_);
			 } 

		} else {
			print "MKEXE - dont do anything\n" if $VRML::verbose::scene;
			# Nada
		}
	}


	# now, is this a PROTO, and is it not expanded yet?

	if(defined $this->{IsProto} && !$this->{ProtoExp}) {
		 # print "MAKE_EXECUTABLE_PROTOEXP $this, $this->{TypeName},
		 #	$this->{Type}\n";
		print "COPYING $this->{Type} $this->{TypeName}\n"
			if $VRML::verbose::scene;

		$this->{ProtoExp} = $this->{Type}->get_copy();
		$this->{ProtoExp}->set_parentnode($this,$scene);
		$this->{ProtoExp}->make_executable();

		#  print "MAKE_EXECUTABLE_PROTOEXP finish $this, $this->{TypeName},
		# 		$this->{Type}, $this->{ProtoExp}\n";

		my $ptr = $this->{ProtoExp};
                # print " lets see, the newp definition of $ptr is: \n";
                # print "         parent:", $ptr->{Parent},"\n";
                # print "         Name:", $ptr->{Name},"\n";
                # print "         Parameters: ";
                # for (keys %{$ptr->{Pars}}) {
                #   print " $_";
               #  }
                # print "\n";
                # print "         FieldKinds: ";
                # for (keys %{$ptr->{FieldKinds}}) {
                #   print ", $_ ", $ptr->{FieldKinds}{$_};
                # }
                # print "\nMAKE_EXECUTABLE_PROTOEXP returning\n";


	} 

	print "END MKEXE $this->{TypeName}\n"
		if $VRML::verbose::scene;
}

sub initialize {
	my($this,$scene) = @_;
	# Inline is initialized at make_backend

	if($this->{Type}{Actions}{Initialize}
	 && $this->{TypeName} ne "Inline") {
		return &{$this->{Type}{Actions}{Initialize}}($this,$this->{RFields},
			(my $timestamp=XXX), $this->{Scene});
		# XXX $this->{Scene} && $scene ??
	}
	return ()
}

sub set_backend_fields {
	my($this, @fields) = @_;
	my $be = $this->{BackEnd};
	# print "Scene.pm: set_backend_fields for this $this, backend $be\n";

	if(!@fields) {@fields = keys %{$this->{Fields}}}
	my %f;
	for(@fields) {
		# print "Scene.pm - iterating field $_\n";
		my $v = $this->{RFields}{$_};
		print "SBEF: $this $_ '",("ARRAY" eq ref $v ?
			(join ' ,',@$v) : $v),"' \n" if 
				$VRML::verbose::be && $_ ne "__data";
		if($this->{Type}{FieldTypes}{$_} =~ /SFNode$/) {
			print "SBEF: SFNODE\n" if $VRML::verbose::be;
			$f{$_} = $v->make_backend($be);
		} elsif($this->{Type}{FieldTypes}{$_} =~ /MFNode$/) {
			print "SBEF: MFNODE @$v\n" if $VRML::verbose::be;
			$f{$_} = [
				map {$_->make_backend($be)} @{$v}
			];
			print "MFNODE GOT $_: @{$f{$_}}\n" if $VRML::verbose::be;

		} else {
			print "SBEF: Not MF or SF Node...\n"  if $VRML::verbose::be;
			$f{$_} = $v;
		}
	}
	# print "Scene.pm: calling set_fields\n";
	$be->set_fields($this->{BackNode},\%f);
}


{
my %NOT = map {($_=>1)} qw/WorldInfo TimeSensor TouchSensor
	ScalarInterpolator ColorInterpolator
	PositionInterpolator
	OrientationInterpolator
	CoordinateInterpolator
	PlaneSensor
	SphereSensor
	CylinderSensor
	VisibilitySensor
	/;
#JAS	Collision #JAS
#	NavigationInfo


#############################################################
#
# NODE::make_backend
#
#############################################################

sub make_backend {
	my($this,$be,$parentbe) = @_;
	
	print "Node::make_backend $this, $this->{TypeName}, $be, $parentbe\n"
		 if $VRML::verbose::be;

	if(defined $this->{BackNode}) {
		return $this->{BackNode};
	}

	if($this->{TypeName} eq "Inline") {
		print "NODE: Inline\n"
			if $VRML::verbose::be;
		&{$this->{Type}{Actions}{Initialize}}($this,$this->{RFields},
			(my $timestamp=XXX), $this->{Scene});
	}
	if($NOT{$this->{TypeName}} or $this->{TypeName} =~ /^__script/) {
		print "NODE: makebe NOT - ", $this->{TypeName},"\n"
			if $VRML::verbose::be;
		return ();
	}
	if(defined $this->{IsProto}) {
		print "NODE: makebe PROTO\n"
			 if $VRML::verbose::be;
		return $this->{ProtoExp}->make_backend($be,$parentbe);
	}
	my $ben = $be->new_node($this->{TypeName});
	$this->{BackNode} = $ben;
	$this->{BackEnd} = $be;
	$this->set_backend_fields();

	# was this a viewpoint? Was it not in a proto definition?
	if ($this->{TypeName} eq "Viewpoint") {
        	# print "Node::register_vp, viewpoint is ",$this->{Fields}{description},"\n";
		if ($this->{BackEnd}) {
			# print " should register this one!\n";
			my $scene = $this->{Scene};
			VRML::NodeType::register_vp($scene, $this);
		}
                 # print "ref t ", ref $this,"\n";
                 # print "ref t backend ", ref $this->{BackEnd},"\n";
                 # print "t backend ", $this->{BackEnd},"\n";
                 # print "ref t backNode ", ref $this->{BackNode},"\n";
                 # print "t backNode ", $this->{BackNode},"\n";
                 # print "ref t protoexp ", ref $this->{ProtoExp},"\n";
                 # print "t protoexp ", $this->{ProtoExp},"\n";

	}
	return $ben;
}
}

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
	my ($this,$level) = @_;

	if ($level > 20) {
		print "level too deep, returning\n";
		return();
	}
	my $lp = $level*2+2;
	my $padded = pack("A$lp","$level ");


	# print out the Scene graph, to help in debugging.
	print $padded,"SCENE DUMP ",VRML::Node::dump_name($this),"\t",$this->{URL},"\n\n";
	foreach (keys %{$this}) {
		print $padded,"$_\t";
		
		# lets do something special for Routes
		if ("Routes" eq $_) {
       		 for (@{$this->{$_}}) {
                	my($fnam, $ff, $tnam, $tf) = @$_;
                	print "$padded    Route from $fnam field $ff to $tnam field $tf\n";
        	 }
					
		} elsif ("ARRAY" eq ref $this->{$_}) {
			print "(array) ";
			foreach (@{$this->{$_}}) {
				print VRML::Node::dump_name($_);
				print " ";
			}
		} elsif ("HASH" eq ref $this->{$_}) {
			print "(hash) ";
			foreach (keys %{ $this->{$_}}) {
			  print "$_ ";
			}
		} else {
		 	print VRML::Node::dump_name($this->{$_});
		}
		print "\n";
	}

	# this was Nodes, but RootNode should contain all.
	if (defined $this->{RootNode}) {
		print "\n$padded(RootNode of ",VRML::Node::dump_name($this),")\n";
 		$this->{RootNode}->dump($level+1);
	}
	
	if (defined $this->{SubScenes}) {
		print "\n$padded(SubScenes of ",VRML::Node::dump_name($this),")\n";
		my $sk;
		foreach $sk (@{{$this}->{SubScenes}}) {
			print "SUBSCENE of ",VRML::Node::dump_name($this);
			$sk->dump($level+1);
		}
	}
	
	print "$level END OF SCENE DUMP for ",VRML::Node::dump_name($this),"\n\n";
}
		



sub new {
	my($type,$eventmodel,$url) = @_;
	my $this = bless {
		EventModel => $eventmodel,
		URL => $url,
	},$type;
	print "Newscene $this, $eventmodel, $url\n" 
		if $VRML::verbose::scene;
	return $this;
	
}

# is this node a new "replaceWorld'd" node, and does it need to be put on the stack?
sub replaceWorld_Bindable {
	my ($this, $node) = @_;

        # Check if it is bindable and first -> bind to it later..

	# print "Scene.pm:replaceWorld_Bindable, looking at node $node, type ",
	#	$node->{TypeName}," type node ", $node->{Type},"\n";

        if($VRML::Nodes::bindable{$node->{TypeName}}) {
		# this should never happen...
                if(!defined $this->{Bindable}{$node->{TypeName}}) {
                        $this->{Bindable}{$node->{TypeName}} = $node;
                }
		# print "Scene.pm:replaceWorld_Bindable, found one! $node\n";
                push @{$this->{Bindables}{$node->{TypeName}}}, $node;
        }
}


	
#JAS sub register_vps {
#JAS 	my ($this, $browser) = @_;
#JAS 
    #JAS     my $np = $this->{Bindables}{Viewpoint};
#JAS 
#JAS 	my $c = 0;
#JAS 	while ($c <= $#$np) {
    #JAS     	print "register_vps for $this ", $np->[$c]->real_node()," viewpoint $c is ",$np->[$c]{Fields}{description},"\n";
#JAS 		$browser->register_vp($this,$np->[$c]->real_node());
#JAS 		$c++;
#JAS 	}
#JAS }


sub set_url {
	$_[0]{URL} = $_[1];
}

sub newp {
	# newp - ...
	# actually creates a new prototype.
        #
	# parameters:
	
	# type:	 probably always VRML::Scene
        # pars:	 not yet decoded. prints as HASH(0x83e45f0)
	#        look below for its usage in extracting fields.
        # parent:which invocationo of scene this is in.  SCENE_2
        # name:	 VRML name, eg, dirigible



	my ($type,$pars,$parent,$name) = @_;

	# print "Scene:newp: \n	type $type \n	pars $pars \n	parent $parent \n	name $name\n";
	my $this = $type->new;
	$this->{Pars} = $pars;
	$this->{Name} = $name;
	$this->{Parent} = $parent;

	# Extract the field types
	$this->{FieldTypes} = {map {$_ => $this->{Pars}{$_}[1]} keys %{$this->{Pars}}};
	$this->{FieldKinds} = {map {$_ => $this->{Pars}{$_}[0]} keys %{$this->{Pars}}};

	$this->{EventModel} = $parent->{EventModel};
	$this->{Defaults} = {map {$_ => $this->{Pars}{$_}[2]} keys %{$this->{Pars}}};
	for(keys %{$this->{FieldKinds}}) {
		my $k = $this->{FieldKinds}{$_};
		if($k eq "exposedField") {
			$this->{EventOuts}{$_} = $_;
			$this->{EventOuts}{$_."_changed"} = $_;
			$this->{EventIns}{$_} = $_;
			$this->{EventIns}{"set_".$_} = $_;
		} elsif($k eq "eventIn") {
			$this->{EventIns}{$_} = $_;
		} elsif($k eq "eventOut") {
			$this->{EventOuts}{$_} = $_;
		} elsif($k ne "field") {
			print "Truly strange - shouldn't happen\n"; 
			exit (1);
		}
	}
	# print "Scene:newp finished, returning $this\n";
	return $this;
}

sub newextp {
	my ($type,$pars,$parent,$name,$url) = @_;
	# XXX marijn: code copied from newp()

	my $this = $type->new;
	$this->{Pars} = $pars;
	$this->{Name} = $name;
	$this->{Parent} = $parent;

	# Extract the field types
	$this->{FieldTypes} = {map {$_ => $this->{Pars}{$_}[1]} keys %{$this->{Pars}}};
	$this->{FieldKinds} = {map {$_ => $this->{Pars}{$_}[0]} keys %{$this->{Pars}}};
	#print "Scene:newextp: this parent is $parent\n";
	$this->{EventModel} = $parent->{EventModel};
	for(keys %{$this->{FieldKinds}}) {
		 my $k = $this->{FieldKinds}{$_};
		 if($k eq "exposedField") {
			 $this->{EventOuts}{$_} = $_;
			 $this->{EventOuts}{$_."_changed"} = $_;
			 $this->{EventIns}{$_} = $_;
			 $this->{EventIns}{"set_".$_} = $_;
		 } elsif($k eq "eventIn") {
			 $this->{EventIns}{$_} = $_;
		 } elsif($k eq "eventOut") {
			 $this->{EventOuts}{$_} = $_;
		 } elsif($k ne "field") {
			 print "Truly strange - shouldn't happen\n";
			 exit (1);
		 }
	}
	# XXX marijn: fix this: only first url currently used
	if(ref $url) {
		 $url = $url->[0];
	}
	print("EXTERNPROTO with URL: $url\n") if $VRML::verbose::parse;
	my ($protourl,$protoname) = split(/\#/,$url,2);
	# marijn: set the url for this proto
	$this->set_url($protourl);

	# XXX marijn: code copied from Browser->load_file()
	my $string = VRML::URL::get_relative($parent->{URL},$protourl);

	# Required due to changes in VRML::URL::get_relative in URL.pm:
	if (!$string) { die "File $protourl was not found"; }

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

	unless($string =~ /^#VRML V2.0/s) {
		 if($string =~ /^#VRML V1.0/) {
			 print "Sorry, this file is according to VRML V1.0, I only know V2.0\n";
			 exit(1);
                }

		 warn("WARNING: file '$protourl' doesn't start with the '#VRML V2.0' header line");
	}

	# XXX marijn: code copied from Browser->parse()
	my $po = pos $string;
	while($string =~ /([#\"])/gsc ) {
		 (pos $string)--;
		 if( $1 eq "#" ) {
			 $string =~ s/#.*$//m;
		 } else {
			 VRML::Field::SFString->parse($this,$string);
		 }
	}
	(pos $string) = $po;

	# marijn: end of copying, now locate right PROTO
	my $succes = 0;
	while($string =~ /[\s,^](PROTO\s+)($VRML::Parser::Word)/gsc ) {
		 if ( !$protoname ) {
			 $protoname = $2;
		 }
		 if ( $2 eq $protoname ) {
			 (pos $string) -= ((length $1) + (length $2));
			 VRML::Parser::parse_statement($this, $string);
			 $succes = 1;
			 last;
		 }
	}
	if(!$succes) {
		 VRML::Error::parsefail("no PROTO found", "$url");
	}

	# marijn: now create an instance of the PROTO, with all fields ISsed.
	# XXX marijn: What about FieldTypes/FieldKinds?
	my %fields = map {$_ => $this->new_is($_)} keys %{$this->{Pars}};
	my $n = $this->new_node($protoname,\%fields);
	my @node = ($n);

	# XXX marijn: code copied from Parser::parse_proto
	$this->topnodes(\@node);

	# marijn: copy defaults from PROTO
	$this->{Defaults} = {map {$_ => $this->{Protos}{$protoname}->{Defaults}{$_}} keys %{$this->{Protos}{$protoname}->{Defaults}}};

	return $this;
}

##################################################################
#
# This is the public API for use of the parser or whoever..

{my $cnt;
sub new_node {
	my($this, $type, $fields) = @_;
	if($type eq "Script") {
		# Special handling for Script which has an interface.
		my $t = "__script__".$cnt++;
		my %f = 
		(url => [MFString, []],
		 directOutput => [SFBool, 0, ""], # not exposedfields
		 mustEvaluate => [SFBool, 0, ""]);
		for(keys %$fields) {
			$f{$_} = [
				$fields->{$_}[1],
				$fields->{$_}[2],
				$fields->{$_}[0],
			];
		}
		my $type = VRML::NodeType->new($t,\%f,
			$VRML::Nodes{Script}{Actions});
		my $node = VRML::Node->new_script(
			$this, $type, {}, $this->{EventModel});
		return $node;
	}
	my $node = VRML::Node->new($this,$type,$fields, $this->{EventModel});

	# Check if it is bindable and first -> bind to it later..
	if($VRML::Nodes::bindable{$type}) {
		if(!defined $this->{Bindable}{$type}) {
			$this->{Bindable}{$type} = $node;
		}
		push @{$this->{Bindables}{$type}}, $node;
	}
	return $node;
}
}

sub new_route {
	my $this = shift;
	print "NEW_ROUTE $_[0][0] $_[0][1] $_[0][2] $_[0][3], this is $this\n" 
		if $VRML::verbose::scene;
	push @{$this->{Routes}}, $_[0];
}

sub delete_route {
	my $this = shift;
	print "DELETE_ROUTE $_[0][0] $_[0][1] $_[0][2] $_[0][3], this is $this\n" 
		if $VRML::verbose::scene;

	# first, take it off the known routes for this node.
	pop @{$this->{Routes}}, $_[0];

	# then make sure that the eventmodel knows that this route no longer exists
	push @{$this->{DelRoutes}}, $_[0];

}

sub new_def {
	my($this,$name,$node) = @_;

	print "NEW DEF ", VRML::Node::dump_name($this),
		 " $name ", VRML::Node::dump_name($node),"\n" if $VRML::verbose::scene;
	my $def = VRML::DEF->new($name,$node);
	$this->{TmpDef}{$name} = $def;
	# print "NEW DEF IS ",VRML::Node::dump_name($def),"\n"; 
        VRML::Handles::reserve($def);
	return $def;
}

sub new_use {
	my($this,$name) = @_;
	return VRML::USE->new($name, VRML::Handles::get($this->{TmpDef}{$name}));
}

sub new_is {
	my($this, $name) = @_;
	return VRML::IS->new($name);
}

sub new_proto {
	my($this,$name,$pars) = @_;
	#print "Scene:new_proto, \n	this $this \n	name $name \n	pars $pars\n";
	# for(keys %{$pars}) {
        #       print "parameter $_\n";
        #}

	print "NEW_PROTO $this $name\n" if $VRML::verbose::scene;
	my $p = $this->{Protos}{$name} = (ref $this)->newp($pars,$this,$name);
	print "Scene:new_proto, returning $p \n" if $VRML::verbose::scene;
	return $p;
}

sub new_externproto {
	my($this,$name,$pars,$url) = @_;
	print "NEW_EXTERNPROTO $this $name\n" if $VRML::verbose::scene;
	my $p = $this->{Protos}{$name} = (ref $this)->newextp($pars,$this,$name,$url);
	return $p;
}

sub topnodes {
	my($this,$nodes) = @_;
	$this->{Nodes} = $nodes;
	$this->{RootNode} = $this->new_node("Group",{children => $nodes});
}

sub get_proto {
	my($this,$name) = @_;
	print "GET_PROTO $this $name\n" if $VRML::verbose::scene;
	if($this->{Protos}{$name}) {return $this->{Protos}{$name}}
	# print "Scene:GET_PROTO: \n	this $this\n	parent ", $this->{Parent},"\n";
	if($this->{Parent}) {return $this->{Parent}->get_proto($name)}
	print "GET_PROTO_UNDEF $this $name\n" if $VRML::verbose::scene;
	return undef;
}

sub get_url {
	my($this) = @_;
	print "Get_url $this\n" if $VRML::verbose::scene;
	if(defined $this->{URL}) {return $this->{URL}}
	# print "Scene:get_url: this parent is ", $this->{Parent},"\n";
	if($this->{Parent}) {return $this->{Parent}->get_url()}
	print "Undefined URL tree\n";
	exit (1);
}

sub get_scene {
	my($this) = @_;
	# print "Scene:get_scene: this $this \n	parent ", $this->{Parent},"\n";
	if($this->{Parent}) {return $this->{Parent}->get_scene()}
	return $this;
}

sub set_browser { $_[0]{Browser} = $_[1]; }

sub get_browser {
	my($this) =@_;
	if($this->{Parent}) {return $this->{Parent}->get_browser()}
	return $this->{Browser};
}

########################################################
#
# Private functions again.


sub get_as_mfnode {
	return VRML::Handles::reserve($_[0]{Nodes});
}

sub mkbe_and_array {
  # lets return an array of nodes that make up this scene...

  # lets get the number of items in there...

  my ($this,$be,$parentscene) = @_;


  # copy over any generic scene DEFs. Proto specific DEFS below
  # note that the DEFS are part of a Scene object. Routes are attached
  # to a node.
  # for (keys %{$this->{Scene}{DEF}}) { print "mkbe_and_array, going to copy over $_\n"; }

  if (defined $this->{Scene}{DEF}) {
	%{$parentscene->{DEF}} = (%{$parentscene->{DEF}},%{$this->{Scene}{DEF}});
  }


  # for (keys %{$parentscene->{DEF}}) { print "mkbe_and_array, parentscene has  $_\n"; }


  # Now, should we return the "RootNode" if it exists (which is a Group node)
  # or, a list of the "Node"s?
  # lets try returning the RootNode, as this way we always have a group to
  # add in the case of a proto. 

  my $c;
  my $q = "";
  my $lastindex = $#{$_[0]{Nodes}};
  # print "mkbe_and_array, Scene $this, parent $parentscene number of nodes: $lastindex\n";
  my $curindex = 0;
  while ($curindex <= $lastindex) {
  	  # PROTOS screw us up; the following line worked, but then
  	  # PROTO information was not available. So, store the PROTO
  	  # node definition, BUT generate the real node!

  	  $c = $_[0]{Nodes}[$curindex];
  	  if("ARRAY" eq ref $c) {
		$c = @{$c};
	  } elsif ("VRML::DEF" eq ref ($c)) {
	 	# print "mkbe - this is a def!\n";
	      $c = $c->real_node();
	  }

	  # print "	mkbe_and_array; index $curindex, c $c, ref ", ref $c,"\n";
	  # lets make backend here, while we are having fun...
	  if (!defined $c->{BackNode}) {
		my $rn = c;
		if(defined $c->{IsProto}) {
			$rn = $c->real_node();
	       		$rn->{BackNode} = VRML::Node::make_backend($rn,$be);
	    		VRML::Handles::reserve($rn);
		} else {
	       		$c->{BackNode} = VRML::Node::make_backend($c,$be);
		}
	  }
	my $id = VRML::Handles::reserve($c);

	$q = "$q $id";
	$curindex++;
  }

  # any Routes in there??? PROTO Routes handled below.
  if (defined $this->{Routes}) {
	    $c->{SceneRoutes} = $this->{Routes};
  }


  # gather any DEFS. Protos will have new DEF nodes (expanded nodes)
  if (defined $this->{Nodes}) {
	# print "\nmkbe_and_array: Gathering defs for Nodes of ",VRML::Node::dump_name($this),")\n";
	foreach (@{$this->{Nodes}}) {
		$_->gather_defs($parentscene);
	}
  }


  # now, go through the protos, and copy over active routes
  # go through any PROTOS associated with this. NOTE: only go through
  # one level - nested protos *may* give trouble

  if (defined $this->{Protos}) {
	my $k;
	foreach $k (keys %{$this->{Protos}}) {
		# PROTO Routes
		for (@{$this->{Protos}{$k}{Routes}}) {
			push (@{$c->{SceneRoutes}}, $_);	
                	my($fnam, $ff, $tnam, $tf) = @$_;
                	# print "Scene.pm - routes in $k are from $fnam field $ff to $tnam field $tf\n";
        	}
  	}
  }


  # need to sanitize ... it probably has trailing newline...
  $q =~ s/^\s+//;
  $q =~ s/\s+$//;
  # print "Scene.pm:mkbe_and_array: $q\n";
  return $q
}


sub getNode {
	my $n = $_[0]{TmpDef}{VRML::Handles::return_def_name($_[1])};
	if(!defined $n) {
		print "Node '$_[1]' not defined\n";
		return "undefined";
	}
	return $n->real_node(1); # Return proto enclosing node.
}

sub as_string {
	my($this) = @_;
print "Scene::as_string, nodes are ", @{$this->{Nodes}},"\n";
	join "\n ($this) ",map {$_->as_string} @{$this->{Nodes}};
}

# Construct a full copy of this scene -- used for protos.
# Note: much data is shared - problems?
sub get_copy {
	my($this,$name) = @_;
	my $new = bless {
	},ref $this;
	if(defined $this->{URL}) {
		$new->{URL} = $this->{URL};
	}
	$new->{Pars} = $this->{Pars};
	$new->{FieldTypes} = $this->{FieldTypes};
	$new->{Nodes} = [map {$_->copy($new)} @{$this->{Nodes}}];
	$new->{EventModel} = $this->{EventModel};
	if (defined $this->{Routes}) {
		$new->{Routes} = $this->{Routes};
	 # print "Scene.pm: GET_COPY: ROUTE ", 
	 #	$new->{Routes}[0][0] , " ", 
	 #	$new->{Routes}[0][1] , " ",
	 #	$new->{Routes}[0][2] , " ",
	 #	$new->{Routes}[0][3] , " from $this to new scene: $new\n";
	}

	return $new;
}

sub make_is {
	my($this, $node, $field, $is) = @_;
	my $retval;
	print "Make_is ",VRML::Node::dump_name($this), " ",
		VRML::Node::dump_name( $node), " $node->{TypeName} $field $is\n"
		 if $VRML::verbose::scene;
	my $pk = $this->{NodeParent}{Type}{FieldKinds}{$is} or
		die("IS node problem") ;
	my $ck = $node->{Type}{FieldKinds}{$field} or
		die("IS node problem 2");
	if($pk ne $ck and $ck ne "exposedField") {
		print "INCOMPATIBLE PROTO TYPES (XXX FIXME Error message)!\n";
		exit (1);
	}
	# If child's a field or exposedField, get initial value
	print "CK: $ck, PK: $pk\n" 
		 if $VRML::verbose::scene;
	if($ck =~ /[fF]ield$/ and $pk =~ /[fF]ield$/) {
		print "SETV: $_ NP : '$this->{NodeParent}' '$this->{NodeParent}{Fields}{$_}'\n" 
			if $VRML::verbose::scene;
		$retval=
		 "VRML::Field::$node->{Type}{FieldTypes}{$field}"->copy(
		 	$this->{NodeParent}{Fields}{$is}
		 );
	} else {
		$retval = $node->{Type}{Defaults}{$_};
	}
	# For eventIn, eventOut or exposedField, store for route
	# building.
	# XXX - JAS - is this code ever run? Can't find an example
	# where the array is looked at. So, we use a global array
	# within the Browser to keep track of things

	if($ck ne "field" and $pk ne "field") {
		if($pk eq "eventIn" or ($pk eq "exposedField" and
			$ck eq "exposedField")) {
			push @{$this->{IS_ALIAS_IN}{$is}}, [$node, $field];
		}
		if($pk eq "eventOut" or ($pk eq "exposedField" and
			$ck eq "exposedField")) {
			push @{$this->{IS_ALIAS_OUT}{$is}}, [$node, $field];
		}
	}
	return $retval;
}

#
# Here come the expansions:
#  - make executable: create copies of all prototypes.
#

sub iterate_nodes {
	print "ITERATE NODES - proto expansion\n" if $VRML::verbose::scene;
	my($this,$sub,$parent) = @_;
	# print "iterate_nodes in package proto expansion , sub is $sub\n";
	# for(@{$this->{Nodes}}) {
	if($this->{RootNode}) {
		for($this->{RootNode}) {
			$_->iterate_nodes($sub,$parent);
		}
	} else {
		for(@{$this->{Nodes}}) {
			$_->iterate_nodes($sub,$parent);
		}
	}
	# print "iterate_nodes in package proto expansion , sub is $sub DONE\n";
}

sub iterate_nodes_all {
	my($this,$subfoo) = @_;
	# print "iterate_nodes in package SCENE::IS (all), sub is $subfoo \n";
	# for(@{$this->{Nodes}}) {
	my @l; 
	if($this->{RootNode}) {@l = $this->{RootNode}}
	else {@l = @{$this->{Nodes}}}
	for(@l) {
		my $sub;
		$sub = sub {
			# print "ALLITER $_[0]\n";
			&$subfoo($_[0]);
			if(ref $_[0] eq "VRML::Node" and $_[0]->{ProtoExp}) {
				$_[0]->{ProtoExp}->iterate_nodes($sub);
			}
		};
		$_->iterate_nodes($sub);
		undef $sub;
	}
	# print "iterate_nodes in package SCENE::IS (all), sub is $subfoo DONE\n";
}

sub set_parentnode { 
	# NodeParent is used in IS's - 
	# Parent is used for tracing back up the tree.

	my ($this,$nodeparent,$parent) = @_;
	my $b = $this->get_browser();

	# print "Scene::set_parentnode\n	this $this\n	nodeparent $nodeparent\n	parent $parent\n	browser	$b\n";
	$this->{NodeParent} = $nodeparent; 
	$this->{Parent} = $parent;
}



# XXX This routine is too static - should be split and executed
# as the scene graph grows/shrinks.

# JAS XXX - I don't think that TouchSensors (maybe TimeSensors??) are 
# cought by this "sends" stuff....

{
my %sends = map {($_=>1)} qw/
	TouchSensor TimeSensor
/;
sub make_executable {
	my($this) = @_;


	# step 1) Go through all nodes in "this" scene. Make executable
	# on them. Hopefully, all nodes are of type VRML::Node!

	for(@{$this->{Nodes}}) {
		# print "in SCENE::make_executable, looking at ", ref $_, " scene $this\n";
		$_->make_executable($this);
	}


	# Step 2) Give all ISs references to my data
	if($this->{NodeParent}) {
		print "MAKEEXNOD\n" if $VRML::verbose::scene;
		$this->iterate_nodes(sub {
			print "MENID\n" if $VRML::verbose::scene;
			return unless ref $_[0] eq "VRML::Node";
			for(keys %{$_[0]->{Fields}}) {
				print "MENIDF $_\n" if $VRML::verbose::scene;
				next unless ((ref $_[0]{Fields}{$_}) eq "VRML::IS");
				print "MENIDFSET $_\n" if $VRML::verbose::scene;
				$_[0]{Fields}{$_}->set_ref(
				  \$this->{NodeParent}{Fields}{
				  	$_[0]{Fields}{$_}->name});
			}
		});
	}

	# step 3) Gather all 'DEF' statements
	my %DEF;
	$this->iterate_nodes(sub {
		return unless ref $_[0] eq "VRML::DEF";
		print "FOUND DEF ($this, $_[0]) ",$_[0]->name,"\n" if $VRML::verbose::scene;
		$DEF{$_[0]->name} = $_[0]->def;
	});

	# Step 4) Set all USEs
	$this->iterate_nodes(sub {
		return unless ref $_[0] eq "VRML::USE";
		print "FOUND USE ($this, $_[0]) ",$_[0]->name,"\n" if $VRML::verbose::scene;
		$_[0]->set_used($DEF{$_[0]->name});
	});
	$this->{DEF} = \%DEF;


	# Step 5) Collect all prototyped nodes from here
	# so we can call their events
	# JAS XXX I think that this is just BS, as Sensors, SubScenes are
	# not found anywhere else, and the ref is VRML::Scene in here...
	# Still, until I make sure that TouchSensors are ok from within
	# PROTOS...

	$this->iterate_nodes(sub {
		return unless ref $_[0] eq "VRML::Node";
		push @{$this->{SubScenes}}, $_[0]
		 	if $_[0]->{ProtoExp};
		push @{$this->{Sensors}}, $_[0] 
			if $sends{$_[0]};
	});
}
}

############################################################3
#
# SCENE::make_backend
#
############################################################


sub make_backend {
	my($this,$be,$parentbe) = @_;
	print "SCENE::make_backend $this $be $parentbe \n" 
		if $VRML::verbose::be;


	if($this->{BackNode}) {return $this->{BackNode}}

	my $bn;
	if($this->{Parent}) {

		# I am a proto -- only my first node renders anything...
		print "Scene: I'm a PROTO ",VRML::Node::dump_name($this)," $be $parentbe\n"
			if $VRML::verbose::be;
		$bn = $this->{Nodes}[0]->make_backend($be,$parentbe);
	} else {
		print "Scene: I'm not proto $this $be $parentbe ($this->{IsInline})\n"
			if $VRML::verbose::be;

		$bn = $this->{RootNode}->make_backend($be,$parentbe);

		$be->set_root($bn) unless $this->{IsInline};

 		$be->set_vp_sub(
 			sub {
 				my $b = $this->get_browser();
 				my $vn = $b->get_vp_node();
 				my $vs = $b->get_vp_scene();
 				if (!defined $vn) {return;}
 				$b->set_next_vp();
 				# print "vp_sub, $b $vn $vs";
 				print "GOING TO VP: '$vn->{Fields}{description}'\n";
 				$vs->{EventModel}->send_event_to(
 					$vn, set_bind, 1);
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
	my($this,$eventmodel,$be) = @_;

	print "SETUP_ROUTING $this $eventmodel $be\n" if $VRML::verbose::scene;

	$this->iterate_nodes(sub {
		print "ITNOREF: $_[0]\n" if $VRML::verbose::scene;
		return unless "VRML::Node" eq ref $_[0];
		print "ITNO: $_[0] $_[0]->{TypeName} ($VRML::Nodes::initevents{$_[0]->{TypeName}})\n" if $VRML::verbose::scene;
		if($VRML::Nodes::initevents{$_[0]->{TypeName}}) {
			print "ITNO:FIRST $_[0]\n" if $VRML::verbose::scene;
			$eventmodel->add_first($_[0]);
		} else {
			if($_[0]{ProtoExp}) {
				 $_[0]{ProtoExp}->setup_routing(
				 	$eventmodel,$be) ;
			}
		}
		# Look at child nodes
		my $c;
		# for(keys %{$_[0]{Fields}}) {
		# 	if("VRML::IS" eq ref $_[0]{Fields}{$_}) {
		# 		$eventmodel->add_is($this->{NodeParent},
		# 			$_[0]{Fields}{$_}->name,
		# 			$_[0],
		# 			$_
		# 		);
		# 	}
		# }
		if(($c = $VRML::Nodes::children{$_[0]->{TypeName}})) {
			my $ref = $_[0]{RFields}{$c};
			print "CHILDFIELD: GOT @$ref FOR CHILDREN\n"
				if $VRML::verbose::scene;
			for(@$ref) {
				# XXX Removing/moving sensors?!?!
				my $n = $_->real_node();
				print "REALNODE: $n $n->{TypeName}\n"
					if $VRML::verbose::scene;
				if($VRML::Nodes::siblingsensitive{$n->{TypeName}}) {
					print "Scene.pm:SES: $n $n->{TypeName}", 
						" bn ", $_[0],"\n" 
						if $VRML::verbose::scene;
					$be->set_sensitive(
						$_[0]->{BackNode},
						sub { $eventmodel->
							    handle_touched($n,
							    		@_);
						}
					);
				}
			}
		}
		if($VRML::Nodes::sensitive{$_[0]{TypeName}}) {
			$be->set_sensitive($_[0]->{BackNode},
				sub {},
			);
		}
	});
	print "DEFINED NODES in ", VRML::Node::dump_name($this),": ",
		(join ',',keys %{$this->{DEF}}),"\n" if $VRML::verbose::scene;

	# any routes to add?
	if (exists $this->{Routes}) {
	  for(@{$this->{Routes}}) {
		my($fn, $ff, $tn, $tf) = @$_;
		# print "Scene.pm - routes from $fn field $ff to $tn field $tf scene $this\n";
		
		# Now, map the DEF names into a node name. Note, EAI will send in
		# the node, ROUTES in one VRML file will use DEFs.

		if ("VRML::Node" ne ref $fn) {
			if (!exists $this->{DEF}{$fn}) {
				print "Routed node name '$fn' not found ($fn, $ff, $tn, $tf)\n";
				exit (1);
			}
			$fn = $this->{DEF}{$fn}
		# } else {
		# 	print "ROUTE: $fn must be from EAI\n";
		}

		if ("VRML::Node" ne ref $tn) {
			if (!exists $this->{DEF}{$tn}) {
				print "Routed node name '$tn' not found ($tn, $ff, $tn, $tf)\n";
				exit (1);
			}
			$tn = $this->{DEF}{$tn}
		# } else {
		# 	print "ROUTE: $tn must be from EAI\n";
		}
		$eventmodel->add_route($fn,$ff,$tn,$tf);
	    }
	}

	# Any routes to delete? Maybe from the EAI...
	if (exists $this->{DelRoutes}) {
	    for(@{$this->{DelRoutes}}) {
		my($fn, $ff, $tn, $tf) = @$_;
		print "Scene.pm - route remove from $fn field $ff to $tn field $tf scene $this\n";
		
		# Now, map the DEF names into a node name. Note, EAI will send in
		# the node, ROUTES in one VRML file will use DEFs.

		if ("VRML::Node" ne ref $fn) {
			if (!exists $this->{DEF}{$fn}) {
				print "Routed node name '$fn' not found ($fn, $ff, $tn, $tf)\n";
				exit (1);
			}
			$fn = $this->{DEF}{$fn}
		} else {
			print "ROUTE: $fn must be from EAI\n";
		}

		if ("VRML::Node" ne ref $tn) {
			if (!exists $this->{DEF}{$tn}) {
				print "Routed node name '$tn' not found ($tn, $ff, $tn, $tf)\n";
				exit (1);
			}
			$tn = $this->{DEF}{$tn}
		} else {
			print "ROUTE: $tn must be from EAI\n";
		}
			
		$eventmodel->delete_route($fn,$ff,$tn,$tf);
	    }
	    # ok, its processed; we don't want this route deleted from the events more than once
	    delete ($this->{DelRoutes});
	}

	for my $isn (keys %{$this->{IS_ALIAS_IN}}) {
		# print "setup_routing: first IS_ALIAS_IN loop\n";
		for(@{$this->{IS_ALIAS_IN}{$isn}}) {
			$eventmodel->add_is_in($this->{NodeParent},
				$isn, @$_);
		}
	}
	for my $isn (keys %{$this->{IS_ALIAS_OUT}}) {
		# print "setup_routing: first IS_ALIAS_OUT loop\n";
		for(@{$this->{IS_ALIAS_OUT}{$isn}}) {
			$eventmodel->add_is_out($this->{NodeParent},
				$isn, @$_);
		}
	}
	print "SETUP_ROUTING FINISHED $this $eventmodel $be\n" if $VRML::verbose::scene;
}


# Send initial events
sub init_routing {
	my($this,$eventmodel, $backend, $no_bind) = @_;
	# XXX no_bind not used - I initialize all subnodes...
	my @e;

	print "INIT_ROUTING\n" if $VRML::verbose::scene;
	$this->iterate_nodes_all(sub { push @e, $_[0]->initialize($this); });

	for(keys %{$this->{Bindable}}) {
		print "INIT_BINDABLE '$_'\n" if $VRML::verbose::scene;
		$eventmodel->send_event_to($this->{Bindable}{$_},
			set_bind, 1);
	}

	$eventmodel->put_events(\@e);
	print "INIT_ROUTING COMPLETE\n"  if $VRML::verbose::scene;

}

sub set_bind {
	my($this, $node, $value, $time) = @_;
	my $t = $node->{TypeName};
	my $s = ($this->{Stack}{$t} or $this->{Stack}{$t} = []);
	print "\nScene: SET_BIND! $this\n   ($node $t $value), \n  STACK #: $#$s\n"
		if $VRML::verbose::bind;
	if($value) {
		print "Scene: BINDING IT!\n"
			if $VRML::verbose::bind;
		if($#$s != -1) {  # Do we have a stack?

			if($node == $s->[-1]) {
				print("Scene: Bind node is top of stack...\n")
					if $VRML::verbose::bind;
				return; ## JAS - according to the book, do nothing.
			}
			my $i;
			for(0..$#$s) {
				if($s->[$_] == $node) {$i = $_}
			}
			print "Scene: WAS AS '$i'\n"
				if $VRML::verbose::bind;
			$s->[-1]->{RFields}{isBound} = 0;
			if($s->[-1]->{Type}{Actions}{WhenUnBound}) {
				&{$s->[-1]->{Type}{Actions}{WhenUnBound}}($s->[-1],$this);
			}
			if(defined $i) {
				splice @$s, $i, 1;
			}
		}
		$node->{RFields}{bindTime} = $time;
		$node->{RFields}{isBound} = 1;
		if($node->{Type}{Actions}{WhenBound}) {
			&{$node->{Type}{Actions}{WhenBound}}($node,$this,0);
		}
		print "Scene: PUSHING $node on @{$s}\n" if $VRML::verbose::bind;
		push @$s, $node;
		print "Scene: ..... is @{$s}\n" if $VRML::verbose::bind;
	} else {
		# We're unbinding a node.
		print "Scene: UNBINDING IT!\n"
			if $VRML::verbose::bind;
		if($node == $s->[-1]) {
			print "Scene: WAS ON TOP!\n"
				if $VRML::verbose::bind;
			$node->{RFields}{isBound} = 0;
			if($node->{Type}{Actions}{WhenUnBound}) {
				&{$node->{Type}{Actions}{WhenUnBound}}($node,$this);
			}
			pop @$s;
			if(@$s) {
				$s->[-1]->{RFields}{isBound} = 1;
				$s->[-1]->{RFields}{bindTime} = $time;
				if($s->[-1]->{Type}{Actions}{WhenBound}) {
					&{$s->[-1]->{Type}{Actions}{WhenBound}}($s->[-1],$this,1);
				}
			}
		} else {
			my $i;
			for(0..$#$s) {
				if($s->[$_] == $node) {$i = $_}
			}
			print "Scene: WAS AS '$i'\n"
				if $VRML::verbose::bind;
			if(defined $i) {
				splice @$s, $i, 1;
			}
		}
	print "\n" if $VRML::verbose::bind;
	}
}

1;
