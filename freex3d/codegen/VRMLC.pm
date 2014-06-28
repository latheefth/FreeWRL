#
# Copyright (C) 1998 Tuomas J. Lukka 1999 John Stewart CRC Canada
# Portions Copyright (C) 1998 Bernhard Reiter
# DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
# See the GNU Library General Public License (file COPYING in the distribution)
# for conditions of use and redistribution.

use strict;
use warnings;

require 'VRMLFields.pm';
require 'VRMLNodes.pm';
require 'VRMLRend.pm';


sub open_codegen_file(*;$)
{
    my $handle = shift;
    my $filename = shift;

    if (! -f $filename) {
	print STDERR "note: creating $filename\n";
	`touch $filename`;
    }

    if (! -w $filename) {
	die "warning: $filename not writable ...\n";
    }

    no strict 'refs';
    open $handle, ">$filename" or
	die("Can't open: $filename\n");

    print STDERR "Generating: $filename\n";
}


# To allow faster internal representations of nodes to be calculated,
# there is the field '_change' which can be compared between the node
# and the internal rep - if different, needs to be regenerated.
#
# the rep needs to be allocated if _intern == 0.
# XXX Freeing?!!?

#######################################################################
#######################################################################
#######################################################################
#
# gen_struct - Generate a node structure, adding fields for
# internal use
my $interalNodeCommonFields =
               "       int _nodeType; /* unique integer for each type */ \n".
               "       int _renderFlags; /*sensitive, etc */ \n"                  	.
               "       int _hit; \n"                   	.
               "       int _change; \n"                	.
	       "       int _ichange; \n"		.
               "       struct Vector* _parentVector; \n"  .
	       "       double _dist; /*sorting for blending */ \n".
	       "       float _extent[6]; /* used for boundingboxes - +-x, +-y, +-z */ \n" .
               "       struct X3D_PolyRep *_intern; \n"              	.
               "       int referenceCount; /* if this reaches zero, nobody wants it anymore */ \n".
	       "       int _defaultContainer; /* holds the container */\n".
	       "       struct X3D_Node* _executionContext; /* scene or protoInstance */\n".
               " 	/*** node specific data: *****/\n";

sub gen_struct {
	my($name,$node) = @_;

	my @unsortedfields = keys %{$node->{FieldTypes}};

	# sort the field array, so that we can ensure the order of structure
	# elements.

	my @sf = sort(@unsortedfields);
	my $nf = scalar @sf;
	# /* Store actual point etc. later */
       my $s = "/***********************/\nstruct X3D_$name {\n" . $interalNodeCommonFields;

	for(@sf) {
		my $cty = "VRML::Field::$node->{FieldTypes}{$_}"->ctype($_);
		$s .= "\t$cty;\n";
	}

	$s .= "};\n";
	return ($s);
}

#######################################################

sub get_rendfunc {
	my($n) = @_;
	# XXX
	my @f = qw/Prep Rend Child Fin RendRay GenPolyRep Proximity Other Collision Compile/;
	my $comma = "";
	my $v = "\n";

	# create function prototypes
	for (@f) {

		# does this function exist?
		no strict 'refs';
		# Make it less cryptic
		my $package = "VRML::Rend::";
		my $var = $_ . "C";
		if (exists ${$package . $var}{$n}) {
			# it exists in the specified hash; now is the function in CFuncs,
			# or generated here? (different names)
			if ($_ eq "Rend") {
				$v .= $comma."void render_".${n}."(struct X3D_".${n}." *);\n";
			} elsif ($_ eq "Prep") {
				$v .= $comma."void prep_".${n}."(struct X3D_".${n}." *);\n";
			} elsif ($_ eq "Fin") {
				$v .= $comma."void fin_".${n}."(struct X3D_".${n}." *);\n";
			} elsif ($_ eq "Child") {
				$v .= $comma."void child_".${n}."(struct X3D_".${n}." *);\n";
			} elsif ($_ eq "Proximity") {
				$v .= $comma."void proximity_".${n}."(struct X3D_".${n}." *);\n";
			} elsif ($_ eq "Other") {
				$v .= $comma."void other_".${n}."(struct X3D_".${n}." *);\n";
			} elsif ($_ eq "Collision") {
				# some collide_XXX nodes are common
				if (("ElevationGrid" ne ${n}) &&
				("ElevationGrid" ne ${n}) &&
				("IndexedFaceSet" ne ${n}) &&
				("IndexedTriangleFanSet" ne ${n}) &&
				("IndexedTriangleSet" ne ${n}) &&
				("IndexedTriangleStripSet" ne ${n}) &&
				("TriangleFanSet" ne ${n}) &&
				("TriangleSet" ne ${n}) &&
				("TriangleStripSet" ne ${n}) &&
				("QuadSet" ne ${n}) &&
				("IndexedQuadSet" ne ${n}) &&
				("GeoElevationGrid" ne ${n})) {
					$v .= $comma."void collide_".${n}."(struct X3D_".${n}." *);\n";
				}
			} elsif ($_ eq "GenPolyRep") {
				# some make_XXX nodes are common
				if (("ElevationGrid" ne ${n}) &&
				("ElevationGrid" ne ${n}) &&
				("IndexedFaceSet" ne ${n}) &&
				("IndexedTriangleFanSet" ne ${n}) &&
				("IndexedTriangleSet" ne ${n}) &&
				("IndexedTriangleStripSet" ne ${n}) &&
				("TriangleFanSet" ne ${n}) &&
				("TriangleSet" ne ${n}) &&
				("TriangleStripSet" ne ${n}) &&
				("QuadSet" ne ${n}) &&
				("IndexedQuadSet" ne ${n}) &&
				("GeoElevationGrid" ne ${n})) {
					$v .= $comma."void make_".${n}."(struct X3D_".${n}." *);\n";
				}
			} elsif ($_ eq "RendRay") {
				# some rendray_XXX nodes are common
				if (("ElevationGrid" ne ${n}) &&
				("ElevationGrid" ne ${n}) &&
				("Extrusion" ne ${n}) &&
				("Text" ne ${n}) &&
				("IndexedFaceSet" ne ${n}) &&
				("IndexedTriangleFanSet" ne ${n}) &&
				("IndexedTriangleSet" ne ${n}) &&
				("IndexedTriangleStripSet" ne ${n}) &&
				("TriangleFanSet" ne ${n}) &&
				("TriangleSet" ne ${n}) &&
				("TriangleStripSet" ne ${n}) &&
				("QuadSet" ne ${n}) &&
				("IndexedQuadSet" ne ${n}) &&
				("GeoElevationGrid" ne ${n})) {
					$v .= $comma."void rendray_".${n}."(struct X3D_".${n}." *);\n";
				}
			} elsif ($_ eq "Compile") {
				$v .= $comma."void compile_".${n}."(struct X3D_".${n}." *);\n";
			}
		}
	}

	# now go and do the actual filling out of the virtual tables
	my $f = "extern struct X3D_Virt virt_${n};\n";
	$v .= "struct X3D_Virt virt_${n} = { ";

	for (@f) {
		# does this function exist?
		no strict 'refs';
		# Make it less cryptic
		my $package = "VRML::Rend::";
		my $var = $_ . "C";
		if (exists ${$package . $var}{$n}) {
			# it exists in the specified hash; now is the function in CFuncs,
			# or generated here? (different names)
			if ($_ eq "Rend") {
				$v .= $comma."(void *)render_".${n};
			} elsif ($_ eq "Prep") {
				$v .= $comma."(void *)prep_".${n};
			} elsif ($_ eq "Fin") {
				$v .= $comma."(void *)fin_".${n};
			} elsif ($_ eq "Child") {
				$v .= $comma."(void *)child_".${n};
			} elsif ($_ eq "Proximity") {
				$v .= $comma."(void *)proximity_".${n};
			} elsif ($_ eq "Other") {
				$v .= $comma."(void *)other_".${n};
			} elsif ($_ eq "Collision") {
				$v .= $comma."(void *)collide_".${n};
			} elsif ($_ eq "GenPolyRep") {
				$v .= $comma."(void *)make_".${n};
			} elsif ($_ eq "RendRay") {
				$v .= $comma."(void *)rendray_".${n};
			} elsif ($_ eq "Compile") {
				$v .= $comma."(void *)compile_".${n};
			} else {
				$v .= $comma."${n}_$_";
			}
		} else {
			$v .= $comma."NULL";
		}
		$comma = ",";
	}
	$v .= "};\n";

	# now we have a table of entries
	return ($f,$v);
}


######################################################################
######################################################################
######################################################################
#
# gen - the main function. this contains much verbatim code
#
#

#############################################################################################
sub gen {
	# make a table of nodetypes, so that at runtime we can determine what kind a
	# node is - comes in useful at times.
	# also create tables, functions, to create/manipulate VRML/X3D at runtime.


	# DESTINATIONS of arrays:
	#	@genFuncs1,	../src/lib/scenegraph/GeneratedCode.c (printed first)
	#	@genFuncs2,	../src/lib/scenegraph/GeneratedCode.c (printed second)
	#	@str,		../src/lib/vrml_parser/Structs.h
	#	@nodeField,	../src/lib/vrml_parser/NodeFields.h
	#	@EAICommon,	../src/libeai/GeneratedCode.c

	my $nodeIntegerType = 0; # make sure this maps to the same numbers in VRMLCU.pm
	my $fieldTypeCount = 0;
	my $fieldNameCount = 0;
	my $keywordIntegerType = 0;
	my %totalfields = ();
	my %allfields = ();
	my %allinputOutputs = ();
	my %allinputOnlyFields = ();
	my %alloutputOnlyFields = ();

	open LICENSE_BLOCK, "../versions/template/license.c.in";
	my @license_block;
	while (<LICENSE_BLOCK>) {
	    push @license_block, $_;
	}
	close LICENSE_BLOCK;

	#####################
	# for scenegraph/GeneratedCode.c - create a header.
	my @genFuncs1;
	push @genFuncs1,
		"/*\n".
		"  GeneratedCode.c: generated by VRMLC.pm. DO NOT MODIFY, MODIFY VRMLC.pm INSTEAD.\n".
		"*/\n".
		" \n".
		"#include <config.h> \n".
		"#include <system.h> \n".
		"#include <display.h> \n".
		"#include <internal.h> \n".
		" \n".
		"#include <libFreeWRL.h> \n".
		"#include <io_files.h> \n".
		" \n".
		"#include \"../vrml_parser/Structs.h\" \n".
		"#include \"../main/headers.h\" \n".
		"#include \"../main/ProdCon.h\" \n".
		"#include \"Component_Networking.h\" \n".
		"#include \"Component_Picking.h\" \n".
		"#include \"../list.h\" \n".
		"#include \"../io_http.h\" \n".
		" \n".
		" \n".
		"/**********************************************************************************************/ \n".
		"/*                                                                                            */ \n".
		"/* This file is part of the FreeWRL/FreeX3D Distribution, from http://freewrl.sourceforge.net */ \n".
		"/*                                                                                            */ \n".
		"/**********************************************************************************************/ \n".
		" \n".
		"#include \"../input/EAIHeaders.h\" \n".
		"#include \"../input/EAIHelpers.h\" \n".
		"#include \"../x3d_parser/Bindable.h\" \n".
		" \n".
		"#include \"../opengl/Textures.h\" \n".
		"#include \"Component_CubeMapTexturing.h\" \n".
		"#include \"Polyrep.h\" \n".
		"void add_OSCsensor(struct X3D_Node* node); /* WANT_OSC*/\n".
		"void addNodeToKeySensorList(struct X3D_Node* node);\n".
		"void collide_genericfaceset (struct X3D_IndexedFaceSet *node );\n".
		"void make_genericfaceset(struct X3D_IndexedFaceSet *this_);\n".
		"void render_ray_polyrep(void *node);\n".
		"void dump_scene(FILE *fp, int level, struct X3D_Node* node);\n".
		"extern char *parser_getNameFromNode(struct X3D_Node* node);\n";

	my $st = "/* definitions to help scanning values in from a string */ \n".
		"#define SCANTONUMBER(value) while (isspace(*value) || (*value==',')) value++; \n".
		"#define SCANTOSTRING(value) while (isspace(*value) || (*value==',')) value++; \n".
		"#define OLDSCANTOSTRING(value) while ((*value==' ') || (*value==',')) value++; \n".
		"#define ISSTARTNUMBER(value) (isdigit(*value) \\\n".
		"		|| (*value == '+') || (*value == '-')) \n".
		"#define SCANPASTFLOATNUMBER(value) while (isdigit(*value) \\\n".
		"		|| (*value == '.') || \\\n".
		"		(*value == 'E') || (*value == 'e') || (*value == '+') || (*value == '-')) value++; \n".
		"#define SCANPASTINTNUMBER(value) if (isdigit(*value) || (*value == '-') || (*value == '+')) value++; \\\n".
		"		while (isdigit(*value) || \\\n".
		"		(*value == 'x') || (*value == 'X') ||\\\n".
		"		((*value >='a') && (*value <='f')) || \\\n".
		"		((*value >='A') && (*value <='F')) || \\\n".
		"		(*value == '-') || (*value == '+')) value++; \n";
	my @str;
	push @str, $st;

	my @EAICommon;
	push @EAICommon,
		"#include <config.h>\n".
		"#include <system.h>\n".
		"#include <display.h>\n".
		"\n".
		"#include <vrml_parser/Structs.h>\n".
		"#include <main/headers.h>\n".
		"#include <input/EAIHeaders.h>\n".
		"\n";

	#####################


        push @str, "/* Data type for index into ID-table. */\ntypedef int indexT;\n\n";


	# now, go through nodes, and do a few things:
	#	- create a "#define NODE_Shape 300" line for CFuncs/Structs.h;
	#	- create a "case NODE_Shape: printf ("Shape")" for CFuncs/GeneratedCode.c;
	#	- create a definitive list of all fieldnames.

        my @unsortedNodeList = keys %VRML::NodeType::Nodes;
        my @sortedNodeList = sort(@unsortedNodeList);
	my %allinputOnlys;
	my %alloutputOnlys;
	for (@sortedNodeList) {

		push @str, "#define NODE_".$_."	$nodeIntegerType\n";
		$nodeIntegerType ++;

		# capture all fields.
		# also check for metadata here
		my $mdf = 0;
		my $omdf = 0;
 		foreach my $field (keys %{$VRML::NodeType::Nodes{$_}{FieldTypes}}) {
			$totalfields{$field} = "recorded";
			if ($field eq "metadata") {$mdf = $mdf + 1;}
			if ($field eq "__oldmetadata") {$omdf = $omdf + 1;}
		};

		# now, tell what kind of field it is. Hopefully, all fields will
		# have  a valid fieldtype, if not, there is an error somewhere.
 		foreach my $field (keys %{$VRML::NodeType::Nodes{$_}{FieldKinds}}) {
			my $fk = $VRML::NodeType::Nodes{$_}{FieldKinds}{$field};
			if ($fk eq "initializeOnly") { $allfields{$field} = $fk;}
			elsif ($fk eq "inputOnly") { $allinputOnlys{$field} = $fk;}
			elsif ($fk eq "outputOnly") { $alloutputOnlys{$field} = $fk;}
			elsif ($fk eq "inputOutput") { $allinputOutputs{$field} = $fk;}
			else {
				print "field $field fieldKind $fk is invalid\n";
			}
		};
	}
	push @str, "\n";


	#####################
	# we have a list of fields from ALL nodes.

	push @str, "\n/* Table of built-in fieldIds */\nextern const char *FIELDNAMES[];\n";
	push @str, "extern const int FIELDNAMES_COUNT;\n";

	push @genFuncs1, "\n/* Table of built-in fieldIds */\n       const char *FIELDNAMES[] = {\n";

	foreach (sort keys %totalfields) {
		#print "totalfields $_\n";
		push @str, "#define FIELDNAMES_".$_."	$fieldNameCount\n";
		$fieldNameCount ++;
		push @genFuncs1, "	\"$_\",\n";
	}
	push @str, "\n";
	push @genFuncs1, "};\nconst int FIELDNAMES_COUNT = ARR_SIZE(FIELDNAMES);\n\n";

	# make a function to print field name from an integer type.
	my @genFuncs2;
	push @genFuncs2, "/* Return a pointer to a string representation of the field type */\n".
		"const char *stringFieldType (int st) {\n".
		"	if ((st < 0) || (st >= FIELDNAMES_COUNT)) return \"(fieldName invalid)\"; \n".
		"	return FIELDNAMES[st];\n}\n\n";
	push @str, "const char *stringFieldType(int st);\n";

	#####################
	# we have a lists  of field types from ALL nodes. print out the ones without the underscores at the beginning
	push @str, "\n/* Table of built-in fieldIds */\nextern const char *EVENT_OUT[];\n";
	push @str, "extern const int EVENT_OUT_COUNT;\n";

	push @genFuncs1, "\n/* Table of EVENT_OUTs */\n       const char *EVENT_OUT[] = {\n";

	$nodeIntegerType = 0;
	foreach (sort keys %alloutputOnlys) {
		if (index($_,"_") !=0) {
			push @genFuncs1, "	\"$_\",\n";
			push @str, "#define EVENT_OUT_$_	$nodeIntegerType\n";
			$nodeIntegerType ++;
		}
	}
	push @str, "\n";
	push @genFuncs1, "};\nconst int EVENT_OUT_COUNT = ARR_SIZE(EVENT_OUT);\n\n";

	push @str, "\n/* Table of built-in fieldIds */\nextern const char *EVENT_IN[];\n";
	push @str, "extern const int EVENT_IN_COUNT;\n";

	push @genFuncs1, "\n/* Table of EVENT_INs */\n       const char *EVENT_IN[] = {\n";

	$nodeIntegerType = 0;
	foreach (sort keys %allinputOnlys) {
		if (index($_,"_") !=0) {
			push @genFuncs1, "	\"$_\",\n";
			push @str, "#define EVENT_IN_$_	$nodeIntegerType\n";
			$nodeIntegerType ++;
		}
	}
	push @str, "\n";
	push @genFuncs1, "};\nconst int EVENT_IN_COUNT = ARR_SIZE(EVENT_IN);\n\n";

	push @str, "\n/* Table of built-in fieldIds */\nextern const char *EXPOSED_FIELD[];\n";
	push @str, "extern const int EXPOSED_FIELD_COUNT;\n";

	push @genFuncs1, "\n/* Table of EXPOSED_FIELDs */\n       const char *EXPOSED_FIELD[] = {\n";

	$nodeIntegerType = 0;
	foreach (sort keys %allinputOutputs) {
		if (index($_,"_") !=0) {
			push @genFuncs1, "	\"$_\",\n";
			push @str, "#define EXPOSED_FIELD_$_	$nodeIntegerType\n";
			$nodeIntegerType ++;
		}
	}
	push @str, "\n";
	push @genFuncs1, "};\nconst int EXPOSED_FIELD_COUNT = ARR_SIZE(EXPOSED_FIELD);\n\n";

	push @str, "\n/* Table of built-in fieldIds */\nextern const char *FIELD[];\n";
	push @str, "extern const int FIELD_COUNT;\n";

	push @genFuncs1, "\n/* Table of FIELDs */\n       const char *FIELD[] = {\n";

	$nodeIntegerType = 0;
	foreach (sort keys %allfields) {
		if (index($_,"_") !=0) {
			push @genFuncs1, "	\"$_\",\n";
			push @str, "#define FIELD_$_	$nodeIntegerType\n";
			$nodeIntegerType ++;
		}
	}
	push @str, "\n";
	push @genFuncs1, "};\nconst int FIELD_COUNT = ARR_SIZE(FIELD);\n\n";



	#####################
	# process keywords
	push @str, "\n/* Table of built-in keywords */\nextern const char *KEYWORDS[];\n";
	push @str, "extern const int KEYWORDS_COUNT;\n";

	push @genFuncs1, "\n/* Table of keywords */\n       const char *KEYWORDS[] = {\n";

	$keywordIntegerType = 0;
	my @sf = sort keys %VRML::Rend::KeywordC if %VRML::Rend::KeywordC;
	for (@sf) {
		# print "node $_ is tagged as $nodeIntegerType\n";
		# tag each node type with a integer key.

		my $kw = $_;
		push @str, "#define KW_".$kw."	$keywordIntegerType\n";
		$keywordIntegerType ++;
		push @genFuncs1, "	\"$_\",\n";
	}
	push @str, "\n";
	push @genFuncs1, "};\nconst int KEYWORDS_COUNT = ARR_SIZE(KEYWORDS);\n\n";

	# make a function to print Keyword name from an integer type.
	push @genFuncs2, "/* Return a pointer to a string representation of the keyword type */\n".
		"const char *stringKeywordType (int st) {\n".
		"	if ((st < 0) || (st >= KEYWORDS_COUNT)) return \"(keyword invalid)\"; \n".
		"	return KEYWORDS[st];\n}\n\n";
	push @str, "const char *stringKeywordType(int st);\n";


	#####################
	# process  profiles
	push @str, "\n/* Table of built-in profiles */\nextern const char *PROFILES[];\n";
	push @str, "extern const int PROFILES_COUNT;\n";

	push @genFuncs1, "\n/* Table of profiles */\n       const char *PROFILES[] = {\n";

	my $profileIntegerType = 0;
	@sf = sort keys %VRML::Rend::ProfileC if %VRML::Rend::ProfileC;
	for (@sf) {
		# print "node $_ is tagged as $nodeIntegerType\n";
		# tag each node type with a integer key.

		# NOTE: We can not have a profile of "MPEG-4", make it MPEG4 (no "-" allowed)
		my $kw = $_;
		if ($kw eq "MPEG-4") {$kw = "MPEG4";}
		push @str, "#define PRO_".$kw."	$profileIntegerType\n";
		$profileIntegerType ++;
		push @genFuncs1, "	\"$_\",\n";
	}
	push @str, "\n";
	push @genFuncs1, "};\nconst int PROFILES_COUNT = ARR_SIZE(PROFILES);\n\n";

	# make a function to print Profile name from an integer type.
	push @genFuncs2, "/* Return a pointer to a string representation of the profile type */\n".
		"const char *stringProfileType (int st) {\n".
		"	if ((st < 0) || (st >= PROFILES_COUNT)) return \"(profile invalid)\"; \n".
		"	return PROFILES[st];\n}\n\n";
	push @str, "const char *stringProfileType(int st);\n";

	#####################
	# process components
	push @str, "\n/* Table of built-in components */\nextern const char *COMPONENTS[];\n";
	push @str, "extern const int COMPONENTS_COUNT;\n";

	push @genFuncs1, "\n/* Table of components */\nconst char *COMPONENTS[] = {\n";

	my $componentIntegerType = 0;
        @sf = sort keys %VRML::Rend::ComponentC if %VRML::Rend::ComponentC;
	for (@sf) {
		# print "node $_ is tagged as $nodeIntegerType\n";
		# tag each node type with a integer key.

		# NOTE: We can not have a component of "H-ANIM", make it HANIM (no "-" allowed)
		my $kw = $_;
		if ($kw eq "H-Anim") {$kw = "HAnim";}
		push @str, "#define COM_".$kw."	$componentIntegerType\n";
		$componentIntegerType ++;
		push @genFuncs1, "	\"$_\",\n";
	}
	push @str, "\n";
	push @genFuncs1, "};\nconst int COMPONENTS_COUNT = ARR_SIZE(COMPONENTS);\n\n";

	# make a function to print Component name from an integer type.
	push @genFuncs2, "/* Return a pointer to a string representation of the component type */\n".
		"const char *stringComponentType (int st) {\n".
		"	if ((st < 0) || (st >= COMPONENTS_COUNT)) return \"(component invalid)\"; \n".
		"	return COMPONENTS[st];\n}\n\n";
	push @str, "const char *stringComponentType(int st);\n";


	#####################
	# process PROTO keywords
	push @str, "\n/* Table of built-in PROTO keywords */\nextern const char *PROTOKEYWORDS[];\n";
	push @str, "extern const int PROTOKEYWORDS_COUNT;\n";

	push @genFuncs1, "\n/* Table of PROTO keywords */\nconst char *PROTOKEYWORDS[] = {\n";

	$keywordIntegerType = 0;
	@sf = sort keys %VRML::Rend::PROTOKeywordC if %VRML::Rend::PROTOKeywordC;
	for my $node (@sf) {
		push @str, "#define PKW_".$node."	$keywordIntegerType\n";
		$keywordIntegerType ++;
		push @genFuncs1, "	\"$node\",\n";
	}
	push @str, "\n";
	push @genFuncs1, "};\nconst int PROTOKEYWORDS_COUNT = ARR_SIZE(PROTOKEYWORDS);\n\n";

	# make a function to print Keyword name from an integer type.
	push @genFuncs2, "/* Return a pointer to a string representation of the PROTO keyword type */\n".
		"const char *stringPROTOKeywordType (int st) {\n".
		"	if ((st < 0) || (st >= PROTOKEYWORDS_COUNT)) return \"(proto keyword invalid)\"; \n".
		"	return PROTOKEYWORDS[st];\n}\n\n";
	push @str, "const char *stringPROTOKeywordType(int st);\n";


	#####################
	# process MULTITEXTUREMODE keywords
	push @str, "\n/* Table of built-in MULTITEXTUREMODE keywords */\nextern const char *MULTITEXTUREMODE[];\n";
	push @str, "extern const int MULTITEXTUREMODE_COUNT;\n";

	push @genFuncs1, "\n/* Table of MULTITEXTUREMODE keywords */\n       const char *MULTITEXTUREMODE[] = {\n";

        @sf = sort keys %VRML::Rend::MultiTextureModeC if %VRML::Rend::MultiTextureModeC;
	$keywordIntegerType = 0;
	my $MultiText_defs = '#define MULTITEXTUREDefs " \\' . "\n";
	for (@sf) {
		# print "node $_ is tagged as $nodeIntegerType\n";
		# tag each node type with a integer key.
		my $new_str = "#define MTMODE_".$_."	$keywordIntegerType";
		push @str, $new_str . "\n";
		$MultiText_defs .= $new_str . '\n \\' . "\n";
		$keywordIntegerType ++;
		push @genFuncs1, "	\"$_\",\n";
	}
	push @str, "\n";
	push @genFuncs1, "};\nconst int MULTITEXTUREMODE_COUNT = ARR_SIZE(MULTITEXTUREMODE);\n\n";

	# make a function to print Keyword name from an integer type.
	push @genFuncs2, "/* Return a pointer to a string representation of the MULTITEXTUREMODE keyword type */\n".
		"const char *stringMULTITEXTUREMODEType (int st) {\n".
		"	if ((st < 0) || (st >= MULTITEXTUREMODE_COUNT)) return \"(special keyword invalid)\"; \n".
		"	return MULTITEXTUREMODE[st];\n}\n\n";
	push @str, "const char *stringMULTITEXTUREMODEType(int st);\n";

	$MultiText_defs .="\";\n" ;
	push @str, $MultiText_defs;

	#####################
	# process MULTITEXTURESOURCE keywords
	push @str, "\n/* Table of built-in MULTITEXTURESOURCE keywords */\nextern const char *MULTITEXTURESOURCE[];\n";
	push @str, "extern const int MULTITEXTURESOURCE_COUNT;\n";

	push @genFuncs1, "\n/* Table of MULTITEXTURESOURCE keywords */\n       const char *MULTITEXTURESOURCE[] = {\n";

        @sf = sort keys %VRML::Rend::MultiTextureSourceC if %VRML::Rend::MultiTextureSourceC;
	$keywordIntegerType = 0;
	for (@sf) {
		# print "node $_ is tagged as $nodeIntegerType\n";
		# tag each node type with a integer key.
		push @str, "#define MTSRC_".$_."	$keywordIntegerType\n";
		$keywordIntegerType ++;
		push @genFuncs1, "	\"$_\",\n";
	}
	push @str, "\n";
	push @genFuncs1, "};\nconst int MULTITEXTURESOURCE_COUNT = ARR_SIZE(MULTITEXTURESOURCE);\n\n";


	#####################
	# process TextureCoordinateGenerator keywords
	push @str, "\n/* Table of built-in TEXTURECOORDINATEGENERATOR keywords */\nextern const char *TEXTURECOORDINATEGENERATOR[];\n";
	push @str, "extern const int TEXTURECOORDINATEGENERATOR_COUNT;\n";

	push @genFuncs1, "\n/* Table of TEXTURECOORDINATEGENERATOR keywords */\n       const char *TEXTURECOORDINATEGENERATOR[] = {\n";

        @sf = sort keys %VRML::Rend::TextureCoordGenModeC if %VRML::Rend::TextureCoordGenModeC;
	$keywordIntegerType = 0;
	my $TextCoordGen_defs = '#define TEXTURECOORDINATEGENERATORDefs " \\' . "\n";
	for (@sf) {
		# print "node $_ is tagged as $nodeIntegerType\n";
		# tag each node type with a integer key.
		# C defines can not have minus signs in it, make it an underscore
		my $nous = $_;
		$nous=~ s/-/_/g;
		my $new_str = "#define TCGT_".$nous."    $keywordIntegerType";
		push @str, $new_str . "\n";
		$TextCoordGen_defs .= $new_str . '\n \\' . "\n";
		$keywordIntegerType ++;
		push @genFuncs1, "	\"$_\",\n";
	}
	push @str, "\n";
	push @genFuncs1, "};\nconst int TEXTURECOORDINATEGENERATOR_COUNT = ARR_SIZE(TEXTURECOORDINATEGENERATOR);\n\n";

	$TextCoordGen_defs .="\";\n" ;
	push @str, $TextCoordGen_defs;

	####################
	# make a function to print Keyword name from an integer type.
	push @genFuncs2, "/* Return a pointer to a string representation of the MULTITEXTURESOURCE keyword type */\n".
		"const char *stringMULTITEXTURESOURCEType (int st) {\n".
		"	if ((st < 0) || (st >= MULTITEXTURESOURCE_COUNT)) return \"(special keyword invalid)\"; \n".
		"	return MULTITEXTURESOURCE[st];\n}\n\n";
	push @str, "const char *stringMULTITEXTURESOURCEType(int st);\n";


	#####################
	# process MULTITEXTUREFUNCTION keywords
	push @str, "\n/* Table of built-in MULTITEXTUREFUNCTION keywords */\nextern const char *MULTITEXTUREFUNCTION[];\n";
	push @str, "extern const int MULTITEXTUREFUNCTION_COUNT;\n";

	push @genFuncs1, "\n/* Table of MULTITEXTUREFUNCTION keywords */\n       const char *MULTITEXTUREFUNCTION[] = {\n";

        @sf = sort keys %VRML::Rend::MultiTextureFunctionC if %VRML::Rend::MultiTextureFunctionC;
	$keywordIntegerType = 0;
	for (@sf) {
		# print "node $_ is tagged as $nodeIntegerType\n";
		# tag each node type with a integer key.
		push @str, "#define MTFN_".$_."	$keywordIntegerType\n";
		$keywordIntegerType ++;
		push @genFuncs1, "	\"$_\",\n";
	}
	push @str, "\n";
	push @genFuncs1, "};\nconst int MULTITEXTUREFUNCTION_COUNT = ARR_SIZE(MULTITEXTUREFUNCTION);\n\n";

	# make a function to print Keyword name from an integer type.
	push @genFuncs2, "/* Return a pointer to a string representation of the MULTITEXTUREFUNCTION keyword type */\n".
		"const char *stringMULTITEXTUREFUNCTIONType (int st) {\n".
		"	if ((st < 0) || (st >= MULTITEXTUREFUNCTION_COUNT)) return \"(special keyword invalid)\"; \n".
		"	return MULTITEXTUREFUNCTION[st];\n}\n\n";
	push @str, "const char *stringMULTITEXTUREFUNCTIONType(int st);\n";


	#####################
	# process X3DSPECIAL keywords
	push @str, "\n/* Table of built-in X3DSPECIAL keywords */\nextern const char *X3DSPECIAL[];\n";
	push @str, "extern const int X3DSPECIAL_COUNT;\n";

	push @genFuncs1, "\n/* Table of X3DSPECIAL keywords */\n       const char *X3DSPECIAL[] = {\n";

        @sf = sort keys %VRML::Rend::X3DSpecialC if %VRML::Rend::X3DSpecialC;
	$keywordIntegerType = 0;
	for (@sf) {
		# print "node $_ is tagged as $nodeIntegerType\n";
		# tag each node type with a integer key.
		push @str, "#define X3DSP_".$_."	$keywordIntegerType\n";
		$keywordIntegerType ++;
		push @genFuncs1, "	\"$_\",\n";
	}
	push @str, "\n";
	push @genFuncs1, "};\nconst int X3DSPECIAL_COUNT = ARR_SIZE(X3DSPECIAL);\n\n";

	# make a function to print Keyword name from an integer type.
	push @genFuncs2, "/* Return a pointer to a string representation of the X3DSPECIAL keyword type */\n".
		"const char *stringX3DSPECIALType (int st) {\n".
		"	if ((st < 0) || (st >= X3DSPECIAL_COUNT)) return \"(special keyword invalid)\"; \n".
		"	return X3DSPECIAL[st];\n}\n\n";
	push @str, "const char *stringX3DSPECIALType(int st);\n";


	#####################
	# process TEXTUREBOUNDARY keywords
	push @str, "\n/* Table of built-in TEXTUREBOUNDARY keywords */\nextern const char *TEXTUREBOUNDARYKEYWORDS[];\n";
	push @str, "extern const int TEXTUREBOUNDARYKEYWORDS_COUNT;\n";

	push @genFuncs1, "\n/* Table of TEXTUREBOUNDARY keywords */\n       const char *TEXTUREBOUNDARYKEYWORDS[] = {\n";

        @sf = sort keys %VRML::Rend::TextureBoundaryC if %VRML::Rend::TextureBoundaryC;
	$keywordIntegerType = 0;
	for (@sf) {
		# print "node $_ is tagged as $nodeIntegerType\n";
		# tag each node type with a integer key.
		push @str, "#define TB_".$_."	$keywordIntegerType\n";
		$keywordIntegerType ++;
		push @genFuncs1, "	\"$_\",\n";
	}
	push @str, "\n";
	push @genFuncs1, "};\nconst int TEXTUREBOUNDARYKEYWORDS_COUNT = ARR_SIZE(TEXTUREBOUNDARYKEYWORDS);\n\n";

	# make a function to print Keyword name from an integer type.
	push @genFuncs2, "/* Return a pointer to a string representation of the TEXTUREBOUNDARY keyword type */\n".
		"const char *stringTEXTUREBOUNDARYKeywordType (int st) {\n".
		"	if ((st < 0) || (st >= TEXTUREBOUNDARYKEYWORDS_COUNT)) return \"(texture param keyword invalid)\"; \n".
		"	return TEXTUREBOUNDARYKEYWORDS[st];\n}\n\n";
	push @str, "const char *stringTEXTUREBOUNDARYKeywordType(int st);\n";

	#####################
	# process TEXTUREMAGNIFICATION keywords
	push @str, "\n/* Table of built-in TEXTUREMAGNIFICATION keywords */\nextern const char *TEXTUREMAGNIFICATIONKEYWORDS[];\n";
	push @str, "extern const int TEXTUREMAGNIFICATIONKEYWORDS_COUNT;\n";

	push @genFuncs1, "\n/* Table of TEXTUREMAGNIFICATION keywords */\n       const char *TEXTUREMAGNIFICATIONKEYWORDS[] = {\n";

        @sf = sort keys %VRML::Rend::TextureMagnificationC if %VRML::Rend::TextureMagnificationC;
	$keywordIntegerType = 0;
	for (@sf) {
		# print "node $_ is tagged as $nodeIntegerType\n";
		# tag each node type with a integer key.
		push @str, "#define TMAG_".$_."	$keywordIntegerType\n";
		$keywordIntegerType ++;
		push @genFuncs1, "	\"$_\",\n";
	}
	push @str, "\n";
	push @genFuncs1, "};\nconst int TEXTUREMAGNIFICATIONKEYWORDS_COUNT = ARR_SIZE(TEXTUREMAGNIFICATIONKEYWORDS);\n\n";

	# make a function to print Keyword name from an integer type.
	push @genFuncs2, "/* Return a pointer to a string representation of the TEXTUREMAGNIFICATION keyword type */\n".
		"const char *stringTEXTUREMAGNIFICATIONKeywordType (int st) {\n".
		"	if ((st < 0) || (st >= TEXTUREMAGNIFICATIONKEYWORDS_COUNT)) return \"(texture param keyword invalid)\"; \n".
		"	return TEXTUREMAGNIFICATIONKEYWORDS[st];\n}\n\n";
	push @str, "const char *stringTEXTUREMAGNIFICATIONKeywordType(int st);\n";


	#####################
	# process TEXTUREMINIFICATION keywords
	push @str, "\n/* Table of built-in TEXTUREMINIFICATION keywords */\nextern const char *TEXTUREMINIFICATIONKEYWORDS[];\n";
	push @str, "extern const int TEXTUREMINIFICATIONKEYWORDS_COUNT;\n";

	push @genFuncs1, "\n/* Table of TEXTUREMINIFICATION keywords */\n       const char *TEXTUREMINIFICATIONKEYWORDS[] = {\n";

        @sf = sort keys %VRML::Rend::TextureMinificationC if %VRML::Rend::TextureMinificationC;
	$keywordIntegerType = 0;
	for (@sf) {
		# print "node $_ is tagged as $nodeIntegerType\n";
		# tag each node type with a integer key.
		push @str, "#define TMIN_".$_."	$keywordIntegerType\n";
		$keywordIntegerType ++;
		push @genFuncs1, "	\"$_\",\n";
	}
	push @str, "\n";
	push @genFuncs1, "};\nconst int TEXTUREMINIFICATIONKEYWORDS_COUNT = ARR_SIZE(TEXTUREMINIFICATIONKEYWORDS);\n\n";

	# make a function to print Keyword name from an integer type.
	push @genFuncs2, "/* Return a pointer to a string representation of the TEXTUREMINIFICATION keyword type */\n".
		"const char *stringTEXTUREMINIFICATIONKeywordType (int st) {\n".
		"	if ((st < 0) || (st >= TEXTUREMINIFICATIONKEYWORDS_COUNT)) return \"(texture param keyword invalid)\"; \n".
		"	return TEXTUREMINIFICATIONKEYWORDS[st];\n}\n\n";
	push @str, "const char *stringTEXTUREMINIFICATIONKeywordType(int st);\n";

	#####################
	# process TEXTURECOMPRESSION keywords
	push @str, "\n/* Table of built-in TEXTURECOMPRESSION keywords */\nextern const char *TEXTURECOMPRESSIONKEYWORDS[];\n";
	push @str, "extern const int TEXTURECOMPRESSIONKEYWORDS_COUNT;\n";

	push @genFuncs1, "\n/* Table of TEXTURECOMPRESSION keywords */\n       const char *TEXTURECOMPRESSIONKEYWORDS[] = {\n";

        @sf = sort keys %VRML::Rend::TextureCompressionC if %VRML::Rend::TextureCompressionC;
	$keywordIntegerType = 0;
	for (@sf) {
		# print "node $_ is tagged as $nodeIntegerType\n";
		# tag each node type with a integer key.
		push @str, "#define TC_".$_."	$keywordIntegerType\n";
		$keywordIntegerType ++;
		push @genFuncs1, "	\"$_\",\n";
	}
	push @str, "\n";
	push @genFuncs1, "};\nconst int TEXTURECOMPRESSIONKEYWORDS_COUNT = ARR_SIZE(TEXTURECOMPRESSIONKEYWORDS);\n\n";

	# make a function to print Keyword name from an integer type.
	push @genFuncs2, "/* Return a pointer to a string representation of the TEXTURECOMPRESSION keyword type */\n".
		"const char *stringTEXTURECOMPRESSIONKeywordType (int st) {\n".
		"	if ((st < 0) || (st >= TEXTURECOMPRESSIONKEYWORDS_COUNT)) return \"(texture param keyword invalid)\"; \n".
		"	return TEXTURECOMPRESSIONKEYWORDS[st];\n}\n\n";
	push @str, "const char *stringTEXTURECOMPRESSIONKeywordType(int st);\n";


	#####################
	# process GEOSPATIAL keywords
	push @str, "\n/* Table of built-in GEOSPATIAL keywords */\nextern const char *GEOSPATIAL[];\n";
	push @str, "extern const int GEOSPATIAL_COUNT;\n";

	push @genFuncs1, "\n/* Table of GEOSPATIAL keywords */\n       const char *GEOSPATIAL[] = {\n";

        @sf = sort keys %VRML::Rend::GEOSpatialKeywordC if %VRML::Rend::GEOSpatialKeywordC;
	$keywordIntegerType = 0;
	for (@sf) {
		# print "node $_ is tagged as $nodeIntegerType\n";
		# tag each node type with a integer key.
		push @str, "#define GEOSP_".$_."	$keywordIntegerType\n";
		$keywordIntegerType ++;
		push @genFuncs1, "	\"$_\",\n";
	}
	push @str, "\n";
	push @genFuncs1, "};\nconst int GEOSPATIAL_COUNT = ARR_SIZE(GEOSPATIAL);\n\n";

	# make a function to print Keyword name from an integer type.
	push @genFuncs2, "/* Return a pointer to a string representation of the GEOSPATIAL keyword type */\n".
		"const char *stringGEOSPATIALType (int st) {\n".
		"	if ((st < 0) || (st >= GEOSPATIAL_COUNT)) return \"(keyword invalid)\"; \n".
		"	return GEOSPATIAL[st];\n}\n\n";
	push @str, "const char *stringGEOSPATIALType(int st);\n";

	##############################################################

	# Convert TO/FROM EAI to Internal field types. (EAI types are ascii).
	$st = "char mapFieldTypeToEAItype (int st) {\n".
	    "	switch (st) { \n";
	push @genFuncs2, $st; push @EAICommon, $st;


	for(@VRML::Field::Fields) {
		# print "node $_ is tagged as $fieldTypeCount\n";
		# tag each node type with a integer key.
		my $st = "\t\tcase FIELDTYPE_".$_.":	return EAI_$_;\n";
		push @genFuncs2, $st; push @EAICommon, $st;
	}
	$st = "\t\tdefault: return -1;\n\t}\n\treturn -1;\n}\n";
	push @genFuncs2, $st; push @EAICommon, $st;
	push @str, "char mapFieldTypeToEAItype (int st);\n";


	####################
	$st = "/* convert an EAI type to an internal type */\n".
		"int mapEAItypeToFieldType (char st) {\n".
		"	switch (st) { \n";
	push @genFuncs2, $st; push @EAICommon, $st;


	for(@VRML::Field::Fields) {
		# print "node $_ is tagged as $fieldTypeCount\n";
		# tag each node type with a integer key.
		my $st = "\t\tcase EAI_".$_.":	return FIELDTYPE_$_;\n";
		push @genFuncs2, $st; push @EAICommon, $st;
	}
	$st = "\t\tdefault: return -1;\n\t}\n\treturn -1;\n}\n";
	push @genFuncs2, $st; push @EAICommon, $st;
	push @str, "int mapEAItypeToFieldType (char st);\n";

	####################
	push @genFuncs2, "/* convert an MF type to an SF type */\n".
		"int convertToSFType (int st) {\n".
		"	switch (st) { \n";


	for(@VRML::Field::Fields) {
		# print "node $_ is tagged as $fieldTypeCount\n";
		# tag each node type with a integer key.
		my $sftype = $_;
		$sftype =~ s/MF/SF/;

		push @genFuncs2,  "\t\tcase FIELDTYPE_$_:	return FIELDTYPE_$sftype;\n";
	}
	push @genFuncs2, 	"	}\n	return -1;\n}\n";
	push @str, "int convertToSFType (int st);\n";





	#####################
	# give each field an identifier
	push @str, "\n/* Table of built-in fieldIds */\nextern const char *FIELDTYPES[];\n";
	push @str, "extern const int FIELDTYPES_COUNT;\n";

	my $ts = "\n/* Table of Field Types */\n       const char *FIELDTYPES[] = {\n";
	push @genFuncs1, $ts;
	push @EAICommon, $ts;

	$fieldTypeCount = 0;
	my $printNodeStr;
	for(@VRML::Field::Fields) {
		# print "node $_ is tagged as $fieldTypeCount\n";
		# tag each node type with a integer key.
		my $defstr = "#define FIELDTYPE_".$_."	$fieldTypeCount\n";
		push @str, $defstr;
		$fieldTypeCount ++;
		$printNodeStr = "	\"$_\",\n";
		push @genFuncs1, $printNodeStr;
		push @EAICommon, $printNodeStr;
	}
	push @str, "\n";
	$ts = "};\nconst int FIELDTYPES_COUNT = ARR_SIZE(FIELDTYPES);\n\n";
	push @genFuncs1, $ts;
	push @EAICommon, $ts;

	for(@VRML::Field::Fields) {
		push @str, ("VRML::Field::$_")->cstruct . "\n";
	}
	# make a function to print fieldtype name from an integer type.
	push @genFuncs2, "/* Return a pointer to a string representation of the fieldtype type */\n".
		"const char *stringFieldtypeType (int st) {\n".
		"	if ((st < 0) || (st >= FIELDTYPES_COUNT)) return \"(fieldType invalid)\"; \n".
		"	return FIELDTYPES[st];\n}\n\n";
	push @str, "const char *stringFieldtypeType(int st);\n";



	#####################
	# handle the nodes themselves
	push @str, "\n/* Table of built-in nodeIds */\nextern const char *NODES[];\n";
	push @str, "extern const int NODES_COUNT;\n";

	push @genFuncs1, "\n/* Table of Node Types */\n       const char *NODES[] = {\n";

        push @str, "\n/* and now the structs for the nodetypes */ \n";
	for(@sortedNodeList) {
		$printNodeStr = "	\"$_\",\n";
		push @genFuncs1, $printNodeStr;
	}
	push @genFuncs1, "};\nconst int NODES_COUNT = ARR_SIZE(NODES);\n\n";

	# make a function to print node name from an integer type.
	push @genFuncs2, "/* Return a pointer to a string representation of the node type */\n".
		"const char *stringNodeType (int st) {\n".
		"	if ((st < 0) || (st >= NODES_COUNT)) return \"(node invalid)\"; \n".
		"	return NODES[st];\n}\n\n";
	push @str, "const char *stringNodeType(int st);\n";


	###################
	# create the virtual tables for each node.
	push @str, "\n/* First, a generic struct, contains only the common elements */\n".
	"struct X3D_Node {\n". $interalNodeCommonFields .  "};\n".
	"#define X3D_LINEPROPERTIES(node) ((struct X3D_LineProperties*)node)\n".
	"#define X3D_FILLPROPERTIES(node) ((struct X3D_FillProperties*)node)\n".
	"#define X3D_TEXTURE_TRANSFORM(node) ((struct X3D_TextureTransform*)node)\n".
	"#define X3D_NODE(node) ((struct X3D_Node*)node)\n".
	"#define X3D_APPEARANCE(node) ((struct X3D_Appearance*)node)\n".
	"#define X3D_MATERIAL(node) ((struct X3D_Material*)node)\n".
	"#define X3D_TWOSIDEDMATERIAL(node) ((struct X3D_TwoSidedMaterial*)node)\n".
	"#define X3D_GROUP(node) ((struct X3D_Group*)node)\n".
	"#define X3D_PROTO(node) ((struct X3D_Proto*)node)\n".
	"#define X3D_PICKABLEGROUP(node) ((struct X3D_PickableGroup*)node)\n".
	"#define X3D_POINTPICKSENSOR(node) ((struct X3D_PointPickSensor*)node)\n".
	"#define X3D_STATICGROUP(node) ((struct X3D_StaticGroup*)node)\n".
	"#define X3D_ANCHOR(node) ((struct X3D_Anchor*)node)\n".
	"#define X3D_COLLISION(node) ((struct X3D_Collision*)node)\n".
	"#define X3D_COMPOSEDSHADER(node) ((struct X3D_ComposedShader*)node)\n".
	"#define X3D_PACKAGEDSHADER(node) ((struct X3D_PackagedShader*)node)\n".
	"#define X3D_PROGRAMSHADER(node) ((struct X3D_ProgramShader*)node)\n".
	"#define X3D_SHADERPROGRAM(node) ((struct X3D_ShaderProgram*)node)\n".
	"#define X3D_SHAPE(node) ((struct X3D_Shape*)node)\n".
	"#define X3D_VISIBILITYSENSOR(node) ((struct X3D_VisibilitySensor*)node)\n".
	"#define X3D_BILLBOARD(node) ((struct X3D_Billboard*)node)\n".
	"#define X3D_NAVIGATIONINFO(node) ((struct X3D_NavigationInfo*)node)\n".
	"#define X3D_BACKGROUND(node) ((struct X3D_Background*)node)\n".
	"#define X3D_TEXTUREBACKGROUND(node) ((struct X3D_TextureBackground*)node)\n".
	"#define X3D_FOG(node) ((struct X3D_Fog*)node)\n".
	"#define X3D_INLINE(node) ((struct X3D_Inline*)node)\n".
	"#define X3D_SWITCH(node) ((struct X3D_Switch*)node)\n".
	"#define X3D_CADASSEMBLY(node) ((struct X3D_CADAssembly*)node)\n".
	"#define X3D_CADFACE(node) ((struct X3D_CADFace*)node)\n".
	"#define X3D_CADLAYER(node) ((struct X3D_CADLayer*)node)\n".
	"#define X3D_CADPART(node) ((struct X3D_CADPart*)node)\n".
	"#define X3D_SCRIPT(node) ((struct X3D_Script*)node)\n".
	"#define X3D_VIEWPOINT(node) ((struct X3D_Viewpoint*)node)\n".
	"#define X3D_ORTHOVIEWPOINT(node) ((struct X3D_OrthoViewpoint*)node)\n".
	"#define X3D_LODNODE(node) ((struct X3D_LOD*)node)\n".
	"#define X3D_TRANSFORM(node) ((struct X3D_Transform*)node)\n".
	"#define X3D_PROXIMITYSENSOR(node) ((struct X3D_ProximitySensor*)node)\n".
	"#define X3D_POINTLIGHT(node) ((struct X3D_PointLight*)node)\n".
	"#define X3D_SPOTLIGHT(node) ((struct X3D_SpotLight*)node)\n".
	"#define X3D_DIRECTIONALLIGHT(node) ((struct X3D_DirectionalLight*)node)\n".
	"#define X3D_INDEXEDFACESET(node) ((struct X3D_IndexedFaceSet*)node)\n".
	"#define X3D_INDEXEDLINESET(node) ((struct X3D_IndexedLineSet*)node)\n".
	"#define X3D_ELEVATIONGRID(node) ((struct X3D_ElevationGrid*)node)\n".
	"#define X3D_INDEXEDTRIANGLEFANSET(node) ((struct X3D_IndexedTriangleFanSet*)node)\n".
	"#define X3D_INDEXEDTRIANGLESET(node) ((struct X3D_IndexedTriangleSet*)node)\n".
	"#define X3D_INDEXEDTRIANGLESTRIPSET(node) ((struct X3D_IndexedTriangleStripSet*)node)\n".
	"#define X3D_TRIANGLEFANSET(node) ((struct X3D_TriangleFanSet*)node)\n".
	"#define X3D_TRIANGLESET(node) ((struct X3D_TriangleSet*)node)\n".
	"#define X3D_TRIANGLESTRIPSET(node) ((struct X3D_TriangleStripSet*)node)\n".
	"#define X3D_QUADSET(node) ((struct X3D_QuadSet*)node)\n".
	"#define X3D_INDEXEDQUADSET(node) ((struct X3D_IndexedQuadSet*)node)\n".


	"#define X3D_GEOORIGIN(node) ((struct X3D_GeoOrigin*)node)\n".
	"#define X3D_GEOLOD(node) ((struct X3D_GeoLOD*)node)\n".
	"#define X3D_GEOCOORD(node) ((struct X3D_GeoCoordinate*)node)\n".
	"#define X3D_GEOVIEWPOINT(node) ((struct X3D_GeoViewpoint*)node)\n".
	"#define X3D_GEOELEVATIONGRID(node) ((struct X3D_GeoElevationGrid*)node)\n".
	"#define X3D_GEOLOCATION(node) ((struct X3D_GeoLocation*)node)\n".
	"#define X3D_GEOTRANSFORM(node) ((struct X3D_GeoTransform*)node)\n".
	"#define X3D_GEOPROXIMITYSENSOR(node) ((struct X3D_GeoProximitySensor*)node)\n".

	"#define X3D_COLOR(node) ((struct X3D_Color*)node)\n".
	"#define X3D_COORDINATE(node) ((struct X3D_Coordinate*)node)\n".
	"#define X3D_COORDINATEINTERPOLATOR(node) ((struct X3D_CoordinateInterpolator*)node)\n".
	"#define X3D_NORMAL(node) ((struct X3D_Normal*)node)\n".
	"#define X3D_TEXTURECOORDINATE(node) ((struct X3D_TextureCoordinate*)node)\n".
	"#define X3D_IMAGETEXTURE(node) ((struct X3D_ImageTexture*)node)\n".
	"#define X3D_TEXTUREPROPERTIES(node) ((struct X3D_TextureProperties*)node)\n".
	"#define X3D_PIXELTEXTURE(node) ((struct X3D_PixelTexture*)node)\n".


	"#undef DEBUG_VALIDNODE\n".
	"#ifdef DEBUG_VALIDNODE	\n".
	"#define X3D_NODE_CHECK(node) checkNode(node,__FILE__,__LINE__)\n".
	"#define MARK_EVENT(node,offset) mark_event_check(node,(int) offset,__FILE__,__LINE__)\n".
	"#else\n".
	"#define X3D_NODE_CHECK(node)\n".
	"#define MARK_EVENT(node,offset)	mark_event(node,(int) offset)\n".
	"#endif\n".
	"#define COPY_SFVEC3F_TO_POINT_XYZ(too,from) { too.x = from[0]; too.y = from[1]; too.z = from[2];}\n".
	"#define COPY_POINT_XYZ_TO_SFVEC3F(too,from) { too[0] = (float) from.x; too[1] = (float) from.y; too[2] = (float) from.z;}\n".
	"#define offsetPointer_deref(t, me, offs) ((t)(((char*)(me))+offs))\n".

	"\n/* now, generated structures for each VRML/X3D Node*/\n";


	push @genFuncs1, "/* Virtual tables for each node */\n";

	for(@sortedNodeList) {
		# print "working on node $_\n";
		my $no = $VRML::NodeType::Nodes{$_};
		my($strret) = gen_struct($_, $no);
		push @str, $strret;

		my($externdeclare, $vstru) = get_rendfunc($_);
		push @str, $externdeclare;

		push @genFuncs1, $vstru;
	}
	push @genFuncs1, "\n";

	push @genFuncs1, "/* table containing pointers to every virtual struct for each node type */ \n";
	push @genFuncs1, "struct X3D_Virt* virtTable[] = { \n";
	push @str, "extern struct X3D_Virt* virtTable[];\n";
	for(@sortedNodeList) {

		push @genFuncs1, "\t &virt_".$_.",\n";
	}
	push @genFuncs1, "\tNULL}; \n\n";

	#####################
	# create a routine to create a new node type

	push @genFuncs2,
	"/* create a new node of type. This can be generated by Perl code, much as the Structs.h is */\n".
	"void *createNewX3DNode0 (int nt) {\n".
	"	void * tmp;\n".
	"	struct X3D_Box *node;\n".
	"\n".
	"	tmp = NULL;\n".
	"	switch (nt) {\n";

	for (@sortedNodeList) {
		push @genFuncs2,
			"		case NODE_$_ : {tmp = MALLOC (struct X3D_$_ *, sizeof (struct X3D_$_)); break;}\n";
	}
	push @genFuncs2, "		default: {printf (\"createNewX3DNode = unknown type %d, this will fail\\n\",nt); return NULL;}\n";

	push @genFuncs2,
	"	}\n\n".
	"	/* now, fill in the node to DEFAULT values This mimics \"alloc_struct\" in the Perl code. */\n".
	"	/* the common stuff between all nodes. We'll use a X3D_Box struct, just because. It is used\n".
	"	   in this way throughought the code */\n".
	"	node = (struct X3D_Box *) tmp;\n".
	"	node->_renderFlags = 0; /*sensitive, etc */\n".
	"	node->_hit = 0;\n".
	"	node->_change = NODE_CHANGE_INIT_VAL; \n".
	"	node->_parentVector = newVector(struct X3D_Node*, 1);\n".
	"	node->_ichange = 0;\n".
	"	node->_dist = -10000.0; /*sorting for blending */\n".
	"	INITIALIZE_EXTENT\n".
	"	node->_intern = 0;\n".
	"	node->_nodeType = nt; /* unique integer for each type */\n".
	"	node->referenceCount = 1; /* we have requested this, we want it! */\n".
	"	\n";


	push @genFuncs2,
	"	/* now, fill in the node specific stuff here. the defaults are in VRMLNodes.pm */\n".
	"	switch (nt) {\n";

	for my $node (@sortedNodeList) {
		push @genFuncs2,
			"\t\tcase NODE_$node : {\n\t\t\tstruct X3D_$node * tmp2;\n";

		push @genFuncs2, "\t\t\ttmp2 = (struct X3D_$node *) tmp;\n";

 		foreach my $field (sort keys %{$VRML::NodeType::Nodes{$node}{Defaults}}) {
			my $ft = $VRML::NodeType::Nodes{$node}{FieldTypes}{$field};
			my $fk = $VRML::NodeType::Nodes{$node}{FieldKinds}{$field};
			my $def = $VRML::NodeType::Nodes{$node}{Defaults}{$field};

			# do we need to initialize the occlusion number for fields?
			my $cf;
			if ($field eq "__OccludeNumber") {
			    $cf = "tmp2->__OccludeNumber = newOcclude();";
			} else {
			    $cf = ("VRML::Field::$ft")->cInitialize("tmp2->".$field,$def);
			}
			push @genFuncs2, "\t\t\t$cf;\n";
		}

	# rig in the default container for X3D parsing.
	if (exists $VRML::Rend::defaultContainerType{$node}) {
		push @genFuncs2, "\t\t\ttmp2->_defaultContainer = FIELDNAMES_".$VRML::Rend::defaultContainerType{$node}.";\n";
	} else {
		print "defaultContainerType for $node missing\n";
	}

		push @genFuncs2,"\t\tbreak;\n\t\t}\n";
	}
	push @genFuncs2, "\t};\n";


	push @genFuncs2, "\treturn tmp;\n}\n";

	push @genFuncs2,
	"/* create a new node of type. This can be generated by Perl code, much as the Structs.h is */\n".
	"void *createNewX3DNode (int nt) {\n".
	"	void * tmp;\n".
	"	tmp = createNewX3DNode0(nt);\n";

	push @genFuncs2,
	"	\n".
	"	/* is this a texture holding node? */\n".
	"	registerTexture(tmp);\n".
	"	/* Node Tracking */\n".
	"	registerX3DNode(tmp);\n".
	"	/* is this a bindable node? */\n".
	"	registerBindable(tmp);\n".
	"	/* is this a OSC sensor node? */\n".
	"	add_OSCsensor(tmp); /* WANT_OSC */\n".
	"	/* is this a pick sensor node? */\n".
	"	add_picksensor(tmp); /* DJTRACK_PICKSENSORS */\n".
	"	/* is this a time tick node? */\n".
	"       add_first(tmp);\n".
	"       /* possibly a KeySensor node? */\n".
	"       addNodeToKeySensorList(X3D_NODE(tmp));\n";
	push @genFuncs2, "\treturn tmp;\n}\n";


	#####################################################################
	# create a routine to dump scene graph.

	push @genFuncs2,
	"/* Dump the scene graph.  */\n".
	"#define Boolean int\n".
	"void dump_scene (FILE *fp, int level, struct X3D_Node* node) {\n".
	"	#define spacer	for (lc=0; lc<level; lc++) fprintf (fp,\" \");\n".
	"	int lc;\n".
	"	int i;\n".
        "	char *nodeName;\n".
	"	#ifdef FW_DEBUG\n\t\tBoolean allFields;\n".
	"		if (fileno(fp) == fileno(stdout)) { allFields = TRUE; } else { allFields = FALSE; }\n".
	"	#else\n\t\tBoolean allFields = FALSE;\n\t#endif\n".
	"	/* See vi +/double_conditional codegen/VRMLC.pm */\n".
	"	if (node==NULL) return; \n\n".
	"	fflush(fp);\n".
	"	if (level == 0) fprintf (fp,\"starting dump_scene\\n\");\n".
	"	nodeName = parser_getNameFromNode(node) ;\n".
	"	if (nodeName == NULL) {\n".
	"		spacer fprintf (fp,\"L%d: node (%p) () type %s\\n\",level,node,stringNodeType(node->_nodeType));\n".
        "	} else {\n".
	"		spacer fprintf (fp,\"L%d: node (%p) (DEF %s) type %s\\n\",level,node,nodeName,stringNodeType(node->_nodeType));\n".
        "	}\n".
	"	switch (node->_nodeType) {\n";

	for my $node (@sortedNodeList) {
		push @genFuncs2, "		case NODE_$node : {\n";
		push @genFuncs2, "			struct X3D_$node *tmp;\n";
		push @genFuncs2, "			tmp = (struct X3D_$node *) node;\n";
		push @genFuncs2, "			UNUSED(tmp); // compiler warning mitigation\n";
		if($node eq "PointPickSensor") {
			push @genFuncs2, "\t\t\tspacer fprintf (fp,\" _nparents (int) %d\\n\",vectorSize(tmp->_parentVector)); /* DJTRACK_PICKSENSORS */\n";
			push @genFuncs2, "\t\t\tfor (i=0; i<vectorSize(tmp->_parentVector); i++) { spacer fprintf (fp,\"    %d: %p\\n\",i, vector_get(struct X3D_Node *, tmp->_parentVector,i)); }\n";
		}
 		foreach my $field (sort keys %{$VRML::NodeType::Nodes{$node}{Defaults}}) {

			my $ft = $VRML::NodeType::Nodes{$node}{FieldTypes}{$field};
			my $fk = $VRML::NodeType::Nodes{$node}{FieldKinds}{$field};
			if ( ($fk eq "field") || ($fk eq "inputOutput") ) {
				#
				# This is effectively a double_conditional,
				# ie the conditional is only inserted if
				# the variable meets certain criteria.
				#
				my $firstC = substr($field, 0, 1);
				if (($firstC eq "_") || $field eq "metadata") {
					push @genFuncs2, "\t\t    if(allFields) {\n";
				}
				if ($ft eq "FreeWRLPTR") {
					push @genFuncs2, "\t\t\tspacer fprintf (fp,\" $field ($ft) (void pointer, not dumped)\\n\");\n";

				} elsif ($ft eq "FreeWRLThread") {
					push @genFuncs2, "\t\t\tspacer fprintf (fp,\" $field ($ft) (thread pointer, not dumped)\\n\");\n";

				} elsif ($ft eq "SFInt32") {
					push @genFuncs2, "\t\t\tspacer fprintf (fp,\" $field ($ft) \\t%d\\n\",tmp->$field);\n";

				} elsif (($ft eq "SFFloat") || ($ft eq "SFTime") || ($ft eq "SFDouble")) {
					push @genFuncs2, "\t\t\tspacer fprintf (fp,\" $field ($ft) \\t%4.3f\\n\",tmp->$field);\n";

				} elsif ($ft eq "SFBool") {
					push @genFuncs2, "\t\t\tspacer fprintf (fp,\" $field ($ft) \\t%d\\n\",tmp->$field);\n";

				} elsif ($ft eq "SFString") {
					push @genFuncs2, "\t\t\tspacer fprintf (fp,\" $field ($ft) \\t%s\\n\",tmp->$field->strptr);\n";

				} elsif ($ft eq "SFNode") {
					push @genFuncs2, "\t\t\tspacer fprintf (fp,\" $field ($ft):\\n\"); ";
                        		push @genFuncs2, "dump_scene(fp,level+1,tmp->$field); \n";

				} elsif (($ft eq "SFRotation") || ($ft eq "SFColorRGBA")) {
					push @genFuncs2, "\t\t\tspacer fprintf (fp,\" $field ($ft): \\t\");\n";
                        		push @genFuncs2, "\t\t\tfor (i=0; i<4; i++) { fprintf (fp,\"%4.3f  \",tmp->$field.c[i]); }\n";
					push @genFuncs2,"\t\t\tfprintf (fp,\"\\n\");\n";

				} elsif (($ft eq "SFVec4f") || ($ft eq "SFVec4d")) {
					push @genFuncs2, "\t\t\tspacer fprintf (fp,\" $field ($ft): \\t\");\n";
                        		push @genFuncs2, "\t\t\tfor (i=0; i<4; i++) { fprintf (fp,\"%4.3f  \",tmp->$field.c[i]); }\n";
					push @genFuncs2,"\t\t\tfprintf (fp,\"\\n\");\n";

				} elsif (($ft eq "SFColor") || ($ft eq "SFVec3f") || ($ft eq "SFVec3d")) {
					push @genFuncs2, "\t\t\tspacer fprintf (fp,\" $field ($ft): \\t\");\n";
                        		push @genFuncs2, "\t\t\tfor (i=0; i<3; i++) { fprintf (fp,\"%4.3f  \",tmp->$field.c[i]); }\n";
					push @genFuncs2,"\t\t\tfprintf (fp,\"\\n\");\n";

				} elsif (($ft eq "SFVec2f") || ($ft eq "SFVec2d")) {
					push @genFuncs2, "\t\t\tspacer fprintf (fp,\" $field ($ft): \\t\");\n";
                        		push @genFuncs2, "\t\t\tfor (i=0; i<2; i++) { fprintf (fp,\"%4.3f  \",tmp->$field.c[i]); }\n";
					push @genFuncs2,"\t\t\tfprintf (fp,\"\\n\");\n";

				} elsif ($ft eq "SFImage") {
					push @genFuncs2, "\t\t\tspacer fprintf (fp,\" $field ($ft): (not dumped)\\t\");\n";
					push @genFuncs2,"\t\t\tfprintf (fp,\"\\n\");\n";

				} elsif (($ft eq "SFMatrix3f") || ($ft eq "SFMatrix3d")) {
					push @genFuncs2, "\t\t\tspacer fprintf (fp,\" $field ($ft): \\t\");\n";
                        		push @genFuncs2, "\t\t\tfor (i=0; i<9; i++) { fprintf (fp,\"%4.3f  \",tmp->$field.c[i]); }\n";
					push @genFuncs2,"\t\t\tfprintf (fp,\"\\n\");\n";

				} elsif (($ft eq "SFMatrix4f") || ($ft eq "SFMatrix4d")) {
					push @genFuncs2, "\t\t\tspacer fprintf (fp,\" $field ($ft): \\t\");\n";
                        		push @genFuncs2, "\t\t\tfor (i=0; i<16; i++) { fprintf (fp,\"%4.3f  \",tmp->$field.c[i]); }\n";
					push @genFuncs2,"\t\t\tfprintf (fp,\"\\n\");\n";

				} elsif ($ft eq "MFString") {
					push @genFuncs2, "\t\t\tspacer fprintf (fp,\" $field ($ft): \\n\");\n";
                        		push @genFuncs2, "\t\t\tfor (i=0; i<tmp->$field.n; i++) { spacer fprintf (fp,\"\t\t\t%d: \\t%s\\n\",i,tmp->$field.p[i]->strptr); }\n";
				} elsif ($ft eq "MFNode") {
					push @genFuncs2, "\t\t\tspacer fprintf (fp,\" $field ($ft):\\n\");\n";
                        		push @genFuncs2, "\t\t\tfor (i=0; i<tmp->$field.n; i++) { dump_scene(fp,level+1,tmp->$field.p[i]); }\n";

				} elsif (($ft eq "MFInt32") || ($ft eq "MFBool")) {
					push @genFuncs2, "\t\t\tspacer fprintf (fp,\" $field ($ft):\\n\");\n";
                        		push @genFuncs2, "\t\t\tfor (i=0; i<tmp->$field.n; i++) { spacer fprintf (fp,\"\t\t\t%d: \\t%d\\n\",i,tmp->$field.p[i]); }\n";
				} elsif (($ft eq "MFFloat") || ($ft eq "MFTime") || ($ft eq "MFDouble")) {
					push @genFuncs2, "\t\t\tspacer fprintf (fp,\" $field ($ft):\\n\");\n";
                        		push @genFuncs2, "\t\t\tfor (i=0; i<tmp->$field.n; i++) { spacer fprintf (fp,\"\t\t\t%d: \\t%4.3f\\n\",i,tmp->$field.p[i]); }\n";
				} elsif (($ft eq "MFVec3f") || ($ft eq "MFColor") || ($ft eq "MFVec3d")) {
					push @genFuncs2, "\t\t\tspacer fprintf (fp,\" $field ($ft):\\n\");\n";
                        		push @genFuncs2, "\t\t\tfor (i=0; i<tmp->$field.n; i++) { spacer ".
						"fprintf (fp,\"\t\t\t%d: \\t[%4.3f, %4.3f, %4.3f]\\n\",i,(tmp->$field.p[i]).c[0], (tmp->$field.p[i]).c[1],(tmp->$field.p[i]).c[2]); }\n";
				} elsif (($ft eq "MFVec2f") || ($ft eq "MFVec2d")) {
					push @genFuncs2, "\t\t\tspacer fprintf (fp,\" $field ($ft):\\n\");\n";
                        		push @genFuncs2, "\t\t\tfor (i=0; i<tmp->$field.n; i++) { spacer ".
						"fprintf (fp,\"\t\t\t%d: \\t[%4.3f, %4.3f]\\n\",i,(tmp->$field.p[i]).c[0], (tmp->$field.p[i]).c[1]); }\n";

				} elsif (($ft eq "MFRotation") || ($ft eq "MFColorRGBA")) {
					push @genFuncs2, "\t\t\tspacer fprintf (fp,\" $field ($ft):\\n\");\n";
                        		push @genFuncs2, "\t\t\tfor (i=0; i<tmp->$field.n; i++) { spacer ".
						"fprintf (fp,\"\t\t\t%d: \\t[%4.3f, %4.3f, %4.3f, %4.3f]\\n\",i,(tmp->$field.p[i]).c[0], (tmp->$field.p[i]).c[1],(tmp->$field.p[i]).c[2],(tmp->$field.p[i]).c[3]); }\n";


				} elsif (($ft eq "MFVec4f") || ($ft eq "MFVec4d")) {
					push @genFuncs2, "\t\t\tspacer fprintf (fp,\" $field ($ft):\\n\");\n";
                        		push @genFuncs2, "\t\t\tfor (i=0; i<tmp->$field.n; i++) { spacer ".
						"fprintf (fp,\"\t\t\t%d: \\t[%4.3f, %4.3f, %4.3f, %4.3f]\\n\",i,(tmp->$field.p[i]).c[0], (tmp->$field.p[i]).c[1],(tmp->$field.p[i]).c[2],(tmp->$field.p[i]).c[3]); }\n";


				} elsif (($ft eq "MFMatrix4f") || ($ft eq "MFMatrix4d")) {
					push @genFuncs2, "\t\t\tspacer fprintf (fp,\" $field ($ft):\\n\");\n";
                        		push @genFuncs2, "\t\t\tfor (i=0; i<tmp->$field.n; i++) { spacer ".
						"fprintf (fp,\"\t\t\t%d: \\t[%4.3f, %4.3f, %4.3f, %4.3f, %4.3f,  %4.3f,  %4.3f,  %4.3f,  %4.3f, %4.3f, %4.3f, %4.3f, %4.3f, %4.3f,  %4.3f,  %4.3f ]\\n\",i,(tmp->$field.p[i]).c[0], (tmp->$field.p[i]).c[1],(tmp->$field.p[i]).c[2],(tmp->$field.p[i]).c[3],(tmp->$field.p[i]).c[4],(tmp->$field.p[i]).c[5],(tmp->$field.p[i]).c[6],(tmp->$field.p[i]).c[7],(tmp->$field.p[i]).c[8],(tmp->$field.p[i]).c[9],(tmp->$field.p[i]).c[10],(tmp->$field.p[i]).c[11],(tmp->$field.p[i]).c[12],(tmp->$field.p[i]).c[13],(tmp->$field.p[i]).c[14],(tmp->$field.p[i]).c[15]); }\n";

				} elsif (($ft eq "MFMatrix3f") || ($ft eq "MFMatrix3d")) {
					push @genFuncs2, "\t\t\tspacer fprintf (fp,\" $field ($ft):\\n\");\n";
                        		push @genFuncs2, "\t\t\tfor (i=0; i<tmp->$field.n; i++) { spacer ".
						"fprintf (fp,\"\t\t\t%d: \\t[%4.3f, %4.3f, %4.3f, %4.3f, %4.3f,  %4.3f,  %4.3f,  %4.3f,  %4.3f ]\\n\",i,(tmp->$field.p[i]).c[0], (tmp->$field.p[i]).c[1],(tmp->$field.p[i]).c[2],(tmp->$field.p[i]).c[3],(tmp->$field.p[i]).c[4],(tmp->$field.p[i]).c[5],(tmp->$field.p[i]).c[6],(tmp->$field.p[i]).c[7],(tmp->$field.p[i]).c[8]); }\n";



				} else {
					print "type $ft not handled yet\n";
				}
				if (($firstC eq "_") || $field eq "metadata") {
					push @genFuncs2, "\t\t    }\n";
				}
			}
		}

		push @genFuncs2, "		    break;\n\t\t}\n";
	}
	push @genFuncs2, "		default: {}\n";

	push @genFuncs2, " }\n fflush(fp) ;\n spacer fprintf (fp,\"L%d end\\n\",level);\n if (level == 0) fprintf (fp,\"ending dump_scene\\n\");\n}\n";

	#####################
	# create an array for each node. The array contains the following:
	# const int OFFSETS_Text[
	# 	FIELDNAMES_string, offsetof (struct X3D_Text, string), MFSTRING, KW_inputOutput,
	#	FIELDNAMES_fontStype, offsetof (struct X3D_Text, fontStyle, SFNODE, KW_inputOutput,
	# ....
	# 	-1, -1, -1, -1];
	# NOTES:
	# 1) we skip any field starting with an "_" (underscore)
	#
	for my $node (@sortedNodeList) {

		push @genFuncs1, "\nconst int OFFSETS_".$node."[] = {\n";

 		foreach my $field (sort keys %{$VRML::NodeType::Nodes{$node}{Defaults}}) {
		    my $ft = $VRML::NodeType::Nodes{$node}{FieldTypes}{$field};
		    #$ft =~ tr/a-z/A-Z/; # convert to uppercase
		    my $fk = $VRML::NodeType::Nodes{$node}{FieldKinds}{$field};
		    my $specVersion = $VRML::NodeType::Nodes{$node}{SpecLevel}{$field};
		    push @genFuncs1, "	(int) FIELDNAMES_$field, (int) offsetof (struct X3D_$node, $field), ".
			" (int) FIELDTYPE_$ft, (int) KW_$fk, (int) $specVersion,\n";
		};
		push @genFuncs1, "	-1, -1, -1, -1, -1};\n";
	}
	#####################
	# create an array for each node. The array contains the following:
	# const int OFFSETS_Text[
	# 	FIELDNAMES_string, offsetof (struct X3D_Text, string), MFSTRING, KW_inputOutput,
	#	FIELDNAMES_fontStype, offsetof (struct X3D_Text, fontStyle, SFNODE, KW_inputOutput,
	# ....
	# 	-1, -1, -1, -1];
	# NOTES:
	# 1) we skip any field starting with an "_" (underscore)
	#####################
	# make an array that contains all of the OFFSETS created above.
	push @str, "\nextern const int *NODE_OFFSETS[];\n";
	push @genFuncs1, "\nconst int *NODE_OFFSETS[] = {\n";
	for my $node (@sortedNodeList) {
		push @genFuncs1, "	OFFSETS_$node,\n";
	}
	push @genFuncs1, "	};\n";


	#####################
	# make the NodeFields.h for the C VRML parser.

	my @fieldNodes;
	for my $node (@sortedNodeList) {
		#print "node $_ is tagged as $nodeIntegerType\n";
		# tag each node type with a integer key.

		push @fieldNodes, "\n/* $node node */\n";
		push @fieldNodes, "BEGIN_NODE($node)\n";

 		foreach my $field (sort keys %{$VRML::NodeType::Nodes{$node}{Defaults}}) {
			if (index($field,"_") !=0) {
				my $fk = "";
				my $ofk = $VRML::NodeType::Nodes{$node}{FieldKinds}{$field};
				if ("outputOnly" eq $ofk)     {$fk = "EVENT_OUT";}
				if ("inputOnly" eq $ofk)      {$fk = "EVENT_IN";}
				if ("inputOutput" eq $ofk) {$fk = "EXPOSED_FIELD";}
				if ("initializeOnly" eq $ofk)        {$fk = "FIELD";}

				if ("" eq $fk) {
					print "error in fieldKind for node $node, was $ofk\n";
				}

				my $ft = $VRML::NodeType::Nodes{$node}{FieldTypes}{$field};
				my $origFt = "FIELDTYPE_".$VRML::NodeType::Nodes{$node}{FieldTypes}{$field};
				$ft =~ tr/A-Z/a-z/; # convert to lowercase

				push @fieldNodes, "$fk($node,$field,$ft,$field,$origFt)\n";
			}
		};
		push @fieldNodes, "END_NODE($node)\n";
	}



	#####################
	# create a function to return the X3D component for each node type
	push @str, "\nint getSAI_X3DNodeType (int FreeWRLNodeType);\n";
	push @genFuncs2, "\nint getSAI_X3DNodeType (int FreeWRLNodeType) {\n\tswitch (FreeWRLNodeType) {\n";
	for my $node (@sortedNodeList) {
		push @genFuncs2, "	case NODE_$node: return ".
		    $VRML::NodeType::Nodes{$node}{X3DNodeType}."; break;\n";
	}
	push @genFuncs2,"\tdefault:return -1;\n\t}\n}\n";


	#####################
	# Scenegraph/GeneratedCode.c
	#
	open_codegen_file(GENFUNC, "../src/lib/scenegraph/GeneratedCode.c");
	print GENFUNC '/*

  GeneratedCode.c: generated by VRMLC.pm. DO NOT MODIFY, MODIFY VRMLC.pm INSTEAD.

*/
';
	print GENFUNC @license_block;

	print GENFUNC join '',@genFuncs1;
	print GENFUNC join '',@genFuncs2;

	close GENFUNC;

	#####################
	# libeai/GeneratedCode.c
	#
	open_codegen_file(GENFUNC, "../src/libeai/GeneratedCode.c");
	print GENFUNC '/*

  GeneratedCode.c: generated by VRMLC.pm. DO NOT MODIFY, MODIFY VRMLC.pm INSTEAD.

*/
';

	print GENFUNC @license_block;

	print GENFUNC join '',@EAICommon;

	close GENFUNC;

	#####################
	# vrml_parser/NodeFields.h
	#
	open_codegen_file(FIELDNODES, "../src/lib/vrml_parser/NodeFields.h");
	print FIELDNODES '/*

  NodeFields.h: generated by VRMLC.pm. DO NOT MODIFY, MODIFY VRMLC.pm INSTEAD.

*/
';

	print FIELDNODES @license_block;

	print FIELDNODES '
/*
Information for all the fields of all the nodes.

Format is as follows:
BEGIN_NODE(NodeName)
 FIELD(Node, field, type, varToAssignInStruct)
 EXPOSED_FIELD(Node, field, type, varToAssignInStruct)
 EVENT_IN(Node, event, type, varToAssignInStruct)
 EVENT_OUT(Node, event, type, varToAssignInStruct)
END_NODE(NodeName)

*/

';

	print FIELDNODES join '',@fieldNodes;

	close FIELDNODES;

	#####################
	# vrml_parser/Structs.h
	#
	open_codegen_file(STRUCTS, "../src/lib/vrml_parser/Structs.h");
	print STRUCTS '/*

  Structs.h: generated by VRMLC.pm. DO NOT MODIFY, MODIFY VRMLC.pm INSTEAD.

  Code here comes almost verbatim from VRMLC.pm

*/
';

	print STRUCTS @license_block;

	print STRUCTS '
#ifndef __FREEWRL_STRUCTS_H__
#define __FREEWRL_STRUCTS_H__

#include <system_threads.h>
struct point_XYZ {GLDOUBLE x,y,z;};
struct orient_XYZA {GLDOUBLE x,y,z,a;};

struct X3D_Virt {
	void (*prep)(void *);
	void (*rend)(void *);
	void (*children)(void *);
	void (*fin)(void *);
	void (*rendray)(void *);
	void (*mkpolyrep)(void *);
	void (*proximity)(void *);
	void (*other)(void *);
	void (*collision)(void *);
	void (*compile)(void *, void *, void *, void *, void *);
};

/* a string is stored as a pointer, and a length of that mallocd pointer */
struct Uni_String {
	int len;
	char * strptr;
	int touched;
};

/* Internal representation of IndexedFaceSet, Text, Extrusion & ElevationGrid:
 * set of triangles.
 * done so that we get rid of concave polygons etc.
 */
struct X3D_PolyRep { /* Currently a bit wasteful, because copying */
	int irep_change;
	int ccw;	/* ccw field for single faced structures */
	int ntri; /* number of triangles */
	int streamed;	/* is this done the streaming pass? */

	/* indicies for arrays. OpenGL ES 2.0 - unsigned short for the DrawArrays call */
	GLuint *cindex;   /* triples (per triangle) */
	GLuint *colindex;   /* triples (per triangle) */
	GLuint *norindex;
        GLuint *tcindex; /* triples or null */

	float *actualCoord; /* triples (per point) */
	float *color; /* triples or null */
	float *normal; /* triples or null */
        float *GeneratedTexCoords;	/* triples (per triangle) of texture coords if there is no texCoord node */
	int tcoordtype; /* type of texture coord node - is this a NODE_TextureCoordGenerator... */
	int texgentype; /* if we do have a TextureCoordinateGenerator, what "TCGT_XXX" type is it? */
	GLfloat minVals[3];		/* for collision and default texture coord generation */
	GLfloat maxVals[3];		/* for collision and default texture coord generation */
	GLfloat transparency;		/* what the transparency value was during compile, put in color array if RGBA colors */
	int isRGBAcolorNode;		/* color was originally an RGBA, DO NOT re-write if transparency changes */
	GLuint VBO_buffers[VBO_COUNT];		/* VBO indexen */
};

/* viewer dimentions (for collision detection) */
struct sNaviInfo {
        double width;
        double height;
        double step;
};

';

	# print out the generated structures
	print STRUCTS join '',@str;

	print STRUCTS '
#endif /* __FREEWRL_STRUCTS_H__ */
';

	close STRUCTS;
}


gen();
