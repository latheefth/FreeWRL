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

	#printf ("new javaclass, this $this, node $node, scri $scriptInvocationNumber, url $url\n");
	$this->{ScriptNum} = $scriptInvocationNumber;  # we go by this script number

	
	#foreach $key (keys(%{$node->{Fields}{enabled}})) {print "key of node Fields enabled is $key\n";}
	#foreach $key (keys(%{$node->{Type}})) {print "key of nodeType is $key\n";}
	#print "value is ",$node->{Type}{FieldKinds}{enabled},"\n";
	#print "exactvalue is ",$node->{Fields}{enabled},"\n";

	# get a nice, short name - strip off the NODE from ref NODEXX, and append
	# the scriptinvocationnumber - returns, "15:0", for instance. 
	my $id = VRML::Browser::EAI_GetNode($node).":".$scriptInvocationNumber;

	#print "newjavaclass, node is now $id\n";

	return VRML::VRMLFunc::do_newJavaClass($scriptInvocationNumber,$url,$id);
}
