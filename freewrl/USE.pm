# Copyright (C) 1998 Tuomas J. Lukka 1999 John Stewart CRC Canada
# DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
# See the GNU Library General Public License (file COPYING in the distribution)
# for conditions of use and redistribution.
#
# $Id$
#
# Package to handle nodes referenced using the USE keyword.

use strict vars;

package VRML::USE;
sub new {
	my ($type, $defname, $defnode) = @_;
	my $this = bless {
		DEFName => $defname,
		DEFNode => $defnode
	}, $type;
	return $this;
}

sub copy {
	my ($this) = @_;
	(ref $this)->new($this->{DEFName}, $this->{DEFNode});
}

## null procedure
sub make_executable {}

# replace the definitions with new ones; especially for the node - it
# should be dereferenced by this time.
sub set_used {
	my ($this, $name, $node) = @_;
	print "VRML::USE::set_used: ", VRML::Debug::toString(\@_), "\n"
		if $VRML::verbose::scene;
	$this->{DEFName} = $name;
	$this->{DEFNode} = $node;
}

sub make_backend {
	my ($this, $be, $parentbe) = @_;

	if ($VRML::verbose::be) {
		my ($package, $filename, $line) = caller;
		print "VRML::USE::make_backend: ", VRML::Debug::toString(\@_),
			" from $package, $line\n";
	}

	my $dn = $this->{DEFNode};

	# this is a DEF here
	if ($dn->{Node}{BackNode}{CNode}) {
		print "\tusing $this->{DEFName}{Node}'s BackNode.\n"
			if $VRML::verbose::be;
		return $this->{DEFNode}{Node}{BackNode};
	} else {
	    	# maybe this is a NodeIntern?
		print "\tno BackNode associated with $this->{DEFNode}{Node}{TypeName}.\n"
			if $VRML::verbose::be;
		return $this->{DEFNode}{Node}->make_backend($be, $parentbe);
	}
}

sub iterate_nodes {
	my ($this, $sub, $parent) = @_;
	&$sub($this, $parent);
}

sub name {
	my ($this) = @_;
	return $this->{DEFName};
}

sub node {
	my ($this) = @_;
	if (!defined  ($this->{DEFNode})) {
		my $msg = "USE name: ", $this->{DEFName}, " not DEF'd\n".  "Unrecoverable error; FreeWRL has to exit. \n";
	    VRML::VRMLFunc::ConsoleMessage($msg); 
	    exit(1);
	}

	return $this->{DEFNode}->node();
}

sub real_node {
	my ($this) = @_;
	return $this->{DEFNode}->real_node();
}

sub initialize { return (); }

sub as_string {
	my ($this) = @_;
	return $this->{DEFNode}->as_string();
}

## null procedure
sub gather_defs {}


sub dump {
	my ($this, $level) = @_;
	my $lp = $level*2+2;
	my $padded = pack("A$lp","$level ");
	print $padded,"$this, name is ", $this->{DEFName},
		" def is ", VRML::NodeIntern::dump_name($this->{DEFNode}{Node}),"\n";
}
1;
