#  X3DParser.pm,
#
# Copyright (C) 2004 Clayton Cottingham 1998 Tuomas J. Lukka 1999 2005 John Stewart CRC Canada.
# DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
# See the GNU Library General Public License (file COPYING in the distribution)
# for conditions of use and redistribution.
#
# X3DParser.pm -- implement a X3D parser
#

package X3D::Parser;

$X3D::verbose = 0;

use strict vars;
use XML::Parser;
if ($X3D::verbose) {use Data::Dumper;}

require 'VRML/VRMLNodes.pm';
require 'VRML/VRMLFields.pm';

my $X3DScene;
my $eventMachine;

my $LASTDEF = 1;

# pass in a blank set of proto fields on global invocation
my %noProtoFields = ();
my $noProtoFieldsRef = \%noProtoFields;


#PROTOTYPES - PROTOTYPES - PROTOTYPES - PROTOTYPES - PROTOTYPES

my %X3DProtos = ();
my $protoTableRef = \%X3DProtos;

# parse prototypes here. Look past this section for non-prototype
# parsing.
# 
# A prototype consists of:
#	
#		---  parseX3DProtoDeclare parses: ---
#    <ProtoDeclare name='MyProto'>
#
#		---  parseProtoInterface parses: ---
#      <ProtoInterface>
#
#		---  parseProtoInterfaceField parses: ---
#        <field name='leg' type='SFColor' value='.8 .4 .7' 
#               accessType='initializeOnly'/>
#      </ProtoInterface>
#      <ProtoBody>
# 		...
#      </ProtoBody>
#    </ProtoDeclare>
#
# Internal Data Structure:
#	Hash X3DProtoTypes{"ProtoName"}{ProtoInterface}{field}{"fieldname"}=value
#		          {"ProtoName"}{ProtoBody} = array
# eg:
#$X3DProtos{proto1}{ProtoInterface}{field}{leg1}{type}="SFColor";
#$X3DProtos{proto1}{ProtoInterface}{field}{leg1}{value}="1.0 0.45 0.0";
#$X3DProtos{proto1}{ProtoBody} = "This belongs to proto1";
#$X3DProtos{proto2}{ProtoInterface}{field}{leg1}{type}="SFColor";
#$X3DProtos{proto2}{ProtoInterface}{field}{leg1}{value}="1.0 0.45 0.0";
#$X3DProtos{proto2}{ProtoBody} = "THIS IS PROTO2Body";

#
sub printX3DPRotoDeclares {
	my ($hash, $space) = @_;

	foreach my $key (keys %{$hash}) {
		my $hk = $hash->{$key};
		if (ref $hk eq "HASH") {
			print "$space $key\n";
			printX3DPRotoDeclares($hk,"$space    ");
		} else {
			if (ref $hk eq "ARRAY") {
				print "$space $key:\n";
			} else {
				print "$space $key is $hk \n";
			}
		}
	}
}




sub parseProtoInterfaceField {
	my ($protoName,$field) = @_;
	my $arele = 0; #the first element will be the name of this proto
	my $nele = $#$field;
	my $bnub;

	#print "parseiProtoInterfaceField - proto $protoName, field $field\n";
	if (ref $field ne "ARRAY") {
		VRML::VRMLFunc::ConsoleMessage ("parseProtoInterfaceField for proto $protoName is not an array\n");
		return;
	}

	while ($arele <= $nele ){
		$bnub = $field->[$arele];
		#print "element $bnub\n";
		if (ref $bnub eq "HASH") {
			my $key;
			foreach $key (keys(%{$bnub})) {
				#print "key $key is ",$bnub->{$key},"\n";
				if ($key ne ("name") &&
				    $key ne ("value") &&
				    $key ne ("accessType") &&
				    $key ne ("type"))  {
					VRML::VRMLFunc::ConsoleMessage (
						"protoInterface field $key ignored in proto $protoName\n");
				}
			}

			#$X3DProtos{proto2}{ProtoInterface}{field}{leg1}{value}="1.0 0.45 0.0";
			my $fn = $bnub->{name};
			$X3DProtos{$protoName}{ProtoInterface}{field}{$fn}{value}=$bnub->{value};
			$X3DProtos{$protoName}{ProtoInterface}{field}{$fn}{type}=$bnub->{type};
			$X3DProtos{$protoName}{ProtoInterface}{field}{$fn}{accessType}=$bnub->{accessType};
		

		} else {
			VRML::VRMLFunc::ConsoleMessage ("parseProtoInterfaceField - expected a HASH here\n");
			return;
		}

		$arele+= 1;
	}
}


sub parseProtoInterface {
	my ($name,$proto) = @_;
	my $arele = 0; #the first element will be the name of this proto
	my $nele = $#$proto;
	my $bnub;
	
	#print "parseProtoInterface  for proto $name- we were passede in $proto\n";

	if (ref $proto ne "ARRAY") {
		VRML::VRMLFunc::ConsoleMessage ("parseProtoInterface for proto $name is not an array\n");
		return;
	}

	while ($arele <= $nele ){
		$bnub = $proto->[$arele];
		#print "element $bnub\n";

		# did we find the hash? If so, these are the fields of the ProtoDeclare node.
		if (ref $bnub eq "HASH") {
			my $key;
			foreach $key (keys(%{$bnub})) {
				# are there ANY valid keys here?
				print "key $key is ",$bnub->{$key},"\n";
				VRML::VRMLFunc::ConsoleMessage ("ProtoInterface - field $key is invalid\n");
			}

		# lets look for a ProtoInterface, if we find it, parse it.
		} elsif ($bnub eq "field") {
			# the next element must be the ProtoInterface fields
			$arele++;
			parseProtoInterfaceField($name,$proto->[$arele]);

		# lets just make sure we did not get any non-garbage stuff here.
		} else {
			$bnub =~ s/\s+//g;
			
			# skip past "empty" elements in tree. 
			if ($bnub ne "0") {
				if ($bnub ne "") {
					print "ProtoInterface: skip this :$bnub: \n";

				}
			}
		}
		$arele += 1;
	}
}


sub parseX3DProtoDeclare {
	my ($proto) = @_;
	my $arele = 0; #the first element will be the name of this proto
	my $nele = $#$proto;
	my $bnub;
	my $protoName = "";


	# go through the tree structure for the ProtoDeclare. 
	while ($arele <= $nele ){
		$bnub = $proto->[$arele];
		#print "Declare: $bnub, ref ",ref $bnub," element $arele\n";

		# did we find the hash? If so, these are the fields of the ProtoDeclare node.
		if (ref $bnub eq "HASH") {
			my $key;
			foreach $key (keys(%{$bnub})) {
				#print "key $key is ",$bnub->{$key},"\n";
				if ($key eq "name") {
					$protoName = $bnub->{$key};
				} else {
					VRML::VRMLFunc::ConsoleMessage ("ProtoDeclare - field $key is invalid\n");
				}
			}

		# lets look for a ProtoInterface, if we find it, parse it.
		} elsif ($bnub eq "ProtoInterface") {
			# the next element must be the ProtoInterface fields
			$arele++;
			parseProtoInterface($protoName,$proto->[$arele]);

		# did we find a ProtoBody? If so, just save this.
		} elsif ($bnub eq "ProtoBody") {
			$arele++;
		
			# save the array that contains the proto code
			$X3DProtos{$protoName}{ProtoBody} = $proto->[$arele];
				
		# lets just make sure we did not get any non-garbage stuff here.
		} else {
			$bnub =~ s/\s+//g;
			
			# skip past "empty" elements in tree. 
			if ($bnub ne "0") {
				if ($bnub ne "") {
					print "ProtoDeclare: skip this :$bnub: \n";

				}
			}
		}
		$arele += 1;
	}
	#print "done parseX3DProtoDeclare - name is $protoName\n";
}

sub parseX3DProtoInstance {
	my ($proto,$protoFields) = @_;
	my $arele = 0; #the first element will be the name of this proto
	my $nele = $#$proto;
	my $bnub;
	my $protoName = "";
	my %fieldValueHash = ();
	my $fvref = \%fieldValueHash;
	my $retval;


	#print "protoInstance call\n"; print Dumper ($proto), "\n\n";
	# go through the tree structure for the ProtoInstance. 
	while ($arele <= $nele ){
		$bnub = $proto->[$arele];
		#print "Instance: $bnub, ref ",ref $bnub," element $arele\n";
		# did we find the hash? If so, these are the fields of the ProtoInstance node.
		if (ref $bnub eq "HASH") {
			my $key;
			foreach $key (keys(%{$bnub})) {
				#print "key $key is ",$bnub->{$key},"\n";
				if ($key eq "name") {
					$protoName = $bnub->{$key};
				} else {
					VRML::VRMLFunc::ConsoleMessage ("ProtoInstance - field $key is invalid\n");
				}
			}

		# lets look for fieldValues if we find it, parse it.
		} elsif ($bnub eq "fieldValue") {
			# the next element must be the fields
			$arele++;

			# make a hash of the fieldValues ;-)
			# get the fieldValue array.
			my $fieldValueArr = $proto->[$arele];

			if (ref $fieldValueArr ne "ARRAY") {
				VRML::VRMLFunc::ConsoleMessage ("ProtoInstance, expected an array for FieldValue params, got ".ref $fieldValueArr);
			} else {
				

				# ok, the first value should be our name/value hash.
				my $hash = $fieldValueArr->[0];
				if (ref $hash ne "HASH") {
					VRML::VRMLFunc::ConsoleMessage ("ProtoInstance, expected a fieldValue hash here, got ".ref $hash);
				} else {
					# yup, go through each, and get the name/val.
					my $name = "";
					my $value = "";
        				foreach my $key (keys %{$hash}) {
                				my $hk = $hash->{$key};
                        			#print "$key is $hk \n";
						if ($key eq "name") {
							$name = $hk;
						} elsif ($key eq "value") {
							$value = $hk;
						} else {
							VRML::VRMLFunc::ConsoleMessage ("ProtoInstance, expected a fieldValue key of name or value got $key");
							
						}
        				}

					# save these values.
					$fieldValueHash{$name}{value} = $value;

				}
			}




		# lets just make sure we did not get any non-garbage stuff here.
		} else {
			$bnub =~ s/\s+//g;
			
			# skip past "empty" elements in tree. 
			if ($bnub ne "0") {
				if ($bnub ne "") {
					print "ProtoInstance: skip this :$bnub: \n";

				}
			}
		}
		$arele += 1;
	}

	# Is this proto declared yet? If so, protoRef should be a HASH.
	my $protoRef = $protoTableRef->{$protoName};

	if (ref $protoRef ne "HASH") {
		VRML::VRMLFunc::ConsoleMessage ("Proto $protoName not defined for ProtoInstance");

		my $doc = "";
		$retval = parse_X3DStatement("Group",$doc,$protoFields);
	} else {
		# copy each of the fields over, for the parse. If the field 
		# exists in the ProtoInstance, use it; if not, use the field/value
		# in the ProtoDeclare.
		my $oldkeyptr = $protoRef->{ProtoInterface}->{field};
		foreach my $oldkey (keys %{$oldkeyptr}) {
			my $kkk = $oldkeyptr->{$oldkey};
			foreach my $oldkeyname (keys %{$kkk}) {
				if (!exists $fvref->{$oldkey}->{$oldkeyname}) {
					#print "new ildkeyname $oldkeyname\n";
					$fieldValueHash{$oldkey}{$oldkeyname} = $kkk->{$oldkeyname};
				} 
				#else {print "$oldkey $oldkeyname in ProtoInstance\n";}
			}

		}

		# verify that all name/values are copied over for this Proto.
       		#foreach my $key (keys %{$fvref}) {
       		#	my $hk = $fvref->{$key};
		#	print "NOW ProtoInstance key $key\n";
		#	foreach my $sk (keys %{$hk}) {
		#		print "	$sk = ",$hk->{$sk},"\n";
		#	}
		#}

		# now parse the proto with new values from the ProtoInstance
		$retval = parse_X3DStatement("Group",$protoRef->{ProtoBody},$fvref);
	}
	#print "parseX3DProtoInstance, returning $retval\n";
	return $retval; 
}
#PROTOTYPES - PROTOTYPES - PROTOTYPES - PROTOTYPES - PROTOTYPES


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
	$eventMachine = VRML::Browser::getEV();
	
	print "entering x3d parser\n*****************\n" if $X3D::verbose;
	
	# Parse the text into an XML tree style document.
	my $p = XML::Parser->new( Style => 'Tree' );
	my $doc = $p->parse($text);

	# dump this internal structure to the output, if required.
	#print Dumper ($doc), "\n\n" if $X3D::verbose;

	# make an initial topnode of a Group type.
	# we'll get the fields later, so for now, just send in blank hash.

	# lets see if this is a valid X3D file; what was
	# returned should be an array, with 2 elements; the
	# first is "X3D", the second the X3D file.
	if ($#$doc ==1) {
		my $ele0 = $doc->[0];
		my $ele1 = $doc->[1];

		if ($ele0 ne "X3D") {
			VRML::VRMLFunc::ConsoleMessage("FreeWRL expected an X3D document, got: $ele0");
		} else {
			$n = parse_X3DStatement($ele0,$ele1,$noProtoFieldsRef);
			#print "in parse, x3d statement returned $n\n";
			if (defined $n) {push @RetArr,$n};
		}

	} else {
		VRML::VRMLFunc::ConsoleMessage("Invalid X3D SceneGraph");
	}
	PARSE_EXIT:

	$X3DScene->topnodes(\@RetArr);
	if ($X3D::parse::verbose)  {printX3DPRotoDeclares ($protoTableRef,"");}
}


# is this node ok? is it a Valid X3D Node, or is it a header node? 
sub verifyX3DNodeExists {
	my ($node) = @_;
	print "verifyX3DNodeExists - looking for $node\n" if $X3D::parse::verbose;
	if (!(defined $VRML::Nodes{$node})) {
		# is this a "Header" node? If so, just skip it.
		if ($node eq "Header") { return 0; }
		if ($node eq "head") { return 0; }

		# the "Scene" node gets tranlated to "Group", so let this one through.
		if ($node eq "Scene") {return 1;}
	

		#PROTOTYPES

		if ($node eq "ProtoDeclare") { return 1; }
		if ($node eq "ProtoInstance") { return 1; }
		if ($node eq "IS") { return 1;}
	
		#PROTOTYPES

		# fall through case - lets see if this is just the X3D node.
		if ($node ne "X3D") {
			VRML::VRMLFunc::ConsoleMessage( "WARNING - node $node is either not valid or not currently handled\n");
			return 0;
		}
	}
	return 1;
}



# add a route for each element in this array. There should only be 1 route, but,
# until proven otherwise, lets go through all elements.
sub parseRoute {
	my ($bnub) = @_;
	foreach my $arel(@{$bnub}) {
		#print "element $arel\n";

		# do we have any extra fields (non-ROUTE related) here?
		foreach my $key (keys(%{$arel})) {
			#print "$key of ROUTE is ",$arel->{$key},"\n";
			if (!defined $VRML::Nodes::routefields{$key}) {
				print "X3DParser: $key found in a ROUTE statement, what is this?\n";
				return;
			}
		}

		# do we have all of the required fields?
		foreach my $key (keys(%VRML::Nodes::routefields)) {
			if (!defined $arel->{$key}) {
				print "X3DParser: did not find $key in a ROUTE statement\n";
				return;
			}
		}

		# now, add the ROUTE.
        	$eventMachine->add_route($X3DScene, 1, $arel->{fromNode}, $arel->{fromField}, 
			$arel->{toNode}, $arel->{toField});
	}
}

sub parseIS {
	my ($parentNode,$fieldref,$IS,$protoFields) = @_;
	my @retArray;
	my $nele = $#$IS;
	my $arele = 0;

	my $bnub;


	#print "parseIS $parentNode here protofields $protoFields fieldref $fieldref\n";
	#print "\nprotoFields:\n";
	#printX3DPRotoDeclares ($protoFields,"");
	#print "\nfieldref:\n";
	#printX3DPRotoDeclares ($fieldref,"");
	#print "\n";

	#print Dumper ($IS);
	# go through the IS node, and pull out fields that we require.
	while ($arele <= $nele ){
		$bnub = $IS->[$arele];
		#print "IS element $arele is $bnub ",ref $bnub,"\n";

		if (ref $bnub eq "HASH") {
       			foreach my $key (keys %{$bnub}) {
				print "huh? IS node hash key $key invalid\n";
			}
		} elsif ($bnub eq "connect") {
			$arele++;
			# presumably, only one connect pair is possible here.
			my $ar = $IS->[$arele]->[0];
			#print "connect is $ar\n";

			# find the nodeField and protoField keys
			my $nF = ""; 
			my $pF = "";
			foreach my $iskey (keys (%{$ar})) {
				#print "key $iskey is ",$ar->{$iskey},"\n";
				if ($iskey eq "nodeField") {
					$nF = $ar->{$iskey};
				} elsif ($iskey eq "protoField") {
					$pF = $ar->{$iskey};
				} else {
					print "IS, expected nodeField or protoField, got $iskey\n";
				}
			}
			#print "\n\nnow, lets do this, with field $nF ISvar $pF\n";
			#print "and we know that node is $parentNode, type ",$fieldref->{TypeName},"\n";
			# now, find the ISvar in the field hash.

			my $isVar = $protoFields->{$pF};
			my $isVarVal = $isVar->{value};
			my $isVarType = $isVar->{type};
			#print "isVar is $isVar, type $isVarType, val $isVarVal\n";

			# lets find out what we expected here
        		my $no = $VRML::Nodes{$parentNode};
			my $ft = $no->{FieldTypes}{$nF};
			#print "field type of node $parentNode is $ft\n";

			# do field types match?
			if ($ft ne $isVarType) {
				print "IS, node mismatch; $nF is $ft, $pF is $isVarType\n";
				return;
			} 
			$fieldref->{$nF} = parseSimpleField ($parentNode,$nF,$isVarVal);
		}else {
			$bnub =~ s/\s+//g;
			
			# skip past "empty" elements in tree. 
			if ($bnub ne "0") {
				if ($bnub ne "") {
					print "IS: Huh? (ele $arele NEXT NODE :$bnub: of ", $parentNode,"\n";
				}
			}
		}
		$arele ++;
	}

	#print "end of parseIS here\n";


}
# go through and parse a field, eg, match Box with Geometry.
sub parseX3DNodeField {
	my ($parentNode,$nextNodeName,$fieldref,$bnub,$protoFields) = @_;
	my $field;
	my $no = $VRML::Nodes{$parentNode};


	#print "parseX3DNodeField, field $field for node $parentNode next $nextNodeName\n";

	if ($nextNodeName eq "IS") {
		#print "nextNodeName is an IS\n";
		parseIS($parentNode,$fieldref,$bnub,$protoFields);
	} else {
	$field = getChildType($parentNode,$nextNodeName);

				
	my $ft = $no->{FieldTypes}{$field};
	print "FT: $ft\n" if $X3D::verbose;
	if(!defined $ft) {
		my $mt = "Invalid field '$field' for node '$parentNode'\n";
		$mt = $mt . "Possible fields are: ";
		foreach (keys % {$no->{FieldTypes}}) {
			if (index($_,"_") !=0) {$mt = $mt . "$_ ";}
		}
		VRML::VRMLFunc::ConsoleMessage($mt);	
		goto PARSE_EXIT;
	}
	
	if ($ft eq "MFNode") {
		#print "have a MFNode\n";
		if (ref $fieldref->{$field} eq "ARRAY") { 
		#print "fieldfield already defined and is an ARRAY\n";

		# find out what index the next element should be.
		my $nextele = @{$fieldref->{$field}};
		#print "element size is $nextele\n";
		my $retval = parse_X3DStatement($nextNodeName, $bnub,$protoFields);
		#print "element return value is $retval ref ",
		#	ref $retval,"\n";
						
		# we had a return value. Things like ProtoDeclares
		# will return an undefined value - we dont want
		# these as children (or whatever)
		if (defined $retval) {
			$fieldref->{$field}->[$nextele] = $retval;
		}
					
	} else {
		#print "new fieldfield\n";
		$fieldref->{$field} = [parse_X3DStatement($nextNodeName, $bnub,$protoFields)];
	}
	
	} elsif ($ft eq "SFNode") {
		#print "new SFNode field\n";
		$fieldref->{$field} = parse_X3DStatement($nextNodeName, $bnub,$protoFields);
	} else {
		print "CANT HANDLE fieldYpue $ft here\n";
	}
	#print "fieldfield is ",$fieldref->{$field}," ref ",ref $fieldref->{$field},"\n";
	#if (ref $fieldref->{$field} eq "ARRAY") {
	#	foreach (@{$fieldref->{$field}}) {
	#		print "element $_\n";
	#	}
	#}
	}
}

sub parse_X3DStatement {
	my ($parentNode,$doc,$protoFields) = @_;

	my @retArray;
	my $nele = $#$doc;
	my $arele = 0;

	my $bnub;

	my %field;

	my $thisLevelNode; 	# this is made from the ascii Node name
	my $nextNodeName;

	my $LocalDEF = "";
	
	#print "start of parse_X3DStatement, protofields $protoFields\n";
	#foreach my $key (keys (%{$protoFields})) {print "field $key\n";}


	print "start parse_X3DStatement, array elements $nele node ",
		$parentNode,", ref doc ",ref $doc,"\n" if $X3D::verbose;

	# lets just make a "Scene" equate to a "Group"
	if ($parentNode eq "Scene") {$parentNode = "Group"};
	if (!verifyX3DNodeExists($parentNode)) { return;}

	# if we have a protoInstance, recursively call back to here, via
	# the parseX3DProtoInstance function.
	if ($parentNode eq "ProtoInstance") {
		my $retval = parseX3DProtoInstance($doc,$protoFields);
		return $retval;
	}


	my $no = $VRML::Nodes{$parentNode};
	
	# is this an "X3D" node? if so, make it into a group, but ignore parameters.
	my $x3dinitnode = 0;

	if (!defined $no) {
		if ($parentNode eq "X3D") {
			print "have that main X3D header node\n" if $X3D::parse::verbose;
			$x3dinitnode = 1;
			$parentNode = "Group";
			$no = $VRML::Nodes{$parentNode};
		} else {
			VRML::VRMLFunc::ConsoleMessage("Invalid X3D node '$parentNode'");
			goto PARSE_EXIT;
		}
	}
	print "parse_X3DStatement, parentnode type $no\n" if $X3D::verbose;


	# go through the array elements for this entry in $doc, and
	# determine just what to do with them.
	while ($arele <= $nele ){
		$bnub = $doc->[$arele];

		# is this a sub-array?
		if (ref $bnub eq "ARRAY") {
			# look ahead to see what type the array is to determine
			# this node's child.
			print "calling getChildtype, parentNode $parentNode, nextNodeName $nextNodeName\n" if $X3D::parse::verbose;

			# handle routes here.
			if ($nextNodeName eq "ROUTE") {
				parseRoute ($bnub);
			# is this a ProtoDeclare?
			} elsif ($nextNodeName eq "ProtoDeclare") {
				parseX3DProtoDeclare($bnub);
			} elsif (verifyX3DNodeExists($nextNodeName)) { 
				parseX3DNodeField($parentNode,$nextNodeName,\%field,$bnub,$protoFields);
			}	

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
				print "$key of $parentNode is ",$bnub->{$key},"\n" 
					if $X3D::verbose;
				
				# is this possibly stuff like 'profile' that can be found in the
				# X3D header? if so, just skip it.


				# is this a DEF?
				if ($key eq "DEF") {
					$LocalDEF = $bnub->{$key};
				} elsif ($key eq "USE") {
					# is is already DEF'd???
					my $dn = VRML::Handles::return_def_name($bnub->{$key});
					if(!defined $dn) {
						VRML::VRMLFunc::ConsoleMessage("USE name ".$bnub->{$key}." not DEFined yet");
						goto PARSE_EXIT;
					}

					print "USE $dn\n" if $X3D::verbose;
					return $X3DScene->new_use($dn);
				} elsif ($x3dinitnode == 0) {
					if (!(exists $VRML::Nodes{$parentNode}->{FieldTypes}{$key})) { 
						print "X3DParse: Error: field $key does NOT exist in $parentNode\n"; 
					} else {
						# parse the simple field, and save the results.
						$field{$key}=parseSimpleField($parentNode,$key,$bnub->{$key});
					}
				}
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

	my $no = $X3DScene->new_node($parentNode,\%field);
	#print "returning $no\n";

	if ($LocalDEF ne "") {
                # store this as a sequence number, because multiple DEFS of the same name
                # must be unique. (see the spec)
                VRML::Handles::def_reserve($LocalDEF, "DEF$LASTDEF");
                $LASTDEF++;
                my $defname = VRML::Handles::return_def_name($LocalDEF);
                print "X3DParser.pm: DEF $LocalDEF as $defname\n"
                        if $X3D::verbose;

                return $X3DScene->new_def($defname, $no, $LocalDEF);
	}
	return $no;
}

# we have to look ahead and see what kind of node is coming down the line.
# eg, in VRML, we can have, of an IndexedFaceSet, "coord Coordinate {"
# but in X3D, we don't have that luxury. So, we have to determine what "Coordinate" is
# to an X3D node - in this case, it is the coord child.

sub getChildType {
	my ($pn,$nnn) = @_;

	my $st = "children";
	my $nextAscii = "";

	my $no = $VRML::Nodes{$pn};

	if (!defined $no) {
		print "X3DParser: getChildType - node $pn is invalid\n";
	}
	#foreach (keys %{$no->{FieldTypes}}){print"possible fields $_\n";}
	#
	#print "getChildType, parent $pn nextNodeName $nnn - lets find the field\n"; 

	# generic matchings - if we have an, eg LineSet, tell the caller that it is a geometry field
	# note that cases where there is a 1 to 1 mapping with just the first letter changed is
	# handled below. eg; "Color" node is returned as a "color" field.

	if ($VRML::Nodes::geometry{$nnn}) { return "geometry"; } 
	if ($VRML::Nodes::texture{$nnn}) { return "texture"; } 
	if ($VRML::Nodes::coord{$nnn}) { return "coord"; } 
	if ($VRML::Nodes::tcoord{$nnn}) { return "texCoord"; } 
  
	# now, see if we can tell by making the first character of the nodename lowercase...
	my $testfield = lcfirst ($nnn);
	
	if (defined ($no->{FieldTypes}{$testfield})) {
		#print "we found $testfield\n";
		$st = $testfield;
	}
	if (!defined ($no->{FieldTypes}{$st})) {
		print "ERROR - could not find $st or $nnn for node $no\n";
	}

	if ($X3D::parse::verbose) {
		if ($st eq "children") {
			print "getChildType for node $pn, field $nnn, returning $st\n";
		}
	}
	return $st;
}

# parse the field values of a node - eg, parse the size field of a Box node.
sub parseSimpleField {
	my ($pn,$field,$value) = @_;
        #print "parseSimpleField field $field, node $pn, value $value \n";

        my $no = $VRML::Nodes{$pn};

	my $ft = $no->{FieldTypes}{$field};
	#print "field type $ft\n";

	# re-work some field types to match VRML field parsing. These hacks eventually should
	# go into the parse subs.

	my $first = substr ($ft,0,2);

	# put brackets around all MF's
	if ($ft eq "MFString") {
		#print "MFString, value $value\n";
		if (substr ($value,0,1) eq "\"") {
			#print "MFString starts with a quote already\n";
			$value = "[".$value."]";
		} else {
			#print "have to add the quote\n";
			$value = "[\"".$value."\"]";
		}
	}
	elsif ($first eq "MF") { $value = "[".$value."]";}
	elsif ($ft eq "SFString") { $value = "\"".$value."\""; }
	elsif ($ft eq "SFBool") { 
		if ($value eq "false") {$value="FALSE";} 
		if ($value eq "true") {$value="TRUE";}
	}
	return "VRML::Field::$ft"->parse($X3DScene,$value);
}
1;
