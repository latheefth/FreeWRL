#
# Copyright (C) 1998 Tuomas J. Lukka 1999 John Stewart CRC Canada
# DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
# See the GNU Library General Public License (file COPYING in the distribution)
# for conditions of use and redistribution.
#
# Field types, parsing and printing, Perl, C and Java.
#


###########################################################
package VRML::Field;

use strict;
use warnings;

# The C type interface for the field type, encapsulated
# By encapsulating things well enough, we'll be able to completely
# change the interface later, e.g. to fit together with javascript etc.
sub ctype ($) {my ($type) = @_; die "VRML::Field::ctype - fori $type abstract function called"}
sub cstruct () {"/*cstruct*/"}

# Field types. NOTE: Keep this order correct in the PARSE_TYPE in CFuncs/CParseParser.c file
our @Fields = qw/
	SFFloat
	MFFloat
	SFRotation
	MFRotation
	SFVec3f
	MFVec3f
	SFBool
	MFBool
	SFInt32
	MFInt32
	SFNode
	MFNode
	SFColor
	MFColor
	SFColorRGBA
	MFColorRGBA
	SFTime
	MFTime
	SFString
	MFString
	SFVec2f
	MFVec2f
	SFImage
	FreeWRLPTR
	FreeWRLThread
	SFVec3d
	MFVec3d
	SFDouble
	MFDouble
	SFMatrix3f
	MFMatrix3f
	SFMatrix3d
	MFMatrix3d
	SFMatrix4f
	MFMatrix4f
	SFMatrix4d
	MFMatrix4d
	SFVec2d
	MFVec2d
	SFVec4f
	MFVec4f
	SFVec4d
	MFVec4d
/;


###########################################################

package VRML::Field::SFBool;
our @ISA="VRML::Field";


sub ctype {return "int " . ($_[1] || "")}
sub cInitialize {
	my ($this,$field,$val) = @_;
	if (!defined $val) {$val = 0} # inputOnlys, set it to any value
	return "$field = $val";
}

package VRML::Field::MFBool;
our @ISA="VRML::Field::Multi";

sub cInitialize {
	my ($this,$field,$val) = @_;
	my $count = @{$val};
	my $retstr;
	my $tmp;

	if (!defined $val) {$count=0} # inputOnlys, set it to any value
	if ($count > 0) {

		$retstr = $retstr . "$field.p = MALLOC (int *, sizeof(int)*$count);\n";
		for ($tmp=0; $tmp<$count; $tmp++) {
			my $arline = @{$val}[$tmp];
			$retstr = $retstr. "\n\t\t\t$field.p[$tmp] = $arline; ";
		}
		$retstr = $retstr."$field.n=$count;";
	} else {
		$retstr = "$field.n=0; $field.p=0";
	}
	return $retstr;
}


###########################################################
package VRML::Field::SFColor;
our @ISA="VRML::Field";

sub cstruct {return "struct SFColor { float c[3]; };"}
sub ctype {return "struct SFColor " . ($_[1] || "")}
sub cInitialize {
        my ($this,$field,$val) = @_;
        if (!defined $val) {print "undefined in SFColor\n"} # inputOnlys, set it to any value
        # get the actual value and ensure that it is a float
        my $av0 = "@{$val}[0]";
        my $av1 = "@{$val}[1]";
        my $av2 = "@{$val}[2]";
        my $pv;
        $pv = index $av0, "."; if ($pv < 0) { $av0 = $av0.".0f"; } else { $av0 = $av0."f"; }
        $pv = index $av1, "."; if ($pv < 0) { $av1 = $av1.".0f"; } else { $av1 = $av1."f"; }
        $pv = index $av2, "."; if ($pv < 0) { $av2 = $av2.".0f"; } else { $av2 = $av2."f"; }
        # print "SFColor, field value now is $av0 $av1 $av2\n";

        return  "$field.c[0] = $av0;".
                "$field.c[1] = $av1;".
                "$field.c[2] = $av2;";
}

package VRML::Field::MFColor;
our @ISA="VRML::Field::Multi";

sub cInitialize {
	my ($this,$field,$val) = @_;
	my $count = @{$val};
	my $retstr;
	my $tmp;
	my $whichVal;

	if (!defined $val) {$count=0} # inputOnlys, set it to any value
	#print "MFCOLOR field $field val @{$val} has $count INIT\n";
	if ($count > 0) {
		#print "MALLOC MFCOLOR field $field val @{$val} has $count INIT\n";

		$retstr = "$field.p = MALLOC (struct SFColor *, sizeof(struct SFColor)*$count);\n";
		for ($tmp=0; $tmp<$count; $tmp++) {
			my $arline = @{$val}[$tmp];
			for ($whichVal = 0; $whichVal < 3; $whichVal++) {
                                # get the actual value and ensure that it is a float
                                my $av = "@{$arline}[$whichVal]";
                                my $pv = index $av, ".";
                                if ($pv < 0) { $av = $av.".0f"; } else { $av = $av."f"; }
                                $retstr = $retstr. "\n\t\t\t$field.p[$tmp].c[$whichVal] = $av; ";
			}
		}
		$retstr = $retstr."\n\t\t\t$field.n=$count;";
	} else {
		$retstr = "$field.n=0; $field.p=0";
	}
	return $retstr;
}


###########################################################
package VRML::Field::SFColorRGBA;
our @ISA="VRML::Field";


sub cstruct {return "struct SFColorRGBA { float c[4]; };"}
sub ctype {return "struct SFColorRGBA " . ($_[1] || "")}

sub cInitialize {
	my ($this,$field,$val) = @_;
	if (!defined $val) {print "undefined in SFColorRGBA\n"} # inputOnlys, set it to any value
	return 	"$field.c[0] = @{$val}[0];".
		"$field.c[1] = @{$val}[1];".
		"$field.c[2] = @{$val}[2];".
		"$field.c[3] = @{$val}[3];";
}



package VRML::Field::MFColorRGBA;
our @ISA="VRML::Field::Multi";


sub cInitialize {
	my ($this,$field,$val) = @_;
	my $count = @{$val};
	my $retstr;
	my $tmp;

	if (!defined $val) {$count=0} # inputOnlys, set it to any value
	#print "MFCOLORRGBA field $field val @{$val} has $count INIT\n";
	if ($count > 0) {
		#print "MALLOC MFROTATION field $field val @{$val} has $count INIT\n";

		$retstr = "$field.p = MALLOC (struct SFColorRGBA *, sizeof(struct SFColorRGBA)*$count);\n";
		for ($tmp=0; $tmp<$count; $tmp++) {
			my $arline = @{$val}[$tmp];
			my $whichVal;
			for ($whichVal = 0; $whichVal < 4; $whichVal++) {
                                # get the actual value and ensure that it is a float
                                my $av = "@{$arline}[$whichVal]";
                                my $pv = index $av, ".";
                                if ($pv < 0) { $av = $av.".0f"; } else { $av = $av."f"; }
                                $retstr = $retstr. "\n\t\t\t$field.p[$tmp].c[$whichVal] = $av; ";
			}
		}
		$retstr = $retstr."\n\t\t\t$field.n=$count;";
	} else {
		$retstr = "$field.n=0; $field.p=0";
	}
	return $retstr;
}


###########################################################
package VRML::Field::SFDouble;
our @ISA="VRML::Field";

sub ctype {return "double " . ($_[1] || "")}
sub cInitialize {
	my ($this,$field,$val) = @_;
	if (!defined $val) {$val = 0} # inputOnlys, set it to any value
	return "$field = $val";
}


package VRML::Field::MFDouble;
our @ISA="VRML::Field::Multi";

sub cInitialize {
	my ($this,$field,$val) = @_;
	my $count = ref $val eq "ARRAY" ? @{$val} : 0;
	my $retstr;
	my $tmp;

	#print "MFDouble field $field val @{$val} has $count INIT\n";
	if ($count > 0) {
		#print "MALLOC MFDouble field $field val @{$val} has $count INIT\n";
		$retstr = $retstr . "$field.p = MALLOC (double *, sizeof(double)*$count);\n";
		for ($tmp=0; $tmp<$count; $tmp++) {
			$retstr = $retstr .  "\t\t\t$field.p[$tmp] = @{$val}[$tmp];\n";
		}
		$retstr = $retstr . "\t\t\t$field.n=$count;";
	} else {
		$retstr =  "$field.n=0; $field.p=0";
	}
	return $retstr;
}


###########################################################
package VRML::Field::SFFloat;
our @ISA="VRML::Field";

sub ctype {"float " . ($_[1] || "")}
sub cInitialize {
	my ($this,$field,$val) = @_;
	if (!defined $val) {$val = "0.0"} # inputOnlys, set it to any value

	# get the actual value and ensure that it is a float
	my $av = "$val";
	my $pv = index $av, ".";
	if ($pv < 0) { $av = $av.".0f"; } else { $av = $av."f"; }
	#print "SFFloat, field value now is $av for orig value $pv\n";
	return "$field = $av";
}

package VRML::Field::MFFloat;
our @ISA="VRML::Field::Multi";

sub cInitialize {
	my ($this,$field,$val) = @_;
	my $count = ref $val eq "ARRAY" ? @{$val} : 0;
	my $retstr;
	my $tmp;

	if ($count > 0) {
		#print "MALLOC MFFLOAT field $field val @{$val} has $count INIT\n";
		$retstr = $retstr . "$field.p = MALLOC (float *, sizeof(float)*$count);\n";
		for ($tmp=0; $tmp<$count; $tmp++) {
			# get the actual value and ensure that it is a float
			my $av = "@{$val}[$tmp]";
			my $pv = index $av, ".";
			if ($pv < 0) { $av = $av.".0f"; } else { $av = $av."f"; }
			# print "MFFloat, field value now is $av for orig value $pv\n";
			$retstr = $retstr .  "\t\t\t$field.p[$tmp] = $av;\n";
		}
		$retstr = $retstr . "\t\t\t$field.n=$count;";

	} else {
		$retstr =  "$field.n=0; $field.p=0";
	}
	return $retstr;
}


###########################################################
package VRML::Field::SFImage;
our @ISA="VRML::Field";


sub ctype {return "struct Multi_Int32 " . ($_[1] || "")}

sub cInitialize {
	my ($this,$field,$val) = @_;
	if (!defined $val) {print "undefined in SFImage\n"} # inputOnlys, set it to any value
	my $count = ref $val eq "ARRAY" ? @{$val} : 0;
	#SFImage defaults to 0,0,0\n";
	return "$field.n=3; $field.p=MALLOC (int *, sizeof(int)*3); $field.p[0] = 0; $field.p[1] = 0; $field.p[2] = 0;";
}


package VRML::Field::MFImage;
our @ISA="VRML::Field::Multi";

sub cInitialize {
	print "MFImage not coded yet\n";
}


###########################################################
package VRML::Field::SFInt32;
our @ISA="VRML::Field";

sub ctype {return "int " . ($_[1] || "")}
sub cInitialize {
	my ($this,$field,$val) = @_;
	if (!defined $val) {$val = 0} # inputOnlys, set it to any value
	return "$field = $val";
}



package VRML::Field::MFInt32;
our @ISA="VRML::Field::Multi";

sub cInitialize {
	my ($this,$field,$val) = @_;
	my $count = ref $val eq "ARRAY" ? @{$val} : 0;
	my $retstr;
	my $tmp;
	#print "MFINT32 field $field val @{$val} has $count INIT\n";
	if (!defined $val) {$count=0} # inputOnlys, set it to any value
	if ($count > 0) {
                #print "MALLOC MFINT32 field $field val @{$val} has $count INIT\n";
                $retstr = $retstr . "$field.p = MALLOC (int *, sizeof(int)*$count);\n";
                for ($tmp=0; $tmp<$count; $tmp++) {
                        $retstr = $retstr .  "\t\t\t$field.p[$tmp] = @{$val}[$tmp];\n";
                }
                $retstr = $retstr . "\t\t\t$field.n=$count;";
	} else {
		return "$field.n=0; $field.p=0";
	}
}



###########################################################
package VRML::Field::SFMatrix3d;
our @ISA="VRML::Field";

sub cstruct {return "struct SFMatrix3d { double c[9]; };"}
sub ctype {return "struct SFMatrix3d " . ($_[1] || "")}
sub cInitialize {
	my ($this,$field,$val) = @_;
	if (!defined $val) {print "undefined in SFColor\n"} # inputOnlys, set it to any value
	return 	"$field.c[0] = @{$val}[0];".
		"$field.c[1] = @{$val}[1];".
		"$field.c[2] = @{$val}[2];".
		"$field.c[3] = @{$val}[3];".
		"$field.c[4] = @{$val}[4];".
		"$field.c[5] = @{$val}[5];".
		"$field.c[6] = @{$val}[6];".
		"$field.c[7] = @{$val}[7];".
		"$field.c[8] = @{$val}[8];";
}

package VRML::Field::MFMatrix3d;
our @ISA="VRML::Field::Multi";

sub cInitialize {
	my ($this,$field,$val) = @_;
	my $count = @{$val};
	my $retstr;
	my $tmp;
	my $whichVal;

	if (!defined $val) {$count = 0;} # inputOnlys, set it to any value
	#print "MFVEC3F field $field val @{$val} has $count INIT\n";
	if ($count > 0) {
		#print "MALLOC MFVEC3F field $field val @{$val} has $count INIT\n";

		$retstr = "$field.p = MALLOC (struct SFMatrix3f *, sizeof(struct SFMatrix3f)*$count);\n";
		for ($tmp=0; $tmp<$count; $tmp++) {
			my $arline = @{$val}[$tmp];
			for ($whichVal = 0; $whichVal < 9; $whichVal++) {
				$retstr = $retstr. "\n\t\t\t$field.p[$tmp].c[$whichVal] = ".@{$arline}[$whichVal]."; ";
			}
		}
		$retstr = $retstr."\n\t\t\t$field.n=$count;";
	} else {
		$retstr = "$field.n=0; $field.p=0";
	}
	return $retstr;
}


###########################################################
package VRML::Field::SFMatrix3f;
our @ISA="VRML::Field";

sub cstruct {return "struct SFMatrix3f { float c[9]; };"}
sub ctype {return "struct SFMatrix3f " . ($_[1] || "")}
sub cInitialize {
	my ($this,$field,$val) = @_;
	if (!defined $val) {print "undefined in SFColor\n"} # inputOnlys, set it to any value
	return 	"$field.c[0] = @{$val}[0];".
		"$field.c[1] = @{$val}[1];".
		"$field.c[2] = @{$val}[2];".
		"$field.c[3] = @{$val}[3];".
		"$field.c[4] = @{$val}[4];".
		"$field.c[5] = @{$val}[5];".
		"$field.c[6] = @{$val}[6];".
		"$field.c[7] = @{$val}[7];".
		"$field.c[8] = @{$val}[8];";
}


package VRML::Field::MFMatrix3f;
our @ISA="VRML::Field::Multi";

sub cInitialize {
	my ($this,$field,$val) = @_;
	my $count = @{$val};
	my $retstr;
	my $tmp;
	my $whichVal;

	if (!defined $val) {$count = 0;} # inputOnlys, set it to any value
	#print "MFVEC3F field $field val @{$val} has $count INIT\n";
	if ($count > 0) {
		#print "MALLOC MFVEC3F field $field val @{$val} has $count INIT\n";

		$retstr = "$field.p = MALLOC (struct SFMatrix3f *, sizeof(struct SFMatrix3f)*$count);\n";
		for ($tmp=0; $tmp<$count; $tmp++) {
			my $arline = @{$val}[$tmp];
			for ($whichVal = 0; $whichVal < 9; $whichVal++) {
                                # get the actual value and ensure that it is a float
                                my $av = "@{$arline}[$whichVal]";
                                my $pv = index $av, ".";
                                if ($pv < 0) { $av = $av.".0f"; } else { $av = $av."f"; }
                                $retstr = $retstr. "\n\t\t\t$field.p[$tmp].c[$whichVal] = $av; ";
			}
		}
		$retstr = $retstr."\n\t\t\t$field.n=$count;";
	} else {
		$retstr = "$field.n=0; $field.p=0";
	}
	return $retstr;
}

###########################################################
package VRML::Field::SFMatrix4d;
our @ISA="VRML::Field";

sub cstruct {return "struct SFMatrix4d { double c[16]; };"}
sub ctype {return "struct SFMatrix4d " . ($_[1] || "")}
sub cInitialize {
	my ($this,$field,$val) = @_;
	if (!defined $val) {print "undefined in SFColor\n"} # inputOnlys, set it to any value
	return 	"$field.c[0] = @{$val}[0];".
		"$field.c[1] = @{$val}[1];".
		"$field.c[2] = @{$val}[2];".
		"$field.c[3] = @{$val}[3];".
		"$field.c[4] = @{$val}[4];".
		"$field.c[5] = @{$val}[5];".
		"$field.c[6] = @{$val}[6];".
		"$field.c[7] = @{$val}[7];".
		"$field.c[8] = @{$val}[8];".
		"$field.c[9] = @{$val}[9];".
		"$field.c[10] = @{$val}[10];".
		"$field.c[11] = @{$val}[11];".
		"$field.c[12] = @{$val}[12];".
		"$field.c[13] = @{$val}[13];".
		"$field.c[14] = @{$val}[14];".
		"$field.c[15] = @{$val}[15];";
}

package VRML::Field::MFMatrix4d;
our @ISA="VRML::Field::Multi";

sub cInitialize {
	my ($this,$field,$val) = @_;
	my $count = @{$val};
	my $retstr;
	my $tmp;
	my $whichVal;

	if (!defined $val) {$count = 0;} # inputOnlys, set it to any value
	#print "MFVEC3F field $field val @{$val} has $count INIT\n";
	if ($count > 0) {
		#print "MALLOC MFVEC3F field $field val @{$val} has $count INIT\n";

		$retstr = "$field.p = MALLOC (struct SFMatrix4d *, sizeof(struct SFMatrix4d)*$count);\n";
		for ($tmp=0; $tmp<$count; $tmp++) {
			my $arline = @{$val}[$tmp];
			for ($whichVal = 0; $whichVal < 16; $whichVal++) {
				$retstr = $retstr. "\n\t\t\t$field.p[$tmp].c[$whichVal] = ".@{$arline}[$whichVal]."; ";
			}
		}
		$retstr = $retstr."\n\t\t\t$field.n=$count;";
	} else {
		$retstr = "$field.n=0; $field.p=0";
	}
	return $retstr;
}

###########################################################
package VRML::Field::SFMatrix4f;
our @ISA="VRML::Field";

sub cstruct {return "struct SFMatrix4f { float c[16]; };"}
sub ctype {return "struct SFMatrix4f " . ($_[1] || "")}
sub cInitialize {
	my ($this,$field,$val) = @_;
	if (!defined $val) {print "undefined in SFColor\n"} # inputOnlys, set it to any value
	return 	"$field.c[0] = @{$val}[0];".
		"$field.c[1] = @{$val}[1];".
		"$field.c[2] = @{$val}[2];".
		"$field.c[3] = @{$val}[3];".
		"$field.c[4] = @{$val}[4];".
		"$field.c[5] = @{$val}[5];".
		"$field.c[6] = @{$val}[6];".
		"$field.c[7] = @{$val}[7];".
		"$field.c[8] = @{$val}[8];".
		"$field.c[9] = @{$val}[9];".
		"$field.c[10] = @{$val}[10];".
		"$field.c[11] = @{$val}[11];".
		"$field.c[12] = @{$val}[12];".
		"$field.c[13] = @{$val}[13];".
		"$field.c[14] = @{$val}[14];".
		"$field.c[15] = @{$val}[15];";
}


package VRML::Field::MFMatrix4f;
our @ISA="VRML::Field::Multi";

sub cInitialize {
	my ($this,$field,$val) = @_;
	my $count = @{$val};
	my $retstr;
	my $tmp;
	my $whichVal;

	if (!defined $val) {$count = 0;} # inputOnlys, set it to any value
	#print "MFVEC3F field $field val @{$val} has $count INIT\n";
	if ($count > 0) {
		#print "MALLOC MFVEC3F field $field val @{$val} has $count INIT\n";

		$retstr = "$field.p = MALLOC (struct SFMatrix4f *, sizeof(struct SFMatrix4f)*$count);\n";
		for ($tmp=0; $tmp<$count; $tmp++) {
			my $arline = @{$val}[$tmp];
			for ($whichVal = 0; $whichVal < 16; $whichVal++) {
                                # get the actual value and ensure that it is a float
                                my $av = "@{$arline}[$whichVal]";
                                my $pv = index $av, ".";
                                if ($pv < 0) { $av = $av.".0f"; } else { $av = $av."f"; }
                                $retstr = $retstr. "\n\t\t\t$field.p[$tmp].c[$whichVal] = $av; ";
			}
		}
		$retstr = $retstr."\n\t\t\t$field.n=$count;";
	} else {
		$retstr = "$field.n=0; $field.p=0";
	}
	return $retstr;
}



###########################################################
package VRML::Field::SFNode;

sub ctype {"struct X3D_Node *" . ($_[1] || "")}
sub cstruct {""}

sub cInitialize {
	my ($this,$field,$val) = @_;
	if (!defined $val) {print "undefined in SFNode\n"} # inputOnlys, set it to any value
	return "$field = $val";
}


package VRML::Field::MFNode;
our @ISA="VRML::Field::Multi";

sub cInitialize {
	my ($this,$field,$val) = @_;
	my $count = ref $val eq "ARRAY" ? @{$val} : 0;

	if ($count > 0) {
		print "MFNODE HAVE TO MALLOC HERE\n";
	} else {
		return "$field.n=0; $field.p=0";
	}
}



###########################################################
package VRML::Field::SFRotation;
our @ISA="VRML::Field";


sub cstruct {return "struct SFRotation { float c[4]; };"}

sub ctype {return "struct SFRotation " . ($_[1] || "")}
sub cInitialize {
	my ($this,$field,$val) = @_;
	if (!defined $val) {print "undefined in SFRotation\n"} # inputOnlys, set it to any value
	return 	"$field.c[0] = @{$val}[0];".
		"$field.c[1] = @{$val}[1];".
		"$field.c[2] = @{$val}[2];".
		"$field.c[3] = @{$val}[3];";
}


package VRML::Field::MFRotation;
our @ISA="VRML::Field::Multi";

sub cInitialize {
	my ($this,$field,$val) = @_;
	my $count = ref $val eq "ARRAY" ? @{$val} : 0;
	my $retstr;
	my $tmp;
	my $whichVal;

	#print "MFROTATION field $field val @{$val} has $count INIT\n";
	if ($count > 0) {
		#print "MALLOC MFROTATION field $field val @{$val} has $count INIT\n";

		$retstr = "$field.p = MALLOC (struct SFRotation *, sizeof(struct SFRotation)*$count);\n";
		for ($tmp=0; $tmp<$count; $tmp++) {
			my $arline = @{$val}[$tmp];
			for ($whichVal = 0; $whichVal < 4; $whichVal++) {
                                # get the actual value and ensure that it is a float
                                my $av = "@{$arline}[$whichVal]";
                                my $pv = index $av, ".";
                                if ($pv < 0) { $av = $av.".0f"; } else { $av = $av."f"; }
                                $retstr = $retstr. "\n\t\t\t$field.p[$tmp].c[$whichVal] = $av; ";
			}
		}
		$retstr = $retstr."\n\t\t\t$field.n=$count;";
	} else {
		$retstr =  "$field.n=0; $field.p=0";
	}
	return $retstr;
}


###########################################################
package VRML::Field::SFString;
our @ISA="VRML::Field";

sub ctype {return "struct Uni_String *" . ($_[1] || "")}
sub cInitialize {
	my ($this,$field,$val) = @_;

	if (!defined $val) {$val = "";} # inputOnlys, set it to any value
	return "$field = newASCIIString(\"$val\")";
}

package VRML::Field::MFString;
our @ISA="VRML::Field::Multi";

sub cInitialize {
	my ($this,$field,$val) = @_;
	my $count = @{$val};
	my $retstr;
	my $tmp;

	if (!defined $val) {$count=0} # inputOnlys, set it to any value
	#print "MFSTRING field $field val @{$val} has $count INIT\n";
	if ($count > 0) {
		#print "MALLOC MFSTRING field $field val @{$val} has $count INIT\n";
		$retstr = $retstr . "$field.p = MALLOC (struct Uni_String **, sizeof(struct Uni_String)*$count);";
		for ($tmp=0; $tmp<$count; $tmp++) {
			$retstr = $retstr .  "$field.p[$tmp] = newASCIIString(\"".@{$val}[$tmp]."\");";
		}
		$retstr = $retstr . "$field.n=$count; ";

	} else {
		$retstr = "$field.n=0; $field.p=0";
	}
	return $retstr;
}


###########################################################
package VRML::Field::SFTime;
our @ISA="VRML::Field::SFFloat";

sub ctype {"double " . ($_[1] || "")}
sub cInitialize {
	my ($this,$field,$val) = @_;
	if (!defined $val) {$val = 0.0} # inputOnlys, set it to any value
	return "$field = $val";
}

package VRML::Field::MFTime;
our @ISA="VRML::Field::MFFloat";

sub cInitialize {
	my ($this,$field,$val) = @_;
	my $count = @{$val};
	#print "MFTIME field $field val @{$val} has $count INIT\n";
	if (!defined $val) {$count=0} # inputOnlys, set it to any value
	if ($count > 0) {
		print "MFTIME HAVE TO MALLOC HERE\n";
	} else {
		return "$field.n=0; $field.p=0";
	}
}

###########################################################
package VRML::Field::SFVec2d;
our @ISA="VRML::Field";

sub cstruct {return "struct SFVec2d { double c[2]; };"}
sub ctype {return "struct SFVec2d " . ($_[1] || "")}
sub cInitialize {
	my ($this,$field,$val) = @_;
	if (!defined $val) {print "undefined in SFVec2d\n"} # inputOnlys, set it to any value
	return 	"$field.c[0] = @{$val}[0];".
		"$field.c[1] = @{$val}[1];";
}


package VRML::Field::MFVec2d;
our @ISA="VRML::Field::Multi";

sub cInitialize {
	my ($this,$field,$val) = @_;
	my $count = @{$val};
	my $retstr;
	my $tmp;
	my $whichVal;

	if (!defined $val) {$count=0} # inputOnlys, set it to any value
	#print "MFVec2F field $field val @{$val} has $count INIT\n";
	if ($count > 0) {
		$retstr = "$field.p = MALLOC (struct SFVec2d *, sizeof(struct SFVec2d)*$count);";
		for ($tmp=0; $tmp<$count; $tmp++) {
			my $arline = @{$val}[$tmp];
			for ($whichVal = 0; $whichVal < 2; $whichVal++) {
				$retstr = $retstr. "\n\t\t\t$field.p[$tmp].c[$whichVal] = ".@{$arline}[$whichVal]."; ";
			}
		}
		$retstr = $retstr."\n\t\t\t$field.n=$count";

	} else {
		$retstr = "$field.n=0; $field.p=0";
	}
	return $retstr;
}


###########################################################
package VRML::Field::SFVec2f;
our @ISA="VRML::Field";

sub cstruct {return "struct SFVec2f { float c[2]; };"}
sub ctype {return "struct SFVec2f " . ($_[1] || "")}
sub cInitialize {
	my ($this,$field,$val) = @_;
	my $count;
	if (!defined $val) {$count=0} else {$count=1} # inputOnlys, set it to any value
	if ($count eq 1) {
		# get the actual value and ensure that it is a float
		my $av0 = "@{$val}[0]";
		my $av1 = "@{$val}[1]";
		my $pv = index $av0, "."; if ($pv < 0) { $av0 = $av0.".0f"; } else { $av0 = $av0."f"; }
		$pv = index $av1, "."; if ($pv < 0) { $av1 = $av1.".0f"; } else { $av1 = $av1."f"; }
		# print "SFVec2f, field value now is $av0 $av1\n";

		return 	"$field.c[0] = $av0;".
			"$field.c[1] = $av1;";
	} else {
		return "$field.c[0] = 0; $field.c[1] = 1;";
	}
}


package VRML::Field::MFVec2f;
our @ISA="VRML::Field::Multi";


sub cInitialize {
	my ($this,$field,$val) = @_;
	my $count = ref $val eq "ARRAY" ? @{$val} : 0;
	my $retstr;
	my $tmp;
	my $whichVal;

	#print "MFVec2F field $field val @{$val} has $count INIT\n";
	if ($count > 0) {
		#print "MALLOC MFVec2F field $field val @{$val} has $count INIT\n";

		$retstr = "$field.p = MALLOC (struct SFVec2f *, sizeof(struct SFVec2f)*$count);";
		for ($tmp=0; $tmp<$count; $tmp++) {
			my $arline = @{$val}[$tmp];
			for ($whichVal = 0; $whichVal < 2; $whichVal++) {
                                # get the actual value and ensure that it is a float
                                my $av = "@{$arline}[$whichVal]";
                                my $pv = index $av, ".";
                                if ($pv < 0) { $av = $av.".0f"; } else { $av = $av."f"; }
                                $retstr = $retstr. "\n\t\t\t$field.p[$tmp].c[$whichVal] = $av; ";
			}
		}
		$retstr = $retstr."\n\t\t\t$field.n=$count";

	} else {
		$retstr = "$field.n=0; $field.p=0";
	}
	return $retstr;
}


###########################################################
package VRML::Field::SFVec3d;
our @ISA="VRML::Field";
sub cstruct {return "struct SFVec3d { double c[3]; };"}
sub ctype {return "struct SFVec3d " . ($_[1] || "");}
sub cInitialize {
	my ($this,$field,$val) = @_;
	if (!defined $val) {print "undefined in SFVec3d\n"} # inputOnlys, set it to any value
	return 	"$field.c[0] = @{$val}[0];".
		"$field.c[1] = @{$val}[1];".
		"$field.c[2] = @{$val}[2];";
}


package VRML::Field::MFVec3d;
our @ISA="VRML::Field::Multi";

sub cInitialize {
	my ($this,$field,$val) = @_;
	my $count = ref $val eq "ARRAY" ? @{$val} : 0;
	my $retstr;
	my $tmp;
	my $whichVal;
	if (!defined $val) {$count = 0;} # inputOnlys, set it to any value
	#print "MFVEC3F field $field val @{$val} has $count INIT\n";
	if ($count > 0) {
		#print "MALLOC MFVEC3d field $field val @{$val} has $count INIT\n";

		$retstr = "$field.p = MALLOC (struct SFVec3d *, sizeof(struct SFVec3d)*$count);\n";
		for ($tmp=0; $tmp<$count; $tmp++) {
			my $arline = @{$val}[$tmp];
			for ($whichVal = 0; $whichVal < 3; $whichVal++) {
				$retstr = $retstr. "\n\t\t\t$field.p[$tmp].c[$whichVal] = ".@{$arline}[$whichVal]."; ";
			}
		}
		$retstr = $retstr."\n\t\t\t$field.n=$count;";
	} else {
		$retstr = "$field.n=0; $field.p=0";
	}
	return $retstr;
}


###########################################################
package VRML::Field::SFVec3f;
sub cstruct {return "struct SFVec3f { float c[3]; };"}
sub ctype {return "struct SFVec3f " . ($_[1] || "")}
sub cInitialize {
        my ($this,$field,$val) = @_;
        if (!defined $val) {print "undefined in SFVec3f\n"} # inputOnlys, set it to any value
        # get the actual value and ensure that it is a float
        my $av0 = "@{$val}[0]";
        my $av1 = "@{$val}[1]";
        my $av2 = "@{$val}[2]";
        my $pv;
        $pv = index $av0, "."; if ($pv < 0) { $av0 = $av0.".0f"; } else { $av0 = $av0."f"; }
        $pv = index $av1, "."; if ($pv < 0) { $av1 = $av1.".0f"; } else { $av1 = $av1."f"; }
        $pv = index $av2, "."; if ($pv < 0) { $av2 = $av2.".0f"; } else { $av2 = $av2."f"; }
        # print "SFVec3f, field value now is $av0 $av1 $av2\n";

        return  "$field.c[0] = $av0;".
                "$field.c[1] = $av1;".
		"$field.c[2] = $av2";
}


package VRML::Field::MFVec3f;
our @ISA="VRML::Field::Multi";

sub cInitialize {
	my ($this,$field,$val) = @_;
	my $count = ref $val eq "ARRAY" ? @{$val} : 0;
	my $retstr;
	my $tmp;
	my $whichVal;

	#print "MFVEC3F field $field val @{$val} has $count INIT\n";
	if ($count > 0) {
		#print "MALLOC MFVEC3F field $field val @{$val} has $count INIT\n";

		$retstr = "$field.p = MALLOC (struct SFVec3f *, sizeof(struct SFVec3f)*$count);\n";
		for ($tmp=0; $tmp<$count; $tmp++) {
			my $arline = @{$val}[$tmp];
			for ($whichVal = 0; $whichVal < 3; $whichVal++) {
                                # get the actual value and ensure that it is a float
                                my $av = "@{$arline}[$whichVal]";
                                my $pv = index $av, ".";
                                if ($pv < 0) { $av = $av.".0f"; } else { $av = $av."f"; }
                                $retstr = $retstr. "\n\t\t\t$field.p[$tmp].c[$whichVal] = $av; ";
			}
		}
		$retstr = $retstr."\n\t\t\t$field.n=$count;";
	} else {
		$retstr = "$field.n=0; $field.p=0";
	}
	return $retstr;
}








###########################################################
package VRML::Field::SFVec4d;
our @ISA="VRML::Field";
sub cstruct {return "struct SFVec4d { double c[4]; };"}
sub ctype {return "struct SFVec4d " . ($_[1] || "");}
sub cInitialize {
	my ($this,$field,$val) = @_;
	if (!defined $val) {print "undefined in SFVec4d\n"} # inputOnlys, set it to any value
	return 	"$field.c[0] = @{$val}[0];".
		"$field.c[1] = @{$val}[1];".
		"$field.c[2] = @{$val}[2];";
}


package VRML::Field::MFVec4d;
our @ISA="VRML::Field::Multi";

sub cInitialize {
	my ($this,$field,$val) = @_;
	my $count = @{$val};
	my $retstr;
	my $tmp;
	my $whichVal;
	if (!defined $val) {$count = 0;} # inputOnlys, set it to any value
	#print "MFVEC3F field $field val @{$val} has $count INIT\n";
	if ($count > 0) {
		#print "MALLOC MFVEC4d field $field val @{$val} has $count INIT\n";

		$retstr = "$field.p = MALLOC (struct SFVec4d *, sizeof(struct SFVec4d)*$count);\n";
		for ($tmp=0; $tmp<$count; $tmp++) {
			my $arline = @{$val}[$tmp];
			for ($whichVal = 0; $whichVal < 4; $whichVal++) {
				$retstr = $retstr. "\n\t\t\t$field.p[$tmp].c[$whichVal] = ".@{$arline}[$whichVal]."; ";
			}
		}
		$retstr = $retstr."\n\t\t\t$field.n=$count;";
	} else {
		$retstr = "$field.n=0; $field.p=0";
	}
	return $retstr;
}


###########################################################
package VRML::Field::SFVec4f;
our @ISA="VRML::Field";
sub cstruct {return "struct SFVec4f { float c[4]; };"}
sub ctype {return "struct SFVec4f " . ($_[1] || "");}
sub cInitialize {
	my ($this,$field,$val) = @_;
	if (!defined $val) {print "undefined in SFVec4f\n"} # inputOnlys, set it to any value
	return 	"$field.c[0] = @{$val}[0];".
		"$field.c[1] = @{$val}[1];".
		"$field.c[2] = @{$val}[2];".
		"$field.c[3] = @{$val}[3];";
}


package VRML::Field::MFVec4f;
our @ISA="VRML::Field::Multi";

sub cInitialize {
	my ($this,$field,$val) = @_;
	my $count = @{$val};
	my $retstr;
	my $tmp;
	my $whichVal;
	if (!defined $val) {$count = 0;} # inputOnlys, set it to any value
	#print "MFVEC3F field $field val @{$val} has $count INIT\n";
	if ($count > 0) {
		#print "MALLOC MFVEC4f field $field val @{$val} has $count INIT\n";

		$retstr = "$field.p = MALLOC (struct SFVec4f *, sizeof(struct SFVec4f)*$count);\n";
		for ($tmp=0; $tmp<$count; $tmp++) {
			my $arline = @{$val}[$tmp];
			for ($whichVal = 0; $whichVal < 4; $whichVal++) {
				$retstr = $retstr. "\n\t\t\t$field.p[$tmp].c[$whichVal] = ".@{$arline}[$whichVal]."; ";
			}
		}
		$retstr = $retstr."\n\t\t\t$field.n=$count;";
	} else {
		$retstr = "$field.n=0; $field.p=0";
	}
	return $retstr;
}





###########################################################
package VRML::Field::FreeWRLPTR;
our @ISA="VRML::Field";

sub ctype {return "void * " . ($_[1] || "")}
sub cInitialize {
	my ($this,$field,$val) = @_;
	if (!defined $val) {$val = 0} # inputOnlys, set it to any value
	if ($field eq "tmp2->_parentResource") {
		return "$field = getInputResource()";
	} else {
		return "$field = $val";
	}
}

###########################################################
package VRML::Field::FreeWRLThread;
our @ISA="VRML::Field";

sub ctype {return "pthread_t " . ($_[1] || "")}
sub cInitialize {
	my ($this,$field,$val) = @_;
		return "$field = _THREAD_NULL_";
}

###########################################################
###########################################################
###########################################################
###########################################################
###########################################################
package VRML::Field::Multi;
our @ISA="VRML::Field";


sub ctype {
	my $r = (ref $_[0] or $_[0]);
	$r =~ s/VRML::Field::MF//;
	return "struct Multi_$r " . ($_[1] || "");
}
sub cstruct {
	my $r = (ref $_[0] or $_[0]);
	my $t = $r;
	$r =~ s/VRML::Field::MF//;
	$t =~ s/::MF/::SF/;
	my $ct = $t->ctype;
	return "struct Multi_$r { int n; $ct *p; };"
}


1;
