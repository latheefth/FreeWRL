package main;

use strict;

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
	my $inc2path = "$INCpath/blib/lib";

	push (@INC,$INCpath);
	push (@INC,$inc2path);
	#foreach (@INC) { print "incline $_\n";}
	#print "setIncpath, $INCpath\n";
}


sub open_browser {
	my ($q, $s) = @_;

	require DynaLoader;
	require "VRML/Config.pm";
	require "VRML/Browser.pm";

	$b = new VRML::Browser({});

	# By default creates front- and back-ends.
	$be = $b->{BE};

	# fonts
	my $testpath =  "$VRML::ENV{FREEWRL_BUILDDIR}/fonts";
	if (-e "$testpath/Baubodi.ttf") {
		VRML::VRMLFunc::save_font_path($testpath);
	} else {
		foreach (@INC) {
			$testpath =  "$_/VRML/fonts";
			if (-e "$testpath/Baubodi.ttf") {
				VRML::VRMLFunc::save_font_path($testpath);
				# print "found font path at $testpath\n";
				return;
			}
		}
	}
}

sub load_file_intro {
	# root is always a Group node, DO NOT CHANGE THIS
	my $string = "Group{}";
	my $url = VRML::VRMLFunc::GetBrowserURL();

        delete $b->{Scene};
        $b->{Scene} = VRML::Scene->new(0, $url, $url);
        $b->{Scene}->set_browser();
	VRML::Parser::parse($b->{Scene},$string);
        VRML::Browser::prepare ($b);
}
