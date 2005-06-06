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

# Parse a whole file into $scene.
sub parse {
	my($scene, $text) = @_;

	my @a;
	my ($n, $r);
	while($text !~ /\G\s*$/gsc) {
		# any valid child node can be a top node.
		$n = parse_statement($scene,$text,0,"children");

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
		return VRML::Field::SFNode->parse($scene,$_[1],$parentNode);
	} else {
		print "WORD WAS: '$Word'\n"
			if $VRML::verbose::parse;
		parsefail($_[1],"Can't find next statement");
	}
}

sub parse_proto {
	my($scene) = @_;
	$_[1] =~ /\G\s*PROTO\s+($Word)\s*/ogsxc
	 or parsefail($_[1], "proto statement");
	my $name = $1;

	#print "parse_proto NAME:", $name,"\n";

	my $int = parse_interfacedecl($scene,1,1,$_[1]);
	$_[1] =~ /\G\s*{\s*/gsc or parsefail($_[1], "proto body start");
	my $pro = $scene->new_proto($name, $int);

	my @a;
	while($_[1] !~ /\G\s*}\s*/gsc) {
		my $n = parse_statement($pro,$_[1],0,"protoTop");
		if(defined $n) {push @a, $n}
	}
	#print "parse_proto, setting topnodes for ",VRML::NodeIntern::dump_name($pro),"\n";
	$pro->topnodes(\@a);
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
		if($_[3] =~ /\G\s*(eventIn|eventOut)\s+
			  ($Word)\s+($Word)\s+/ogsxc) {
			print "PARSING 1: $1 $2 $3\n"
				if $VRML::verbose::parse;
			$f{$3} = [$1,$2];
			my $n = $3;
			if($script and
			   $_[3] =~ /\G\s*IS\s+($Word)\s+/ogsc) {
				print "PARSING 1.1: A:$1 B:$2 C:$3, N:$n\n"
				      if $VRML::verbose::parse;
			   	push @{$f{$n}}, $scene->new_is($1, $n);
				print "Is SCRIPT.\n" if $VRML::verbose::parse;
			}
		} elsif($_[3] =~ /\G\s*(field|exposedField)\s+
			  ($Word)\s+($Word)/ogsxc) {
			  if($1 eq "exposedField" and !$exposed) {
			  	parsefail($_[3], "interface",
					   "exposedFields not allowed here");
			  }
			my($ft, $t, $n) = ($1, $2, $3);
			print "PARSING 2: $ft $t $n $fieldval\n"
				if $VRML::verbose::parse;
			$f{$n} = [$ft, $t];
			if($fieldval) {
				if($_[3] =~ /\G\s*IS\s+($Word)/gsc) {
					push @{$f{$n}}, $scene->new_is($1, $n);
				} else {
					push @{$f{$n}},
					  "VRML::Field::$t"->parse($scene,$_[3]);
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


package VRML::Field::SFNode;
use vars qw/$Word/;
VRML::Error->import;


my $LASTDEF = 1;

sub parse {
	my($type, $scene, $txt, $parentField) = @_;
	$_[2] =~ /\G\s*/gsc;

	if ($VRML::verbose::parse) {
		my ($package, $filename, $line) = caller;
		print "VRML::Field::SFNode::parse: ",
			(pos $_[2]), " ",
				length $_[2], " from $package, $line\n";
	}

	$_[2] =~ /\G\s*($Word)/ogsc or parsefail($_[2],"didn't match for sfnode fword");

	my $nt = $1;
	if($nt eq "NULL") {
		return "NULL";
	}
	my $vrmlname;
	my $is_name;
	my $p;
	my $rep_field;
	my $field_counter = 1;


	if($nt eq "DEF") {
		$_[2] =~ /\G\s*($Word)/ogsc or parsefail($_[2],
			"DEF must be followed by a defname");

		# store this as a sequence number, because multiple DEFS of the same name
		# must be unique. (see the spec)
		$vrmlname = $1;
		VRML::Handles::def_reserve($vrmlname, "DEF$LASTDEF");
		$LASTDEF++;
		my $defname = VRML::Handles::return_def_name($vrmlname);
		print "Parser.pm: DEF $vrmlname as $defname\n"
			if $VRML::verbose::parse;

		my $node = VRML::Field::SFNode->parse($scene, $_[2],$parentField);
		print "DEF - node $defname is $node \n" if  $VRML::verbose::parse;

		return $scene->new_def($defname, $node, $vrmlname);

	}
	if($nt eq "USE") {
		$_[2] =~ /\G\s*($Word)/ogsc or parsefail($_[2],
			"USE must be followed by a defname");

		$vrmlname = $1;
		# is is already DEF'd???
		my $dn = VRML::Handles::return_def_name($vrmlname);
        	if(!defined $dn) {
			VRML::VRMLFunc::ConsoleMessage( "USE name $vrmlname not DEFined yet\n");
			goto PARSE_EXIT;
		}

		print "USE $dn\n"
			if $VRML::verbose::parse;
		return $scene->new_use($dn);
	}
	if($nt eq "Script") {
		print "SCRIPT!\n"
			if $VRML::verbose::parse;
		return VRML::Parser::parse_script($scene,$_[2]);
	}

	# verify that this node is ok for the parent field type.
	#print "--- sub parse, nt $nt parentfield $parentField\n";
	if ($parentField eq "") {
		my ($package, $filename, $line) = caller;
		print "VRML::Field::SFNode::parse: null parentField  ",
			(pos $_[2]), " ",
				length $_[2], " from $package, $line\n";
	} else {
		# is this a switch choice?
		if ($parentField eq "choice") {$parentField = "children";}

		# is this a LOD level?
		if ($parentField eq "level") {$parentField = "children";}

		# is this a PROTO?
		if ($parentField eq "protoTop") {$parentField = "children";}

		if ($VRML::Nodes::{$parentField}{$nt}) {
		        #print "node $nt is ok for a parentField of $parentField\n";
		} else {
			my $okPROTO = 0;

			# is this a PROTO Expansion?
			#if ($parentField eq "children") {
				#print "Hmmm... verifying if $nt is a proto expansion scene $scene\n";
				if (exists $scene->{Protos}{$nt}) {
#					print "I found it!!!",
#VRML::NodeIntern::dump_name($scene),"\n";
#print "scene rootnode type is ",$scene->{Protos}{$nt}{RootNode}{Type}{Name},"\n";


					$okPROTO = 1;
					#foreach (keys % {$scene->{Protos}{$nt}}) {
#					foreach (keys % {$scene->{Protos}{$nt}{RootNode}}) {
#						print "proto $nt has key $_\n";
#					}
				}
			#}
	
			# nope, it failed even the PROTO test.
			if ($okPROTO == 0) {
				my ($package, $filename, $line) = caller;
        			VRML::VRMLFunc::ConsoleMessage("WARNING -- node $nt may not be ok as a ".
					"field of type $parentField\n(...called from $package, $line)");
			}
		}
	}


	my $proto;
	$p = pos $_[2];

	my $no = $VRML::Nodes{$nt};
	## look in PROTOs that have already been processed
	if (!defined $no) {
		$no = $scene->get_proto($nt);
		print "PROTO? '$no'\n"
			if $VRML::verbose::parse;
	}

	## next, let's try looking for EXTERNPROTOs in the file
	if (!defined $no) {
		## return to the beginning
		pos $_[2] = 0;
		VRML::Parser::parse_externproto($scene, $_[2], $nt);

		## reset position and try looking for PROTO nodes again
		pos $_[2] = $p;
		$no = $scene->get_proto($nt);
		print "PROTO? '$no'\n"
			if $VRML::verbose::parse;
	}

	if (!defined $no) {
		parsefail($_[2],"Invalid node '$nt'");
	}

	$proto=1;
	print "Match: '$nt'\n"
		if $VRML::verbose::parse;

	$_[2] =~ /\G\s*{\s*/gsc or parsefail($_[2],"didn't match brace!\n");
	my $isscript = ($nt eq "Script");
	my %f;
	while(1) {
		while(VRML::Parser::parse_statement($scene,$_[2],1,$parentField)
			!= -1) {}; # check for PROTO & co
		last if ($_[2] =~ /\G\s*}\s*/gsc);
		print "Pos: ",(pos $_[2]),"\n"
			if $VRML::verbose::parse;
		# Apparently, some people use it :(
		$_[2] =~ /\G\s*,\s*/gsc and parsewarnstd($_[2], "Comma not really right");

		$_[2] =~ /\G\s*($Word)\s*/gsc or parsefail($_[2],"Node body","field name not found");
		print "FIELD: '$1'\n"
			if $VRML::verbose::parse;

		my $f = VRML::Parser::parse_exposedField($1, $no);
		my $ft = $no->{FieldTypes}{$f};
		print "FT: $ft\n"
			if $VRML::verbose::parse;
		if(!defined $ft) {
			my $em = "Invalid field '$f' for node '$nt'\n";
			$em = $em. "Possible fields are: ";
			foreach (keys % {$no->{FieldTypes}}) {
				if (index($_,"_") !=0) {$em= $em. "$_ ";}
			}
			$em = $em. "\n";

			VRML::VRMLFunc::ConsoleMessage ($em);
			goto PARSE_EXIT;
		}

		# the following lines return something like:
		# 	Storing values... SFInt32 for node VNetInfo, f port
		#       storing type 2, port, (8888)

		print "Storing values... $ft for node $nt, f $f\n"
			 if $VRML::verbose::parse;

		if($_[2] =~ /\G\s*IS\s+($Word)/gsc) {
			$is_name = $1;

			# Allow multiple IS statements for a single field in a node in
			# a prototype definition.
			# Prepending digits to the field name should be safe, since legal
			# VRML names may not begin with numerical characters.
			#
			# See NIST test Misc, PROTO, #19 (30eventouts.wrl) as example.
			if (exists $f{$f}) {
				$rep_field = ++$field_counter.$f;
				print "VRML::Field::SFNode::parse: an IS for $ft $f exists, try $rep_field.\n"
					if $VRML::verbose::parse;
				$no->{FieldTypes}{$rep_field} = $no->{FieldTypes}{$f};
				$no->{FieldKinds}{$rep_field} = $no->{FieldKinds}{$f};
				$no->{Defaults}{$rep_field} = $no->{Defaults}{$f};

				if (exists $no->{EventIns}{$f}) {
					$no->{EventIns}{$rep_field} = $rep_field;
				}

				if (exists $no->{EventOuts}{$f}) {
					$no->{EventOuts}{$rep_field} = $rep_field;
				}

				$f{$rep_field} = $scene->new_is($is_name, $rep_field);
			} else {
				$f{$f} = $scene->new_is($is_name, $f);
			}
			print "storing type 1, $f, (name ",
				VRML::Debug::toString($f{$f}), ")\n"
						if $VRML::verbose::parse;
		} else {
			$f{$f} = "VRML::Field::$ft"->parse($scene,$_[2],$f);
				print "storing type 2, $f, (",
					VRML::NodeIntern::dump_name($f{$f}), ")\n"
						if $VRML::verbose::parse;
		}
	}
	print "END\n"
		if $VRML::verbose::parse;
	return $scene->new_node($nt, \%f);
}


sub print {
	my($typ, $this) = @_;
	if($this->{Type}{Name} eq "DEF") {
		print "DEF $this->{Fields}{id} ";
		$this->{Type}{Fields}{node}->print($this->{Fields}{node});
		return;
	}
	if($this->{Type}{Name} eq "USE") {
		print "USE $this->{Fields}{id} ";
		return;
	}
	print "$this->{Type}{Name} {";
	for(keys %{$this->{Fields}}) {
		print "$_ ";
		$this->{Type}{Fields}{$_}->print($this->{Fields}{$_});
		print "\n";
	}
	print "}\n";
}

1;
