use ExtUtils::MakeMaker;


# See lib/ExtUtils/MakeMaker.pm for details of how to influence
# the contents of the Makefile that is written.

WriteMakefile(
			  NAME => "VRML::CFuncs",
			  CCFLAGS => $VRML_CONFIG{FREEWRL_CCFLAGS},
			  DEFINE => $VRML_CONFIG{FREEWRL_DEFINE},
			  INC => $VRML_CONFIG{FREEWRL_INC},
			  dynamic_lib => {
							  OTHERLDFLAGS => $VRML_CONFIG{LDFLAGS},
							  LDDLFLAGS => $VRML_CONFIG{LDDLFLAGS}
							 },
			  LIBS => $VRML_CONFIG{FREEWRL_LIBS}
			 );

sub MY::post_initialize { return qq{\nLIBDIR=\n}; }

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
	##my $cxx_rule = qq{\t\$(CCCMD) -c \$< \$(CCCDLFLAGS) -I\$(PERL_INC) \$(PASTHRU_DEFINE) \$(DEFINE) -o \$@\n};
	my $cxx_rule = qq{\t\$(CCCMD) \$< \$(CCCDLFLAGS) -I\$(PERL_INC) \$(PASTHRU_DEFINE) \$(DEFINE) -o \$@\n};

	my $c_o = join(
				   "\n",
				   qq{\n%.i : %.c},
				   qq{\tgcc -E -c \$(PASTHRU_INC) \$(INC)\\},
				   qq{\t\$(CCFLAGS) \$(OPTIMIZE)\\},
				   qq{\t\$(PERLTYPE) \$(MPOLLUTE) \$(DEFINE_VERSION)\\},
				   qq{\t\$(XS_DEFINE_VERSION) \$(CCCDLFLAGS) -I\$(PERL_INC) \$(PASTHRU_DEFINE) \$(DEFINE) \$< > \$@\n},
				   qq{\n%.s : %.c},
				   qq{\t\$(CCCMD) -S \$(CCCDLFLAGS) -I\$(PERL_INC) \$(PASTHRU_DEFINE) \$(DEFINE) -o \$@\n},
				   qq{\n%.o : %.c},
				   $cxx_rule,
				   qq{%.o : %.C},
				   $cxx_rule,
				   qq{%.o : %.cpp},
				   $cxx_rule,
				   qq{%.o : %.cxx},
				   $cxx_rule,
				   qq{%.o : %.cc},
				   $cxx_rule
				  );

	return $c_o;
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
	my $postamble = join(
						 "\n",
						 qq{\nvpath %.c\n},
						 qq{all :: \${O_FILES}\n}
						);
	return $postamble;
}
