# Makefile.PL for VRML::OpenGL module.

use ExtUtils::MakeMaker;
require '../Config.pm';

my $inc = join(" ", $VRML::Config::vrml_config{FREEWRL_INC}, "-I../CFuncs");
my $objs = "../CFuncs/OpenGL_Utils.o";

WriteMakefile(
			  NAME => 'VRML::OpenGL',
			  LIBS => $VRML::Config::vrml_config{FREEWRL_LIBS},
			  DEFINE => $VRML::Config::vrml_config{FREEWRL_DEFINE},
			  INC => $inc,
			  dynamic_lib => {
							  OTHERLDFLAGS => $VRML::Config::vrml_config{LDFLAGS},
							  LDDLFLAGS => $VRML::Config::vrml_config{LDDLFLAGS}
							 },
			 );

{
	package MY;

	sub dynamic_lib {
		if ($VRML::Config::vrml_config{PLATFORM} =~ /macosx/i) {
			return "ARMAYBE = :
OTHERLDFLAGS = $VRML::Config::vrml_config{LDFLAGS}
INST_DYNAMIC_DEP =

\$\(INST_DYNAMIC\): \$\(OBJECT\) \$\(MYEXTLIB\) \$\(BOOTSTRAP\) \$\(INST_ARCHAUTODIR\)/.exists \$\(EXPORT_LIST\) \$\(PERL_ARCHIVE\) \$\(PERL_ARCHIVE_AFTER\) \$\(INST_DYNAMIC_DEP\)
	\$\(RM_F\) \$\@
	LD_RUN_PATH=\$\(LD_RUN_PATH\) \$\(LD\) \$\(LDFROM\) \$\(OTHERLDFLAGS\) -o \$\@ \$\(MYEXTLIB\) \$\(PERL_ARCHIVE\) \$\(LDLOADLIBS\) \$\(PERL_ARCHIVE_AFTER\) \$\(EXPORT_LIST\)
	\$\(CHMOD\) \$\(PERM_RWX\) \$\@\n";
		} else {
			return shift->SUPER::dynamic_lib(@_);
		}
	}


	sub const_loadlibs {
		my $loadlibs = shift->SUPER::const_loadlibs(@_);
		$loadlibs =~ s/(LD_RUN_PATH\s*=.*)$/$1:$VRML::Config::vrml_config{LIBJS_INST}/m;
		return $loadlibs;
	}


	sub post_constants {
		return qq{\nOBJECT+=$objs\n};
	}
}
