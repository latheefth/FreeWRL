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

freewrl -- run the FreeWRL VRML97 browser on the command line, or, if the plugin
is installed, via an HTML browser.

=head1 SYNOPSIS

C<freewrl> [I<options>] I<url> [I<debugcode> ...]

=head1 DESCRIPTION

This command runs the FreeWRL VRML browser.

=head1 KEYS/MOUSE

FreeWRL supports, as per VRML97 spec, several different I<navigation modes>.
In each mode, the meanings of mouse movements and keystrokes are different.

Mouse button 2 + motion is usually unbound so if you want to click
and drag something in the scene, use it.

In addition to the keys described below, there are some keys
which work in all the modes:

=over 4

=item ?

Display this man page in a window

=item .

Switch status bar display on or off

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
F<freewrl.snap.NNNN.ppm> and sequences to F<freewrl.seq.mpg>; see
options "-gif", "-seqb" and "-snapb". While saving images, freewrl is
slower.

=item h / NumLock

Toggle the headlight on or off.

=item c

Toggle collision detection on or off.

=item q

Quit the browser 

=back

=head2 Walk

Drag the mouse while mouse button 1 is pressed to move forwards/backwards
or turn.
For translation in the x-y plane press mouse button 3 and drag.

=head2 Fly - keyboard mode

This mode allows translation/rotations in all planes. There are two
keymappings: those from the game Descent, and a mapping using the
keypad.

All motion is controlled
by 12 keys, two for each of your 6 degrees of freedom (3 translations, 
3 rotations).

The keymap in this mode is

=over 4

=item '8'/'k' and 'Keypad Down'/'Keypad Up' 

rotation down/up



=item 'u'/'o' and 'Keypad Left'/'Keypad Right'

rotation left/right



=item '7'/'9' and 'Keypad 7'/'Keypad 9'

rotation about the Z axis



=item 'a'/'z' and 'Keypad 0'/'Keypad .' 

translation forwards/backwards



=item 'j'/'l' and 'Arrow left'/'Arrow right'

translation left/right



=item 'p'/';' and 'Arrow up'/'Arrow down'

translation up/down



=back

It may take a while to get used to the FLY navigation mode but it is the only
one with full freedom of motion currently.

=head2 Fly - External Sensor input

This mode is entered via the "f" key. It takes input from the file F</tmp/inpdev>: 
this file contains x,y,z and Quaternion-based rotation information, and puts the viewpoint there.

It is possible to force this mode in a NavigationInfo mode by making type "EXFLY".

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

=item C<--version>

Prints out the current FreeWRL version.

=item C<--fullscreen>

Forces FreeWRL to run in fullscreen mode.  Note that if your graphics drivers
have an alternate way to enable fullscreen rendering it may be best to use that
way instead. The fullscreen flag will not enable hardware acceleration if it has
not already been enabled. (see also --big option)

=item C<--big>

Forces FreeWRL to run in 800x600 mode.   See also --fullscreen option

=item C<--nostatus>

Starts FreeWRL without the on-screen status bar. See the "." key definition
for how to toggle this status bar display when running.

=item C<--keypress>

Sends the sequence of characters following to FreeWRL for processing once the
	original URL is loaded.

=item C<--plugin> I<PIDstring>

Tells FreeWRL that it is running within a web browser, and thus to get all URLs
from the browser, use the browser for EAI, not to handle the "q" key, pass the
XID (see X11 header file X.h) of the parent window, and to use the PID as the
window title.

=item C<--geo[metry]> I<geom>

=item C<--parent> I<xid>

Set the initial window geometry with the usual X window format string.

=item C<--seq>

The "s" key will start/stop saving a sequence of images rather than
saving a single snapshot.

=item C<--seqb> I<basename>

Specify base name for the sequence file. Default is
"freewrl.seq". Sets the "--seq" option.

=item C<--snapb> I<basename>

Specify base name for the snapshot files. Default is
"freewrl.snap". Unsets the "--seq" option.

=item C<--seqtmp> I<basename>

Specify base name for the sequence temporary files. Default is
~/freewrl_tmp.

=item C<--shutter>

Forces FreeWRL to start in shutterglas stereo mode via GLX_STEREO in
attributs of glXChooseVisual. This needs software and hardware like
XIG DX Platiuum Xserver on a Linux PC with 3DRelevator shutterglasses or
IRIX on a SGI Indigo2 Impact with crystal eyes shutterglasses.
On some SGI machines, you need to switch to a stereo capable visual 
as root with setmon/xsetmon and then restart the X Server.

=item C<--eyedist> I<number>

Distance between the eyes of the viewer in shutter mode, default 0.06

=item C<--screendist> I<number>

Distance between screen and the eyes of the viewer in shutter mode, 
default 0.8

=item C<--gif>

In conjunction of "--seq" only, the sequence will be saved as an animated gif,
rather than a mpg file.

=item C<--maximg>

Maximum number of snapshots or images in the sequence. Default is 100
to avoid filling up disk.

=item C<--eai> I<host:port>

The browser will try to connect to the socket at I<host:port> to obtain
an EAI connection. This option is mostly for internal use by
the EAI libraries.

=item C<--server>

FreeWRL will behave as a server : it spawns a child browser, prints
its PID on STDOUT and exits. The child then reloads the url given on
the command line upon receipt of a SIGUSR1 signal. 

=item C<--sig>

Reload the URL given on the command line upon receipt of a SIGUSR1 signal.

=item C<--ps>

Allow perl scripts in vrml files. Make sure your perl scripts are secure.

=back

=head1 USING FREEWRL WITH NETSCAPE AND OTHER BROWSERS

If Netscape is installed, and the entries in vrml.conf are correct, the
FreeWRL plug-in will be installed in Netscape's system plugins folder.
If you go to Netscape's Help menu and select the About Plug-ins,
there should be an entry for FreeWRL with Mime Type x-world/x-vrml.

=head1 AUTHOR

Project started by Tuomas J. Lukka, continued by John Stewart and others.


Please send bug reports to C<freewrl-04 --at--  rogers.com>
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
print "\nFreeWRL Error....\n";
print "this should not happen - you are running the freewrl manpage pod,\n";
print "not the freewrl executable. On Unix, make sure that you are running\n";
print "the installed freewrl executable, or the CFrontEnd/freewrl in the build directory.\n";
print "\n\n";
!NO!SUBS!
