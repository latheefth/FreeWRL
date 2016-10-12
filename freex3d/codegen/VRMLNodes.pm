#
# Copyright (C) 1998 Tuomas J. Lukka 1999 John Stewart CRC Canada.
# DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
# See the GNU Library General Public License (file COPYING in the distribution)
# for conditions of use and redistribution.

package VRML::NodeType;

use strict;
use warnings;

# see note top of file - the VRML Parser REQUIRES for routing that
# each field name exists in only one table- eg, "inputOutput, "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)", "initializeOnly, (SPEC_VRML | SPEC_X3D30)",
# etc. So, in order to do this, we make sure that each field name follows this, even
# though we may, for instance, change "value" to an "inputOutput, "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)" when the spec
# says it is an "initializeOnly, "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)". This has little if any effect on parsing.

# SPEC_VRML tag verified against http://web3d.org/x3d/specifications/vrml/ISO-IEC-14772-VRML97/part1/nodesRef.html

########################################################################

{
   sub new {
		my($type, $name, $fields, $X3DNodeType) = @_;
		if ($X3DNodeType eq "") {
			print "NodeType, X3DNodeType blank for $name\n";
			$X3DNodeType = "unknown";
		}
		# DEBUG: print "Node: $X3DNodeType\n";
		my $this = bless {
						  Name => $name,
						  Defaults => {},
						  X3DNodeType => $X3DNodeType
						 },$type;
		my $t;
		my $i;
		my $j;
		my $fname;
		my $farray;
		my @field;
		my $size = @$fields;
		my @fnames;
		#for (keys %$fields) {
		for($i=0; $i < $size; $i = $i + 2) {
			$j = $i + 1;
			$fname = $fields->[$i];
			push(@fnames, $fname);
			$farray = $fields->[$j];
			@field = @$farray;
			#print "field key $_\n";
			#print "fname $fname field @field $field[0]\n";
			if (ref $field[1] eq "ARRAY") {
				push @{$this->{Defaults}{$fname}}, @{$field[1]};
			} else {
				$this->{Defaults}{$fname} = $field[1];
			}
			$this->{FieldTypes}{$fname} = $field[0];

			$t = $field[2];
			if (!defined $t) {
				die("Missing field or event type $type X3DNodeType $X3DNodeType for $fname in $name");
			}
			$this->{FieldKinds}{$fname} = $t;

			$t = $field[3];
			if (!defined $t) {
				die("Missing field or event type $type X3DNodeType $X3DNodeType for $fname in $name");
			}
			$this->{SpecLevel}{$fname} = $t;

		}
		$this->{fnames} = \@fnames;
		return $this;
    }
}

our %Nodes = (

	###################################################################################

	# chapter 7: 		Core Component

	###################################################################################


	"WorldInfo" => new VRML::NodeType("WorldInfo", [
		info => ["MFString", [], "initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		title => ["SFString", "", "initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
	],"X3DChildNode"),

	"Proto" => new VRML::NodeType("Proto", [
		# sept 2014: keep Inline the same as Proto, so one can be cast to the other, unless/until executionContext is extracted from both
		__children => ["MFNode", [], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		bboxCenter => ["SFVec3f", [0, 0, 0], "initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		bboxSize => ["SFVec3f", [-1, -1, -1], "initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		_sortedChildren => ["MFNode", [], "inputOutput", 0],
		addChildren => ["MFNode", undef, "inputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		removeChildren => ["MFNode", undef, "inputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		__sibAffectors => ["MFNode", [], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		__protoDeclares => ["FreeWRLPTR", 0, "initializeOnly", 0],
		__externProtoDeclares => ["FreeWRLPTR", 0, "initializeOnly", 0],
		__nodes => ["FreeWRLPTR", 0, "initializeOnly", 0],
		__subcontexts => ["FreeWRLPTR", 0, "initializeOnly", 0],
		__GC => ["FreeWRLPTR", 0, "initializeOnly", 0],
		__protoDef => ["FreeWRLPTR", 0, "initializeOnly", 0], #user fields
		__protoFlags => ["SFInt32", 0, "initializeOnly", 0],
		__prototype => ["SFNode", "NULL", "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"], #first node in protobody
		__parentProto => ["SFNode", "NULL", "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"], #first node in protobody
		__ROUTES => ["FreeWRLPTR", 0, "initializeOnly", 0],
		__EXPORTS => ["FreeWRLPTR", 0, "initializeOnly", 0],
		__IMPORTS => ["FreeWRLPTR", 0, "initializeOnly", 0],
		__DEFnames => ["FreeWRLPTR", 0, "initializeOnly", 0],
		__IS => ["FreeWRLPTR", 0, "initializeOnly", 0],
		__scripts => ["FreeWRLPTR", 0, "initializeOnly", 0],
		url => ["MFString", [], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		__oldurl => ["MFString", [], "initializeOnly", 0],
		__afterPound => ["FreeWRLPTR", 0, "initializeOnly", 0],
		__loadstatus =>["SFInt32",0,"initializeOnly", 0],
		_parentResource =>["FreeWRLPTR",0,"initializeOnly", 0],
		__loadResource => ["FreeWRLPTR", 0, "initializeOnly", 0],
		__typename => ["FreeWRLPTR", 0, "initializeOnly", 0],
		load => ["SFBool", "TRUE", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		__oldload => ["SFBool", "FALSE", "initializeOnly", 0],
	],"X3DProtoInstance"),

	"MetadataBoolean" => new VRML::NodeType("MetadataBoolean", [
			metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			name => ["SFString","","inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"], # see note top of file
			reference => ["SFString","","initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			value => ["MFBool",[],"inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],  # see note top of file
	], "X3DChildNode"),

	"MetadataInteger" => new VRML::NodeType("MetadataInteger", [
			metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			name => ["SFString","","inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"], # see note top of file
			reference => ["SFString","","initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			value => ["MFInt32",[],"inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],  # see note top of file
	], "X3DChildNode"),

	"MetadataDouble" => new VRML::NodeType("MetadataDouble", [
			metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			name => ["SFString","","inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],# see note top of file:
			reference => ["SFString","","initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			value => ["MFDouble",[],"inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"], # see note top of file
	], "X3DChildNode"),

	"MetadataFloat" => new VRML::NodeType("MetadataFloat", [
			metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			name => ["SFString","","inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"], # see note top of file
			reference => ["SFString","","initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			value => ["MFFloat",[],"inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"], # see note top of file
	], "X3DChildNode"),

	"MetadataString" => new VRML::NodeType("MetadataString", [
			metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			name => ["SFString","","inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			reference => ["SFString","","initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			value => ["MFString",[],"inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"], # see note top of file
	], "X3DChildNode"),

	"MetadataSet" => new VRML::NodeType("MetadataSet", [
			metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			name => ["SFString","","inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"], # see note top of file
			reference => ["SFString","","initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			value => ["MFNode",[],"inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"], # see note top of file
	], "X3DChildNode"),

	###################################################################################

	# Chapter 8:		Time Component

	###################################################################################

	"TimeSensor" => new VRML::NodeType("TimeSensor", [
		cycleInterval => ["SFTime", 1, "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		enabled => ["SFBool", "TRUE", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		loop => ["SFBool", "FALSE", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		pauseTime => ["SFTime",0,"inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		resumeTime => ["SFTime",0,"inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		startTime => ["SFTime", 0, "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		stopTime => ["SFTime", 0, "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		cycleTime => ["SFTime", -1, "outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		elapsedTime => ["SFTime",0,"outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		fraction_changed => ["SFFloat", 0.0, "outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		isActive => ["SFBool", "FALSE", "outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		isPaused => ["SFTime",0,"outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		time => ["SFTime", -1, "outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],

		# time that we were initialized at
		__inittime => ["SFTime", 0, "initializeOnly", 0],
		# cycleTimer flag.
		__ctflag =>["SFTime", 10, "inputOutput", 0],
		__oldEnabled => ["SFBool", "TRUE", "inputOutput", 0],
	],"X3DSensorNode"),

	###################################################################################

	# Chapter 9:		Networking Component

	###################################################################################

	"Anchor" => new VRML::NodeType("Anchor", [
		addChildren => ["MFNode", undef, "inputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		removeChildren => ["MFNode", undef, "inputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		__sibAffectors => ["MFNode", [], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		children => ["MFNode", [], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		description => ["SFString", "", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		parameter => ["MFString", [], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		url => ["MFString", [], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		bboxCenter => ["SFVec3f", [0, 0, 0], "initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		bboxSize => ["SFVec3f", [-1, -1, -1], "initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],

		_parentResource =>["FreeWRLPTR",0,"initializeOnly", 0],
	],"X3DGroupingNode"),


	"Inline" => new VRML::NodeType("Inline", [
		# sept 2014: keep Inline the same as Proto, so one can be cast to the other, unless/until executionContext is extracted from both
		__children => ["MFNode", [], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		bboxCenter => ["SFVec3f", [0, 0, 0], "initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		bboxSize => ["SFVec3f", [-1, -1, -1], "initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		_sortedChildren => ["MFNode", [], "inputOutput", 0],
		addChildren => ["MFNode", undef, "inputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		removeChildren => ["MFNode", undef, "inputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		__sibAffectors => ["MFNode", [], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		__protoDeclares => ["FreeWRLPTR", 0, "initializeOnly", 0],
		__externProtoDeclares => ["FreeWRLPTR", 0, "initializeOnly", 0],
		__nodes => ["FreeWRLPTR", 0, "initializeOnly", 0],
		__subcontexts => ["FreeWRLPTR", 0, "initializeOnly", 0],
		__GC => ["FreeWRLPTR", 0, "initializeOnly", 0],
		__protoDef => ["FreeWRLPTR", 0, "initializeOnly", 0], #user fields
		__protoFlags => ["SFInt32", 0, "initializeOnly", 0],
		__prototype => ["SFNode", "NULL", "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"], #first node in protobody
		__parentProto => ["SFNode", "NULL", "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"], #first node in protobody
		__ROUTES => ["FreeWRLPTR", 0, "initializeOnly", 0],
		__EXPORTS => ["FreeWRLPTR", 0, "initializeOnly", 0],
		__IMPORTS => ["FreeWRLPTR", 0, "initializeOnly", 0],
		__DEFnames => ["FreeWRLPTR", 0, "initializeOnly", 0],
		__IS => ["FreeWRLPTR", 0, "initializeOnly", 0],
		__scripts => ["FreeWRLPTR", 0, "initializeOnly", 0],
		url => ["MFString", [], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		__oldurl => ["MFString", [], "initializeOnly", 0],
		__afterPound => ["FreeWRLPTR", 0, "initializeOnly", 0],
		__loadstatus =>["SFInt32",0,"initializeOnly", 0],
		_parentResource =>["FreeWRLPTR",0,"initializeOnly", 0],
		__loadResource => ["FreeWRLPTR", 0, "initializeOnly", 0],
		__typename => ["FreeWRLPTR", 0, "initializeOnly", 0],
		load => ["SFBool", "TRUE", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		__oldload => ["SFBool", "FALSE", "initializeOnly", 0],
		
		
		# load => ["SFBool", "TRUE","initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		# metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		# url => ["MFString", [], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		# bboxCenter => ["SFVec3f", [0, 0, 0], "initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		# bboxSize => ["SFVec3f", [-1, -1, -1], "initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],

                # __children => ["MFNode", [], "inputOutput", 0],
		# __loadstatus =>["SFInt32",0,"initializeOnly", 0],
		# _parentResource =>["FreeWRLPTR",0,"initializeOnly", 0],
		 # __loadResource => ["FreeWRLPTR", 0, "initializeOnly", 0],
	],"X3DNetworkSensorNode"),

	"LoadSensor" => new VRML::NodeType("LoadSensor", [
		enabled => ["SFBool", "FALSE","inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		timeOut  => ["SFTime",0,"inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		watchList => ["MFNode", [], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		isActive  => ["SFBool", "TRUE","outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		isLoaded  => ["SFBool", "TRUE","outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		loadTime  => ["SFTime",0,"outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		progress  => ["SFFloat",0,"outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],

		__loading => ["SFBool", "TRUE","initializeOnly", 0],		# current internal status
		__finishedloading => ["SFBool", "TRUE","initializeOnly", 0],	# current internal status
		__StartLoadTime => ["SFTime",0,"outputOnly", 0], # time we started loading...
		__oldEnabled => ["SFBool", "TRUE", "inputOutput", 0],
	],"X3DNetworkSensorNode"),


	###################################################################################

	# Chapter 10:		Grouping Component

	###################################################################################

	"Group" => new VRML::NodeType("Group", [
		addChildren => ["MFNode", undef, "inputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		removeChildren => ["MFNode", undef, "inputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		__sibAffectors => ["MFNode", [], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		children => ["MFNode", [], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		bboxCenter => ["SFVec3f", [0, 0, 0], "initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		bboxSize => ["SFVec3f", [-1, -1, -1], "initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		_sortedChildren => ["MFNode", [], "inputOutput", 0],
	],"X3DGroupingNode"),

	"StaticGroup" => new VRML::NodeType("StaticGroup", [
		metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		children => ["MFNode", [], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		bboxCenter => ["SFVec3f", [0, 0, 0], "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		bboxSize => ["SFVec3f", [-1, -1, -1], "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		__sibAffectors => ["MFNode", [], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],

		__transparency => ["SFInt32", -1, "initializeOnly", 0], # display list for transparencies
		__solid => ["SFInt32", -1, "initializeOnly", 0],	 # display list for solid geoms.
		_sortedChildren => ["MFNode", [], "inputOutput", 0],
	],"X3DGroupingNode"),

	"Switch" => new VRML::NodeType("Switch", [
		addChildren => ["MFNode", undef, "inputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		removeChildren => ["MFNode", undef, "inputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		__sibAffectors => ["MFNode", [], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		choice => ["MFNode", [], "inputOutput", "(SPEC_VRML)"],		# VRML nodes....
		children => ["MFNode", [], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],		# X3D nodes....
		metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		whichChoice => ["SFInt32", -1, "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		bboxCenter => ["SFVec3f", [0, 0, 0], "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		bboxSize => ["SFVec3f", [-1, -1, -1], "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],

		__isX3D => ["SFBool", "(inputFileVersion[0]==3)" , "initializeOnly", 0], # "TRUE" for X3D V3.x files
	],"X3DGroupingNode"),

	"Transform" => new VRML::NodeType ("Transform", [
		addChildren => ["MFNode", undef, "inputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		removeChildren => ["MFNode", undef, "inputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		__sibAffectors => ["MFNode", [], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		center => ["SFVec3f", [0, 0, 0], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		children => ["MFNode", [], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		rotation => ["SFRotation", [0, 0, 1, 0], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		scale => ["SFVec3f", [1, 1, 1], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		scaleOrientation => ["SFRotation", [0, 0, 1, 0], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		translation => ["SFVec3f", [0, 0, 0], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		bboxCenter => ["SFVec3f", [0, 0, 0], "initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		bboxSize => ["SFVec3f", [-1, -1, -1], "initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],

		# fields for reducing redundant calls
		__do_center => ["SFInt32", "FALSE", "initializeOnly", 0],
		__do_trans => ["SFInt32", "FALSE", "initializeOnly", 0],
		__do_rotation => ["SFInt32", "FALSE", "initializeOnly", 0],
		__do_scaleO => ["SFInt32", "FALSE", "initializeOnly", 0],
		__do_scale => ["SFInt32", "FALSE", "initializeOnly", 0],
		__do_anything => ["SFInt32", "FALSE", "initializeOnly", 0],
		_sortedChildren => ["MFNode", [], "inputOutput", 0],
	],"X3DGroupingNode"),


	###################################################################################

	# Chapter 11:		Rendering Component

	###################################################################################

	"ClipPlane" => new VRML::NodeType("ClipPlane", [
		enabled => ["SFBool", "TRUE", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		plane => ["SFVec4f", [0, 1, 0, 0], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],

	],"X3DChildNode"),

	"Color" => new VRML::NodeType("Color", [
		color => ["MFColor", [], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],

	],"X3DColorNode"),

	"ColorRGBA" => new VRML::NodeType("ColorRGBA", [
		color => ["MFColorRGBA", [], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],

	],"X3DColorNode"),

	"Coordinate" => new VRML::NodeType("Coordinate", [
		point => ["MFVec3f", [], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],

	],"X3DCoordinateNode"),

	"IndexedLineSet" => new VRML::NodeType("IndexedLineSet", [
		set_colorIndex => ["MFInt32", undef, "inputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		set_coordIndex => ["MFInt32", undef, "inputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		attrib	=> ["MFNode", [], "inputOutput", "(SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		color => ["SFNode", "NULL", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		coord => ["SFNode", "NULL", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		fogCoord => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		colorIndex => ["MFInt32", [], "initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		colorPerVertex => ["SFBool", "TRUE", "initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		coordIndex => ["MFInt32", [], "initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		__vertArr  =>["FreeWRLPTR",0,"initializeOnly", 0],
		__vertIndx  =>["FreeWRLPTR",0,"initializeOnly", 0],
		__xcolours  =>["FreeWRLPTR",0,"initializeOnly", 0],
		__vertices  =>["FreeWRLPTR",0,"initializeOnly", 0],
		__vertexCount =>["FreeWRLPTR",0,"initializeOnly", 0],
		__segCount =>["SFInt32",0,"initializeOnly", 0],
	],"X3DGeometryNode"),

	"IndexedTriangleFanSet" => new VRML::NodeType("IndexedTriangleFanSet", [
		set_index => ["MFInt32", undef, "inputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		attrib	=> ["MFNode", [], "inputOutput", "(SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		color => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		coord => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		fogCoord => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		normal => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		texCoord => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		ccw => ["SFBool", "TRUE", "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		colorPerVertex => ["SFBool", "TRUE", "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		normalPerVertex => ["SFBool", "TRUE", "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		solid => ["SFBool", "TRUE", "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		index => ["MFInt32", [], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],

		_coordIndex => ["MFInt32", [], "initializeOnly", 0],
	],"X3DGeometryNode"),

	"IndexedTriangleSet" => new VRML::NodeType("IndexedTriangleSet", [
		set_index => ["MFInt32", undef, "inputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		attrib	=> ["MFNode", [], "inputOutput", "(SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		color => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		coord => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		fogCoord => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		normal => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		texCoord => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		ccw => ["SFBool", "TRUE", "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		colorPerVertex => ["SFBool", "TRUE", "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		normalPerVertex => ["SFBool", "TRUE", "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		solid => ["SFBool", "TRUE", "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		index => ["MFInt32", [], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],

		_coordIndex => ["MFInt32", [], "initializeOnly", 0],
	],"X3DGeometryNode"),

	"IndexedTriangleStripSet" => new VRML::NodeType("IndexedTriangleStripSet", [
		set_index => ["MFInt32", undef, "inputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		attrib	=> ["MFNode", [], "inputOutput", "(SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		color => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		coord => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		fogCoord => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		normal => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		texCoord => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		ccw => ["SFBool", "TRUE", "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		colorPerVertex => ["SFBool", "TRUE", "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		normalPerVertex => ["SFBool", "TRUE", "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		solid => ["SFBool", "TRUE", "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		index => ["MFInt32", [], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],

		_coordIndex => ["MFInt32", [], "initializeOnly", 0],
	],"X3DGeometryNode"),

	"LineSet" => new VRML::NodeType("LineSet", [
		attrib	=> ["MFNode", [], "inputOutput", "(SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		color => ["SFNode", "NULL", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		coord => ["SFNode", "NULL", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		fogCoord => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		vertexCount => ["MFInt32",[],"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		__vertArr  =>["FreeWRLPTR",0,"initializeOnly", 0],
		__vertIndx  =>["FreeWRLPTR",0,"initializeOnly", 0],
		__segCount =>["SFInt32",0,"initializeOnly", 0],
	],"X3DGeometryNode"),

	"Normal" => new VRML::NodeType("Normal", [
		metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		vector => ["MFVec3f", [], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"] ,
	],"X3DNormalNode"),

	"PointSet" => new VRML::NodeType("PointSet", [
		attrib	=> ["MFNode", [], "inputOutput", "(SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		color => ["SFNode", "NULL", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		coord => ["SFNode", "NULL", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		fogCoord => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		_pointsVBO =>["SFInt32", 0, "initializeOnly", 0],
		_coloursVBO =>["SFInt32", 0, "initializeOnly", 0],
		_npoints =>["SFInt32", 0, "initializeOnly", 0],
		_colourSize =>["SFInt32", 0, "initializeOnly", 0],
	],"X3DGeometryNode"),

	"TriangleFanSet" => new VRML::NodeType("TriangleFanSet", [
		attrib	=> ["MFNode", [], "inputOutput", "(SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		color => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		coord => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		fanCount => ["MFInt32", [3], "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		fogCoord => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		normal => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		texCoord => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		ccw => ["SFBool", "TRUE", "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		colorPerVertex => ["SFBool", "TRUE", "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		normalPerVertex => ["SFBool", "TRUE", "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		solid => ["SFBool", "TRUE", "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],

		_coordIndex => ["MFInt32", [], "initializeOnly", 0],
	],"X3DGeometryNode"),

	"TriangleStripSet" => new VRML::NodeType("TriangleStripSet", [
		attrib	=> ["MFNode", [], "inputOutput", "(SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		color => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		coord => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		fogCoord => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		normal => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		stripCount => ["MFInt32", [], "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		texCoord => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		ccw => ["SFBool", "TRUE", "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		colorPerVertex => ["SFBool", "TRUE", "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		normalPerVertex => ["SFBool", "TRUE", "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		solid => ["SFBool", "TRUE", "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],

		_coordIndex => ["MFInt32", [], "initializeOnly", 0],

	],"X3DGeometryNode"),

	"TriangleSet" => new VRML::NodeType("TriangleSet", [
		attrib	=> ["MFNode", [], "inputOutput", "(SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		color => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		coord => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		fogCoord => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		normal => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		texCoord => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		ccw => ["SFBool", "TRUE", "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		colorPerVertex => ["SFBool", "TRUE", "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		normalPerVertex => ["SFBool", "TRUE", "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		solid => ["SFBool", "TRUE", "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],

		_coordIndex => ["MFInt32", [], "initializeOnly", 0],
	],"X3DGeometryNode"),


	###################################################################################

#	Chapter 12:		Shape Component

	###################################################################################

	"Appearance" => new VRML::NodeType ("Appearance", [
		fillProperties => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		lineProperties => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		material => ["SFNode", "NULL", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
                metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		shaders => ["MFNode", [], "inputOutput", "(SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		effects => ["MFNode", [], "inputOutput", "(SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		texture => ["SFNode", "NULL", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		textureTransform => ["SFNode", "NULL", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
	],"X3DAppearanceNode"),

	"FillProperties" => new VRML::NodeType ("FillProperties", [
		filled => ["SFBool", "TRUE", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		hatchColor => ["SFColor", [1,1,1], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		hatched => ["SFBool", "TRUE", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		hatchStyle => ["SFInt32", 1, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
                metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		_enabled =>["SFBool", "TRUE", "inputOutput",0], # literally, is this thing used or not?
		_hatchScale =>["SFVec2f", [0.1,0.1], "inputOutput",0], # the rate of the lines, 0.1 = 10 lines/meter
	],"X3DAppearanceChildNode"),

	"LineProperties" => new VRML::NodeType ("LineProperties", [
		applied => ["SFBool", "TRUE", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		linetype => ["SFInt32", 1, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		linewidthScaleFactor => ["SFFloat", 0, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
                metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
	],"X3DAppearanceChildNode"),

	"Material" => new VRML::NodeType ("Material", [
		ambientIntensity => ["SFFloat", 0.2, "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		diffuseColor => ["SFColor", [0.8, 0.8, 0.8], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		emissiveColor => ["SFColor", [0, 0, 0], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
                metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		shininess => ["SFFloat", 0.2, "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		specularColor => ["SFColor", [0, 0, 0], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		transparency => ["SFFloat", 0, "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		_verifiedColor => ["MFFloat",[
			0.0, 0.0, 0.0, 0.0,
			0.0, 0.0, 0.0, 0.0,
			0.0, 0.0, 0.0, 0.0,
			0.0, 0.0, 0.0, 0.0,
			0.0],"initializeOnly",0], # for making materials shader-friendly
	],"X3DMaterialNode"),

	"Shape" => new VRML::NodeType ("Shape", [
		appearance => ["SFNode", "NULL", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		geometry => ["SFNode", "NULL", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
                metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		bboxCenter => ["SFVec3f", [0, 0, 0], "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		bboxSize => ["SFVec3f", [-1, -1, -1], "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		__visible =>["SFInt32",0,"initializeOnly", 0], # for Occlusion tests.
		__occludeCheckCount =>["SFInt32",-1,"initializeOnly", 0], # for Occlusion tests.
		__Samples =>["SFInt32",-1,"initializeOnly", 0],		# Occlude samples from last pass

		_shaderflags_base =>["SFInt32",0,"initializeOnly",0], # shaders
		_shaderflags_effects =>["SFInt32",0,"initializeOnly",0], # shaders
		_shaderflags_usershaders =>["SFInt32",0,"initializeOnly",0], # shaders


	],"X3DBoundedObject"),

	"TwoSidedMaterial" => new VRML::NodeType ("TwoSidedMaterial", [
		ambientIntensity => ["SFFloat", 0.2, "inputOutput", "(SPEC_X3D33)"],
		backAmbientIntensity => ["SFFloat", 0.2, "inputOutput", "(SPEC_X3D33)"],
		backDiffuseColor => ["SFColor", [0.8, 0.8, 0.8], "inputOutput", "(SPEC_X3D33)"],
		backEmissiveColor => ["SFColor", [0, 0, 0], "inputOutput", "(SPEC_X3D33)"],
		backShininess => ["SFFloat", 0.2, "inputOutput", "(SPEC_X3D33)"],
		backSpecularColor => ["SFColor", [0, 0, 0], "inputOutput", "(SPEC_X3D33)"],
		backTransparency => ["SFFloat", 0, "inputOutput", "(SPEC_X3D33)"],
		diffuseColor => ["SFColor", [0.8, 0.8, 0.8], "inputOutput", "(SPEC_X3D33)"],
		emissiveColor => ["SFColor", [0, 0, 0], "inputOutput", "(SPEC_X3D33)"],
                metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D33)"],
		shininess => ["SFFloat", 0.2, "inputOutput", "(SPEC_X3D33)"],
		separateBackColor =>["SFBool","FALSE","inputOutput", "(SPEC_X3D33)"],
		specularColor => ["SFColor", [0, 0, 0], "inputOutput", "(SPEC_X3D33)"],
		transparency => ["SFFloat", 0, "inputOutput", "(SPEC_X3D33)"],
		_verifiedFrontColor => ["MFFloat",[
			0.0, 0.0, 0.0, 0.0,
			0.0, 0.0, 0.0, 0.0,
			0.0, 0.0, 0.0, 0.0,
			0.0, 0.0, 0.0, 0.0,
			0.0],"initializeOnly",0], # for making materials shader-friendly
		_verifiedBackColor => ["MFFloat",[
			0.0, 0.0, 0.0, 0.0,
			0.0, 0.0, 0.0, 0.0,
			0.0, 0.0, 0.0, 0.0,
			0.0, 0.0, 0.0, 0.0,
			0.0],"initializeOnly",0], # for making materials shader-friendly
	],"X3DMaterialNode"),



	###################################################################################

	# Chapter 13:		Geometry3D Component

	###################################################################################

	"Box" => new VRML::NodeType("Box", [
                metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		size => ["SFVec3f", [2, 2, 2], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"], # see note top of file
		solid => ["SFBool", "TRUE", "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		__points  =>["MFVec3f",[],"initializeOnly", 0],
	],"X3DGeometryNode"),

	"Cone" => new VRML::NodeType ("Cone", [
                metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		bottom => ["SFBool", "TRUE", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"], # see note top of file
		bottomRadius => ["SFFloat", 1.0, "initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		height => ["SFFloat", 2.0, "initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		side => ["SFBool", "TRUE", "initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		solid => ["SFBool", "TRUE", "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		 __sidepoints =>["MFVec3f",[],"initializeOnly", 0],
		 __botpoints =>["MFVec3f",[],"initializeOnly", 0],
		 __normals =>["MFVec3f",[],"initializeOnly", 0],
		__coneVBO =>["SFInt32",0,"initializeOnly",0],
		__coneTriangles =>["SFInt32",0,"initializeOnly",0],
		__wireindices => ["FreeWRLPTR", 0, "initializeOnly", 0],
	],"X3DGeometryNode"),

	"Cylinder" => new VRML::NodeType ("Cylinder", [
                metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		bottom => ["SFBool", "TRUE", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"], # see note top of file
		height => ["SFFloat", 2.0, "initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		radius => ["SFFloat", 1.0, "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"], # see note top of file
		side => ["SFBool", "TRUE", "initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		solid => ["SFBool", "TRUE", "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		top => ["SFBool", "TRUE", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"], # see note top of file
		 __points =>["MFVec3f",[],"initializeOnly", 0],
		 __normals =>["MFVec3f",[],"initializeOnly", 0],
		__cylinderVBO =>["SFInt32",0,"initializeOnly",0],
		__cylinderTriangles =>["SFInt32",0,"initializeOnly",0],
		__wireindices => ["FreeWRLPTR", 0, "initializeOnly", 0],
	],"X3DGeometryNode"),

	"ElevationGrid" => new VRML::NodeType("ElevationGrid", [
		set_height => ["MFFloat", undef, "inputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		attrib	=> ["MFNode", [], "inputOutput", "(SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		color => ["SFNode", "NULL", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		fogCoord => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
                metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		normal => ["SFNode", "NULL", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		texCoord => ["SFNode", "NULL", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		ccw => ["SFBool", "TRUE", "initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		colorPerVertex => ["SFBool", "TRUE", "initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		creaseAngle => ["SFFloat", 0, "initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		height => ["MFFloat", [], "initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		normalPerVertex => ["SFBool", "TRUE", "initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		solid => ["SFBool", "TRUE", "initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		xDimension => ["SFInt32", 0, "initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		xSpacing => ["SFFloat", 1.0, "initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		zDimension => ["SFInt32", 0, "initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		zSpacing => ["SFFloat", 1.0, "initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		_coordIndex => ["MFInt32", [], "initializeOnly", 0],
	],"X3DGeometryNode"),

	"Extrusion" => new VRML::NodeType("Extrusion", [
		set_crossSection => ["MFVec2f", undef, "inputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		set_orientation => ["MFRotation", undef, "inputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		set_scale => ["MFVec2f", undef, "inputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		set_spine => ["MFVec3f", undef, "inputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
                metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		beginCap => ["SFBool", "TRUE", "initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		ccw => ["SFBool", "TRUE", "initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		convex => ["SFBool", "TRUE", "initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		creaseAngle => ["SFFloat", 0, "initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		crossSection => ["MFVec2f", [[1, 1],[1, -1],[-1, -1],
						   [-1, 1],[1, 1]], "initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		endCap => ["SFBool", "TRUE", "initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		orientation => ["MFRotation", [[0, 0, 1, 0]],"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"], # see note top of file
		scale => ["MFVec2f", [[1, 1]], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"], # see note top of file
		solid => ["SFBool", "TRUE", "initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		spine => ["MFVec3f", [[0, 0, 0],[0, 1, 0]], "initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
	],"X3DGeometryNode"),

	"IndexedFaceSet" => new VRML::NodeType("IndexedFaceSet", [
		set_colorIndex => ["MFInt32", undef, "inputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		set_coordIndex => ["MFInt32", undef, "inputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		set_normalIndex => ["MFInt32", undef, "inputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		set_texCoordIndex => ["MFInt32", undef, "inputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		attrib	=> ["MFNode", [], "inputOutput", "(SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		color => ["SFNode", "NULL", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		coord => ["SFNode", "NULL", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		fogCoord => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
                metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		normal => ["SFNode", "NULL", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		texCoord => ["SFNode", "NULL", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		ccw => ["SFBool", "TRUE", "initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		colorIndex => ["MFInt32", [], "initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		colorPerVertex => ["SFBool", "TRUE", "initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		convex => ["SFBool", "TRUE", "initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		coordIndex => ["MFInt32", [], "initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		creaseAngle => ["SFFloat", 0, "initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		normalIndex => ["MFInt32", [], "initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		normalPerVertex => ["SFBool", "TRUE", "initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		solid => ["SFBool", "TRUE", "initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		texCoordIndex => ["MFInt32", [], "initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
	],"X3DGeometryNode"),

	"Sphere" => new VRML::NodeType("Sphere", [
                metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		radius => ["SFFloat", 1.0, "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"], # see note top of file
		solid => ["SFBool", "TRUE", "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		__points =>["MFVec3f",[],"initializeOnly", 0],
		_sideVBO =>["SFInt32", 0, "initializeOnly", 0],
		__SphereIndxVBO =>["SFInt32", 0, "initializeOnly", 0],
		__pindices => ["FreeWRLPTR", 0, "initializeOnly", 0],
		__wireindicesVBO =>["SFInt32", 0, "initializeOnly", 0],
 	],"X3DGeometryNode"),

	"Teapot" => new VRML::NodeType("Teapot", [
                metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		solid => ["SFBool", "TRUE", "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		__ifsnode => ["FreeWRLPTR", 0, "initializeOnly", 0],
 	],"X3DGeometryNode"),



	###################################################################################

	#	Chapter 14:	Geometry 2D Component

	###################################################################################

	"Arc2D" => new VRML::NodeType("Arc2D", [
                metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		endAngle => ["SFFloat", 1.5707, "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		radius => ["SFFloat", 1.0, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"], # see note top of file
		startAngle => ["SFFloat", 0.0, "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		__points  =>["MFVec2f",[],"initializeOnly", 0],
		__numPoints =>["SFInt32",0,"initializeOnly", 0],
 	],"X3DGeometryNode"),

	"ArcClose2D" => new VRML::NodeType("ArcClose2D", [
                metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		closureType => ["SFString","PIE","initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
	    	endAngle => ["SFFloat", 1.5707, "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
	    	radius => ["SFFloat", 1.0, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"], # see note top of file
		solid => ["SFBool", "FALSE", "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
	    	startAngle => ["SFFloat", 0.0, "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		__points  =>["MFVec2f",[],"initializeOnly", 0],
		__numPoints =>["SFInt32",0,"initializeOnly", 0],
 	],"X3DGeometryNode"),


	"Circle2D" => new VRML::NodeType("Circle2D", [
                metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
	    	radius => ["SFFloat", 1.0, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"], # see note top of file
		__points  =>["MFVec2f",[],"initializeOnly", 0],
		__numPoints =>["SFInt32",0,"initializeOnly", 0],
 	],"X3DGeometryNode"),

	"Disk2D" => new VRML::NodeType("Disk2D", [
                metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		innerRadius => ["SFFloat", 0.0, "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		outerRadius => ["SFFloat", 1.0, "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		solid => ["SFBool", "FALSE", "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		__points  =>["MFVec2f",[],"initializeOnly", 0],
		__texCoords  =>["MFVec2f",[],"initializeOnly", 0],
		__numPoints =>["SFInt32",0,"initializeOnly", 0],
		__simpleDisk => ["SFBool", "TRUE","initializeOnly", 0],
		__wireindices => ["FreeWRLPTR", 0, "initializeOnly", 0],
	],"X3DGeometryNode"),

	"Polyline2D" => new VRML::NodeType("Polyline2D", [
                metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		lineSegments => ["MFVec2f", [], "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
 	],"X3DGeometryNode"),

	"Polypoint2D" => new VRML::NodeType("Polypoint2D", [
                metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
	    	point => ["MFVec2f", [], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
 	],"X3DGeometryNode"),

	"Rectangle2D" => new VRML::NodeType("Rectangle2D", [
                metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		size => ["SFVec2f", [2.0, 2.0], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"], # see note top of file
		solid => ["SFBool", "FALSE", "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		__points  =>["MFVec3f",[],"initializeOnly", 0],
		__numPoints =>["SFInt32",0,"initializeOnly", 0],
 	],"X3DGeometryNode"),


	"TriangleSet2D" => new VRML::NodeType("TriangleSet2D", [
                metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
	    	vertices => ["MFVec2f", [], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		solid => ["SFBool", "FALSE", "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		__texCoords  =>["MFVec2f",[],"initializeOnly", 0],
		__wireindices => ["FreeWRLPTR", 0, "initializeOnly", 0],
 	],"X3DGeometryNode"),

	###################################################################################

	#	Chapter 15:		Text Component

	###################################################################################

	"Text" => new VRML::NodeType ("Text", [
		fontStyle => ["SFNode", "NULL", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		length => ["MFFloat", [], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		maxExtent => ["SFFloat", 0, "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		string => ["MFString", [], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		lineBounds => ["MFVec2f",[],"outputOnly", "(SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		origin => ["SFVec3f", [0, 0, 0], "outputOnly", "(SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		solid => ["SFBool", "TRUE", "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		textBounds => ["SFVec2f", [0, 0], "outputOnly", "(SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		_isScreen => ["SFInt32", 0, "inputOutput", 0], # > 0 for screenfont
		_screendata => ["FreeWRLPTR", 0, "initializeOnly", 0], # screentext rowvec
	],"X3DTextNode"),

	"FontStyle" => new VRML::NodeType("FontStyle", [
                metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		family => ["MFString", ["SERIF"], "initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		horizontal => ["SFBool", "TRUE", "initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		justify => ["MFString", ["BEGIN"], "initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		language => ["SFString", "", "initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		leftToRight => ["SFBool", "TRUE", "initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		size => ["SFFloat", 1.0, "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"], # see note top of file
		spacing => ["SFFloat", 1.0, "initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		style => ["SFString", "PLAIN", "initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		topToBottom => ["SFBool", "TRUE", "initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
	],"X3DFontStyleNode"),

	###################################################################################

	#	Chapter 16:		Sound Component

	###################################################################################

	"AudioClip" => new VRML::NodeType("AudioClip", [
		description => ["SFString", "", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		loop =>	["SFBool", "FALSE", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
                metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		pauseTime => ["SFTime",0,"inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		pitch => ["SFFloat", 1.0, "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		resumeTime => ["SFTime",0,"inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		startTime => ["SFTime", 0, "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		stopTime => ["SFTime", 0, "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		url => ["MFString", [], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		duration_changed => ["SFTime", -1, "outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		elapsedTime => ["SFTime",0,"outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		isActive => ["SFBool", "FALSE", "outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		isPaused => ["SFBool", "FALSE","outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		_parentResource =>["FreeWRLPTR",0,"initializeOnly", 0],
		__loadstatus =>["SFInt32",0,"initializeOnly", 0],
		__loadResource => ["FreeWRLPTR", 0, "initializeOnly", 0],
		# internal sequence number, openal buffer number
		__sourceNumber => ["SFInt32", -1, "initializeOnly", 0],
		# time that we were initialized at
		__inittime => ["SFTime", 0, "initializeOnly", 0],
		# local name, as received on system
		# old audio __localFileName => ["FreeWRLPTR", 0,"initializeOnly", 0],
	],"X3DSoundSourceNode"),

	"Sound" => new VRML::NodeType("Sound", [
		direction => ["SFVec3f", [0, 0, 1], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		intensity => ["SFFloat", 1.0, "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		location => ["SFVec3f", [0, 0, 0], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		maxBack => ["SFFloat", 10.0, "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		maxFront => ["SFFloat", 10.0, "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
                metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		minBack => ["SFFloat", 1.0, "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		minFront => ["SFFloat", 1.0, "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		priority => ["SFFloat", 0, "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		source => ["SFNode", "NULL", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		spatialize => ["SFBool", "FALSE", "initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		# openal sound source number
		__sourceNumber => ["SFInt32", -1, "initializeOnly", 0],
		__lastlocation => ["SFVec3f", [0, 0, 0], "initializeOnly",0],
		__lasttime => ["SFTime", 0, "initializeOnly", 0],
	],"X3DSoundSourceNode"),


	###################################################################################

	# Chapter 17:		Lighting Component

	###################################################################################

	"DirectionalLight" => new VRML::NodeType("DirectionalLight", [
		ambientIntensity => ["SFFloat", 0, "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		color => ["SFColor", [1, 1, 1], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		direction => ["SFVec3f", [0, 0, -1], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		global => ["SFBool", "FALSE", "inputOutput", "(SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		intensity => ["SFFloat", 1.0, "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
                metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		on => ["SFBool", "TRUE", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		_dir =>["SFVec4f",[0,0,0,0],"initializeOnly",0],
		_col =>["SFVec4f",[0,0,0,0],"initializeOnly",0],
		_amb =>["SFVec4f",[0,0,0,0],"initializeOnly",0],
	],"X3DLightNode"),

	"PointLight" => new VRML::NodeType("PointLight", [
		ambientIntensity => ["SFFloat", 0, "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		attenuation => ["SFVec3f", [1, 0, 0], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		color => ["SFColor", [1, 1, 1], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		global => ["SFBool", "TRUE", "inputOutput", "(SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		intensity => ["SFFloat", 1.0, "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		location => ["SFVec3f", [0, 0, 0], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
                metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		on => ["SFBool", "TRUE", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		radius => ["SFFloat", 100.0, "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		_loc =>["SFVec4f",[0,0,0,0],"initializeOnly",0],
		_col =>["SFVec4f",[0,0,0,0],"initializeOnly",0],
		_amb =>["SFVec4f",[0,0,0,0],"initializeOnly",0],
	],"X3DLightNode"),

	"SpotLight" => new VRML::NodeType("SpotLight", [
		ambientIntensity => ["SFFloat", 0, "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		attenuation => ["SFVec3f", [1, 0, 0], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		beamWidth => ["SFFloat", 1.570796, "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		color => ["SFColor", [1, 1, 1], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		cutOffAngle => ["SFFloat", 0.785398, "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		direction => ["SFVec3f", [0, 0, -1], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		global => ["SFBool", "TRUE", "inputOutput", "(SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		intensity => ["SFFloat", 1.0, "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		location => ["SFVec3f", [0, 0, 0], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
                metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		on => ["SFBool", "TRUE", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		radius => ["SFFloat", 100.0, "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		_dir =>["SFVec4f",[0,0,0,0],"initializeOnly",0],
		_loc =>["SFVec4f",[0,0,0,0],"initializeOnly",0],
		_col =>["SFVec4f",[0,0,0,0],"initializeOnly",0],
		_amb =>["SFVec4f",[0,0,0,0],"initializeOnly",0],
	],"X3DLightNode"),

	###################################################################################

	#	Chapter18:	Texturing Component

	###################################################################################

	"ImageTexture" => new VRML::NodeType("ImageTexture", [
                metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		url => ["MFString", [], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		repeatS => ["SFBool", "TRUE", "initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		repeatT => ["SFBool", "TRUE", "initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		textureProperties => ["SFNode", 0, "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		__textureTableIndex => ["SFInt32", 0, "initializeOnly", 0],
		_parentResource =>["FreeWRLPTR",0,"initializeOnly", 0],
	],"X3DTextureNode"),

	"MovieTexture" => new VRML::NodeType ("MovieTexture", [
		#SoundSource / AudioClip compatible section, keep in same order as AudioClip
		description => ["SFString", "", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		loop => ["SFBool", "FALSE", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
                metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		pauseTime => ["SFTime",0,"inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		pitch => ["SFFloat", 1.0, "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		resumeTime => ["SFTime",0,"inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		startTime => ["SFTime", 0, "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		stopTime => ["SFTime", 0, "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		url => ["MFString", [""], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		duration_changed => ["SFTime", -1, "outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		elapsedTime => ["SFTime",0,"outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		isActive => ["SFBool", "FALSE", "outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		isPaused => ["SFTime",0,"outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		_parentResource =>["FreeWRLPTR",0,"initializeOnly", 0],
		__loadstatus =>["SFInt32",0,"initializeOnly", 0],
		__loadResource => ["FreeWRLPTR", 0, "initializeOnly", 0],
		 # internal sequence number
		 __sourceNumber => ["SFInt32", -1, "initializeOnly", 0],
		 # time that we were initialized at
		 __inittime => ["SFTime", 0, "initializeOnly", 0],
		#Texture2D and Movie section
		repeatS => ["SFBool", "TRUE", "initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		repeatT => ["SFBool", "TRUE", "initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		textureProperties => ["SFNode", 0, "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		__textureTableIndex => ["SFInt32", 0, "initializeOnly", 0],
		speed => ["SFFloat", 1.0, "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		 # which texture number is used
		 __ctex => ["SFInt32", 0, "initializeOnly", 0],
		 # lowest frame
		 __lowest => ["SFInt32", 0, "initializeOnly", 0],
		 # highest frame
		 __highest => ["SFInt32", 0, "initializeOnly", 0],
	],"X3DTextureNode"),


	"MultiTexture" => new VRML::NodeType("MultiTexture", [
		alpha =>["SFFloat", 1, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		color =>["SFColor",[1,1,1],"inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		function =>["MFString",[],"inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
                metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		mode =>["MFString",[],"inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		source =>["MFString",[],"inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		texture=>["MFNode",undef,"inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		__xparams => ["FreeWRLPTR", 0, "initializeOnly", 0],
	],"X3DTextureNode"),

	"MultiTextureCoordinate" => new VRML::NodeType("MultiTextureCoordinate", [
                metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		texCoord =>["MFNode",undef,"inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
	],"X3DTextureCoordinateNode"),

	"MultiTextureTransform" => new VRML::NodeType("MultiTextureTransform", [
                metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		textureTransform=>["MFNode",undef,"inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
	],"X3DTextureTransformNode"),

	"PixelTexture" => new VRML::NodeType("PixelTexture", [
		image => ["SFImage", "0, 0, 0", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
                metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		repeatS => ["SFBool", "TRUE", "initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		repeatT => ["SFBool", "TRUE", "initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		textureProperties => ["SFNode", 0, "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		_parentResource =>["FreeWRLPTR",0,"initializeOnly", 0],
		__textureTableIndex => ["SFInt32", 0, "initializeOnly", 0],
	],"X3DTextureNode"),

	"TextureCoordinate" => new VRML::NodeType("TextureCoordinate", [
                metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		point => ["MFVec2f", [], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
	],"X3DTextureCoordinateNode"),

	"TextureCoordinateGenerator" => new VRML::NodeType("TextureCoordinateGenerator", [
                metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		mode => ["SFString","SPHERE","inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		parameter => ["MFFloat", [], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
	],"X3DTextureCoordinateNode"),

	"TextureProperties" => new VRML::NodeType("TextureProperties", [
		anisotropicDegree => ["SFFloat", 1.0, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		borderColor=>["SFColorRGBA",[0,0,0,0],"inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		borderWidth => ["SFInt32", 0, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		boundaryModeS => ["SFString", "REPEAT", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		boundaryModeT => ["SFString", "REPEAT", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		boundaryModeR => ["SFString", "REPEAT", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		magnificationFilter => ["SFString", "FASTEST", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
                metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		minificationFilter => ["SFString", "FASTEST", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		textureCompression => ["SFString", "FASTEST", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		texturePriority => ["SFFloat", 0.0, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		generateMipMaps => ["SFBool", "FALSE", "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],

	], "X3DSFNode"),

	"TextureTransform" => new VRML::NodeType ("TextureTransform", [
		center => ["SFVec2f", [0, 0], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
                metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		rotation => ["SFFloat", 0, "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		scale => ["SFVec2f", [1, 1], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		translation => ["SFVec2f", [0, 0], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
	],"X3DTextureTransformNode"),


	###################################################################################

	#	Chapter 19:		Interpolation Component

	###################################################################################

	"ColorInterpolator" => new VRML::NodeType("ColorInterpolator", [
		set_fraction => ["SFFloat", undef, "inputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		key => ["MFFloat", [], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		keyValue => ["MFColor", [], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
                metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		value_changed => ["SFColor", [0, 0, 0], "outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
	],"X3DInterpolatorNode"),

	"CoordinateInterpolator" => new VRML::NodeType("CoordinateInterpolator", [
		set_fraction => ["SFFloat", undef, "inputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		key => ["MFFloat", [], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		keyValue => ["MFVec3f", [], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
                metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		value_changed => ["MFVec3f", [], "outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		_GPU_Routes_out => ["SFInt32", 0, "initializeOnly", 0],
		_CPU_Routes_out => ["SFInt32", 0, "initializeOnly", 0],

		# GPU running only - run the interpolator on the GPU, use these...
		_keyVBO =>["SFInt32", 0, "initializeOnly", 0],
		_keyValueVBO =>["SFInt32", 0, "initializeOnly", 0],
	],"X3DInterpolatorNode"),

	"CoordinateInterpolator2D" => new VRML::NodeType("CoordinateInterpolator2D", [
		set_fraction => ["SFFloat", undef, "inputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		key => ["MFFloat", [], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		keyValue => ["MFVec2f", [], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
                metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		value_changed => ["MFVec2f", [[0,0]], "outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
	],"X3DInterpolatorNode"),

	"EaseInEaseOut" => new VRML::NodeType("EaseInEaseOut", [
		set_fraction => ["SFFloat", undef, "inputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		easeInEaseOut => ["MFVec2f", [], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		key => ["MFFloat", [], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
                metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		modifiedFraction_changed => ["SFFloat", 0.0, "outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
	],"X3DInterpolatorNode"),



	"NormalInterpolator" => new VRML::NodeType("NormalInterpolator", [
		set_fraction => ["SFFloat", undef, "inputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		key => ["MFFloat", [], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		keyValue => ["MFVec3f", [], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
                metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		value_changed => ["MFVec3f", [], "outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
	],"X3DInterpolatorNode"),

	"OrientationInterpolator" => new VRML::NodeType("OrientationInterpolator", [
		set_fraction => ["SFFloat", undef, "inputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		key => ["MFFloat", [], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		keyValue => ["MFRotation", [], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
                metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		value_changed => ["SFRotation", [0, 0, 1, 0], "outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
	],"X3DInterpolatorNode"),

	"PositionInterpolator" => new VRML::NodeType("PositionInterpolator", [
		set_fraction => ["SFFloat", undef, "inputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		key => ["MFFloat", [], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		keyValue => ["MFVec3f", [], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
                metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		value_changed => ["SFVec3f", [0, 0, 0], "outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
	],"X3DInterpolatorNode"),

	"PositionInterpolator2D" => new VRML::NodeType("PositionInterpolator2D", [
		set_fraction => ["SFFloat", undef, "inputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		key => ["MFFloat", [], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		keyValue => ["MFVec2f", [], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
                metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		value_changed => ["SFVec2f", [0, 0], "outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
	],"X3DInterpolatorNode"),

	"ScalarInterpolator" => new VRML::NodeType("ScalarInterpolator", [
		set_fraction => ["SFFloat", undef, "inputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		key => ["MFFloat", [], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		keyValue => ["MFFloat", [], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
                metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		value_changed => ["SFFloat", 0.0, "outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
	],"X3DInterpolatorNode"),

	"SplinePositionInterpolator" => new VRML::NodeType("SplinePositionInterpolator", [
		set_fraction => ["SFFloat", undef, "inputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		closed => ["SFBool", "FALSE", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		key => ["MFFloat", [], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		keyValue => ["MFVec3f", [], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		keyVelocity => ["MFVec3f", [], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
                metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		normalizeVelocity => ["SFBool", "FALSE", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		value_changed => ["SFVec3f", [0,0,0], "outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
	],"X3DInterpolatorNode"),

	"SplinePositionInterpolator2D" => new VRML::NodeType("SplinePositionInterpolator2D", [
		set_fraction => ["SFFloat", undef, "inputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		closed => ["SFBool", "FALSE", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		key => ["MFFloat", [], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		keyValue => ["MFVec2f", [], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		keyVelocity => ["MFVec2f", [], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
                metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		normalizeVelocity => ["SFBool", "FALSE", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		value_changed => ["SFVec2f", [0,0], "outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
	],"X3DInterpolatorNode"),

	"SplineScalarInterpolator" => new VRML::NodeType("SplineScalarInterpolator", [
		set_fraction => ["SFFloat", undef, "inputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		closed => ["SFBool", "FALSE", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		key => ["MFFloat", [], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		keyValue => ["MFFloat", [], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		keyVelocity => ["MFFloat", [], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
                metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		normalizeVelocity => ["SFBool", "FALSE", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		value_changed => ["SFFloat", 0.0, "outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
	],"X3DInterpolatorNode"),

	"SquadOrientationInterpolator" => new VRML::NodeType("SquadOrientationInterpolator", [
		set_fraction => ["SFFloat", undef, "inputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		key => ["MFFloat", [], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		keyValue => ["MFRotation", [], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
                metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		normalizeVelocity => ["SFBool", "FALSE", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		value_changed => ["SFRotation", [0,0,1,0], "outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
	],"X3DInterpolatorNode"),

	###################################################################################

	#		Cubemap Texturing Component

	###################################################################################


	"ComposedCubeMapTexture" => new VRML::NodeType("ComposedCubeMapTexture", [
		metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		back =>["SFNode","NULL","inputOutput", "(SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		bottom =>["SFNode","NULL","inputOutput", "(SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		front =>["SFNode","NULL","inputOutput", "(SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		left =>["SFNode","NULL","inputOutput", "(SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		top =>["SFNode","NULL","inputOutput", "(SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		right =>["SFNode","NULL","inputOutput", "(SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		_parentResource =>["FreeWRLPTR",0,"initializeOnly", 0],
	],"X3DEnvironmentTextureNode"),

	"GeneratedCubeMapTexture" => new VRML::NodeType("GeneratedCubeMapTexture", [
		metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		textureProperties => ["SFNode", "NULL", "initializeOnly", "(SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		__textureTableIndex => ["SFInt32", 0, "initializeOnly", 0],
		_parentResource =>["FreeWRLPTR",0,"initializeOnly", 0],
		__subTextures => ["MFNode",[],"initializeOnly",0],
		__regenSubTextures => ["SFBool","FALSE","initializeOnly",0],
		update => ["SFString","NONE","inputOutput", "(SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		size => ["SFInt32",128,"initializeOnly", "(SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"], # see note top of file
	],"X3DEnvironmentTextureNode"),

	#same order of fields up to __regenSubtextures as GeneratedCubeMapTexture
	"ImageCubeMapTexture" => new VRML::NodeType("ImageCubeMapTexture", [
		metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		textureProperties => ["SFNode", "NULL", "initializeOnly", "(SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		__textureTableIndex => ["SFInt32", 0, "initializeOnly", 0],
		_parentResource =>["FreeWRLPTR",0,"initializeOnly", 0],
		__subTextures => ["MFNode",[],"initializeOnly",0],
		__regenSubTextures => ["SFBool","FALSE","initializeOnly",0],
		url => ["MFString",[],"inputOutput", "(SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
	],"X3DEnvironmentTextureNode"),




	###################################################################################

	#	20	Pointing Device Component

	###################################################################################

	"TouchSensor" => new VRML::NodeType("TouchSensor", [
			enabled => ["SFBool", "TRUE", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			hitNormal_changed => ["SFVec3f", [0, 0, 0], "outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			hitPoint_changed => ["SFVec3f", [0, 0, 0], "outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			hitTexCoord_changed => ["SFVec2f", [0, 0], "outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			_oldhitNormal => ["SFVec3f", [0, 0, 0], "outputOnly", 0], 	# send event only if changed
			_oldhitPoint => ["SFVec3f", [0, 0, 0], "outputOnly", 0], 	# send event only if changed
			_oldhitTexCoord => ["SFVec2f", [0, 0], "outputOnly", 0], 	# send event only if changed
			isActive => ["SFBool", "FALSE", "outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			isOver => ["SFBool", "FALSE", "outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			description => ["SFString", "", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"], # see note top of file
			touchTime => ["SFTime", -1, "outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			__oldEnabled => ["SFBool", "TRUE", "inputOutput", 0],
					   ],"X3DPointingDeviceSensorNode"),

	"PlaneSensor" => new VRML::NodeType("PlaneSensor", [
			autoOffset => ["SFBool", "TRUE", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			axisRotation => ["SFRotation", [0, 0, 1, 0], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			enabled => ["SFBool", "TRUE", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			maxPosition => ["SFVec2f", [-1, -1], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			minPosition => ["SFVec2f", [0, 0], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			offset => ["SFVec3f", [0, 0, 0], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			isActive => ["SFBool", "FALSE", "outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			isOver => ["SFBool", "FALSE", "outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			description => ["SFString", "", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"], # see note top of file
			trackPoint_changed => ["SFVec3f", [0, 0, 0], "outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			translation_changed => ["SFVec3f", [0, 0, 0], "outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			_oldtrackPoint => ["SFVec3f", [0, 0, 0], "outputOnly", 0],
			_oldtranslation => ["SFVec3f", [0, 0, 0], "outputOnly", 0],
			# where we are at a press...
			_origPoint => ["SFVec3f", [0, 0, 0], "initializeOnly", 0],
			__oldEnabled => ["SFBool", "TRUE", "inputOutput", 0],
					   ],"X3DPointingDeviceSensorNode"),

#
# Experimental node: LineSensor
#
	"LineSensor" => new VRML::NodeType("LineSensor", [
			autoOffset => ["SFBool", "TRUE", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			direction => ["SFVec3f", [1, 0, 0], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			enabled => ["SFBool", "TRUE", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			maxPosition => ["SFFloat", -1, "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			minPosition => ["SFFloat", 0, "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			offset => ["SFFloat", 0, "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			isActive => ["SFBool", "FALSE", "outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			isOver => ["SFBool", "FALSE", "outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			description => ["SFString", "", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"], # see note top of file
			trackPoint_changed => ["SFVec3f", [0, 0, 0], "outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			translation_changed => ["SFVec3f", [0, 0, 0], "outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			_oldtrackPoint => ["SFVec3f", [0, 0, 0], "outputOnly", 0],
			_oldtranslation => ["SFVec3f", [0, 0, 0], "outputOnly", 0],
			# where we are at a press...
			_origPoint => ["SFVec3f", [0, 0, 0], "initializeOnly", 0],
			__oldEnabled => ["SFBool", "TRUE", "inputOutput", 0],
					   ],"X3DPointingDeviceSensorNode"),

	"SphereSensor" => new VRML::NodeType("SphereSensor", [
			autoOffset => ["SFBool", "TRUE", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			enabled => ["SFBool", "TRUE", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			offset => ["SFRotation", [0, 1, 0, 0], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			isActive => ["SFBool", "FALSE", "outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			rotation_changed => ["SFRotation", [0, 0, 1, 0], "outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			trackPoint_changed => ["SFVec3f", [0, 0, 0], "outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			_oldtrackPoint => ["SFVec3f", [0, 0, 0], "outputOnly", 0],
			_oldrotation => ["SFRotation", [0, 0, 1, 0], "outputOnly", 0],
			isOver => ["SFBool", "FALSE", "outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			description => ["SFString", "", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"], # see note top of file
			# where we are at a press...
			metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			_origPoint => ["SFVec3f", [0, 0, 0], "initializeOnly", 0],
			_origNormalizedPoint => ["SFVec3f", [0, 0, 0], "initializeOnly", 0],
			_radius => ["SFFloat", 0, "initializeOnly", 0],
			__oldEnabled => ["SFBool", "TRUE", "inputOutput", 0],
					   ],"X3DPointingDeviceSensorNode"),

	"CylinderSensor" => new VRML::NodeType("CylinderSensor", [
			autoOffset => ["SFBool", "TRUE", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			axisRotation => ["SFRotation", [0, 1, 0, 0], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			diskAngle => ["SFFloat", 0.262, "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			enabled => ["SFBool", "TRUE", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			maxAngle => ["SFFloat", -1.0, "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			minAngle => ["SFFloat", 0, "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			offset => ["SFFloat", 0, "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			isActive => ["SFBool", "FALSE", "outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			isOver => ["SFBool", "FALSE", "outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			description => ["SFString", "", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"], # see note top of file
			rotation_changed => ["SFRotation", [0, 0, 1, 0], "outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			trackPoint_changed => ["SFVec3f", [0, 0, 0], "outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			_oldtrackPoint => ["SFVec3f", [0, 0, 0], "outputOnly", 0],
			_oldrotation => ["SFRotation", [0, 0, 1, 0], "outputOnly", 0],
			# where we are at a press...
			_origPoint => ["SFVec3f", [0, 0, 0], "initializeOnly", 0],
			_radius => ["SFFloat", 0, "initializeOnly", 0],
			_dlchange => ["SFInt32", 0, "initializeOnly", 0],
			__oldEnabled => ["SFBool", "TRUE", "inputOutput", 0],
					   ],"X3DPointingDeviceSensorNode"),


	###################################################################################

	#	21	Key Device Component

	###################################################################################

	# KeySensor
	"KeySensor" => new VRML::NodeType("KeySensor", [
			enabled => ["SFBool", "TRUE", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			actionKeyPress =>["SFInt32",0,"outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			actionKeyRelease =>["SFInt32",0,"outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			altKey =>["SFBool", "TRUE","outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			controlKey =>["SFBool", "TRUE","outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			isActive =>["SFBool", "TRUE","outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			keyPress =>["SFString","","outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			keyRelease =>["SFString","","outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			shiftKey =>["SFBool", "TRUE","outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			__oldEnabled => ["SFBool", "TRUE", "inputOutput", 0],
					   ],"X3DKeyDeviceSensorNode"),

	# StringSensor
	"StringSensor" => new VRML::NodeType("StringSensor", [
			deletionAllowed => ["SFBool", "TRUE", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			enabled => ["SFBool", "TRUE", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			enteredText => ["SFString","","outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			finalText => ["SFString","","outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			isActive =>["SFBool", "TRUE","outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			_initialized =>["SFBool", "FALSE","initializeOnly", 0],
			__oldEnabled => ["SFBool", "TRUE", "inputOutput", 0],
					   ],"X3DKeyDeviceSensorNode"),


	###################################################################################

	#	22	Environmental Sensor Component

	###################################################################################


	"ProximitySensor" => new VRML::NodeType("ProximitySensor", [
			center => ["SFVec3f", [0, 0, 0], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			size => ["SFVec3f", [0, 0, 0], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			enabled => ["SFBool", "TRUE", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			isActive => ["SFBool", "FALSE", "outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			position_changed => ["SFVec3f", [0, 0, 0], "outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			orientation_changed => ["SFRotation", [0, 0, 1, 0], "outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			enterTime => ["SFTime", -1, "outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			exitTime => ["SFTime", -1, "outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			centerOfRotation_changed =>["SFVec3f", [0,0,0], "outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],

			# These fields are used for the info.
			__hit => ["SFInt32", 0, "inputOutput", 0],
			__t1 => ["SFVec3f", [10000000, 0, 0], "inputOutput", 0],
			__t2 => ["SFRotation", [0, 1, 0, 0], "inputOutput", 0],
			__oldEnabled => ["SFBool", "TRUE", "inputOutput", 0],
	],"X3DEnvironmentalSensorNode"),
					   
	"TransformSensor" => new VRML::NodeType("TransformSensor", [
			center => ["SFVec3f", [0, 0, 0], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			size => ["SFVec3f", [0, 0, 0], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			enabled => ["SFBool", "TRUE", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			isActive => ["SFBool", "FALSE", "outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			position_changed => ["SFVec3f", [0, 0, 0], "outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			orientation_changed => ["SFRotation", [0, 0, 1, 0], "outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			enterTime => ["SFTime", -1, "outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			exitTime => ["SFTime", -1, "outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			targetObject => ["SFNode", "NULL", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],

			# These fields are used for the info.
			__hit => ["SFInt32", 0, "inputOutput", 0],
			__t1 => ["SFVec3f", [10000000, 0, 0], "inputOutput", 0],
			__t2 => ["SFRotation", [0, 1, 0, 0], "inputOutput", 0],
			__oldEnabled => ["SFBool", "TRUE", "inputOutput", 0],
	],"X3DEnvironmentalSensorNode"),
					   

	"VisibilitySensor" => new VRML::NodeType("VisibilitySensor", [
			center => ["SFVec3f", [0, 0, 0], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			enabled => ["SFBool", "TRUE", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			size => ["SFVec3f", [0, 0, 0], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			enterTime => ["SFTime", -1, "outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			exitTime => ["SFTime", -1, "outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			isActive => ["SFBool", "FALSE", "outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			 __visible =>["SFInt32",0,"initializeOnly", 0], # for Occlusion tests.
			 __occludeCheckCount =>["SFInt32",-1,"initializeOnly", 0], # for Occlusion tests.
			__points  =>["MFVec3f",[],"initializeOnly", 0],	# for Occlude Box.
			__Samples =>["SFInt32",0,"initializeOnly", 0],		# Occlude samples from last pass
			__oldEnabled => ["SFBool", "TRUE", "inputOutput", 0],
	],"X3DEnvironmentalSensorNode"),



	###################################################################################

	#	23	Navigation Component

	###################################################################################

	"LOD" => new VRML::NodeType("LOD", [
		addChildren => ["MFNode", undef, "inputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		removeChildren => ["MFNode", undef, "inputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		__sibAffectors => ["MFNode", [], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		level => ["MFNode", [], "inputOutput", "(SPEC_VRML)"], 		# for VRML spec
		children => ["MFNode", [], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],		# for X3D spec
		center => ["SFVec3f", [0, 0, 0],  "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"], # see note top of file
		range => ["MFFloat", [], "initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		bboxCenter => ["SFVec3f", [0, 0, 0], "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		bboxSize => ["SFVec3f", [-1, -1, -1], "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		levelChanged => ["SFInt32", 0, "outputOnly", "(SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		forceTransitions => ["SFBool", "FALSE", "initializeOnly", "(SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		__isX3D => ["SFBool", "(inputFileVersion[0]==3)" , "initializeOnly", 0], # "TRUE" for X3D V3.x files
		_selected =>["FreeWRLPTR",0,"initializeOnly", 0],
	],"X3DGroupingNode"),

	"Billboard" => new VRML::NodeType("Billboard", [
			addChildren => ["MFNode", undef, "inputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			removeChildren => ["MFNode", undef, "inputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			__sibAffectors => ["MFNode", [], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			axisOfRotation => ["SFVec3f", [0, 1, 0], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			children => ["MFNode", [], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			bboxCenter => ["SFVec3f", [0, 0, 0], "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			bboxSize => ["SFVec3f", [-1, -1, -1], "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			_rotationAngle =>["SFDouble", 0, "initializeOnly", 0],
			#JAS _sortedChildren => ["MFNode", [], "inputOutput", 0],
					   ],"X3DGroupingNode"),

	"Collision" => new VRML::NodeType("Collision", [
			addChildren => ["MFNode", undef, "inputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			removeChildren => ["MFNode", undef, "inputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			__sibAffectors => ["MFNode", [], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			children => ["MFNode", [], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			enabled => ["SFBool", "TRUE", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			collide => ["SFBool", "TRUE", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			bboxCenter => ["SFVec3f", [0, 0, 0], "initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			bboxSize => ["SFVec3f", [-1, -1, -1], "initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			proxy => ["SFNode", "NULL", "initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			collideTime => ["SFTime", -1, "outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			#JAS _sortedChildren => ["MFNode", [], "inputOutput", 0],
			# return info for collisions
			# bit 0 : collision or not
			# bit 1: changed from previous of not
			__hit => ["SFInt32", 0, "inputOutput", 0]
					   ],"X3DEnvironmentalSensorNode"),


	"Viewpoint" => new VRML::NodeType("Viewpoint", [
		#generic Viewpoint fields
		set_bind => ["SFBool", 100, "inputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		bindTime => ["SFTime", -1, "outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		isBound => ["SFBool", "FALSE", "outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		description => ["SFString", "", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"], # see note top of file
		jump => ["SFBool", "TRUE", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		fieldOfView => ["SFFloat", 0.785398, "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
                metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		orientation => ["SFRotation", [0, 0, 1, 0], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		position => ["SFVec3f",[0, 0, 10], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		_layerId => ["SFInt32",0,"initializeOnly",0],
		_donethispass => ["SFInt32",0,"initializeOnly",0],
		centerOfRotation =>["SFVec3f", [0,0,0], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		retainUserOffsets => ["SFBool", "FALSE", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		# augmented reality extensions:
		fovMode => ["SFString", "", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"], # see note top of file
		aspectRatio => ["SFFloat", 0.785398, "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		
	],"X3DBindableNode"),

	"OrthoViewpoint" => new VRML::NodeType("OrthoViewpoint", [
		#generic Viewpoint fields
		set_bind => ["SFBool", 100, "inputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		bindTime => ["SFTime", -1, "outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		isBound => ["SFBool", "FALSE", "outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		description => ["SFString", "", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"], # see note top of file
		jump => ["SFBool", "TRUE", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		fieldOfView => ["MFFloat", [-1.0, -1.0, 1.0, 1.0], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
                metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		orientation => ["SFRotation", [0, 0, 1, 0], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		position => ["SFVec3f",[0, 0, 10], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		_layerId => ["SFInt32",0,"initializeOnly",0],
		_donethispass => ["SFInt32",0,"initializeOnly",0],
		centerOfRotation =>["SFVec3f", [0,0,0], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		retainUserOffsets => ["SFBool", "FALSE", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
	],"X3DBindableNode"),



	"NavigationInfo" => new VRML::NodeType("NavigationInfo", [
		set_bind => ["SFBool", 100, "inputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		avatarSize => ["MFFloat", [0.25, 1.6, 0.75], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		headlight => ["SFBool", "TRUE", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		speed => ["SFFloat", 1.0, "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		type => ["MFString", ["EXAMINE", "ANY"], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		visibilityLimit => ["SFFloat", 0, "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		isBound => ["SFBool", "FALSE", "outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		_layerId => ["SFInt32",0,"initializeOnly",0],
		transitionType => ["MFString", [],"inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		bindTime => ["SFTime", -1, "outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		transitionTime => ["SFTime", 1.0, "inputOutput", "(SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		transitionComplete => ["SFBool", "FALSE", "outputOnly", "(SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
	],"X3DBindableNode"),

	"ViewpointGroup" => new VRML::NodeType("ViewpointGroup", [
		center => ["SFVec3f", [0, 0, 0], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		children => ["MFNode", [], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		description => ["SFString", "", "inputOutput", "(SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"], # see note top of file
		displayed => ["SFBool", "TRUE", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		retainUserOffsets => ["SFBool", "FALSE", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		size => ["SFVec3f", [0,0,0], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		__proxNode=> ["SFNode", "NULL", "inputOutput", "0"],
	],"X3DGroupingNode"),



	###################################################################################

	#	24	Environmental Effects Component

	###################################################################################

	"Background" => new VRML::NodeType("Background", [
		set_bind => ["SFBool", 100, "inputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		groundAngle => ["MFFloat", [], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		groundColor => ["MFColor", [], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		skyAngle => ["MFFloat", [], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		skyColor => ["MFColor", [[0, 0, 0]], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		bindTime => ["SFTime",0,"outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		isBound => ["SFBool", "FALSE", "outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		_layerId => ["SFInt32",0,"initializeOnly",0],
		_parentResource =>["FreeWRLPTR",0,"initializeOnly", 0],
		__points =>["MFVec3f",[],"initializeOnly", 0],
		__colours =>["MFColor",[],"initializeOnly", 0],
		__quadcount => ["SFInt32",0,"initializeOnly", 0],

		transparency => ["SFFloat", 0.0, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],

		frontUrl => ["MFString", [], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		backUrl => ["MFString", [], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		topUrl => ["MFString", [], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		bottomUrl => ["MFString", [], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		leftUrl => ["MFString", [], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		rightUrl => ["MFString", [], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		__textureright => ["SFInt32", 0, "inputOutput", 0],
		__frontTexture=>["SFNode","NULL","inputOutput", 0],
		__backTexture=>["SFNode","NULL","inputOutput", 0],
		__topTexture=>["SFNode","NULL","inputOutput", 0],
		__bottomTexture=>["SFNode","NULL","inputOutput", 0],
		__leftTexture=>["SFNode","NULL","inputOutput", 0],
		__rightTexture=>["SFNode","NULL","inputOutput", 0],

		__VBO=>["SFInt32",0,"initializeOnly",0],  # Vertex Buffer Object, if required.
	],"X3DBackgroundNode"),



	"Fog" => new VRML::NodeType("Fog", [
		#Fog interface - keep same order, offsets as LocalFog
		color => ["SFColor", [1, 1, 1], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		fogType => ["SFString", "LINEAR", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		visibilityRange => ["SFFloat", 0, "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
                __fogScale => ["SFFloat", 1.0, "inputOutput", 0],
                __fogType => ["SFInt32",1,"initializeOnly",0],
                #Bindable interface
		set_bind => ["SFBool", 100, "inputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		bindTime => ["SFTime", -1, "outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		isBound => ["SFBool", "FALSE", "outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		_layerId => ["SFInt32",0,"initializeOnly",0],
                metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
	  ],"X3DBindableNode"),

	"FogCoordinate" => new VRML::NodeType("FogCoordinate", [
		depth => ["MFFloat", [], "inputOutput", "(SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
                metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
					   ],"X3DGeometricPropertyNode"),

	"LocalFog" => new VRML::NodeType("Fog", [
		#Fog interface - keep same order, offsets as Fog
		color => ["SFColor", [1, 1, 1], "inputOutput", "(SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		fogType => ["SFString", "LINEAR", "inputOutput", "(SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		visibilityRange => ["SFFloat", 0, "inputOutput", "(SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
                __fogScale => ["SFFloat", 1.0, "inputOutput", 0],
                __fogType => ["SFInt32",1,"initializeOnly",0],
                #other
		enabled => ["SFBool", "TRUE", "inputOutput", "(SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
                metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
	],"X3DChildNode"),

	"TextureBackground" => new VRML::NodeType("TextureBackground", [
		set_bind => ["SFBool", 100, "inputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		groundAngle => ["MFFloat", [], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		groundColor => ["MFColor", [], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		skyAngle => ["MFFloat", [], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		skyColor => ["MFColor", [[0,0,0]], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		bindTime => ["SFTime",0,"outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		isBound => ["SFBool", "FALSE", "outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		_layerId => ["SFInt32",0,"initializeOnly",0],
                metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		_parentResource =>["FreeWRLPTR",0,"initializeOnly", 0],
		__points =>["MFVec3f",[],"initializeOnly", 0],
		__colours =>["MFVec3f",[],"initializeOnly", 0],
		__quadcount => ["SFInt32",0,"initializeOnly", 0],
		__VBO=>["SFInt32",0,"initializeOnly",0],  # Vertex Buffer Object, if required.

		frontTexture=>["SFNode","NULL","inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		backTexture=>["SFNode","NULL","inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		topTexture=>["SFNode","NULL","inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		bottomTexture=>["SFNode","NULL","inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		leftTexture=>["SFNode","NULL","inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		rightTexture=>["SFNode","NULL","inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		transparency=> ["MFFloat",[0],"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
	],"X3DBackgroundNode"),

	# see augmented reality for 2 more background nodes
	
	
	###################################################################################

	#	25	Geospatial Component

	###################################################################################


	"GeoCoordinate" => new VRML::NodeType("GeoCoordinate", [
			metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			point => ["MFVec3d",[],"inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"], # see note top of file
			geoOrigin => ["SFNode", "NULL", "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			geoSystem => ["MFString",["GD","WE"],"initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			__geoSystem => ["MFInt32",[],"initializeOnly", 0],
			__movedCoords => ["MFVec3f", [], "inputOutput", 0],
					],"X3DCoordinateNode"),

	"GeoElevationGrid" => new VRML::NodeType("GeoElevationGrid", [
		set_height => ["MFDouble", undef, "inputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		color => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		normal => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		texCoord => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		yScale => ["SFFloat", 1.0, "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		ccw => ["SFBool", "FALSE","initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		colorPerVertex => ["SFBool", "TRUE", "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		creaseAngle => ["SFDouble", 0, "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		geoGridOrigin => ["SFVec3d",[0,0,0],"initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		geoOrigin => ["SFNode", "NULL", "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		geoSystem => ["MFString",["GD","WE"],"initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		height => ["MFDouble", [0,0], "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		normalPerVertex => ["SFBool", "TRUE", "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		solid => ["SFBool", "TRUE", "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		xDimension => ["SFInt32", 0, "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		xSpacing => ["SFDouble", 1.0, "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		zDimension => ["SFInt32", 0, "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		zSpacing => ["SFDouble", 1.0, "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		_coordIndex => ["MFInt32", [], "initializeOnly", 0],

		__geoSystem => ["MFInt32",[],"initializeOnly", 0],
	],"X3DGeometryNode"),

	"GeoLOD" => new VRML::NodeType("GeoLOD", [
			metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],

			# the following screws up routing in the old VRML parser, because children can
			# be an "EXPOSED_FIELD" AND an "EVENT_OUT", so by changing this to an ""inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"
			# we can have only one field, the EXPOSED_FIELD_children
			#children => ["MFNode",[],"outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],

			children => ["MFNode", [], "outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			level_changed =>["SFInt32",0,"outputOnly", "(SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],

			center => ["SFVec3d",[0,0,0],"inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"], # see note top of file
			child1Url =>["MFString",[],"initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			child2Url =>["MFString",[],"initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			child3Url =>["MFString",[],"initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			child4Url =>["MFString",[],"initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			geoOrigin => ["SFNode", "NULL", "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			geoSystem => ["MFString",["GD","WE"],"initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			range => ["SFFloat",10.0,"initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			rootUrl => ["MFString",[],"initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			rootNode => ["MFNode",[],"initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			bboxCenter => ["SFVec3f", [0, 0, 0], "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			bboxSize => ["SFVec3f", [-1, -1, -1], "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],

			__geoSystem => ["MFInt32",[],"initializeOnly", 0],
			__movedCoords => ["SFVec3d", [0, 0, 0], "inputOutput", 0],
			__inRange =>["SFBool", "FALSE", "inputOutput", 0],
			__child1Node => ["SFNode", "NULL", "inputOutput", 0],
			__child2Node => ["SFNode", "NULL", "inputOutput", 0],
			__child3Node => ["SFNode", "NULL", "inputOutput", 0],
			__child4Node => ["SFNode", "NULL", "inputOutput", 0],
			__rootUrl => ["SFNode", "NULL", "inputOutput", 0],
			__childloadstatus => ["SFInt32",0,"inputOutput", 0],
			__rooturlloadstatus => ["SFInt32",0,"inputOutput", 0],

			# ProximitySensor copies.
			#__t1 => ["SFVec3d", [10000000, 0, 0], "inputOutput", 0],
			__level => ["SFInt32",-1,"inputOutput", 0], # only for debugging purposes
					],"X3DGroupingNode"),


	GeoMetadata=> new VRML::NodeType("GeoMetadata", [
			data => ["MFNode",[],"inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			summary => ["MFString",[],"inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			url => ["MFString",[],"inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
					],"X3DChildNode"),

	GeoPositionInterpolator=> new VRML::NodeType("GeoPositionInterpolator", [
			set_fraction => ["SFFloat",undef,"inputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			key => ["MFFloat",[],"inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			keyValue => ["MFVec3d",[],"inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			geovalue_changed => ["SFVec3d",[0,0,0],"outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			value_changed => ["SFVec3f",[0,0,0],"outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			geoOrigin => ["SFNode", "NULL", "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			geoSystem => ["MFString",["GD","WE"],"initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			__geoSystem => ["MFInt32",[],"initializeOnly", 0],
			__movedValue => ["MFVec3d", [], "inputOutput", 0],
			__oldKeyPtr => ["MFFloat", "NULL", "outputOnly", 0],
			__oldKeyValuePtr => ["MFVec3d", "NULL", "outputOnly", 0],
					],"X3DInterpolatorNode"),


	"GeoProximitySensor" => new VRML::NodeType("ProximitySensor", [
			enabled => ["SFBool", "TRUE", "inputOutput", "(SPEC_X3D33)"],
			geoCenter => ["SFVec3d", [0, 0, 0], "inputOutput", "(SPEC_X3D33)"],
			metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D33)"],
			size => ["SFVec3f", [0, 0, 0], "inputOutput", "(SPEC_X3D33)"],
			centerOfRotation_changed =>["SFVec3f", [0,0,0], "outputOnly", "(SPEC_X3D33)"],
			enterTime => ["SFTime", -1, "outputOnly", "(SPEC_X3D33)"],
			exitTime => ["SFTime", -1, "outputOnly", "(SPEC_X3D33)"],
			geoCoord_changed => ["SFVec3d",[0,0,0],"outputOnly", "(SPEC_X3D33)"],
			isActive => ["SFBool", "FALSE", "outputOnly", "(SPEC_X3D33)"],
			orientation_changed => ["SFRotation", [0, 0, 1, 0], "outputOnly", "(SPEC_X3D33)"],
			position_changed => ["SFVec3f", [0, 0, 0], "outputOnly", "(SPEC_X3D33)"],
			geoOrigin => ["SFNode", "NULL", "initializeOnly", "(SPEC_X3D33)"],
			geoSystem => ["MFString",["GD","WE"],"initializeOnly", "(SPEC_X3D33)"],


			# These fields are used for the info.
			__hit => ["SFInt32", 0, "inputOutput", 0],
			__t1 => ["SFVec3f", [10000000, 0, 0], "inputOutput", 0],
			__t2 => ["SFRotation", [0, 1, 0, 0], "inputOutput", 0],

			# "compiled" versions of strings above
			__geoSystem => ["MFInt32",[],"initializeOnly", 0],
			__movedCoords => ["SFVec3d", [0, 0, 0], "inputOutput", 0],
			__localOrient => ["SFVec4d", [0, 0, 1, 0], "inputOutput", 0],
			__oldEnabled => ["SFBool", "TRUE", "inputOutput", 0],
			__oldGeoCenter => ["SFVec3d", [0, 0, 0], "inputOutput", 0],
			__oldSize => ["SFVec3f", [0, 0, 0], "inputOutput", 0],
					   ],"X3DEnvironmentalSensorNode"),

	GeoTouchSensor=> new VRML::NodeType("GeoTouchSensor", [
			description => ["SFString", "", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"], # see note top of file
			enabled => ["SFBool", "FALSE","inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			hitNormal_changed => ["SFVec3f", [0, 0, 0], "outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			hitPoint_changed => ["SFVec3f", [0, 0, 0], "outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			hitTexCoord_changed => ["SFVec2f", [0, 0], "outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			hitGeoCoord_changed => ["SFVec3d", [0, 0, 0] ,"outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			isActive => ["SFBool", "FALSE", "outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			isOver => ["SFBool", "FALSE", "outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			touchTime => ["SFTime", -1, "outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			geoOrigin => ["SFNode", "NULL", "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			geoSystem => ["MFString",["GD","WE"],"initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			__geoSystem => ["MFInt32",[],"initializeOnly", 0],
			_oldhitNormal => ["SFVec3f", [0, 0, 0], "outputOnly", 0], 	# send event only if changed
			_oldhitPoint => ["SFVec3f", [0, 0, 0], "outputOnly", 0], 	# send event only if changed
			_oldhitTexCoord => ["SFVec2f", [0, 0], "outputOnly", 0], 	# send event only if changed
			__oldEnabled => ["SFBool", "TRUE", "inputOutput", 0],
					],"X3DPointingDeviceSensorNode"),


	"GeoTransform" => new VRML::NodeType ("GeoTransform", [
			addChildren => ["MFNode", undef, "inputOnly", "(SPEC_X3D33)"],
			removeChildren => ["MFNode", undef, "inputOnly", "(SPEC_X3D33)"],
			__sibAffectors => ["MFNode", [], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			children => ["MFNode", [], "inputOutput", "(SPEC_X3D33)"],
			geoCenter => ["SFVec3d", [0, 0, 0], "inputOutput", "(SPEC_X3D33)"],
			metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D33)"],
			rotation => ["SFRotation", [0, 0, 1, 0], "inputOutput", "(SPEC_X3D33)"],
			scale => ["SFVec3f", [1, 1, 1], "inputOutput", "(SPEC_X3D33)"],
			scaleOrientation => ["SFRotation", [0, 0, 1, 0], "inputOutput", "(SPEC_X3D33)"],
			translation => ["SFVec3f", [0, 0, 0], "inputOutput", "(SPEC_X3D33)"],
			bboxCenter => ["SFVec3f", [0, 0, 0], "initializeOnly", "(SPEC_X3D33)"],
			bboxSize => ["SFVec3f", [-1, -1, -1], "initializeOnly", "(SPEC_X3D33)"],
			geoOrigin => ["SFNode", "NULL", "initializeOnly", "(SPEC_X3D33)"],
			geoSystem => ["MFString",["GD","WE"],"initializeOnly", "(SPEC_X3D33)"],

			# fields for reducing redundant calls
			__do_center => ["SFInt32", 0, "initializeOnly", 0],
			__do_trans => ["SFInt32", 0, "initializeOnly", 0],
			__do_rotation => ["SFInt32", 0, "initializeOnly", 0],
			__do_scaleO => ["SFInt32", 0, "initializeOnly", 0],
			__do_scale => ["SFInt32", 0, "initializeOnly", 0],

			# "compiled" versions of strings above
			__geoSystem => ["MFInt32",[],"initializeOnly", 0],
			__movedCoords => ["SFVec3d", [0, 0, 0], "inputOutput", 0],
			__localOrient => ["SFVec4d", [0, 0, 1, 0], "inputOutput", 0],
			__oldGeoCenter => ["SFVec3d", [0, 0, 0], "inputOutput", 0],
			__oldChildren => ["MFNode", [], "inputOutput", 0],
		_sortedChildren => ["MFNode", [], "inputOutput", 0],
					],"X3DGroupingNode"),

	"GeoViewpoint" => new VRML::NodeType("GeoViewpoint", [
			# generic Viewpoint fields
			set_bind => ["SFBool", 100, "inputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			bindTime => ["SFTime", -1, "outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			isBound => ["SFBool", "FALSE", "outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			description => ["SFString", "", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			jump => ["SFBool", "TRUE", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			fieldOfView => ["SFFloat", 0.785398, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			orientation => ["SFRotation", [0, 0, 1, 0], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"], # see note top of file
			position => ["SFVec3d",[0, 0, 100000], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			_layerId => ["SFInt32",0,"initializeOnly",0],
			_donethispass => ["SFInt32",0,"initializeOnly",0],
			set_orientation => ["SFRotation", ["IO_FLOAT", "IO_FLOAT", "IO_FLOAT", "IO_FLOAT"], "inputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			set_position => ["SFVec3d", ["IO_FLOAT", "IO_FLOAT", "IO_FLOAT"], "inputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			# GeoViewpoint fields
			headlight => ["SFBool", "TRUE", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			navType => ["MFString", ["EXAMINE","ANY"],"inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			geoOrigin => ["SFNode", "NULL", "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			geoSystem => ["MFString",["GD","WE"],"initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			speedFactor => ["SFFloat",1.0,"initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],

			# "compiled" versions of strings above
			__geoSystem => ["MFInt32",[],"initializeOnly", 0],
			__movedPosition => ["SFVec3d", [0, 0, 0], "inputOutput", 0],
			__movedOrientation => ["SFRotation", [0, 0, 1, 0], "initializeOnly", 0],

			__oldSFString => ["SFString", "", "inputOutput", 0], #the description field
			__oldFieldOfView => ["SFFloat", 0.785398, "inputOutput", 0],
			__oldHeadlight => ["SFBool", "TRUE", "inputOutput", 0],
			__oldJump => ["SFBool", "TRUE", "inputOutput", 0],
			__oldMFString => ["MFString", [],"inputOutput", 0], # the navType

	],"X3DBindableNode"),

	"GeoOrigin" => new VRML::NodeType("GeoOrigin", [
			geoCoords => ["SFVec3d", [0, 0, 0], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			geoSystem => ["MFString",["GD","WE"],"initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			rotateYUp => ["SFBool", "FALSE","initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],

			# these are now static in CFuncs/GeoVRML.c
			# "compiled" versions of strings above
			__geoSystem => ["MFInt32",[],"initializeOnly", 0],
			__movedCoords => ["SFVec3d", [0, 0, 0], "inputOutput", 0],
			__oldgeoCoords => ["SFVec3d", [0, 0, 0], "inputOutput", 0],
			__oldMFString => ["MFString", [],"inputOutput", 0], # the navType
			__rotyup => ["SFVec4d", [0, 1, 0, 0], "inputOutput", 0],

					],"X3DChildNode"),

	"GeoLocation" => new VRML::NodeType("GeoLocation", [
			addChildren => ["MFNode", undef, "inputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			removeChildren => ["MFNode", undef, "inputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			__sibAffectors => ["MFNode", [], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			children => ["MFNode", [], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			geoCoords => ["SFVec3d", [0, 0, 0], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			geoOrigin => ["SFNode", "NULL", "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			geoSystem => ["MFString",["GD","WE"],"initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			bboxCenter => ["SFVec3f", [0, 0, 0], "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			bboxSize => ["SFVec3f", [-1, -1, -1], "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],

			# "compiled" versions of strings above
			__geoSystem => ["MFInt32",[],"initializeOnly", 0],
			__movedCoords => ["SFVec3d", [0, 0, 0], "inputOutput", 0],
			__localOrient => ["SFVec4d", [0, 0, 1, 0], "inputOutput", 0],
			__oldgeoCoords => ["SFVec3d", [0, 0, 0], "inputOutput", 0],
			__oldChildren => ["MFNode", [], "inputOutput", 0],
		_sortedChildren => ["MFNode", [], "inputOutput", 0],
					],"X3DGroupingNode"),


	###################################################################################

	#	26	H-Anim Component

	###################################################################################

	"HAnimDisplacer" => new VRML::NodeType("HAnimDisplacer", [
			coordIndex => ["MFInt32", [], "inputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"], # see note top of file
			displacements => ["MFVec3f", [], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			name => ["SFString", "", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			weight => ["SFFloat", 0.0, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
					],"X3DGeometricPropertyNode"),

	"HAnimHumanoid" => new VRML::NodeType("HAnimHumanoid", [
			center => ["SFVec3f", [0, 0, 0], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			info => ["MFString", [],"initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"], # see note top of file
			joints => ["MFNode",[],"inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			name => ["SFString", "", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			rotation => ["SFRotation",[0,0,1,0], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			scale => ["SFVec3f",[1,1,1],"inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			scaleOrientation => ["SFRotation", [0, 0, 1, 0], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			segments => ["MFNode",[],"inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			sites => ["MFNode",[],"inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			skeleton => ["MFNode",[],"inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			skin => ["MFNode",[],"inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			skinCoord => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			skinNormal => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			__sibAffectors => ["MFNode", [], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			translation => ["SFVec3f", [0, 0, 0], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			version => ["SFString","","inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			viewpoints => ["MFNode",[],"inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			bboxCenter => ["SFVec3f", [0, 0, 0], "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			bboxSize => ["SFVec3f", [-1, -1, -1], "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
					],"X3DChildNode"),

	"HAnimJoint" => new VRML::NodeType("HAnimJoint", [

			addChildren => ["MFNode", undef, "inputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			removeChildren => ["MFNode", undef, "inputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			__sibAffectors => ["MFNode", [], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			children => ["MFNode", [], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			center => ["SFVec3f", [0, 0, 0], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			rotation => ["SFRotation", [0, 0, 1, 0], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			scale => ["SFVec3f", [1, 1, 1], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			scaleOrientation => ["SFRotation", [0, 0, 1, 0], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			translation => ["SFVec3f", [0, 0, 0], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			displacers => ["MFNode", [], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			limitOrientation => ["SFRotation", [0, 0, 1, 0], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			llimit => ["MFFloat",[],"inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			name => ["SFString", "", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			skinCoordIndex => ["MFInt32",[],"inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			skinCoordWeight => ["MFFloat",[],"inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			stiffness => ["MFFloat",[0,0,0],"inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			ulimit => ["MFFloat",[],"inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			bboxCenter => ["SFVec3f", [0, 0, 0], "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			bboxSize => ["SFVec3f", [-1, -1, -1], "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],

			 # fields for reducing redundant calls
			 __do_center => ["SFInt32", 0, "initializeOnly", 0],
			 __do_trans => ["SFInt32", 0, "initializeOnly", 0],
			 __do_rotation => ["SFInt32", 0, "initializeOnly", 0],
			 __do_scaleO => ["SFInt32", 0, "initializeOnly", 0],
			 __do_scale => ["SFInt32", 0, "initializeOnly", 0],
					],"X3DChildNode"),

	"HAnimSegment" => new VRML::NodeType("HAnimSegment", [
			addChildren => ["MFNode", undef, "inputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			removeChildren => ["MFNode", undef, "inputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			__sibAffectors => ["MFNode", [], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			children => ["MFNode", [], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			name => ["SFString", "", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			bboxCenter => ["SFVec3f", [0, 0, 0], "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			bboxSize => ["SFVec3f", [-1, -1, -1], "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			centerOfMass => ["SFVec3f", [0, 0, 0], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			coord => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			displacers => ["MFNode",[],"inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			mass => ["SFFloat", 0, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			momentsOfInertia =>["MFFloat", [0, 0, 0, 0, 0, 0, 0, 0, 0],"inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
					],"X3DChildNode"),



	"HAnimSite" => new VRML::NodeType("HAnimSite", [
			addChildren => ["MFNode", undef, "inputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			removeChildren => ["MFNode", undef, "inputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			__sibAffectors => ["MFNode", [], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			children => ["MFNode", [], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			name => ["SFString", "", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			bboxCenter => ["SFVec3f", [0, 0, 0], "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			bboxSize => ["SFVec3f", [-1, -1, -1], "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			center => ["SFVec3f", [0, 0, 0], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			rotation => ["SFRotation", [0, 0, 1, 0], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			scale => ["SFVec3f", [1, 1, 1], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			scaleOrientation => ["SFRotation", [0, 0, 1, 0], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			translation => ["SFVec3f", [0, 0, 0], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],

			 # fields for reducing redundant calls
			 __do_center => ["SFInt32", 0, "initializeOnly", 0],
			 __do_trans => ["SFInt32", 0, "initializeOnly", 0],
			 __do_rotation => ["SFInt32", 0, "initializeOnly", 0],
			 __do_scaleO => ["SFInt32", 0, "initializeOnly", 0],
			 __do_scale => ["SFInt32", 0, "initializeOnly", 0],
					],"X3DGroupingNode"),


	###################################################################################

	#	27	NURBS Component

	###################################################################################

	"Contour2D" => new VRML::NodeType("Contour2D", [
			addChildren => ["MFNode", undef, "inputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			removeChildren => ["MFNode", undef, "inputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			__sibAffectors => ["MFNode", [], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			children => ["MFNode", [], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		],"X3DSFNode"),


	"ContourPolyLine2D" => new VRML::NodeType("ContourPolyline2D", [
			metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			controlPoint => ["MFVec2d", [], "inputOutput","(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		],"X3DNurbsControlCurveNode"),

	"CoordinateDouble" => new VRML::NodeType("CoordinateDouble", [
			metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			point => ["MFVec3d", [], "inputOutput","(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		],"X3DCoordinateNode"),

	"NurbsCurve" => new VRML::NodeType("NurbsCurve", [
			metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			controlPoint =>["SFNode","NULL","inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			weight => ["MFDouble",[],"inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			knot => ["MFDouble",[],"initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			order => ["SFInt32",3,"inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			tessellation => ["SFInt32",0,"inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			__points  =>["MFVec3f",[],"initializeOnly", 0],
			__numPoints =>["SFInt32",0,"initializeOnly", 0],
		],"X3DParametricGeometryNode"),

	"NurbsCurve2D" => new VRML::NodeType("NurbsCurve2D", [
			metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			controlPoint =>["MFVec2d",[],"inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			weight => ["MFDouble",[],"inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			knot => ["MFDouble",[],"initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			order => ["SFInt32",3,"inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			tessellation => ["SFInt32",0,"inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		],"X3DNurbsControlCurveNode"),


	"NurbsOrientationInterpolator" => new VRML::NodeType("NurbsOrientationInterpolator", [
			metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			controlPoint =>["SFNode","NULL","inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			weight  => ["MFDouble",[],"inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			knot => ["MFDouble",[],"inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			order => ["SFInt32",3,"inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			set_fraction => ["SFFloat",undef,"inputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			value_changed => ["SFRotation",[0,0,0,0],"outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		],"X3DChildNode"),

	"NurbsPatchSurface" => new VRML::NodeType("NurbsPatchSurface", [
			metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			controlPoint =>["SFNode","NULL","inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			weight => ["MFDouble",[],"inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			uKnot => ["MFDouble",[],"initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			uOrder => ["SFInt32",3,"inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			uDimension => ["SFInt32",0,"inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			uTessellation => ["SFInt32", 0, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			uClosed => ["SFBool", "FALSE","initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			vKnot => ["MFDouble",[],"initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			vOrder => ["SFInt32",3,"inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			vDimension => ["SFInt32",0,"inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			vTessellation => ["SFInt32", 0, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			vClosed => ["SFBool", "FALSE","initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			texCoord => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			solid => ["SFBool", "TRUE", "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		],"X3DNurbsSurfaceGeometryNode"),

	"NurbsPositionInterpolator" => new VRML::NodeType("NurbsPositionInterpolator", [
			metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			controlPoint =>["SFNode","NULL","inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			weight => ["MFDouble",[],"inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			knot => ["MFDouble",[],"inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			order => ["SFInt32",3,"inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			set_fraction => ["SFFloat",undef,"inputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			value_changed => ["SFVec3f",[0,0,0],"outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		],"X3DChildNode"),

	"NurbsSet" => new VRML::NodeType("NurbsSet", [
			addGeometry => ["MFNode", undef, "inputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			removeGeometry => ["MFNode", undef, "inputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			geometry => ["MFNode", [], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			tessellationScale => ["SFFloat",1.0,"inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			bboxCenter => ["SFVec3f", [0, 0, 0], "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			bboxSize => ["SFVec3f", [-1, -1, -1], "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		],"X3DChildNode"),

	"NurbsSurfaceInterpolator" => new VRML::NodeType("NurbsSurfaceInterpolator", [
			metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			controlPoint =>["SFNode","NULL","inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			weight => ["MFDouble",[],"inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			uKnot => ["MFDouble",[],"initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			uOrder => ["SFInt32",3,"inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			uDimension => ["SFInt32",0,"inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			vKnot => ["MFDouble",[],"initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			vOrder => ["SFInt32",3,"inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			vDimension => ["SFInt32",0,"inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			set_fraction => ["SFVec2f",undef,"inputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			position_changed => ["SFVec3f",[0,0,0],"outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			normal_changed => ["SFVec3f",[0,0,0],"outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		],"X3DChildNode"),

	"NurbsSweptSurface" => new VRML::NodeType("NurbsSweptSurface", [
			crossSectionCurve =>["SFNode","NULL","inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			trajectoryCurve => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			ccw => ["SFBool", "TRUE", "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			solid => ["SFBool", "TRUE", "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		],"X3DParametricGeometryNode"),

	"NurbsSwungSurface" => new VRML::NodeType("NurbsSwungSurface", [
			metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			profileCurve =>["SFNode","NULL","inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			trajectoryCurve => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			ccw => ["SFBool", "TRUE", "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			solid => ["SFBool", "TRUE", "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		],"X3DParametricGeometryNode"),

	"NurbsTextureCoordinate" => new VRML::NodeType("NurbsTextureCoordinate", [
			metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			controlPoint =>["MFVec2f",[],"inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			weight => ["MFFloat",[],"inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			uKnot => ["MFDouble",[],"initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			uOrder => ["SFInt32",3,"inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			uDimension => ["SFInt32",0,"inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			vKnot => ["MFDouble",[],"initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			vOrder => ["SFInt32",3,"inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			vDimension => ["SFInt32",0,"inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		], "X3DSFNode"),

	#TrimmedSurface == PatchSurface + trimmingContour - keep them in the same order so Trimmed can be downcast to Patch
	"NurbsTrimmedSurface" => new VRML::NodeType("NurbsTrimmedSurface", [
			metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			controlPoint =>["SFNode","NULL","inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			weight => ["MFDouble",[],"inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			uKnot => ["MFDouble",[],"initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			uOrder => ["SFInt32",3,"inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			uDimension => ["SFInt32",0,"inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			uTessellation => ["SFInt32", 0, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			uClosed => ["SFBool", "FALSE","initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			vKnot => ["MFDouble",[],"initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			vOrder => ["SFInt32",3,"inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			vDimension => ["SFInt32",0,"inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			vTessellation => ["SFInt32", 0, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			vClosed => ["SFBool", "FALSE","initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			texCoord => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			solid => ["SFBool", "TRUE", "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			addTrimmingContour => ["MFNode",[],"inputOnly","(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			removeTrimmingContour => ["MFNode",[],"inputOnly","(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			trimmingContour =>["MFNode",[], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		], "X3DNurbsSurfaceGeometryNode"),


	###################################################################################

	# Chapter 28: Distributed Interactive Simulation Component

	###################################################################################


	"DISEntityManager" => new VRML::NodeType("DISEntityManager", [
		address => ["SFString", "localhost", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		applicationID => ["SFInt32", 1, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		mapping => ["MFNode", [], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		port => ["SFInt32", 0, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		siteID => ["SFInt32", 0, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		addedEntities => ["MFNode", [], "outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		removedEntities => ["MFNode", [], "outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
	],"X3DChildNode"),

	"DISEntityTypeMapping" => new VRML::NodeType("DISEntityTypeMapping", [
		metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		url => ["MFString", [], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		category => ["SFInt32", 0, "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		country => ["SFInt32", 0, "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		domain => ["SFInt32", 0, "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		extra => ["SFInt32", 0, "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		kind => ["SFInt32", 0, "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		specific => ["SFInt32", 0, "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		subcategory => ["SFInt32", 0, "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],

	],"X3DInfoNode"),


	"EspduTransform" => new VRML::NodeType("EspduTransform", [
		addChildren => ["MFNode", [], "inputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		removeChildren => ["MFNode", [], "inputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		__sibAffectors => ["MFNode", [], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		set_articulationParameterValue0 => ["SFFloat", 0.0, "inputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		set_articulationParameterValue1 => ["SFFloat", 0.0, "inputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		set_articulationParameterValue2 => ["SFFloat", 0.0, "inputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		set_articulationParameterValue3 => ["SFFloat", 0.0, "inputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		set_articulationParameterValue4 => ["SFFloat", 0.0, "inputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		set_articulationParameterValue5 => ["SFFloat", 0.0, "inputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		set_articulationParameterValue6 => ["SFFloat", 0.0, "inputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		set_articulationParameterValue7 => ["SFFloat", 0.0, "inputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		address => ["SFString", "localhost", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		applicationID => ["SFInt32", 1, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		articulationParameterCount => ["SFInt32", 0, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		articulationParameterDesignatorArray => ["MFInt32", [], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		articulationParameterChangeIndicatorArr => ["MFInt32", [], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		articulationParameterIdPartAttachedToAr => ["MFInt32", [], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		articulationParameterTypeArray => ["MFInt32", [], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		articulationParameterArray => ["MFFloat", [], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		center => ["SFVec3f", [0,0,0], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		children => ["MFNode", [], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		collisionType => ["SFInt32", 0, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		deadReckoning => ["SFInt32", 0, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		detonationLocation => ["SFVec3f", [0,0,0], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		detonationRelativeLocation => ["SFVec3f", [0,0,0], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		detonationResult => ["SFInt32", 0, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		enabled => ["SFBool", "TRUE", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		entityCategory => ["SFInt32", 0, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		entityCountry => ["SFInt32", 0, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		entityDomain => ["SFInt32", 0, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		entityExtra => ["SFInt32", 0, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		entityID => ["SFInt32", 0, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		entityKind => ["SFInt32", 0, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		entitySpecific => ["SFInt32", 0, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		entitySubCategory => ["SFInt32", 0, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		eventApplicationID => ["SFInt32", 1, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		eventEntityID => ["SFInt32", 0, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		eventNumber => ["SFInt32", 0, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		eventSiteID => ["SFInt32", 0, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		fired1 => ["SFBool", "FALSE", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		fired2 => ["SFBool", "FALSE", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		fireMissionIndex => ["SFInt32", 0, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		firingRange => ["SFFloat", 0.0, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		firingRate => ["SFInt32", 0, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		forceID => ["SFInt32", 0, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		fuse => ["SFInt32", 0, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		linearVelocity => ["SFVec3f", [0,0,0], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		linearAcceleration => ["SFVec3f", [0,0,0], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		marking => ["SFString", "", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		multicastRelayHost => ["SFString", "", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		multicastRelayPort => ["SFInt32", 0, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		munitionApplicationID => ["SFInt32", 1, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		munitionEndPoint => ["SFVec3f", [0,0,0], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		munitionEntityID => ["SFInt32", 0, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		munitionQuantity => ["SFInt32", 0, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		munitionSiteID => ["SFInt32", 0, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		munitionStartPoint => ["SFVec3f", [0,0,0], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		networkMode => ["SFString", "standAlone", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		port => ["SFInt32", 0, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		readInterval => ["SFTime", 0.1, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		rotation => ["SFRotation", [0,0,1,0], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		scale => ["SFVec3f", [1,1,1], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		scaleOrientation => ["SFRotation", [0,0,1,0], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		siteID => ["SFInt32", 0, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		translation => ["SFVec3f", [0,0,0], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		warhead => ["SFInt32", 0, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		writeInterval => ["SFTime", 1.0, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		articulationParameterValue0_changed => ["SFFloat", 0.0, "outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		articulationParameterValue1_changed => ["SFFloat", 0.0, "outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		articulationParameterValue2_changed => ["SFFloat", 0.0, "outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		articulationParameterValue3_changed => ["SFFloat", 0.0, "outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		articulationParameterValue4_changed => ["SFFloat", 0.0, "outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		articulationParameterValue5_changed => ["SFFloat", 0.0, "outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		articulationParameterValue6_changed => ["SFFloat", 0.0, "outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		articulationParameterValue7_changed => ["SFFloat", 0.0, "outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		collideTime => ["SFTime", 0, "outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		detonateTime => ["SFTime", 0, "outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		firedTime => ["SFTime", 0, "outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		isActive => ["SFBool", "FALSE", "outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		isCollided => ["SFBool", "FALSE", "outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		isDetonated => ["SFBool", "FALSE", "outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		isNetworkReader => ["SFBool", "FALSE", "outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		isNetworkWriter => ["SFBool", "FALSE", "outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		isRtpHeaderHeard => ["SFBool", "FALSE", "outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		isStandAlone => ["SFBool", "FALSE", "outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		timestamp => ["SFTime", 0, "outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		bboxCenter => ["SFVec3f", [0,0,0], "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		bboxSize => ["SFVec3f", [-1,-1,-1], "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		rtpHeaderExpected => ["SFBool", "FALSE", "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
	], "X3DGroupingNode"),


	"ReceiverPdu" => new VRML::NodeType("ReceiverPdu", [
		address => ["SFString", "localhost", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		applicationID => ["SFInt32", 1, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		enabled => ["SFBool", "TRUE", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		entityID => ["SFInt32", 0, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		multicastRelayHost => ["SFString", "", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		multicastRelayPort => ["SFInt32", 0, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		networkMode => ["SFString", "standAlone", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		port => ["SFInt32", 0, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		radioID => ["SFInt32", 0, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		readInterval => ["SFFloat", 0.1, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		receivedPower => ["SFFloat", 0.0, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		receiverState => ["SFInt32", 0, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		rtpHeaderExpected => ["SFBool", "FALSE", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		siteID => ["SFInt32", 0, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		transmitterApplicationID => ["SFInt32", 1, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		transmitterEntityID => ["SFInt32", 0, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		transmitterRadioID => ["SFInt32", 0, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		transmitterSiteID => ["SFInt32", 0, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		whichGeometry => ["SFInt32", 1, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		writeInterval => ["SFFloat", 1.0, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		isActive => ["SFBool", "FALSE", "outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		isNetworkReader => ["SFBool", "FALSE", "outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		isNetworkWriter => ["SFBool", "FALSE", "outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		isRtpHeaderHeard => ["SFBool", "FALSE", "outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		isStandAlone => ["SFBool", "FALSE", "outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		timestamp => ["SFTime", 0, "outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		bboxCenter => ["SFVec3f", [0,0,0], "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		bboxSize => ["SFVec3f", [-1,-1,-1], "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
	], "X3DChildNode"),

	"SignalPdu" => new VRML::NodeType("SignalPdu", [
		address => ["SFString", "localhost", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		applicationID => ["SFInt32", 1, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		data => ["MFInt32", [], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		dataLength => ["SFInt32", 0, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		enabled => ["SFBool", "TRUE", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		encodingScheme => ["SFInt32", 0, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		entityID => ["SFInt32", 0, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		multicastRelayHost => ["SFString", "", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		multicastRelayPort => ["SFInt32", 0, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		networkMode => ["SFString", "standAlone", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		port => ["SFInt32", 0, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		radioID => ["SFInt32", 0, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		readInterval => ["SFFloat", 0.1, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		rtpHeaderExpected => ["SFBool", "FALSE", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		sampleRate => ["SFInt32", 0, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		samples => ["SFInt32", 0, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		siteID => ["SFInt32", 0, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		tdlType => ["SFInt32", 0, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		whichGeometry => ["SFInt32", 1, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		writeInterval => ["SFFloat", 1.0, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		isActive => ["SFBool", "FALSE", "outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		isNetworkReader => ["SFBool", "FALSE", "outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		isNetworkWriter => ["SFBool", "FALSE", "outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		isRtpHeaderHeard => ["SFBool", "FALSE", "outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		isStandAlone => ["SFBool", "FALSE", "outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		timestamp => ["SFTime", "FALSE", "outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		bboxCenter => ["SFVec3f", [0,0,0], "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		bboxSize => ["SFVec3f", [-1,-1,-1], "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
	], "X3DChildNode"),

	"TransmitterPdu" => new VRML::NodeType("TransmitterPdu", [
		address => ["SFString", "localhost", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		antennaLocation => ["SFVec3f", [0,0,0], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		antennaPatternLength => ["SFInt32", 0, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		antennaPatternType => ["SFInt32", 0, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		applicationID => ["SFInt32", 1, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		cryptoKeyID => ["SFInt32", 0, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		cryptoSystem => ["SFInt32", 0, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		enabled => ["SFBool", "TRUE", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		entityID => ["SFInt32", 0, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		frequency => ["SFInt32", 0, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		inputSource => ["SFInt32", 0, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		lengthOfModulationParameters => ["SFInt32", 0, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		modulationTypeDetail => ["SFInt32", 0, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		modulationTypeMajor => ["SFInt32", 0, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		modulationTypeSpreadSpectrum => ["SFInt32", 0, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		modulationTypeSystem => ["SFInt32", 0, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		multicastRelayHost => ["SFString", "", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		multicastRelayPort => ["SFInt32", 0, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		networkMode => ["SFString", "standAlone", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		port => ["SFInt32", 0, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		power => ["SFFloat", 0.0, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		radioEntityTypeCategory => ["SFInt32", 0, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		radioEntityTypeCountry => ["SFInt32", 0, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		radioEntityTypeDomain => ["SFInt32", 0, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		radioEntityTypeKind => ["SFInt32", 0, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		radioEntityTypeNomenclature => ["SFInt32", 0, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		radioEntityTypeNomenclatureVersion => ["SFInt32", 0, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		radioID => ["SFInt32", 0, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		readInterval => ["SFFloat", 0.1, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		relativeAntennaLocation => ["SFVec3f", [0,0,0], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		rtpHeaderExpected => ["SFBool", "FALSE", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		siteID => ["SFInt32", 0, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		transmitFrequencyBandwidth => ["SFFloat", 0.0, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		transmitState => ["SFInt32", 0, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		whichGeometry => ["SFInt32", 1, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		writeInterval => ["SFFloat", 1.0, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		isActive => ["SFBool", "FALSE", "outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		isNetworkReader => ["SFBool", "FALSE", "outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		isNetworkWriter => ["SFBool", "FALSE", "outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		isRtpHeaderHeard => ["SFBool", "FALSE", "outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		isStandAlone => ["SFBool", "FALSE", "outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		timestamp => ["SFTime", 0, "outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		bboxCenter => ["SFVec3f", [0,0,0], "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		bboxSize => ["SFVec3f", [-1,-1,-1], "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
	], "X3DChildNode"),





	###################################################################################

	#	29.	Scripting Component

	###################################################################################
	"Script" => new VRML::NodeType("Script",
					   [
			url => ["MFString", [], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			directOutput => ["SFBool", "FALSE", "initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			mustEvaluate => ["SFBool", "FALSE", "initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			 __scriptObj => ["FreeWRLPTR", 0, "initializeOnly", 0],
		_parentResource =>["FreeWRLPTR",0,"initializeOnly", 0],
					   ],"X3DScriptNode"
					  ),

	###################################################################################

	#	32.	CAD Component

	###################################################################################

	"CADAssembly" => new VRML::NodeType("CADAssembly", [
		addChildren => ["MFNode", undef, "inputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		removeChildren => ["MFNode", undef, "inputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		__sibAffectors => ["MFNode", [], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		children => ["MFNode", [], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		name => ["SFString", "", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		bboxCenter => ["SFVec3f", [0,0,0], "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		bboxSize => ["SFVec3f", [-1,-1,-1], "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		_sortedChildren => ["MFNode", [], "inputOutput", 0],
		], "X3DGroupingNode"
	),

	"CADFace" => new VRML::NodeType("CADFace", [
		metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		name => ["SFString", "", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		shape => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		bboxCenter => ["SFVec3f", [0,0,0], "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		bboxSize => ["SFVec3f", [-1,-1,-1], "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		], "X3DProductStructureChildNode"
	),

	"CADLayer" => new VRML::NodeType("CADLayer", [
		addChildren => ["MFNode", undef, "inputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		removeChildren => ["MFNode", undef, "inputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		__sibAffectors => ["MFNode", [], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		children => ["MFNode", [], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		name => ["SFString", "", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		visible => ["MFBool", [], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		bboxCenter => ["SFVec3f", [0,0,0], "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		bboxSize => ["SFVec3f", [-1,-1,-1], "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		], "X3DGroupingNode"
	),

	"CADPart" => new VRML::NodeType("CADPart", [
		addChildren => ["MFNode", undef, "inputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		removeChildren => ["MFNode", undef, "inputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		__sibAffectors => ["MFNode", [], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		center => ["SFVec3f",[0,0,0],"inputOutput","(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		children => ["MFNode", [], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		name => ["SFString", "", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],

		rotation => ["SFRotation", [0, 0, 1, 0], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		scale => ["SFVec3f", [1, 1, 1], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		scaleOrientation => ["SFRotation", [0, 0, 1, 0], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		translation => ["SFVec3f", [0, 0, 0], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		bboxCenter => ["SFVec3f", [0, 0, 0], "initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		bboxSize => ["SFVec3f", [-1, -1, -1], "initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],

		# fields for reducing redundant calls
		__do_center => ["SFInt32", "FALSE", "initializeOnly", 0],
		__do_trans => ["SFInt32", "FALSE", "initializeOnly", 0],
		__do_rotation => ["SFInt32", "FALSE", "initializeOnly", 0],
		__do_scaleO => ["SFInt32", "FALSE", "initializeOnly", 0],
		__do_scale => ["SFInt32", "FALSE", "initializeOnly", 0],
		__do_anything => ["SFInt32", "FALSE", "initializeOnly", 0],
		_sortedChildren => ["MFNode", [], "inputOutput", 0],
		] ,"X3DGroupingNode"
	),

	"IndexedQuadSet" => new VRML::NodeType("IndexedQuadSet", [
		set_index => ["MFInt32", undef, "inputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		attrib  => ["MFNode", [], "inputOutput", "(SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		color => ["SFNode", "NULL", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		coord => ["SFNode", "NULL", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		fogCoord => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		normal => ["SFNode", "NULL", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		texCoord => ["SFNode", "NULL", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		ccw => ["SFBool", "TRUE", "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		index => ["MFInt32", [], "initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		colorPerVertex => ["SFBool", "TRUE", "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		normalPerVertex => ["SFBool", "TRUE", "initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		solid => ["SFBool", "TRUE", "initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		_coordIndex => ["MFInt32", [], "initializeOnly", 0],
		], "X3DComposedGeometryNode"
	),

	"QuadSet" => new VRML::NodeType("QuadSet", [
		attrib  => ["MFNode", [], "inputOutput", "(SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		color => ["SFNode", "NULL", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		coord => ["SFNode", "NULL", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		fogCoord => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		normal => ["SFNode", "NULL", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		texCoord => ["SFNode", "NULL", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		ccw => ["SFBool", "TRUE", "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		colorPerVertex => ["SFBool", "TRUE", "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		normalPerVertex => ["SFBool", "TRUE", "initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		solid => ["SFBool", "TRUE", "initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		_coordIndex => ["MFInt32", [], "initializeOnly", 0],
		], "X3DComposedGeometryNode"
	),


	###################################################################################

	#	30.	EventUtilities Component

	###################################################################################

	"BooleanFilter" => new VRML::NodeType("BooleanFilter", [
			set_boolean =>["SFBool",undef,"inputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			inputFalse => ["SFBool", "FALSE", "outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			inputNegate => ["SFBool", "FALSE", "outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			inputTrue => ["SFBool", "TRUE", "outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
	],"X3DChildNode"),


	"BooleanSequencer" => new VRML::NodeType("BooleanSequencer", [
			next =>["SFBool",undef,"inputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			previous =>["SFBool",undef,"inputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			set_fraction =>["SFFloat",undef,"inputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			key => ["MFFloat", [], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			keyValue => ["MFBool", [], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			value_changed => ["SFBool", "FALSE", "outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
	],"X3DSequencerNode"),


	"BooleanToggle" => new VRML::NodeType("BooleanToggle", [
			set_boolean =>["SFBool",undef,"inputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			toggle => ["SFBool", "FALSE", "outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
	],"X3DChildNode"),


	"BooleanTrigger" => new VRML::NodeType("BooleanTrigger", [
			set_triggerTime => ["SFTime",undef ,"inputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			triggerTrue => ["SFBool", "FALSE", "outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
	],"X3DTriggerNode"),


	"IntegerSequencer" => new VRML::NodeType("IntegerSequencer", [
			next =>["SFBool",undef,"inputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			previous =>["SFBool",undef,"inputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			set_fraction =>["SFFloat",undef,"inputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			key => ["MFFloat", [], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			keyValue => ["MFInt32", [], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			value_changed => ["SFInt32", 0, "outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
	],"X3DSequencerNode"),

	"IntegerTrigger" => new VRML::NodeType("IntegerTrigger", [
			set_boolean =>["SFBool",undef,"inputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			integerKey => ["SFInt32", 0, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			triggerValue => ["SFInt32", 0, "outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
	],"X3DTriggerNode"),

	"TimeTrigger" => new VRML::NodeType("TimeTrigger", [
			set_boolean =>["SFBool",undef,"inputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			triggerTime => ["SFTime", 0, "outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
	],"X3DTriggerNode"),


	###################################################################################

	#	31.	ProgrammableShaders Component

	###################################################################################

	"ComposedShader" => new VRML::NodeType("ComposedShader", [
			activate =>["SFBool",undef,"inputOnly", "(SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			parts => ["MFNode",[],"inputOutput", "(SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			isSelected => ["SFBool", "TRUE","outputOnly", "(SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			isValid => ["SFBool", "TRUE","outputOnly", "(SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			language => ["SFString", "", "initializeOnly", "(SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],

			_initialized => ["SFBool", "FALSE" ,"initializeOnly", 0],
			_shaderUserDefinedFields => ["SFNode", "NULL", "initializeOnly", 0],
			_shaderUserNumber => ["SFInt32",-1,"initializeOnly",0],
			_shaderLoadThread => ["FreeWRLThread", 0, "initializeOnly",0],
			_retrievedURLData => ["SFBool", "FALSE" ,"initializeOnly", 0],
	],"X3DShaderNode"),


	"FloatVertexAttribute" => new VRML::NodeType("FloatVertexAttribute", [
			value => ["MFFloat",[],"inputOutput", "(SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			name => ["SFString","","inputOutput", "(SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			numComponents => ["SFInt32", 4, "initializeOnly", "(SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"], # 1...4 valid values
			metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
	], "X3DVertexAttributeNode"),

	"Matrix3VertexAttribute" => new VRML::NodeType("Matrix3VertexAttribute", [
			value => ["MFMatrix3f",[],"inputOutput", "(SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			name => ["SFString","","inputOutput", "(SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
	], "X3DVertexAttributeNode"),

	"Matrix4VertexAttribute" => new VRML::NodeType("Matrix4VertexAttribute", [
			metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			value => ["MFMatrix4f",[],"inputOutput", "(SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			name => ["SFString","","inputOutput", "(SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
	], "X3DVertexAttributeNode"),

	"PackagedShader" => new VRML::NodeType("PackagedShader", [
			activate =>["SFBool",undef,"inputOnly", "(SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			url => ["MFString", [], "inputOutput", "(SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			isSelected => ["SFBool", "TRUE","outputOnly", "(SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			isValid => ["SFBool", "TRUE","outputOnly", "(SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			language => ["SFString","","initializeOnly", "(SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],

			_initialized => ["SFBool", "FALSE" ,"initializeOnly", 0],
			_shaderUserNumber => ["SFInt32",-1,"initializeOnly",0],
			_shaderUserDefinedFields => ["SFNode", "NULL", "initializeOnly", 0],
			_shaderLoadThread => ["FreeWRLThread", 0, "initializeOnly",0],
			_retrievedURLData => ["SFBool", "FALSE" ,"initializeOnly", 0],
	], "X3DProgrammableShaderObject"),

	"ProgramShader" => new VRML::NodeType("ProgramShader", [
			activate =>["SFBool",undef,"inputOnly", "(SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			programs => ["MFNode", [], "inputOutput", "(SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			isSelected => ["SFBool", "TRUE","outputOnly", "(SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			isValid => ["SFBool", "TRUE","outputOnly", "(SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			language => ["SFString","","initializeOnly", "(SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],

			_initialized => ["SFBool", "FALSE" ,"initializeOnly", 0],
			_shaderUserNumber => ["SFInt32",-1,"initializeOnly",0],
			_shaderLoadThread => ["FreeWRLThread", 0, "initializeOnly",0],
			_retrievedURLData => ["SFBool", "FALSE" ,"initializeOnly", 0],
	], "X3DProgrammableShaderObject"),

	"ShaderPart" => new VRML::NodeType("ShaderPart", [
			metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			url => ["MFString", [], "inputOutput", "(SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			type => ["SFString","VERTEX","inputOutput", "(SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"], # see note top of file
			__loadstatus =>["SFInt32",0,"initializeOnly", 0],
			_parentResource =>["FreeWRLPTR",0,"initializeOnly", 0],
			__loadResource => ["FreeWRLPTR", 0, "initializeOnly", 0],
			_shaderUserDefinedFields => ["SFNode", "NULL", "initializeOnly", 0],
	], "X3DUrlObject"),

	"ShaderProgram" => new VRML::NodeType("ShaderProgram", [
			metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			url => ["MFString", [], "inputOutput", "(SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			type => ["SFString","","inputOutput", "(SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"], # see note top of file
			__loadstatus =>["SFInt32",0,"initializeOnly", 0],
			_parentResource =>["FreeWRLPTR",0,"initializeOnly", 0],
			__loadResource => ["FreeWRLPTR", 0, "initializeOnly", 0],
			_shaderUserDefinedFields => ["SFNode", "NULL", "initializeOnly", 0],
	], "X3DUrlObject"),

	# castle EffectPart made from ShaderPart - fields in same order
	"EffectPart" => new VRML::NodeType("EffectPart", [
			metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			url => ["MFString", [], "inputOutput", "(SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			type => ["SFString","VERTEX","inputOutput", "(SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"], # see note top of file
			__loadstatus =>["SFInt32",0,"initializeOnly", 0],
			_parentResource =>["FreeWRLPTR",0,"initializeOnly", 0],
			__loadResource => ["FreeWRLPTR", 0, "initializeOnly", 0],
			_shaderUserDefinedFields => ["SFNode", "NULL", "initializeOnly", 0],
	], "X3DUrlObject"),

	# castle Effect made from ComposedShader - fields in same order
	"Effect" => new VRML::NodeType("Effect", [
			activate =>["SFBool",undef,"inputOnly", "(SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			parts => ["MFNode",[],"inputOutput", "(SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			isSelected => ["SFBool", "TRUE","outputOnly", "(SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			isValid => ["SFBool", "TRUE","outputOnly", "(SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			language => ["SFString", "", "initializeOnly", "(SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],

			_initialized => ["SFBool", "FALSE" ,"initializeOnly", 0],
			_shaderUserDefinedFields => ["SFNode", "NULL", "initializeOnly", 0],
			_shaderUserNumber => ["SFInt32",-1,"initializeOnly",0],
			_shaderLoadThread => ["FreeWRLThread", 0, "initializeOnly",0],
			_retrievedURLData => ["SFBool", "FALSE" ,"initializeOnly", 0],
	],"X3DShaderNode"),


	###################################################################################

	#	33.	Texturing3D Component

	###################################################################################
	"ImageTexture3D" => new VRML::NodeType("ImageTexture3D", [
                metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		url => ["MFString", [], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		repeatS => ["SFBool", "FALSE", "initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		repeatT => ["SFBool", "FALSE", "initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		repeatR => ["SFBool", "FALSE", "initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		textureProperties => ["SFNode", 0, "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		__textureTableIndex => ["SFInt32", 0, "initializeOnly", 0],
		_parentResource =>["FreeWRLPTR",0,"initializeOnly", 0],
	],"X3DTextureNode"),

	"PixelTexture3D" => new VRML::NodeType("PixelTexture3D", [
		image => ["MFInt32", "0, 0, 0, 0", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
                metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		repeatS => ["SFBool", "FALSE", "initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		repeatT => ["SFBool", "FALSE", "initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		repeatR => ["SFBool", "FALSE", "initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		textureProperties => ["SFNode", 0, "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		__textureTableIndex => ["SFInt32", 0, "initializeOnly", 0],
		_parentResource =>["FreeWRLPTR",0,"initializeOnly", 0],
	],"X3DTextureNode"),

	"TextureCoordinate3D" => new VRML::NodeType("TextureCoordinate3D", [
                metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		point => ["MFVec3f", [], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
	],"X3DTextureCoordinateNode"),

	"TextureCoordinate4D" => new VRML::NodeType("TextureCoordinate4D", [
                metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		point => ["MFVec4f", [], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
	],"X3DTextureCoordinateNode"),

	"TextureTransformMatrix3D" => new VRML::NodeType("TextureTransformMatrix3D", [
                metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		matrix => ["SFMatrix4f", [1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
	],"X3DTextureTransformNode"),

	"TextureTransform3D" => new VRML::NodeType ("TextureTransform3D", [
		center => ["SFVec3f", [0, 0, 0], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
                metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		rotation => ["SFRotation", [0, 0, 1, 0], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		scale => ["SFVec3f", [1, 1, 1], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		translation => ["SFVec3f", [0, 0, 0], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
	],"X3DTextureTransformNode"),

	"ComposedTexture3D" => new VRML::NodeType("ComposedTexture3D", [
                metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		texture=>["MFNode",undef,"inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		textureProperties => ["SFNode", 0, "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		repeatS => ["SFBool", "FALSE", "initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		repeatT => ["SFBool", "FALSE", "initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		repeatR => ["SFBool", "FALSE", "initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		__textureTableIndex => ["SFInt32", 0, "initializeOnly", 0],
		_parentResource =>["FreeWRLPTR",0,"initializeOnly", 0],
	],"X3DTexture3DNode"),


	###################################################################################

	#	35.	Layering Component

	###################################################################################

	"Viewport" => new VRML::NodeType("Viewport", [
		addChildren => ["MFNode", undef, "inputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		removeChildren => ["MFNode", undef, "inputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		__sibAffectors => ["MFNode", [], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		children => ["MFNode", [], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		clipBoundary => ["MFFloat",[],"inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		bboxCenter => ["SFVec3f", [0,0,0], "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		bboxSize => ["SFVec3f", [-1,-1,-1], "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		], "X3DViewportNode"),

	"Layer" => new VRML::NodeType("Layer", [
		addChildren => ["MFNode", undef, "inputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		removeChildren => ["MFNode", undef, "inputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		__sibAffectors => ["MFNode", [], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		children => ["MFNode", [], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		isPickable => ["SFBool", "TRUE", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		viewport => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		], "X3DLayerNode"),

	"LayerSet" => new VRML::NodeType("LayerSet", [
		activeLayer => ["SFInt32", 0, "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		layers => ["MFNode", [], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		order => ["MFInt32",[0],"inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		], "X3DLayerSetNode"),

	###################################################################################

	#	36.	Layout Component

	###################################################################################
	"Layout" => new VRML::NodeType("Layout", [
		align => ["MFString", ["CENTER","CENTER"], "inputOutput", "(SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		offset => ["MFFloat",[0,0],"inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		offsetUnits => ["MFString", ["WORLD","WORLD"], "inputOutput", "(SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		scaleMode => ["MFString", ["NONE","NONE"], "inputOutput", "(SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		size => ["MFFloat",[1,1],"inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		sizeUnits => ["MFString", ["WORLD","WORLD"], "inputOutput", "(SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		_align => ["MFInt32",[0,0],"initializeOnly", 0],
		_offsetUnits => ["MFInt32",[0,0], "initializeOnly", 0],
		_scaleMode => ["MFInt32",[0,0], "initializeOnly", 0],
		_sizeUnits => ["MFInt32",[0,0], "initializeOnly", 0],
		_scale => ["MFFloat",[1,1], "initializeOnly", 0],
		], "X3DLayoutNode"),

	"LayoutGroup" => new VRML::NodeType("LayoutGroup", [
		addChildren => ["MFNode", undef, "inputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		removeChildren => ["MFNode", undef, "inputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		__sibAffectors => ["MFNode", [], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		children => ["MFNode", [], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		bboxCenter => ["SFVec3f", [0,0,0], "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		bboxSize => ["SFVec3f", [0,0,0], "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		layout => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		viewport => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		], "X3DGroupingNode"),



	"LayoutLayer" => new VRML::NodeType("LayoutLayer", [
		addChildren => ["MFNode", undef, "inputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		removeChildren => ["MFNode", undef, "inputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		__sibAffectors => ["MFNode", [], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		children => ["MFNode", [], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		isPickable => ["SFBool", "TRUE", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		viewport => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		layout => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		], "X3DGroupingNode"),

	"ScreenFontStyle" => new VRML::NodeType("ScreenFontStyle", [
                metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		family => ["MFString", ["SERIF"], "initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		horizontal => ["SFBool", "TRUE", "initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		justify => ["MFString", ["BEGIN"], "initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		language => ["SFString", "", "initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		leftToRight => ["SFBool", "TRUE", "initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		pointSize => ["SFFloat", 12.0, "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"], # see note top of file
		spacing => ["SFFloat", 1.0, "initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		style => ["SFString", "PLAIN", "initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		topToBottom => ["SFBool", "TRUE", "initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
	],"X3DFontStyleNode"),


	"ScreenGroup" => new VRML::NodeType("ScreenGroup", [
		addChildren => ["MFNode", undef, "inputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		removeChildren => ["MFNode", undef, "inputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		__sibAffectors => ["MFNode", [], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		children => ["MFNode", [], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		bboxCenter => ["SFVec3f", [0,0,0], "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		bboxSize => ["SFVec3f", [-1,-1,-1], "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		], "X3DGroupingNode"),

	###################################################################################

	#	37.	Rigid Body Physics Component

	###################################################################################

	"BallJoint" => new VRML::NodeType("BallJoint", [
		anchorPoint => ["SFVec3f", [0,0,0], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		body1 => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		body2 => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		forceOutput => ["MFString", ["NONE"], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		body1AnchorPoint => ["SFVec3f",[0,0,0],"outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		body2AnchorPoint => ["SFVec3f",[0,0,0],"outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		], "X3DRigidJointNode"),

	"CollidableOffset" => new VRML::NodeType("CollidableOffset", [
		enabled => ["SFBool", "TRUE", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		rotation => ["SFRotation", [0, 0, 1, 0], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		translation => ["SFVec3f", [0, 0, 0], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		bboxCenter => ["SFVec3f", [0,0,0], "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		bboxSize => ["SFVec3f", [-1,-1,-1], "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		__do_trans => ["SFInt32", "FALSE", "initializeOnly", 0],
		__do_rotation => ["SFInt32", "FALSE", "initializeOnly", 0],
		collidable => ["SFNode", "NULL", "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		], "X3DNBodyCollidableNode"),

	"CollidableShape" => new VRML::NodeType("CollidableShape", [
		enabled => ["SFBool", "TRUE", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		rotation => ["SFRotation", [0, 0, 1, 0], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		translation => ["SFVec3f", [0, 0, 0], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		bboxCenter => ["SFVec3f", [0,0,0], "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		bboxSize => ["SFVec3f", [-1,-1,-1], "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		__do_trans => ["SFInt32", "FALSE", "initializeOnly", 0],
		__do_rotation => ["SFInt32", "FALSE", "initializeOnly", 0],
		shape => ["SFNode", "NULL", "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		_geom => ["FreeWRLPTR", 0, "initializeOnly", 0],
		], "X3DNBodyCollidableNode"),

	"CollisionCollection" => new VRML::NodeType("CollisionCollection", [
		appliedParameters => ["MFString", ["BOUNCE"], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		bounce => ["SFFloat", 0.0, "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"], # see note top of file
		collidables => ["MFNode", [], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		enabled => ["SFBool", "TRUE", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		frictionCoefficients => ["SFVec2f", [0, 0], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
                metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		minBounceSpeed => ["SFFloat", 0.1, "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"], # see note top of file
		slipFactors => ["SFVec2f", [0, 0], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		softnessConstantForceMix => ["SFFloat", 0.0001, "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"], # see note top of file
		softnessErrorCorrection => ["SFFloat", 0.8, "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"], # see note top of file
		surfaceSpeed => ["SFVec2f", [0, 0], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		_class => ["FreeWRLPTR", 0, "initializeOnly", 0],
		],"X3DChildNode"),

	"CollisionSensor" => new VRML::NodeType("CollsionSensor", [
		collider => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		enabled => ["SFBool", "TRUE", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		intersections => ["MFNode", [], "outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		contacts => ["MFNode", [], "outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		isActive => ["SFBool", "TRUE", "outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		], "X3DSensorNode"),

	"CollisionSpace" => new VRML::NodeType("CollisionSpace", [
		collidables => ["MFNode", [], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		enabled => ["SFBool", "TRUE", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		useGeometry => ["SFBool", "FALSE", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		bboxCenter => ["SFVec3f", [0,0,0], "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		bboxSize => ["SFVec3f", [-1,-1,-1], "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		_space => ["FreeWRLPTR", 0, "initializeOnly", 0],
		], "X3DNBodyCollidableNode"),

	"Contact" => new VRML::NodeType("Contact", [
		appliedParameters => ["MFString", ["BOUNCE"], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		body1 => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		body2 => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		bounce => ["SFFloat", 0.0, "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		contactNormal => ["SFVec3f", [0,1,0], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		depth => ["SFFloat", 0.0, "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		frictionCoefficients => ["SFVec2f", [0, 0], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		frictionDirection => ["SFVec3f", [0,1,0], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		geometry1 => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		geometry2 => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		minBounceSpeed => ["SFFloat", 0.0, "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		position => ["SFVec3f", [0, 0, 0], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		slipCoefficients => ["SFVec2f", [0, 0], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		softnessConstantForceMix => ["SFFloat", 0.0001, "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		softnessErrorCorrection => ["SFFloat", 0.8, "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		surfaceSpeed => ["SFVec2f", [0, 0], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		], "X3DSFNode"),

	"DoubleAxisHingeJoint" => new VRML::NodeType("DoubleAxisHingeJoint", [
		anchorPoint => ["SFVec3f", [0,0,0], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		axis1 => ["SFVec3f", [0,0,0], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		axis2 => ["SFVec3f", [0,0,0], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		body1 => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		body2 => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		desiredAngularVelocity1 => ["SFFloat", 0.0, "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		desiredAngularVelocity2 => ["SFFloat", 0.0, "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		forceOutput => ["MFString", ["NONE"], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		maxAngle1 => ["SFFloat", "PIF+", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		maxTorque1 => ["SFFloat", 0, "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		maxTorque2 => ["SFFloat", 0, "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		minAngle1 => ["SFFloat", "-PIF+", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		stopBounce1 => ["SFFloat", 0, "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		stopConstantForceMix1 => ["SFFloat", .001, "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		stopErrorCorrection1 => ["SFFloat", .8, "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		suspensionErrorCorrection => ["SFFloat", .8, "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		suspensionForce => ["SFFloat", 0, "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		body1AnchorPoint => ["SFVec3f",[0,0,0],"outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		body1Axis => ["SFVec3f",[0,0,0],"outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		body2AnchorPoint => ["SFVec3f",[0,0,0],"outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		body2Axis => ["SFVec3f",[0,0,0],"outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		hinge1Angle=>["SFFloat",0.0,"outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		hinge1AngleRate=>["SFFloat",0.0,"outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		hinge2Angle=>["SFFloat",0.0,"outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		hinge2AngleRate=>["SFFloat",0.0,"outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		], "X3DRigidJointNode"),

	"MotorJoint" => new VRML::NodeType("MotorJoint", [
		axis1Angle => ["SFFloat", 0, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		axis1Torque => ["SFFloat", 0, "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		axis2Angle => ["SFFloat", 0, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		axis2Torque => ["SFFloat", 0, "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		axis3Angle => ["SFFloat", 0, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		axis3Torque => ["SFFloat", 0, "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		body1 => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		body2 => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		enabledAxes => ["SFInt32", 1, "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		forceOutput => ["MFString", ["NONE"], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		motor1Axis => ["SFVec3f",[0,0,0],"outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		motor2Axis => ["SFVec3f",[0,0,0],"outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		motor3Axis => ["SFVec3f",[0,0,0],"outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		stop1Bounce => ["SFFloat", 0, "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		stop1ErrorCorrection => ["SFFloat", .8, "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		stop2Bounce => ["SFFloat", 0, "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		stop2ErrorCorrection => ["SFFloat", .8, "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		stop3Bounce => ["SFFloat", 0, "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		stop3ErrorCorrection => ["SFFloat", .8, "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		
		motor1Angle=>["SFFloat",0.0,"outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		motor1AngleRate=>["SFFloat",0.0,"outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		motor2Angle=>["SFFloat",0.0,"outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		motor2AngleRate=>["SFFloat",0.0,"outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		motor3Angle=>["SFFloat",0.0,"outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		motor3AngleRate=>["SFFloat",0.0,"outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		autoCalc => ["SFBool", "FALSE", "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		], "X3DRigidJointNode"),

	"RigidBody" => new VRML::NodeType("RigidBody", [
		angularDampingFactor => ["SFFloat", 0.001, "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		angularVelocity => ["SFVec3f", [0,0,0], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		autoDamp => ["SFBool", "FALSE", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		autoDisable => ["SFBool", "FALSE", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		centerOfMass => ["SFVec3f", [0,0,0], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		disableAngularSpeed => ["SFFloat", 0.0, "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		disableLinearSpeed => ["SFFloat", 0.0, "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		disableTime => ["SFFloat", 0.0, "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		enabled => ["SFBool", "TRUE", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		finiteRotationAxis => ["SFVec3f", [0,0,0], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		fixed => ["SFBool", "FALSE", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		forces => ["MFVec3f",[],"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		geometry => ["MFNode",[],"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		inertia => ["SFMatrix3f",[1,0,0,0,1,0,0,0,1],"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		linearDampingFactor => ["SFFloat", 0.001, "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		linearVelocity => ["SFVec3f", [0,0,0], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		mass => ["SFFloat", 1, "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		massDensityModel => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		orientation => ["SFRotation", [0, 0, 1, 0], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		position => ["SFVec3f", [0, 0, 0], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		torques => ["MFVec3f",[],"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		useFiniteRotation => ["SFBool", "FALSE", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		useGlobalGravity => ["SFBool", "TRUE", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		_body => ["FreeWRLPTR", 0, "initializeOnly", 0],
		], "X3DSFNode"),

	"RigidBodyCollection" => new VRML::NodeType("RigidBodyCollection", [
		set_contacts =>["MFNode",[],"inputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		autoDisable => ["SFBool", "FALSE", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		bodies => ["MFNode",[],"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		constantForceMix => ["SFFloat", .0001, "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		contactSurfaceThickness => ["SFFloat", 0, "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		disableAngularSpeed => ["SFFloat", 0, "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		disableLinearSpeed => ["SFFloat", 0, "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		disableTime => ["SFFloat", 0, "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		enabled => ["SFBool", "TRUE", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		errorCorrection => ["SFFloat", 0.8, "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		gravity => ["SFVec3f", [0, -9.8, 0], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		iterations => ["SFInt32",10,"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		joints => ["MFNode",[],"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		maxCorrectionSpeed => ["SFFloat", -1.8, "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		preferAccuracy => ["SFBool", "FALSE", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		collider => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		_world => ["FreeWRLPTR", 0, "initializeOnly", 0],
		_space => ["FreeWRLPTR", 0, "initializeOnly", 0],
		], "X3DChildNode"),

	"SingleAxisHingeJoint" => new VRML::NodeType("SingleAxisHingeJoint", [
		anchorPoint => ["SFVec3f", [0,0,0], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		axis => ["SFVec3f", [0,0,0], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		body1 => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		body2 => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		forceOutput => ["MFString", ["NONE"], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		maxAngle => ["SFFloat", "PIF+", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		minAngle => ["SFFloat", "-PIF+", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		stopBounce => ["SFFloat", 0, "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		stopErrorCorrection => ["SFFloat", .8, "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		angle=>["SFFloat",0.0,"outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		angleRate=>["SFFloat",0.0,"outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		body1AnchorPoint => ["SFVec3f",[0,0,0],"outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		body2AnchorPoint => ["SFVec3f",[0,0,0],"outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		], "X3DRigidJointNode"),

	"SliderJoint" => new VRML::NodeType("SliderJoint", [
		axis => ["SFVec3f", [0,0,0], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		body1 => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		body2 => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		forceOutput => ["MFString", ["NONE"], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		maxSeparation => ["SFFloat", 1.0, "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		minSeparation => ["SFFloat", 0, "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		sliderForce => ["SFFloat", 0, "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		stopBounce => ["SFFloat",  0, "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		stopErrorCorrection => ["SFFloat",  1, "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		separation => ["SFFloat",0.0,"outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		separationRate=>["SFFloat",0.0,"outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		], "X3DRigidJointNode"),
	
	"UniversalJoint" => new VRML::NodeType("UniversalJoint", [
		anchorPoint => ["SFVec3f", [0,0,0], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		axis1 => ["SFVec3f", [0,0,0], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		axis2 => ["SFVec3f", [0,0,0], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		body1 => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		body2 => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		forceOutput => ["MFString", ["NONE"], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		stop1Bounce => ["SFFloat", 0, "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		stop1ErrorCorrection => ["SFFloat", .8, "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		stop2Bounce => ["SFFloat", 0, "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		stop2ErrorCorrection => ["SFFloat", .8, "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		body1AnchorPoint => ["SFVec3f",[0,0,0],"outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		body1Axis => ["SFVec3f",[0,0,0],"outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		body2AnchorPoint => ["SFVec3f",[0,0,0],"outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		body2Axis => ["SFVec3f",[0,0,0],"outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		], "X3DRigidJointNode"),
	
	
	###################################################################################

	#	38.	Picking Component

	###################################################################################
	
	# A PickableGroup node is an X3DGroupingNode that contains children that are marked
	# as being of a given classification of picking types, as well as the ability to enable or disable picking of the children.

# DJTRACK_PICKSENSORS
	"PickableGroup" => new VRML::NodeType("PickableGroup", [
		addChildren => ["MFNode", undef, "inputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		removeChildren => ["MFNode", undef, "inputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		__sibAffectors => ["MFNode", [], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		children => ["MFNode", [], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		objectType => ["MFString", ["ALL","NONE","TERRAIN"],"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		pickable => ["SFBool", "TRUE","inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		bboxCenter => ["SFVec3f", [0, 0, 0], "initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		bboxSize => ["SFVec3f", [-1, -1, -1], "initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],

		#FreeWRL__protoDef => ["SFInt32", "INT_ID_UNDEFINED", "initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"], # tell renderer that this is a proto...
		#FreeWRL_PROTOInterfaceNodes =>["MFNode", [], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
	],"X3DGroupingNode"),

	# The PointPickSensor node tests one or more points in space as lying inside the provided target geometry.
	# For each point that lies inside the geometry, the point coordinate is returned in the pickedGeometry field
	# with the corresponding geometry inside which the point lies.
	# Because points represent an infinitely small location in space, the "CLOSEST" and "ALL_SORTED" sort orders
	# are defined to mean "ANY" and "ALL" respectively.

	"PointPickSensor" => new VRML::NodeType("PointPickSensor", [
		enabled => ["SFBool", "FALSE","inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		objectType => ["MFString", ["ALL","NONE","TERRAIN"],"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		pickingGeometry => ["SFNode", "NULL", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		pickTarget => ["MFNode", [], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		isActive  => ["SFBool", "FALSE","outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		pickedGeometry => ["MFNode", [], "outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		intersectionType => ["SFString","BOUNDS","initializeOnly", "(SPEC_VRML | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		sortOrder => ["SFString","CLOSEST","initializeOnly", "(SPEC_VRML | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		# These fields are used for the info.
		__oldEnabled => ["SFBool", "TRUE", "inputOutput", 0],
		pickedPoint => ["MFVec3f", [], "outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		
		#DJTRACK
		_oldisActive  => ["SFBool", "FALSE","outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		_oldpickTarget => ["MFNode", [], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		_oldpickedGeometry => ["MFNode", [], "outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		_oldpickedPoint => ["MFVec3f", [], "outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		_bboxCenter => ["SFVec3f", [0, 0, 0], "initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		_bboxSize => ["SFVec3f", [-1, -1, -1], "initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		set_intersectionType => ["SFString", undef, "inputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		set_sortOrder => ["SFString", undef, "inputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		
	],"X3DSensorNode"),

	"LinePickSensor" => new VRML::NodeType("LinePickSensor", [
		enabled => ["SFBool", "FALSE","inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		objectType => ["MFString", ["ALL","NONE","TERRAIN"],"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		pickingGeometry => ["SFNode", "NULL", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		pickTarget => ["MFNode", [], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		isActive  => ["SFBool", "FALSE","outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		pickedGeometry => ["MFNode", [], "outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		intersectionType => ["SFString","BOUNDS","initializeOnly", "(SPEC_VRML | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		sortOrder => ["SFString","CLOSEST","initializeOnly", "(SPEC_VRML | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		# These fields are used for the info.
		__oldEnabled => ["SFBool", "TRUE", "inputOutput", 0],
		pickedPoint => ["MFVec3f", [], "outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		pickedNormal => ["MFVec3f", [], "outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		pickedTextureCoordinate => ["MFVec3f", [], "outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
	],"X3DSensorNode"),
	
	#38.4.4 PrimitivePickSensor
	"PrimitivePickSensor" => new VRML::NodeType("PrimitivePickSensor", [
		enabled => ["SFBool", "FALSE","inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		objectType => ["MFString", ["ALL","NONE","TERRAIN"],"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		pickingGeometry => ["SFNode", "NULL", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		pickTarget => ["MFNode", [], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		isActive  => ["SFBool", "FALSE","outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		pickedGeometry => ["MFNode", [], "outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		intersectionType => ["SFString","BOUNDS","initializeOnly", "(SPEC_VRML | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		sortOrder => ["SFString","CLOSEST","initializeOnly", "(SPEC_VRML | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		# These fields are used for the info.
		__oldEnabled => ["SFBool", "TRUE", "inputOutput", 0],
	],"X3DSensorNode"),
	
	
	#38.4.5 VolumePickSensor
	"VolumePickSensor" => new VRML::NodeType("VolumePickSensor", [
		enabled => ["SFBool", "FALSE","inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		objectType => ["MFString", ["ALL","NONE","TERRAIN"],"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		pickingGeometry => ["SFNode", "NULL", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		pickTarget => ["MFNode", [], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		isActive  => ["SFBool", "FALSE","outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		pickedGeometry => ["MFNode", [], "outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		intersectionType => ["SFString","BOUNDS","initializeOnly", "(SPEC_VRML | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		sortOrder => ["SFString","CLOSEST","initializeOnly", "(SPEC_VRML | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		# These fields are used for the info.
		__oldEnabled => ["SFBool", "TRUE", "inputOutput", 0],
	],"X3DSensorNode"),
	
	###################################################################################

	#	39.	Followers Component

	###################################################################################
	
	# value_changed is the first field-type-sepcific field so that offsetof(,value_changed) will be generic for all chasers, and for all dampers
	"ColorChaser" => new VRML::NodeType("ColorChaser", [
		metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		_p => ["FreeWRLPTR", "NULL", "initializeOnly", 0],
		_t => ["FreeWRLPTR", "NULL", "initializeOnly", 0],
		isActive  => ["SFBool", "FALSE","outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		duration  => ["SFTime", 1,"initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		_bufferendtime  => ["SFTime", 0,"initializeOnly", 0],
		_steptime  => ["SFTime", 0,"initializeOnly", 0],
		value_changed => ["SFColor", [0,0,0], "outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		initialDestination => ["SFColor", [.8,.8,.8], "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		initialValue => ["SFColor", [.8,.8,.8], "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		set_destination => ["SFColor", [0,0,0], "inputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		set_value => ["SFColor", [0,0,0], "inputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		_buffer => ["FreeWRLPTR", "NULL", "initializeOnly", 0],
		_previousvalue => ["SFColor", [0,0,0], "initializeOnly", 0],
		_destination => ["SFColor", [0,0,0], "initializeOnly", 0],
	],"X3DChaserNode"),
	
	"ColorDamper" => new VRML::NodeType("ColorDamper", [
		metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		_p => ["FreeWRLPTR", "NULL", "initializeOnly", 0],
		_t => ["FreeWRLPTR", "NULL", "initializeOnly", 0],
		tau  => ["SFTime", .3,"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		tolerance => ["SFFloat", -1,"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		isActive  => ["SFBool", "FALSE","outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		order => ["SFInt32", 3, "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		_tau  => ["SFTime", .3,"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		_lasttick  => ["SFTime", 0,"initializeOnly", 0],
		_takefirstinput => ["SFBool", "TRUE", "initializeOnly", 0],
		value_changed => ["SFColor", [0,0,0], "outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		initialDestination => ["SFColor", [.8,.8,.8], "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		initialValue => ["SFColor", [.8,.8,.8], "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		set_destination => ["SFColor", [0,0,0], "inputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		set_value => ["SFColor", [0,0,0], "inputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		_values => ["FreeWRLPTR", "NULL", "initializeOnly", 0],
		_input => ["SFColor", [0,0,0], "initializeOnly", 0],
		
	],"X3DDamperNode"),
	
	"CoordinateChaser" => new VRML::NodeType("CoordinateChaser", [
		metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		_p => ["FreeWRLPTR", "NULL", "initializeOnly", 0],
		_t => ["FreeWRLPTR", "NULL", "initializeOnly", 0],
		isActive  => ["SFBool", "FALSE","outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		duration  => ["SFTime", 1,"initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		_bufferendtime  => ["SFTime", 0,"initializeOnly", 0],
		_steptime  => ["SFTime", 0,"initializeOnly", 0],
		value_changed => ["MFVec3f", [], "outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		initialDestination => ["MFVec3f", [[0,0,0]], "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		initialValue => ["MFVec3f", [[0,0,0]], "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		set_destination => ["MFVec3f", [], "inputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		set_value => ["MFVec3f", [], "inputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		_buffer => ["FreeWRLPTR", "NULL", "initializeOnly", 0],
		_previousvalue => ["MFVec3f", [[0,0,0]], "initializeOnly", 0],
		_destination => ["MFVec3f", [[0,0,0]], "initializeOnly", 0],
	],"X3DChaserNode"),
	
	"CoordinateDamper" => new VRML::NodeType("CoordinateDamper", [
		metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		_p => ["FreeWRLPTR", "NULL", "initializeOnly", 0],
		_t => ["FreeWRLPTR", "NULL", "initializeOnly", 0],
		tau  => ["SFTime", .3,"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		tolerance => ["SFFloat", -1,"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		isActive  => ["SFBool", "FALSE","outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		order => ["SFInt32", 3, "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		_tau  => ["SFTime", .3,"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		_lasttick  => ["SFTime", 0,"initializeOnly", 0],
		_takefirstinput => ["SFBool", "TRUE", "initializeOnly", 0],
		value_changed => ["MFVec3f", [], "outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		initialDestination => ["MFVec3f", [[0,0,0]], "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		initialValue => ["MFVec3f", [[0,0,0]], "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		set_destination => ["MFVec3f", [], "inputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		set_value => ["MFVec3f", [], "inputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		_values => ["FreeWRLPTR", "NULL", "initializeOnly", 0],
		_input => ["MFVec3f", [], "initializeOnly", 0],
	],"X3DDamperNode"),

	"OrientationChaser" => new VRML::NodeType("OrientationChaser", [
		metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		_p => ["FreeWRLPTR", "NULL", "initializeOnly", 0],
		_t => ["FreeWRLPTR", "NULL", "initializeOnly", 0],
		isActive  => ["SFBool", "FALSE","outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		duration  => ["SFTime", 1,"initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		_bufferendtime  => ["SFTime", 0,"initializeOnly", 0],
		_steptime  => ["SFTime", 0,"initializeOnly", 0],
		value_changed => ["SFRotation", [0,1,0,0], "outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		initialDestination => ["SFRotation", [0,1,0,0], "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		initialValue => ["SFRotation", [0,1,0,0], "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		set_destination => ["SFRotation", [0,1,0,0], "inputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		set_value => ["SFRotation", [0,1,0,0], "inputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		_buffer => ["FreeWRLPTR", "NULL", "initializeOnly", 0],
		_previousvalue => ["SFRotation", [0,1, 0,0], "initializeOnly", 0],
		_destination => ["SFRotation", [0,1,0,0], "initializeOnly", 0],
	],"X3DChaserNode"),
	
	"OrientationDamper" => new VRML::NodeType("OrientationDamper", [
		metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		_p => ["FreeWRLPTR", "NULL", "initializeOnly", 0],
		_t => ["FreeWRLPTR", "NULL", "initializeOnly", 0],
		tau  => ["SFTime", .3,"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		tolerance => ["SFFloat", -1,"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		isActive  => ["SFBool", "FALSE","outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		order => ["SFInt32", 3, "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		_tau  => ["SFTime", .3,"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		_lasttick  => ["SFTime", 0,"initializeOnly", 0],
		_takefirstinput => ["SFBool", "TRUE", "initializeOnly", 0],
		value_changed => ["SFRotation", [0,1,0,0], "outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		initialDestination => ["SFRotation", [0,1,0,0], "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		initialValue => ["SFRotation", [0,1,0,0], "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		set_destination => ["SFRotation", [0,1,0,0], "inputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		set_value => ["SFRotation", [0,1,0,0], "inputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		_values => ["FreeWRLPTR", "NULL", "initializeOnly", 0],
		_input => ["SFRotation", [0,1,0,0], "initializeOnly", 0],
	],"X3DDamperNode"),

	"PositionChaser" => new VRML::NodeType("PositionChaser", [
		metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		_p => ["FreeWRLPTR", "NULL", "initializeOnly", 0],
		_t => ["FreeWRLPTR", "NULL", "initializeOnly", 0],
		isActive  => ["SFBool", "FALSE","outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		duration  => ["SFTime", 1,"initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		_bufferendtime  => ["SFTime", 0,"initializeOnly", 0],
		_steptime  => ["SFTime", 0,"initializeOnly", 0],
		value_changed => ["SFVec3f", [0,0,0], "outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		initialDestination => ["SFVec3f", [0,0,0], "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		initialValue => ["SFVec3f", [0,0,0], "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		set_destination => ["SFVec3f", [0,0,0], "inputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		set_value => ["SFVec3f", [0,0,0], "inputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		_buffer => ["FreeWRLPTR", "NULL", "initializeOnly", 0],
		_previousvalue => ["SFVec3f", [0,0,0], "initializeOnly", 0],
		_destination => ["SFVec3f", [0,0,0], "initializeOnly", 0],
	],"X3DChaserNode"),
	
	"PositionDamper" => new VRML::NodeType("PositionDamper", [
		metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		_p => ["FreeWRLPTR", "NULL", "initializeOnly", 0],
		_t => ["FreeWRLPTR", "NULL", "initializeOnly", 0],
		tau  => ["SFTime", .3,"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		tolerance => ["SFFloat", -1,"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		isActive  => ["SFBool", "FALSE","outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		order => ["SFInt32", 3, "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		_tau  => ["SFTime", .3,"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		_lasttick  => ["SFTime", 0,"initializeOnly", 0],
		_takefirstinput => ["SFBool", "TRUE", "initializeOnly", 0],
		value_changed => ["SFVec3f", [0,0,0], "outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		initialDestination => ["SFVec3f", [0,0,0], "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		initialValue => ["SFVec3f", [0,0,0], "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		set_destination => ["SFVec3f", [0,0,0], "inputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		set_value => ["SFVec3f", [0,0,0], "inputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		_values => ["FreeWRLPTR", "NULL", "initializeOnly", 0],
		_input => ["SFVec3f", [0,0,0], "initializeOnly", 0],
	],"X3DDamperNode"),

	"PositionChaser2D" => new VRML::NodeType("PositionChaser2D", [
		metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		_p => ["FreeWRLPTR", "NULL", "initializeOnly", 0],
		_t => ["FreeWRLPTR", "NULL", "initializeOnly", 0],
		isActive  => ["SFBool", "FALSE","outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		duration  => ["SFTime", 1,"initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		_bufferendtime  => ["SFTime", 0,"initializeOnly", 0],
		_steptime  => ["SFTime", 0,"initializeOnly", 0],
		value_changed => ["SFVec2f", [0,0], "outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		initialDestination => ["SFVec2f", [0,0], "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		initialValue => ["SFVec2f", [0,0], "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		set_destination => ["SFVec2f", [0,0], "inputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		set_value => ["SFVec2f", [0,0], "inputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		_buffer => ["FreeWRLPTR", "NULL", "initializeOnly", 0],
		_previousvalue => ["SFVec2f", [0,0], "initializeOnly", 0],
		_destination => ["SFVec2f", [0,0], "initializeOnly", 0],
	],"X3DChaserNode"),
	
	"PositionDamper2D" => new VRML::NodeType("PositionDamper2D", [
		metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		_p => ["FreeWRLPTR", "NULL", "initializeOnly", 0],
		_t => ["FreeWRLPTR", "NULL", "initializeOnly", 0],
		tau  => ["SFTime", .3,"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		tolerance => ["SFFloat", -1,"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		isActive  => ["SFBool", "FALSE","outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		order => ["SFInt32", 3, "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		_tau  => ["SFTime", .3,"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		_lasttick  => ["SFTime", 0,"initializeOnly", 0],
		_takefirstinput => ["SFBool", "TRUE", "initializeOnly", 0],
		value_changed => ["SFVec2f", [0,0], "outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		initialDestination => ["SFVec2f", [0,0], "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		initialValue => ["SFVec2f", [0,0], "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		set_destination => ["SFVec2f", [0,0], "inputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		set_value => ["SFVec2f", [0,0], "inputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		_values => ["FreeWRLPTR", "NULL", "initializeOnly", 0],
		_input => ["SFVec2f", [0,0], "initializeOnly", 0],
	],"X3DDamperNode"),

	"ScalarChaser" => new VRML::NodeType("ScalarChaser", [
		metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		_p => ["FreeWRLPTR", "NULL", "initializeOnly", 0],
		_t => ["FreeWRLPTR", "NULL", "initializeOnly", 0],
		isActive  => ["SFBool", "FALSE","outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		duration  => ["SFTime", 1,"initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		_bufferendtime  => ["SFTime", 0,"initializeOnly", 0],
		_steptime  => ["SFTime", 0,"initializeOnly", 0],
		value_changed => ["SFFloat", 0, "outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		initialDestination => ["SFFloat", 0, "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		initialValue => ["SFFloat", 0, "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		set_destination => ["SFFloat", 0, "inputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		set_value => ["SFFloat", 0, "inputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		_buffer => ["FreeWRLPTR", "NULL", "initializeOnly", 0],
		_previousvalue => ["SFFloat", 0, "initializeOnly", 0],
		_destination => ["SFFloat", 0, "initializeOnly", 0],
	],"X3DChaserNode"),
	
	"ScalarDamper" => new VRML::NodeType("ScalarDamper", [
		metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		_p => ["FreeWRLPTR", "NULL", "initializeOnly", 0],
		_t => ["FreeWRLPTR", "NULL", "initializeOnly", 0],
		tau  => ["SFTime", .3,"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		tolerance => ["SFFloat", -1,"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		isActive  => ["SFBool", "FALSE","outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		order => ["SFInt32", 3, "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		_tau  => ["SFTime", .3,"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		_lasttick  => ["SFTime", 0,"initializeOnly", 0],
		_takefirstinput => ["SFBool", "TRUE", "initializeOnly", 0],
		value_changed => ["SFFloat", 0, "outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		initialDestination => ["SFFloat", 0, "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		initialValue => ["SFFloat", 0, "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		set_destination => ["SFFloat", 0, "inputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		set_value => ["SFFloat", 0, "inputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		_values => ["FreeWRLPTR", "NULL", "initializeOnly", 0],
		_input => ["SFFloat", 0, "initializeOnly", 0],
	],"X3DDamperNode"),

	"TexCoordChaser2D" => new VRML::NodeType("TexCoordChaser2D", [
		metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		_p => ["FreeWRLPTR", "NULL", "initializeOnly", 0],
		_t => ["FreeWRLPTR", "NULL", "initializeOnly", 0],
		isActive  => ["SFBool", "FALSE","outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		duration  => ["SFTime", 1,"initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		_bufferendtime  => ["SFTime", 0,"initializeOnly", 0],
		_steptime  => ["SFTime", 0,"initializeOnly", 0],
		value_changed => ["MFVec2f", [], "outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		initialDestination => ["MFVec2f", [[0,0]], "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		initialValue => ["MFVec2f", [[0,0]], "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		set_destination => ["MFVec2f", [], "inputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		set_value => ["MFVec2f", [], "inputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		_buffer => ["FreeWRLPTR", "NULL", "initializeOnly", 0],
		_previousvalue => ["MFVec2f", [[0,0]], "initializeOnly", 0],
		_destination => ["MFVec2f", [[0,0]], "initializeOnly", 0],
	],"X3DChaserNode"),
	
	"TexCoordDamper2D" => new VRML::NodeType("TexCoordDamper2D", [
		metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		_p => ["FreeWRLPTR", "NULL", "initializeOnly", 0],
		_t => ["FreeWRLPTR", "NULL", "initializeOnly", 0],
		tau  => ["SFTime", .3,"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		tolerance => ["SFFloat", -1,"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		isActive  => ["SFBool", "FALSE","outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		order => ["SFInt32", 3, "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		_tau  => ["SFTime", .3,"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		_lasttick  => ["SFTime", 0,"initializeOnly", 0],
		_takefirstinput => ["SFBool", "TRUE", "initializeOnly", 0],
		value_changed => ["MFVec2f", [], "outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		initialDestination => ["MFVec2f", [[0,0]], "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		initialValue => ["MFVec2f", [[0,0]], "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		set_destination => ["MFVec2f", [], "inputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		set_value => ["MFVec2f", [], "inputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		_values => ["FreeWRLPTR", "NULL", "initializeOnly", 0],
		_input => ["MFVec2f", [], "initializeOnly", 0],
	],"X3DDamperNode"),


	###################################################################################

	#	40.	Particle Systems Component

	###################################################################################
	#40.4.1 BoundedPhysicsModel
	"BoundedPhysicsModel" => new VRML::NodeType("BoundedPhysicsModel", [
		enabled  => ["SFBool", "TRUE","inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		geometry => ["SFNode", "NULL", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
	],"X3DParticlePhysicsModelNode"),
	
	# 40.4.2 ConeEmitter
	"ConeEmitter" => new VRML::NodeType("ConeEmitter", [
		angle  => ["SFFloat", "PIF*.25","inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		direction  => ["SFVec3f", [0,1,0],"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		position  => ["SFVec3f", [0,0,0],"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		speed  => ["SFFloat", 0,"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		variation  => ["SFFloat", .25,"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		mass  => ["SFFloat", 0,"initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		surfaceArea  => ["SFFloat", 0,"initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
	],"X3DParticleEmitterNode"),
	
	# 40.4.3 ExplosionEmitter
	"ExplosionEmitter" => new VRML::NodeType("ExplosionEmitter", [
		metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		position  => ["SFVec3f", [0,0,0],"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		speed  => ["SFFloat", 0,"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		variation  => ["SFFloat", .25,"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		mass  => ["SFFloat", 0,"initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		surfaceArea  => ["SFFloat", 0,"initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
	],"X3DParticleEmitterNode"),
	
	# 40.4.4 ForcePhysicsModel
	"BoundedPhysicsModel" => new VRML::NodeType("BoundedPhysicsModel", [
		enabled  => ["SFBool", "TRUE","inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		force => ["SFVec3f", [0,-9.8,0], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
	],"X3DParticlePhysicsModelNode"),
	
	# 40.4.5 ParticleSystem
	"ParticleSystem" => new VRML::NodeType ("ParticleSystem", [
		appearance => ["SFNode", "NULL", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		createParticles  => ["SFBool", "TRUE","inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		geometry => ["SFNode", "NULL", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		enabled  => ["SFBool", "TRUE","inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		lifetimeVariation  => ["SFFloat", .25,"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		maxParticles  => ["SFInt32", 200,"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
                metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		particleLifetime  => ["SFFloat", 5,"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		particleSize => ["SFVec2f", [.02,.02], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		isActive  => ["SFBool", "TRUE","outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		bboxCenter => ["SFVec3f", [0, 0, 0], "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		bboxSize => ["SFVec3f", [-1, -1, -1], "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		colorRamp => ["SFNode", "NULL", "initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		colorKey => ["MFFloat", [], "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		emitter => ["SFNode", "NULL", "initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		geometryType => ["SFString", "QUAD", "initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		physics => ["MFNode", [], "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		texCoordRamp => ["SFNode", "NULL", "initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		texCoordKey => ["MFFloat", [], "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
	],"X3DShapeNode"),
	
	# 40.4.6 PointEmitter
	"PointEmitter" => new VRML::NodeType("PointEmitter", [
		direction  => ["SFVec3f", [0,1,0],"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		position  => ["SFVec3f", [0,0,0],"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		speed  => ["SFFloat", 0,"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		variation  => ["SFFloat", .25,"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		mass  => ["SFFloat", 0,"initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		surfaceArea  => ["SFFloat", 0,"initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
	],"X3DParticleEmitterNode"),
	
	# 40.4.7 PolylineEmitter
	"PolylineEmitter" => new VRML::NodeType("PolylineEmitter", [
		set_coordIndex => ["MFInt32", [], "inputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		coord => ["SFNode", "NULL", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		direction  => ["SFVec3f", [0,1,0],"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		speed  => ["SFFloat", 0,"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		variation  => ["SFFloat", .25,"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		coordIndex => ["MFInt32", [-1], "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		mass  => ["SFFloat", 0,"initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		surfaceArea  => ["SFFloat", 0,"initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
	],"X3DParticleEmitterNode"),
	
	# 40.4.8 SurfaceEmitter
	"SurfaceEmitter" => new VRML::NodeType("SurfaceEmitter", [
		set_coordIndex => ["MFInt32", [], "inputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		speed  => ["SFFloat", 0,"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		variation  => ["SFFloat", .25,"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		coordIndex => ["MFInt32", [-1], "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		mass  => ["SFFloat", 0,"initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		surface => ["SFNode", "NULL", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		surfaceArea  => ["SFFloat", 0,"initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
	],"X3DParticleEmitterNode"),
	
	# 40.4.9 VolumeEmitter
	"PolylineEmitter" => new VRML::NodeType("PolylineEmitter", [
		set_coordIndex => ["MFInt32", [], "inputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		coord => ["SFNode", "NULL", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		direction  => ["SFVec3f", [0,1,0],"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		speed  => ["SFFloat", 0,"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		variation  => ["SFFloat", .25,"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		coordIndex => ["MFInt32", [-1], "initializeOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		internal  => ["SFBool", "TRUE","inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		mass  => ["SFFloat", 0,"initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		surfaceArea  => ["SFFloat", 0,"initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
	],"X3DParticleEmitterNode"),
	
	# 40.4.10 WindPhysicsModel
	"WindPhysicsModel" => new VRML::NodeType("WindPhysicsModel", [
		direction  => ["SFVec3f", [0,1,0],"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		enabled  => ["SFBool", "TRUE","inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		gustiness => ["SFFloat", .1,"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		speed  => ["SFFloat", .1,"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		turbulence  => ["SFFloat", 0,"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
	],"X3DParticlePhysicsModelNode"),
	

	###################################################################################

	#	41.	Volume Rendering Component

	###################################################################################
	# LEVEL 1
	
	"OpacityMapVolumeStyle" => new VRML::NodeType("OpacityMapVolumeStyle", [
		enabled  => ["SFBool", "TRUE","inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		transferFunction => ["SFNode", "NULL", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
	],"X3DComposableVolumeRenderStyleNode"),
	
	"VolumeData" => new VRML::NodeType("VolumeData", [
		dimensions  => ["SFVec3f", [1,1,1],"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		renderStyle => ["SFNode", "NULL", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		voxels => ["SFNode", "NULL", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		bboxCenter => ["SFVec3f", [0, 0, 0], "initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		bboxSize => ["SFVec3f", [-1, -1, -1], "initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		_boxtris => ["FreeWRLPTR", "NULL", "initializeOnly", 0],
	],"X3DVolumeDataNode"),


	#level 2
	# BoundaryEnhancementVolumeStyle	All fields fully supported.
	"BoundaryEnhancementVolumeStyle" => new VRML::NodeType("BoundaryEnhancementVolumeStyle", [
		enabled  => ["SFBool", "TRUE","inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		boundaryOpacity => ["SFFloat", .9,"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		opacityFactor => ["SFFloat", 2.0,"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		retainedOpacity => ["SFFloat", .2,"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
	],"X3DComposableVolumeRenderStyleNode"),
	
	# ComposedVolumeStyle	ordered field is always treated as FALSE. All other fields fully supported.
	"ComposedVolumeStyle" => new VRML::NodeType("ComposedVolumeStyle", [
		enabled  => ["SFBool", "TRUE","inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		renderStyle => ["MFNode", [], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
	],"X3DComposableVolumeRenderStyleNode"),
	
	# EdgeEnhancementVolumeStyle	All fields fully supported.
	"EdgeEnhancementVolumeStyle" => new VRML::NodeType("EdgeEnhancementVolumeStyle", [
		enabled  => ["SFBool", "TRUE","inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		surfaceNormals => ["SFNode", "NULL", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		edgeColor=>["SFColorRGBA",[0,0,0,1],"inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		gradientThreshold => ["SFFloat", .4,"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
	],"X3DComposableVolumeRenderStyleNode"),
	
	# IsoSurfaceVolumeData	All fields fully supported.
	"IsoSurfaceVolumeData" => new VRML::NodeType("IsoSurfaceVolumeData", [
		dimensions  => ["SFVec3f", [1,1,1],"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		renderStyle => ["SFNode", "NULL", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		voxels => ["SFNode", "NULL", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		bboxCenter => ["SFVec3f", [0, 0, 0], "initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		bboxSize => ["SFVec3f", [-1, -1, -1], "initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		_boxtris => ["FreeWRLPTR", "NULL", "initializeOnly", 0],
		contourStepSize => ["SFFloat", 0,"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		gradients => ["SFNode", "NULL", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		surfaceTolerance => ["SFFloat", 0,"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		surfaceValues => ["MFFloat",[],"inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"], # see note top of file
	],"X3DVolumeDataNode"),
	
	# see level1: OpacityMapVolumeStyle	All fields fully supported. 3D transfer functions shall be supported.
	# ProjectionVolumeStyle	All fields fully supported
	"ProjectionVolumeStyle" => new VRML::NodeType("ProjectionVolumeStyle", [
		enabled  => ["SFBool", "TRUE","inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		intensityThreshold => ["SFFloat", 0,"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		type => ["SFString", "MAX", "initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
	],"X3DComposableVolumeRenderStyleNode"),
	
	# SegmentedVolumeData	All fields fully supported.
	"SegmentedVolumeData" => new VRML::NodeType("SegmentedVolumeData", [
		dimensions  => ["SFVec3f", [1,1,1],"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		renderStyle => ["SFNode", "NULL", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		voxels => ["SFNode", "NULL", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		bboxCenter => ["SFVec3f", [0, 0, 0], "initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		bboxSize => ["SFVec3f", [-1, -1, -1], "initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		_boxtris => ["FreeWRLPTR", "NULL", "initializeOnly", 0],
		segmentEnabled => ["MFBool",[],"inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],  # see note top of file
		segmentIdentifiers => ["SFNode", "NULL", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
	],"X3DVolumeDataNode"),
	
	# SilhouetteEnhancementVolumeStyle	All fields fully supported.
	"SilhouetteEnhancementVolumeStyle" => new VRML::NodeType("SilhouetteEnhancementVolumeStyle", [
		enabled  => ["SFBool", "TRUE","inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		surfaceNormals => ["SFNode", "NULL", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		silhouetteBoundaryOpacity => ["SFFloat", 0,"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		silhouetteRetainedOpacity => ["SFFloat", 1,"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		silhouetteSharpness => ["SFFloat", .5,"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
	],"X3DComposableVolumeRenderStyleNode"),
	
	
	# ToneMappedVolumeStyle	All fields fully supported.
	"ToneMappedVolumeStyle" => new VRML::NodeType("ToneMappedVolumeStyle", [
		enabled  => ["SFBool", "TRUE","inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		surfaceNormals => ["SFNode", "NULL", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		coolColor=>["SFColorRGBA",[0,0,1,0],"inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		warmColor=>["SFColorRGBA",[1,1,0,1],"inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
	],"X3DComposableVolumeRenderStyleNode"),
	
	#level 3
	# BlendedVolumeStyle	All fields fully supported.
	"BlendedVolumeStyle" => new VRML::NodeType("BlendedVolumeStyle", [
		enabled  => ["SFBool", "TRUE","inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		surfaceNormals => ["SFNode", "NULL", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		renderStyle => ["SFNode", "NULL", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		voxels => ["SFNode", "NULL", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		weightConstants1 => ["SFFloat", 0.5,"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		weightConstants2 => ["SFFloat", 0.5,"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		weightFunctions1 => ["SFString", "CONSTANT", "initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		weightFunctions2 => ["SFString", "CONSTANT", "initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		weightTransferFunctions1 => ["SFNode", "NULL", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		weightTransferFunctions2 => ["SFNode", "NULL", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
	],"X3DComposableVolumeRenderStyleNode"),
	
	# CartoonVolumeStyle	All fields fully supported.
	"CartoonVolumeStyle" => new VRML::NodeType("CartoonVolumeStyle", [
		enabled  => ["SFBool", "TRUE","inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		surfaceNormals => ["SFNode", "NULL", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		orthogonalColor=>["SFColorRGBA",[1,1,1,1],"inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		parallelColor=>["SFColorRGBA",[0,0,0,1],"inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		colorStepse => ["SFInt32", 4, "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
	],"X3DComposableVolumeRenderStyleNode"),
	
	# CompositeVolumeStyle	All fields fully supported.
	"CompositeVolumeStyle" => new VRML::NodeType("CompositeVolumeStyle", [
		enabled  => ["SFBool", "TRUE","inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		#the other renderStyles are SF, this one MF
		renderStyle => ["MFNode", [], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
	],"X3DComposableVolumeRenderStyleNode"),
	
	# ShadedVolumeStyle	All fields fully supported except shadows. Shadows supported with at least Phong shading.
	#level 4
	# ShadedVolumeStyle	All fields fully supported with at least Phong shading and  Henyey-Greenstein phase function. Shadows fully supported.
	"ShadedVolumeStyle" => new VRML::NodeType("ShadedVolumeStyle", [
		enabled  => ["SFBool", "TRUE","inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		lighting  => ["SFBool", "FALSE","inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		shadows => ["SFBool", "FALSE","inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		material => ["SFNode", "NULL", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		surfaceNormals => ["SFNode", "NULL", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		phaseFunction => ["SFString", "Henyey-Greenstein", "initializeOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
	],"X3DComposableVolumeRenderStyleNode"),


	###################################################################################

	# Augmented Reality - not in specs, proposed:
	# http://www.web3d.org/wiki/index.php?title=AR_Proposal_Public_Review
	
	###################################################################################

	"BackdropBackground" => new VRML::NodeType("BackdropBackground", [
		set_bind => ["SFBool", 100, "inputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		bindTime => ["SFTime",0,"outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		isBound => ["SFBool", "FALSE", "outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		transparency => ["SFFloat", 0.0, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		color => ["SFColor", [0,0,0], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		__texture => ["SFInt32", 0, "inputOutput", 0],
		__VBO=>["SFInt32",0,"initializeOnly",0],  # Vertex Buffer Object, if required.
		url => ["MFString", [], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
	],"X3DBackgroundNode"),
	
	"ImageBackdropBackground" => new VRML::NodeType("ImageBackdropBackground", [
		set_bind => ["SFBool", 100, "inputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		bindTime => ["SFTime",0,"outputOnly", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		isBound => ["SFBool", "FALSE", "outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		transparency => ["SFFloat", 0.0, "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		color => ["SFColor", [0,0,0], "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		__texture => ["SFInt32", 0, "inputOutput", 0],
		__VBO=>["SFInt32",0,"initializeOnly",0],  # Vertex Buffer Object, if required.
		image => ["SFImage", "0, 0, 0", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
	],"X3DBackgroundNode"),

	"CalibratedCameraSensor" => new VRML::NodeType("CalibratedCameraSensor", [
		enabled => ["SFBool", "TRUE", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		isActive => ["SFBool", "FALSE", "outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		description => ["SFString", "", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		image => ["SFImage", "0, 0, 0", "outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		focalPoint => ["SFVec2f", [0, 0], "outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		fieldOfView => ["SFFloat", 0.0, "outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		fovMode => ["SFString", "", "outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		aspectRatio => ["SFFloat", 0.0, "outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
	],"X3DSensorNode"),
	
	"TrackingSensor" => new VRML::NodeType("TrackingSensor", [
		enabled => ["SFBool", "TRUE", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		position => ["SFVec3f", [0, 0, 0], "outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		rotation => ["SFRotation", [0, 0, 1, 0], "outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		isActive => ["SFBool", "FALSE", "outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		description => ["SFString", "", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		isPositionAvailable => ["SFBool", "FALSE", "outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		isRotationAvailable => ["SFBool", "FALSE", "outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
	],"X3DSensorNode"),


	# Metadata nodes ...

	###################################################################################

	#used mainly for (pre-2014 era text-based PROTOs attached to Group nodes aka TROTO) PROTO invocation parameters
	#(2014+ era: switched to binary PROTOs (aka Brotos) with their own (not Group) node, which uses routing to go from 
	#  BrotoInterface to BrotoBody nodes - don't need the following now, or the __protoDEF thing in Group, 
	#   except to compile left-over code)
	"MetadataSFFloat" => new VRML::NodeType("MetadataSFFloat", [
			value => ["SFFloat",0.0,"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			valueChanged=>["SFFloat",0.0,"outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			setValue =>["SFFloat",0.0,"inputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			tickTime=>["SFTime",0,"inputOnly",0],
	], "X3DChildNode"),

	#used mainly for PROTO invocation parameters
	"MetadataMFFloat" => new VRML::NodeType("MetadataMFFloat", [
			value => ["MFFloat",[],"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			valueChanged=>["MFFloat",[],"outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			setValue =>["MFFloat",[],"inputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			tickTime=>["SFTime",0,"inputOnly",0],
	], "X3DChildNode"),

	#used mainly for PROTO invocation parameters
	"MetadataSFRotation" => new VRML::NodeType("MetadataSFRotation", [
			value => ["SFRotation",[0,0,0,0],"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			valueChanged=>["SFRotation",[0,0,0,0],"outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			setValue =>["SFRotation",[0,0,0,0],"inputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			tickTime=>["SFTime",0,"inputOnly",0],
	], "X3DChildNode"),

	#used mainly for PROTO invocation parameters
	"MetadataMFRotation" => new VRML::NodeType("MetadataMFRotation", [
			value => ["MFRotation",[],"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			valueChanged=>["MFRotation",[],"outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			setValue =>["MFRotation",[],"inputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			tickTime=>["SFTime",0,"inputOnly",0],
	], "X3DChildNode"),

	#used mainly for PROTO invocation parameters
	"MetadataSFVec3f" => new VRML::NodeType("MetadataSFVec3f", [
			value => ["SFVec3f",[0,0,0],"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			valueChanged=>["SFVec3f",[0,0,0],"outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			setValue =>["SFVec3f",[0,0,0],"inputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			tickTime=>["SFTime",0,"inputOnly",0],
	], "X3DChildNode"),

	#used mainly for PROTO invocation parameters
	"MetadataMFVec3f" => new VRML::NodeType("MetadataMFVec3f", [
			value => ["MFVec3f",[],"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			valueChanged=>["MFVec3f",[],"outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			setValue =>["MFVec3f",[],"inputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			tickTime=>["SFTime",0,"inputOnly",0],
	], "X3DChildNode"),

	#used mainly for PROTO invocation parameters
	"MetadataSFBool" => new VRML::NodeType("MetadataSFBool", [
			value => ["SFBool","FALSE","inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			valueChanged=>["SFBool","FALSE","outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			setValue =>["SFBool","FALSE","inputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			tickTime=>["SFTime",0,"inputOnly",0],
	], "X3DChildNode"),

	#used mainly for PROTO invocation parameters
	"MetadataMFBool" => new VRML::NodeType("MetadataMFBool", [
			value => ["MFBool",[],"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			valueChanged=>["MFBool",[],"outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			setValue =>["MFBool",[],"inputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			tickTime=>["SFTime",0,"inputOnly",0],
	], "X3DChildNode"),

	#used mainly for PROTO invocation parameters
	"MetadataSFInt32" => new VRML::NodeType("MetadataSFInt32", [
			value => ["SFInt32",0,"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			valueChanged=>["SFInt32",0,"outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			setValue =>["SFInt32",0,"inputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			tickTime=>["SFTime",0,"inputOnly",0],
	], "X3DChildNode"),

	#used mainly for PROTO invocation parameters
	"MetadataMFInt32" => new VRML::NodeType("MetadataMFInt32", [
			value => ["MFInt32",[],"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			valueChanged=>["MFInt32",[],"outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			setValue =>["MFInt32",[],"inputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			tickTime=>["SFTime",0,"inputOnly",0],
	], "X3DChildNode"),

	#used mainly for PROTO invocation parameters
	"MetadataSFNode" => new VRML::NodeType("MetadataSFNode", [
			value => ["SFNode",0,"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			valueChanged=>["SFNode",0,"outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			setValue =>["SFNode",0,"inputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			tickTime=>["SFTime",0,"inputOnly",0],
	], "X3DChildNode"),

	#used mainly for PROTO invocation parameters
	"MetadataMFNode" => new VRML::NodeType("MetadataMFNode", [
			value => ["MFNode",[],"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			valueChanged=>["MFNode",[],"outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			setValue =>["MFNode",[],"inputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			tickTime=>["SFTime",0,"inputOnly",0],
	], "X3DChildNode"),

	#used mainly for PROTO invocation parameters
	"MetadataSFColor" => new VRML::NodeType("MetadataSFColor", [
			value => ["SFColor",[0,0,0],"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			valueChanged=>["SFColor",[0,0,0],"outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			setValue =>["SFColor",[0,0,0],"inputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			tickTime=>["SFTime",0,"inputOnly",0],
	], "X3DChildNode"),

	#used mainly for PROTO invocation parameters
	"MetadataMFColor" => new VRML::NodeType("MetadataMFColor", [
			value => ["MFColor",[],"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			valueChanged=>["MFColor",[],"outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			setValue =>["MFColor",[],"inputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			tickTime=>["SFTime",0,"inputOnly",0],
	], "X3DChildNode"),

	#used mainly for PROTO invocation parameters
	"MetadataSFColorRGBA" => new VRML::NodeType("MetadataSFColorRGBA", [
			value => ["SFColorRGBA",[0,0,0,0],"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			valueChanged=>["SFColorRGBA",[0,0,0,0],"outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			setValue =>["SFColorRGBA",[0,0,0,0],"inputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			tickTime=>["SFTime",0,"inputOnly",0],
	], "X3DChildNode"),

	#used mainly for PROTO invocation parameters
	"MetadataMFColorRGBA" => new VRML::NodeType("MetadataMFColorRGBA", [
			value => ["MFColorRGBA",[],"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			valueChanged=>["MFColorRGBA",[],"outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			setValue =>["MFColorRGBA",[],"inputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			tickTime=>["SFTime",0,"inputOnly",0],
	], "X3DChildNode"),

	#used mainly for PROTO invocation parameters
	"MetadataSFTime" => new VRML::NodeType("MetadataSFTime", [
			value => ["SFTime",0,"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			valueChanged=>["SFTime",0,"outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			setValue =>["SFTime",0,"inputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			tickTime=>["SFTime",0,"inputOnly",0],
	], "X3DChildNode"),

	#used mainly for PROTO invocation parameters
	"MetadataMFTime" => new VRML::NodeType("MetadataMFTime", [
			value => ["MFTime",[],"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			valueChanged=>["MFTime",[],"outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			setValue =>["MFTime",[],"inputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			tickTime=>["SFTime",0,"inputOnly",0],
	], "X3DChildNode"),

	#used mainly for PROTO invocation parameters
	"MetadataSFString" => new VRML::NodeType("MetadataSFString", [
			value => ["SFString","","inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			valueChanged=>["SFString","","outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			setValue =>["SFString","","inputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			tickTime=>["SFTime",0,"inputOnly",0],
	], "X3DChildNode"),

	#used mainly for PROTO invocation parameters
	"MetadataMFString" => new VRML::NodeType("MetadataMFString", [
			value => ["MFString",[],"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			valueChanged=>["MFString",[],"outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			setValue =>["MFString",[],"inputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			tickTime=>["SFTime",0,"inputOnly",0],
	], "X3DChildNode"),

	#used mainly for PROTO invocation parameters
	"MetadataSFVec2f" => new VRML::NodeType("MetadataSFVec2f", [
			value => ["SFVec2f",[0,0],"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			valueChanged=>["SFVec2f",[0,0],"outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			setValue =>["SFVec2f",[0,0],"inputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			tickTime=>["SFTime",0,"inputOnly",0],
	], "X3DChildNode"),

	#used mainly for PROTO invocation parameters
	"MetadataMFVec2f" => new VRML::NodeType("MetadataMFVec2f", [
			value => ["MFVec2f",[],"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			valueChanged=>["MFVec2f",[],"outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			setValue =>["MFVec2f",[],"inputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			tickTime=>["SFTime",0,"inputOnly",0],
	], "X3DChildNode"),

	#used mainly for PROTO invocation parameters
	"MetadataSFImage" => new VRML::NodeType("MetadataSFImage", [
			value => ["SFImage",[0,0,0],"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			valueChanged=>["SFImage",[0,0,0],"outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			setValue =>["SFImage",[0,0,0],"inputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			tickTime=>["SFTime",0,"inputOnly",0],
	], "X3DChildNode"),

	#used mainly for PROTO invocation parameters
	"MetadataSFVec3d" => new VRML::NodeType("MetadataSFVec3d", [
			value => ["SFVec3d",[0,0,0],"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			valueChanged=>["SFVec3d",[0,0,0],"outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			setValue =>["SFVec3d",[0,0,0],"inputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			tickTime=>["SFTime",0,"inputOnly",0],
	], "X3DChildNode"),

	#used mainly for PROTO invocation parameters
	"MetadataMFVec3d" => new VRML::NodeType("MetadataMFVec3d", [
			value => ["MFVec3d",[],"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			valueChanged=>["MFVec3d",[],"outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			setValue =>["MFVec3d",[],"inputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			tickTime=>["SFTime",0,"inputOnly",0],
	], "X3DChildNode"),

	#used mainly for PROTO invocation parameters
	"MetadataSFDouble" => new VRML::NodeType("MetadataSFDouble", [
			value => ["SFDouble",0,"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			valueChanged=>["SFDouble",0,"outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			setValue =>["SFDouble",0,"inputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			tickTime=>["SFTime",0,"inputOnly",0],
	], "X3DChildNode"),

	#used mainly for PROTO invocation parameters
	"MetadataMFDouble" => new VRML::NodeType("MetadataMFDouble", [
			value => ["MFDouble",[],"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			valueChanged=>["MFDouble",[],"outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			setValue =>["MFDouble",[],"inputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			tickTime=>["SFTime",0,"inputOnly",0],
	], "X3DChildNode"),

	#used mainly for PROTO invocation parameters
	"MetadataSFMatrix3f" => new VRML::NodeType("MetadataSFMatrix3f", [
			value => ["SFMatrix3f",[0,0,0,0,0,0,0,0,0],"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			valueChanged=>["SFMatrix3f",[0,0,0,0,0,0,0,0,0],"outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			setValue =>["SFMatrix3f",[0,0,0,0,0,0,0,0,0],"inputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			tickTime=>["SFTime",0,"inputOnly",0],
	], "X3DChildNode"),

	#used mainly for PROTO invocation parameters
	"MetadataMFMatrix3f" => new VRML::NodeType("MetadataMFMatrix3f", [
			value => ["MFMatrix3f",[],"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			valueChanged=>["MFMatrix3f",[],"outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			setValue =>["MFMatrix3f",[],"inputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			tickTime=>["SFTime",0,"inputOnly",0],
	], "X3DChildNode"),

	#used mainly for PROTO invocation parameters
	"MetadataSFMatrix3d" => new VRML::NodeType("MetadataSFMatrix3d", [
			value => ["SFMatrix3d",[0,0,0,0,0,0,0,0,0],"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			valueChanged=>["SFMatrix3d",[0,0,0,0,0,0,0,0,0],"outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			setValue =>["SFMatrix3d",[0,0,0,0,0,0,0,0,0],"inputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			tickTime=>["SFTime",0,"inputOnly",0],
	], "X3DChildNode"),

	#used mainly for PROTO invocation parameters
	"MetadataMFMatrix3d" => new VRML::NodeType("MetadataMFMatrix3d", [
			value => ["MFMatrix3d",[],"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			valueChanged=>["MFMatrix3d",[],"outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			setValue =>["MFMatrix3d",[],"inputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			tickTime=>["SFTime",0,"inputOnly",0],
	], "X3DChildNode"),

	#used mainly for PROTO invocation parameters
	"MetadataSFMatrix4f" => new VRML::NodeType("MetadataSFMatrix4f", [
			value => ["SFMatrix4f",[0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			valueChanged=>["SFMatrix4f",[0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],"outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			setValue =>["SFMatrix4f",[0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],"inputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			tickTime=>["SFTime",0,"inputOnly",0],
	], "X3DChildNode"),

	#used mainly for PROTO invocation parameters
	"MetadataMFMatrix4f" => new VRML::NodeType("MetadataMFMatrix4f", [
			value => ["MFMatrix4f",[],"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			valueChanged=>["MFMatrix4f",[],"outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			setValue =>["MFMatrix4f",[],"inputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			tickTime=>["SFTime",0,"inputOnly",0],
	], "X3DChildNode"),

	#used mainly for PROTO invocation parameters
	"MetadataSFMatrix4d" => new VRML::NodeType("MetadataSFMatrix4d", [
			value => ["SFMatrix4d",[0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			valueChanged=>["SFMatrix4d",[0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],"outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			setValue =>["SFMatrix4d",[0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],"inputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			tickTime=>["SFTime",0,"inputOnly",0],
	], "X3DChildNode"),

	#used mainly for PROTO invocation parameters
	"MetadataMFMatrix4d" => new VRML::NodeType("MetadataMFMatrix4d", [
			value => ["MFMatrix4d",[],"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			valueChanged=>["MFMatrix4d",[],"outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			setValue =>["MFMatrix4d",[],"inputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			tickTime=>["SFTime",0,"inputOnly",0],
	], "X3DChildNode"),

	#used mainly for PROTO invocation parameters
	"MetadataSFVec2d" => new VRML::NodeType("MetadataSFVec2d", [
			value => ["SFVec2d",[0,0],"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			valueChanged=>["SFVec2d",[0,0],"outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			setValue =>["SFVec2d",[0,0],"inputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			tickTime=>["SFTime",0,"inputOnly",0],
	], "X3DChildNode"),

	#used mainly for PROTO invocation parameters
	"MetadataMFVec2d" => new VRML::NodeType("MetadataMFVec2d", [
			value => ["MFVec2d",[],"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			valueChanged=>["MFVec2d",[],"outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			setValue =>["MFVec2d",[],"inputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			tickTime=>["SFTime",0,"inputOnly",0],
	], "X3DChildNode"),

	#used mainly for PROTO invocation parameters
	"MetadataSFVec4f" => new VRML::NodeType("MetadataSFVec4f", [
			value => ["SFVec4f",[0,0,0,0],"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			valueChanged=>["SFVec4f",[0,0,0,0],"outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			setValue =>["SFVec4f",[0,0,0,0],"inputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			tickTime=>["SFTime",0,"inputOnly",0],
	], "X3DChildNode"),

	#used mainly for PROTO invocation parameters
	"MetadataMFVec4f" => new VRML::NodeType("MetadataMFVec4f", [
			value => ["MFVec4f",[],"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			valueChanged=>["MFVec4f",[],"outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			setValue =>["MFVec4f",[],"inputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			tickTime=>["SFTime",0,"inputOnly",0],
	], "X3DChildNode"),

	#used mainly for PROTO invocation parameters
	"MetadataSFVec4d" => new VRML::NodeType("MetadataSFVec4d", [
			value => ["SFVec4d",[0,0,0,0],"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			valueChanged=>["SFVec4d",[0,0,0,0],"outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			setValue =>["SFVec4d",[0,0,0,0],"inputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			tickTime=>["SFTime",0,"inputOnly",0],
	], "X3DChildNode"),

	#used mainly for PROTO invocation parameters
	"MetadataMFVec4d" => new VRML::NodeType("MetadataMFVec4d", [
			value => ["MFVec4d",[],"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			valueChanged=>["MFVec4d",[],"outputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			setValue =>["MFVec4d",[],"inputOnly", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
			tickTime=>["SFTime",0,"inputOnly",0],
	], "X3DChildNode"),




	###################################################################################

	# testing...

	###################################################################################
#
# Experimental node: OSC_Sensor
#
# Theory:
# You define one or more OSC_Sensor node in your VRML file,
# and route 'gotEvent' to a JavaScript node which triggers
# when incoming data is received. Vague plans for a OSC_transmitter.
#
# Caveat: Only UDP is supported because that seems to be a hole in liblo.
# *It defines  lo_server_thread_new_with_proto but does not seem to implement it.*
#
# See sample WRL: freewrl/tests/18-OSC-1.wrl
#
# listenfor: typical OSC data spec, for example iii
# filter: typical OSC filter, for example /alpha/beta/gamma
#
# handler: You can choose to write your own handler in src/lib/scenegraph/OSCcallbacks.c
# Uses: You may choose to take 3 incoming delta values and turn it into a vector.
#	You may want to examine the values and turn a stream of packets into a single gesture
# 2 handlers are supplied to use 'as is' or to use as a template for your own code:
# nullOSC_handler - just swallows the callback. This is invoked if handler is undefined (or "")
# defaultOSC_handler - just puts the data into the FIFOs; invoked if handler is defined as "default"
#
# Incoming values are placed into a set of FIFOs. So, if the external agent sent 3 deltas values,
# all 3 values are placed into the FIFO. So, the dummy values intVal, strVal and fltVal are there
# merely so that the JavaScript has something to talk about. The actaul Javascript utility routines
# have been modified to look at FIFOsize. If FIFOsize > 0, then instead of doing a memcpy the
# utility routines retrieve a single value out of the respective FIFO
#
	"OSC_Sensor" => new VRML::NodeType("OSC_Sensor", [

		enabled => ["SFBool", "FALSE","inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		description => ["SFString", "", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		protocol => ["SFString", "UDP", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		listenfor => ["SFString", "", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		port => ["SFInt32", 7000, "inputOutput", 0],
		filter => ["SFString", "", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		handler => ["SFString", "", "inputOutput", "(SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		talksTo => ["MFString", [], "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		FIFOsize  => ["SFInt32", 64, "inputOutput",, 0],
		int32Inp => ["SFInt32", 0, "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		floatInp => ["SFFloat", 0.0, "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		stringInp => ["SFString", "", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],
		gotEvents => ["SFInt32", 0,"inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],

		metadata => ["SFNode", "NULL", "inputOutput", "(SPEC_VRML | SPEC_X3D30 | SPEC_X3D31 | SPEC_X3D32 | SPEC_X3D33)"],

		_talkToNodes => ["MFNode", [], "inputOutput", 0],
		_status => ["SFInt32", -1, "inputOutput", 0],
		_int32InpFIFO => ["FreeWRLPTR",0,"initializeOnly", 0],
		_floatInpFIFO => ["FreeWRLPTR",0,"initializeOnly", 0],
		_stringInpFIFO => ["FreeWRLPTR",0,"initializeOnly", 0],
		_int32OutFIFO => ["FreeWRLPTR",0,"initializeOnly", 0],
		_floatOutFIFO => ["FreeWRLPTR",0,"initializeOnly", 0],
		_stringOutFIFO => ["FreeWRLPTR",0,"initializeOnly", 0],
		__oldmetadata => ["SFNode", 0, "inputOutput", 0], # see code for event macro

	],"X3DNetworkSensorNode"),




);


1;
