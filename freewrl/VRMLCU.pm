# Copyright (C) 1998 Tuomas J. Lukka 1999 John Stewart CRC Canada
# DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
# See the GNU Library General Public License (file COPYING in the distribution)
# for conditions of use and redistribution.


# Use the C routines..

use strict;
no strict "refs";

package VRML::CU;

use VRML::VRMLFunc;

VRML::VRMLFunc::load_data(); # Fill hashes.

#JAS unused sub alloc_struct {
#JAS unused 	my($node) = @_;
#JAS unused 
#JAS unused 	my $type = $node->{Type}{Name};
#JAS unused 	print "alloc struct type $type\n";
#JAS unused 	if(!defined $VRML::CNodes{$type}) {
#JAS unused 		return undef;
#JAS unused 	}
#JAS unused 	my $s = VRML::VRMLFunc::alloc_struct($VRML::CNodes{$type}{Offs}{_end_},
#JAS unused 		$VRML::CNodes{$type}{Virt}
#JAS unused 		);
#JAS unused 	for(keys %{$node->{Fields}}) {
#JAS unused 		print "DO FIELD $_\n";
#JAS unused 		my $o;
#JAS unused 		if(!defined ($o=$VRML::CNodes{$type}{Offs}{$_})) {
#JAS unused 			die("Field $_ undefined for $type in C");
#JAS unused 		}
#JAS unused 		print "$node->{Fields}{$_} \n";
#JAS unused 		&{"VRML::VRMLFunc::alloc_offs_$node->{Type}{FieldTypes}{$_}"}(
#JAS unused 			$s, $o
#JAS unused 		);
#JAS unused 	}
#JAS unused 	$node->{CNode} = $s;
#JAS unused 	print "End alloc_struct\n";
#JAS unused }

sub alloc_struct_be {
	my($type) = @_;
	if(!defined $VRML::CNodes{$type}) {
		die("No CNode for $type\n");
	}
	# print "ALLNod: '$type' $VRML::CNodes{$type}{Offs}{_end_} $VRML::CNodes{$type}{Virt}\n";
	my $s = VRML::VRMLFunc::alloc_struct($VRML::CNodes{$type}{Offs}{_end_},
		$VRML::CNodes{$type}{Virt}
		);
	my($k,$o);
	while(($k,$o) = each %{$VRML::CNodes{$type}{Offs}}) {
		next if $k eq '_end_';
		my $ft = $VRML::Nodes{$type}{FieldTypes}{$k};
		# print "ALLS: $type $k $ft $o\n";
		&{"VRML::VRMLFunc::alloc_offs_$ft"}(
			$s, $o
		);
	}
	return $s;
	print "end of alloc_struct_be\n";
}

sub free_struct_be {
	my($node,$type) = @_;
	if(!defined $VRML::CNodes{$type}) {
		die("No CNode for $type\n");
	}
	my($k,$o);
	while(($k,$o) = each %{$VRML::CNodes{$type}{Offs}}) {
		my $ft = $VRML::Nodes{$type}{FieldTypes}{$k};
		&{"VRML::VRMLFunc::free_offs_$ft"}(
			$node, $o
		);
	}
	VRML::VRMLFunc::free_struct($node)
}

sub set_field_be {
	my($node, $type, $field, $value) = @_;
	my $o;
	#print "VRMLCU.pm:set_field_be, node $node, type $type, field $field, value $value\n";
	#if ("ARRAY" eq ref $value) {
	#	print "VRMLCU.pm:set_field_be, value of array is @{$value}\n";
	#}

	if(!defined ($o=$VRML::CNodes{$type}{Offs}{$field})) {
		#JAS don't die; sometimes listeners try to change while other changes happen
		#JAS (eg, addChild, removeChild...)

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

	&{"VRML::VRMLFunc::set_offs_$ft"}(
		$node, $o, 
		$value
	);
}

#JAS unused sub set_c_value {
#JAS unused 	my($node, $field) = @_;
#JAS unused 	my $type = $node->{Type}{Name};
#JAS unused 
#JAS unused 	
#JAS unused 	print "set_c_value DO FIELD $field\n";
#JAS unused 	my $o;
#JAS unused 	if(!defined ($o=$VRML::CNodes{$type}{Offs}{$field})) {
#JAS unused 		die("Field $field undefined for $type in C");
#JAS unused 	}
#JAS unused 	# print "$node->{Fields}{$field} \n";
#JAS unused 	if("ARRAY" eq ref $node->{Fields}{$field}) {
#JAS unused 		print "C: ",(join ',',@{$node->{Fields}{$field}}),"\n";
#JAS unused 	}
#JAS unused 	print "$node $node->{Type}{Name} $field $node->{Type}{FieldTypes}{$field}\n";
#JAS unused 	my $val = $node->{Fields}{$field};
#JAS unused 	if((ref $val) =~ /Node$/) {
#JAS unused 		if(!defined $val->{CNode}) {
#JAS unused 			print "UNABLE TO RETURN UNCNODED\n";
#JAS unused 			return undef;
#JAS unused 		}
#JAS unused 		$val = $val->{CNode};
#JAS unused 	}
#JAS unused 	&{"VRML::VRMLFunc::set_offs_$node->{Type}{FieldTypes}{$field}"}(
#JAS unused 		$node->{CNode}, $o, 
#JAS unused 		$val
#JAS unused 	);
#JAS unused 	print "set_c_value DO FIELD $field FINISHED\n";
#JAS unused }
#JAS unused 
#JAS unused sub make_struct {
#JAS unused 	my($node) = @_;
#JAS unused 	print "make_struct : ",(join ',',keys %{$node->{Fields}}),"\n";
#JAS unused 	if(!defined $node->{CNode}) {
#JAS unused 		alloc_struct($node);
#JAS unused 	}
#JAS unused 	for(keys %{$node->{Fields}}) {
#JAS unused 		set_c_value($node,$_);
#JAS unused 	}
#JAS unused }

1;
