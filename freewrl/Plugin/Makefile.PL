# Copyright (C) 1998 Tuomas J. Lukka 1999 John Stewart CRC Canada.
# DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
# See the GNU Library General Public License (file COPYING in the distribution)
# for conditions of use and redistribution.

$moduleName = "VRML::PluginGlue";

require '../vrml.conf';

if ($^V lt v5.6.0) {
    warn "Warning: module $moduleName was created under Perl v5.6.0.\n";
}

makePlugin(0);

use ExtUtils::MakeMaker;
# See lib/ExtUtils/MakeMaker.pm for details of how to influence
# the contents of the Makefile that is written.
WriteMakefile(
    'NAME'		=> "VRML::PluginGlue",
    'VERSION_FROM'	=> 'PluginGlue.pm', # finds $VERSION
    'PREREQ_PM'		=> {}, # e.g., Module::Name => 1.1
    'LIBS'		=> [''], # e.g., '-lm'
    'DEFINE'		=> '', # e.g., '-DHAVE_SOMETHING'
    'INC'		=> "$VRML_CONFIG{JAVA_INC} -I./netscape/include",
);

# Takes 1 argument:
#     0 if debugging is to be turned off
#     1 if debugging is to be turned on
sub makePlugin {
    use Cwd qw(getcwd chdir);

    my $debug = shift(@_);
    my $makeCmd;
    my $dir = getcwd();

    if ($debug) { $makeCmd = "make DEBUG=1"; }
    else { $makeCmd = "make"; }

    chdir "netscape";
    if (-e "Makefile") {
	print "\nMaking the Netscape Plugin.\n";
	system $makeCmd;
	chdir $dir;
    }

print <<INSTALL;

Run \'make install\' from directory Plugin/netscape as superuser
to install npfreewrl.so in the system Netscape plugin directory
(the path may need to be edited in Makefile.plugin).

If you don't have superuser access, install locally,
or set environment variable NPX_PLUGIN_PATH to point to the
directory where npfreewrl.so is installed.

INSTALL

    return;
}

