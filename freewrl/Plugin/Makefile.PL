# Copyright (C) 1998 Tuomas J. Lukka 1999, John Stewart CRC Canada.
# DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
# See the GNU Library General Public License (file COPYING in the distribution)
# for conditions of use and redistribution.

$moduleName = "VRML::PluginGlue";

require '../vrml.conf';

if ($^V lt v5.6.0) {
    warn "Warning: module $moduleName was created under Perl v5.6.0.\n";
}

use ExtUtils::MakeMaker;
# See lib/ExtUtils/MakeMaker.pm for details of how to influence
# the contents of the Makefile that is written.
WriteMakefile(
			  'NAME' => $moduleName,
			  'VERSION_FROM' => "PluginGlue.pm", # finds $VERSION
			  'PREREQ_PM' => {},	# e.g., Module::Name => 1.1
			  'LIBS' => [''],	# e.g., '-lm'
#'DEFINE' => "",	# e.g., '-DHAVE_SOMETHING'
			  'INC' => "-ICFuncs"
			 );

my $install = 0;

sub MY::post_initialize {
    use Cwd;
    my($ver, $nc);
    my $dir = getcwd();
    my $post_init = "";


    if ($VRML_CONFIG{NETSCAPE_INST}) {
		$install = 1;
		$post_init = join(
						  "\n",
						  qq{\nPLUGIN=$dir/netscape},
						  qq{LIB_DIR=$dir/netscape/_lib},
						  qq{PLUGIN_DIR=$VRML_CONFIG{NETSCAPE_PLUGINS}}
						 );
	} elsif ($VRML_CONFIG{MOZILLA_INST}) {
		$install = 1;
		$post_init = join(
						  "\n",
						  qq{\nPLUGIN=$dir/mozilla},
						  qq{LIB_DIR=$dir/mozilla/_lib},
						  qq{PLUGIN_DIR=$VRML_CONFIG{MOZILLA_PLUGINS}}
						 );
    }

    return($post_init);
}


sub MY::post_constants {
	my $post_const = "";

	$post_const =
q{

#ifeq ($(OSNAME),linux)

DEFINE+=\
-DXP_UNIX=1\
-D_GNU_SOURCE=1

INC+=\
-I/usr/X11R6/include\
-I/usr/X11/include	

#endif

vpath %.c .:./CFuncs

CFUNCS_SRC:=$(notdir $(wildcard ./CFuncs/*.c))
OBJECT+=$(CFUNCS_SRC:%c=%o)

};

	return ($post_const);
}

sub MY::c_o {
	my $c_o = "";

	$c_o =
q{

%.o : %.c
	$(CCCMD) -c $< $(CCCDLFLAGS) -I$(PERL_INC) $(DEFINE) -o $@

%.o : %.C
	$(CCCMD) -c $< $(CCCDLFLAGS) -I$(PERL_INC) $(DEFINE) -o $@

%.o : %.cpp
	$(CCCMD) -c $< $(CCCDLFLAGS) -I$(PERL_INC) $(DEFINE) -o $@

%.o : %.cxx
	$(CCCMD) -c $< $(CCCDLFLAGS) -I$(PERL_INC) $(DEFINE) -o $@

%.o : %.cc
	$(CCCMD) -c $< $(CCCDLFLAGS) -I$(PERL_INC) $(DEFINE) -o $@

};

	return ($c_o);
}


sub MY::postamble {
    my $postamble = "";

    if ($install) {
	$postamble =
q{
# --- FreeWRL web browser plugin:

all :: plugin

plugin ::
	@cd $(PLUGIN) && $(MAKE) all LIB_DIR=$(LIB_DIR) OSNAME=$(OSNAME) PLUGIN_DIR=$(PLUGIN_DIR) CC=$(CC)

install ::
	@cd $(PLUGIN) && $(MAKE) install LIB_DIR=$(LIB_DIR) OSNAME=$(OSNAME) PLUGIN_DIR=$(PLUGIN_DIR) CC=$(CC)

clean ::
	@cd $(PLUGIN) && $(MAKE) clean LIB_DIR=$(LIB_DIR)

realclean ::
	@cd $(PLUGIN) && $(MAKE) realclean PLUGIN_DIR=$(PLUGIN_DIR)

distclean :: realclean
};

    }
    return($postamble);
}
