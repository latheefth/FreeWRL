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

sub add_route {
	my($this, $fromNode, $eventOut, $toNode, $eventIn) = @_;

	my $outoffset;
	my $inoffset;
	my $outptr;
	my $inptr;
	my $datalen;

	# are there backends made for both from and to nodes?
	if (!defined $fromNode->{BackNode}) {
		print "add_route, from $eventOut - no backend node\n";
		return 1;
	}
	if (!defined $toNode->{BackNode}) {
		print "add_route, to $eventin - no backend node\n";
		return 1;
	}

	# are there backend CNodes made for both from and to nodes?
	if (!defined ($outptr=$fromNode->{BackNode}{CNode})) {
		print "add_route, from $eventOut - no backend CNode node\n";
		return 1;
	}
	if (!defined ($inptr=$toNode->{BackNode}{CNode})) {
		print "add_route, to $eventin - no backend CNode node\n";
		return 1;
	}


	## ElevationGrid, Extrusion, IndexedFaceSet and IndexedLineSet
	## eventIns (see VRML97 node reference)
	## these things have set_xxx and xxx... if we have one of these...
	if ($eventIn =~ /^set_($VRML::Error::Word+)/) {
		$tmp = $1;
		if ($toNode->{Type}{EventIns}{$eventIn} and
			$toNode->{Type}{FieldKinds}{$tmp} eq "field") {
			$eventIn = $tmp;
		}
	}


	# are there offsets for these eventins and eventouts?
	if(!defined ($outoffset=$VRML::CNodes{$fromNode->{TypeName}}{Offs}{$eventOut})) {
		print "add_route, eventout $eventOut offset not defined\n";
		return 1;
	}
	if(!defined ($inoffset=$VRML::CNodes{$toNode->{TypeName}}{Offs}{$eventIn})) {
		print "add_route, eventin $eventIn offset not defined\n";
		return 1;
	}

	# length of the field
	$datalen=VRML::VRMLFunc::getClen(
		"VRML::Field::$fromNode->{Type}{FieldTypes}{$eventOut}"->clength($eventOut));

	# do we handle this type of data within C yet?
	if ($datalen==0) {
		print "add_route, dont handle $eventOut types in C yet\n";
		return 1;
	}

	# is this an interpolator that is handled by C yet?
	$intptr = VRML::VRMLFunc::InterpPointer($toNode->{Type}{Name});

	VRML::VRMLFunc::do_CRoutes_Register($outptr, $outoffset, $inptr, $inoffset, $datalen,
		$intptr);
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

	for(values %{$this->{First}}) {
		print "GETFIRST ", VRML::NodeIntern::dump_name($_), " $_\n" 
			if $VRML::verbose::events;
		push @e, $_->get_firstevent($timestamp);
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
				$_->[1],$timestamp,$_->[3]);
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
	print "Put_events\n";
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
