# Copyright (C) 1998 Tuomas J. Lukka 1999 John Stewart, Ayla Khan CRC Canada.
# DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
# See the GNU Library General Public License (file COPYING in the distribution)
# for conditions of use and redistribution.
#
# $Id$
#
# Useful for verbose/debugging statements.

use strict vars;

require 'VRML/NodeIntern.pm';


package VRML::Debug;
use File::Basename;

sub toString {
	my ($dt, $offset) = @_;

	return "NULL" if (!defined $dt);
	return "\"\"" if ($dt eq "");

	my $ref = ref $dt;
	my $key;
	my $value;

	return $dt if (!$ref);

	if ($ref eq "HASH") {
		my @str;
		my $tab1 = "\t";
		my $tab2 = "";
		my $i;

		return "{} $dt" if (!%{$dt});
		for($i = 0; $i < $offset; $i++) {
			$tab1 .= "\t";
			$tab2 .= "\t";
		}

		push @str, "{";
		while (($key, $value) = each %{$dt}) {
			if ($key =~ /Intern/) {
				$key = VRML::NodeIntern::dump_name($key);
			} else {
				$key = toString($key);
			}
			$value = toString($value, $offset+1);
			push @str, "$tab1$key => $value,";
		}
		push @str, "$tab2} $dt";
		return join("\n", @str);
	} elsif ($ref eq "ARRAY") {
		return "[] $dt" if (!@{$dt});

		return "[ ".join(", ", map(toString($_), @{$dt}))." ] $dt";
	} elsif ($ref eq "VRML::DEF") {
		return "$dt { $dt->{Name}, ".VRML::NodeIntern::dump_name($dt->{Node}).", $dt->{VRMLName} }";
	} elsif ($ref eq "VRML::USE") {
		return "$dt { $dt->{DEFName}, ".toString($dt->{DEFNode})." }";
	} elsif ($ref eq "VRML::IS") {
		return "$dt { $dt->{Name}, ".toString($dt->{Ref}).", ".toString($dt->{ISField})." }";
	} elsif ($ref =~ /(Intern|Scene|Type)/) {
		return VRML::NodeIntern::dump_name($dt);
	} else {
		return "$ref ".$dt;
	}
}


## untested
sub stackTrace {
	my $frame = 0;
	my $basename;
	my @str;
	while (my ($package, $filename, $line, $subroutine, $hasargs,
			   $wantarray, $evaltext, $is_require, $hints, $bitmask) =
		   caller($frame)) {
		$basename = basename($filename);
		push @str, "frame ".$frame++.": $package, $basename, $subroutine, ".($wantarray ? "wantarray, " : "")."line $line";
	}
	return join("\n", @str);
}


1;
