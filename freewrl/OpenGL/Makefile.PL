# Makefile.PL for VRML::OpenGL module.

use ExtUtils::MakeMaker;

WriteMakefile(
	NAME => 'VRML::OpenGL',
    
    LIBS      => $VRML_CONFIG{FREEWRL_LIBS},

    DEFINE => $VRML_CONFIG{FREEWRL_DEFINE},
    INC => $VRML_CONFIG{FREEWRL_INC},
);

