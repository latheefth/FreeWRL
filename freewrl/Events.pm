# Copyright (C) 1998 Tuomas J. Lukka 1999 John Stewart - CRC Canada.
# DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
# See the GNU Library General Public License (file COPYING in the distribution)
# for conditions of use and redistribution.

# Events.pm -- event handling code for VRML::Browser.
#
# We take all the nodes referenced by events in the scenegraph
# and at each timestep ask each node able to produce events
# whether it wants to right now.
#
# Then we propagate the events along the routes. 

package VRML::EventMachine;


# to save processing power, we only render when we need to.
my $havevent = 0;

my %SCENENUMBERS = ();
my $scenecount = 0;

sub new {
	my($type) = @_;
	bless {
		   First => {},
		   Listen => undef,
		   IS => undef,
		   Queue => undef,
		   Route => undef,
		  },$type;
}

sub print {
	my($this) = @_;
	return unless $VRML::verbose::events;
	my $handle;

	print "DUMPING EventModel\nFIRST:\n";
	for(values %{$this->{First}}) {
		print "\t",VRML::NodeIntern::dump_name($_),":\t$_->{TypeName}\n";
	}

	print "ROUTE hash: ", VRML::Debug::toString($this->{Route}), "\n";
	print "\nIS hash: ", VRML::Debug::toString($this->{IS}), "\n";
}

# XXX Softref
sub add_first {
	my($this,$node) = @_;
	print "Events.pm: add_first ", VRML::NodeIntern::dump_name($node),"\n"
		if $VRML::verbose::events;
	$this->{First}{$node} = $node;

}

sub remove_first {
	my($this,$node) = @_;
	delete $this->{First}{$node};
}

###############################################################################
#
# Initialize Scripting interfaces. 

sub verify_script_started {
	my ($scene, $node, $number) = @_;

	#print "verify_script_started, $node, $number\n";

	print "ScriptInit t ",
		VRML::NodeIntern::dump_name($node), "\n"
 			if $VRML::verbose::script;

	my $h;
	my $Browser = $node->{Scene}->get_browser();

	for (@{$node->{Fields}{url}}) {
		# is this already made???
		print "Working on $_\n" 
			if $VRML::verbose::script;
		if (defined  $node->{J}) {
			print "...{J} already defined, skipping\n"
 				if $VRML::verbose::script;
			last;
		}

		my $str = $_;
		print "TRY $str\n" if $VRML::verbose::script;
		if (s/^perl(_tjl_xxx1)?://) {
			print "perl scripting not moved yet to new routing structure\n";
			last;

			print "XXX1 script\n" if $VRML::verbose::script;
			check_perl_script();

			# See about RFields in file ARCHITECTURE and in
			# Scene.pm's VRML::FieldHash package
			my $u = $node->{Fields};

			my $node = $node->{RFields};

			# This string ties scalars
			my $nodeie = join("", map {
				"tie \$$_, 'MTS',  \\\$node->{$_};"
			} script_variables($u));

			$h = eval "({$_})";

			# Wrap up each sub in the script node
			foreach (keys %$h) {
				my $nodemp = $h->{$_};
				my $src = join ("\n",
					"sub {",
					"  $nodeie",
					"  \&\$nodemp (\@_)",
					"}");
					## print "---- src ----$src\n--------------",
					$h->{$_} = eval $src ;
			}

			print "Evaled: $h\n",
				"-- h = $h --\n",
					(map {"$_ => $h->{$_}\n"}
					 keys %$h),
					"-- u = $u --\n",
					(map {
					"$_ => $u->{$_}\n"
					} keys %$u),
					"-- t = $node --\n",
					(map {
					"$_ => $node->{$_}\n"
					} keys %$node)
			 if $VRML::verbose::script;
			if ($@) {
				die "Invalid script '$@'"
			}
			last;
		} elsif (/\.class$/) {
			print "java class invocation scripting not moved yet to new routing structure\n";
			last;

			my $wurl = $scene->get_world_url();
			$node->{PURL} = $scene->get_url();
			if (!defined $VRML::J) {
				eval('require "VRML/VRMLJava.pm"');
				die $@ if ($@);

				$VRML::J =
					VRML::JavaCom->new($scene->get_browser);
			}
			if (defined $wurl) {
				$VRML::J->newscript($wurl, $_, $node);
			} else {
				$VRML::J->newscript($node->{PURL}, $_, $node);
			}

			$node->{J} = $VRML::J;
			last;
		} elsif (/\.js/) {
			# New js url handling
			my $code = VRML::NodeType::getTextFromURLs($scene, $_, $node);

			print "JS url: code = $code\n"
				if $VRML::verbose::script;
			eval('require VRML::JS;');
			die $@ if ($@);

			$node->{J} = VRML::JS->new($number, $code, $node, $Browser);
			last;
		} elsif (s/^\s*?(java|vrml)script://) {
			eval('require VRML::JS;');
			die $@ if ($@);

			$node->{J} = VRML::JS->new($number, $_, $node, $Browser);
			last;
		} else {
			warn("Unknown script: $_");
		}
	}

	die "Didn't find a valid perl(_tjl_xxx)? or java script"
		if (!defined $h and !defined $node->{J});

	print "Script got: ", (join ',',keys %$h), "\n"
		if $VRML::verbose::script;
	$node->{ScriptScript} = $h;
	my $s;
	if (($s = $node->{ScriptScript}{"initialize"})) {
		print "CALL $s\n if $VRML::verbose::script"
			if $VRML::verbose::script;
		perl_script_output(1);
		my @res = &{$s}();
		perl_script_output(0);
	}
}


###############################################################################
#
# Nodes get stored in many ways, depending on whether it is a PROTO, normal
# node, etc, etc. This tries to find a backend (CNode, or script) for a node.

# node = the node pointer; whatever we can get
# field = the field name, eg, "clicked"
# direction = "eventOut" or "eventIn".

# returns node, field, script, ok

sub resolve_node_cnode {
	my ($this, $scene, $node, $field, $direction) = @_;

	# return values.
	my $outptr = 0;
	my $outoffset = 0;
	my $to_count = 0;
	my $tonode_str = "";
	my @tonodes;
	my $scrpt = 0;
	my $il = 0;
	my $ok = 0;
	my $cs;
	my $proto_node;
	my $proto_field;
	my $is_proto;
	my $tmp;


	print "\nVRML::EventMachine::resolve_node_cnode: ", VRML::Debug::toString(\@_), "\n"
		if $VRML::verbose::events;

	$tmp = VRML::Handles::get($node);
	if (ref $tmp eq "VRML::NodeIntern") {
		$node = $tmp;
	} else {
		$node = $scene->getNode($tmp);
		if (!defined $node) {
			warn("DEF node $tmp is not defined");
			return (0,0,0,0,0);
		}
	}
	print "handle got $node ", ($node->{IsProto} ? "PROTO " : ""),
		"$node->{TypeName} ", VRML::NodeIntern::dump_name($node),"\n"
			if $VRML::verbose::events;

	# is this an IS?
	if ($node->{IsProto} && exists $this->{IS}{$node}{$field}) {
		$is_proto = 1;
		if ($direction =~ /eventIn/) {
			foreach (@{$this->{IS}{$node}{$field}}) {
				$proto_node = $_->[0];
				$proto_field = $_->[1];

				next if ($proto_node->{TypeName} =~ /script/i);

				## see VRML::Field::SFNode::parse for why this is necessary
				if (defined ($outptr = $proto_node->{BackNode}{CNode})) {
					if ($proto_field =~ /^[0-9]+($VRML::Error::Word)$/) {
						$proto_field = $1;
					}
					if (!defined ($outoffset =
								  $VRML::CNodes{$proto_node->{TypeName}}{Offs}{$proto_field})) {
						print "No offset for $proto_field.\n";
						$outptr = undef;
					} else {
						print "$proto_node->{TypeName} ",
							VRML::NodeIntern::dump_name($proto_node),
									" CNode: $outptr, $proto_field eventIn: $outoffset.\n";
						push @tonodes, "$outptr:$outoffset";
						$to_count++;
					}
				} else {
					print "No CNode for $proto_node->{TypeName} ",
						VRML::NodeIntern::dump_name($proto_node), ".\n";
				}
			}
			$tonode_str = join(" ", @tonodes);
		} else { # XXX PROTO multiple eventOuts not handled yet

			foreach (@{$this->{IS}{$node}{$field}}) {
				$proto_node = $_->[0];
				$proto_field = $_->[1];

				next if ($proto_node->{TypeName} =~ /script/i);

				## see VRML::Field::SFNode::parse for why this is necessary
				if (defined ($outptr = $proto_node->{BackNode}{CNode})) {
					if ($proto_field =~ /^[0-9]+($VRML::Error::Word)$/) {
						$proto_field = $1;
					}
					if (!defined ($outoffset =
								  $VRML::CNodes{$proto_node->{TypeName}}{Offs}{$proto_field})) {
						print "No offset for $proto_field.\n";
						$outptr = undef;
					} else {
						print "$proto_node->{TypeName} ",
							VRML::NodeIntern::dump_name($proto_node),
									" CNode: $outptr, $proto_field eventOut: $outoffset.\n";
						last;
					}
				} else {
					print "No CNode for $proto_node->{TypeName} ",
						VRML::NodeIntern::dump_name($proto_node), ".\n";
				}
			}
			if (! ($outptr || $offset) &&
				! $proto_node->{TypeName} =~ /script/i) {
				print "Failed to find CNode info for PROTO $node->{TypeName} ",
					VRML::NodeIntern::dump_name($node), " in IS hash.\n";
				return (0,0,0,0,0);
			}
		}
	}


	# Protos, when expanded, still have the same script number. We need to make
	# sure that a script within a proto is uniquely identified by the scene
	# proto expansion; otherwise routing will cross-pollinate .

	$cs = $scene;
	if (defined $node->{ProtoExp}) {
		#print "this is a protoexp, I am ",VRML::NodeIntern::dump_name($node->{Scene}), 
		#		" ProtoExp ",VRML::NodeIntern::dump_name($node->{ProtoExp}),"\n";
		$cs = $node->{ProtoExp};

		## needed ???
		$node = $node->real_node();
		print "ProtoExp node now is $node->{TypeName} ", VRML::NodeIntern::dump_name($node), "\n";
	}
	if (!defined $SCENENUMBERS{$cs}) { $SCENENUMBERS{$cs} = $scenecount++; }
	my $scenenum = $SCENENUMBERS{$cs};
	#print "current scene number is $scenenum\n";

	#addChildren really is Children
	if (($field eq "addChildren") || ($field eq "removeChildren")) {
		$field = "children";
	}

	# ElevationGrid, Extrusion, IndexedFaceSet and IndexedLineSet
	# eventIns (see VRML97 node reference)
	# these things have set_xxx and xxx... if we have one of these...
	if (($node->{TypeName} eq "Extrusion") ||
	    ($node->{TypeName} eq "ElevationGrid") ||
	    ($node->{TypeName} eq "IndexedFaceSet") ||
	    ($node->{TypeName} eq "IndexedLineSet")) {
		if ($field =~ /^set_($VRML::Error::Word+)/) {
			$field = $1;
		}
	}

	if (!$is_proto && $node->{TypeName} =~ /script/i) {
		$outptr = $scenenum;
		$outoffset = VRML::VRMLFunc::paramIndex($field, $node->{Type}{FieldTypes}{$field});
		verify_script_started($scene, $node, $outptr);

		if ($direction eq "eventOut") {
			$scrpt = 1;
		} else {
			$scrpt = 2;
			$to_count = 1;
			$tonode_str = "$outptr:$outoffset";
		}
		# print "got a script, outptr $outptr, offset $outoffset, scenenum $scenenum\n";
	} elsif ($proto_node->{TypeName} =~ /script/i) { ## needed ???
		$outptr = $scenenum;
		$outoffset = VRML::VRMLFunc::paramIndex($proto_field, $proto_node->{Type}{FieldTypes}{$proto_field});
		verify_script_started($scene, $proto_node, $outptr);

		if ($direction eq "eventOut") {
			$scrpt = 1;
		} else {
			$scrpt = 2;
			$to_count = 1;
			$tonode_str = "$outptr:$outoffset";
			}
			# print "PROTO: got a script, outptr $outptr, offset $outoffset, scenenum $scenenum\n";
	} else {
		if (!defined $node->{BackNode}) {
			# check if this node resides within a Javascript invocation...
			# if so, we have to ensure the equivalence of nodes between 
			# the C structures and the Javascript invocation.
			# check out tests/8.wrl in the FreeWRL source distribution for
			# a script with a DEF'd node in it's parameter declarations.

			# Javascript will "know" to send/get values from this
			# VRML node, and will use perl calls to do so.


			my $brow = $scene->get_browser();
			#print "No backend, but browser has ",$brow->{BE}, " for node ",
			#	VRML::NodeIntern::dump_name($node),"\n";

			# make a backend
			$node->make_backend($brow->{BE},$brow->{BE});
		}

		if (!$is_proto) {
			if (!defined ($outptr=$node->{BackNode}{CNode})) {
				# are there backend CNodes made for both from and to nodes?
				print "add_route, from $field - no backend CNode node\n";
				return (0,0,0,0,0);
			}

			# are there offsets for these eventins and eventouts?
			if (!defined ($outoffset=$VRML::CNodes{$node->{TypeName}}{Offs}{$field})) {
				print "resolve_node_cnode: CNodes entry ",
					VRML::Debug::toString($VRML::CNodes{$node->{TypeName}}),
							", $node->{TypeName} ",
								VRML::NodeIntern::dump_name($node),
										", event $field offset not defined\n";
				return (0,0,0,0,0);
			}
			if ($direction =~ /eventIn/i) {
				$to_count = 1;
				$tonode_str = "$outptr:$outoffset";
			}
		}
#if ($is_proto) {
#	print "$proto_node->{TypeName} ", VRML::NodeIntern::dump_name($proto_node), " CNode: $outptr, $field $direction: $outoffset.\n";
#} else {
#	print "$node->{TypeName} ", VRML::NodeIntern::dump_name($node), " CNode: $outptr, $field $direction: $outoffset.\n";
#}
	}

	# ok, we have node and field, lets find either field length, or interpolator
	if ($direction =~ /eventOut/i) {
		# do we handle this type of data within C yet?
		if ($is_proto) {
			$il = VRML::VRMLFunc::getClen("VRML::Field::$proto_node->{Type}{FieldTypes}{$proto_field}"->clength($proto_field));
		} else {
			$il = VRML::VRMLFunc::getClen("VRML::Field::$node->{Type}{FieldTypes}{$field}"->clength($field));
		}
		if ($il == 0) {
			print "add_route, dont handle $eventOut types in C yet\n";
			return (0,0,0,0,0);
		}
	} else {
		# is this an interpolator that is handled by C yet?
		if ($is_proto) {
			$il = VRML::VRMLFunc::InterpPointer($proto_node->{Type}{Name});
		} else {
			$il = VRML::VRMLFunc::InterpPointer($node->{Type}{Name});
		}
		#print "interp pointer for ",$node->{Type}{Name}," is $il\n";
	}

	#return ($outptr, $outoffset, $scrpt, $is_count, $is_str, 1, $il);
	#return ($outptr, $outoffset, $scrpt, 1, $il);

	if ($direction =~ /eventIn/i) {
		return ($to_count, $tonode_str, $scrpt, 1, $il);
	} else {
		return ($outptr, $outoffset, $scrpt, 1, $il);
	}
}


################################################################################
# add_route
#
# go through, and find:
#	- pointer to datastructures in C
#	- offsets of actual data within these datastructures
#	- length of the data.
#	- function pointer for an Interpolator function, if required.
#
# with the following caveats:
#	- data length of "-1" is special - it signifies that the field is a MF
#	  field. (eg, Orientation/Normal Interpolators) the copy length is
#	  found at "event prop" time.
#
#	- script value of 1 - fromNode is a script node
#	- script value of 2 - toNode is a script node
#	- script value of 3 - fromNode and toNodes are script nodes
#

sub add_route {
	my($this, $scene, $fromNode, $eventOut, $toNode, $eventIn) = @_;

	my $outptr;
	my $outoffset;
#	my $inptr;
#	my $inoffset;
	my $to_count = 0;
	my $tonode_str = "";
	my $datalen;
	my $scrpt = 0;
	my $is_count = 0;
	my $is_str = "";
	my $extraparam = 0;
	my $fc = 0; my $tc = 0; #from and to script nodes.

	# print "\nstart of add_route, $scene, $fromNode, $eventOut, $toNode, $eventIn\n";

	# FROM NODE
	my ($outptr, $outoffset, $fc, $ok, $datalen) = $this->resolve_node_cnode($scene, $fromNode, $eventOut, "eventOut");
	if ($ok == 0) {return 1;} # error message already printed


	# TO NODE
	my ($to_count, $tonode_str, $tc, $ok, $intptr) = $this->resolve_node_cnode($scene, $toNode, $eventIn, "eventIn");
	if ($eventIn eq "addChildren") {
		$extraparam = 1;
	}
		
	if ($ok == 0) {return 1;} # error message already printed

	$scrpt = $fc + $tc;

	#print "\nVRML::EventMachine::add_route: outptr $outptr, ofst $outoffset, in $to_count '$tonode_str', len $datalen interp $intptr sc $scrpt\n";

	VRML::VRMLFunc::do_CRoutes_Register($outptr, $outoffset,
										$to_count, $tonode_str,
										$datalen, $intptr, $scrpt, $extraparam);
}

## needs to be tested with more than just the EAI AddRoute test and to have CRoutes added to it.
sub delete_route {
	my($this, $fn, $ff0, $tn, $tf0) = @_;
	my $i = 0;
	my @ar;

	print "Events.pm: DELETE_ROUTE $fn $ff $tn $tf\n" ;#JAS  if $VRML::verbose::events;
	$ff = $fn->{Type}{EventOuts}{$ff0};
	$tf = $tn->{Type}{EventIns}{$tf0};
	print "Events.pm: DELETE_ROUTE mapped $ff, $tf\n" ;#JAS if $VRML::verbose::events;

	die("Invalid fromfield '$ff0' for ROUTE") if (!defined $ff);

	die("Invalid tofield '$tf0' for ROUTE") if (!defined $tf);

	foreach (@{$this->{Route}{$fn}{$ff}}) {
		if ($_->[0] eq $tn && $_->[1] eq $tf) {
			splice(@{$this->{Route}{$fn}{$ff}}, $i, 1);
			last;
		}
		$i++;
	}
}

sub add_is {
	my ($this, $nodeParent, $is, $node, $field) = @_;

	## VRML97 4.83: multiple IS statements for the fields, eventIns, and
	## eventOuts in the prototype interface declaration is valid
	push @{$this->{IS}{$nodeParent}{$is}}, [ $node, $field ];
}

sub register_listener {
	my($this, $node, $field, $sub) = @_;
	$this->{Listen}{$node}{$field} = $sub;
}

# get_firstevent returns [$node, fieldName, value]
sub propagate_events {
	my($this, $timestamp, $be, $scene) = @_;
	my @e;
	my @ne;
	my $fk;
	my %sent; # to prevent sending twice, always set bit here

	for(values %{$this->{First}}) {
		print "GETFIRST ", VRML::NodeIntern::dump_name($_), " $_\n" 
			if $VRML::verbose::events;
		push @e, $_->get_firstevent();
	}
	for(@{$this->{MouseSensitive}}) {
		# Mouse Sensitive nodes, eg, PlaneSensors, TouchSensors. Actual
		# position, hyperhit, etc, now stored in C structures; only the
		# node event is passed from GLBackEnd now.

		# this can be sped up quite a bit, but it requires some 
		# rewriting on GLBackEnd.pm for Sensitive nodes.

		print "MEV node ", VRML::Debug::toString($_), " type $_->[0]->{TypeName}\n"
			if $VRML::verbose::events;
		my $nd = $_->[0];
		VRML::VRMLFunc::handle_mouse_sensitive($nd->{Type}{Name},
				$nd->{BackNode}{CNode},
				$_->[1],$_->[3]);
	}
	my $n = scalar @e;
	push @e, @{$this->{Queue}};

	$this->{MouseSensitive} = [];

	# CRoutes
	VRML::VRMLFunc::do_propagate_events();

	while(1) {
		my %ep; # All nodes for which ep must be called
		# Propagate our events as long as they last
		$this->{Queue} = [];
		@ne = ();

		# Call eventsprocessed
		#JAS - this happens only for Scripts, for now. Is it still required?
		for (values %ep) {
			push @ne,$_->events_processed($timestamp,$be);
		}
		last if(!@ne);
		@e = (@ne,@{$this->{Queue}}); # Here we go again ;)
		$this->{Queue} = [];
	}
}

# This sends an event TO node - only used by JavaScript to add/remove children.
sub send_event_to {
	my ($this, $node, $field, $value) = @_;
	my $mych;
	
	print "send_event_to depreciated $node, $field, @{$value} ",{@{$value}}->{CNode},"\n";
}

# This sends a bind/unbind event TO node
sub send_set_bind_to {
	my ($this, $node, $bindValue) = @_;
	my $outptr;
	my $outoffset;

	# are there a backend made for from node?
	if (!defined $node->{BackNode}) {
		print "set_bind - no backend node\n";
		return;
	}

	# are there backend CNodes made for both from and to nodes?
	if (!defined ($outptr=$node->{BackNode}{CNode})) {
		print "set_bind - no backend CNode node\n";
		return;
	}

	# are there offsets for these binds?
	if(!defined ($outoffset=$VRML::CNodes{$node->{TypeName}}{Offs}{set_bind})) {
		print "set_bind offset not defined\n";
		return;
	}

	VRML::VRMLFunc::do_bind_to($node->{TypeName}, $outptr, $bindValue);
}

sub handle_touched {
	my($this, $node, $but, $move, $over) = @_;
	#print "HTOUCH: node $node, but $but, move $move, over $over \n";
	push @{$this->{MouseSensitive}}, [ $node, $but, $move, $over];
}

1;
