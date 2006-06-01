# Copyright (C) 1998 Tuomas J. Lukka 1999 John Stewart CRC Canada
# DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
# See the GNU Library General Public License (file COPYING in the distribution)
# for conditions of use and redistribution.


# Use the C routines..

use strict;
no strict "refs";

package VRML::CU;

use VRML::VRMLFunc;

VRML::VRMLFunc::load_data(); # Fill hashes

my $nodeMappingInitialized = 0;
my %NODEINTEGERMAPPING = ();


# make a list of node to integer maps. look at Structs.h, #define NODE_Group for instance.
# these mappings for the C runtime are made in VRMLC.pm - we make the same here.
sub initNodeMapping {
	#print "start of VRMLCU.pm $VRML::Nodes\n";

        my @unsortedfields = keys %VRML::Nodes;
        my @sf = sort(@unsortedfields);
	my $nodeIntegerType = 0; # keep equivalent to the equiv in VRMLC.pm
	for (@sf) {
		$NODEINTEGERMAPPING{$_} = $nodeIntegerType;
		$nodeIntegerType ++;
	}
	$nodeMappingInitialized=1;
}


sub alloc_struct_be {
	my($type) = @_;
	if(!defined $VRML::CNodes{$type}) {
		die("No CNode for $type in alloc_struct_be\n");
	}

	if ($nodeMappingInitialized == 0) {initNodeMapping();}

	my $nodeType = $NODEINTEGERMAPPING{$type};
	#print "VRMLCU - nt $nodeType\n";

	my $s = VRML::VRMLFunc::alloc_struct($nodeType);

	#print "VRMLCU: s is $s\n";
	return $s;
}

sub set_field_be {
	my($node, $type, $field, $value) = @_;
	my $o;
	#print "VRMLCU.pm:set_field_be, node $node, type $type, field $field, value $value ref ";
	
	#if ("ARRAY" eq ref $value) {
	#	print "VRMLCU.pm:set_field_be, value of array is @{$value}\n";
	#}

	if(!defined ($o=$VRML::CNodes{$type}{Offs}{$field})) {
		print "Field $field undefined for $type in C\n" if $VRML::verbose::warn;
		return;
	}

	if((ref $value) eq "HASH") {
		if(!defined $value->{CNode}) {
			print "UNABLE TO RETURN UNCNODED\n";
			return undef;
		}
		$value = $value->{CNode};
	}
	if((ref $value) eq "ARRAY" and (ref $value->[0]) eq "HASH") {
		$value = [map {$_->{CNode}} @{$value}];
	}


	my $ft = $VRML::Nodes{$type}{FieldTypes}{$field};
	my $fk = $VRML::Nodes{$type}{FieldKinds}{$field};

	# if($fk !~ /[fF]ield$/ and !defined $value) {return}
	print "SETS: $node $type $field '$value' (",(
		"ARRAY" eq ref $value ? join ',',@$value : $value ),") $ft $o\n"
		if $VRML::verbose::be;

	my $nv;
	$nv = $value;
	if ("ARRAY" eq ref $value) {
		$nv = "";

		#print "we have an array here, go through each element\n";
		my $elcount = $#$value;

		# for MFStrings, prepend a total element count on front 
		if ($ft eq "MFString") {
			if ($elcount >= 0) {
				$nv="$elcount:";
			}
		}

		#print "element count $elcount\n";
		my $ec = 0;
		while  ($ec <= $elcount) {
			my $thisVal = @{$value}[$ec];

			# if it is a string, put the string length on the front.
			if ($ft eq "MFString") {
				$thisVal = length($thisVal).":".$thisVal;
			}

			if ("ARRAY" ne ref $thisVal) {
				$nv = $nv."$thisVal,";
			} else {
				#print "$thisVal is an array\n";
				my $subelecount = $#$thisVal;
				my $sec = 0;
				while ($sec <= $subelecount) {
					my $subval = @{$thisVal}[$sec];
					$nv = $nv.$subval.",";
					$sec++;
				}
			}
			$ec ++;
		}
		
		# remove that last comma...
		$nv = substr ($nv,0,length($nv)-1);
		#print "end of loop, nv $nv\n";
	}

	#print "VRMLCU, ft $ft, node $node, o $o, nv $nv\n";

	VRML::VRMLFunc::set_field_be($node,$field,$nv);
}

1;
