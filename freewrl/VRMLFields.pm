#
# $Id$
#
# Copyright (C) 1998 Tuomas J. Lukka 1999 John Stewart CRC Canada
# DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
# See the GNU Library General Public License (file COPYING in the distribution)
# for conditions of use and redistribution.
#
# Field types, parsing and printing, Perl, C and Java.
#
# SFNode is in Parse.pm
#
# $Log$
# Revision 1.42  2005/01/16 20:55:08  crc_canada
# Various compile warnings removed; some code from Matt Ward for Alldev;
# some perl changes for generated code to get rid of warnings.
#
# Revision 1.41  2004/12/07 15:05:26  crc_canada
# Various changes; eat comma before SFColors for Rasmol; Anchor work,
# and general configuration changes.
#
# Revision 1.40  2004/08/25 14:56:18  crc_canada
# handle ISO extended characters within SFString
#
# Revision 1.39  2004/07/16 13:17:50  crc_canada
# SFString as a Java .class script field entry.
#
# Revision 1.38  2004/05/27 15:22:04  crc_canada
# javascripting problems fixed.
#
# Revision 1.37  2004/05/20 19:01:34  crc_canada
# pre4 files.
#
# Revision 1.36  2003/12/04 18:33:57  crc_canada
# Basic threading ok
#
# Revision 1.35  2003/09/16 14:57:24  crc_canada
# EAI in C updates for Sept 15 2003
#
# Revision 1.34  2003/06/12 19:08:56  crc_canada
# more work on getting javascript routing into C
#
# Revision 1.33  2003/06/02 18:21:40  crc_canada
# more work on CRoutes for Scripting
#
# Revision 1.32  2003/05/28 14:18:35  crc_canada
# Scripts moved to CRoute structure
#
# Revision 1.31  2003/05/08 16:01:33  crc_canada
# Moving code to C
#
# Revision 1.30  2003/04/25 19:43:13  crc_canada
# changed SFTime to double from float - float was not enough precision
# to hold time since epoch values.
#
# Revision 1.29  2003/03/20 18:37:22  ayla
#
# Added init() functions to be used by VRML::NodeIntern::do_defaults() to
# supply eventOuts with default values (used by Script, PROTO definitions).
#
# Revision 1.28  2003/01/28 18:23:59  ayla
# Problem with checking for undefined values in VRML::Field::Multi::as_string fixed.
#
# Revision 1.27  2002/11/29 17:07:10  ayla
#
# Parser was choking on escaped double quotes inside strings.  This should
# make it better.
#
# Revision 1.26  2002/11/28 20:15:41  crc_canada
# For 0.37, PixelTextures are handled in the same fashion as other static images
#
# Revision 1.25  2002/11/25 16:55:27  ayla
#
# Tweaked float/double formatting to strings and made changes to property
# setting for SFNodes.
#
# Revision 1.24  2002/11/22 22:09:14  ayla
# Tweaking SFFloat and SFTime formatting in as_string.
#
# Revision 1.23  2002/11/22 16:28:57  ayla
#
# Format floating point numbers for string conversion.
#
# Revision 1.22  2002/11/20 21:33:55  ayla
#
# Modified as_string functions as needed, commented unused js functions.
#
# Revision 1.21  2002/09/19 19:39:35  crc_canada
# some changes brought about by much EAI work.
#
# Revision 1.20  2002/07/11 16:21:42  crc_canada
# ImageTexture fields changed to read in textures by C rather than Perl
#
# Revision 1.19  2002/06/21 19:51:54  crc_canada
# Comma handling in number strings is much better now; previous code would
# not handle commas when preceeded by a space.
#
# Revision 1.18  2002/06/17 16:39:19  ayla
#
# Fixed VRML::DEF copy problem in package VRML::Field.
#
# Revision 1.17  2002/05/22 21:47:52  ayla
#
# Broke Scene.pm into files containing constituent packages for sanity's sake.
#
# Revision 1.16  2002/05/01 15:12:24  crc_canada
# add __texture field to store OpenGLs bound texture number
#
# Revision 1.15  2002/01/12 15:44:18  hoenicke
# Removed the Java SAI stuff from here.  It moved to VRMLJava.pm
# and java/classes/vrml/genfields.pl
#
# Revision 1.14  2001/11/28 08:34:30  ayla
#
# Tweaked regexp processing for VRML::Field::SFString::parse due to a submitted
# bug report.  It appeared that perl crashed with a segmentation fault while
# attempting to process the regexp for an SFString.  This process has
# been optimized.  Hopefully it won't break anything!
#
# Revision 1.13  2001/08/16 16:55:54  crc_canada
# Viewpoint work
#
# Revision 1.12  2001/07/11 20:43:05  ayla
#
#
# Fixed problem with Plugin/Source/npfreewrl.c, so all debugging info. is turned
# off.
#
# Committing merge between NetscapeIntegration and the trunk.
#
# Revision 1.11  2001/06/18 17:25:11  crc_canada
# IRIX compile warnings removed.
#
#
#
# Merging recent changes from the freewrl trunk to this branch.
#
# Revision 1.11  2001/06/18 17:25:11  crc_canada
# IRIX compile warnings removed.
#
# Revision 1.10  2001/05/30 19:07:44  ayla
#
# Ref. bug id #426972: sfvec3f values don't take commas, submitted by J. Stewart (crc_canada).
# Altered the code used to read in SFColor, SFVec2f and SFRotation floating point numbers.
#
# Revision 1.9  2001/05/25 21:34:00  ayla
#
#
# The regexp used to identify and capture strings was altered to follow the VRML97
# specification more closely.
# The function parse in package VRML::Field::Multi was altered to handle the occurrence
# of multiple commas in VRML fields (as in the NIST test file Appearance/FontStyle/default.wrl).
#
# Revision 1.8  2001/03/23 16:02:11  crc_canada
# unknown, unrecorded changes.
#
# Revision 1.7  2000/12/13 14:41:57  crc_canada
# Bug hunting.
#
# Revision 1.6  2000/12/01 02:01:39  crc_canada
# More SAI work
#
# Revision 1.5  2000/11/29 18:45:19  crc_canada
# Format changes, and trying to get more SAI node types working.
#
# Revision 1.4  2000/11/15 13:51:25  crc_canada
# First attempt to work again on SAI
#
# Revision 1.3  2000/08/13 14:26:46  rcoscali
# Add a js constructor for SFImage (need some work to do)
# Fixed default SFImage JS statement
#
# Revision 1.2  2000/08/07 08:28:35  rcoscali
# Removed reference to Image::Xpm (not usefull anymore for PixelTexture)
#
#

# XXX Decide what's the forward assertion..

@VRML::Fields = qw/
	SFFloat
	MFFloat
	SFRotation
	MFRotation
	SFVec3f
	MFVec3f
	SFBool
	SFInt32
	MFInt32
	SFNode
	MFNode
	SFColor
	MFColor
	SFTime
	SFString
	MFString
	SFVec2f
	MFVec2f
	SFImage
/;

###########################################################
package VRML::Field;
VRML::Error->import();

sub es {
	$p = (pos $_[1]) - 20;
	return substr $_[1],$p,40;
	
}

# The C type interface for the field type, encapsulated
# By encapsulating things well enough, we'll be able to completely
# change the interface later, e.g. to fit together with javascript etc.
sub ctype ($) {die "VRML::Field::ctype - abstract function called"}
sub clength ($) {0} #for C routes. Keep in sync with getClen in VRMLC.pm. 
sub calloc ($$) {""}
sub cassign ($$) {"$_[1] = $_[2];/*cassign*/"}
sub cfree ($) {if($_[0]->calloc) {return "free($_[1]);"} return ""}
sub cget {if(!defined $_[2]) {return "$_[1] /*cget*/"}
	else {die "If CGet with indices, abstract must be overridden"} }
sub cstruct () {"/*cstruct*/"}
sub cfunc {die("Must overload cfunc")}

sub copy {
	my($type, $value) = @_;

	if (!ref $value) {
		return $value 
	} elsif (ref $value eq "VRML::USE") {
		#$value = $value->real_node(); return $value;
		my $ret = $value->copy();
		return $ret;
	} elsif (ref $value eq "ARRAY") {
		return [map {copy("",$_)} @$value]
	} elsif (ref $value eq "VRML::NodeIntern") {
		return $value;
	} elsif (ref $value eq "VRML::DEF") {
		##my $ret = [map {copy("",$_)} @$value];
		##bless $ret, "VRML::DEF";
		##return $ret;
		my $ret = bless { map {copy("",$_)} %$value }, "VRML::DEF";
		return $ret;
	} else {
		print "going to die;\n";
		print "copy ", ref $value, "\n";
		die("Can't copy this");
	}
}

sub as_string {"VRML::Field - can't print this!"}


###########################################################
package VRML::Field::SFFloat;
@ISA=VRML::Field;
VRML::Error->import();

sub init { return 0.0; }

sub parse {
	my($type,$p,$s,$n) = @_;
	$_[2] =~ /\G\s*($Float)/ogcs or 
		parsefail($_[2], "didn't match SFFloat");
	return $1;
}

sub as_string { return sprintf("%.4g", $_[1]); }

sub print {print $_[1]}

sub ctype {"float $_[1]"}
sub clength {2} #for C routes. Keep in sync with getClen in VRMLC.pm. 
sub cfunc {"$_[1] = SvNV($_[2]); /*aa*/\n"}


###########################################################
package VRML::Field::SFTime;
@ISA=VRML::Field::SFFloat;

sub as_string { return sprintf("%f", $_[1]); }
sub ctype {"double $_[1]"}
sub clength {3} #for C routes. Keep in sync with getClen in VRMLC.pm.

###########################################################
package VRML::Field::SFInt32;
@ISA=VRML::Field;
VRML::Error->import;

sub init { return 0; }

sub parse {
	my($type,$p,$s,$n) = @_;
	$_[2] =~ /\G\s*($Integer)\b/ogsc 
		or parsefail($_[2],"not proper SFInt32");
	return $1;
}

sub print {print " $_[1] "}
sub as_string {$_[1]}

sub ctype {return "int $_[1]"}
sub clength {1} #for C routes. Keep in sync with getClen in VRMLC.pm. 
sub cfunc {return "$_[1] = SvIV($_[2]);/*bb*/\n"}


###########################################################
package VRML::Field::SFColor;
@ISA=VRML::Field;
VRML::Error->import;

sub init { return [0, 0, 0]; }

sub parse {
	my($type,$p) = @_;

	# eat comma if one is there (thanks, rasmol, for invalid VRML...)
	$_[2] =~ /\G\s*,\s*/gsc;

	$_[2] =~ /\G\s{0,}($Float)\s{0,},{0,1}\s{0,}($Float)\s{0,},{0,1}\s{0,}($Float)/ogsc
	    or parsefail($_[2],"Didn't match SFColor");

	return [$1,$2,$3];
}

sub print {print join ' ',@{$_[1]}}
sub as_string {join ' ',@{$_[1]}}

sub cstruct {return "struct SFColor {
	float c[3]; };"}
sub ctype {return "struct SFColor $_[1]"}
sub clength {5} #for C routes. Keep in sync with getClen in VRMLC.pm. 
sub cget {return "($_[1].c[$_[2]])"}

sub cfunc {
	return "{
		AV *a;
		SV **b;
		int i;
		if(!SvROK($_[2])) {
			$_[1].c[0] = 0;
			$_[1].c[1] = 0;
			$_[1].c[2] = 0;
			/* die(\"Help! SFColor without being ref\"); */
		} else {
			if(SvTYPE(SvRV($_[2])) != SVt_PVAV) {
				freewrlDie(\"Help! SFColor without being arrayref\");
			}
			a = (AV *) SvRV($_[2]);
			for(i=0; i<3; i++) {
				b = av_fetch(a, i, 1); /* LVal for easiness */
				if(!b) {
					freewrlDie(\"Help: SFColor b == 0\");
				}
				$_[1].c[i] = SvNV(*b);
			}
		}
	} /*cc*/
	"
}


###########################################################
package VRML::Field::SFVec3f;
@ISA=VRML::Field::SFColor;
sub cstruct {return ""}

sub vec_add { join '',map {"$_[3].c[$_] = $_[1].c[$_] + $_[2].c[$_];"} 0..2; }
sub vec_subtract { join '',map {"$_[3].c[$_] = $_[1].c[$_] - $_[2].c[$_];"} 0..2; }
sub vec_negate { join '',map {"$_[2].c[$_] = -$_[1].c[$_];"} 0..2; }
sub vec_length { "$_[2] = sqrt(".(join '+',map {"$_[1].c[$_]*$_[1].c[$_]"} 0..2)
	.");"; }
sub vec_normalize { "{double xx = sqrt(".(join '+',map {"$_[1].c[$_]*$_[1].c[$_]"} 0..2)
	.");
	". (join '', map {"$_[2].c[$_] = $_[1].c[$_]/xx;"} 0..2)."}" }

sub vec_cross {
	"
		$_[3].c[0] = 
			$_[1].c[1] * $_[2].c[2] - 
			$_[2].c[1] * $_[1].c[2];
		$_[3].c[1] = 
			$_[1].c[2] * $_[2].c[0] - 
			$_[2].c[2] * $_[1].c[0];
		$_[3].c[2] = 
			$_[1].c[0] * $_[2].c[1] - 
			$_[2].c[0] * $_[1].c[1];
	"
}


###########################################################
package VRML::Field::SFVec2f;
@ISA=VRML::Field;
VRML::Error->import();

sub init { return [0, 0]; }

sub parse {
	my($type,$p) = @_;

	$_[2] =~ /\G\s{0,}($Float)\s{0,},{0,1}\s{0,}($Float)/ogsc
	    or parsefail($_[2],"didn't match SFVec2f");
	return [$1,$2];
}

sub print {print join ' ',@{$_[1]}}
sub as_string {join ' ',@{$_[1]}}

sub cstruct {return "struct SFVec2f {
	float c[2]; };"}
sub ctype {return "struct SFVec2f $_[1]"}
sub clength {6} #for C routes. Keep in sync with getClen in VRMLC.pm. 
sub cget {return "($_[1].c[$_[2]])"}

sub cfunc {
	return "{
		AV *a;
		SV **b;
		int i;
		if(!SvROK($_[2])) {
			$_[1].c[0] = 0;
			$_[1].c[1] = 0;
			/* die(\"Help! SFVec2f without being ref\"); */
		} else {
			if(SvTYPE(SvRV($_[2])) != SVt_PVAV) {
				freewrlDie(\"Help! SFVec2f without being arrayref\");
			}
			a = (AV *) SvRV($_[2]);
			for(i=0; i<2; i++) {
				b = av_fetch(a, i, 1); /* LVal for easiness */
				if(!b) {
					freewrlDie(\"Help: SFColor b == 0\");
				}
				$_[1].c[i] = SvNV(*b);
			}
		}
	} /*dd*/
	"
}


###########################################################
package VRML::Field::SFRotation;
@ISA=VRML::Field;
VRML::Error->import();

sub init { return [0, 0, 1, 0]; }

sub parse {
	my($type,$p) = @_;

	# first, look for 4 floats, can have commas
	$_[2] =~ /\G\s{0,}($Float)\s{0,},{0,1}\s{0,}($Float)\s{0,},{0,1}\s{0,}($Float)\s{0,},{0,1}\s{0,}($Float)/ogsc
		or VRML::Error::parsefail($_[2],"not proper rotation");
	return [$1,$2,$3,$4];
}

sub print {print join ' ',@{$_[1]}}
sub as_string {join ' ',@{$_[1]}}

sub cstruct {return "struct SFRotation {
 	float r[4]; };"}

sub rot_invert {
	"
	 $_[2].r[0] = $_[1].r[0];
	 $_[2].r[1] = $_[1].r[1];
	 $_[2].r[2] = $_[1].r[2];
	 $_[2].r[3] = -$_[1].r[3];
	"
}

$VRML::Field::avecmacros = "
#define AVECLEN(x) (sqrt((x)[0]*(x)[0]+(x)[1]*(x)[1]+(x)[2]*(x)[2]))
#define AVECPT(x,y) ((x)[0]*(y)[0]+(x)[1]*(y)[1]+(x)[2]*(y)[2])
#define AVECCP(x,y,z)   (z)[0]=(x)[1]*(y)[2]-(x)[2]*(y)[1]; \\
			(z)[1]=(x)[2]*(y)[0]-(x)[0]*(y)[2]; \\
			(z)[2]=(x)[0]*(y)[1]-(x)[1]*(y)[0];
#define AVECSCALE(x,y) x[0] *= y; x[1] *= y; x[2] *= y;
";

sub rot_multvec {
	qq~
		double rl = AVECLEN($_[1].r);
		double vl = AVECLEN($_[2].c);
		float c1[3];
		float c2[3];
		double s = sin($_[1].r[3]), c = cos($_[1].r[3]);
		AVECCP($_[1].r,$_[2].c,c1); AVECSCALE(c1, 1.0 / rl );
		AVECCP($_[1].r,c1,c2); AVECSCALE(c2, 1.0 / rl) ;
		$_[3].c[0] = $_[2].c[0] + s * c1[0] + (1-c)*c2[0];
		$_[3].c[1] = $_[2].c[1] + s * c1[1] + (1-c)*c2[1];
		$_[3].c[2] = $_[2].c[2] + s * c1[2] + (1-c)*c2[2];
		/*
		printf("ROT MULTVEC (%f %f %f : %f) (%f %f %f) -> (%f %f %f)\\n",
			$_[1].r[0], $_[1].r[1], $_[1].r[2], $_[1].r[3],
			$_[2].c[0], $_[2].c[1], $_[2].c[2],
			$_[3].c[0], $_[3].c[1], $_[3].c[2]);
		*/
	~
}

sub ctype {return "struct SFRotation $_[1]"}
sub clength {4} #for C routes. Keep in sync with getClen in VRMLC.pm. 
sub cget {return "($_[1].r[$_[2]])"}

sub cfunc {
	return "{
		AV *a;
		SV **b;
		int i;
		if(!SvROK($_[2])) {
			$_[1].r[0] = 0;
			$_[1].r[1] = 1;
			$_[1].r[2] = 0;
			$_[1].r[3] = 0;
			/* die(\"Help! SFRotation without being ref\"); */
		} else {
			if(SvTYPE(SvRV($_[2])) != SVt_PVAV) {
				freewrlDie(\"Help! SFRotation without being arrayref\");
			}
			a = (AV *) SvRV($_[2]);
			for(i=0; i<4; i++) {
				b = av_fetch(a, i, 1); /* LVal for easiness */
				if(!b) {
					freewrlDie(\"Help: SFColor b == 0\");
				}
				$_[1].r[i] = SvNV(*b);
			}
		}
	} /*ee*/
	"
}


###########################################################
package VRML::Field::SFBool;
@ISA=VRML::Field;

sub init { return 0; }

sub parse {
	my($type,$p,$s,$n) = @_;
	$_[2] =~ /\G\s*(TRUE|FALSE)\b/gs
		or die "Invalid value for SFBool\n";
	if ($1 eq "TRUE") {return 1};
	return 0;
}

sub ctype {return "int $_[1]"}
sub clength {1} #for C routes. Keep in sync with getClen in VRMLC.pm. 
sub cget {return "($_[1])"}
sub cfunc {return "$_[1] = SvIV($_[2]);/*ff*/\n"}

sub print {print ($_[1] ? TRUE : FALSE)}
sub as_string {
	my ($this, $bool, $as_ecmascript) = @_;
	if ($as_ecmascript) {
		return ($bool ? true : false);
	} else {
		return ($bool ? TRUE : FALSE);
	}
}


###########################################################
package VRML::Field::SFString;
@ISA=VRML::Field;

# Ayla... This is closer to the VRML97 specification,
# which allows anything but unescaped " or \ in SFStrings.

#$Chars = qr/(?:[\x00-\x21\x23-\x5b\x5d-\x7f]*|\x5c\x22|\x5c{2}[^\x22])*/o;
$Chars = qr/(?:[\x00-\x21\x23-\x5b\x5d-\xffff]*|\x5c\x22|\x5c{2}[^\x22])*/o;

sub init { return ""; }

# XXX Handle backslashes in string properly
sub parse {
	$_[2] =~ /\G\s*\x22($Chars)\x22\s*/g
		or VRML::Error::parsefail($_[2], "improper SFString");
	my $str = $1;
	## cleanup ms-dos/windows end of line chars
	$str =~ s///g;
	$str =~ s/\\(.)/$1/g;
	#print "sfstring str is $str\n";
	return $str;
}

sub ctype {return "SV *$_[1]"}
sub calloc {"$_[1] = newSVpv(\"\",0);"}
sub cassign {"sv_setsv($_[1],$_[2]);"}
sub cfree {"SvREFCNT_dec($_[1]);"}
sub cfunc {"sv_setsv($_[1],$_[2]);/*gg*/"}

sub print {print "\"$_[1]\""}
sub as_string{"\"$_[1]\""}

sub clength {1} #for C routes. Keep in sync with getClen in VRMLC.pm. 

###########################################################
package VRML::Field::MFString;
@ISA=VRML::Field::Multi;



# XXX Should be optimized heavily! Other MFs are ok.

###########################################################
package VRML::Field::MFFloat;
@ISA=VRML::Field::Multi;

sub parse {
	my($type,$p) = @_;
	if($_[2] =~ /\G\s*\[\s*/gsc) {
		$_[2] =~ /\G([^\]]*)\]/gsc or
		 VRML::Error::parsefail($_[2],"unterminated MFFloat");
		my $a = $1;
		$a =~ s/^\s*//;
		$a =~ s/\s*$//;
		# XXX Errors ???
		my @a = split /\s*,\s*|\s+/,$a;
		pop @a if $a[-1] =~ /^\s+$/;
		# while($_[2] !~ /\G\s*,?\s*\]\s*/gsc) {
		# 	$_[2] =~ /\G\s*,\s*/gsc; # Eat comma if it is there...
		# 	my $v =  $stype->parse($p,$_[2],$_[3]);
		# 	push @a, $v if defined $v; 
		# }
		return \@a;
	} else {
		my $res = [VRML::Field::SFFloat->parse($p,$_[2],$_[3])];
		# Eat comma if it is there
		$_[2] =~ /\G\s*,\s*/gsc;
		return $res;
	}
}


###########################################################
package VRML::Field::MFVec3f;
@ISA=VRML::Field::Multi;

sub parse { 
	my($type,$p) = @_;
	my $pos = pos $_[2];
	if($_[2] =~ /\G\s*\[\s*/gsc) {
		(pos $_[2]) = $pos;
		my $nums = VRML::Field::MFFloat->parse($p,$_[2]);
		my @arr;
		for(0..($#$nums-2)/3) {
			push @arr,[$nums->[$_*3], $nums->[$_*3+1], $nums->[$_*3+2]];
		}
		return \@arr;
	} else {
		my $res = [VRML::Field::SFVec3f->parse($p,$_[2],$_[3])];
		# Eat comma if it is there
		$_[2] =~ /\G\s*,\s*/gsc;
		return $res;
	}
}


###########################################################
package VRML::Field::MFNode;
@ISA=VRML::Field::Multi;


###########################################################
package VRML::Field::MFColor;
@ISA=VRML::Field::Multi;


###########################################################
package VRML::Field::MFVec2f;
@ISA=VRML::Field::Multi;


###########################################################
package VRML::Field::MFInt32;
@ISA=VRML::Field::Multi;

sub parse {
	my($type,$p) = @_;
	if($_[2] =~ /\G\s*\[\s*/gsc) {
		$_[2] =~ /\G([^\]]*)\]/gsc or
		 VRML::Error::parsefail($_[2],"unterminated MFFloat");
		my $a = $1;
		$a =~ s/^\s*//s;
		$a =~ s/\s*$//s;
		# XXX Errors ???
		my @a = split /\s*,\s*|\s+/,$a;
		pop @a if $a[-1] =~ /^\s+$/;
		# while($_[2] !~ /\G\s*,?\s*\]\s*/gsc) {
		# 	$_[2] =~ /\G\s*,\s*/gsc; # Eat comma if it is there...
		# 	my $v =  $stype->parse($p,$_[2],$_[3]);
		# 	push @a, $v if defined $v; 
		# }
		return \@a;
	} else {
		my $res = [VRML::Field::SFInt32->parse($p,$_[2],$_[3])];
		# Eat comma if it is there
		$_[2] =~ /\G\s*,\s*/gsc;
		return $res;
	}
}


###########################################################
package VRML::Field::MFRotation;
@ISA=VRML::Field::Multi;


###########################################################
package VRML::Field::Multi;
@ISA=VRML::Field;

sub init { return []; }

sub ctype {
	my $r = (ref $_[0] or $_[0]);
	$r =~ s/VRML::Field::MF//;
	return "struct Multi_$r $_[1]";
}
sub clength {
	#for C routes. Keep in sync with getClen in VRMLC.pm.
	#clengths that are negative signal that something more than just a straight
	#copy for javascripting is required. Other than the fact that numbers are
	#negative, there is no symbolism placed on their values.

	my $r = (ref $_[0] or $_[0]);
	$r =~ s/VRML::Field::MF//;
	# these negative ones are returned in other places...
	# -11 = SFNode
	# -12 = SFImage

	if ($r eq "String") {return -13;}	# signal that a clength -13 is a Multi_String for CRoutes
	if ($r eq "Float") {return -14;}	# signal that a clength -14 is a Multi_Float for CRoutes
	if ($r eq "Rotation") {return -15;}	# signal that a clength -15 is a Multi_Rotation for CRoutes
	if ($r eq "Int32") {return -16;}	# signal that a clength -16 is a Multi_Int32 for CRoutes
	if ($r eq "Color") {return -17;}	# signal that a clength -1  is a Multi_Color for CRoutes
	if ($r eq "Vec2f") {return -18;}	# signal that a clength -18 is a Multi_Vec2f for CRoutes
	if ($r eq "Vec3f") {return -1;} 	# signal that a clength -1  is a Multi_Vec3f for CRoutes
	if ($r eq "Node") {return -10;} 	# signal that a clength -10 is a Multi_Node for CRoutes

	print "clength struct not handled Multi_$r $_[1]\n";
	return 0;
}
sub cstruct {
	my $r = (ref $_[0] or $_[0]);
	my $t = $r;
	$r =~ s/VRML::Field::MF//;
	$t =~ s/::MF/::SF/;
	my $ct = $t->ctype;
	return "struct Multi_$r { int n; $ct *p; };"
}
sub calloc {
	return "$_[1].n = 0; $_[1].p = 0;";
}
sub cassign {
	my $t = (ref $_[0] or $_[0]);
	$t =~ s/::MF/::SF/;
	my $cm = $t->calloc("$_[1].n");
	my $ca = $t->cassign("$_[1].p[__i]", "$_[2].p[__i]");
	"if($_[1].p) {free($_[1].p)};
	 $_[1].n = $_[2].n; $_[1].p = malloc(sizeof(*($_[1].p))*$_[1].n);
	 {int __i;
	  for(__i=0; __i<$_[1].n; __i++) {
	  	$cm
		$ca
	  }
	 }
	"
}
sub cfree {
	"if($_[1].p) {free($_[1].p);$_[1].p=0;} $_[1].n = 0;"
}
sub cgetn { "($_[1].n)" }
sub cget { if($#_ == 1) {"($_[1].p)"} else {
	my $r = (ref $_[0] or $_[0]);
	$r =~ s/::MF/::SF/;
	if($#_ == 2) {
		return "($_[1].p[$_[2]])";
	}
	return $r->cget("($_[1].p[$_[2]])", @$_[3..$#_])
	} }

sub cfunc {
	my $r = (ref $_[0] or $_[0]);

	$r =~ s/::MF/::SF/;

	# get the base type, ie, get something like SFFloat from VRML::Field::SFFloat
	my $baseType = $r;
	$baseType =~ s/VRML::Field:://;
	print "cfunc, basetype $baseType\n";
	# we do not need to make a float into a (struct SFFloat *)...
	# and we have other compiler warnings. So, change some castings around to satisfy
	# the compilers.

	if ($baseType eq "SFFloat") {
		$baseType = "float";
	} elsif ($baseType eq "SFVec3f") {
		$baseType = "struct SFColor";
	} elsif ($baseType eq "SFInt32") {
		$baseType = "int";
	} elsif ($baseType eq "SFNode") {
		$baseType = "void *";
	} elsif ($baseType eq "SFString") {
		$baseType = "SV *";
	} else {
		$baseType = "struct $baseType";
	}

	
	my $cm = $r->calloc("$_[1].p[iM]");
	my $su = $r->cfunc("($_[1].p[iM])","(*bM)");
	return "{
		/* generic cfunc function */
		AV *aM;
		SV **bM;
		int iM;
		int lM;
		if(!SvROK($_[2])) {
			$_[1].n = 0;
			$_[1].p = 0;
			/* die(\"Help! Multi without being ref\"); */
		} else {
			if(SvTYPE(SvRV($_[2])) != SVt_PVAV) {
				freewrlDie(\"Help! Multi without being arrayref\");
			}
			aM = (AV *) SvRV($_[2]);
			lM = av_len(aM)+1;
			/* XXX Free previous p */
			$_[1].n = lM;
			$_[1].p = ($baseType *)malloc(lM * sizeof(*($_[1].p)));
			/* XXX ALLOC */
			for(iM=0; iM<lM; iM++) {
				bM = av_fetch(aM, iM, 1); /* LVal for easiness */
				if(!bM) {
					freewrlDie(\"Help: Multi $r bM == 0\");
				}
				$cm
				$su
			}
		}
	} /*hh*/
	"
}


sub parse {
	my($type,$p) = @_;
	my $stype = $type;
	$stype =~ s/::MF/::SF/;
	if($_[2] =~ /\G\s*\[\s*/gsc) {
		my @a;
		while($_[2] !~ /\G\s*,?\s*\]\s*/gsc) {
			# print "POS0: ",(pos $_[2]),"\n";
			# removing $r = causes this to be evaluated
			# in array context -> fail.
			# eat commas if they are there...
			my $r = ($_[2] =~ /\G\s*,+\s*/gsc);
			# my $wa = wantarray;
			# print "R: '$r' (WA: $wa)\n";
			# print "POS1: ",(pos $_[2]),"\n";
			my $v =  $stype->parse($p,$_[2],$_[3]);
			# print "POS2: ",(pos $_[2]),"\n";
			push @a, $v if defined $v; 
		}
		return \@a;
	} else {
		my $res = [$stype->parse($p,$_[2],$_[3])];
		# eat commas if they are there
		my $r = $_[2] =~ /\G\s*,+\s*/gsc;
		return $res;
	}
}

sub print {
	my($type) = @_;
	print " [ ";
	my $r = $type;
	$r =~ s/::MF/::SF/;
	for(@{$_[1]}) {
		$r->print($_);
	}
	print " ]\n";
}

sub as_string {
	my ($type, $value, $as_ecmascript) = @_;
	$type =~ s/::MF/::SF/;

	if (!$value) { return; }

	if("ARRAY" eq ref $value) {
		return "[]" if (!@{$value});
		return "[ ".(join ' ', map {$type->as_string($_)} @{$value})." ]";
	} else {
		return $value->as_string();
	}
}



###########################################################
package VRML::Field::SFNode;

sub init { return "NULL"; }

sub copy { return $_[1] }

sub ctype {"void *$_[1]"}      # XXX ???
sub calloc {"$_[1] = 0;"}
sub cfree {"NODE_REMOVE_PARENT($_[1]); $_[1] = 0;"}
sub cstruct {""}
sub cfunc {
	"$_[1] = (void *)SvIV($_[2]); NODE_ADD_PARENT($_[1]);/*ii*/"
}
sub cget {if(!defined $_[2]) {return "$_[1]"}
	else {die "SFNode index!??!"} }

sub as_string {
	$_[1]->as_string();
}
sub clength {-11} #for C routes. Keep in sync with getClen in VRMLC.pm.


##
## RCS: Implement SFImage
## Remi Cohen-Scali
##


###########################################################
package VRML::Field::SFImage;
@ISA=VRML::Field;
VRML::Error->import;

sub init { return [0, 0, 0]; }

sub parse {
  my($type,$p) = @_;

# define SFImage as being a range of "numbers" separated by spaces, tabs, newlines, etc.
# this will end at what hopefully is the trailing brace.

$SFImageChars = qr~[a-fA-FxX+\- \n\t\s\d]+~;

# now, get the image data
$_[2] =~ /\G\s*($SFImageChars)\s*/gc
	or parsefail ($_[2], " problem finding SFImage");
  return $1;
}


sub ctype {return "SV *$_[1]"}
sub calloc {"$_[1] = newSVpv(\"\",0);"}
sub cassign {"sv_setsv($_[1],$_[2]);"}
sub cfree {"SvREFCNT_dec($_[1]);"}
sub cfunc {"sv_setsv($_[1],$_[2]);/*jj*/"}

sub print {print "\"$_[1]\""}
sub as_string{"\"$_[1]\""}

sub clength {-12}; # signal that a -12 is a SFImage for CRoutes #for C routes. Keep in sync with getClen in VRMLC.pm.



1;

