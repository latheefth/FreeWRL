# Copyright (C) 1998 Tuomas J. Lukka 1999 John Stewart CRC Canada
# DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
# See the GNU Library General Public License (file COPYING in the distribution)
# for conditions of use and redistribution.

# $Log$
# Revision 1.4  2001/08/18 02:16:56  ayla
#
# Committing merge of Plugin files from NetscapeIntegration to trunk.
#
# Revision 1.1.2.3  2001/08/01 16:46:12  ayla
#
# A backward compatabitity problem with Perl 5.0x was fixed in the main branch.
#
# Revision 1.3  2001/07/20 21:12:28  ayla
#
#
# Fixed some backward compatibility problems with perl 5.00.
#
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
$VERSION = '0.10';

BEGIN {
    if ($^V lt v5.6.0) {
        # Perl voodoo to stop interpreters < v5.6.0 from complaining about
        # using our:
        sub our { return; }
    }
}

require Exporter;
require DynaLoader;

# The next few lines (until call to bootstrap) come from perldoc perlxstut:
our @ISA = qw(Exporter DynaLoader);

# Items to export into callers namespace by default. Note: do not export
# names by default without a very good reason. Use EXPORT_OK instead.
# Do not simply export all your public functions/methods/constants.

our @EXPORT = qw(plugin_connect close_fd);

bootstrap VRML::PluginGlue $VERSION;

1;
__END__

## Module POD documentation goes here.
