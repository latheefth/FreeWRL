# Copyright (C) 1998 Tuomas J. Lukka 1999 John Stewart CRC Canada.
# DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
# See the GNU Library General Public License (file COPYING in the distribution)
# for conditions of use and redistribution.

print "\nMaking the Netscape Plugin.\n";
system "make -f Makefile.plugin";
print "Run \'make -f Makefile.plugin install\' as superuser to install npfreewrl.so\n";
print "in the system Netscape plugin directory\n";
print "(the path may need to be edited in Makefile.plugin).\n";
print "If you don't have superuser access, install locally.\n\n";

$moduleName = "VRML::PluginGlue";

if ($^V lt v5.6.0) {
    warn "Warning: module $moduleName was created under Perl v5.6.0.\n";
}


use ExtUtils::MakeMaker;

# See lib/ExtUtils/MakeMaker.pm for details of how to influence
# the contents of the Makefile that is written.
WriteMakefile(
    'NAME'		=> $moduleName,
    'VERSION_FROM'	=> "PluginGlue.pm", # finds $VERSION
    'PREREQ_PM'		=> {}, # e.g., Module::Name => 1.1
    'LIBS'		=> [''], # e.g., '-lm'
    'DEFINE'		=> '', # e.g., '-DHAVE_SOMETHING'
    'INC'		=> '-Iinclude', # e.g., '-I/usr/include/other'
);

#
#sub MY::install {
#}
#
#sub MY::top_targets {
#}
#

# Need to find a way to include this!!!
sub MY::postamble {
'
plugin:
	$(MAKE) -f Makefile.plugin
'
}
