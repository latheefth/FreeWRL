use Carp;
$SIG{__DIE__} = sub {print Carp::longmess(@_);die;};

# this allows us to get config information; eg, version et al.
require DynaLoader;
use VRML::Config;

use VRML::Browser;


my $b;
my $be;


sub open_browser {
	my ($q, $s) = @_;

	$b = new VRML::Browser({
                                                CocoaContext => undef,
                                                FullScreen => $fullscreen,
                                                Shutter => $shutter,
                                                EyeDist => $eyedist,
                                                Parent => $parent,
                                                ScreenDist => $screendist,
                                                BackEnd => [Geometry => $geometry],
                                                XSLTpath => $xsltpath,
                                           });

	# By default creates front- and back-ends.
	$be = $b->{BE};
	
	# fonts
	$testpath =  "$VRML::ENV{FREEWRL_BUILDDIR}/fonts";
	if (-e "$testpath/Baubodi.ttf") {
		VRML::VRMLFunc::save_font_path($testpath);
	} else {
		foreach (@INC) {
			$testpath =  "$_/VRML/fonts";
			if (-e "$testpath/Baubodi.ttf") {
				VRML::VRMLFunc::save_font_path($testpath);
				print "found font path at $testpath\n";
				break;
			}
		}
	}
}

sub load_file_intro {
        my $string;

	print "start of load_file_intro on fw2init.pl\n";

	# testing...
	my $url = "./s.wrl";
	 VRML::VRMLFunc::SaveURL($url);
	 
	# root is always a Group node, DO NOT CHANGE THIS
	$string = "Group{}";

        delete $b->{Scene};
        $b->{Scene} = VRML::Scene->new($this->{EV}, $url, $url);
        $b->{Scene}->set_browser($this);
	VRML::Parser::parse($b->{Scene},$string);
        VRML::Browser::prepare ($b);
}
