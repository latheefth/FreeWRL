BEGIN {
	if ($^V lt v5.6.0) {
		warn "Warning: module $moduleName was created under Perl v5.6.0 or greater.\n";
	}
}

use ExtUtils::MakeMaker;
require '../Config.pm';

# See lib/ExtUtils/MakeMaker.pm for details of how to influence
# the contents of the Makefile that is written.

WriteMakefile(
			  NAME => "VRML::CFuncs",
			  CCFLAGS => $VRML::Config::vrml_config{FREEWRL_CCFLAGS},
			  DEFINE => $VRML::Config::vrml_config{FREEWRL_DEFINE},
			  INC => $VRML::Config::vrml_config{FREEWRL_INC},
			  LIBS => $VRML::Config::vrml_config{FREEWRL_LIBS}
			 );

sub MY::post_initialize { return qq{\nLIBDIR=\n}; }

sub MY::c_o {
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
