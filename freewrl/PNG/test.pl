
use blib;
use VRML::PNG;

$a = "";
# XXX
VRML::PNG::read_file("/tmp/freewrl_root.png",$a);

print "LEN: ",(length $a),"\n";
# print $a;
