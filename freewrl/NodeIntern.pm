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
use VRML::DEF;
use VRML::FieldHash;
use VRML::IS;
use VRML::Parser;
use VRML::USE;


###################

package NULL; # ;)

###################

sub dump {}
sub make_backend { return (); }
sub make_executable {}
sub gather_defs {}
sub iterate_nodes {}

sub as_string { "NULL" }


###############################################################
#
# This is VRML::NodeIntern, the internal representation for a single
# node.

package VRML::NodeIntern;

my %DUMPNAMES = ();
my $DNINDEX = 1;
my $SCINDEX = 1;

sub dump_name {
    my ($name) = @_;

    my $nr = ref $name;

    if (!exists $DUMPNAMES{$name}) {
	if ("VRML::NodeIntern" eq $nr) {
	    $DUMPNAMES{$name} = "NODE$DNINDEX";
	    $DNINDEX++;
	} elsif ("VRML::Scene" eq $nr) {
	    $DUMPNAMES{$name} = "SCENE$SCINDEX";
	    $SCINDEX++;
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
    my ($this, $parentnode) = @_;

    # print "Scene::VRML::NodeIntern gather_defs, this ",dump_name($this), "   parentnode ",
    # 	dump_name($parentnode),"\n";

    if (defined $this->{IsProto}) {
	# possibly this is a Scene PROTO Expansion
	# print "gather_defs, isProto defined for ",VRML::NodeIntern::dump_name($this),"\n";

	foreach (@{$this->{ProtoExp}{Nodes}}) {
	    # print "VRML::NodeIntern::gather_defs ProtoExp ",dump_name($this)," we are going to look at ",
	    # dump_name($_),"\n";
	    $_->gather_defs($parentnode);
	    # print "VRML::NodeIntern::gather_defs, done proto expansion for ",dump_name($_),"\n";
	}

	if (defined $this->{ProtoExp}{DEF}) {
		# print "VRML::NodeIntern::gather_defs, PROTO EXP DEF FOUND!!! parent ",dump_name($parentnode), "\n";
		my $df;

		# print "before adding, keys %d\n";
		# for (keys %{$parentnode->{DEF}}) {print "key $_ ";}
		# print "\n";

		for $df (keys %{$this->{ProtoExp}{DEF}}) {
			# print "and it has a key of $df\n";
			# copy it to the parentnode
			$parentnode->{DEF}{$df} = $this->{ProtoExp}{DEF}{$df};
		}
	}

    } elsif (defined $this->{Fields}) {
	# print "VRML::NodeIntern gather_defs Fields (children of ",dump_name($this),")\n";

	my $fld;
	for $fld (keys %{$this->{Fields}}) {
	    if (ref $this->{Fields}{$fld} eq "ARRAY") {
		# first level of an array...
		# print "VRML::NodeIntern gather_defs, ",dump_name($this),"->{Fields}{$fld} is an array\n";

		foreach (@{$this->{Fields}{$fld}}) { 
		    if (ref $_ ne "") { 
			if (ref $_ eq "ARRAY") {
			    # two dimensional array

			    my $ae;
			    foreach $ae (@{$_}) {
				# print "VRML::NodeIntern gather_defs, ",dump_name($this), "element $ae\n";
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
    my ($this, $level) = @_;

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
		my ($fnam, $ff, $tnam, $tf) = @$_;
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
print "debugging\n";
	    print "(array) ";
	    foreach (@{$this->{SceneRoutes}}) {
		#print "	$_[0]  $_[1] $_[2] $_[3]\n";
		# print @{$_},"\n";
		my ($fnam, $ff, $tnam, $tf) = @$_;
		print "Route from $fnam field $ff to $tnam field $tf\n";
		my $fld;
		for $fld (keys %{$fnam}) {
			print "		from name key $_\n"; 
		}
	    }


	my $sk;
	foreach $sk (@{{$this}->{SceneRoutes}}) {
	    print "SceneRoutes of ",dump_name($this),") ";
	    print "$sk\n";
	    $sk->dump($level+1);
	}

print "end of debugging\n";
    }

    if (defined $this->{ProtoExp}) {
	print "\n$padded(ProtoExp of ",dump_name($this),"\n";
	$this->{ProtoExp}->dump($level+1);
    } elsif (defined $this->{Type}) {
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
				    # print "$padded(DATA) $ae\n";
				}
			    }
			} else {
			    $_->dump($level+1);
			}
		    } else {
			# print "$padded(DATA) $_\n";
		    }
		}
	    } else {
		if (ref $this->{Fields}{$fld} ne "") {
		    $this->{Fields}{$fld}->dump($level+1);
		} else {
		    # print "$padded(DATA) $_\n";
		}
	    }
	}
    }
}


sub new {
    my ($type, $scene, $ntype, $fields, $eventmodel) = @_;
    print "VRML::NodeIntern::new $ntype:\n" if $VRML::verbose::nodec;
    my %rf;
    my $this = bless {
		TypeName => $ntype,
		Fields => $fields,
		EventModel => $eventmodel,
		Scene => $scene,
		SceneRoutes => undef,
		BackNode => undef,
		BackEnd => undef,
		RFields => undef,
		IsProto => undef,
		Type => undef
    }, $type;
    tie %rf, VRML::FieldHash, $this;
    $this->{RFields} = \%rf;
    my $t;
    if (!defined ($t = $VRML::Nodes{$this->{TypeName}})) {
	# PROTO
	$this->{IsProto} = 1;
	$this->{Type} = $scene->get_proto($this->{TypeName});
	print "\tnew Node ", VRML::NodeIntern::dump_name($this),
	    "of type $ntype is a PROTO, type is ",
	    	VRML::NodeIntern::dump_name($this->{Type}),"\n"
		 		if $VRML::verbose::nodec;
    } else {
	# REGULAR
	$this->{Type} = $t;
	print "\tnew Node of type $ntype is regular, type is ", %{$this->{Type}},"\n"
			 if $VRML::verbose::nodec;
    }
    $this->do_defaults();
    return $this;
}

# Construct a new Script node -- the Type argument is different
# and there is no way of this being a proto.
sub new_script {
    my ($type, $scene, $stype, $fields, $eventmodel) = @_;
    print "VRML::NodeIntern::new_script: $stype->{Name}\n" if $VRML::verbose::nodec;
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
	my ($this) = @_;

	for (keys %{$this->{Type}{Defaults}}) {
		if (!exists $this->{Fields}{$_}) {
			$this->{Fields}{$_} = $this->{Type}{Defaults}{$_};
		}
	}
}

sub as_string {
    my ($this) = @_;

    my $s = "$this->{TypeName} {";

    # is this a script being sent back via EAI?
    if ("__script" eq substr($this->{TypeName},0,8)) {
    	$s .= " SCRIPT NOT PRINTED } ";
    	return $s;
    }

    for (keys %{$this->{Fields}}) {
		$s .= " $_ ";
		if ("VRML::IS" eq ref $this->{Fields}{$_}) {
			$s .= $this->{Fields}{$_}->as_string();
		} else {
			$s .= "VRML::Field::$this->{Type}{FieldTypes}{$_}"->
				as_string($this->{Fields}{$_});
		}
    }
    $s .= " }";
    return $s;
}

# If this is a PROTO expansion, return the actual "physical" VRML
# node under it.
sub real_node {
    my ($this, $proto) = @_;

    if (!$proto and defined $this->{IsProto}) {
	 #print "Scene.pm - PROTO real node returning the proto expansion...\n";
	 #print "\t{protoExp}{Nodes}[0] is ", dump_name($this->{ProtoExp}{Nodes}[0]),"\n";
	 #print "\t{protoExp}{Nodes}[1] is ", dump_name($this->{ProtoExp}{Nodes}[1]),"\n";
	 #print "\t{protoExp}{Nodes}[2] is ", dump_name($this->{ProtoExp}{Nodes}[2]),"\n";
	
	VRML::Handles::front_end_child_reserve(
	    $this->{ProtoExp}{Nodes}[0]->real_node(),
	    $this);
	return $this->{ProtoExp}{Nodes}[0]->real_node;
    } else {
	# print "Scene.pm - PROTO real node returning just $this\n";
	return $this;
    }
}

# Return the initial events returned by this node.
sub get_firstevent {
    my ($this, $timestamp) = @_;
    print "VRML::NodeIntern::get_firstevent $this $this->{TypeName} $timestamp\n"
	    if $VRML::verbose;
    if ($this->{Type}{Actions}{ClockTick}) {
		print "\tAction clocktick!\n" if $VRML::verbose;
		my @ev = &{$this->{Type}{Actions}{ClockTick}}(
				$this, $this->{RFields}, $timestamp);
		for (@ev) {
			$this->{Fields}{$_->[1]} = $_->[2];
		}
		return @ev;
    }
    return ();
}

sub receive_event {
    my ($this, $field, $value, $timestamp) = @_;

    if (!exists $this->{Fields}{$field}) {
	die("Invalid event received: $this->{TypeName} $field")
	unless($field =~ s/^set_// and
		   exists($this->{Fields}{$field})) ;
    }
    print "VRML::NodeIntern::receive_event ",
		VRML::NodeIntern::dump_name($this),
				" $this->{TypeName} $field $timestamp ",
					(ref $value eq "ARRAY" ?
					 "[ ".(join ", ", @{$value})." ]" :
					 $value),"\n"
						if $VRML::verbose::events;
    $this->{RFields}{$field} = $value;

    if ($this->{Type}{Actions}{$field}) {
	print "\treceived event action!\n" if $VRML::verbose::events;
	my @ev = &{$this->{Type}{Actions}{$field}}($this, $this->{RFields},
			$value, $timestamp);
	for (@ev) {
	    $this->{Fields}{$_->[1]} = $_->[2];
	}
	return @ev;
    }  elsif ($this->{Type}{Actions}{__any__}) {
	my @ev = &{$this->{Type}{Actions}{__any__}}(
		    $this,
		    $this->{RFields},
		    $value,
		    $timestamp,
		    $field,
	);
	# XXXX!!!???
	for (@ev) {
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
    my ($this) = @_;
    return $this->{Scene}->get_scene();
}

sub events_processed {
    my ($this, $timestamp, $be) = @_;
    print "VRML::NodeIntern::events_processed $this $this->{TypeName} $timestamp $be\n"
        if $VRML::verbose;

    if ($this->{Type}{Actions}{EventsProcessed}) {
	print "\tprocessed event action!\n" if $VRML::verbose;
	return &{$this->{Type}{Actions}{EventsProcessed}}($this, 
		    $this->{RFields},
		    $timestamp);
    }

    # JAS - this seemed to be a spurrious call - this simply resets ALL
    # fields.
    # if ($this->{BackNode}) {
    # 	$this->set_backend_fields();
    # }
}

# Copy a deeper struct
sub ccopy {
    my ($v, $scene) = @_;
    if (!ref $v) { return $v }
    elsif ("ARRAY" eq ref $v) { return [map {ccopy($_, $scene)} @$v] }
    else { return $v->copy($scene) }
}

# Copy me
sub copy {
    my ($this, $scene) = @_;
    my $new = {};
    $new->{Type} = $this->{Type};
    $new->{TypeName} = $this->{TypeName};
    $new->{EventModel} = $this->{EventModel} ;
    my %rf;
    if (defined $this->{IsProto}) {$new->{IsProto} = $this->{IsProto};};

    tie %rf, VRML::FieldHash, $new;
    $new->{RFields} = \%rf;
    for (keys %{$this->{Fields}}) {
	my $v = $this->{Fields}{$_};
	$new->{Fields}{$_} = ccopy($v, $scene);
    }
    $new->{Scene} = $scene;
    return bless $new, ref $this;
}

sub iterate_nodes {
    my ($this, $sub, $parent) = @_;
	my $ft;
    print "VRML::NodeIntern::iterate_nodes\n" if $VRML::verbose::scene;
    &$sub($this, $parent);

	for (keys %{$this->{Fields}}) {
		$ft = $this->{Type}{FieldTypes}{$_};
		if ($ft =~ /SFNode$/) {
			print "\tField type SFNode\n" if $VRML::verbose::scene;
			if (!defined $this->{Fields}{$_}) {
				$this->{Fields}{$_} = "NULL";
			}
			$this->{Fields}{$_}->iterate_nodes($sub, $this);
		} elsif ($ft =~ /MFNode$/) {
			print "\tField type MFNode\n" if $VRML::verbose::scene;
			my $ref = $this->{RFields}{$_};
			for (@$ref) {
				print "\titerate_nodes 3 going down... $_\n" if $VRML::verbose::scene;
				$_->iterate_nodes($sub, $this);
			}
		} else {
		}
	}
}

sub make_executable {
    my ($this, $scene) = @_;
    print "VRML::NodeIntern::make_executable ",
		VRML::NodeIntern::dump_name($this),
				" $this->{TypeName}\n"
					if $VRML::verbose::scene;

    # loop through all the fields for this node type.

    for (keys %{$this->{Fields}}) {
	print "MKEXE - key $_\n" if $VRML::verbose::scene;

	# First, get ISes values
	if (ref $this->{Fields}{$_} eq "VRML::IS" ) {
	    print "MKEXE - its an IS!!!\n" if $VRML::verbose::scene;
	    my $n = $this->{Fields}{$_}->name;
	    $this->{Fields}{$_} = $scene->make_is($this, $_, $n);
	}
	# Then, make the elements executable. Note that 
	# we do two things; the first is for non-arrays, the
	# second for arrays.

	if (ref $this->{Fields}{$_} and
		    "ARRAY" ne ref $this->{Fields}{$_}) {
	    print "EFIELDT: SFReference\n" if $VRML::verbose::scene;
	    $this->{Fields}{$_}->make_executable($scene, $this, $_);

	} elsif ( $this->{Type}{FieldTypes}{$_} =~ /^MF/) {
	    print "EFIELDT: MF\n" if $VRML::verbose::scene;
	    my $ref = $this->{RFields}{$_};
	    for (@$ref) {
		# print "EFIELDT, field $_, ",ref $_,"\n";
		$_->make_executable($scene) if (ref $_ and "ARRAY" ne ref $_);
	    }

	} else {
	    print "MKEXE - doesn't do anything.\n" if $VRML::verbose::scene;
	    # Nada
	}
    }

    # now, is this a PROTO, and is it not expanded yet?

    if (defined $this->{IsProto} && !$this->{ProtoExp}) {
	# print "MAKE_EXECUTABLE_PROTOEXP $this, $this->{TypeName},
	# $this->{Type}\n";

	print "COPYING $this->{Type} $this->{TypeName}\n"
	    if $VRML::verbose::scene;

	$this->{ProtoExp} = $this->{Type}->get_copy();
	$this->{ProtoExp}->set_parentnode($this, $scene);
	$this->{ProtoExp}->make_executable();

	# print "MAKE_EXECUTABLE_PROTOEXP finish $this, $this->{TypeName},
	# $this->{Type}, $this->{ProtoExp}\n";

	my $ptr = $this->{ProtoExp};
	# print " lets see, the newp definition of $ptr is: \n";
	# print "\tparent:", $ptr->{Parent},"\n";
	# print "\tName:", $ptr->{Name},"\n";
	# print "\tParameters: ";
	# for (keys %{$ptr->{Pars}}) {
	    # print " $_";
	# }
	# print "\n";
	# print "\tFieldKinds: ";
	# for (keys %{$ptr->{FieldKinds}}) {
	    # print ", $_ ", $ptr->{FieldKinds}{$_};
	# }
	# print "\nMAKE_EXECUTABLE_PROTOEXP returning\n";

    } 

    print "END MKEXE ",VRML::NodeIntern::dump_name($this)," $this->{TypeName}\n"
	    if $VRML::verbose::scene;
}

sub initialize {
    my ($this, $scene) = @_;
    # Inline is initialized at make_backend

    # print "initializing ", dump_name($this), " typename ", $this->{TypeName},"\n";
    if ($this->{Type}{Actions}{Initialize}
	     && $this->{TypeName} ne "Inline") {
	return &{$this->{Type}{Actions}{Initialize}}($this, $this->{RFields},
			(my $timestamp=(POSIX::times())[0] / 100), $this->{Scene});
	# XXX $this->{Scene} && $scene ??
    }
    return ();
}

sub set_backend_fields {
    my ($this, @fields) = @_;
    my $be = $this->{BackEnd};

    if (!@fields) { @fields = keys %{$this->{Fields}} }

    my %f;
    for (@fields) {
	my $v = $this->{RFields}{$_};
	print "SBEF: ",VRML::NodeIntern::dump_name($this)," $_ '",
	    ("ARRAY" eq ref $v ? (join ', ', @$v) : $v), "' \n"
		if $VRML::verbose::be;
	
	if ($this->{Type}{FieldTypes}{$_} =~ /SFNode$/) {
	    print "SBEF: SFNODE\n" if $VRML::verbose::be;
	    $f{$_} = $v->make_backend($be);
	    print "SFNODE GOT $_: ", %{$f{$_}}, "\n\n" if $VRML::verbose::be;
	} elsif ($this->{Type}{FieldTypes}{$_} =~ /MFNode$/) {
	    print "SBEF: MFNODE @$v\n" if $VRML::verbose::be;
	    $f{$_} = [ map {$_->make_backend($be)} @{$v} ];
	    print "MFNODE GOT $_: @{$f{$_}}\n" if $VRML::verbose::be;
	} else {
	    print "SBEF: Not MF or SF Node...\n"  if $VRML::verbose::be;
	    $f{$_} = $v;
	}
    }
    $be->set_fields($this->{BackNode},\%f);
}


{

	my %NOT = map {($_=>1)} qw/
		WorldInfo
		TimeSensor
		TouchSensor
		ScalarInterpolator
		ColorInterpolator
		PositionInterpolator
		OrientationInterpolator
		CoordinateInterpolator
		NormalInterpolator
		PlaneSensor
		SphereSensor
		CylinderSensor
		VisibilitySensor
	/;

	#############################################################
	#
	# VRML::NodeIntern::make_backend
	#
	#############################################################

	sub make_backend {
		my ($this, $be, $parentbe) = @_;

		print "VRML::NodeIntern::make_backend ", VRML::NodeIntern::dump_name($this),
			", $this->{TypeName}: ", %{$this}, ",\n\tBackEnd: ", %{$be},
				",\n\tParent BackEnd: ", %{$parentbe},"\n"
					if $VRML::verbose::be;

		if (!defined $this->{BackNode}) {

			if ($this->{TypeName} eq "Inline") {
				print "\tVRML::NodeIntern::make_backend Inline initialize: ",
					{
					 $this->{Type}{Actions}{Initialize}}, ", RFields: ",
						 %{$this->{RFields}},"\n"
							 if $VRML::verbose::be;
			
				&{$this->{Type}{Actions}{Initialize}}($this, $this->{RFields},
					(my $timestamp=(POSIX::times())[0] / 100), $this->{Scene});
			}

			if ($NOT{$this->{TypeName}} or $this->{TypeName} =~ /^__script/) {
				print "VRML::NodeIntern::make_backend NOT - ", $this->{TypeName},"\n"
					if $VRML::verbose::be;
				return ();
			}

			if (defined $this->{IsProto}) {
				print "\tVRML::NodeIntern::make_backend IsProto ",
					VRML::NodeIntern::dump_name($this), "\n"
							if $VRML::verbose::be;

				## Inline'd node backends need special handling
				if ($this->{TypeName} eq "Inline") {
					$this->{BackNode} = $this->{ProtoExp}->make_backend($be, $parentbe);
					return $this->{BackNode};
				}

				return $this->{ProtoExp}->make_backend($be, $parentbe);
			}

			my $ben = $be->new_node($this->{TypeName});
			$this->{BackNode} = $ben;
			$this->{BackEnd} = $be;
			$this->set_backend_fields();

			# was this a viewpoint? Was it not in a proto definition?
			if ($this->{TypeName} eq "Viewpoint") {
				# print "NodeIntern::register_vp, viewpoint is ", $this->{Fields}{description},"\n";
				if ($this->{BackEnd}) {
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
		}
		print "\tVRML::NodeIntern::make_backend finished ",
			VRML::NodeIntern::dump_name($this),
					" $this->{TypeName}, ", %{$this->{BackNode}}, "\n"
						if $VRML::verbose::be;

		return $this->{BackNode};
	}

}

1;
