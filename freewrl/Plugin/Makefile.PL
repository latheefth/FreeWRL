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
    'NAME'		=> "VRML::PluginGlue",
    'VERSION_FROM'	=> 'PluginGlue.pm', # finds $VERSION
    'PREREQ_PM'		=> {}, # e.g., Module::Name => 1.1
    'LIBS'		=> [''], # e.g., '-lm'
    'DEFINE'		=> '', # e.g., '-DHAVE_SOMETHING'
    'INC'		=> "-Inetscape/include"
);

sub MY::post_initialize {
    use Cwd;
    my($ver, $nc);
    my $dir = getcwd();
    my $post_init = "";

    if ($VRML_CONFIG{NETSCAPE_INST}) {
	$post_init = join("\n",
	    qq{\nNP=$dir/netscape},
	    qq{INST_NP=$dir/netscape/lib},
	    qq{INSTALLNP=$VRML_CONFIG{NETSCAPE_PLUGINS}}
	);

    }
    return($post_init);
}

sub MY::postamble {
    my $postamble = "";

    if ($VRML_CONFIG{NETSCAPE_INST}) {
	$postamble = 
q{
# --- Netscape Plugin:

all :: plugin

plugin ::
	@cd $(NP) && $(MAKE) all

install_plugin ::
	@cd $(NP) && $(MAKE) install INSTALLNP=$(INSTALLNP)

clean ::
	@cd $(NP) && $(MAKE) clean
};

    }
    return($postamble);
}

