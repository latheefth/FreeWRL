# $Id$
#
# Copyright (C) 1998 Tuomas J. Lukka 1999 John Stewart CRC Canada
# Portions Copyright (C) 1998 Bernhard Reiter
# DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
# See the GNU Library General Public License (file COPYING in the distribution)
# for conditions of use and redistribution.

#
# $Log$
# Revision 1.268  2007/03/20 20:36:10  crc_canada
# MALLOC/REALLOC macros to check mallocs for errors.
#
# Revision 1.267  2007/02/27 13:32:14  crc_canada
# initialize eventIn fields to a zero value.
#
# Revision 1.266  2007/02/22 13:41:09  crc_canada
# more ReWire work
#
# Revision 1.265  2007/02/13 22:45:24  crc_canada
# PixelTexture default image should now be ok
#
# Revision 1.264  2007/02/12 15:18:16  crc_canada
# ReWire work.
#
# Revision 1.263  2007/02/09 20:43:51  crc_canada
# More ReWire work.
#
# Revision 1.262  2007/02/08 21:49:37  crc_canada
# added Statusbar for OS X safari
#
# Revision 1.261  2007/02/07 16:03:44  crc_canada
# ReWire work
#
# Revision 1.260  2007/01/30 18:16:39  crc_canada
# PROTO transparent material; some EAI changes.
#
# Revision 1.259  2007/01/22 18:44:51  crc_canada
# EAI conversion from "Ascii" type to internal FIELDTYPE happens as close to the
# EAI interface as possible.
#
# Revision 1.258  2007/01/19 21:58:55  crc_canada
# Initial for 1.18.13 - changing internal types to simplify the numbers of changes.
#
# Revision 1.257  2007/01/18 18:06:34  crc_canada
# X3D Parser should parse Scripts now.
#
# Revision 1.256  2007/01/17 21:29:28  crc_canada
# more X3D XML parsing work.
#
# Revision 1.255  2007/01/13 18:17:10  crc_canada
# Makes ordering of fields in Uni_String same as other Multi_ fields
#
# Revision 1.254  2007/01/11 21:07:46  crc_canada
# X3D Parser work.
#
# Revision 1.253  2007/01/10 15:20:09  crc_canada
# reducing more perl code.
#
# Revision 1.252  2007/01/09 22:58:39  crc_canada
# containerField created.
#
# Revision 1.251  2006/12/21 20:51:51  crc_canada
# PROTO code added to make backlinks (parents).
#
# Revision 1.250  2006/11/22 21:50:56  crc_canada
# Modified Texture registration
#
# Revision 1.249  2006/10/19 18:28:46  crc_canada
# More changes for removing Perl from the runtime
#
# Revision 1.248  2006/10/18 20:22:43  crc_canada
# More removal of Perl code
#
# Revision 1.247  2006/10/17 18:51:52  crc_canada
# Step 1 in getting rid of PERL parsing.
#
# Revision 1.246  2006/09/21 08:24:54  domob
# Script fields *should* be parsed correctly now.
#
#

# To allow faster internal representations of nodes to be calculated,
# there is the field '_change' which can be compared between the node
# and the internal rep - if different, needs to be regenerated.
#
# the rep needs to be allocated if _intern == 0.
# XXX Freeing?!!?

require 'VRMLFields.pm';
require 'VRMLNodes.pm';
require 'VRMLRend.pm';

#######################################################################
#######################################################################
#######################################################################
#
# gen_struct - Generate a node structure, adding fields for
# internal use
my $interalNodeCommonFields = 
               "       struct X3D_Virt *v;\n"         	.
               "       int _renderFlags; /*sensitive, etc */ \n"                  	.
               "       int _sens; /*THIS is sensitive */ \n"                  	.
               "       int _hit; \n"                   	.
               "       int _change; \n"                	.
	       "       int _dlchange; \n"              	.
               "       GLuint _dlist; \n"              	.
	       "       void **_parents; \n"	  	.
	       "       int _nparents; \n"		.
	       "       int _nparalloc; \n"		.
	       "       int _ichange; \n"		.
	       "       float _dist; /*sorting for blending */ \n".
	       "       float _extent[6]; /* used for boundingboxes - +-x, +-y, +-z */ \n" .
               "       void *_intern; \n"              	.
               "       int _nodeType; /* unique integer for each type */ \n".
	       "       int _defaultContainer; /* holds the container */\n".
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

	$s .= $ExtraMem{$name};
	$s .= "};\n";
	return ($s);
}

#######################################################

sub get_rendfunc {
	my($n) = @_;
	#JAS print "RENDF $n ";
	# XXX
	my @f = qw/Prep Rend Child Fin RendRay GenPolyRep Light Changed Proximity Collision Compile/;
	my $comma = "";
	my $f = "extern struct X3D_Virt virt_${n};\n";
	my $v = "struct X3D_Virt virt_${n} = { ";

	for (@f) {
		# does this function exist?
		if (exists ${$_."C"}{$n}) {
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
			} elsif ($_ eq "Light") {
				$v .= $comma."(void *)light_".${n};
			} elsif ($_ eq "Changed") {
				$v .= $comma."(void *)changed_".${n};
			} elsif ($_ eq "Proximity") {
				$v .= $comma."(void *)proximity_".${n};
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
	#	@genFuncs1,	CFuncs/GeneratedCode.c (printed first)
	#	@genFuncs2,	CFuncs/GeneratedCode.c (printed second)
	#	@str,		CFuncs/Structs.h
	#	@nodeField,	CFuncs/NodeFields.h
	#	@EAICommon,	ReWire/GeneratedCode.c 
	#	@EAIHeader,	ReWire/GeneratedHeaders.h

	my $nodeIntegerType = 0; # make sure this maps to the same numbers in VRMLCU.pm
	my $fieldTypeCount = 0; 
	my $fieldNameCount = 0;
	my $keywordIntegerType = 0; 
	my %totalfields = ();
	my %allfields = ();
	my %allexposedFields = ();
	my %alleventInFields = ();
	my %alleventOutFields = ();


	#####################
	# for CFuncs/GeneratedCode.c - create a header.
	push @genFuncs1,
		"/******************************************************************* \n".
		 "Copyright (C) 2006, 2007 Daniel Kraft, John Stewart (CRC Canada) \n".
		 "DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED. \n".
		 "See the GNU Library General Public License (file COPYING in the distribution) \n".
		 "for conditions of use and redistribution. \n".
		"*********************************************************************/ \n".
		"\n\n\n/* GENERATED BY VRMLC.pm - do NOT change this file */\n\n" .
		"#include \"headers.h\"\n".
		"#include \"EAIheaders.h\"\n".
		"#include \"Component_Geospatial.h\"\n".
		"#include \"Polyrep.h\"\n".
		"#include \"Bindable.h\"\n\n";

	# for ReWire/GeneratedCode.c - create a header.
	push @EAICommon,
		"/******************************************************************* \n".
		 "Copyright (C) 2007 John Stewart (CRC Canada) \n".
		 "DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED. \n".
		 "See the GNU Library General Public License (file COPYING in the distribution) \n".
		 "for conditions of use and redistribution. \n".
		"*********************************************************************/ \n".
		"\n\n\n/* GENERATED BY VRMLC.pm - do NOT change this file */\n\n" .
		"#include \"Eai_C.h\"\n";

	my $st = "/* definitions to help scanning values in from a string */ \n".
		"#define SCANTONUMBER(value) while ((*value==' ') || (*value==',')) value++; \n".
		"#define SCANTOSTRING(value) while ((*value==' ') || (*value==',')) value++; \n".
		"#define SCANPASTFLOATNUMBER(value) while (isdigit(*value) \\\n".
		"		|| (*value == '.') || \\\n".
		"		(*value == 'E') || (*value == 'e') || (*value == '-')) value++; \n".
		"#define SCANPASTINTNUMBER(value) if (isdigit(*value) || (*value == '-')) value++; \\\n".
		"		while (isdigit(*value) || \\\n".
		"		(*value == 'x') || (*value == 'X') ||\\\n".
		"		((*value >='a') && (*value <='f')) || \\\n".
		"		((*value >='A') && (*value <='F')) || \\\n".
		"		(*value == '-')) value++; \n";
	push @str, $st; push @EAIHeader, $st;

	#####################
	
	push @str, "/* Data type for index into ID-table. */\ntypedef size_t indexT;\n\n";

	# now, go through nodes, and do a few things:
	#	- create a "#define NODE_Shape 300" line for CFuncs/Structs.h;
	#	- create a "case NODE_Shape: printf ("Shape")" for CFuncs/GeneratedCode.c;
	#	- create a definitive list of all fieldnames.

        my @unsortedNodeList = keys %VRML::Nodes;
        my @sortedNodeList = sort(@unsortedNodeList);
	for (@sortedNodeList) {
		#print "node $_ is tagged as $nodeIntegerType\n";
		# tag each node type with a integer key.

		push @str, "#define NODE_".$_."	$nodeIntegerType\n";
		$nodeIntegerType ++;


		#{ use Devel::Peek 'Dump'; print "start of dump\n"; Dump $VRML::Nodes{$_}{FieldKinds}, 30; print "end of dump\n"; } 

 		#foreach my $field (keys %{$VRML::Nodes{$_}{FieldKinds}}) {print "field1 $field ". $VRML::Nodes{$_}{FieldKinds}{$field}."\n";}
 		#foreach my $field (keys %{$VRML::Nodes{$_}{FieldKinds}}) {print "field2 $field ". $VRML::Nodes{$_}{Fields}."\n";}

		# capture all fields.
 		foreach my $field (keys %{$VRML::Nodes{$_}{FieldTypes}}) {
			$totalfields{$field} = "recorded";
			#print "field2 $field\n"
		};

		# now, tell what kind of field it is. Hopefully, all fields will
		# have  a valid fieldtype, if not, there is an error somewhere.
 		foreach my $field (keys %{$VRML::Nodes{$_}{FieldKinds}}) {
			my $fk = $VRML::Nodes{$_}{FieldKinds}{$field};
			if ($fk eq "field") { $allfields{$field} = $fk;}
			elsif ($fk eq "eventIn") { $alleventIns{$field} = $fk;}
			elsif ($fk eq "eventOut") { $alleventOuts{$field} = $fk;}
			elsif ($fk eq "exposedField") { $allexposedFields{$field} = $fk;}
			else {
				print "field $field fieldKind $fk is invalid\n";
			}
		};
	}
	push @str, "\n";


	#####################
	# we have a list of fields from ALL nodes. 
	push @str, "\n/* Table of built-in fieldIds */\nextern const char *FIELDNAMES[];\n";
	push @str, "extern const indexT FIELDNAMES_COUNT;\n";

	push @genFuncs1, "\n/* Table of built-in fieldIds */\n       const char *FIELDNAMES[] = {\n";

	foreach (keys %totalfields) { 
		#print "totalfields $_\n";
		push @str, "#define FIELDNAMES_".$_."	$fieldNameCount\n";
		$fieldNameCount ++;
		push @genFuncs1, "	\"$_\",\n";
	}
	push @str, "\n";
	push @genFuncs1, "};\nconst indexT FIELDNAMES_COUNT = ARR_SIZE(FIELDNAMES);\n\n";
	
	# make a function to print field name from an integer type.
	push @genFuncs2, "/* Return a pointer to a string representation of the field type */\n". 
		"const char *stringFieldType (int st) {\n".
		"	if ((st < 0) || (st >= FIELDNAMES_COUNT)) return \"FIELD OUT OF RANGE\"; \n".
		"	return FIELDNAMES[st];\n}\n\n";
	push @str, "const char *stringFieldType(int st);\n";

	#####################
	# we have a lists  of field types from ALL nodes. print out the ones without the underscores at the beginning
	push @str, "\n/* Table of built-in fieldIds */\nextern const char *EVENT_OUT[];\n";
	push @str, "extern const indexT EVENT_OUT_COUNT;\n";

	push @genFuncs1, "\n/* Table of EVENT_OUTs */\n       const char *EVENT_OUT[] = {\n";

	$nodeIntegerType = 0;
	foreach (keys %alleventOuts) { 
		if (index($_,"_") !=0) {
			push @genFuncs1, "	\"$_\",\n";
			push @str, "#define EVENT_OUT_$_	$nodeIntegerType\n";
			$nodeIntegerType ++;
		}
	}
	push @str, "\n";
	push @genFuncs1, "};\nconst indexT EVENT_OUT_COUNT = ARR_SIZE(EVENT_OUT);\n\n";
	
	push @str, "\n/* Table of built-in fieldIds */\nextern const char *EVENT_IN[];\n";
	push @str, "extern const indexT EVENT_IN_COUNT;\n";

	push @genFuncs1, "\n/* Table of EVENT_INs */\n       const char *EVENT_IN[] = {\n";

	$nodeIntegerType = 0;
	foreach (keys %alleventIns) { 
		if (index($_,"_") !=0) {
			push @genFuncs1, "	\"$_\",\n";
			push @str, "#define EVENT_IN_$_	$nodeIntegerType\n";
			$nodeIntegerType ++;
		}
	}
	push @str, "\n";
	push @genFuncs1, "};\nconst indexT EVENT_IN_COUNT = ARR_SIZE(EVENT_IN);\n\n";
	
	push @str, "\n/* Table of built-in fieldIds */\nextern const char *EXPOSED_FIELD[];\n";
	push @str, "extern const indexT EXPOSED_FIELD_COUNT;\n";

	push @genFuncs1, "\n/* Table of EXPOSED_FIELDs */\n       const char *EXPOSED_FIELD[] = {\n";

	$nodeIntegerType = 0;
	foreach (keys %allexposedFields) { 
		if (index($_,"_") !=0) {
			push @genFuncs1, "	\"$_\",\n";
			push @str, "#define EXPOSED_FIELD_$_	$nodeIntegerType\n";
			$nodeIntegerType ++;
		}
	}
	push @str, "\n";
	push @genFuncs1, "};\nconst indexT EXPOSED_FIELD_COUNT = ARR_SIZE(EXPOSED_FIELD);\n\n";
	
	push @str, "\n/* Table of built-in fieldIds */\nextern const char *FIELD[];\n";
	push @str, "extern const indexT FIELD_COUNT;\n";

	push @genFuncs1, "\n/* Table of FIELDs */\n       const char *FIELD[] = {\n";

	$nodeIntegerType = 0;
	foreach (keys %allfields) { 
		if (index($_,"_") !=0) {
			push @genFuncs1, "	\"$_\",\n";
			push @str, "#define FIELD_$_	$nodeIntegerType\n";
			$nodeIntegerType ++;
		}
	}
	push @str, "\n";
	push @genFuncs1, "};\nconst indexT FIELD_COUNT = ARR_SIZE(FIELD);\n\n";
	


	#####################
	# process keywords 
	push @str, "\n/* Table of built-in keywords */\nextern const char *KEYWORDS[];\n";
	push @str, "extern const indexT KEYWORDS_COUNT;\n";

	push @genFuncs1, "\n/* Table of keywords */\n       const char *KEYWORDS[] = {\n";

        my @sf = keys %KeywordC;
	for (@sf) {
		# print "node $_ is tagged as $nodeIntegerType\n";
		# tag each node type with a integer key.
		push @str, "#define KW_".$_."	$keywordIntegerType\n";
		$keywordIntegerType ++;
		push @genFuncs1, "	\"$_\",\n";
	}
	push @str, "\n";
	push @genFuncs1, "};\nconst indexT KEYWORDS_COUNT = ARR_SIZE(KEYWORDS);\n\n";

	# make a function to print Keyword name from an integer type.
	push @genFuncs2, "/* Return a pointer to a string representation of the keyword type */\n". 
		"const char *stringKeywordType (int st) {\n".
		"	if ((st < 0) || (st >= KEYWORDS_COUNT)) return \"KEYWORD OUT OF RANGE\"; \n".
		"	return KEYWORDS[st];\n}\n\n";
	push @str, "const char *stringKeywordType(int st);\n";


	#####################
	# process PROTO keywords 
	push @str, "\n/* Table of built-in PROTO keywords */\nextern const char *PROTOKEYWORDS[];\n";
	push @str, "extern const indexT PROTOKEYWORDS_COUNT;\n";

	push @genFuncs1, "\n/* Table of PROTO keywords */\n       const char *PROTOKEYWORDS[] = {\n";

        my @sf = keys %PROTOKeywordC;
	$keywordIntegerType = 0;
	for (@sf) {
		# print "node $_ is tagged as $nodeIntegerType\n";
		# tag each node type with a integer key.
		push @str, "#define PKW_".$_."	$keywordIntegerType\n";
		$keywordIntegerType ++;
		push @genFuncs1, "	\"$_\",\n";
	}
	push @str, "\n";
	push @genFuncs1, "};\nconst indexT PROTOKEYWORDS_COUNT = ARR_SIZE(PROTOKEYWORDS);\n\n";

	# make a function to print Keyword name from an integer type.
	push @genFuncs2, "/* Return a pointer to a string representation of the PROTO keyword type */\n". 
		"const char *stringPROTOKeywordType (int st) {\n".
		"	if ((st < 0) || (st >= PROTOKEYWORDS_COUNT)) return \"KEYWORD OUT OF RANGE\"; \n".
		"	return PROTOKEYWORDS[st];\n}\n\n";
	push @str, "const char *stringPROTOKeywordType(int st);\n";



	#####################
	# give each field an identifier
	push @str, "\n/* Table of built-in fieldIds */\nextern const char *X3DACCESSORS[];\n";
	push @str, "extern const indexT X3DACCESSORS_COUNT;\n";

	push @genFuncs1, "\n/* Table of Field Types */\n       const char *X3DACCESSORS[] = {\n";

	$fieldTypeCount = 7000;
	for(keys %X3Daccessors) {
		# print "node $_ is tagged as $fieldTypeCount\n";
		# tag each node type with a integer key.
		my $defstr = "#define X3DACCESSOR_".$_."	$fieldTypeCount\n";
		push @str, $defstr;
		$fieldTypeCount ++;
		$printNodeStr = "	\"$_\",\n";
		push @genFuncs1, $printNodeStr;
	}
	push @str, "\n";
	push @genFuncs1, "};\nconst indexT X3DACCESSORS_COUNT = ARR_SIZE(X3DACCESSORS);\n\n";

	##############################################################

	# Convert TO/FROM EAI to Internal field types. (EAI types are ascii).
		my $st = "/* convert an internal type to EAI type */\n". 
		"char mapFieldTypeToEAItype (int st) {\n".
		"	switch (st) { \n";
	push @genFuncs2, $st; push @EAICommon, $st;


	for(@VRML::Fields) {
		# print "node $_ is tagged as $fieldTypeCount\n";
		# tag each node type with a integer key.
		my $st = "\t\tcase FIELDTYPE_".$_.":	return EAI_$_;\n";
		push @genFuncs2, $st; push @EAICommon, $st;
	}
	my $st = "\t\tdefault: return -1;\n\t}\n\treturn -1;\n}\n";
	push @genFuncs2, $st; push @EAICommon, $st;
	push @str, "char mapFieldTypeToEAItype (int st);\n";


	####################
	my $st = "/* convert an EAI type to an internal type */\n". 
		"int mapEAItypeToFieldType (char st) {\n".
		"	switch (st) { \n";
	push @genFuncs2, $st; push @EAICommon, $st;


	for(@VRML::Fields) {
		# print "node $_ is tagged as $fieldTypeCount\n";
		# tag each node type with a integer key.
		my $st = "\t\tcase EAI_".$_.":	return FIELDTYPE_$_;\n";
		push @genFuncs2, $st; push @EAICommon, $st;
	}
	my $st = "\t\tdefault: return -1;\n\t}\n\treturn -1;\n}\n";
	push @genFuncs2, $st; push @EAICommon, $st;
	push @str, "int mapEAItypeToFieldType (char st);\n";

	####################
	push @genFuncs2, "/* convert an MF type to an SF type */\n". 
		"int convertToSFType (int st) {\n".
		"	switch (st) { \n";


	for(@VRML::Fields) {
		# print "node $_ is tagged as $fieldTypeCount\n";
		# tag each node type with a integer key.
		my $sftype = $_;
		$sftype =~ s/MF/SF/;

		push @genFuncs2,  "\t\tcase FIELDTYPE_$_:	return FIELDTYPE_$sftype;\n";
	}
	push @genFuncs2, 	"	return -1;;\n}\n}\n";
	push @str, "int convertToSFType (int st);\n";





	#####################
	# give each field an identifier
	push @str, "\n/* Table of built-in fieldIds */\nextern const char *FIELDTYPES[];\n";
	push @str, "extern const indexT FIELDTYPES_COUNT;\n";

	my $ts = "\n/* Table of Field Types */\n       const char *FIELDTYPES[] = {\n";
	push @genFuncs1, $ts;
	push @EAICommon, $ts;

	$fieldTypeCount = 0;
	for(@VRML::Fields) {
		# print "node $_ is tagged as $fieldTypeCount\n";
		# tag each node type with a integer key.
		my $defstr = "#define FIELDTYPE_".$_."	$fieldTypeCount\n";
		push @str, $defstr; push @EAIHeader, $defstr;
		$fieldTypeCount ++;
		$printNodeStr = "	\"$_\",\n";
		push @genFuncs1, $printNodeStr;
		push @EAICommon, $printNodeStr;
	}
	push @str, "\n";
	my $ts = "};\nconst indexT FIELDTYPES_COUNT = ARR_SIZE(FIELDTYPES);\n\n";
	push @genFuncs1, $ts;
	push @EAICommon, $ts;

	for(@VRML::Fields) {
		push @str, ("VRML::Field::$_")->cstruct . "\n";
	}
	# make a function to print fieldtype name from an integer type.
	push @genFuncs2, "/* Return a pointer to a string representation of the fieldtype type */\n". 
		"const char *stringFieldtypeType (int st) {\n".
		"	if ((st < 0) || (st >= FIELDTYPES_COUNT)) return \"FIELDTYPE OUT OF RANGE\"; \n".
		"	return FIELDTYPES[st];\n}\n\n";
	push @str, "const char *stringFieldtypeType(int st);\n";



	#####################
	# handle the nodes themselves
	push @str, "\n/* Table of built-in nodeIds */\nextern const char *NODES[];\n";
	push @str, "extern const indexT NODES_COUNT;\n";

	push @genFuncs1, "\n/* Table of Node Types */\n       const char *NODES[] = {\n";

        push @str, "\n/* and now the structs for the nodetypes */ \n";
	for(@sortedNodeList) {
		$printNodeStr = "	\"$_\",\n";
		push @genFuncs1, $printNodeStr;
	}
	push @genFuncs1, "};\nconst indexT NODES_COUNT = ARR_SIZE(NODES);\n\n";

	# make a function to print node name from an integer type.
	push @genFuncs2, "/* Return a pointer to a string representation of the node type */\n". 
		"const char *stringNodeType (int st) {\n".
		"	if ((st < 0) || (st >= NODES_COUNT)) return \"NODE OUT OF RANGE\"; \n".
		"	return NODES[st];\n}\n\n";
	push @str, "const char *stringNodeType(int st);\n";


	###################
	# create the virtual tables for each node.
	push @str, "\n/* First, a generic struct, contains only the common elements */\n".
	"struct X3D_Node {\n". $interalNodeCommonFields .  "};\n".
	"#define X3D_NODE(node) ((struct X3D_Node*)node)\n".
	"#define X3D_GROUP(node) ((struct X3D_Group*)node)\n".
	"#define X3D_SCRIPT(node) ((struct X3D_Script*)node)\n".
	"\n/* now, generated structures for each VRML/X3D Node*/\n";


	push @genFuncs1, "/* Virtual tables for each node */\n";
	for(@sortedNodeList) {
		# print "working on node $_\n";
		my $no = $VRML::Nodes{$_};
		my($strret) = gen_struct($_, $no);
		push @str, $strret;
		my($externdeclare, $vstru) = get_rendfunc($_);
		push @str, $externdeclare;
		push @genFuncs1, $vstru;
	}
	push @genFuncs1, "\n";


	#####################
	# create a routine to create a new node type

	push @genFuncs2,
	"/* create a new node of type. This can be generated by Perl code, much as the Structs.h is */\n".
	"void *createNewX3DNode (int nt) {\n".
	"	void * tmp;\n".
	"	struct X3D_Box *node;\n".
	"\n".
	"	tmp = NULL;\n".
	"\n".
	"	switch (nt) {\n";

	for (@sortedNodeList) {
		push @genFuncs2,
			"		case NODE_$_ : {tmp = MALLOC (sizeof (struct X3D_$_)); break;}\n";
	}
	push @genFuncs2, "		default: {printf (\"createNewX3DNode = unknown type %d, this will fail\\n\",nt); return NULL;}\n";
	
	push @genFuncs2,
	"	}\n\n".
	"	/* now, fill in the node to DEFAULT values This mimics \"alloc_struct\" in the Perl code. */\n".
	"	/* the common stuff between all nodes. We'll use a X3D_Box struct, just because. It is used\n".
	"	   in this way throughought the code */\n".
	"	node = (struct X3D_Box *) tmp;\n".
	"	node->_renderFlags = 0; /*sensitive, etc */\n".
	"	node->_sens = FALSE; /*THIS is sensitive */\n".
	"	node->_hit = 0;\n".
	"	node->_change = 153; \n".
	"	node->_dlchange = 0;\n".
	"	node->_dlist = 0;\n".
	"	node->_parents = 0;\n".
	"	node->_nparents = 0;\n".
	"	node->_nparalloc = 0;\n".
	"	node->_ichange = 0;\n".
	"	node->_dist = -10000.0; /*sorting for blending */\n".
	"	INITIALIZE_EXTENT\n".
	"	node->_intern = 0;\n".
	"	node->_nodeType = nt; /* unique integer for each type */\n".
	"	\n";


	push @genFuncs2,
	"	/* now, fill in the node specific stuff here. the defaults are in VRMLNodes.pm */\n".
	"	switch (nt) {\n";

	for my $node (@sortedNodeList) {
		push @genFuncs2,
			"\t\tcase NODE_$node : {\n\t\t\tstruct X3D_$node * tmp2;\n";
		push @genFuncs2,
			"\t\t\ttmp2 = (struct X3D_$node *) tmp;\n\t\t\ttmp2->v = &virt_$node;\n";


		#print "\nnode $node:\n";
 		foreach my $field (keys %{$VRML::Nodes{$node}{Defaults}}) {
			my $ft = $VRML::Nodes{$node}{FieldTypes}{$field};
			my $fk = $VRML::Nodes{$node}{FieldKinds}{$field};
			my $def = $VRML::Nodes{$node}{Defaults}{$field};
			#print "	fieldX $field \n";
			#print "		fieldDefaults ". $VRML::Nodes{$node}{Defaults}{$field}."\n";
			#print "		fieldKinds ". $VRML::Nodes{$node}{FieldKinds}{$field}."\n";
			#print "		fieldTypes ". $VRML::Nodes{$node}{FieldTypes}{$field}."\n";
	#		if ($fk ne "eventIn") {
				#print "		do thisfield\n";

				# do we need to initialize the occlusion number for fields?
				my $cf;
				if ($field eq "__OccludeNumber") {
					$cf = "tmp2->__OccludeNumber = newOcclude();";
				} else {
					$cf = ("VRML::Field::$ft")->cInitialize("tmp2->".$field,$def);
				}

				push @genFuncs2, "\t\t\t$cf;\n";
	#		}
		}

	# rig in the default container for X3D parsing.
	if (exists $defaultContainerType{$node}) {
		#print "node $node, defaultContainer is " . $defaultContainerType{$node}."\n";
		push @genFuncs2, "\t\t\ttmp2->_defaultContainer = FIELDNAMES_".$defaultContainerType{$node}.";\n";
	} else {
		print "defaultContainerType for $node missing\n";
	}
		
		push @genFuncs2,"\t\tbreak;\n\t\t}\n";
	}
	push @genFuncs2, "\t};\n";
	push @genFuncs2,
	"	\n".
	"	/* is this possibly the text node for the statusbar?? */ \n".
	"	if (nt == NODE_Text) lastTextNode = (struct X3D_Text *) tmp; \n".
	"	/* is this a ReWire node?? */ \n".
	"	registerReWireNode(tmp); \n".
	"	/* is this a texture holding node? */\n".
	"	registerTexture(tmp);\n".
	"	/* is this a bindable node? */\n".
	"	registerBindable(tmp);\n".
	"	/* is this a time tick node? */\n".
	"	add_first(tmp);\n";


	push @genFuncs2, "\treturn tmp;\n}\n";

	#####################
	# create an array for each node. The array contains the following:
	# const int OFFSETS_Text[
	# 	FIELDNAMES_string, offsetof (struct X3D_Text, string), MFSTRING, KW_exposedField,
	#	FIELDNAMES_fontStype, offsetof (struct X3D_Text, fontStyle, SFNODE, KW_exposedField,
	# ....
	# 	-1, -1, -1, -1];
	# NOTES:
	# 1) we skip any field starting with an "_" (underscore)
	# 2) addChildren, removeChildren, map to children.
	# 
	for my $node (@sortedNodeList) {
		#print "node $_ is tagged as $nodeIntegerType\n";
		# tag each node type with a integer key.

		push @genFuncs1, "\nconst int OFFSETS_".$node."[] = {\n";

 		foreach my $field (keys %{$VRML::Nodes{$node}{Defaults}}) {
			#if (index($field,"_") !=0) {
				my $ft = $VRML::Nodes{$node}{FieldTypes}{$field};
				#$ft =~ tr/a-z/A-Z/; # convert to uppercase
				my $fk = $VRML::Nodes{$node}{FieldKinds}{$field};
				push @genFuncs1, "	FIELDNAMES_$field, offsetof (struct X3D_$node, $field), ".
					" FIELDTYPE_$ft, KW_$fk,\n";
			#}
		};
		push @genFuncs1, "	-1, -1, -1, -1};\n";
	}

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

	
	for my $node (@sortedNodeList) {
		#print "node $_ is tagged as $nodeIntegerType\n";
		# tag each node type with a integer key.

		push @fieldNodes, "\n/* $node node */\n";
		push @fieldNodes, "BEGIN_NODE($node)\n";

 		foreach my $field (keys %{$VRML::Nodes{$node}{Defaults}}) {
			if (index($field,"_") !=0) {
				my $fk = $VRML::Nodes{$node}{FieldKinds}{$field};
				if ("eventOut" eq $fk)     {$fk = "EVENT_OUT";}
				if ("eventIn" eq $fk)      {$fk = "EVENT_IN";}
				if ("exposedField" eq $fk) {$fk = "EXPOSED_FIELD";}
				if ("field" eq $fk)        {$fk = "FIELD";}

				my $ft = $VRML::Nodes{$node}{FieldTypes}{$field};
				$ft =~ tr/A-Z/a-z/; # convert to lowercase
				push @fieldNodes, "$fk($node,$field,$ft,$field)\n";
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
				$VRML::Nodes{$node}{X3DNodeType}."; break;\n";
	}
	push @genFuncs2,"\tdefault:return -1;\n\t}\n}\n";


	#####################
	# print out generated functions to a file
	open GENFUNC, ">CFuncs/GeneratedCode.c";
	print GENFUNC '

/* GeneratedCode.c  generated by VRMLC.pm. DO NOT MODIFY, MODIFY VRMLC.pm INSTEAD */
';

	print GENFUNC join '',@genFuncs1;
	print GENFUNC join '',@genFuncs2;

	#####################
	# print out generated functions to a file
	open GENFUNC, ">ReWire/GeneratedHeaders.h";
	print GENFUNC '

/* GeneratedCode.c  generated by VRMLC.pm. DO NOT MODIFY, MODIFY VRMLC.pm INSTEAD */
';

	print GENFUNC join '',@EAIHeader;

	#####################
	# print out generated functions to a file
	open GENFUNC, ">ReWire/GeneratedCode.c";
	print GENFUNC '

/* GeneratedCode.c  generated by VRMLC.pm. DO NOT MODIFY, MODIFY VRMLC.pm INSTEAD */
';

	print GENFUNC join '',@EAICommon;

	#####################

	open FIELDNODES, ">CFuncs/NodeFields.h";
	print FIELDNODES '

/* NodeFields.h  generated by VRMLC.pm. DO NOT MODIFY, MODIFY VRMLC.pm INSTEAD */

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


	# print out structures to a file
	open STRUCTS, ">CFuncs/Structs.h";
	print STRUCTS '
/* Structs.h generated by VRMLC.pm. DO NOT MODIFY, MODIFY VRMLC.pm INSTEAD */

/* Code here comes almost verbatim from VRMLC.pm */

#ifndef STRUCTSH
#define STRUCTSH

/* OS X gives us a compile warning - hopefully this will cure it for all time */
#ifdef AQUA
//struct tm {};
#endif

/* for time tick calculations */
#include <sys/time.h>

struct pt {GLdouble x,y,z;};
struct orient {GLdouble x,y,z,a;};

struct X3D_Virt {
	void (*prep)(void *);
	void (*rend)(void *);
	void (*children)(void *);
	void (*fin)(void *);
	void (*rendray)(void *);
	void (*mkpolyrep)(void *);
	void (*light)(void *);
	void (*changed)(void *);
	void (*proximity)(void *);
	void (*collision)(void *);
	void (*compile)(void *);
	/* char *name; */
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
	int _change;
	int ccw;	/* ccw field for single faced structures */
	int ntri; /* number of triangles */
	int streamed;	/* is this done the streaming pass? */
	int alloc_tri; /* number of allocated triangles */
	int *cindex;   /* triples (per triangle) */
	float *coord; /* triples (per point) */
	int *colindex;   /* triples (per triangle) */
	float *color; /* triples or null */
	int *norindex;
	float *normal; /* triples or null */
        int *tcindex; /* triples or null */
        float *GeneratedTexCoords;	/* triples (per triangle) of texture coords if there is no texCoord node */
	int tcoordtype; /* type of texture coord node - is this a NODE_TextureCoordGenerator... */
};

/* viewer dimentions (for collision detection) */
struct sNaviInfo {
        double width;
        double height;
        double step;
};

';



	# print out the generated structures
	print STRUCTS join '',@NODEDEFS;
	print STRUCTS join '',@str;

	print STRUCTS '
#endif /* ifndef */
';

}


gen();


