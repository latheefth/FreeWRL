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
	# parameters are:
	# 0 - the object ref type (VRML::DEF)
	# 1 - the name of the object
	# 2 - the node of the object
	# print "VRML::DEF, blessing ", $_[1], " and ",VRML::NodeIntern::dump_name($_[2]),"\n";
	my ($type, $name, $node) = @_;
	my $this = bless {
		Name => $name,
		Node => $node
	}, $type;
	return $this;
}

sub copy {
	my ($this, $node) = @_;
	return (ref $this)->new($this->{Name}, $this->{Node}->copy($node));
}

sub make_executable {
	my ($this, $obj) = @_;
	$this->{Node}->make_executable($obj);
}

sub make_backend {
	my ($this, $be, $parentbe) = @_;
	
	print "VRML::DEF::make_backend $this->{Name}, ",
		VRML::NodeIntern::dump_name($this->{Node}),
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

sub get_ref {
	my ($this) = @_;
	$this->{Node};
}

sub real_node {
	my ($this, $obj) = @_;
        # print "(a)in real_node $_ in VRML::DEF, ", $_[0],"\n";
        # print "(a)in real_node1  in VRML::DEF, ", $_[1],"\n";
        # print "(Name)\t\t\t", $_[0][0], "\n";
        # print "(Node)\t\t\t", $_[0][1], "\n";
        # print "ref of Node is\t\t", ref($_[0][1]), "\n";
	return $this->{Node}->real_node($obj);
}

sub initialize {()}

sub as_string {
    my ($this) = @_;
    return 
	" ($this) DEF Name: $this->{Name} Node: $this->{Node} ".
	    $this->{Node}->as_string;
}

sub gather_defs {
	my ($this, $parentnode) = @_;

	# print "VRML::DEF - gather defs, this is ",
	# VRML::NodeIntern::dump_name($this), "name is ",
	# $this->name(), " ref is ", $this->get_ref(),
	# " parent is ", VRML::NodeIntern::dump_name($parentnode),"\n";

	$parentnode->{DEF}{$this->{Name}} = $this->get_ref();
	my $real = $this->get_ref();
	$real->gather_defs($parentnode);
}
	
sub dump {
	my ($this, $level) = @_;
	my $lp = $level*2+2;
	my $padded = pack("A$lp","$level ");

	print $padded,"VRML::DEF, name is ", $this->{Name}," def is ",
		VRML::NodeIntern::dump_name($this->node),"\n";
	my $real =  $this->get_ref();
	$real->dump($level+1);
}


1;
