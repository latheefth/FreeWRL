# Copyright (C) 1998 Tuomas J. Lukka, 2002 John Stewart, Ayla Khan CRC Canada
# DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
# See the GNU Library General Public License (file COPYING in the distribution)
# for conditions of use and redistribution, EXCEPT on the files
# which belong under the Mozilla public license.
#
# $Id$

my $moduleName = 'VRML::JS';

##require '../vrml.conf';


if ($^V lt v5.6.0) {
    warn "Warning: module $moduleName was created under Perl v5.6.0.\n";
}


use ExtUtils::MakeMaker;

WriteMakefile(
			  NAME => $moduleName,
			  VERSION_FROM	=> 'JS.pm', # finds $VERSION
			  # You have to explain here how to find your opengl libs.
			  ##LIBS => $VRML_CONFIG{JS_LIBS},
			  LIBS => "",
			  ##INC => $VRML_CONFIG{JS_INC},
			  INC => "",
			  dynamic_lib => {
							  OTHERLDFLAGS => $VRML_CONFIG{LDFLAGS},
							  LDDLFLAGS => $VRML_CONFIG{LDDLFLAGS}
							 },
			  DEFINE => "",
			  CCFLAGS => "",
			  OPTIMIZE => '-O2'
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
