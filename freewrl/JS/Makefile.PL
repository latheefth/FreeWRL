# Copyright (C) 1998 Tuomas J. Lukka, 2002 John Stewart, Ayla Khan CRC Canada
# DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
# See the GNU Library General Public License (file COPYING in the distribution)
# for conditions of use and redistribution, EXCEPT on the files
# which belong under the Mozilla public license.
#
# $Id$

BEGIN {
	if ($^V lt v5.6.0) {
		warn "Warning: module $moduleName was created under Perl v5.6.0 or greater.\n";
	}
}

my $moduleName = 'VRML::JS';

use ExtUtils::MakeMaker;
require '../Config.pm';

WriteMakefile(
			  NAME => $moduleName,
			  VERSION_FROM	=> 'JS.pm', # finds $VERSION
			  LIBS => "",
			  INC => "",
			  dynamic_lib => {
							  OTHERLDFLAGS => $VRML::Config::vrml_config{LDFLAGS},
							  LDDLFLAGS => $VRML::Config::vrml_config{LDDLFLAGS}
							 },
			  DEFINE => "",
			  CCFLAGS => "",
			  OPTIMIZE => '-O2'
			 );

{
	package MY;

	sub dynamic_lib {
		if ($VRML::Config::vrml_config{PLATFORM} =~ /Darwin/i) {
			return "ARMAYBE = :
OTHERLDFLAGS = $VRML::Config::vrml_config{LDFLAGS2}
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
