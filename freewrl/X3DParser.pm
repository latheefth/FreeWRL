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
my %X3DProtoTypes = ();

#PROTOTYPES - PROTOTYPES - PROTOTYPES - PROTOTYPES - PROTOTYPES

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
#	Hash X3DProtoTypes{ProtoName}{ProtoInterface}{fieldname}...
#		          {ProtoBody}

sub parseProtoInterfaceField {
	my ($protoName,$field) = @_;
	my $arele = 0; #the first element will be the name of this proto
	my $nele = $#$field;
	my $bnub;
	my %rethash=();

	print "parseiProtoInterfaceField - proto $protoName, field $field\n";
	if (ref $field ne "ARRAY") {
		ConsoleMessage ("parseProtoInterfaceField for proto $protoName is not an array\n");
		return;
	}

	while ($arele <= $nele ){
		$bnub = $field->[$arele];
		#print "element $bnub\n";
		if (ref $bnub eq "HASH") {
			my $key;
			foreach $key (keys(%{$bnub})) {
				print "key $key is ",$bnub->{$key},"\n";
				$rethash{$key} = $bnub->{$key};
			}

		} else {
			ConsoleMessage ("parseProtoInterfaceField - expected a HASH here\n");
			return;
		}

		$arele+= 1;
	}
	print "end of parseProtoInterfaceField for $protoName, returning ,
		%rethash,"\n";
	return %rethash;
}


sub parseProtoInterface {
	my ($name,$proto) = @_;
	my $arele = 0; #the first element will be the name of this proto
	my $nele = $#$proto;
	my $bnub;
	
	#print "parseProtoInterface  for proto $name- we were passede in $proto\n";

	if (ref $proto ne "ARRAY") {
		ConsoleMessage ("parseProtoInterface for proto $name is not an array\n");
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
				ConsoleMessage ("ProtoInterface - field $key is invalid\n");
			}

		# lets look for a ProtoInterface, if we find it, parse it.
		} elsif ($bnub eq "field") {
			# the next element must be the ProtoInterface fields
			 $arele++;
			my $rethash = parseProtoInterfaceField($name,$proto->[$arele]);
			print "field is $rethash\n";

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
					ConsoleMessage ("ProtoDeclare - field $key is invalid\n");
				}
			}

		# lets look for a ProtoInterface, if we find it, parse it.
		} elsif ($bnub eq "ProtoInterface") {
			# the next element must be the ProtoInterface fields
			$arele++;
			parseProtoInterface($protoName,$proto->[$arele]);

		# did we find a ProtoBody? If so, just save this.
		} elsif ($bnub eq "ProtoBody") {
			print "Proto Body here\n";	
			$arele++;
				
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
	print "done parseX3DProtoDeclare - name is $protoName\n";

}

sub parseX3DProtoInstance {
	my ($name) = @_;
	print "protoInstance $name\n";
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
	print Dumper ($doc), "\n\n" if $X3D::verbose;

	# make an initial topnode of a Group type.
	# we'll get the fields later, so for now, just send in blank hash.

	# lets see if this is a valid X3D file; what was
	# returned should be an array, with 2 elements; the
	# first is "X3D", the second the X3D file.
	if ($#$doc ==1) {
		my $ele0 = $doc->[0];
		my $ele1 = $doc->[1];

		if ($ele0 ne "X3D") {
			ConsoleMessage("FreeWRL expected an X3D document, got: $ele0");
		} else {
			$n = parse_X3DStatement($ele0,$ele1);
			if (defined $n) {push @RetArr,$n};
		}

	} else {
		ConsoleMessage("Invalid X3D SceneGraph");
	}
	PARSE_EXIT:

	$X3DScene->topnodes(\@RetArr);
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

if ($node eq "ProtoDeclare") {
	return 1;
}
if ($node eq "ProtoInstance") {
	return 1;
}
	
#PROTOTYPES
		# fall through case - lets see if this is just the X3D node.
		if ($node ne "X3D") {
			ConsoleMessage( "WARNING - node $node is either not valid or not currently handled\n");
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

	my $LocalDEF = "";
	
	print "start parse_X3DStatement, array elements $nele node ",
		$parentNode,", ref doc ",ref $doc,"\n" if $X3D::verbose;

	# lets just make a "Scene" equate to a "Group"
	if ($parentNode eq "Scene") {$parentNode = "Group"};
	if (!verifyX3DNodeExists($parentNode)) { return;}


	# is this a ProtoDeclare?
	if ($parentNode eq "ProtoDeclare") {
		parseX3DProtoDeclare($doc);
		return;
	}
	if ($parentNode eq "ProtoInstance") {
		parseX3DProtoDeclare($parentNode);
		return;
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
			ConsoleMessage("Invalid X3D node '$parentNode'");
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
			} elsif (verifyX3DNodeExists($nextNodeName)) { 
				$field = getChildType($parentNode,$nextNodeName);
				#print "field $field\n";
				
				my $ft = $no->{FieldTypes}{$field};
					print "FT: $ft\n" if $X3D::verbose;
				if(!defined $ft) {
					my $mt = "Invalid field '$field' for node '$parentNode'\n";
					$mt = $mt . "Possible fields are: ";
					foreach (keys % {$no->{FieldTypes}}) {
						if (index($_,"_") !=0) {$mt = $mt . "$_ ";}
					}
					ConsoleMessage($mt);	
					goto PARSE_EXIT;
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
						ConsoleMessage("USE name ".$bnub->{$key}." not DEFined yet");
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

	##print "parse_X3DStatement, returning from $parentNode  field is \n";
	#foreach (keys %{\%field}) {print "kss $_ value ",$field{$_},"\n";}

	my $no = $X3DScene->new_node($parentNode,\%field);

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
