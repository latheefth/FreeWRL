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
#errorrequire 'Tie::Scalar';
#errorrequire 'Tie::StdHash';

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
	my @n;
	my %f;

	$X3DScene = $scene;

	$VRML::verbose::parse = 1;
	
	print "entering x3d parser\n*****************\n" if $VRML::verbose::parse;
	
	# Parse the text into an XML tree style document.
	my $p = XML::Parser->new( Style => 'Tree' );
	my $doc = $p->parse($text);

	# dump this internal structure to the output, if required.
	print Dumper ($doc), "\n\n";

	# make an initial topnode of a Group type.
	# we'll get the fields later, so for now, just send in blank hash.
	my $TopNode = $X3DScene->new_node("Group", \%f);

	# lets see if this is a valid X3D file; what was
	# returned should be an array, with 2 elements; the
	# first is "X3D", the second the X3D file.
	if ($#$doc ==1) {
		my $ele0 = $doc->[0];
		my $ele1 = $doc->[1];

		if ($ele0 ne "X3D") {
			print "FreeWRL expected an X3D document, got: $ele0\n";
		} else {
			print "lets parse!\n";
			@n = parseTree($TopNode,$ele1,0);
		}

	} else {
		print "Invalid X3D SceneGraph\n";
	}

	push @RetArr, @n;
	$X3DScene->topnodes(\@RetArr);
	return @RetArr;
}

sub parseTree {
	my ($thisNode,$doc,$level) = @_;

	my @returnArray;
	my $nele = $#$doc;
	my $arele = 0;
	my @subArray;


	my $lp = $level*2+2;
	my $padded = pack("A$lp","$level ");


	print "$padded start parseTree, array elements $nele node ",
		$thisNode->{Type}{Name},"\n";

	my $bnub;

	my %f;

	my $NewNode; 	# this is made from the ascii Node name
	my $nextNodeName;

	while ($arele <= $nele ){
		#print "$padded in while loop, looking at element $arele\n";
		$bnub = $doc->[$arele];
		if (ref $bnub eq "ARRAY") {
			#print "$padded (ele $arele) array\n";
			@subArray = ();
			my @n = parseTree($NewNode, $bnub,$level+1);
			print "n is @n\n";
			if (@n ne 0) {
				push @subArray, @n;		
				print "$padded subArray is @subArray\n";
				# assume this is a Group for now
				my $f = "children";
				print "checking on fields\n";
				if (!exists $NewNode->{Fields}{$f}) {
					print "this field should exist: $f\n";
					exit (1);
				}
				print "$padded field already created $f ";
				foreach (@{$NewNode->{RFields}{$f}}) {
					print VRML::NodeIntern::dump_name ($_)," ";
				}
				print "\n";
				print "ref a ",ref $NewNode->{RFields}{$f},"\n";
				print "ref b ",ref @subArray,"\n";
				print "pushing to ",VRML::NodeIntern::dump_name($NewNode)," array ",@subArray,"\n";
				print "is ",
				$NewNode->{RFields}{$f}," ...",@{$NewNode->{RFields}{$f}},"\n";
				push @{$NewNode->{RFields}{$f}},@subArray;
	
				print "$padded so, for node ",
				VRML::NodeIntern::dump_name($NewNode),
				" we have ";
				foreach (@{$NewNode->{RFields}{$f}}) {
					print VRML::NodeIntern::dump_name($_)," ";
				} print "\n";
				print "$padded ",VRML::NodeIntern::dump_name($NewNode)," is $NewNode\n";
				print "$padded fields is ",$NewNode->{RFields},"\n";
				
				# erase name; helps in error tracking.
				$nextNodeName = "";
				@subArray = ();
			} else {
				print "$padded subArray is blank\n";
			}

		} elsif (ref $bnub eq "HASH") {
			# this had better be the first element, as it
			# is the parameters for the node we are parsing.
			if ($arele != 0) {
				print "Invalid X3D Tree, found a misplaced HASH at element $arele\n";
			}
			print "$padded (ele $arele) hash\n";
			parseFields ($thisNode, $bnub);

		}else {
			$bnub =~ s/\s+//g;
			
			# skip past "empty" elements in tree. 
			if ($bnub ne "0") {
				if ($bnub ne "") {
					print "$padded (ele $arele) NEXT NODE :$bnub:\n";
					$nextNodeName = $bnub;
					# we equate a Scene to a Group.
					if ($nextNodeName eq "Scene") {$nextNodeName = "Group";}

					print "$padded making new $nextNodeName \n";
					# we'll get the fields later, so for now, just send in blank hash.
					$NewNode = $X3DScene->new_node($nextNodeName, \%f);
					print "$padded created ",
					VRML::NodeIntern::dump_name($NewNode),
					"\n";

					if(defined $NewNode) {
						push @returnArray, $NewNode;
						print "$padded here, a is ";
						foreach (@returnArray) {
							print VRML::NodeIntern::dump_name($_)," ";
						}
						print "\n";
					}
				
				}
			}
		}
		$arele++;
		
	}
	print "$padded end parseTree, returning ";
	foreach (@returnArray) {
		print VRML::NodeIntern::dump_name($_)," ";
	} print "\n";
	return @returnArray;
}

sub parseFields {
	my ($me,$fieldVals) = @_;


	print "start parseFields\n";
	my $key;
	foreach $key (keys(%{$fieldVals})) {
		print "$key of nodeType ",$me->{Type}{Name}," is $key\n";
		print "value ",$fieldVals->{$key},"\n";
	}

	print "end parseFields\n";
}


1;
