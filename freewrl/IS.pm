# Copyright (C) 1998 Tuomas J. Lukka 1999 John Stewart CRC Canada
# DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
# See the GNU Library General Public License (file COPYING in the distribution)
# for conditions of use and redistribution.
#
# $Id$
#
# Package to handle IS statements in prototype definitions.
#
# The implementation here may change for efficiency later.
#
# However, the fact that we go through these does not usually
# make performance too bad since it only affects us when there
# are changes of the scene graph or IS'ed field/event values.

use strict vars;
package VRML::IS;


sub new {
	my ($type, $name) = @_;
	my $this = bless {
		Name => $name
	}, $type;
	return $this;
}

sub copy {
	my ($this) = @_;
	my $a = $this->{Name}; 
	bless { Name => $a }, ref $this;
}

sub make_executable {
	my ($this, $scene, $node, $field) = @_;
}

sub iterate_nodes {
	my ($this, $sub) = @_;
	&$sub($this);
}

sub name { 
	my ($this) = @_;
	return $this->{Name};
}

sub set_ref {
	my ($this, $ref) = @_;
	$this->{Ref} = $ref;
}

sub get_ref {
	my ($this) = @_;
	if (!defined $this->{Ref}) {
		print "IS not def!\n";
		exit(1);
	}
	return $this->{Ref};
}

sub initialize {()}

sub as_string {
	my ($this) = @_;
	## " ($_) IS $_[0][0] "
	return " ($_) IS $this->{Name} ";
}

## null procedure
sub gather_defs {}

sub dump {
	my ($this, $level) = @_;
	my $lp = $level*2+2;
	my $padded = pack("A$lp","$level ");
	print "$padded node $this IS ", $this->{Name},"\n";
}


1;
