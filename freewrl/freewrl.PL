# Copyright (C) 1998 Tuomas J. Lukka
# DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
# See the GNU Library General Public License (file COPYING in the distribution)
# for conditions of use and redistribution.
#
# Based on files from PDL and Perl 

use Config;
use File::Basename qw(&basename &dirname);

# List explicitly here the variables you want Configure to
# generate.  Metaconfig only looks for shell variables, so you
# have to mention them as if they were shell variables, not
# %Config entries.  Thus you write
#  $startperl
# to ensure Configure will look for $Config{startperl}.

# This forces PL files to create target in same directory as PL file.
# This is so that make depend always knows where to find PL derivatives.
chdir(dirname($0));
($file = basename($0)) =~ s/\.PL$//;
$file =~ s/\.pl$//
        if ($^O eq 'VMS' or $^O eq 'os2');  # "case-forgiving"

unlink $file if -f $file;
open OUT,">$file" or die "Can't create $file: $!";

print "Extracting $file (with variable substitutions)\n";

# In this section, perl variables will be expanded during extraction.
# You can use $Config{...} to use Configure variables.

print OUT <<"!GROK!THIS!";
$Config{'startperl'}
    eval 'exec perl -S \$0 "\$@"'
        if 0;
!GROK!THIS!

# In the following, perl variables are not expanded during extraction.

print OUT <<'!NO!SUBS!';

##########################################################################
# Here starts the actual script
# GENERATED FROM freewrl.PL -- DO NOT MODIFY

# Copyright (C) 1998 Tuomas J. Lukka
# DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
# See the GNU Library General Public License (file COPYING in the distribution)
# for conditions of use and redistribution.

# The following is POD documentation for the freewrl command.
# It will be automatically installed in a manual page upon make install.
# See the perlpod manpage for the I<>, C<> &c explanation

=head1 NAME

freewrl -- run the FreeWRL VRML97 browser on the command line

=head1 SYNOPSIS

C<freewrl> [I<options>] I<url> [I<debugcode> ...]

=head1 DESCRIPTION

See the bottom of this text for the copyright and license.

This command runs the FreeWRL VRML browser.
See L<VRML::Viewer> for details on the user interface and 
L<VRML::Browser> for some other things. This manual
page explains how to use the browser, how to start it on 
the command line, and how to use it with Netscape.

=head1 KEYS/MOUSE

FreeWRL supports, as per VRML97 spec, several different I<navigation modes>.
In each mode, the meanings of mouse movements and keystrokes are different.

Mouse button 2 + motion is usually unbound so if you want to click
and drag something in the scene, use it.

In addition to the keys described below, there are some keys
which work in all the modes:

=over 4

=item d

Switch to the Fly (Keyboard input) navigation mode

=item f

Switch to the Fly (External Sensor input) navigation mode

=item e

Switch to the Examine navigation mode

=item w

Switch to the Walk navigation mode

=item v

Go to the next viewpoint in the scene

=item b

Go to the previous viewpoint in the scene

=item /

Print out the position and orientation of the current viewpoint
in the current Viewpoint node's local coordinate system. 

You can add the view you currently see in the window by creating a new
Viewpoint node (in the same coordinate system as the one you used to
go to this viewpoint) with these values.

=item s

Save a snapshot or (with the "-seq" option) start/stop saving a
sequence of images. By default, snapshots are saved to
F<freewrl.snap.NNNN.ppm> and sequences to F<freewrl.seq.gif>; see
options "-ppm", "-seqb" and "-snapb". While saving images, freewrl is
slower.

=item h

Toggle the headlight on or off.

=item q

Quit the browser 

=back

=head2 Walk

Drag the mouse while mouse button 1 is pressed to move forwards/backwards
or turn.
For translation in the x-y plane press mouse button 3 and drag.

=head2 Fly - keyboard mode

This mode is fun if you have ever played the game Descent
using the keyboard (definitely worth doing ;).

There is currently no mouse control, rather, all motion is controlled
by 12 keys, two for each of your 6 degrees of freedom (3 translations, 
3 rotations).

The default keymap in this mode is

=over 4

=item '8'/'k' 

rotation down/up



=item 'u'/'o' 

rotation left/right



=item '7'/'9'

rotation about the Z axis



=item 'a'/'z' 

translation forwards/backwards



=item 'j'/'l'

translation left/right



=item 'p'/';'

translation up/down



=back

It may take a while to get used to the FLY navigation mode but it is the only
one with full freedom of motion currently.

=head2 Fly - External Sensor input

This mode is entered via the "f" key. It takes input from a file in
the /tmp directory - this file contains x,y,z and quat info, and
puts the viewpoint there. 

As of December 17, 1999, only Polhemus code works, but Joystick 
input is literally around the corner.

=head2 Examine

The examine navigation mode is analogous to holding an object in your
hand and rotating it to see it from various sides: your line of view
always goes through the same point but you can rotate the object and
translate yourself closer and farther away.

Currently, dragging with mouse button 1 pressed
rotates the scene and dragging up or down mouse button 3 pressed 
translates you towards and away from the scene. There are no other controls.

The center around which you rotate is determined by taking the initial
position and orientation of the viewpoint given in the VRML file. 
A ray is cast along your line of view and the closest that ray comes 
to the origin of the local coordinate system is defined as the origin
of the rotations.

=head2 None

As the name says, there is nothing you can do in the NONE navigation mode.


=head1 OPTIONS

=over 4

=item C<-url> I<baseurl>

The browser downloads the URL given as the first argument on the command
line. The contents of the VRML file can refer to relative URLs just like HTML.
If given, this option is used to resolve the relative URLs instead
of the first commandline argument.

For example,

	freewrl -url http://www.foo.com/bigworld.wrl  tmp.wrl

can be used to run C<bigrworld.wrl> from a copy on the local hard drive
so that relative links will still work.

=item C<-best>

Depreciated

=item C<-fast>

Enables the fastest rendering in exchange for poorer picture quality.
This is overrides any C<-best> options seen on the command line.

=item C<-fullscreen>

Forces FreeWRL to run in fullscreen mode.  Note that if your graphics drivers
have an alternate way to enable fullscreen rendering it may be best to use that
way instead. The fullscreen flag will not enable hardware acceleration if it has
not already been enabled.

=item C<-plugin> I<PIDstring>

Tells FreeWRL that it is running within a web browser, and thus to get all URLs
from the browser, use the browser for EAI, not to handle the "q" key, pass the
XID (see X11 header file X.h) of the parent window, and to use the PID as the
window title.

=item C<-geo[metry]> I<geom>

=item C<-parent> I<xid>

Set the initial window geometry with the usual X window format string.

=item C<-[no]lwp>

Use / don't use libwww-perl. If you use the option -nolwp, you can only
view documents on your local file system.

=item C<-zpl> I<number>

Set the Z-buffer backplane level. If your world is very big, try setting
this number large. (NOTE: once NavigationInfo properly works, this option
will probably be deprecated)

=item C<-log> I<file>

Log stdout and stderr into I<file>.

=item C<-save> I<dir>

Save all the documents used in I<dir>. Useful if you want to see what
went wrong over a slow network connection.

This feature will probably be upgraded later to modify the urls and
filenames in a proper way, for now it just plonks down files with
names "s0", "s1" etc.

=item C<-seq>

The "s" key will start/stop saving a sequence of images rather than
saving a single snapshot.

=item C<-seqb> I<basename>

Specify base name for the sequence file. Default is
"freewrl.seq". Sets the "-seq" option.

=item C<-snapb> I<basename>

Specify base name for the snapshot files. Default is
"freewrl.snap". Unsets the "-seq" option.

=item C<-seqtmp> I<basename>

Specify base name for the sequence temporary files. Default is
~/freewrl_tmp.

=item C<-shutter>

Forces FreeWRL to start in shutterglas stereo mode via GLX_STEREO in
attributs of glXChooseVisual. This needs software and hardware like
XIG DX Platiuum Xserver on a Linux PC with 3DRelevator shutterglasses or
IRIX on a SGI Indigo2 Impact with crystal eyes shutterglasses.
On some SGI machines, you need to switch to a stereo capable visual 
as root with setmon/xsetmon and then restart the X Server.

=item C<-eyedist> I<number>

Distance between the eyes of the viewer in shutter mode, default 0.06

=item C<-screendist> I<number>

Distance between screen and the eyes of the viewer in shutter mode, 
default 0.8

=item C<-ppm>

In conjunction of "-seq" only, the sequence will be saved as lots of
ppm files.

=item C<-maximg>

Maximum number of snapshots or images in the sequence. Default is 100
to avoid filling up disk.

=item C<-eai> I<host:port>

The browser will try to connect to the socket at I<host:port> to obtain
an EAI connection. This option is mostly for internal use by
the EAI libraries.

=item C<-server>

FreeWRL will behave as a server : it spawns a child browser, prints
its PID on STDOUT and exits. The child then reloads the url given on
the command line upon receipt of a SIGUSR1 signal. 

=item C<-sig>

Reload the URL given on the command line upon receipt of a SIGUSR1 signal.

=item C<-ps>

Allow perl scripts in vrml files. Make sure your perl scripts are secure.

=item C<-psout> I<file>

Print statements within perl scripts will -by default- go to I<file>.

=item I<debugcode>

Various strings here turn on debugging for various parts of the browser.
Mostly of interest to developers, who can see the file C<freewrl.PL> 
in the distribution for the codes.

=back

=head1 USING FREEWRL WITH NETSCAPE

If Netscape is installed, and the entries in vrml.conf are correct, the
FreeWRL plug-in will be installed in Netscape's system plugins folder.
If you go to Netscape's Help menu and select the About Plug-ins,
there should be an entry for FreeWRL with Mime Type x-world/x-vrml.

=head1 AUTHOR

Project started by Tuomas J. Lukka, continued by John Stewart.


Please send bug reports to C<john.stewart@crc.ca>
including the word "freewrl"
on the subject line will make sure that I'll notice it. Also,
see the FreeWRL home page at C<http://www.crc.ca/FreeWRL>.

There is absolutely no warranty, express or implied for this software.
For details on the conditions of use, see the FreeWRL distribution.

FreeWRL is Copyright (C) 1998, 1999 Tuomas J. Lukka,
John Stewart and others.


This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public
License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public
License along with this library; if not, write to the Free
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA


=cut

use Carp;
$SIG{__DIE__} = sub {print Carp::longmess(@_);die;};

BEGIN {

	# use VRML::Browser;
        $main::saving = 0;		# Currently saving?
        $main::seqcnt = 0;		# Number of images in sequence
        $main::snapcnt = 0;	        # Number of snapshots
        @main::saved = ();		# List of [im name, ncols, nrows, im number]

	           			# Options : seq, nogif, maximg
        $main::seq =    0  ;
        $main::nogif =  0  ;
        $main::maximg = 100;
        $main::seqtmp = "$ENV{HOME}/freewrl_tmp" ;
        $main::seqname = "freewrl.seq" ;
        $main::snapname = "freewrl.snap" ;
	use Getopt::Long;

	GetOptions(
		"url=s" => \$url,
		"eai=s" => \$eai,
		"zpl=f" => \$zpl,
		"best" => \$best,
		"fast" => \$fast, 
		"fullscreen" => \$fullscreen,
		"geometry=s" => \$geometry,
		"lwp!" => \$VRML::ENV{FREEWRL_NO_LWP},
		"save=s" => \$VRML::ENV{FREEWRL_SAVE},
		"log=s" => \$log,
		"seq"=>\$seq,
		"seqb=s"=>\$seqname_opt,
		"seqtmp=s"=>\$seqtmp_opt,
		"shutter" => \$shutter,
		"eyedist=f"=>\$eyedist_opt,
		"screendist=f"=>\$screendist_opt,
		"snapb=s"=>\$snapname_opt,
		"ppm"=>\$nogif,
		"ps"=>\$VRML::DO_PERL,
		"psout=s"=>\$VRML::script_out_file,
		"maximg=i"=>\$maximg,
		"help" => \$usage,
		"usage" => \$usage,
		"server"=> \$server,
		"sig" => \$sig_reload,
		"plugin=s" => \$VRML::ENV{AS_PLUGIN},
		"parent=i" => \$parent,
		"fd=i" => \$VRML::PluginGlue::globals{pluginSock},
		"instance=s" => \$VRML::PluginGlue::globals{instance},
	);

	if (defined $snapname_opt) { # Snapshot overrides sequence
		$snapname = $snapname_opt;
		$seq = 0;
	} elsif (defined $seqname_opt) {
		$seqname = $seqname_opt;
		$seq = 1;
	}
	$seqtmp = $seqtmp_opt if defined $seqtmp_opt ;

	exec ("perldoc -t $0 | cat") if $usage ;

	if($log) {
		open STDOUT, ">$log" || die("Can't redirect stdout to $log: $!");
		open STDERR, ">&STDOUT" || die("Can't redirect stderr to stdout: $!");
	}
	eval"use blib;";
	## I (Etienne) prefer having verbose output in the source dir (where
	## I'm likely to be testing) 
	## warn "Use blib failed" if $@;
	#warn "Use blib succeeded" unless $@;

	if ($server) {		# Behave as server : print pid and shut up
	  $| = $sig_reload = 1;
	  if ($pid = fork ()){
	    print "$pid\n" ;
	    if (!$log){
               close STDOUT;
	       close STDERR;
            }
	    exit 0;
	  } else {
	    if (!$log) {
		   close STDOUT;
	       close STDERR;
		}
	  }
	}
}

my @verb = @ARGV[1..$#ARGV];

sub verb {return !!grep {$_ eq $_[0] or $_ eq "all"} @verb}
sub verbn {return !!grep {$_ eq $_[0]} @verb}

sub clean_exit {
	$be->{QuitPressed} = 1;
}

# Turn these switches on if you are debugging
$VRML::verbose = verb all;
$VRML::verbose::be = verb be;
$VRML::verbose::wwarn = verb wwarn; # warnings, info messages, etc...
$VRML::verbose::oint = verb oint; # OrientationIntepr
$VRML::verbose::events = verb ev;
$VRML::verbose::script = verb scr ;
$VRML::verbose::glsens = verb glsens;  # GL sensor code
$VRML::verbose::tief = verb "tie";    # The tied RFields-hash
$VRML::verbose::timesens = verb "time";
$VRML::verbose::interp = verb "interp"; # interpolators
$VRML::verbose::text = verbn "text";
$VRML::verbose::rend = verb "rend";
$VRML::verbose::collision = verb "collision";
$VRML::verbose::nodec = verb "nodec";
$VRML::verbose::bind = verb "bind";
$VRML::verbose::prox = verb "prox";
$VRML::verbose::parse = verb "parse";
$VRML::verbose::js = verb "js";
$VRML::verbose::java = verb "java";
$VRML::verbose::scene = verb "scene";
$VRML::verbose::EAI = verb eai;   # EAI code...
$VRML::verbose::url = verb "url"; # URL processing and data loading

if($VRML::verbose::text) {
	eval 'require VRML::Text; VRML::Text::set_verbose(1);'
}

require 'VRML/Browser.pm';

if (!$fullscreen) {
	$fullscreen = 0;
}


if ($shutter) {
        $shutter = 1;
} else {
        $shutter = 0;
}


if ($eyedist_opt) {
        $eyedist = $eyedist_opt;
} else {
        $eyedist = 0.06;
}

if ($screendist_opt) {
        $screendist = $screendist_opt;
} else {
        $screendist = 0.8;
}

if (!$parent) {
	$parent = 0;
}

# Save font path, text nodes will require this
my $ok = 0;

$ok = VRML::VRMLFunc::save_font_path(($VRML::ENV{FREEWRL_FONTS}||"fonts"));
if ($ok == 0) {
	foreach (@INC) {
		$ok = VRML::VRMLFunc::save_font_path("$_/fonts");
	}
}

# open the browser

$b = new VRML::Browser({
						FullScreen => $fullscreen,
						Shutter => $shutter,
						EyeDist => $eyedist,
						Parent => $parent,
						ScreenDist => $screendist,
						BackEnd => [Geometry => $geometry]
					   });

if (0) {
#my $host = "localhost"; ## move to vrml.conf eventually

#if ($VRML::ENV{AS_PLUGIN}) {
#	if (($VRML::PluginGlue::globals{freeWRLSock} =
#			VRML::PluginGlue::connectToPlugin($host)) < 0) {
#	    VRML::PluginGlue::closeFileDesc($VRML::PluginGlue::globals{freeWRLSock});
#	    VRML::PluginGlue::closeFileDesc($VRML::PluginGlue::globals{pluginSock});
#	    exit(1);
#	}


#	if (VRML::PluginGlue::setIOOptions($VRML::PluginGlue::globals{freeWRLSock},
#											$$, 0, 1) < 0) {
#	    VRML::PluginGlue::closeFileDesc($VRML::PluginGlue::globals{freeWRLSock});
#	    VRML::PluginGlue::closeFileDesc($VRML::PluginGlue::globals{pluginSock});
#	    exit(1);
#	}
#}
}

# By default creates front- and back-ends.

$be = $b->get_backend;

# machines are getting better and better - make great rendering the default
if($fast) {
	$be->set_fast();
}

# texture size - if fast, reduce texture size (really helps sw rendering)
if ($fast) {
	&VRML::VRMLFunc::set_maxtexture_size (64);
} else {
	&VRML::VRMLFunc::set_maxtexture_size(2048);
}

$b->load_file($ARGV[0],$url);

# lets handle a normal term...
$SIG{TERM} = \&clean_exit;
$SIG{QUIT} = \&clean_exit;
$SIG{USR2} = \&clean_exit;

$SIG{USR1} = sub {
    $b->load_file($ARGV[0],$url);
    &VRML::OpenGL::raise_me_please();
} if $sig_reload;

## either make better or remove...
$SIG{BUS} = \&clean_exit;
$SIG{SEGV} = \&clean_exit;

if($zpl) {VRML::VRMLFunc::set_vpdist($zpl)}

## How long should freewrl poll before giving up on an EAI
## socket connect while using FreeWRL as a Netscape plugin?
## See VRMLServ.pm.

$VRML::ENV{EAI_CONN_RETRY} = 25;

if($eai) {
	require "VRML/VRMLServ.pm";
	$s = VRML::EAIServer->new($b);
	$s->connect($eai);
}

$b->eventloop();

!NO!SUBS!

close OUT;

chmod 0555, $file;


