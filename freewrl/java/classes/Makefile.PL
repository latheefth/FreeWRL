# Copyright (C) 1998 Tuomas J. Lukka, 1999 John Stewart CRC Canada.
# DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
# See the GNU Library General Public License (file COPYING in the distribution)
# for conditions of use and redistribution.

BEGIN {
    require '../../vrml.conf';
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

WriteMakefile(
    NAME => 'EAIJava'
);

## The Netscape jar file containing the package netscape.security is
## required to compile vrml/external/Browser.java.

sub MY::post_initialize {
    my $post_init = "";
    if ($VRML_CONFIG{JAVA_INST} && $VRML_CONFIG{NETSCAPE_INST}) {
	my $class_re = qr/^.*\.java\z/;
	my $jar_re = qr/^.*\.jar\z/;
	my $source = findFiles("vrml", $class_re,"\\\n"); 
	my $netscape_classpath =
	    findFiles($VRML_CONFIG{NETSCAPE_INST}, $jar_re, ":");

	my $dir_re = qr/^classes\z/;
	my $netscape_class_dir =
	    findFiles($VRML_CONFIG{NETSCAPE_INST}, $dir_re, "");

	
	$post_init = join("\n",
	    qq{\n},
q{JAR_FILE=vrml.jar
SOURCE=\\},
	    $source,
	    qq{\n},
q{CLASSES=$(SOURCE:.java=.class)
},
	    qq{NETSCAPE_CLASSPATH=$netscape_classpath},
	    qq{NETSCAPE_CLASS_DIR=$netscape_class_dir},
q{
JFLAGS=-classpath .:$(NETSCAPE_CLASSPATH)});
    }
    return($post_init);
}

## NB.  Since this script is not making a Perl module, a great many of
## the overridable functions belonging to ExtUtils::MM_Unix have been
## overridden.
## A relatively sparse Makefile to handle compiling the java code and
## updating the jar file is written by ExtUtils::MakeMaker.
##
## If Java is not installed, nothing will happen.

## Override ExtUtils::MM_Unix::install, since we don't need to install any
## Perl modules here.
sub MY::install {
    return("");
}

## Override ExtUtils::MM_Unix::installbin, since we don't need to install any 
## Perl modules here.
sub MY::installbin {
    return("");
}

## Override ExtUtils::MM_Unix::force, since we don't need to install any 
## Perl modules here.
sub MY::force {
    return("");
}

## Override ExtUtils::MM_Unix::c_o, since we don't need to install any 
## Perl modules here.
sub MY::c_o {
    return("");
}

## Override ExtUtils::MM_Unix::pasthru, since we don't need to install any
## Perl modules here.
sub MY::pasthru {
    return("");
}

## Override ExtUtils::MM_Unix::clean, since we don't need to install any 
## Perl modules here.
sub MY::clean {
    my $clean =
q{
clean ::
	-mv Makefile Makefile.old $(DEV_NULL)};
    return($clean);
}

## Override ExtUtils::MM_Unix::realclean, since we don't need to install any 
## Perl modules here.
sub MY::realclean {
    my $realclean =
q{
realclean purge :: clean
	rm -rf Makefile Makefile.old};
    return($realclean);
}

## Override and customize ExtUtils::MM_Unix::constants.
sub MY::constants {
    my($self) = @_;
    my(@m,$tmp);

    push(@m,
qq{
JAVAC=javac
JAR=jar
});
    for $tmp (qw/
              NAME DISTNAME VERSION
	      PERL FULLPERL PERL_LIB
              PREFIX PERL_ARCHLIB PERL_INC
              / ) {
        next unless defined $self->{$tmp};
        push(@m, "$tmp = $self->{$tmp}\n");
    }

    push(@m,
qq{
VERSION_MACRO = VERSION
DEFINE_VERSION = -D\$(VERSION_MACRO)=\\\"\$(VERSION)\\\"
PERL_MALLOC_DEF = -DPERL_EXTMALLOC_DEF -Dmalloc=Perl_malloc -Dfree=Perl_mfree -Drealloc=Perl_realloc -Dcalloc=Perl_calloc
});

    push(@m,
qq{
MAKEMAKER = $INC{'ExtUtils/MakeMaker.pm'}
MM_VERSION = $ExtUtils::MakeMaker::VERSION
});

    for $tmp (qw/
              INST_HTMLPRIVLIBDIR INSTALLHTMLPRIVLIBDIR
              INST_HTMLSITELIBDIR INSTALLHTMLSITELIBDIR
              INST_HTMLSCRIPTDIR  INSTALLHTMLSCRIPTDIR
              INST_HTMLLIBDIR     HTMLEXT
              INST_MAN1DIR        INSTALLMAN1DIR         MAN1EXT
              INST_MAN3DIR        INSTALLMAN3DIR         MAN3EXT
              /) {
        next unless defined $self->{$tmp};
        push @m, "$tmp = $self->{$tmp}\n";
    }

    for $tmp (qw(
                PERM_RW PERM_RWX
                )
             ) {
        my $method = lc($tmp);
        # warn "self[$self] method[$method]";
        push @m, "$tmp = ", $self->$method(), "\n";
    }

    push @m, q{
.NO_CONFIG_REC: Makefile
} if $ENV{CLEARCASE_ROOT};

    # why not q{} ? -- emacs
    push @m, qq{
# work around a famous dec-osf make(1) feature(?):
makemakerdflt: all

.SUFFIXES: .java .class .jar

# Nick wanted to get rid of .PRECIOUS. I don't remember why. I seem to recall, that
# some make implementations will delete the Makefile when we rebuild it. Because
# we call false(1) when we rebuild it. So make(1) is not completely wrong when it
# does so. Our milage may vary.
# .PRECIOUS: Makefile    # seems to be not necessary anymore

.PHONY: all config static dynamic test linkext manifest

# Where is the Config information that we are using/depend on
CONFIGDEP = \$(PERL_ARCHLIB)/Config.pm \$(PERL_INC)/config.h
};

    $tmp = $self->export_list;
    push( @m,
qq{
EXPORT_LIST = $tmp
});

    return(join('',@m));
}


## Override ExtUtils::MM_Unix::xs_c, since we don't need to install any 
## Perl modules here.
sub MY::xs_c {
    return("");
}

## Override ExtUtils::MM_Unix::xs_o, since we don't need to install any 
## Perl modules here.
sub MY::xs_o {
    return("");
}

## Override ExtUtils::MM_Unix::dynamic, since we don't need to install any 
## Perl modules here.
sub MY::dynamic {
    return("");
}


## Override ExtUtils::MM_Unix::dynamic_bs, since we don't need to install any 
## Perl modules here.
sub MY::dynamic_bs {
    return("");
}

## Override ExtUtils::MM_Unix::dynamic_lib, since we don't need to install any 
## Perl modules here.
sub MY::dynamic_lib {
    return("");
}

## Override and customize ExtUtils::MM_Unix::top_targets:
sub MY::top_targets {
    my $top_targets = "";
    if ($VRML_CONFIG{JAVA_INST} && $VRML_CONFIG{NETSCAPE_INST}) {
        $top_targets =
q{
vpath %.java vrml/*

all: pure_all

pure_all: $(JAR_FILE)

%.class: %.java
	$(JAVAC) $(JFLAGS) $<

$(JAR_FILE): $(CLASSES)
	$(JAR) cf $(JAR_FILE) $(CLASSES)

install: install_jar

install_jar: $(JAR_FILE)
	cp $(JAR_FILE) $(NETSCAPE_CLASS_DIR)
}
    }
    return($top_targets);
}

## Override and customize ExtUtils::MM_Unix::test - we have no tests, so
## don't bother looking for them.
sub MY::test {
# --- Test and Installation Sections ---

    my($self, %attribs) = @_;
    my(@m);
    push(@m,
	qq{test ::},
	qq{\t$self->{NOECHO}echo 'No tests defined for \$(NAME) extension.'\n}
    );

    return(join("\n", @m));
}

## The ExtUtils::MM_Unix dist methods have not been touched.
## Would it be useful to customize them???

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
