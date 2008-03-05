# Copyright (C) 1998 Tuomas J. Lukka 1999, 2005, 2006 John Stewart CRC Canada.
# DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
# See the GNU Library General Public License (file COPYING in the distribution)
# for conditions of use and redistribution.

# Version of FreeWRL
my $VERSION = "V1.20.0";

# what OS are we running under?
my $OS = `uname`;
chomp($OS);

if ($OS eq "Linux") { print "This install is running on a Linux based computer\n";
} else { print "The uname command returned $OS - dont know what platform - assuming Linux\n"; }

my $HAVE_UBUNTU = "NO";
chomp ($pwd = getcwd());

# Do we need ldconfig ?
# If we are running under the control of dpkg (Debian, Ubuntu, Fink, ...)
# we do not need ldconfig...
my $NEED_LDCONFIG = 1;

# What compilers do we have?
my $JAVAC = `which javac`;
chomp($JAVAC);
my $JAR = `which jar`;
chomp($JAR);

if (($JAVAC eq "") || ($JAR eq "")) { 
	print "can not find a java compiler and archiver  - skipping java class making\n";
	$JAVAC = "";
}

# Detect if SELinux is activated
my $SELINUX = "NO";
if ($OS eq "Linux") {
	if (-e "/etc/selinux/config") {
		my $selinux = `grep ^SELINUX= /etc/selinux/config 2>/dev/null`;
		chomp($selinux);
		$selinux =~ s/^SELINUX=//;
		if ($selinux eq "enforcing") {
			$SELINUX = "YES";
		}
	}
}
print "SELINUX=$SELINUX\n";

# Some versions of Linux require the command 
# chcon -t texrel_shlib_t /usr/lib/libFreeWRL*so to be run.
# If SELinux is activated, search for chcon
my $CHCON = "";
if ($SELINUX eq "YES") {
	$CHCON = `which chcon`;
	if ($CHCON eq "") {print "chcon command not on system; will NOT run it on install.\n";}
	if ($CHCON ne "") {print "chcon command found, will run it on install.\n";}
	$CHCON =~ s/\s+$//;
}

# create the generated code/header files.
system "perl VRMLC.pm";

use Cwd qw(getcwd chdir);

# get the correct.conf file.
# Test different flavours of Linux
my $issue = `head -1 /etc/issue`;

my $ARCH = `uname -m`;
chomp($ARCH);
print "ARCH=$ARCH\n";

require './vrml.conf.linux';

if (($issue =~ /Fedora/) || ($issue =~ /RedHat/)) {
    print "Linux flavour: RedHat\n";

    # find out what the distribution is.
    my ($rpm_prefix,$sourcepackage, $rpmversion, $osname, $specfile) = 
	linux_system_info();
    if ( -f "$specfile") {
	print "RedHat specfile is: $specfile\n";
    }

    # add these keys for later use when making distribution (rpm) file
    $VRML_CONFIG{RPMPREFIX} = $rpm_prefix;
    $VRML_CONFIG{SOURCEPACKAGE} = $sourcepackage;
    $VRML_CONFIG{RPMVERSION} = $rpmversion;
    $VRML_CONFIG{OSNAME} = $osname;
    $VRML_CONFIG{SPECFILE} = $specfile;
    
}

# Can we add FreeWRL to the desktop?
$VRML_CONFIG{DESKTOP} = "NO";	# default value.
if (-e "/usr/share/applications") {
    if (-e "/usr/share/pixmaps") {
	$VRML_CONFIG{DESKTOP} = "/usr/share";
    }
}

# Default Mozilla / Firefox plugins directory
if ($issue =~ /Debian/) {
    print "Linux flavour: Debian\n";
    $VRML_CONFIG{NETSCAPE_INST} = "/usr/lib/mozilla-firefox/plugins";
    $NEED_LDCONFIG = 0;
} elsif ($issue =~ /Ubuntu/) {
    print "Linux flavour: Ubuntu\n";
    $NEED_LDCONFIG = 0;
} else {
}


$VRML_CONFIG{VERSION} = $VERSION;

# where are we installing fonts, etc?
if ("" eq $VRML_CONFIG{INSTALL_DIR}) {
	print "do not have an INSTALL_DIR defined, assuming /usr/share\n";
	$VRML_CONFIG{INSTALL_DIR} = "/usr/share";
}

# save this for JS/Makefile.PL
$VRML_CONFIG{PLATFORM} = $OS;

# Netscape / Mozilla
my $pkgconfig = `which pkg-config`;
if ($pkgconfig eq "") {
    print "No pkg-config in PATH\n";
}

## Netscape plugin API

my $pg_nspr = "";

if (!($pkgconfig eq "")) {
    $pg_nspr = `pkg-config --cflags xulrunner-nspr 2>0`;
    chomp($pg_nspr);
}

## Handle platform-specific compile options for the Javascript engine
## and others specific to FreeWRL.

my $pg_js = "";
my $jsCFLAGS = "";
my $jsLDFLAGS = "";
my $jsdir = "js1.5/src";
my $libFreeWRLjspath = "";
my $jsobjdir;
my $noStackProtector = "";
my @inc;
my @libs;
my $INCLUDED_JS = 0;

if (!($pkgconfig eq "")) {
    $pg_js = `pkg-config --cflags xulrunner-js 2>0`;
    chomp($pg_js);
}

if ($pg_js eq "") {
    if ($OS eq  "IRIX64") {
	$jsobjdir = "$jsdir/IRIX6.5";
    } elsif ($OS eq "SunOS") {
	$jsobjdir = "$jsdir/SunOS5.8";
    } else { ## linux used by default
	$jsobjdir = "$jsdir/Linux_All";
    }

    #location of the spidermonkey js engine.
    $jsobjdir .= "_OPT.OBJ";
    $libFreeWRLjspath = "./JS/".$jsobjdir."/libFreeWRLjs.so";
    $INCLUDED_JS = 1;
} else {

    $jsCFLAGS = $pg_js;
    $jsLDFLAGS = `pkg-config --libs xulrunner-js 2>0`;
    chomp($jsLDFLAGS);
    print "Javascript:\ncflags=$jsCFLAGS\nlibs=$jsLDFLAGS\n";
}

#print "the javascript engine should be located at: $libFreeWRLjspath\n";



######################
# Stack Protector flag. Some gcc compiler environments REQUIRE this flag.
# just check to see if it exists.

my $ok;
system ("echo \"int main(){}\" > sp.c");
$ok = system ("gcc -fno-stack-protector sp.c 2>0");
system ("rm sp.c");

# did this -no-stack-protector flag compile ok? If so, lets include it
if ($ok eq 0) { 
	print "adding -fno-stack-protector flag to compiles\n";
	system ("rm a.out");
	$noStackProtector = "-fno-stack-protector";
} else {
	print "this system does not use the -fno-stack-protector flag\n";
}

if ($INCLUDED_JS) {
    push @inc, $VRML_CONFIG{FREEWRL_INC}, "-IJS/$jsobjdir", "-IJS/$jsdir", "-ICFuncs -ICFrontEnd ";
} else {
    push @inc, $VRML_CONFIG{FREEWRL_INC}, "$jsCFLAGS", "-ICFuncs -ICFrontEnd ";
}
if ($INCLUDED_JS) {
    push @libs, $VRML_CONFIG{FREEWRL_LIBS}, "-LJS/$jsobjdir", "-lFreeWRLjs";
} else {
    push @libs, $VRML_CONFIG{FREEWRL_LIBS}, "$jsLDFLAGS";
}

# push build directory, for running standalone

$VRML_CONFIG{FREEWRL_INC} = join(' ', @inc);
$VRML_CONFIG{FREEWRL_LIBS} = join(' ', @libs);

if (defined $VRML_CONFIG{'USE_JAVA_HOME'})
{
	$VRML_CONFIG{JAVA} = $ENV{JAVA_HOME}.'/bin/java';
	$VRML_CONFIG{JAVA_INST} = $ENV{JAVA_HOME};
}

# print out the CFuncs/vrmlconf.c C header file.
open OUT, ">CFuncs/vrmlconf.h" or die("Couldn't write to CFuncs/vrmlconf.h");
print OUT "/* AUTOMATICALLY GENERATED BY TOP-LEVEL MAKEFILE.PL -- DO NOT EDIT */\n\n\n";

# lets write some generated #defines.

print OUT "#define BUILDDIR \"$pwd\"\n";
print OUT "#define FWVER \"",$VERSION,"\"\n";
print OUT "#define XMESSAGE \"",$VRML_CONFIG{BIN_DIR},"/FreeWRL_Message\"\n";
if ($VRML_CONFIG{MIDI_ENGINE}) {
	print OUT "#define REWIRE_SERVER \"",$VRML_CONFIG{BIN_DIR},"/freewrlReWireServer\"\n";
}
print OUT "#define SOUNDSERVERBINARY \"",$VRML_CONFIG{BIN_DIR},"/FreeWRL_SoundServer\"\n";

findAndPrintUtilities();


close OUT;



#######################################################################################
# write the Makefile
#######################################################################################

open MK, ">Makefile" or die("Couldn't write the Makefile");

writeMakefileInitialize();
writeVariables();
writeAllTarget();

writeMakefileLibFreeWRL();
writeMakefileCfrontend();

writeMakefileSoundEngine();
writeMakefileMIDI();
writeMakefileFreeWRLMessage();
write_linux_makefile_javascript();
write_linux_makefile_javaClasses();
writeMakefileBrowserPlugin();
writeMakeDebAndRpm();
writeMakefileDistclean();
writeMakefileFinalTargets();

# print "The Makefile has been generated. Type \"make\" and\/or \"make install" to build (and install)\n\n";

sub writeAllTarget () {
	# Javascript library will be build only if $jsLDFLAGS is not empty (= this means
        # mozjs library was found in system.
	my $all = "all:";
	if ($INCLUDED_JS) {
	    $all = "$all Javascript";
	}
	$all = "$all libFreeWRL CFrontEnd";
	if ($VRML_CONFIG{NETSCAPE_INST} ne "") {
		$all = "$all Plugin";
	}
	$all = "$all FreeWRL_Message";
	if ($JAVAC ne "") {
	    $all = "$all JavaClasses";
	}
	if ($VRML_CONFIG{SOUND_ENGINE}) {
	    $all = "$all SoundEngine";
	}
	if ($VRML_CONFIG{MIDI_ENGINE}) {
	    $all = "$all MIDI";
	}
	print MK $all;
	print MK "\n";
}

sub writeMakefileInitialize() {	
	my $FELIBS = "";

	my $FRONTEND_DIR  = $VRML_CONFIG{BIN_DIR};
	my $LIB_DIR       = $VRML_CONFIG{LIB_DIR};
	my $MYINC         = $VRML_CONFIG{FREEWRL_INC};
	my $MYLIBS 	= $VRML_CONFIG{FREEWRL_LIBS};

        my $MYLDFLAGS     = $VRML_CONFIG{LDFLAGS};

	my $MYDEFINE      = $VRML_CONFIG{FREEWRL_DEFINE};

	$MYFELIBS =  "-L. ". $MYLIBS . " -lFreeWRLFunc -lpthread";

	my $MYSELIBS = "-lm ";
	my $MYMESLIBS = "-lXaw ";

	# are we compiling the front end with Motif or GTK2?
	if (index ($VRML_CONFIG{FREEWRL_DEFINE},"HAVE_MOTIF") > 0) {
		 $MYFELIBS = $MYFELIBS . " -lXm ";
	}

	# add on the shared libs, etc. to MYFELIBS for making the freewrl executable.
	# NOTE: order of linking is important; the javascript library should be near
	# the end.
	my $MYFELIBS =  $MYFELIBS." ".$MYLIBS;
	
	
	open ID, ">CFuncs/installdir.h" or die ("Couldn't create installdir.h");
	print ID "#define INSTALLDIR \"$VRML_CONFIG{INSTALL_DIR}/freewrl\"\n";
	close ID;
	
	# do the headers.
	print MK "#Make file for FreeWRL\n";
	print MK "CC = gcc\n";
	print MK "JAVAC = javac\n";
	print MK "INC = $MYINC\n";
	if ($DEBUG) {
	    $MYDEFINE = "$MYDEFINE -DDEBUG -D_DEBUG -DPLUGINSOCKETVERBOSE";
	}
	print MK "DEFINE = $noStackProtector $MYDEFINE\n";
	#print MK "FWFLAGS = -c -Wall -O2 -g -pipe -m32 -march=i386 -mtune=pentium4\n";

	# different compiler flags for different architectures
#	if ($ARCH eq "x86_64") {
#		print MK "FWFLAGS = -c -O2 -g -pipe -m32 -march=i386 -mtune=pentium4\n";
#	} else {
		print MK "FWFLAGS = -O0 -g -pipe -fno-strict-aliasing -fPIC\n";
#	}
	print MK "MYLIBS = $MYLIBS\n";
	print MK "MYFELIBS = $MYFELIBS\n";
	print MK "MYSELIBS = $MYSELIBS\n";
	print MK "MYMESLIBS = $MYMESLIBS\n";
	print MK "FRONTEND_DIR = $FRONTEND_DIR\n";
	print MK "LIB_DIR = $LIB_DIR\n";
	print MK "JAVAC = $JAVAC\n";
	print MK "JAR = $JAR\n";
	print MK "INSTALL_DIR = $VRML_CONFIG{INSTALL_DIR}\n";

	print MK "CFLAGS = \$(FWFLAGS) \$(INC) \$(DEFINE)\n"; # -IPlugin/netscape/include

	print MK "COMPILE := \$(CC) \$(CFLAGS)\n";
}
	
sub writeMakefileFinalTargets() {
	print MK "\n";

	print MK "install:\tall	\n";
	print MK "\tinstall -s -D CFrontEnd/freewrl \$(DESTDIR)\$(FRONTEND_DIR)/freewrl\n";
	print MK "\tinstall -s -D FreeWRL_Msg/FreeWRL_Message \$(DESTDIR)\$(FRONTEND_DIR)/FreeWRL_Message\n";
	if ($VRML_CONFIG{MIDI_ENGINE}) {
		print MK "\tinstall -s -D MIDI/freewrlReWireServer \$(DESTDIR)\$(FRONTEND_DIR)/freewrlReWireServer\n";
	}
	if ($OS eq "Linux") {
		print MK "\tinstall -D libFreeWRLFunc.so \$(DESTDIR)\$(LIB_DIR)/libFreeWRLFunc.so\n";
		if ($INCLUDED_JS) {
		    print MK "\tinstall -D $libFreeWRLjspath \$(DESTDIR)\$(LIB_DIR)/libFreeWRLjs.so\n";
		}
		if ($CHCON ne "") {
		    if ($INCLUDED_JS) {
			print MK "\t-$CHCON -t texrel_shlib_t  \$(DESTDIR)\$(LIB_DIR)/libFreeWRLjs.so\n";
		    }
		    print MK "\t-$CHCON -t texrel_shlib_t  \$(DESTDIR)\$(LIB_DIR)/libFreeWRLFunc.so\n";
		}
		if ($NEED_LDCONFIG) {
			print MK "\t/sbin/ldconfig\n";
		}
	}
	if ($VRML_CONFIG{SOUND_ENGINE}) { 
		print MK "\tinstall -s -D SoundEngine/FreeWRL_SoundServer \$(DESTDIR)\$(FRONTEND_DIR)/FreeWRL_SoundServer\n";
	}
	if ($VRML_CONFIG{NETSCAPE_INST} ne "") {
		print MK "\tinstall -D Plugin/npfreewrl.so \$(DESTDIR)$VRML_CONFIG{NETSCAPE_INST}/npfreewrl.so\n";
		if ($CHCON ne "") {
		    print MK "\t-$CHCON -t texrel_shlib_t \$(DESTDIR)$VRML_CONFIG{NETSCAPE_INST}/npfreewrl.so\n";
		}
	}
	if ($JAVAC ne "") {
		print MK "\tinstall -D java/classes/vrml.jar \$(DESTDIR)$VRML_CONFIG{NETSCAPE_INST}/vrml.jar\n";
	}
	print MK "\tinstall -D Desktop/freewrl.png \$(DESTDIR)\$(INSTALL_DIR)\/pixmaps/freewrl.png\n";
	print MK "\tinstall -D Desktop/freewrl.desktop \$(DESTDIR)\$(INSTALL_DIR)\/applications/freewrl.desktop\n";
	# fonts shall not be installed : prefer depends on vera ttf package
	#print MK "\tinstall -d fonts \$(DESTDIR)\$(INSTALL_DIR)/freewrl/fonts\n";
	#print MK "\tinstall -D fonts/* \$(DESTDIR)\$(INSTALL_DIR)/freewrl/fonts\n";
}

sub writeVariables () {	
	my @CFuncsSourceFiles = split (' ',`ls CFuncs/*c`);
	print MK "\n#CFuncs Make\n";
	print MK "LIBFWSRCS=";
	foreach my $fullname (@CFuncsSourceFiles) { print MK "$fullname "; }
	print MK "\n";
	print MK "LIBFWOBJS=";
	foreach my $fullname (@CFuncsSourceFiles) { $fullname =~ s/.c$/.o/; print MK "$fullname "; }
	print MK "\n\n";
	
	my @CFrontEndSourceFiles = split (' ',`ls CFrontEnd/*c`);
	print MK "\n#CFrontEnd Make\n";
	print MK "FESRCS=";
	foreach my $fullname (@CFrontEndSourceFiles) { print MK "$fullname "; }
	print MK "\n";
	print MK "FEOBJS=";
	foreach my $fullname (@CFrontEndSourceFiles) { $fullname =~ s/.c$/.o/; print MK "$fullname "; }
	print MK "\n\n";

	## MIDI sources. Have to link in some files in, and compile them here, but with different flags.
	if ($VRML_CONFIG{MIDI_ENGINE}) {
	    system ("cp ReWire/EAI_C_Control.c MIDI");
	    system ("cp ReWire/EAI_C_Field.c MIDI");
	    system ("cp ReWire/EAI_C_Internals.c MIDI");
	    system ("cp ReWire/EAI_C_Node.c MIDI");
	    system ("cp ReWire/EAI_C_Advise.c MIDI");
	    system ("cp CFuncs/EAI_C_CommonFunctions.c MIDI");
	    system ("cp ReWire/EAI_C_ReWire.c MIDI");
	    system ("cp ReWire/GeneratedCode.c MIDI");
	    system ("cp ReWire/Eai_C.h MIDI");
	    system ("cp ReWire/GeneratedHeaders.h MIDI");
	    system ("cp ReWire/X3DNode.h MIDI");
	    
	    my @CMIDISourceFiles = split (' ',`ls MIDI/*c`);
	    print MK "\n#CMIDI Make\n";
	    print MK "MIDISRCS=";
	    foreach my $fullname (@CMIDISourceFiles) { print MK "$fullname "; }
	    print MK "\n";
	    print MK "MIDIOBJS=";
	    foreach my $fullname (@CMIDISourceFiles) { $fullname =~ s/.c$/.o/; print MK "$fullname "; }
	    print MK "\n\n";
	}

	
	my @SoundEngineSourceFiles = split (' ',`ls SoundEngine/*c`);
	print MK "\n#SoundEngine Make\n";
	print MK "SESRCS=";
	foreach my $fullname (@SoundEngineSourceFiles) { print MK "$fullname "; }
	print MK "\n";
	print MK "SEOBJS=";
	foreach my $fullname (@SoundEngineSourceFiles) { $fullname =~ s/.c$/.o/; print MK "$fullname "; }
	print MK "\n\n";

	print MK "\n#Console Message Files\n";
	print MK "FWMSRCS=FreeWRL_Msg/FreeWRL_Message.c\n";
	print MK "FWMOBJS=FreeWRL_Msg/FreeWRL_Message.o\n";
	print MK "\n\n";
	
	print MK "\n#Plugin Files\n";
	print MK "PLSRCS=Plugin/netscape/source/npfreewrl.c Plugin/netscape/source/common/npunix.c\n";
	print MK "PLOBJS=Plugin/netscape/source/npfreewrl.o Plugin/netscape/source/common/npunix.o\n";
	print MK "\n\n";
}

sub writeMakefileLibFreeWRL() {
	#############################################################################
	#LIBFREEWRL
	print MK "\$(LIBFWSRCS):\n";
	print MK "\t\$(COMPILE) -c \$*.c\n\n";
	print MK "libFreeWRL: libFreeWRLFunc.so\n";
	print MK "libFreeWRLFunc.so: \$(LIBFWOBJS)\n";
	print MK "\t\$(COMPILE) -shared -o libFreeWRLFunc.so \$(LIBFWOBJS) \n\n";
}

sub writeMakefileCfrontend() {
	#############################################################################
	#CFRONTEND
	    print MK "\n# make the Unix front end\n";
	    print MK "\$(FESRCS):\n";
	    print MK "\t\$(COMPILE) -c \$*.c\n\n";
	    print MK "CFrontEnd: CFrontEnd/freewrl\n";
	    print MK "CFrontEnd/freewrl: \$(FEOBJS)\n";
	    print MK "\t\$(COMPILE) \$(FEOBJS) \$(MYFELIBS) -o CFrontEnd/freewrl\n\n";
}

sub writeMakefileSoundEngine() {
	#############################################################################
	# SoundEngine
	# make the sound engine, console message, and frontend if NOT OSX.

	if ($VRML_CONFIG{SOUND_ENGINE}) {
		print MK "\n# make the Unix sound engine \n";
		print MK "\$(SESRCS):\n";
		print MK "\t\$(COMPILE) -c \$*.c\n\n";
		print MK "SoundEngine: SoundEngine/FreeWRL_SoundServer\n";
		print MK "SoundEngine/FreeWRL_SoundServer: \$(SEOBJS)\n";
		print MK "\t\$(COMPILE) -o SoundEngine/FreeWRL_SoundServer \$(MYLIBS) \$(SEOBJS) \$(MYSELIBS)\n\n";
	}
}

sub writeMakefileMIDI() {
	#############################################################################
	# MIDI Interface (Linux only)

	if ($VRML_CONFIG{MIDI_ENGINE}) {
		print MK "\n# make the Unix MIDI Interface \n";
		print MK "\$(MIDISRCS):\n";
		print MK "\t\$(COMPILE) -c -DREWIRE -DMIDI -I../ReWire -I../CFuncs \$*.c\n\n";
		print MK "MIDI: MIDI/freewrlReWireServer\n";
		print MK "MIDI/freewrlReWireServer: \$(MIDIOBJS)\n";
		print MK "\t\$(COMPILE) -o MIDI/freewrlReWireServer -L/usr/lib -lpthread  \$(MIDIOBJS) \n\n";
	}
}

sub writeMakefileFreeWRLMessage() {
	#############################################################################
	# FreeWRL_Message 
	# make a program that displays a message if NOT OSX and if we are not using Motif.

	    print MK "\n# make the Unix Console Message \n";
	    print MK "\$(FWMSRCS):\n";
	    print MK "\t\$(COMPILE) -c \$*.c\n\n";
	    print MK "FreeWRL_Message: FreeWRL_Msg/FreeWRL_Message\n";
	    print MK "FreeWRL_Msg/FreeWRL_Message: \$(FWMOBJS)\n";
	    print MK "\t\$(COMPILE) FreeWRL_Msg/FreeWRL_Message.o -o FreeWRL_Msg/FreeWRL_Message \$(MYLIBS) \$(MYMESLIBS) \n\n";
}

sub write_linux_makefile_javascript() {
	#############################################################################
	## make the Javascript libraries if needed
    
	    print MK "Javascript:\n";
	    print MK "\tcd JS/$jsdir; make -f Makefile.ref BUILD_OPT=1 STACKPROT=$noStackProtector; cd $pwd\n";
}

sub write_linux_makefile_javaClasses() {
	if ($JAVAC ne "" ) {
		my @JavaSourceFiles = split (' ',`find . -name "*java" | grep "java\/classes"`);
		foreach my $fullname (@JavaSourceFiles) {
			# remove the Java from the front, and the .c from the end.
			my $nm = $fullname;
			$nm =~ s/^\.\///; $nm =~ s/.java$/.class/;
	
			print MK "$nm:\n	\$(JAVAC) -classpath java/classes $fullname\n";
		}
		print MK "JAVASOURCE=$JavaSourceFiles";
		foreach my $fullname (@JavaSourceFiles) {
			# remove the Java from the front, and the .c from the end.
			my $nm = $fullname;
			$nm =~ s/^\.\///; $nm =~ s/.java$/.class/;
	
			print MK "$nm ";
		}
		print MK "\n";

		print MK "JavaClasses: java/classes/vrml.jar\n";
		print MK "java/classes/vrml.jar: \$(JAVASOURCE)\n";
		# remove the old jar file (it could be read-only 'cause of CVS, and jar will fail...)
		print MK "\trm -f \$@\n";
		# make the jar file here
		print MK "\t\$(JAR) -cf java/classes/vrml.jar $@ `find java/classes/vrml -name *.class`\n";
	}
}

sub writeMakefileBrowserPlugin() {
    if ($VRML_CONFIG{NETSCAPE_INST}) {
	    print MK "\n# make the Unix front end\n";
	    print MK "\$(PLOBJS): \$(PLSRCS)\n";
	    print MK "\t\$(COMPILE) -DMOZ_X11 $pg_nspr -IPlugin/netscape/include -c \$*.c -o \$*.o\n\n";
	    print MK "Plugin: Plugin/npfreewrl.so\n";
	    print MK "Plugin/npfreewrl.so: \$(PLOBJS)\n";
	    print MK "\t\$(COMPILE) -shared -o Plugin/npfreewrl.so \$(PLOBJS)\n\n";
    }
}

sub writeMakeDebAndRpm () {

	if ($OS eq "Linux") {

	    if (($issue =~ /Fedora/) || ($issue =~ /RedHat/)) {
		$specfile = $VRML_CONFIG{SPECFILE};
		$rpm_prefix = $VRML_CONFIG{RPMPREFIX};
		$sourcepackage = $VRML_CONFIG{SOURCEPACKAGE};
		$rpmversion = $VRML_CONFIG{RPMVERSION}; 
		$osname = $VRML_CONFIG{OSNAME};
		print MK "rpm::\n";
		print MK "\t# make the gzip file of the dist.\n";
		print MK "\ttar -cf $rpm_prefix/SOURCES/$sourcepackage ..\/$srcdir\n";
		print MK "\trm -f $rpm_prefix/SOURCES/$sourcepackage.gz\n";
		print MK "\tgzip $rpm_prefix/SOURCES/$sourcepackage\n\n";
		print MK "\t#make the spec file.\n";
		print MK "\techo \"Summary: A vrml viewer\" > $specfile\n";
		print MK "\techo \"Name: freewrl\" >> $specfile\n";
		print MK "\techo \"Version: $rpmversion\" >> $specfile\n";
		print MK "\techo \"Release: $osname\" >> $specfile\n";
		#print MK "\techo \"Copyright: GPL\" >> $specfile\n";
		print MK "\techo \"License: GPL\" >> $specfile\n";
		print MK "\techo \"Group: Applications\/Multimedia\" >> $specfile\n";
		print MK "\techo \"Source0: $sourcepackage.gz\" >> $specfile\n";
		print MK "\techo \"URL: http:\/\/www.crc.ca\/FreeWRL\" >> $specfile\n";
		print MK "\techo \"Distribution: Linux\" >> $specfile\n";
		print MK "\techo \"Vendor: CRC Canada\" >> $specfile\n";
		print MK "\techo \"Packager: Autogenerated, John Stewart, <freewrl-06@rogers.com>\" >> $specfile\n";
		print MK "\techo \"Provides: freewrl\" >> $specfile\n";
		print MK "\techo \"%define __check_files %{nil} \" >> $specfile\n";
		print MK "\techo \"%description\" >> $specfile\n";
		print MK "\techo \"FreeWRL is an Open Source, cross platform VRML2/X3D compliant browser, with script and EAI support.\" >> $specfile\n";
		print MK "\techo \"The primary platform is Linux.\" >> $specfile\n";
		print MK "\techo \"%prep\" >> $specfile\n";
		# split the zcat into 2 lines because we need the $RPM_SOURCE_DIR in the rpm spec file...
		print MK "\techo -n \"zcat \$\$\" >> $specfile\n";
		print MK "\techo \"RPM_SOURCE_DIR\/$sourcepackage.gz | tar -xvf -\" >> $specfile\n";
		print MK "\techo \"%setup -q\" >> $specfile\n";
		print MK "\techo \"%build\" >> $specfile\n";
		print MK "\techo \"perl Makefile.PL\" >> $specfile\n";
		print MK "\techo \"%install\" >> $specfile\n";
		print MK "\techo \"make install\" >> $specfile\n";
		print MK "\techo \"%post\" >> $specfile\n";
		print MK "\techo \"\/sbin\/ldconfig\" >> $specfile\n";
		print MK "\techo \"%clean\" >> $specfile\n";
		print MK "\techo \"%files\" >> $specfile\n";
		print MK "\techo \"%defattr(-,root,root)\" >> $specfile\n";
		print MK "\techo \"\$(DESTDIR)\$(LIB_DIR)/libFreeWRLFunc.so\" >> $specfile\n";
		if ($INCLUDED_JS) {
		    print MK "\techo \"\$(DESTDIR)\$(LIB_DIR)/libFreeWRLjs.so\" >> $specfile\n";
		}
		print MK "\techo \"\$(DESTDIR)\$(FRONTEND_DIR)/FreeWRL_Message\" >> $specfile\n";
		print MK "\techo \"\$(DESTDIR)\$(FRONTEND_DIR)/freewrl\" >> $specfile\n";
		print MK "\techo \"\$(DESTDIR)\$(FRONTEND_DIR)/FreeWRL_SoundServer\" >> $specfile\n";
                print MK "\techo \"\$(DESTDIR)$VRML_CONFIG{NETSCAPE_INST}/vrml.jar\" >> $specfile\n";
                print MK "\techo \"\$(DESTDIR)$VRML_CONFIG{NETSCAPE_INST}/npfreewrl.so\" >> $specfile\n";
                print MK "\techo \"\$(DESTDIR)\$(INSTALL_DIR)\/pixmaps/freewrl.png\" >> $specfile\n";
                print MK "\techo \"\$(DESTDIR)\$(INSTALL_DIR)\/applications/freewrl.desktop\" >> $specfile\n";
		#print MK "\techo \"\$(DESTDIR)\$(INSTALL_DIR)/freewrl/fonts\" >> $specfile\n";
	    } 

	    if (($issue =~ /Debian/) || ($issue =~ /Ubuntu/)) {
		print MK "deb::\n";
		print MK "\tsudo debuild -S\n";
		print MK "\tsudo pbuilder build ../*.dsc\n";
	    }
	}
}

sub writeMakefileDistclean() {
	#############################################################################
	# Misc targets
	print MK ".PHONY: clean distclean\n";
	print MK "clean:\n";
	print MK "\trm -f CFuncs/*o\n";
	print MK "\trm -f libFreeWRLFunc.so\n";
	print MK "\trm -f FreeWRL_Msg/*o\n";
	print MK "\trm -f FreeWRL_Msg/FreeWRL_Message\n";
	print MK "\trm -f CFrontEnd/*o\n";
	print MK "\trm -f CFrontEnd/freewrl\n";
	print MK "\trm -f freewrl\n";
	if ($VRML_CONFIG{SOUND_ENGINE}) {
	    print MK "\trm -f SoundEngine/*o\n";
	    print MK "\trm -f SoundEngine/FreeWRL_SoundServer\n";
	}
	if ($VRML_CONFIG{MIDI_ENGINE}) {
		print MK "\trm -f MIDI/*o\n";
		print MK "\trm -f MIDI/freewrlReWireServer\n";
	}
	if ($VRML_CONFIG{NETSCAPE_INST} ne "") {
		print MK "\trm -f Plugin/npfreewrl.so\n";
		print MK "\trm -f Plugin/netscape/source/npfreewrl.o\n";
		print MK "\trm -f Plugin/netscape/source/common/npunix.o\n";
	}
	if ($JAVAC ne "") {
	    print MK "\trm -f java/classes/vrml/external/field/*class\n";
	    print MK "\trm -f java/classes/vrml/field/*class\n";
	    print MK "\trm -f java/classes/vrml/external/node/*class\n";
	    print MK "\trm -f java/classes/vrml/external/FreeWRLEAI/*class\n";
	    print MK "\trm -f java/classes/vrml/external/*class\n";
	    print MK "\trm -f java/classes/vrml/external/exception/*class\n";
	    print MK "\trm -f java/classes/vrml/node/*class\n";
	    print MK "\trm -f java/classes/vrml/*class\n";
	    print MK "\trm -f java/classes/vrml/*class\n";
	    print MK "\trm -f java/classes/sai/*class\n";
	    print MK "\trm -f java/classes/sai/eai/*class\n";
	    print MK "\trm -f java/classes/org/web3d/x3d/sai/*class\n";
	    print MK "\trm -f java/classes/vrml.jar\n";
	}

	if ($INCLUDED_JS) {
	    print MK "\trm -rf JS/$jsobjdir\n";
	    $jsobjdir =~s/src/src\/editline/;
	    print MK "\trm -rf JS/$jsobjdir\n";
	    $jsobjdir =~ s/editline/fdlibm/;
	    print MK "\trm -rf JS/$jsobjdir\n";
	}

	print MK "\n#remove installed images\n";
	print MK "distclean: clean\n";
	print MK "\trm -f Makefile\n";
	print MK "\trm -f \$(shell find . -type f -name \"*~\")\n";
	print MK "\trm -f TAGS\n";
	print MK "\trm -f \$(DESTDIR)\$(FRONTEND_DIR)/freewrl\n";
	print MK "\trm -f \$(DESTDIR)\$(FRONTEND_DIR)/FreeWRL_Message\n";

	if ($OS eq "Linux") {
		print MK "\trm -f \$(DESTDIR)\$(LIB_DIR)/libFreeWRLFunc.so\n";
		if ($INCLUDED_JS) {
		    print MK "\trm -f \$(DESTDIR)\$(LIB_DIR)/libFreeWRLjs.so\n";
		}
		if ($NEED_LDCONFIG) {
		    print MK "\t/sbin/ldconfig\n";
		}
	}
	if ($VRML_CONFIG{SOUND_ENGINE}) { 
		print MK "\trm -f \$(DESTDIR)\$(FRONTEND_DIR)/FreeWRL_SoundServer\n";
	}
	if ($VRML_CONFIG{NETSCAPE_INST} ne "") {
		print MK "\trm -f \$(DESTDIR)$VRML_CONFIG{NETSCAPE_INST}/npfreewrl.so\n";
	}
	if ($JAVAC ne "") {
		print MK "\trm -f \$(DESTDIR)$VRML_CONFIG{NETSCAPE_INST}/vrml.jar\n";
	}
	print MK "\trm -f \$(DESTDIR)\$(INSTALL_DIR)\/pixmaps/freewrl.png\n";
	print MK "\trm -f \$(DESTDIR)\$(INSTALL_DIR)\/applications/freewrl.desktop\n";
# 	print MK "\trm -rf \$(DESTDIR)\$(INSTALL_DIR)/freewrl/fonts\n";
	print MK "\trm -rf \$(DESTDIR)\$(INSTALL_DIR)/freewrl/x3d\n";
}

###################################################################################
# determine on what system we are running, and if we can make a package for it for
# redistribution
sub linux_system_info() {
	
	my $rpm_prefix;
	my $osname;
	my $mightBeUbuntu = FALSE;
	
	# determine where the package should be built.
	# is this a Red Hat, Fedora package?
	#rpm_prefix = "/tmp";
	if (-e '/usr/src/redhat') {
		$rpm_prefix = "/usr/src/redhat";
	
	# is this SuSE/Novell linux??
	} elsif (-e '/usr/src/packages') {
		$rpm_prefix = "/usr/src/packages";

	# yikes - do not know...
	} else { $rpm_prefix = "/tmp"; }
		
	# what OS version is this built on? each Linux release has a different
	# file name here - might be /etc/SuSE-release. 

	my $distrib = `lsb_release --id | cut -f2`;
	$distrib =~ s/\b(\w)/\U$1/g;
	# remove trailing cr
	$distrib =~s/\n.*//;

	my $ver = `lsb_release --release | cut -f2`;
	# remove trailing cr
	$ver =~s/\n.*//;

	$osname = $distrib.$ver;
	
	if ($osname eq "") {
		print "could not determine how to make redistributable packages on this system \n";
		print "so the make rpm target will not work - oh well!\n";
		$osname = "unknown";
	}
	#print "osname is: $osname\n";
	
	# sourcepackage is a file created by "packagemaker" - it contains the version, etc.
	if (-f "sourcepackage") {
	    my $sourcepackage = `cat sourcepackage`;
	    # remove whitespace at beginning and end of this package name
	    $sourcepackage =~ s/\s+$//;
	    $sourcepackage =~ s/^\s+//;
	    my $srcdir = substr($sourcepackage, 0, rindex ($sourcepackage,".tar"));
	    my $rpmversion = substr ($srcdir,index($srcdir,"-")+1,100);
	    #print "srcdir $srcdirrpmversion $rpmversion sourcepackage $sourcepackage\n";
	
	    # make spec file name.
	    my $specfile = "$rpm_prefix/SPECS/".$srcdir.".".$osname.".spec";
	    #print "spec file is $specfile\n";
	
	    return ($rpm_prefix,$sourcepackage, $rpmversion, $osname, $specfile);
	} else {
	    return "sourcepackage must be created with ./packagemaker\n";
	}
}

sub findAndPrintUtilities() {
	# lets just dump out some standard defines, from the vrml.conf file.
	my @dumpToVRMLCONF = ( "BROWSER", "SOUNDCONV", "UNZIP", "WGET", "CONVERT", "COPIER", "JAVA");

	my $val;

	foreach (@dumpToVRMLCONF) {

		if (defined $VRML_CONFIG{$_}) {
			if (!(-e $VRML_CONFIG{$_})) {
				print "WARNING: Check your specific vrml.conf file; can not find system executable ".
					$VRML_CONFIG{$_}." for identifier $_\n";
			}
			$val = $VRML_CONFIG{$_};
 		} else {
			# lets see if we can find it....
			$val = tryToFind($_);
	 	}
		print OUT "#define\t$_\t\t\"$val\"\n";
	}
}

sub tryToFind() {
	my ($tofind) = @_;
	my $val = "ISNOTDEFINED";
	my $try = "";
	
	if ("BROWSER" eq $tofind) {
		# more than 1 possibility - remove whitespace after each try
		$try = `which firefox`; $try =~ s/^\s+//; $try =~ s/\s+$//;
		if ("" eq $try) { $try = `which safari`; $try =~ s/^\s+//; $try =~ s/\s+$//; }
		if ("" eq $try) { $try = `which mozilla`; $try =~ s/^\s+//; $try =~ s/\s+$//; }
		if ("" eq $try) { $try = `which netscape`; $try =~ s/^\s+//; $try =~ s/\s+$//; }
		if ("" eq $try) { $try = `which opera`; $try =~ s/^\s+//; $try =~ s/\s+$//; }
		if ("" eq $try) { $try = `which firefox`; $try =~ s/^\s+//; $try =~ s/\s+$//; }
		if ("" eq $try) { $try = `which seamonkey`; $try =~ s/^\s+//; $try =~ s/\s+$//; }
	} elsif ("SOUNDCONV" eq $tofind) {
		$try = `which sox`; $try =~ s/^\s+//; $try =~ s/\s+$//;
	} elsif ("UNZIP" eq $tofind) {
		$try = `which gunzip`; $try =~ s/^\s+//; $try =~ s/\s+$//;
	} elsif ("WGET" eq $tofind) {
		# more than 1 possibility - remove whitespace after each try
		$try = `which wget`; $try =~ s/^\s+//; $try =~ s/\s+$//;
		if ("" eq $try) {$try = `which curl`; $try =~ s/^\s+//; $try =~ s/\s+$//;}
	} elsif ("CONVERT" eq $tofind) {
		$try = `which convert`; $try =~ s/^\s+//; $try =~ s/\s+$//;
	} elsif ("COPIER" eq $tofind) {
		$try = `which cp`; $try =~ s/^\s+//; $try =~ s/\s+$//;
	} elsif ("JAVA" eq $tofind) {
		$try = `which java`; $try =~ s/^\s+//; $try =~ s/\s+$//;
	}

	if ("" ne $try) {
		$val = $try; # found it, hopefully
	} else {
		print "\tWARNING: can not find  $tofind in the conf file, or on the system"; print ": returning: $val\n";		
	}

	return $val;
}
