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

sub set_used {
    my ($this, $name, $node) = @_;
    $this->{DEFName} = $name;
    $this->{DEFNode} = $node;
}

sub make_backend {
    my ($this, $be, $parentbe) = @_;
    print "VRML::USE::make_backend $this $this->{DEFName} ",
	VRML::NodeIntern::dump_name($this->{DEFNode}), ":\n" 
	    if $VRML::verbose::be;

    if ($this->{DEFNode}{BackNode}) {
	print "\tusing $this->{DEFName}'s BackNode.\n"
	    if $VRML::verbose::be;
	return $this->{DEFNode}{BackNode};
    } else {
	print "\tno BackNode associated with $this->{DEFNode}{TypeName}.\n"
	    if $VRML::verbose::be;
	## original code
	return $this->{DEFNode}->make_backend($be, $parentbe);
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

sub real_node { 
    my ($this, $obj) = @_;
    # ok - the following conventions/params are here...
    #
    # appearance DEF Grey Appearance 
    #	{ material Material 
    #		{ emissiveColor .2 .2 .2 specularColor 1.0 0 0 } 
    #	}
    #
    # $_ is the "name" of the DEF... eg, "appearance".
    # $this is a "use" hash, defined and blessed. It contains...
    # $this->{DEFName} name. In this case, "Grey".
    # $this->{DEFNode} the node for the definition. 
    # $this->{Scene} the original scene for the definition,
    #	which doesn't appear anywhere in this package at the moment.

    # print "(a)in real_node $_ in VRML::USE, ", $_[0], $_[0]->as_string(),"\n";
    # print "(Name)                       ", $_[0][0], "\n";
    # print "(Node)                       ", $_[0][1], "\n";
    # print "ref of Node is               ", ref($_[0][1]), "\n";

    return $this->{DEFNode}->real_node($obj); 
}


sub get_ref {
    my ($this) = @_;
    return $this->{DEFNode};
}

sub initialize {()}

sub as_string {
	my ($this) = @_;
	return " ($_) USE Name: $this->{DEFName}  Node: $this->{DEFNode} ";
}

## null procedure
sub gather_defs {}


sub dump { 
	my ($this, $level) = @_;
	my $lp = $level*2+2;
	my $padded = pack("A$lp","$level ");
	print $padded,"$this, name is ", $this->{DEFName},
		" def is ", VRML::NodeIntern::dump_name($this->{DEFNode}),"\n";
}


1;
