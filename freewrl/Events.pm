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


sub new {
	my($type) = @_;
	bless {
		   First => {},
		   Listen => undef,
		   CIs => undef,
		   PIs => undef,
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

	print "ROUTEs\n", VRML::Debug::toString($this->{Route}), "\n";

	print "ISs\nPI hash:\n",
		VRML::Debug::toString($this->{PIs}),
				"\nCI hash:\n",
					VRML::Debug::toString($this->{CIs}), "\n";
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
	my ($node,$number) = @_;

#JAS	print "verify_script_started, $node, $number\n";
#JAS	foreach (keys %$node) {
#JAS	 	print "we have this key $_\n";
#JAS	}
#JAS
#JAS	print "verify_script_started, $node\n";
#JAS	foreach (keys %{$node->{Fields}}) {
#JAS		print "we have this Fields key $_, type ",$node->{Fields}{$_},"\n";
#JAS		#print "we have this Fields key $_, type ",$node->{Fields}{$_}{TypeName},"\n";
#JAS	}


	print "ScriptInit t ",
		VRML::NodeIntern::dump_name($node), "\n"
			if $VRML::verbose::script;

	my $h;
	my $Browser = $node->{Scene}->get_browser();

	for (@{$node->{Fields}{url}}) {
		# is this already made???
		print "Working on $_\n" if $VRML::verbose::script;
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
			my $code = getTextFromURLs($scene, $_, $node);

			print "JS url: code = $code\n"
				if $VRML::verbose::script;
			eval('require VRML::JS;');
			die $@ if ($@);

			$node->{J} = VRML::JS->new($number, $code, $node, $Browser);
			last;
		} elsif (s/^(java|vrml)script://) {
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

# Nodes get stored in many ways, depending on whether it is a PROTO, normal
# node, etc, etc. This tries to find a backend (CNode, or script) for a node.

# node = the node pointer; whatever we can get
# field = the field name, eg, "clicked"
# direction = "eventOut" or "eventIn".

# returns node, field, script, ok

sub resolve_node_cnode {
	my ($scene, $node, $field, $direction) = @_;

	# return values.
	my $outptr = 0;
	my $outoffset = 0;
	my $scrpt = 0; 
	my $il = 0;
	my $ok = 0;

	#print "start of resolve_node_cnode, node $node, field $field,  direction $direction\n";

	my $tmp = VRML::Handles::get($node);
	if (ref $tmp eq "VRML::NodeIntern") {
		$node = $tmp;
	} else {
		$node = $scene->getNode($tmp);
		if (!defined $node) {
			warn("DEF node $tmp is not defined");
			return (0,0,0,0,0);
		}
	}
	#print "handle got $node\n";

	#addChildren really is Children
	if (($field eq "addChildren") || ($field eq "removeChildren")) {
		$field = "children";
	}

	# ElevationGrid, Extrusion, IndexedFaceSet and IndexedLineSet
	# eventIns (see VRML97 node reference)
	# these things have set_xxx and xxx... if we have one of these...
	if ($field =~ /^set_($VRML::Error::Word+)/) {
		my $tmp = $1;
		#print "found a set ...\n";
		#print "one ",$node->{Type}{EventIns}{$tmp},"\n";
		#print "two ",$node->{Type}{FieldKinds}{$tmp},"\n";

		if ($node->{Type}{EventIns}{$tmp} and
			$node->{Type}{FieldKinds}{$tmp} =~ /^exposed/) {
			$field = $tmp;
		}
	}
	if ($field =~ /($VRML::Error::Word+)_changed$/) {
		$tmp = $1;
		#print "found a changed...\n";
		#print "one ",$node->{Type}{EventOuts}{$tmp},"\n";
		#print "two ",$node->{Type}{FieldKinds}{$tmp},"\n";

		if ($node->{Type}{EventOuts}{$tmp} and
			$node->{Type}{FieldKinds}{$tmp} =~ /^exposed/) {
			$field = $tmp;
		}
	}
	#print "event now is  $field\n";



	if (!defined $node->{BackNode}) {
		if ($node->{TypeName} =~/^__script__/) {
			$outptr = substr($node->{TypeName},10,100);
			$outoffset = VRML::VRMLFunc::paramIndex($field,
				$node->{Type}{FieldTypes}{$field});

			verify_script_started($node,$outptr);
			if ($direction eq "eventOut") {
				$scrpt = 1;
			} else {
				$scrpt = 2;
			}

			#print "got a script, outptr $outptr, offset $outoffset, scrpt $scrpt\n";

		} else {
			print "add_route, from $field - no backend node\n";
			return (0,0,0,0);
		}
	} else {
		if (!defined ($outptr=$node->{BackNode}{CNode})) {
			# are there backend CNodes made for both from and to nodes?
			print "add_route, from $field - no backend CNode node\n";
			return (0,0,0,0);
		}

		# are there offsets for these eventins and eventouts?
		if(!defined ($outoffset=$VRML::CNodes{$node->{TypeName}}{Offs}{$field})) {
			print "add_route, event $field offset not defined\n";
			return (0,0,0,0);
		}
	}

	# ok, we have node and field, lets find either field length, or interpolator
	if ($direction eq "eventOut") {
		# do we handle this type of data within C yet?
		$il=VRML::VRMLFunc::getClen(
			"VRML::Field::$node->{Type}{FieldTypes}{$field}"->clength($field));
		if ($il==0) {
			print "add_route, dont handle $eventOut types in C yet\n";
			return (0,0,0,0);
		}
	} else {
		# is this an interpolator that is handled by C yet?
		$il = VRML::VRMLFunc::InterpPointer($node->{Type}{Name});
	}

	return ($outptr, $outoffset, $scrpt, 1,$il);

# code from Scene.pm

#JAS		$tmp = VRML::Handles::get($_->[0]);
#JAS		print "From now is $tmp\n";
#JAS		if (ref $tmp eq "VRML::NodeIntern") {
#JAS			$fromNode = $tmp;
#JAS		} else {
#JAS			$fromNode = $this->getNode($tmp);
#JAS			if (!defined $fromNode) {
#JAS				warn("DEF node $tmp is not defined");
#JAS				next;
#JAS			}
#JAS		}
#JAS		print "fromnode $fromNode\n";
#JAS		foreach (keys %{$fromNode}) {print "fromnode key: $_\n";}
#JAS
#JAS		$tmp = VRML::Handles::get($_->[2]);
#JAS		if (ref $tmp eq "VRML::NodeIntern") {
#JAS			$toNode = $tmp;
#JAS		} else {
#JAS			$toNode = $this->getNode($tmp);
#JAS			if (!defined $toNode) {
#JAS				warn("DEF node $tmp is not defined");
#JAS				next;
#JAS			}
#JAS		}
#JAS
#JAS		## exposedFields and eventOuts with some error checking
#JAS		$eventOut = $_->[1];
#JAS		print "Scene routing, eventout $eventOut\n";
#JAS		if ($eventOut =~ /($VRML::Error::Word+)_changed$/) {
#JAS			print "Scene routing, _changed found, d1 is $1\n";
#JAS			$tmp = $1;
#JAS			print "evo ",$fromNode->{Type}{EventOuts}{$tmp},",fk ",$fromNode->{Type}{FieldKinds}{$tmp},"\n";
#JAS			
#JAS			foreach (keys %{$fromNode->{Type}}) { print "node type $_\n";}
#JAS			foreach (keys %{$fromNode->{Type}{EventOuts}}) { print "node type EV $_\n";}
#JAS			if ($fromNode->{Type}{EventOuts}{$tmp} and
#JAS				$fromNode->{Type}{FieldKinds}{$tmp} =~ /^exposed/) {
#JAS				$eventOut = $tmp;
#JAS			}
#JAS		}
#JAS		print "Scene routing, eventout now is  $eventOut\n";
#JAS
#JAS		if (!$fromNode->{Type}{EventOuts}{$eventOut}) {
#JAS			warn("Invalid eventOut $eventOut in route for $fromNode->{TypeName}");
#JAS			next;
#JAS		}
#JAS
#JAS		## exposedFields and eventIns with some error checking
#JAS		$eventIn = $_->[3];
#JAS		if ($eventIn =~ /^set_($VRML::Error::Word+)/) {
#JAS			$tmp = $1;
#JAS			if ($toNode->{Type}{EventIns}{$tmp} and
#JAS				$toNode->{Type}{FieldKinds}{$tmp} =~ /^exposed/) {
#JAS				$eventIn = $tmp;
#JAS			}
#JAS		}
#JAS
#JAS		if (!$toNode->{Type}{EventIns}{$eventIn}) {
#JAS			warn("Invalid eventIn $eventIn in route for $toNode->{TypeName}");
#JAS			next;
#JAS		}

# end of code from scene.pm

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

	my $outoffset;
	my $inoffset;
	my $outptr;
	my $inptr;
	my $datalen;
	my $scrpt = 0;
	my $fc = 0; my $tc = 0; #from and to script nodes.

	#print "\nstart of add_route, $scene, $fromNode, $eventOut, $toNode, $eventIn\n";

	# FROM NODE
	my ($outptr,$outoffset,$fc,$ok,$datalen) = resolve_node_cnode ($scene,$fromNode,$eventOut,"eventOut");
	if ($ok == 0) {return 1;} # error message already printed


	# TO NODE
	my ($inptr,$inoffset,$tc,$ok,$intptr) = resolve_node_cnode ($scene,$toNode,$eventIn,"eventIn");
	if ($ok == 0) {return 1;} # error message already printed

	$scrpt = $fc + $tc;

	# print "add_route, outptr $outptr, ofst $outoffset, inptr $inptr, ofst $inoffset len $datalen interp $intptr sc $scrpt\n";
	VRML::VRMLFunc::do_CRoutes_Register($outptr, $outoffset, $inptr, $inoffset, $datalen,
		$intptr, $scrpt);
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

sub add_is_out {
	my($this, $pn, $pf, $cn, $cf) = @_;

	$this->{CIs}{$cn}{$cf} = [$pn, $pf];
}

sub add_is_in {
	my($this, $pn, $pf, $cn, $cf) = @_;

	push @{$this->{PIs}{$pn}{$pf}}, [$cn, $cf];
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

	#save timestamp in backend 
	VRML::VRMLFunc::setTick($timestamp);
	
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

		print "MEV node $_->[0], but $_->[1], move $_->[2], over $_->[3]\n"
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
		while(@e || @{$this->{ToQueue}}) {
			print "Something in the old queue\n";
			$havevent = 1;
			$this->{Queue} = [];
			@ne = ();
			for my $e (@e) {
				print "Events.pm: old loop while SEND ",
					VRML::Debug::toString($e), "\n"
					;#JAS		if $VRML::verbose::events;
				#AK The following line of code causes problems when a node
				#AK ($e->[0]) needs to have more than one event processed
				#AK for a given EventsProcessed. This causes problems with
				#AK Script nodes in particular since they may have any
				#AK number of event types.
				#AK # don't send same event again
				#AK next if($sent{$e->[0]}{$e->[1]}++);

				# now send it! JAS.
				my $sub;
				if($this->{Listen}{$e->[0]} and
				   $sub = $this->{Listen}{$e->[0]}{$e->[1]}) {
				   	$sub->($e->[2]);
				}

				for(@{$this->{Route}{$e->[0]}{$e->[1]}}) {
					push @{$this->{ToQueue}},
						[ $_->[0], $_->[1], $e->[2] ];
				}
				my $c;
				# Was this eventOut a child of someone?
				if($c = $this->{CIs}{$e->[0]}{$e->[1]}) {

					# Check that it is not a field or eventIn!
					$fk = $c->[0]->{Type}{FieldKinds}{$c->[1]};
					die("Field kind for $fk is not defined") if (!defined $fk);
					if ($fk eq "eventOut" or $fk eq "exposedField") {
						push @ne, [ $c->[0], $c->[1], $e->[2] ];
					}
				}
			}
			my @te = @{$this->{ToQueue}};
			$this->{ToQueue} = [];
			for my $e (@te) {
					push @ne, 
					   $e->[0]->receive_event($e->[1],
							$e->[2], $timestamp);
					$ep{$e->[0]} = $e->[0];
					# Was this event routed to someone
					# who has children?

					for(@{$this->{PIs}{$e->[0]}{$e->[1]}}) {
						$fk = $_->[0]->{Type}{FieldKinds}{$_->[1]};
						die("Field kind for $fk is not defined") if (!defined $fk);

						if ($fk eq "eventIn" or $fk eq "exposedField") {
							push @{$this->{ToQueue}},
								[ $_->[0], $_->[1], $e->[2] ];
						}
					}
			}
			@e = (@ne,@{$this->{Queue}});
		}
		$this->{Queue} = [];
		@ne = ();
		# Call eventsprocessed
		for (values %ep) {
			push @ne,$_->events_processed($timestamp,$be);
		}
		if ($VRML::verbose::events) {
			print "NEWEVENTS:\n";
			for (@ne) {
				print "$_->[0] $_->[1] $_->[2]\n";
			}
		}
		last if(!@ne);
		@e = (@ne,@{$this->{Queue}}); # Here we go again ;)
		$this->{Queue} = [];
	}
	# if we had an event, render the back end.
	if ($havevent > 0) {
		VRML::OpenGL::set_render_frame();
		$havevent -= 1;
	}
}

# This puts an event coming FROM node


sub put_event {
	my ($this, $node, $field, $value) = @_;
	print "put_event\n";
	push @{$this->{Queue}}, [ $node, $field, $value ];
	return;
}

# This sends an event TO node - should be removed for CRoutes.
sub send_event_to {
	my ($this, $node, $field, $value) = @_;
	my $outptr;
	my $datalen;

	print "send_event_to (node $node, field $field...) depreciated\n";
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

	VRML::VRMLFunc::do_bind_to ($node->{TypeName}, $outptr, $bindValue);
}

sub put_events {
	my ($this, $events) = @_;
	# print "Put_events\n";
	for (@$events) {
		die("Invalid put_events event $_\n") if (ref $_ ne "ARRAY");
	}
	push @{$this->{Queue}}, @$events;
}

sub handle_touched {
	my($this, $node, $but, $move, $over) = @_;
	#print "HTOUCH: node $node, but $but, move $move, over $over \n";
	push @{$this->{MouseSensitive}}, [ $node, $but, $move, $over];
}

1;
