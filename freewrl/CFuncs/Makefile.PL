use ExtUtils::MakeMaker;

# See lib/ExtUtils/MakeMaker.pm for details of how to influence
# the contents of the Makefile that is written.

WriteMakefile(
    'NAME'              => "VRML::CFuncs",
    'CCFLAGS'		=> "",
    'DEFINE'		=> "",
    'INC'               => $VRML_CONFIG{FREEWRL_INC},
dynamic_lib => { 'OTHERLDFLAGS' => $VRML_CONFIG{LDFLAGS}, 'LDDLFLAGS' => $VRML_CONFIG{LDDLFLAGS} },
    'LIBS'		=> ""
);

sub MY::post_initialize {
	my $post_init =
q{
LIBDIR=
};
	return ($post_init);
}
sub MY::dynamic_lib {
"ARMAYBE = :
OTHERLDFLAGS = $VRML_CONFIG{LDFLAGS} 
INST_DYNAMIC_DEP =

\$\(INST_DYNAMIC\): \$\(OBJECT\) \$\(MYEXTLIB\) \$\(BOOTSTRAP\) \$\(INST_ARCHAUTODIR\)/.exists \$\(EXPORT_LIST\) \$\(PERL_ARCHIVE\) \$\(PERL_ARCHIVE_AFTER\) \$\(INST_DYNAMIC_DEP\)
	\$\(RM_F\) \$\@
	LD_RUN_PATH=\$\(LD_RUN_PATH\) \$\(LD\) \$\(LDFROM\) \$\(OTHERLDFLAGS\) -o \$\@ \$\(MYEXTLIB\) \$\(PERL_ARCHIVE\) \$\(LDLOAD LIBS\) \$\(PERL_ARCHIVE_AFTER\) \$\(EXPORT_LIST\)
	\$\(CHMOD\) \$\(PERM_RWX\) \$\@
"
}

sub MY::c_o {
	my $c_o =
q{
%.o: %.c
	$(CCCMD) -I$(PERL_INC) $(CCFLAGS) -o $@ $(LIBDIR) $< $(LIBS)
};
	return ($c_o);
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
	my $postamble =
q{

vpath %.c

all :: ${O_FILES}

};
	return ($postamble);
}
