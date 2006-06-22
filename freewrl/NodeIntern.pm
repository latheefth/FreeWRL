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

	# possibly this is a Scene PROTO Expansion
    if ($this->{IsProto}) {
		# print "NodeIntern - gather_defs on ",dump_name($parentnode),"\n";

		foreach (@{$this->{ProtoExp}{Nodes}}) {
			$_->gather_defs($parentnode);
		}

		if (defined $this->{ProtoExp}{DEF}) {
			my $df;

			for $df (keys %{$this->{ProtoExp}{DEF}}) {
				# copy it to the parentnode
				$parentnode->{DEF}{$df} = $this->{ProtoExp}{DEF}{$df};
			}
		}
    } elsif (defined $this->{Fields}) {
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
			} elsif (ref $this->{Fields}{$fld} ne "") {
				$this->{Fields}{$fld}->gather_defs($parentnode);
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

		} elsif ("BackNode" eq $_) {
			print "BN: ";
			foreach my $bnub (keys %{ $this->{$_}}) {
				print "$bnub, ",$this->{BackNode}{$bnub}," ";
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


###############################################################################
#
# Initialize Scripting interfaces.

my $scriptInvocationNumber = 0;		# keep track of script invocations - scripts in protos i
					# have same TypeName but different runtime environments.

sub startScript {
	my ($node) = @_;

	print "VRML::EventMachine::startScript: ",
		VRML::Debug::toString(\@_), "\n"
			if $VRML::verbose::events;

	my $retval;
	my $scene = $node->{Scene};
	my $Browser = $node->{Scene}->get_browser();

	#foreach (keys %$node) { print "startScript node key $_\n"; }
	#foreach (keys %{$node->{Fields}}) { print "startScript nodefield key $_ val ",
	#		$node->{Fields}{$_},"\n"; }

	for (@{$node->{Fields}{url}}) {
		# is this already made???
		print "URL $_\n" if $VRML::verbose::events;
		if (defined  $node->{J}) {
			print "$node->{J} already defined for node ",
				VRML::NodeIntern::dump_name($node), ", skipping\n"
						if $VRML::verbose::script;
			return;
		}

		my $str = $_;
		print "TRY $str\n"
			if $VRML::verbose::script;

		if (/\.class$/) {
			eval ('require VRML::VRMLJava');
			$node->{J}= VRML::JavaClass->new($node,$scriptInvocationNumber, $_);
			last;
		} elsif (/\.js/) {
			# New js url handling
			# might have to put parent url in 2nd parameter
			my $code = VRML::Browser::getTextFromFile($_,"parent");

			print "JS url: code = $code\n"
				if $VRML::verbose::script;
			eval('require VRML::JS;');
			die $@ if ($@);

			$node->{J} = VRML::JS->new($scriptInvocationNumber, $code, $node, $Browser);
			last;
		} elsif (s/^\s*?(ecma|java|vrml)script://) {
			eval('require VRML::JS;');
			die $@ if ($@);

			$node->{J} = VRML::JS->new($scriptInvocationNumber, $_, $node, $Browser);
			last;
		} else {
			warn("Unknown script: $_");
		}
	}

	die "Didn't find a valid script"
		if (!defined $node->{J});

	$node->{scriptInvocationNumber} = $scriptInvocationNumber;

	#print "NODE $node ",
	#	($node->{IsProto} ?
	#	 "PROTO ".VRML::NodeIntern::dump_name($node->{ProtoExp})." " : " is not Proto "),
	#		 "$node->{TypeName} ", VRML::NodeIntern::dump_name($node),
	#		 " scriptInvocationNumber:",$node->{scriptInvocationNumber},
	#		 "\n";

	$scriptInvocationNumber ++;
}

sub new {
    my ($type, $scene, $ntype, $fields, $eventmodel) = @_;
    my %rf;
    my $this = bless {
					  BackNode => undef,
					  EventModel => $eventmodel,
					  Fields => $fields,
					  IsProto => undef,
					  ProtoExp => undef,
					  PURL => undef,
					  RFields => undef,
					  Scene => $scene,
					  Type => undef,
					  TypeName => undef
					 }, $type;
    tie %rf, VRML::FieldHash, $this;
    $this->{RFields} = \%rf;
    my $t;

	if ($ntype->{Name} =~ /script/i) {
		$this->{TypeName} = $ntype->{Name};
		$this->{Type} = $ntype;
    } elsif (!defined ($t = $VRML::Nodes{$ntype})) {
		# PROTO
		$this->{IsProto} = 1;
		$this->{TypeName} = $ntype;
		$this->{Type} = $scene->get_proto($this->{TypeName});
    } else {
		# REGULAR
		$this->{TypeName} = $ntype;
		$this->{Type} = $t;
    }

    $this->do_defaults($scene);

	print "VRML::NodeIntern::new: ", dump_name($this->{Scene}),
		($this->{IsProto} ? " PROTO" : " NOTPROTO "),
			dump_name($this->{Type}), " $this->{TypeName} ", dump_name($this), "\n"
		 		if $VRML::verbose::nodec;

	# if this is a bindable node, lets make the backend here, and register it so
	# that it is registered in order that the nodes are found in a file. If we do
	# not do this here, then the order may be out of line if another node is routed to
	if ($VRML::Nodes::bindable{$this->{TypeName}}) {
		my $be = VRML::Browser::getBE();
		my $ben = $be->new_CNode($this->{TypeName});

		$this->{BackNode} = $ben;
		$this->set_backend_fields();

		# is this a bindable node?
		#print "this is a bindable, ".$this->{TypeName}." cnode ".
		#	$this->{BackNode}{CNode}."\n";
	}

    return $this;
}


# Fill in nonexisting field values by the default values.

sub do_defaults {
	my ($this,$scene) = @_;
	my $ftype;
	my $init;

	for (keys %{$this->{Type}{Defaults}}) {
		if (!exists $this->{Fields}{$_}) {
			## eventOuts need initial values, so supply them
			## if the VRML file is missing some (may happen in Script node
			## declarations or in PROTO instances)
			if (!defined $this->{Type}{Defaults}{$_} and
					 $this->{Type}{FieldKinds}{$_} =~ /out/i) {
				$ftype = "VRML::Field::".$this->{Type}{FieldTypes}{$_};
				$init = $ftype->VRMLFieldInit();
				if (ref $init eq "ARRAY") {
					push @{$this->{Fields}{$_}}, @{$init};
				} else {
					$this->{Fields}{$_} = $init;
				}
			} elsif (ref $this->{Type}{Defaults}{$_} eq "ARRAY") {
				push @{$this->{Fields}{$_}}, @{$this->{Type}{Defaults}{$_}};
			} else {
				$this->{Fields}{$_} = $this->{Type}{Defaults}{$_};
			}
		}
	}
}

sub as_string {
    my ($this) = @_;

    my $s = "$this->{TypeName} {";

    # is this a script being sent back via EAI or JS?
    if ($this->{TypeName} =~ /^__script/) {
    	$s .= " SCRIPT NOT PRINTED } ";
    	return $s;
    }

    for (keys %{$this->{Fields}}) {
		if (ref $this->{Fields}{$_} =~ /(IS|USE|DEF)$/) {
		    $s .= " $_ " . $this->{Fields}{$_}->as_string();
		} elsif (exists $this->{Type}{FieldTypes}{$_}) {
		    $s .= " $_ " . "VRML::Field::$this->{Type}{FieldTypes}{$_}"->as_string($this->{Fields}{$_});
		}
    }
    $s .= " }";
    return $s;
}


sub real_node {
    my ($this) = @_;

	if ($VRML::verbose) {
		my ($package, $filename, $line) = caller;
		print "VRML::NodeIntern::real_node: $this->{TypeName} ", dump_name($this),
			($this->{IsProto} ?
			 " PROTO first node is ".VRML::Debug::toString($this->{ProtoExp}{Nodes}[0]) :
			 ""), " from $package, $line\n";

	print "start of real_node\n";
	if ($this->{IsProto}) {
	print "this is a PROTO\n";
		foreach (@{$this->{ProtoExp}{Nodes}}) {
			print "proto has a node $_\n";
		}

	} else {
		print "this is NOT a PROTO, just returning $this\n";
	}
}

	return $this->{ProtoExp}{Nodes}[0]->real_node() if ($this->{IsProto});

	return $this;
}

# Copy a deeper struct
sub ccopy {
    my ($v, $scene) = @_;

    if (!ref $v) {
		return $v;
	} elsif ("ARRAY" eq ref $v) {
		return [ map { ccopy($_, $scene) } @{$v} ];
	} else {
		return $v->copy($scene);
	}
}

# Copy me
sub copy {
    my ($this, $scene) = @_;

    # print "NodeIntern:copy invoked\n";
	my $new = bless {}, ref $this;
    $new->{Type} = $this->{Type};
    $new->{TypeName} = $this->{TypeName};
    $new->{EventModel} = $this->{EventModel} ;
    my %rf;
    if ($this->{IsProto}) {
		$new->{IsProto} = $this->{IsProto};
	}

    tie %rf, VRML::FieldHash, $new;
    $new->{RFields} = \%rf;

    for (keys %{$this->{Fields}}) {
		my $v = $this->{Fields}{$_};
		$new->{Fields}{$_} = ccopy($v, $scene);
    }
    $new->{Scene} = $scene;
	VRML::Handles::reserve($new);

	return $new;
}

sub iterate_nodes {
    my ($this, $sub, $parent) = @_;
	my $ft;

    print "VRML::NodeIntern::iterate_nodes: ", VRML::Debug::toString(\@_),
		", node type name: $this->{TypeName}\n"  if $VRML::verbose::scene;

    &$sub($this, $parent);

	for (keys %{$this->{Fields}}) {
		$ft = $this->{Type}{FieldTypes}{$_};
		if ($ft =~ /SFNode$/) {
			print "\t$_: Field type SFNode\n"  if $VRML::verbose::scene;
			$this->{Fields}{$_}->iterate_nodes($sub, $this);
		} elsif ($ft =~ /MFNode$/) {
			print "\t$_: Field type MFNode\n"  if $VRML::verbose::scene;
			my $ref = $this->{RFields}{$_};
			for (@{$ref}) {
				print "\titerate_nodes 3 going down... $_\n"  if $VRML::verbose::scene;
				$_->iterate_nodes($sub, $this);
			}
		} else {
		}
	}
}

sub make_executable {
    my ($this, $scene) = @_;
    

	print "VRML::NodeIntern::make_executable: ", VRML::Debug::toString(\@_),
		" $this->{TypeName}\n" if $VRML::verbose::scene;

    # loop through all the fields for this node type.

	my $field;
	my $rfield;
	my $entry;
	my $ref;
	my $r;
	my $n;

    foreach $field (keys %{$this->{Fields}}) {
		print "MKEXE - key $field\n" if $VRML::verbose::scene;

		$ref = ref $this->{Fields}{$field};

		# First, get ISes values
		if ($ref =~ /IS$/) {
			print "MKEXE - its an IS!!!\n" if $VRML::verbose::scene;
			$n = $this->{Fields}{$field}->name();

			$this->{Fields}{$field} = $scene->make_is($this, $field, $n);
			$ref = ref $this->{Fields}{$field};

			#print "mkexe, scene ",VRML::NodeIntern::dump_name($scene),
			#" this " ,VRML::NodeIntern::dump_name($this), " field $field n, $n, ref $ref",
			#	"\n";

			# save this information in a convenient way so that EAI can get at it.
			VRML::Browser::save_EAI_info ($scene,$this,$field,$n);
		}
		# Then, make the elements executable. Note that
		# we do two things; the first is for non-arrays, the
		# second for arrays.

		if ($ref and $ref ne "ARRAY") {
			print "EFIELDT: SFReference\n" if $VRML::verbose::scene;

			$this->{Fields}{$field}->make_executable($scene, $this, $field)
				if ($ref !~ /USE$/); # VRML::USE::make_executable does nothing
		} elsif ($this->{Type}{FieldTypes}{$field} =~ /^MF/) {
			print "EFIELDT: MF\n" if $VRML::verbose::scene;

			$rfield = $this->{RFields}{$field};

			foreach $entry (@{$rfield}) {
				$r = ref $entry;
				$entry->make_executable($scene) if ($r and $r ne "ARRAY");
			}
		} else {
			print "MKEXE - doesn't do anything.\n" if $VRML::verbose::scene;
		}
    }

    # now, is this a PROTO, and is it not expanded yet?

    if ($this->{IsProto} && !$this->{ProtoExp}) {
		print "NodeIntern COPYING ",dump_name($this->{Type})," $this->{TypeName}\n"
		 	if $VRML::verbose::scene;

		$this->{ProtoExp} = $this->{Type}->get_copy(); ## a scene
		$this->{ProtoExp}->set_parentnode($this, $scene);
		$this->{ProtoExp}->make_executable();
		print "NodeIntern, copy is $this->{ProtoExp}, name ",dump_name($this->{ProtoExp}),"\n"
			if $VRML::verbose::scene;
    }

    print "END MKEXE ",dump_name($this)," $this->{TypeName}\n"
	   if $VRML::verbose::scene;
}

sub set_backend_fields {
    my ($this, @fields) = @_;

    if (!@fields) { @fields = keys %{$this->{Fields}} }

    #print "at set_backend_fields of $this, ",
    #VRML::NodeIntern::dump_name($this),"\n";
    #my $key;
    #foreach $key (keys(%{$this->{Scene}})) {print "node key $key\n";}
    #foreach $key (keys(%{$this->{RFields}})) {print "node has RField $key\n";}
    #foreach $key (keys(%{$this->{Fields}})) {print "node has Field $key\n";}

    my $be = VRML::Browser::getBE();
    my %f;
    for (@fields) {
		my $v = $this->{RFields}{$_};
		print "SBEF: ",dump_name($this)," $_ '",
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

    $be->set_fields($this->{BackNode}, \%f);
    # print "finished set_backend_fields\n";
}


{
	#############################################################
	#
	# VRML::NodeIntern::make_backend
	#
	#############################################################

	sub make_backend {
		my ($this, $be, $parentbe) = @_;

		#my ($package, $filename, $line) = caller;
		#print "WSDT make_backend, have type of ",$this->{TypeName}," for ",
		#	VRML::NodeIntern::dump_name($this), " called from $package, line $line\n";

		

		# if this is a Text node, check the attached fontStyle. If the fontStyle
		# is a proto, copy it over. We have to do this for Text nodes here,
		# because fontStyle is handled in C in the same routines as Text, and we
		# can handle protos here easier than within C code.
		if ("Text" eq $this->{TypeName}) {
			#print "have a Text node - is the FontStyle a proto? \n";
			#foreach (keys %{$this->{Fields}}) { print "Text key $_\n"; }

			 # does the fontStyle node exist?
			 if (NULL ne  $this->{Fields}{fontStyle}) {

				# it exists - is it a PROTO?
				if ($this->{Fields}{fontStyle}{IsProto}) {

					# get the ProtoExpansion:
					my $pexp = $this->{Fields}{fontStyle}{ProtoExp}{Nodes}[0];

					# $pexp should hold the first node of the Proto Expansion,
					# which should be a Group node. So, get the first child
					# of the Group node - this had better be the fontStyle.

					my $fsnode = $pexp->{Fields}{children}[0];

					#print "pexp is $pexp, ",dump_name($pexp),"\n";
					#print "fsnode is ",dump_name($fsnode),"\n";

					if ("FontStyle" eq $fsnode->{TypeName}) {
						$this->{Fields}{fontStyle} = $fsnode;
					} else {
						print "expect a FontStyle for PROTO expansion, but got ",
							$fsnode->{TypeName},"\n";
						$this->{Fields}{fontStyle} = NULL;
					}
				}
			}
		}


		if ($VRML::verbose::be) {
			my ($package, $filename, $line) = caller;
			print "VRML::NodeIntern::make_backend: ", VRML::Debug::toString(\@_),
				" $this->{TypeName} from $package, $line\n";
			}

			if (!defined $this->{BackNode}{CNode}) {

			if ($this->{TypeName} eq "WorldInfo") {
				print "VRML::NodeIntern::make_backend NOT - ", $this->{TypeName},"\n"
					if $VRML::verbose::be;
				return ();
			}
			if ($this->{TypeName} eq "GeoMetadata") {
				print "VRML::NodeIntern::make_backend NOT - ", $this->{TypeName},"\n"
					if $VRML::verbose::be;
				return ();
			}

			if ($this->{TypeName} =~ /^__script/) {
				print "VRML::NodeIntern::make_backend script - ", $this->{TypeName},"\n"
					if $VRML::verbose::be;
				startScript($this);
				return();
			}

			if ($this->{IsProto}) {
				print "\tIsProto ", dump_name($this), "\n"
					if $VRML::verbose::be;
				#print "protoexp is ",$this->{ProtoExp}," ref ", ref $this->{ProtoExp},"\n";
				return $this->{ProtoExp}->make_backend($be, $parentbe);
			}

			my $ben = $be->new_CNode($this->{TypeName});

			$this->{BackNode} = $ben;
			$this->set_backend_fields();

                        # is this a bindable node?
                        if ($VRML::Nodes::bindable{$this->{TypeName}}) {
                                #VRML::Browser::register_bind($this);
				print "this is a bindable, ".$this->{TypeName}." cnode ".
				$this->{BackNode}{CNode}."\n";
                        }


		}
		print "\tVRML::NodeIntern::make_backend finished ",
			dump_name($this),
			" $this->{TypeName}, ", %{$this->{BackNode}}, "\n"
				if $VRML::verbose::be;

		return $this->{BackNode};
	}

}
1;
