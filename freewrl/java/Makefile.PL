# Copyright (C) 1998 Tuomas J. Lukka, 1999 John Stewart CRC Canada.
# DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
# See the GNU Library General Public License (file COPYING in the distribution)
# for conditions of use and redistribution.

BEGIN {
    require '../vrml.conf';
    eval "use 5.004";
    warn "VRML::Browser requires Perl v5.0.4 or later -- go to http://www.perl.com"
    if $@;
}

use ExtUtils::MakeMaker;
use File::Find;
use Cwd;
use English;

if ($PERL_VERSION lt v5.6.0) {
    my $dir = getcwd();
    warn "Warning: $dir/$PROGRAM_NAME was created under Perl v5.6.0.\n";
}

sub MY::postamble {
    return q{
# --- Java Archive

install ::
	@cd classes && $(MAKE) install
};
}

WriteMakefile(
    NAME => 'Java'
);
