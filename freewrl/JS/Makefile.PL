# Copyright (C) 1998 Tuomas J. Lukka, 2002 John Stewart, Ayla Khan CRC Canada
# DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
# See the GNU Library General Public License (file COPYING in the distribution)
# for conditions of use and redistribution, EXCEPT on the files
# which belong under the Mozilla public license.
#
# $Id$
#
#
# a.out: js.c
# 	 cc -I js/Linux_All_DBG.OBJ/ -Ijs/ js.c -L js/Linux_All_DBG.OBJ/ -ljs -lm

my $moduleName = 'VRML::JS';

require '../vrml.conf';


if ($^V lt v5.6.0) {
    warn "Warning: module $moduleName was created under Perl v5.6.0.\n";
}


use ExtUtils::MakeMaker;
use Cwd qw(getcwd chdir);

chomp ($pwd = getcwd());

# Add several possible places for -L.

## Check for required Javascript libraries,
## if they don't exist, make them.

$SRCDIR="js/src";

print "Checking for the Mozilla JavaScript Reference library and application.\n";
if (-e "$SRCDIR/Linux_All_OPT.OBJ/libjs.a"
	&& -e "$SRCDIR/Linux_All_OPT.OBJ/libjs.so"
	&& -e "$SRCDIR/Linux_All_OPT.OBJ/js") {
    print "The Javascript library and application were found.\n";
} else {
    print "The Javascript library and application were not found - make them.\n";
    chdir("$SRCDIR");
    system("make -f Makefile.ref BUILD_OPT=1");
    chdir($pwd);
}

use ExtUtils::MakeMaker;

## get rid of -W eventually
my $ccflags = q{-Wall\
-W\
-Wfloat-equal\
-Wundef\
-Wshadow\
-Wpointer-arith\
-Wconversion\
-Wbad-function-cast\
-Wcast-qual\
-Wsign-compare\
-Wmissing-prototypes\
-Wmissing-declarations
};

my $inc = join(' ', $VRML_CONFIG{JS_INC}, q{-ICFuncs}, q{-I../CFuncs});
my $libs = join(' ', $VRML_CONFIG{JS_LIBS}, q{-lm});

## OSNAME = linux, irix, ...

WriteMakefile(
			  NAME => $moduleName,
			  VERSION_FROM	=> 'JS.pm', # finds $VERSION
			  # You have to explain here how to find your opengl libs.
			  LIBS => $libs,
			  INC => $inc,
			  DEFINE => '-DXP_UNIX', # assume Unix and related OSs for now
			  CCFLAGS => $ccflags,
			  OPTIMIZE => '-O2'
			 );

sub MY::post_constants {
	my $post_const = "";

	$post_const =
q{

vpath %.c .:CFuncs:../CFuncs
vpath %.o .:CFuncs:../CFuncs

CFUNC_SRC:=\
$(wildcard CFuncs/*.c)\
../CFuncs/LinearAlgebra.c
OBJECT+=$(CFUNC_SRC:%c=%o)

};

	return ($post_const);
}

sub MY::c_o {
	my $c_o = "";

	$c_o =
q{

%.o : %.c
	$(CCCMD) -c $< $(CCCDLFLAGS) -I$(PERL_INC) $(DEFINE) -o $@

%.o : %.C
	$(CCCMD) -c $< $(CCCDLFLAGS) -I$(PERL_INC) $(DEFINE) -o $@

%.o : %.cpp
	$(CCCMD) -c $< $(CCCDLFLAGS) -I$(PERL_INC) $(DEFINE) -o $@

%.o : %.cxx
	$(CCCMD) -c $< $(CCCDLFLAGS) -I$(PERL_INC) $(DEFINE) -o $@

%.o : %.cc
	$(CCCMD) -c $< $(CCCDLFLAGS) -I$(PERL_INC) $(DEFINE) -o $@

};

	return ($c_o);
}

sub MY::postamble {
	my $postamble = "";

	$postamble =
q{
clean ::
	rm -rf $(OBJECT)
};

}
