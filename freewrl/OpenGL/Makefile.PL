# Makefile.PL for VRML::OpenGL module.

use ExtUtils::MakeMaker;

WriteMakefile(
	NAME => 'VRML::OpenGL',
    
    LIBS      => $VRML_CONFIG{FREEWRL_LIBS},

    DEFINE => $VRML_CONFIG{FREEWRL_DEFINE},
    INC => $VRML_CONFIG{FREEWRL_INC},
dynamic_lib => { 'OTHERLDFLAGS' => $VRML_CONFIG{LDFLAGS}, 'LDDLFLAGS' => $VRML_CONFIG{LDDLFLAGS} },
);
sub MY::dynamic_lib {
"ARMAYBE = :
OTHERLDFLAGS = $VRML_CONFIG{LDFLAGS} 
INST_DYNAMIC_DEP =

\$\(INST_DYNAMIC\): \$\(OBJECT\) \$\(MYEXTLIB\) \$\(BOOTSTRAP\) \$\(INST_ARCHAUTODIR\)/.exists \$\(EXPORT_LIST\) \$\(PERL_ARCHIVE\) \$\(PERL_ARCHIVE_AFTER\) \$\(INST_DYNAMIC_DEP\)
	\$\(RM_F\) \$\@
	LD_RUN_PATH=\$\(LD_RUN_PATH\) \$\(LD\) \$\(LDFROM\) \$\(OTHERLDFLAGS\) -o \$\@ \$\(MYEXTLIB\) \$\(PERL_ARCHIVE\) \$\(LDLOADLIBS\) \$\(PERL_ARCHIVE_AFTER\) \$\(EXPORT_LIST\)
	\$\(CHMOD\) \$\(PERM_RWX\) \$\@
"
}

