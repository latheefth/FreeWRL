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
	},$type;
}

sub print {
	my($this) = @_;
	return unless $VRML::verbose::events;
	print "DUMPING EVENTMODEL\nFIRST:\n";
	for(values %{$this->{First}}) {
		print "\t",VRML::NodeIntern::dump_name($_),":\t$_->{TypeName}\n";
	}
	print "ROUTES:\n";
	for my $fn (keys %{$this->{Route}}) {
		print "\t",VRML::NodeIntern::dump_name($fn)," $this->{Route}{$fn}{TypeName}\n";
		for my $ff (keys %{$this->{Route}{$fn}}) {
			print "\t\t$ff\n";
			for (@{$this->{Route}{$fn}{$ff}}) {
				print "\t\t\t",VRML::NodeIntern::dump_name($_->[0]),":\t$_->[0]{TypeName}\t$_->[1]\n";
			}
		}
	}
	print "ISS:\n";
	for my $pn (keys %{$this->{PIs}}) {
		print "\t",VRML::NodeIntern::dump_name($pn)," $this->{PIsN}{$pn}{TypeName}\n";
		for my $pf (keys %{$this->{PIs}{$pn}}) {
			print "\t\t$pf\n";
			for(@{$this->{PIs}{$pn}{$pf}}) {
				print "\t\t\t$_->[0]:\t$_->[0]{TypeName}\t$_->[1]\n";
			}
		}
	}
}

# XXX Softref
sub add_first {
	my($this,$node) = @_;
	print "Events.pm add_first ", VRML::NodeIntern::dump_name($node),"\n"
		if $VRML::verbose::events;
	$this->{First}{$node} = $node;

}

sub remove_first {
	my($this,$node) = @_;
	delete $this->{First}{$node};
}

sub add_route {
	my($this,$fn, $ff0, $tn, $tf0) = @_;
	print "ADD_ROUTE $fn $ff $tn $tf\n" if $VRML::verbose::events;
	$ff = $fn->{Type}{EventOuts}{$ff0};
	$tf = $tn->{Type}{EventIns}{$tf0};
	print "MAPPED: $ff, $tf\n" if $VRML::verbose::events;
	if(!defined $ff) {
		die("Invalid fromfield '$ff0' for ROUTE");
	}
	if(!defined $tf) {
		die("Invalid tofield '$tf0' for ROUTE");
	}

	# is this route already here? If so, then don't bother adding it to
	# the event loop, again. This situation can happen with the EAI,
	# because we can add/delete nodes/routes in a pseudo random fashon.
	# we don't want to loose the routes associated with a particular
	# scene, but we don't want scene::new_route to be able to duplicate
	# routes, either.

	foreach (@{$this->{Route}{$fn}{$ff}}) {
		# print "Events: add_route: route: ",@{$_},"\n";
		my ($n, $f) = @{$_};
		if ($n eq $tn) {
			if ($f eq $tf) {
				return;
			}
		} 
	}
	push @{$this->{Route}{$fn}{$ff}}, [$tn, $tf]
}

sub delete_route {
	my($this,$fn, $ff0, $tn, $tf0) = @_;
	print "DELETE_ROUTE $fn $ff $tn $tf\n"  if $VRML::verbose::events;
	$ff = $fn->{Type}{EventOuts}{$ff0};
	$tf = $tn->{Type}{EventIns}{$tf0};
	print "delete_route: MAPPED: $ff, $tf\n" if $VRML::verbose::events;
	if(!defined $ff) {
		die("Invalid fromfield '$ff0' for ROUTE");
	}
	if(!defined $tf) {
		die("Invalid tofield '$tf0' for ROUTE");
	}
	print "delete_route: current routes: \n";
	foreach (@{$this->{Route}{$fn}{$ff}}) {
		print "delete_route: route: ",@{$_},"\n";
	}
	pop @{$this->{Route}{$fn}{$ff}}, [$tn, $tf]
}

sub add_is_out {
	my($this,$pn, $pf, $cn, $cf) = @_;
	# print "Event.pm : add_is_out - this $this pn $pn pf $pf cn $cn cf $cf\n";
	$this->{PIsN}{$pn} = $pn;
	$this->{CIs}{$cn}{$cf} = [$pn,$pf];
}

sub add_is_in {
	my($this,$pn, $pf, $cn, $cf) = @_;
	# print "Event.pm : add_is_in - this $this pn $pn pf $pf cn $cn cf $cf\n";
	$this->{PIsN}{$pn} = $pn;
	push @{$this->{PIs}{$pn}{$pf}}, [$cn,$cf];
}

sub register_listener {
	my($this, $node, $field, $sub) = @_;
	$this->{Listen}{$node}{$field} = $sub;
}

# get_firstevent returns [$node, fieldName, value]
sub propagate_events {
	my($this,$timestamp,$be,$scene) = @_;
	my @e;
	my @ne;
	my %sent; # to prevent sending twice, always set bit here
	for(values %{$this->{First}}) {
		print "GETFIRST $_\n" 
			if $VRML::verbose::events;
		push @e, $_->get_firstevent($timestamp);
	}
	for(@{$this->{Mouse}}) {
		print "MEV $_->[0] $_->[1] $_->[2]\n"
			if $VRML::verbose::events;
		$_->[0]->{Type}{Actions}{__mouse__}->($_->[0], $_->[0]{RFields},
			$timestamp,
			$_->[2], $_->[1], $_->[3], @{$_}[4..6]);
	}
	my $n = scalar @e;
	push @e, @{$this->{Queue}};
	$this->{Mouse} = [];
	print "GOT ",scalar(@e)," FIRSTEVENTS ($n n/q)\n" if $VRML::verbose::events;

	while(1) {
		my %ep; # All nodes for which ep must be called
		# Propagate our events as long as they last
		while(@e || @{$this->{ToQueue}}) {
			$havevent = 1;
			$this->{Queue} = [];
			@ne = ();
			for my $e (@e) {

				if($VRML::verbose::events) {
					print "Events.pm:while:SEND $e->[0] $e->[0]{TypeName} $e->[1] $e->[2]\n" ;
					if("ARRAY" eq ref $e->[2]) {
						print "Events.pm:while:ARRAYVAL: @{$e->[2]}\n";
					}
				}
				# don't send same event again
				next if($sent{$e->[0]}{$e->[1]}++);

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
					print "CHILD_IS! Send from P\n"
					 if $VRML::verbose::events;
					# Check that it is not a field
					# or eventIn!
					my $fk = $c->[0]->{Type}{FieldKinds}{$c->[1]};
					if(!defined $fk) {die("Fieldkind getting")}
					if($fk eq "eventOut" or 
					   $fk eq "exposedField") {
						push @ne, [
							$c->[0], $c->[1], $e->[2]
						];
					}
				}
			}
			my @te = @{$this->{ToQueue}};
			$this->{ToQueue} = [];
			for my $e (@te) {
					push @ne, 
					   $e->[0]->receive_event($e->[1],
							$e->[2],$timestamp);
					$ep{$e->[0]} = $e->[0];
					# Was this event routed to someone
					# who has children?
					for(@{$this->{PIs}{$e->[0]}{$e->[1]}}) {
						print "Events.pm:P_IS: send to $_\n"
						 if $VRML::verbose::events;
						my $fk = $_->[0]->{Type}{FieldKinds}{$_->[1]};
						# print"Events.pm - fk $fk one ",$_->[1], " , e2 ", $e->[2],"\n";
						if(!defined $fk) {die("Fieldkind getting")}
						if($fk eq "eventIn" or 
						   $fk eq "exposedField") {
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
		for(values %ep) {
			push @ne,$_->events_processed($timestamp,$be);
		}
		if($VRML::verbose::events) {
			print "NEWEVENTS:\n";
			for(@ne) {
				print "$_->[0] $_->[1] $_->[2]\n";
			}
		}
		if(!@ne) {last}
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
	my($this,$node,$field,$value) = @_;
	push @{$this->{Queue}}, [$node, $field, $value];
	return;
}

# This sends an event TO node
sub send_event_to {
	my($this,$node,$field,$value) = @_;
	# print "Events.pm:send_event_to, pushing $node, $field $value\n";
	push @{$this->{ToQueue}}, [$node, $field, $value];
}

sub put_events {
	my($this,$events) = @_;
	print "Put_events\n"
		if $VRML::verbose::events;
	for(@$events) {
		if(ref $_ ne "ARRAY") {
			die("Invalid put_events event: '$_'\n");
		}
	}
	push @{$this->{Queue}}, @$events;
}

sub handle_touched {
	my($this,$node,$but,$move,$over,$pos,$norm,$texc) = @_;
	# print "HTOUCH: $node $but $move\n";
	push @{$this->{Mouse}}, [$node, $but, $move,$over,$pos,$norm,$texc];
}

1;
