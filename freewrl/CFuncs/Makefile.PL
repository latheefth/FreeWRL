use ExtUtils::MakeMaker;

# See lib/ExtUtils/MakeMaker.pm for details of how to influence
# the contents of the Makefile that is written.

WriteMakefile(
    'NAME'              => "VRML::CFuncs",
    'CCFLAGS'		=> "",
    'DEFINE'		=> "",
    'INC'		=> "",
    'LIBS'		=> ""
);

sub MY::post_initialize {
    return q /
LIBDIR  = 
    /;
}

sub MY::c_o {
    q /
%.o: %.c
	$(CCCMD) -I$(PERL_INC) $(CCFLAGS) -o $@ $(LIBDIR) $< $(LIBS)
    /;
}

## Override all this stuff
## (see perldoc ExtUtils::MakeMaker and ExtUtils::MM_Unix)

sub MY::dynamic { "" }
sub MY::dynamic_bs { "" }
sub MY::dynamic_lib { "" }
sub MY::install { "" }
sub MY::static { "" }
sub MY::static_lib { "" }
sub MY::test { "" }
sub MY::top_targets { "" }
sub MY::xs_c { "" }
sub MY::xs_cpp { "" }
sub MY::xs_o { "" }

## C_FILES and O_FILES are generated automatically by MakeMaker

sub MY::postamble {
    return q /

vpath %.c

all :: ${O_FILES}

    /;
}
