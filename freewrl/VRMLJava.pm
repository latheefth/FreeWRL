# Copyright (C) 1998 Tuomas J. Lukka
#               1999 John Stewart CRC Canada
#               2002 Jochen Hoenicke
# DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
# See the GNU Library General Public License (file COPYING in the distribution)
# for conditions of use and redistribution.

# Implement VRMLPERL-JAVA communication.

package VRML::JavaClass;

#debug 
$VRML::verbose::javaclass = 1;
 
sub new {
	my ($this, $node,$scriptInvocationNumber, $url) = @_;

	$this->{ScriptNum} = $scriptInvocationNumber;  # we go by this script number
	return VRML::VRMLFunc::do_newJavaClass($scriptInvocationNumber,$_,$node);
}
