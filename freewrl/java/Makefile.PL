# Copyright (C) 1998 Tuomas J. Lukka, 1999 John Stewart CRC Canada.
# DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
# See the GNU Library General Public License (file COPYING in the distribution)
# for conditions of use and redistribution.

BEGIN {
    eval "use 5.004";
    warn "VRML::Browser requires Perl v5.0.4 or later -- go to http://www.perl.com"
    if $@;
}

use ExtUtils::MakeMaker;
require '../Config.pm';

WriteMakefile(
			  NAME => 'VRML::Java',
			  dynamic_lib => {
							  OTHERLDFLAGS => $VRML::Config::vrml_config{LDFLAGS},
							  LDDLFLAGS => $VRML::Config::vrml_config{LDDLFLAGS}
							 },
);

{
	package MY;

	sub postamble {
		return join("\n",
					qq{\# --- Java Archive\n},
					qq{install ::},
					qq{\tcd classes && \$(MAKE) install\n}
				   );
	}

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
}
