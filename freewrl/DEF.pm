# Copyright (C) 1998 Tuomas J. Lukka 1999 John Stewart CRC Canada
# DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
# See the GNU Library General Public License (file COPYING in the distribution)
# for conditions of use and redistribution.
#
# $Id$
#
# Package to handle nodes given a name using the DEF keyword.

use strict vars;

package VRML::DEF;


sub new {
	my ($type, $name, $node, $vrmlname) = @_;
	my $this = bless {
		Name => $name,
		Node => $node,
		VRMLName => $vrmlname
	}, $type;

	return $this;
}

sub copy {
	my ($this, $node) = @_;

	return (ref $this)->new($this->{Name}, $this->{Node}->copy($node), $this->{VRMLName});
}

sub make_executable {
	my ($this, $obj) = @_;
	$this->{Node}->make_executable($obj);
}

sub make_backend {
	my ($this, $be, $parentbe) = @_;
	
	print "VRML::DEF::make_backend: ",
		VRML::Debug::toString($this),
				", $this->{Node}{TypeName}\n"
					if $VRML::verbose::be;

	# use the node's make_backend
	return $this->{Node}->make_backend($be, $parentbe);
}

sub iterate_nodes {
	my ($this, $sub, $parent) = @_;
	&$sub($this, $parent);
	$this->{Node}->iterate_nodes($sub, $parent);
}

sub name {
	my ($this) = @_;
	return $this->{Name};
}

sub node {
	my ($this) = @_;
	return $this->{Node};
}

sub real_node {
	#AK - #my ($this, $proto) = @_;
	my ($this) = @_;

	#AK - #return $this->{Node}->real_node($proto);
	return $this->{Node}->real_node();
}

sub initialize {()}

sub as_string {
    my ($this) = @_;
    return $this->{Node}->as_string();
}
sub gather_defs {
	my ($this, $parentnode) = @_;

	#JAS - this is not the DEF we want $parentnode->{DEF}{$this->{Name}} = $this->get_ref();

	my $real = $this->{Node};
	$real->gather_defs($parentnode);
}
	
sub dump {
	my ($this, $level) = @_;
	my $lp = $level*2+2;
	my $padded = pack("A$lp","$level ");

	print $padded,"VRML::DEF, name is ", $this->{Name}," def is ",
		VRML::NodeIntern::dump_name($this->node),"\n";

	my $real = $this->{Node};
	$real->dump($level+1);
}


1;
