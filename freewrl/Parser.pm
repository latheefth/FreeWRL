#
# $Id$
#
# Copyright (C) 1998 Tuomas J. Lukka 1999 John Stewart CRC Canada.
# DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
# See the GNU Library General Public License (file COPYING in the distribution)
# for conditions of use and redistribution.
#
# Parser.pm -- implement a VRML parser
#

use strict vars;

$VRML::verbose::parse=0;

package VRML::Error;
use vars qw/@ISA @EXPORT $Word $qre $cre $Float $Integer/;
require Exporter;
@ISA=qw/Exporter/;

@EXPORT=qw/parsefail parsewarnstd $Word $qre $cre $Float $Integer/;

# Define the RE for a VRML word.
# $Word = q|[^\-+0-9"'#,\.\[\]\\{}\0-\x20][^"'#,\.\{\}\\{}\0-\x20]*|;

## Bug 424524:
## It was reported that there was some difficulty parsing VRML words when the
## character > occurred at the end of a word.
##
## The problem lay in the usage of the Perl assertion \b to designate a word
## boundary : \b is the position between \w and \W, either \W\w at the beginning
## of a word or \w\W at the end. Characters such as >, while legal in VRML97,
## are not included in \w, causing the truncation of the word.
##
## This bug was fixed by including the possibility of a premature word boundary:

$Word = qr|[^\x30-\x39\x0-\x20\x22\x23\x27\x2b\x2c\x2d\x2e\x5b\x5c\x5d\x7b\x7d\x7f][^\x0-\x20\x22\x23\x27\x2c\x2e\x5b\x5c\x5d\x7b\x7d\x7f]*(?:\b[^\x0-\x20\x22\x23\x27\x2c\x2e\x5b\x5c\x5d\x7b\x7d\x7f])?|;

$qre = qr{(?<!\\)\"};		# " Regexp for unquoted double quote
$cre = qr{[^\"\n]};		# " Regexp for not dquote, not \n char

$Float = qr~[\deE+\-\.]+~;
$Integer = qr~[\da-fA-FxX+\-]+~;

sub parsefail {
	my $p = pos $_[0];
	my $n = ($p>=50 ? 50 : $p);
	my $textb = substr($_[0],$p-$n,$n);
	my $texta = substr($_[0],$p,50);

	VRML::VRMLFunc::ConsoleMessage ("PARSE ERROR: '$textb' XXX '$texta', $_[1] $_[2]\n");
	goto PARSE_EXIT;
}

sub parsewarnstd {
	my $p = pos $_[0];
	my $n = ($p>=100 ? 100 : $0);
	my $textb = substr($_[0],$p-$n,$n);
	warn("Parse warning: nonstandard feature: '$textb': $_[1]");
}

require 'VRML/VRMLNodes.pm';
require 'VRML/VRMLFields.pm';

package VRML::Parser;
use vars qw/$Word $qre $cre/;
VRML::Error->import;


my $Chars = qr/(?:[\x00-\x21\x23-\x5b\x5d-\xffff]*|\x5c\x22|\x5c{2}[^\x22])*/o;

# Parse a whole file into $scene.
sub parse {
	my($scene, $text) = @_;

	my @a;
	my ($n, $r);
	# my ($package, $filename, $line) = caller;
	# print "--- sub parse from $package, $line\n";
	while($text !~ /\G\s*$/gsc) {
		# any valid child node can be a top node.
		$n = parse_statement($scene,$text,0,"topNodes");

		$r = ($text =~ /\G\s*,\s*/gsc); # Eat comma if it is there...
		if(defined $n) {push @a, $n}
		# print "parsing statement, my n is $n array is ", scalar(@a), " long\n";
	}
	PARSE_EXIT:
	$scene->topnodes(\@a);
}

# Parse a statement, return a node if it is a node, otherwise
# return undef.
sub parse_statement { # string in $_[1]
	my($scene, $text, $justcheck, $parentNode) = @_;

	## commas again
	$_[1] =~ /\G\s*,\s*/gsc;

	my $justcheck = $_[2];

	print "PARSE: parse_statement; justcheck $justcheck parentNode $parentNode '",substr($_[1],pos $_[1]),"'\n"
		if $VRML::verbose::parse;
	# Peek-ahead to see what is to come... store pos.
	my $p = pos $_[1];
	print "POSN: $p\n"
		if $VRML::verbose::parse;
	if($_[1] =~ /\G\s*EXTERNPROTO\b/gsc) {
		(pos $_[1]) = $p;
		parse_externproto($scene,$_[1]);
		return undef;
	} elsif($_[1] =~ /\G\s*PROTO\b/gsc) {
		(pos $_[1]) = $p;
		parse_proto($scene,$_[1]);
		return undef;
	} elsif($_[1] =~ /\G\s*PROFILE\b/gsc) {
		(pos $_[1]) = $p;
		parse_PROFILE($scene, $_[1]);
		return undef;
	} elsif($_[1] =~ /\G\s*META\b/gsc) {
		(pos $_[1]) = $p;
		parse_META($scene, $_[1]);
		return undef;
	} elsif($_[1] =~ /\G\s*COMPONENT\b/gsc) {
		(pos $_[1]) = $p;
		parse_COMPONENT($scene, $_[1]);
		return undef;
	} elsif($_[1] =~ /\G\s*ROUTE\b/gsc) {
		(pos $_[1]) = $p;
		parse_route($scene,$_[1]);
		return undef;
	} elsif($justcheck == 1) {
		return -1;
	} elsif($_[1] =~ /\G\s*($Word)/gsc) {
		(pos $_[1]) = $p;
		print "AND NOW: ",(pos $_[1]),"\n"
			if $VRML::verbose::parse;
		#my ($package, $filename, $line) = caller;
		#print "--- sub parsestatement, parentnode $parentNode  from $package, $line\n";
		return VRML::Field::SFNode->parse($scene,$_[1],$parentNode);
	} else {
		print "WORD WAS: '$Word'\n"
			if $VRML::verbose::parse;
		parsefail($_[1],"Can't find next statement");
	}
}

#parse a PROFILE statement in a VRML file. Dont do anything with it yet.
sub parse_PROFILE  {
	my($scene) = @_;
	$_[1] =~ /\G\s*PROFILE\s+($Word)\s*/ogsxc
	 or parsefail($_[1], "PROFILE statement");
	#my $name = $1;
	#print "parse profile:", $name,"\n";
}

#parse a COMPONENT statement in a VRML file. Dont do anything with it yet.
sub parse_COMPONENT  {
	my($scene) = @_;
	$_[1] =~ /\G\s*COMPONENT\s*/ogsxc
		 or parsefail($_[1], "COMPONENT statement");

        $_[1] =~ /\G\s*\x22($Chars)\x22\s*/g
                or VRML::Error::parsefail($_[1], "improper SFString");

        $_[1] =~ /\G\s*\x22($Chars)\x22\s*/g
                or VRML::Error::parsefail($_[1], "improper SFString");
}

#parse a META statement in a VRML file. Dont do anything with it yet.
sub parse_META  {
	my($scene) = @_;
	$_[1] =~ /\G\s*META\s*/ogsxc
		 or parsefail($_[1], "META statement");

        $_[1] =~ /\G\s*\x22($Chars)\x22\s*/g
                or VRML::Error::parsefail($_[1], "improper SFString");

        $_[1] =~ /\G\s*\x22($Chars)\x22\s*/g
                or VRML::Error::parsefail($_[1], "improper SFString");
}

sub parse_proto {
	my($scene) = @_;
	$_[1] =~ /\G\s*PROTO\s+($Word)\s*/ogsxc
	 or parsefail($_[1], "proto statement");
	my $name = $1;

	# print "parse_proto NAME:", $name,"\n";

	my $int = parse_interfacedecl($scene,1,1,$_[1]);
	$_[1] =~ /\G\s*{\s*/gsc or parsefail($_[1], "proto body start");
	my $pro = $scene->new_proto($name, $int);

	my @a;
	while($_[1] !~ /\G\s*}\s*/gsc) {
		my $n = parse_statement($pro,$_[1],0,"protoTop");
		if(defined $n) {push @a, $n}
		# print "parseproto, pushing $n to array a\n"; 
	}
	# print "parse_proto, setting topnodes for ",VRML::NodeIntern::dump_name($pro),"\n";
	# print "parse_proto, a is ";
	# foreach (@a) {print "$_ \n"; }
	
	# make the top nodes be encased within a group; this helps with displaying only first child...
	$pro->prototopnodes(\@a);

# print "parse_proto, topnodes \n";
# foreach (@{$pro->{Nodes}}) {
# 	print "node ", VRML::NodeIntern::dump_name($_),"\n";
# 	}
# print "parse_proto, finshed printing topnodes\n";

}

sub parse_externproto {
	my ($scene) = @_;
	my $name = $_[2];

	if ($name) {
		my $regex = qr{$name};
		$_[1] =~ /\s*EXTERNPROTO\s+($regex)/gsxc
			or parsefail($_[1], "externproto statement - looking for $name");
	} else {
		$_[1] =~ /\G\s*EXTERNPROTO\s+($Word)\s*/ogsxc
			or parsefail($_[1], "externproto statement");
		$name = $1;
	}

	my $int = parse_interfacedecl($scene,1,0,$_[1]);
	my $str = VRML::Field::MFString->parse($scene,$_[1]);
	my $pro = $scene->new_externproto($name, $int,$str);
}

# Returns:
#  [field, SVFec3F, foo, [..]]
sub parse_interfacedecl {
	#print "parse_interfacedecl begin...\n";
	my($scene,$exposed,$fieldval,$s, $script,$open,$close) = @_;
	$open = ($open || "\\[");
	$close = ($close || "\\]");
	print "VRML::Parser::parse_interfacedecl: terminal symbols are '$open' '$close'\n"
		 if $VRML::verbose::parse;
	$_[3] =~ /\G\s*$open\s*/gsxc or parsefail($_[3], "interface declaration");
	my %f;
	while($_[3] !~ /\G\s*$close\s*/gsxc) {
		print "VRML::Parser::parse_interfacedecl: "
		 	if $VRML::verbose::parse;
		# Parse an interface statement
		if($_[3] =~ /\G\s*(eventIn|eventOut|inputOnly|outputOnly)\s+
			  ($Word)\s+($Word)\s+/ogsxc) {
			# x3dv scripting
			my $access = $1;
			if ($access eq "inputOnly") {$access = "eventIn";}
			if ($access eq "outputOnly") { $access = "eventOut";}

			print "PARSING 1: $access $2 $3\n"
				if $VRML::verbose::parse;
			$f{$3} = [$access,$2];
			my $n = $3;
			if($script and
			   $_[3] =~ /\G\s*IS\s+($Word)\s+/ogsc) {
				print "PARSING 1.1: A:$1 B:$2 C:$3, N:$n\n"
				      if $VRML::verbose::parse;
			   	push @{$f{$n}}, $scene->new_is($1, $n);
				print "Is SCRIPT.\n" if $VRML::verbose::parse;
			}
		} elsif($_[3] =~ /\G\s*(field|exposedField|inputOutput|initializeOnly)\s+
			  ($Word)\s+($Word)/ogsxc) {
			  # x3dv parsing...
			  my $access = $1;
			  if ($access eq "inputOutput") { $access = "exposedField"; }
			  if ($access eq "initializeOnly") { $access = "field"; }

			  if($access eq "exposedField" and !$exposed) {
			  	parsefail($_[3], "interface",
					   "exposedFields not allowed here");
			  }
			my($ft, $t, $n) = ($access, $2, $3);
			print "PARSING 2: $ft $t $n $fieldval\n"
				if $VRML::verbose::parse;
			warn "$ft $t $n redeclared\n" if exists $f{$n};
			$f{$n} = [$ft, $t];
			if($fieldval) {
				if($_[3] =~ /\G\s*IS\s+($Word)/gsc) {
					push @{$f{$n}}, $scene->new_is($1, $n);
				} else {
					push @{$f{$n}},
					  "VRML::Field::$t"->parse($scene,$_[3],"protoTop");
				}
			}
		} elsif($script && $_[3] =~ /\G\s*(url|directOutput|mustEvaluate)\b/gsc) {
			my $f = $1;
			my $ft = $VRML::Nodes{Script}->{FieldTypes}{$1};
			my $eft = ($f eq "url" ? "exposedField":"field");
			print "Script field $f $ft $eft\n"
		 		if $VRML::verbose::parse;
			if($_[3] =~ /\G\s*IS\s+($Word)/gsc) {
				$f{$f} = [$ft, $f, $scene->new_is($1, $f)];
			} else {
				$f{$f} = [$ft, $f, "VRML::Field::$ft"->parse($scene,$_[3])];
				print "\tparsed $f $ft $eft\n"
		 			if $VRML::verbose::parse;
			}
		} else {
			parsefail($_[3], "interface");
		}
	}
	#print "parse_interfacedecl end...\n";
	return \%f;
}

sub parse_route {
	my($scene) = @_;
	$_[1] =~ /\G
		\s*ROUTE\b
		\s*($Word)\s*\.
		\s*($Word)\s+(TO\s+|to\s+|)
		\s*($Word)\s*\.
		\s*($Word)
	/ogsxc or parsefail($_[1], "route statement");
	# remember - we have our own internal names for these things...
	my $from = VRML::Handles::return_def_name($1);
	my $eventOut = $2;
	my $to = VRML::Handles::return_def_name($4);
	my $eventIn = $5;

	if ($3 !~ /TO\s+/) {
		parsewarnstd($_[1],
		   "lowercase or omission of TO");
	}

	my $fromNode = $scene->getNode($from);
	$eventOut = parse_exposedField($eventOut, $fromNode->{Type});

	my $toNode = $scene->getNode($to);
	$eventIn = parse_exposedField($eventIn, $toNode->{Type});

	$scene->new_route($from, $eventOut, $to, $eventIn);
}

sub parse_script {
	my($scene) = @_;
	my $i = parse_interfacedecl($scene, 0,1,$_[1],1 ,'{','}');

	return $scene->new_node("Script", $i); # Scene knows that Script is different
}


sub parse_exposedField {
	my ($field, $nodeType) = @_;
	my $tmp;
	if ($field =~ /^set_($VRML::Error::Word+)/) {
		$tmp = $1;
		if ($nodeType->{EventIns}{$tmp} and
			$nodeType->{FieldKinds}{$tmp} =~ /^exposed/) {
			$field = $tmp;
		}
	}

	if ($field =~ /($VRML::Error::Word+)_changed$/) {
		$tmp = $1;
		if ($nodeType->{EventOuts}{$tmp} and
			$nodeType->{FieldKinds}{$tmp} =~ /^exposed/) {
			$field = $tmp;
		}
	}
	return $field;
}

1;
