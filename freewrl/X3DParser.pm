#
#  X3DParser.pm,
#
# Copyright (C) 2004 Clayton Cottingham 1998 Tuomas J. Lukka 1999 John Stewart CRC Canada.
# DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
# See the GNU Library General Public License (file COPYING in the distribution)
# for conditions of use and redistribution.
#
# X3DParser.pm -- implement a X3D parser
#

use strict vars;

package X3D::Error;
use vars qw/@ISA @EXPORT $Word $qre $cre $Float $Integer/;
require Exporter;
@ISA = qw/Exporter/;

@EXPORT = qw/$Word $qre $cre $Float $Integer/;

$Word =
qr|[^\x30-\x39\x0-\x20\x22\x23\x27\x2b\x2c\x2d\x2e\x5b\x5c\x5d\x7b\x7d\x7f][^\x0-\x20\x22\x23\x27\x2c\x2e\x5b\x5c\x5d\x7b\x7d\x7f]*(?:\b[^\x0-\x20\x22\x23\x27\x2c\x2e\x5b\x5c\x5d\x7b\x7d\x7f])?|;

$qre = qr{(?<!\\)\"};    # " Regexp for unquoted double quote
$cre = qr{[^\"\n]};      # " Regexp for not dquote, not \n char

### Defer to regex compiler:
$Float = qr~[\deE+\-\.]+~;

### Defer to regex compiler:
$Integer = qr~[\da-fA-FxX+\-]+~;

package X3D::Parser;

use strict vars;
use warnings;
use XML::Parser;
use Data::Dumper;

require Tie::Hash;
@X3D::X3DParser::ISA=Tie::StdHash;

require 'VRML/VRMLNodes.pm';
require 'VRML/VRMLFields.pm';
require 'VRML/FieldHash.pm';

use vars qw/$Word $qre $cre/;

X3D::Error->import;

my $X3DScene;


# Doc tree format.
# Each level has:
#	- Node type
#	- Array containing fields and sub-nodes (children, etc)
#	- possibly 2 more fields, a "0" and a blank field.

# Parse a whole file into $scene.
sub parse {
	my ( $scene, $text ) = @_;
	my @RetArr;
	my $n;

	$X3DScene = $scene;
	$X3D::verbose::parse = 0;
	
	print "entering x3d parser\n*****************\n" if $X3D::verbose::parse;
	
	# Parse the text into an XML tree style document.
	my $p = XML::Parser->new( Style => 'Tree' );
	my $doc = $p->parse($text);

	# dump this internal structure to the output, if required.
	print Dumper ($doc), "\n\n";# if $X3D::verbose::parse;

	# make an initial topnode of a Group type.
	# we'll get the fields later, so for now, just send in blank hash.

	# lets see if this is a valid X3D file; what was
	# returned should be an array, with 2 elements; the
	# first is "X3D", the second the X3D file.
	if ($#$doc ==1) {
		my $ele0 = $doc->[0];
		my $ele1 = $doc->[1];

		if ($ele0 ne "X3D") {
			print "FreeWRL expected an X3D document, got: $ele0\n";
		} else {
			$n = parse_X3DStatement("Group",$ele1);
			if (defined $n) {push @RetArr,$n};
		}

	} else {
		print "Invalid X3D SceneGraph\n";
	}

	$X3DScene->topnodes(\@RetArr);
}

sub parse_X3DStatement {
	my ($parentNode,$doc) = @_;

	my @retArray;
	my $nele = $#$doc;
	my $arele = 0;

	my $bnub;
	my $field;

	my %field;

	my $thisLevelNode; 	# this is made from the ascii Node name
	my $nextNodeName;
	
	print "start parse_X3DStatement, array elements $nele node ",
		$parentNode,", ref doc ",ref $doc,"\n" if $X3D::verbose::parse;

	# is this a "Header" node? If so, just skip it.
	if ($parentNode eq "Header") {
		return;
	}

	# lets just make a "Scene" equate to a "Group"
	if ($parentNode eq "Scene") {$parentNode = "Group"};

	my $no = $VRML::Nodes{$parentNode};

	if (!defined $no) {
		print "Invalid X3D node '$parentNode'\n";
		exit(1);
	}
	print "parse_X3DStatement, parentnode type $no\n" if $X3D::verbose::parse;


	# go through the array elements for this entry in $doc, and
	# determine just what to do with them.
	while ($arele <= $nele ){
		$bnub = $doc->[$arele];

		# is this a sub-array?
		if (ref $bnub eq "ARRAY") {
			# look ahead to see what type the array is to determine
			# this node's child.
			#print "calling getChildtype, parentNode $parentNode, nextNodeName $nextNodeName\n";
			#$field = getChildType($parentNode,$bnub);
			$field = getChildType($parentNode,$nextNodeName);
			#print "field $field\n";
			
			my $ft = $no->{FieldTypes}{$field};
				print "FT: $ft\n"
				if $X3D::verbose::parse;
			if(!defined $ft) {
				print "Invalid field '$field' for node '$parentNode'\n";
				print "Possible fields are: ";
				foreach (keys % {$no->{FieldTypes}}) {
					if (index($_,"_") !=0) {print "$_ ";}
				}
				print "\n";

				exit(1);
			}

			if ($ft eq "MFNode") {
				#print "have a MFNode\n";
			if (ref $field{$field} eq "ARRAY") { 
				#print "fieldfield already defined and is an ARRAY\n";
				my $nextele = @{$field{$field}};
				#print "element size is $nextele\n";
				$field{$field}->[$nextele] = parse_X3DStatement($nextNodeName, $bnub);
				
			} else {
				#print "new fieldfield\n";
				$field{$field} = [parse_X3DStatement($nextNodeName, $bnub)];
			}

			} elsif ($ft eq "SFNode") {
				$field{$field} = parse_X3DStatement($nextNodeName, $bnub);
			} else {
				print "CANT HANDLE fieldYpue $ft here\n";
			}
			#print "fieldfield is ",$field{$field}," ref ",ref $field{$field},"\n";
			

		# is this a simple field of the node?
		} elsif (ref $bnub eq "HASH") {
			# this had better be the first element, as it
			# is the parameters for parent of the level we are parsing.
			if ($arele != 0) {
				print "Invalid X3D Tree, found a misplaced HASH at element $arele\n";
			}


			# copy any keys over to the parent for later parsing. 
			my $key;
			foreach $key (keys(%{$bnub})) {
				print "$key of nodeType $parentNode ";
				print "is ",$bnub->{$key},"\n";
				my $value = "\"".$bnub->{$key}."\"";
				#$field{$key} = "VRML::Field::SFString"->parse(
					#	$X3DScene,$value);
				$field{$key} = $bnub->{$key};
			}



		# else is this just junk, or a new Node type?
		}else {
			$bnub =~ s/\s+//g;
			
			# skip past "empty" elements in tree. 
			if ($bnub ne "0") {
				if ($bnub ne "") {
					#print "(ele $arele) NEXT NODE :$bnub: of ", $parentNode,"\n";

					$nextNodeName = $bnub;
				}
			}
		}
		$arele++;
		
	}

	#print "parse_X3DStatement, returning from $parentNode  field is \n";
	#foreach (keys %{\%field}) {print "kss $_ value ",$field{$_},"\n";}
	return $X3DScene->new_node($parentNode,\%field);
}

# we have to look ahead and see what kind of node is coming down the line.
sub getChildType {
	my ($pn,$nnn) = @_;

	my $st = "children";
	my $nextAscii = "";

	my $no = $VRML::Nodes{$pn};

	#print "\n\n";
	if (!defined $no) {
		print "getChildType - node $pn is invalid\n";
	}
	#foreach (keys %{$no->{FieldTypes}}){print"possible fields $_\n";}
	#
	#print "getChildType, parent $pn nextNodeName $nnn - lets find the field\n"; 
  
	# lets see if this is a special one for Shape
	if ($pn eq "Shape") {
		#print "did find a Shape nextnode is $nnn\n";
		if ($nnn =~/(Box|Sphere|Cone|Cylinder|ElevationGrid|IndexedFaceSet)/) {
			return "geometry";
		}
	} elsif ($pn eq "Appearance") {
		if ($nnn =~/(ImageTexture|PixelTexture|MovieTexture)/) {
			return "texture";
		}
	}


		
	my $testfield = lcfirst ($nnn);
	
	if (defined ($no->{FieldTypes}{$testfield})) {
		#print "we found $testfield\n";
		$st = $testfield;
	}
	if (!defined ($no->{FieldTypes}{$st})) {
		print "ERROR - could not find $st\n";
	}

	#print "getChildType for node $pn, returning $st\n";
	return $st;
}


sub parseSimpleFields {
	my ($me,$fieldVals) = @_;

#	return; # do nothing for now.

	print "start parseFields for $me\n";
	my $key;
	foreach $key (keys(%{$fieldVals})) {
		print "$key of nodeType ",$me->{Type}{Name}," is $key\n";
		print "value ",$fieldVals->{$key},"\n";
	}
	print "end parseFields\n";
}
1;
