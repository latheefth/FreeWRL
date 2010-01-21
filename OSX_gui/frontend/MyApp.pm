package MyApp;

use Foundation;
use Foundation::Functions;
use AppKit;
use AppKit::Functions;

use MyWindowController;

@ISA = qw(Exporter);

sub new {
    # Typical Perl constructor
    # See 'perltoot' for details
    my $proto = shift;
    my $class = ref($proto) || $proto;
    my $self = {
        'wc' => undef,
    };
    bless ($self, $class);

    return $self;
}

sub applicationWillFinishLaunching {
    my ($self, $notification) = @_;
    
    # Create the new controller object
    $wc = new MyWindowController;
       
    use VRML::Browser;
    $main::saving = 0;		# Currently saving?
    $main::seqcnt = 0;		# Number of images in sequence
    $main::snapcnt = 0;	        # Number of snapshots
    @main::saved = ();		# List of [im name, ncols, nrows, im number]

	           			# Options : seq, nogif, maximg
    $main::seq =    0  ;
    $main::convtompg =  0  ;
    $main::maximg = 100;
    $main::seqtmp = "$ENV{HOME}/freewrl_tmp" ;
    $main::seqname = "freewrl.seq" ;
    $main::snapname = "freewrl.snap" ;

    exec ("perldoc -t $0 | cat") if $usage ;

    eval"use blib;";

    ## I (Etienne) prefer having verbose output in the source dir (where
    ## I'm likely to be testing) 
    ## warn "Use blib failed" if $@;
    #warn "Use blib succeeded" unless $@;

    #if ($server) {		# Behave as server : print pid and shut up
    #    $| = $sig_reload = 1;
    #    if ($pid = fork ()){
    #    print "$pid\n" ;
    #    if (!$log){
    #        close STDOUT;
    #       close STDERR;
    #    }
    #    exit 0;
    #    } else {
    #    if (!$log) {
    #        close STDOUT;
    #        close STDERR;
    #       }
    #    }
    #}

push (@INC, '/Library/Perl/darwin/5.8.1/darwin-thread-multi-2level/');

# Turn these switches on if you are debugging
#$VRML::verbose = true;

#$VRML::verbose::be = true;
#$VRML::verbose::wwarn = true; # warnings, info messages, etc...
#$VRML::verbose::oint = true; # OrientationIntepr
#$VRML::verbose::events = true;
#$VRML::verbose::scenegraph = true; # Scene graph dump
#$VRML::verbose::script = true ;
#$VRML::verbose::glsens = true;  # GL sensor code
#$VRML::verbose::tief = true;    # The tied RFields-hash
#$VRML::verbose::timesens = true;
#$VRML::verbose::interp = true; # interpolators
#$VRML::verbose::text = true;
#$VRML::verbose::rend = true;
#$VRML::verbose::collision = true;
#$VRML::verbose::nodec = true;
#$VRML::verbose::bind = true;
#$VRML::verbose::prox = true;
#$VRML::verbose::parse = true; 
#$VRML::verbose::js = true;
#$VRML::verbose::java = true;
#$VRML::verbose::scene = true;
#$VRML::verbose::EAI = true;   # EAI code...
#$VRML::verbose::url = true; # URL processing and data loading
 
#Last good from above
if($VRML::verbose::text) {
	eval 'require VRML::Text; VRML::Text::set_verbose(1);'
}

# Save font path, text nodes will require this
my $ok = 0;
my $xsltpath = "";
my $testpath = "";

# fonts
#$testpath =  "$VRML::ENV{FREEWRL_BUILDDIR}/fonts";
#if (-e "$testpath/Baubodi.ttf") {
#        VRML::VRMLFunc::save_font_path($testpath);
#} else {
#	foreach (@INC) {
#                $testpath =  "$_/VRML/fonts";
#                if (-e "$testpath/Baubodi.ttf") {
#                        print "saving font path $testpath";
#                        VRML::VRMLFunc::save_font_path($testpath);
#                        break;
#                }
#        }
#}

$testpath = "";

### I NEED TO DO THIS IN SET_FAST
# texture size - if fast, reduce texture size (really helps sw rendering)
#if ($fast) {
#	&VRML::VRMLFunc::set_maxtexture_size (64);
#} else {
#	&VRML::VRMLFunc::set_maxtexture_size(2048);
#}

## How long should freewrl poll before giving up on an EAI
## socket connect while using FreeWRL as a Netscape plugin?
## See VRMLServ.pm.

$VRML::ENV{EAI_CONN_RETRY} = 25;

#if($eai) {
#	require "VRML/VRMLServ.pm";
#	$s = VRML::EAIServer->new($b);
#	$s->connect($eai);
#}
    return 1;


}

1;
