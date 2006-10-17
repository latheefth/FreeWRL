# Copyright (C) 1998 Tuomas J. Lukka, 2002 John Stewart, Ayla Khan CRC Canada
# DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
# See the GNU Library General Public License (file COPYING in the distribution)
# for conditions of use and redistribution.
#
# $Id$
#
#
#


package VRML::JS;

BEGIN {
    if ($^V lt v5.6.0) {
        # Perl voodoo to stop interpreters < v5.6.0 from complaining about
        # using our:
        sub our { return; }
    }
}
