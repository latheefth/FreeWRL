# $Id$
#
# Copyright (C) 1998 Tuomas J. Lukka 1999 John Stewart CRC Canada
# Portions Copyright (C) 1998 Bernhard Reiter
# DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
# See the GNU Library General Public License (file COPYING in the distribution)
# for conditions of use and redistribution.

#
# $Log$
# Revision 1.247  2006/10/17 18:51:52  crc_canada
# Step 1 in getting rid of PERL parsing.
#
# Revision 1.246  2006/09/21 08:24:54  domob
# Script fields *should* be parsed correctly now.
#
# Revision 1.245  2006/09/20 20:31:34  crc_canada
# screen sensitive nodes now work properly with Daniel Kraft's parser.
#
# Revision 1.244  2006/09/19 17:06:10  domob
# Moved ARR_SIZE from GeneratedCode.c to headers.h
#
# Revision 1.243  2006/09/07 19:30:37  domob
# Routing of PROTO-events to built-in events.
#
# Revision 1.242  2006/08/29 18:59:31  crc_canada
# Sound nodes should be working again.
#
# Revision 1.241  2006/08/17 20:36:29  crc_canada
# working on Javascript from C, not perl
#
# Revision 1.240  2006/08/16 19:27:29  crc_canada
# rearrange files to logically put get and set of scenegraph functions together.
#
# Revision 1.239  2006/08/11 19:48:55  crc_canada
# Use Experimental Parser as default; more Javascript work.
#
# Revision 1.238  2006/08/03 15:32:51  crc_canada
# Javascript SF/MFnode handling changes started - no longer uses Perl for this.
#
# Revision 1.237  2006/07/10 14:30:55  crc_canada
# semantic error fixed in previous version.
#
# Revision 1.236  2006/07/10 14:24:11  crc_canada
# add keywords for PROTO interface fields.
#
# Revision 1.235  2006/07/07 15:38:16  crc_canada
# Updated package htmls, more work on Frustum culling.
#
# Revision 1.234  2006/06/22 14:05:44  crc_canada
# Bindables now handled totally in C.
#
# Revision 1.233  2006/06/19 20:37:14  crc_canada
# merge BrowserFullPath and BrowserURL
#
# Revision 1.232  2006/06/19 17:26:34  crc_canada
# handles step1 of parenturls.
#
# Revision 1.231  2006/06/15 18:23:17  crc_canada
# getNodeType for SAI
#
# Revision 1.230  2006/06/15 15:46:14  crc_canada
# now, if a field has more than 1 type, depending on node (eg, field, exposedField)
# it can be found in all relevant tables (EXPOSED_FIELD, FIELD, EVENT_IN, EVENT_OUT)
#
# Revision 1.229  2006/06/15 12:41:37  domob
# Fixed wrong filename of FieldNodes.h to correct NodeFields.h
#
# Revision 1.228  2006/06/15 11:57:05  crc_canada
# Generate NodeFields.h for Daniel Kraft
#
# Revision 1.227  2006/06/14 16:53:07  crc_canada
# add_first routine changed; no longer requires string, and is called now in
# routing code.
#
# Revision 1.226  2006/06/13 17:14:51  crc_canada
# EAI routing in C.
#
# Revision 1.225  2006/06/12 14:55:20  crc_canada
# more #defines for C code parser
#
# Revision 1.224  2006/06/09 20:09:54  crc_canada
# changes to help on 64 bit environments
#
# Revision 1.223  2006/06/07 19:27:05  crc_canada
# semantic mistake corrected.
#
# Revision 1.222  2006/06/07 16:44:18  crc_canada
# More generated tables for C parsing. (EVENT_OUT, EVENT_IN EXPOSED_FIELD, FIELD)
#
# Revision 1.221  2006/06/05 15:41:43  crc_canada
# Files for 1.17.6; fix reading scientific notation floats
#
# Revision 1.220  2006/06/02 17:19:14  crc_canada
# more C code changes, reduces the amount of perl interface calls dramatically.
#
# Revision 1.218  2006/05/31 14:52:28  crc_canada
# more changes to code for SAI.
#
# Revision 1.217  2006/05/29 17:54:12  crc_canada
# SAI changes - almost complete
#
# Revision 1.216  2006/05/24 19:29:03  crc_canada
# More VRML C Parser code
#
# Revision 1.215  2006/05/24 16:27:15  crc_canada
# more changes for VRML C parser.
#
# Revision 1.214  2006/05/23 23:46:49  crc_canada
# More generated code for VRML C parser.
#
# Revision 1.213  2006/05/23 16:15:10  crc_canada
# remove print statements, add more defines for a VRML C parser.
#
# Revision 1.212  2006/05/16 13:49:24  crc_canada
# Threading of shape loading now works. No menu buttons for it yet, though.
# It is a compile time option -DO_MULTI_OPENGL...
#
# Revision 1.211  2006/05/15 14:05:59  crc_canada
# Various fixes; CVS was down for a week. Multithreading for shape compile
# is the main one.
#
# Revision 1.210  2006/04/13 14:51:41  crc_canada
# EAI changes for SAI additions.
#
# Revision 1.209  2006/04/05 17:49:40  sdumoulin
# Universal binary build
#
# Revision 1.208  2006/03/09 20:43:33  crc_canada
# Initial Event Utilities Component work.
#
# Revision 1.207  2006/03/08 19:26:15  crc_canada
# addRemove children in javascript and EAI changed.
#
# Revision 1.206  2006/03/01 15:16:57  crc_canada
# Changed include file methodology and some Frustum work.
#
# Revision 1.205  2006/02/28 16:19:41  crc_canada
# BoundingBox
#
# Revision 1.204  2006/02/01 20:24:53  crc_canada
# MultiTexture work.
#
# Revision 1.203  2006/01/12 21:25:01  crc_canada
# More Occlusion stuff.
#
# Revision 1.202  2006/01/11 16:31:18  crc_canada
# starting Frustum Culling with Occlusion tests.
#
# Revision 1.201  2006/01/06 14:30:59  crc_canada
# OcclusionCulling starting.
#
# Revision 1.200  2005/12/22 14:50:30  crc_canada
# remove name field of X3D_Virt
#
# Revision 1.199  2005/12/22 14:20:58  crc_canada
# Group rendering problems fixed = sorting of children and alpha channels
#
# Revision 1.198  2005/12/21 18:16:40  crc_canada
# Rework Generation code.
#
# Revision 1.197  2005/12/16 18:31:25  crc_canada
# remove print debug statements
#
# Revision 1.196  2005/12/16 18:07:05  crc_canada
# rearrange perl generation
#
# Revision 1.195  2005/12/16 13:49:23  crc_canada
# updating generation functions.
#
# Revision 1.194  2005/12/15 20:42:01  crc_canada
# CoordinateInterpolator2D PositionInterpolator2D
#
# Revision 1.193  2005/12/10 20:26:18  crc_canada
# Move some functions into new "Component" files.
#
# Revision 1.192  2005/12/07 22:04:44  crc_canada
# replaceWorld functionality being added.
#
# Revision 1.191  2005/11/17 18:51:45  crc_canada
# revisit bindable nodes; add beginnings of TextureBackground
#
# Revision 1.190  2005/11/14 14:18:53  crc_canada
# Texture rework in progress...
#
# Revision 1.189  2005/11/09 14:25:19  crc_canada
# segfault fixing...
#
# Revision 1.188  2005/11/09 13:29:08  crc_canada
# TextureCoordinateGenerator nodes - first try
#
# Revision 1.187  2005/11/08 16:00:20  crc_canada
# reorg for 10.4.3 (OSX) dylib problem.
#
# Revision 1.186  2005/11/07 19:22:51  crc_canada
# OSX 10.4.3 broke FreeWRL - had to move defns out of VRMLC.pm
#
# Revision 1.185  2005/11/03 16:15:06  crc_canada
# MultiTextureTransform - textureTransforms changed considerably.
#
# Revision 1.184  2005/10/30 15:55:53  crc_canada
# Review the way nodes are identified at runtime.
#
# Revision 1.183  2005/10/29 16:24:00  crc_canada
# Polyrep rendering changes - step 1
#
# Revision 1.182  2005/10/27 13:47:23  crc_canada
# repeatS, repeatT flags were not handled correctly if same image but different flags.
# So, removed the "check image already loaded" code.
#
# Revision 1.181  2005/09/30 12:53:21  crc_canada
# Initial MultiTexture support.
#
# Revision 1.180  2005/09/29 03:01:13  crc_canada
# initial MultiTexture support
#
# Revision 1.179  2005/09/27 02:31:48  crc_canada
# cleanup of verbose code.
#
# Revision 1.178  2005/08/05 18:54:38  crc_canada
# ElevationGrid to new structure. works ok; still some minor errors.
#
# Revision 1.177  2005/08/04 14:39:38  crc_canada
# more work on moving elevationgrid to streaming polyrep structure
#
# Revision 1.176  2005/08/03 18:41:40  crc_canada
# Working on Polyrep structure.
#
# Revision 1.175  2005/08/02 13:22:44  crc_canada
# Move ElevationGrid to new face set order.
#
# Revision 1.174  2005/06/29 17:00:11  crc_canada
# EAI and X3D Triangle code added.
#
# Revision 1.173  2005/06/24 18:51:29  crc_canada
# more 64 bit compile changes.
#
# Revision 1.171  2005/06/09 14:52:49  crc_canada
# ColorRGBA nodes supported.
#
# Revision 1.170  2005/06/03 17:05:51  crc_canada
# More add/remove children from scripts/eai problems fixed. The ProdCon
# code had an invalid "complete" flag; the remove code in AddRemoveChildren
# had a pointer/contents of problem.
#
# Revision 1.169  2005/04/18 15:27:06  crc_canada
# speedup shapes that do not have textures by removing a glPushAttrib and glPopAttrib
#
# Revision 1.168  2005/04/06 16:56:08  crc_canada
# more OS X changes.
#
# Revision 1.167  2005/04/05 19:41:39  crc_canada
# some make problems fixed; this time back on Linux.
#
# Revision 1.166  2005/03/22 15:15:44  crc_canada
# more compile bugs; binary files were dinked in last upload.
#
# Revision 1.165  2005/03/22 13:25:24  crc_canada
# compile warnings reduced.
#
# Revision 1.164  2005/03/21 13:39:04  crc_canada
# change permissions, remove whitespace on file names, etc.
#
# Revision 1.163  2005/01/28 14:55:26  crc_canada
# Javascript SFImage works; Texture parsing changed to speed it up; and Cylinder side texcoords fixed.
#
# Revision 1.162  2005/01/18 20:52:33  crc_canada
# Make a ConsoleMessage that displays xmessage for running in a plugin.
#
# Revision 1.161  2005/01/16 20:55:08  crc_canada
# Various compile warnings removed; some code from Matt Ward for Alldev;
# some perl changes for generated code to get rid of warnings.
#
# Revision 1.160  2005/01/12 15:43:55  crc_canada
# TouchSensor hitPoint_changed and hitNormal_changed
#
# Revision 1.159  2004/12/01 21:19:07  crc_canada
# Anchor work.
#
# Revision 1.158  2004/10/22 19:02:25  crc_canada
# javascript work.
#
# Revision 1.157  2004/10/06 13:39:44  crc_canada
# Debian patches from Sam Hocevar.
#
# Revision 1.156  2004/09/30 20:11:55  crc_canada
# Bug fixes for EAI.
#
# Revision 1.155  2004/09/21 17:52:46  crc_canada
# make some rendering improvements.
#
# Revision 1.154  2004/09/15 19:21:18  crc_canada
# woops - problems with previous Sensitive rendering optimizations.
#
# Revision 1.153  2004/09/15 18:34:40  crc_canada
# Sensitive rendering pass now only traverses branches that have
# sensitive nodes in it. Speeds up (on my machine) tests/33.wrl from
# 14.71fps to 17.4fps.
#
# Revision 1.152  2004/09/08 18:58:58  crc_canada
# More Frustum culling work.
#
# Revision 1.151  2004/08/25 14:57:12  crc_canada
# more Frustum culling work
#
# Revision 1.150  2004/08/23 17:46:26  crc_canada
# Bulk commit: IndexedLineWidth width setting, and more Frustum culling work.
#
# Revision 1.149  2004/08/06 15:46:23  crc_canada
# if a fontStyle is a PROTO, expand the proto in NodeIntern.pm (used to
# segfault!)
#
# Revision 1.148  2004/07/21 19:04:00  crc_canada
# working on EXTERNPROTOS
#
# Revision 1.147  2004/07/16 13:17:50  crc_canada
# SFString as a Java .class script field entry.
#
# Revision 1.146  2004/07/12 13:30:36  crc_canada
# more steps to getting frustum culling working.
#
# Revision 1.145  2004/06/25 18:19:09  crc_canada
# EXTERNPROTO geturl; Solaris changes, and general changing the way URLs are
# handled.
#
# Revision 1.144  2004/06/10 20:05:52  crc_canada
# Extrusion (with concave endcaps) bug fixed; some javascript changes.
#
# Revision 1.143  2004/05/25 18:18:51  crc_canada
# more sorting of nodes
#
# Revision 1.140  2004/05/06 14:37:21  crc_canada
# more .class changes.
#
# Revision 1.138  2004/04/20 19:20:23  crc_canada
# Alberto Dubuc cleanup; java .class work.
#
# Revision 1.137  2004/04/02 21:32:42  crc_canada
# anchor work
#
# Revision 1.136  2004/03/29 20:39:54  crc_canada
# Compile for IRIX
#
# Revision 1.135  2004/03/29 19:14:14  crc_canada
# Irix compilation fixes
#
# Revision 1.134  2004/02/25 19:09:14  crc_canada
# code cleanup.
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


{
        @NodeTypes = keys %VRML::Nodes;
        # foreach my $key (keys (%{$VRML::Nodes})) {print "field $key\n";}
}


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
		 "Copyright (C) 2006 Daniel Kraft, John Stewart (CRC Canada) \n".
		 "DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED. \n".
		 "See the GNU Library General Public License (file COPYING in the distribution) \n".
		 "for conditions of use and redistribution. \n".
		"*********************************************************************/ \n".
		"\n\n\n/* GENERATED BY VRMLC.pm - do NOT change this file */\n\n" .
		"#include \"headers.h\"\n".
		"#include \"Component_Geospatial.h\"\n".
		"#include \"Polyrep.h\"\n".
		"#include \"Bindable.h\"\n\n";

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
	push @str, "\n/* Table of built-in fieldIds */\nextern const char *FIELDTYPES[];\n";
	push @str, "extern const indexT FIELDTYPES_COUNT;\n";

	push @genFuncs1, "\n/* Table of Field Types */\n       const char *FIELDTYPES[] = {\n";

	for(@VRML::Fields) {
		# print "node $_ is tagged as $fieldTypeCount\n";
		# tag each node type with a integer key.
		my $defstr = "#define FIELDTYPE_".$_."	$fieldTypeCount\n";
		push @str, $defstr;
		$fieldTypeCount ++;
		$printNodeStr = "	\"$_\",\n";
		push @genFuncs1, $printNodeStr;
	}
	push @str, "\n";
	push @genFuncs1, "};\nconst indexT FIELDTYPES_COUNT = ARR_SIZE(FIELDTYPES);\n\n";

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
			"		case NODE_$_ : {tmp = malloc (sizeof (struct X3D_$_)); break;}\n";
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
			#if (($fk ne "eventIn") && ($field ne "__parenturl")) {
			if ($fk ne "eventIn") {
				#print "		do thisfield\n";

				# do we need to initialize the occlusion number for fields?
				my $cf;
				if ($field eq "__OccludeNumber") {
					$cf = "tmp2->__OccludeNumber = newOcclude();";
				} else {
					$cf = ("VRML::Field::$ft")->cInitialize("tmp2->".$field,$def);
				}

				push @genFuncs2, "\t\t\t$cf;\n";
			}
		}
		
		push @genFuncs2,"\t\tbreak;\n\t\t}\n";
	}
	push @genFuncs2, "\t};\n";
	push @genFuncs2,
	"	\n".
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
				$ft =~ tr/a-z/A-Z/; # convert to uppercase
				my $fk = $VRML::Nodes{$node}{FieldKinds}{$field};
				push @genFuncs1, "	FIELDNAMES_$field, offsetof (struct X3D_$node, $field), ".
					" $ft, KW_$fk,\n";
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
	push @genFuncs2,"\tdefaut:return -1;\n\t}\n}\n";


	#####################
	# print out generated functions to a file
	open GENFUNC, ">CFuncs/GeneratedCode.c";
	print GENFUNC '

/* GeneratedCode.c  generated by VRMLC.pm. DO NOT MODIFY, MODIFY VRMLC.pm INSTEAD */
';

	print GENFUNC join '',@genFuncs1;
	print GENFUNC join '',@genFuncs2;


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

/* Perl linking */
#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

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

	open XS, ">VRMLFunc.xs";
	print XS '
/* VRMLFunc.c generated by VRMLC.pm. DO NOT MODIFY, MODIFY VRMLC.pm INSTEAD */

/* Code here comes almost verbatim from VRMLC.pm */

#include "CFuncs/headers.h"
#include "CFuncs/Component_Geospatial.h"
#include "CFuncs/jsUtils.h"
#include "CFuncs/Structs.h"
#include "XSUB.h"

#include <math.h>

#ifndef AQUA
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glx.h>
#else
#include <gl.h>
#include <glu.h>
#include <glext.h>
#endif

#include "CFuncs/Viewer.h"
#include "CFuncs/OpenGL_Utils.h"
#include "CFuncs/Collision.h"
#include "CFuncs/Bindable.h"
#include "CFuncs/Polyrep.h"
#include "CFuncs/sounds.h"
#include "CFuncs/SensInterps.h"

/*********************************************************************
 * Code here is generated from the hashes in VRMLC.pm and VRMLRend.pm
 */
	';

	print XS join '',@func;
	print XS join '',@vstruc;
	print XS <<'ENDHERE'


MODULE = VRML::VRMLFunc PACKAGE = VRML::VRMLFunc
PROTOTYPES: ENABLE




void *
alloc_struct(itype)
	int itype
CODE:
	void *ptr;
	ptr = createNewX3DNode(itype);
	/* printf("new Alloc: type %s -> %d\n", stringNodeType(itype), ptr);  */
	RETVAL=ptr;
OUTPUT:
	RETVAL

# return an offset, or -1 if field not found
int
get_field_offset (node, field)
	char *node
	char *field
CODE:
	int rv;
	int myNode;
	int myField;
	int coffset;
	int ctype;
	int ctmp;

	rv = -1;
	
	/* does node exist? */
	myNode = findNodeInNODES(node);	
	myField = findFieldInALLFIELDNAMES(field);
	
	/* ok so far... */
	if ((myNode>=0) && (myField >=0)) {
		/* printf ("finding %s in %s, field % %d\n",field, node, myField); */
		findFieldInOFFSETS(NODE_OFFSETS[myNode], myField, &coffset, &ctype, &ctmp);

		/* did we find this field in this node? */
		if (coffset >=0) rv = coffset;
	}


	RETVAL = rv;
OUTPUT:
	RETVAL



void
set_field_be (ptr, field, value)
	void *ptr
	char *field
	char *value
CODE:

	setField_method1 (ptr,field,value);

void
release_struct(ptr)
	void *ptr
CODE:
	struct X3D_Box *p;

	p = (struct X3D_Box *) ptr;

	if(p->_parents) free(p->_parents);
	if(p->_dlist) glDeleteLists(p->_dlist,1);
	printf ("release_struct, texture needs deletion \n");
	free(ptr); /* COULD BE MEMLEAK IF STUFF LEFT INSIDE */

void
set_sensitive(ptr,datanode,type)
	void *ptr
	void *datanode
	char *type
CODE:
	setSensitive (ptr,datanode);

#*****************************************************************************
# return a C pointer to a func for the interpolator functions. Used in CRoutes
# to enable event propagation to call the correct interpolator
#
void *
InterpPointer(x)
	char *x
CODE:
	RETVAL = returnInterpolatorPointer (x);
OUTPUT:
	RETVAL


#*******************************************************************************
# return lengths if C types - used for Routing C to C structures. Platform
# dependent sizes...
# These have to match the clength subs in VRMLFields.pm
# the value of zero (0) is used in VRMLFields.pm to indicate a "don't know".
# some, eg MultiVec3f have a value of -1, and this is just passed back again.
# (this is handled by the routing code; it's more time consuming)
# others, eg ints, have a value from VRMLFields.pm of 1, and return the
# platform dependent size.

int
getClen(x)
	int x
CODE:
	switch (x) {
		case 1:	RETVAL = sizeof (int);
			break;
		case 2:	RETVAL = sizeof (float);
			break;
		case 3:	RETVAL = sizeof (double);
			break;
		case 4:	RETVAL = sizeof (struct SFRotation);
			break;
		case 5: RETVAL = sizeof (struct SFColor);
			break;
		case 6: RETVAL = sizeof (struct SFVec2f);
			break;
		case 7: RETVAL = sizeof (struct SFColorRGBA);
			break;
		case -11: RETVAL = sizeof (unsigned int);
			break;
		default:	RETVAL = x;
	}
OUTPUT:
	RETVAL



#********************************************************************************
#
# We have a new class invocation to worry about...

int
do_newJavaClass(scriptInvocationNumber,nodeURLstr,node)
	int scriptInvocationNumber
	char *nodeURLstr
	char *node
CODE:
	RETVAL = (int) newJavaClass(scriptInvocationNumber,nodeURLstr,node);
OUTPUT:
	RETVAL


#********************************************************************************
#
# register a route that can go via C, rather than perl.
#CRoutes_Register(int adrem, unsigned int from, int fromoffset, unsigned int to_count, char *tonode_str,
#                                 int length, void *intptr, int scrdir, int extra)


void
do_CRoutes_Register(adrem, from, fromoffset, to_count, tonode_str, len, intptr, scrpt, extra)
	int adrem
	void *from
	int fromoffset
	int to_count
	char *tonode_str
	int len
	void *intptr
	int scrpt
	int extra
CODE:
	CRoutes_Register(adrem, from, fromoffset, to_count, tonode_str, len, intptr, scrpt, extra);

#********************************************************************************
#
# Free memory allocated in CRoutes_Register

void
do_CRoutes_free()
CODE:
	CRoutes_free();


#********************************************************************************
# Viewer functions implemented in C replacing viewer Perl module

unsigned int
do_get_buffer()
CODE:
	RETVAL = get_buffer();
OUTPUT:
	RETVAL

void
set_root(rn)
	unsigned long  rn
CODE:
	/* printf ("VRMLC; set_root to %d\n", rn); */
	rootNode = (void *) rn;

#********************************************************************************

# save the specific FreeWRL version number from the Config files.
void
SaveVersion(str)
	char *str
CODE:
	BrowserVersion = (char *)malloc (strlen(str)+1);
	strcpy (BrowserVersion,str);


# EAI/SAI might want to know what kind of a file was just read in.
# SAI spec - getSpecification tells us that this is a 3 (VRML) or
# 4 (X3D)...
void
SaveFileType(c)
	int c
CODE:
	currentFileVersion = c;

SV *
GetBrowserFullPath()
CODE:
	if (BrowserFullPath == NULL) {
		/* printf ("BFP null\n"); */
		RETVAL = newSVpv("",0);
	} else { 
		/* printf ("BFP %s\n",BrowserFullPath); */
		RETVAL = newSVpv(BrowserFullPath, strlen(BrowserFullPath));
	}
OUTPUT:
	RETVAL

# get the last file read in in InputFunctions.c
SV *
GetLastReadFile()
CODE:
	RETVAL = newSVpv(lastReadFile, strlen(lastReadFile));
OUTPUT:
	RETVAL


#****************JAVASCRIPT FUNCTIONS*********************************

## worry about garbage collection here ???
void
jsinit(num, sv_js)
	int num
	SV *sv_js
CODE:
	/* JAS JSInit(num,sv_js); */
	JSInit(num);

int
jsrunScript(num, script, rstr, rnum)
	int num
	char *script
	SV *rstr
	SV *rnum
CODE:
	RETVAL = JSrunScript (num, script, rstr, rnum);
OUTPUT:
RETVAL
rstr
rnum


int
jsGetProperty(num, script, rstr)
	int num
	char *script
	SV *rstr
CODE:
	RETVAL = JSGetProperty (num, script, rstr);
OUTPUT:
RETVAL
rstr

int
addSFNodeProperty(num, nodeName, name, str)
	int num
	char *nodeName
	char *name
	char *str
CODE:
	RETVAL = JSaddSFNodeProperty(num, nodeName, name, str);
OUTPUT:
RETVAL


int
addGlobalAssignProperty(num, name, str)
	int num
	char *name
	char *str
CODE:
	RETVAL = JSaddGlobalAssignProperty(num, name, str);
OUTPUT:
RETVAL


int
addGlobalECMANativeProperty(num, name)
	int num
	char *name
CODE:
	RETVAL = JSaddGlobalECMANativeProperty(num, name);
OUTPUT:
RETVAL

int
paramIndex(evin, evtype)
	char *evin
	char *evtype
CODE:
	RETVAL = JSparamIndex (evin,evtype);
OUTPUT:
RETVAL

void
initScriptFields(num,kind,type,field,fieldValue)
	int num
	char *kind
	char *type
	char *field
	char *fieldValue
CODE:
	InitScriptField(num,kind,type,field,fieldValue);
OUTPUT:

# allow Javascript to add/remove children when the parent is a USE - see JS/JS.pm.
void
jsManipulateChild(ptr, par, fiel, child)
	void *ptr
	void *par
	char *fiel
	int child
CODE:
	char onechildline[100];
	int flag;

	/* add (1), remove (2) or replace (0) */
	/* jsManipulateChild does only add or remove, not set... */
	if (strncmp(fiel,"addChild",strlen ("addChild")) == 0) {
		flag = 1;
	} else {
		flag = 2;
	}

	sprintf (onechildline, "[ %d ]",child);

	getMFNodetype (onechildline, (struct Multi_Node *) ptr,
		(struct X3D_Box *)par, flag);

# link into EAI.

int
EAIExtraMemory (type,size,data)
	char *type
	int size
	SV *data
	CODE:
	RETVAL = EAI_do_ExtraMemory (size,data,type);
OUTPUT:
RETVAL

# simple malloc - used for Java CLASS parameters
void *
malloc_this (size)
	int size
	CODE:
	RETVAL = malloc(size);
OUTPUT:
RETVAL

# print a string to the console, - this will be an xmessage
# window if we are running as a plugin.
void
ConsoleMessage (str)
	char *str;
	CODE:
	ConsoleMessage(str);


# read in a string from a file
SV *
readFile(fn,parent)
	char *fn
	char *parent
CODE:
	char *buffer;

	buffer = readInputString(fn,parent);
	RETVAL= newSVpv(buffer,strlen(buffer));
	OUTPUT:
RETVAL


#****************END JAVASCRIPT FUNCTIONS*********************************

ENDHERE
;
	print XS '#**************************START XSFN*************************';
	print XS join '',@xsfn;
	print XS '#**************************END XSFN*************************
';

	open PM, ">VRMLFunc.pm";
	print PM "
# VRMLFunc.pm, generated by VRMLC.pm. DO NOT MODIFY, MODIFY VRMLC.pm INSTEAD
package VRML::VRMLFunc;
require DynaLoader;
require Exporter;
\@ISA=qw(Exporter DynaLoader);
bootstrap VRML::VRMLFunc;
";

}


gen();


