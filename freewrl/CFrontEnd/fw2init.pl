use Carp;
$SIG{__DIE__} = sub {print Carp::longmess(@_);die;};

# this allows us to get config information; eg, version et al.
eval ('require DynaLoader;');
eval ('use VRML::Config;');
eval ('use VRML::Browser;');


my $b;
my $be;

# some people like to use rpms on systems that they were not built for,
# with perl module problems. So, attempt to put the compiled in path
# (and thus, the path in the rpm or equiv) onto the perl internal path.
sub setINCPath {
	my ($INCpath) = @_;

	$INCpath =~ s/\/VRML$//;

	push (@INC,$INCpath);
	#foreach (@INC) { print "incline $_\n";}
	#print "setIncpath, $INCpath\n";
}


sub open_browser {
	my ($q, $s) = @_;

	require DynaLoader;
	require "VRML/Config.pm";
	require "VRML/Browser.pm";

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
	# root is always a Group node, DO NOT CHANGE THIS
	my $string = "Group{}";
	my $url = VRML::VRMLFunc::GetBrowserURL();

        delete $b->{Scene};
        $b->{Scene} = VRML::Scene->new($this->{EV}, $url, $url);
        $b->{Scene}->set_browser($this);
	VRML::Parser::parse($b->{Scene},$string);
        VRML::Browser::prepare ($b);
}
