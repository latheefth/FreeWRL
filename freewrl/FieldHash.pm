# Copyright (C) 1998 Tuomas J. Lukka 1999 John Stewart CRC Canada
# DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
# See the GNU Library General Public License (file COPYING in the distribution)
# for conditions of use and redistribution.
#
# $Id$
#
# The FieldHash
#
# This is the object behind the "RFields" hash member of
# the object VRML::NodeIntern. It allows you to send an event by
# simply saying "$node->{RFields}{xyz} = [3,4,5]" for which 
# calls the STORE method here which then queues the event.
#
# XXX This needs to be separated into eventins and eventouts --
# assigning has different meanings.

use strict vars;

package VRML::FieldHash;
@VRML::FieldHash::ISA=Tie::StdHash;

sub TIEHASH {
	my ($type, $node) = @_;
	bless \$node, $type;
}

{
my %DEREF = map {($_=>1)} qw/VRML::IS/;
my %REALN = map {($_=>1)} qw/VRML::DEF VRML::USE/;
sub FETCH {
	my ($this, $k) = @_;
	my $node = $$this;
	my $v = $node->{Fields}{$k};

# print "FETCH, this $this, node ",VRML::NodeIntern::dump_name($node), " v $v k $k\n";

	if ($VRML::verbose::tief) {
		print "TIEH: FETCH $k $v\n" ;
		if ("ARRAY" eq ref $v) {
			print "TIEHARRVAL: @$v\n";
		}
	}
	# while($DEREF{ref $v}) {
	#	$v = ${$v->get_ref};
	#	print "DEREF: $v\n" if $VRML::verbose::tief;
	#}

	while ($REALN{ref $v}) {
		$v = $v->real_node;
		print "TIEH: MOVED TO REAL NODE: $v\n"
			if $VRML::verbose::tief;
	}
	if (ref $v eq "VRML::IS") {
		print "Is should've been dereferenced by now -- something's cuckoo\n";
		exit (1);
	}
	return $v;
}

sub STORE {
	my ($this, $k, $value) = @_;
	if ($VRML::verbose::tief) {
		print "TIEH: STORE Node $this K $k value $value\n" ;
		if ("ARRAY" eq ref $value) {
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
	  if (defined $node->{BackNode}) { 
		print "STORE, BackNode defined\n"  if $VRML::verbose::events;
		$node->set_backend_fields($k);
	  }
	}
}
}

sub FIRSTKEY {
	return undef
}

1;
