# Copyright (C) 1998 Tuomas J. Lukka 1999 John Stewart CRC Canada
# DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
# See the GNU Library General Public License (file COPYING in the distribution)
# for conditions of use and redistribution.

# $Log$
# Revision 1.2  2001/07/11 20:43:05  ayla
#
#
# Fixed problem with Plugin/Source/npfreewrl.c, so all debugging info. is turned
# off.
#
# Committing merge between NetscapeIntegration and the trunk.
#
# Revision 1.1.2.2  2001/07/05 20:38:59  ayla
#
#
# Shuffled $VERSION around, added symbols to be exported.
#
# Revision 1.1.2.1  2001/06/19 22:26:16  ayla
#
#
# First draft of PluginGlue.pm needed to load package VRML::PluginGlue.
#
#
# Implement FreeWRL - Netscape Plugin Communication
# 
# The glue needed to implement C functions that, in turn, call Netscape
# Communicator functions...

package VRML::PluginGlue;
our $VERSION = 0.1;

# The next few lines (until call to bootstrap) come from perldoc perlxstut:
use strict vars;
#use warnings;

require Exporter;
require DynaLoader;

our @ISA = qw(
	      Exporter
	      DynaLoader
	     );

# Items to export into callers namespace by default. Note: do not export
# names by default without a very good reason. Use EXPORT_OK instead.
# Do not simply export all your public functions/methods/constants.

our @EXPORT = qw(
		 plugin_connect
		 close_fd
		);

bootstrap VRML::PluginGlue $VERSION;
