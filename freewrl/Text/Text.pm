# Copyright (C) 1998 Tuomas J. Lukka, (C)2001 John Stewart CRC Canada.
# DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
# See the GNU Library General Public License (file COPYING in the distribution)
# for conditions of use and redistribution.

package VRML::Text;
require DynaLoader;
@ISA=DynaLoader;
bootstrap VRML::Text;


# just tell Text.xs where to find the fonts. If the first path (first param)
# can not be found, then the second should have the baklava font supplied
# with FreeWRL

open_font($VRML::ENV{FREETYPE_FONT_PATH},($VRML::ENV{FREEWRL_FONTS}||"fonts"));

1;
