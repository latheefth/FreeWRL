# Copyright (C) 1998 Tuomas J. Lukka, 1999 John Stewart CRC Canada.
# DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
# See the GNU Library General Public License (file COPYING in the distribution)
# for conditions of use and redistribution.

BEGIN {
    eval "use 5.004";
    warn "VRML::Browser requires Perl v5.0.4 or later -- go to http://www.perl.com"
    if $@;
}

use ExtUtils::MakeMaker;
require '../Config.pm';
use File::Find;
use Cwd;
use English;

WriteMakefile(
    NAME => 'VRML::Java',
    # Is there a better way to install data files?
    PM => { 'java.policy' => '$(INST_LIBDIR)/java.policy',
	    'vrml.jar' => '$(INST_LIBDIR)/vrml.jar'}
);

## Override and customize ExtUtils::MM_Unix::constants.
sub MY::post_constants {
    my $constants ="";
    if ($VRML::Config::vrml_config{JAVA_INST}) {
	my $class_re = qr/^.*\.java\z/;
	my $source = findFiles("vrml", $class_re,"\\\n");
	$constants .= qq{
JAVAC=javac
JAR=jar
SOURCE=\\
$source

CLASSES=\$(SOURCE:.java=.class)

JFLAGS=-classpath .
};
	if ($VRML::Config::vrml_config{NETSCAPE_CLASSES}) {
	    $constants .= qq{
NETSCAPE_CLASS_DIR=$VRML::Config::vrml_config{NETSCAPE_CLASSES}
};
	}
    }
    return "$constants";
}

## The non perl specific make rules
sub MY::postamble {
    $_ = q{
%.class: %.java
	$(JAVAC) $(JFLAGS) $<

vrml.jar: $(CLASSES)
	$(JAR) cf $@ `find vrml -name \*.class`
};
    if ($VRML::Config::vrml_config{NETSCAPE_CLASSES}) {
        $_ .= q{

install ::
	cp vrml.jar $(NETSCAPE_CLASS_DIR)
};
    }
}

## Helper subs:
sub findFiles {
    my $dir = shift(@_);
    local $stringPattern = shift(@_);
    my $join = shift(@_);
    local @fileList;

    # Set the variable $File::Find::dont_use_nlink if you're using AFS,
    # since AFS cheats.

    # for the convenience of &wanted calls, including -eval statements:
    use vars qw/*name *dir *prune/;
    *name   = *File::Find::name;
    *dir    = *File::Find::dir;
    *prune  = *File::Find::prune;

    if ($PERL_VERSION ge v5.6.0) {
	# Traverse desired filesystems
	File::Find::find(
	    {wanted => sub {
	    /$stringPattern/s &&
	    push @fileList, $File::Find::name;
	    }},
	    $dir
	);
    } else {
	sub wanted {
	    /$stringPattern/s &&
	    push @fileList, $File::Find::name;
	}
	# Traverse desired filesystems
	File::Find::find(\&wanted, $dir);
    }

    if ($#fileList < 0) {
	warn "No files matching $stringPattern could be found at $dir";
    }
    return wantarray ? @fileList : join($join, @fileList);
}
