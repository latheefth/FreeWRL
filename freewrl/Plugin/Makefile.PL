# Copyright (C) 1998 Tuomas J. Lukka 1999, John Stewart CRC Canada.
# DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
# See the GNU Library General Public License (file COPYING in the distribution)
# for conditions of use and redistribution.

BEGIN {
	if ($^V lt v5.6.0) {
		warn "Warning: module $moduleName was created under Perl v5.6.0 or greater.\n";
	}
}

use ExtUtils::MakeMaker;
require '../Config.pm';

# See lib/ExtUtils/MakeMaker.pm for details of how to influence
# the contents of the Makefile that is written.

$moduleName = "VRML::PluginGlue";

WriteMakefile(
	  NAME => $moduleName,
	  VERSION_FROM => "PluginGlue.pm", # finds $VERSION
	  PREREQ_PM => {},		# e.g., Module::Name => 1.1
	  LIBS => "",			# e.g., '-lm'
	  DEFINE => $VRML::Config::vrml_config{FREEWRL_DEFINE},
	  dynamic_lib => {
		  OTHERLDFLAGS => $VRML::Config::vrml_config{LDFLAGS},
		  LDDLFLAGS => $VRML::Config::vrml_config{LDDLFLAGS}
			 },
	  INC => "-ICFuncs"
 );

my $install = 0;

{
	package MY;

	sub post_initialize {
		use Cwd;
		my($ver, $nc);
		my $dir = getcwd();
		my $post_init = "";


		if ($VRML::Config::vrml_config{NETSCAPE_INST}) {
			$install = 1;
			$post_init = join(
			  "\n",
			  qq{\nPLUGIN=$dir/netscape},
			  qq{LIB_DIR=$dir/netscape/_lib},
			  qq{PLUGIN_DIR=$VRML::Config::vrml_config{NETSCAPE_PLUGINS}}
			 );
		}

		return($post_init);
	}


	sub post_constants {
		my $post_const = "";

		$post_const = join("\n",
			   qq{\n\n\#ifeq (\$(OSNAME),linux)},
			   qq{DEFINE+=-DXP_UNIX=1 -D_GNU_SOURCE=1\n},
			   qq{INC+= -I/usr/X11R6/include -I/usr/X11/include\n},
			   qq{\#endif\n\n},
			   qq{vpath %.c .:./CFuncs:../CFuncs},
			   qq{CFUNCS_SRC:=\$(notdir \$(wildcard ./CFuncs/*.c))},
			   qq{OBJECT+=\$(CFUNCS_SRC:%c=%o)}
		  );

		return ($post_const);
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

	sub c_o {
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


	sub postamble {
		my $postamble = "";

		if ($install) {
			my $defines = "LIB_DIR=\$(LIB_DIR) OSNAME=\$(OSNAME) PLUGIN_DIR=\$(PLUGIN_DIR) CC=\$(CC)";
			$postamble = join("\n",
			  qq{\n\# --- FreeWRL web browser plugin:},
			  qq{all :: plugin\n},
			  qq{plugin ::},
			  qq{\tcd \$(PLUGIN) && \$(MAKE) all $defines\n},
			  qq{install ::},
			  qq{\tcd \$(PLUGIN) && \$(MAKE) install $defines\n},
			  qq{clean ::},
			  qq{\tcd \$(PLUGIN) && \$(MAKE) clean LIB_DIR=\$(LIB_DIR)\n},
			  qq{realclean ::},
			  qq{\tcd \$(PLUGIN) && \$(MAKE) clean PLUGIN_DIR=\$(PLUGIN_DIR)\n},
			  qq{distclean :: realclean\n}
			 );

		}
		return($postamble);
	}
}
