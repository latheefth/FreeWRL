# See file Readme; this is from Tuomas, from his article in
# the Linux Journal. -- John Stewart.

$SIG{__DIE__}= sub {print Carp::longmess(@_); die FOO};
use Carp;

use Gtk;
use Gtk::Atoms;

for((),events,
# qw/tief be events scene/
) { ${"VRML::verbose::$_"} = 1; }

use blib "/home/tjl/c/VRML";
use VRML::Browser;

$xmin = 0;
$xmax = 7;
$ymin = 0;
$ymax = 7;
$nx = 40;
$ny = 40;
$func = '0.5 * sin($x)**4 * sin($y)**4';

my $b = new VRML::Browser;
$b->load_string("
	Viewpoint { position 0 0.5 3.0 }
	PROTO TransColored [ 
		field SFColor color 0.7 0.7 0.7 
		field SFNode node NULL 
		exposedField SFVec3f translation 0 0 0 
	] {
		Transform { 
			translation IS translation 
			children [
				Shape { 
				   appearance Appearance { 
				      material Material { 
				      	  diffuseColor IS color 
				   }}
				   geometry IS node
				}
			]
		}
	}
        DEF CARROW Transform {
		scale 1.1 1.1 1.1
		children [
			TransColored { 
				translation 0 0.5 0  
				color 0.7 0.7 0.7 
				node Cylinder { radius 0.02 height 1 }
			}
			TransColored { 
				translation 0 1.05 0
				color 0.7 0.7 0.7 
				node Cone { height 0.1 bottomRadius 0.04 }
			}
		]
        }
        Transform {
                rotation 1 0 0 1.5708
                children [USE CARROW]
        }
        Transform {
                rotation 0 0 1 -1.5708
                children [USE CARROW]
        }
	DEF BLUEBOX TransColored {
		color 0.2 0.2 0.8 
		node  Box {
			size 0.03 0.2 0.03
		}
	}
	Group {
		children [
			TransColored { 
				color 0.7 0.2 0.7
				node DEF EGRID ElevationGrid {
					solid FALSE
				}
			}
			DEF TOUCH TouchSensor {}
		]
	}
","directly from source code");

($egrid,$bbox,$touch) = map {$b->api_getNode($_)} qw/EGRID BLUEBOX TOUCH/;

$b->api__registerListener($touch, hitPoint_changed, \&set_point);

sub set_point {
	my($val) = @_;
	local ($x,$y);
	$x = $val->[0]*($xmax-$xmin)+$xmin;
	$y = $val->[2]*($ymax-$ymin)+$ymin;
	$value = eval "$func";
	($x,$y,$value) = map {sprintf "%2.2f",$_} ($x,$y,$value);
	$l1->set("X: $x");
	$l2->set("Y: $y");
	$l3->set("F: $value");
	$b->api__sendEvent($bbox, "translation", $val);
}

sub do_plot {
	my @funcs;
	local($x,$y);
	for my $cx (0..$nx) {
	    for my $cy (0..$ny) {
	    	$x = $cx * ($xmax-$xmin) / $nx + $xmin;
	    	$y = $cy * ($ymax-$ymin) / $ny + $ymin;
	    	$funcs[$cx*($nx+1) + $cy] = eval $func;
	    }
	}
	$b->api__sendEvent($egrid, height, \@funcs);
	$b->api__sendEvent($egrid, xDimension, $nx+1);
	$b->api__sendEvent($egrid, xSpacing, 1/$nx);
	$b->api__sendEvent($egrid, zDimension, $ny+1);
	$b->api__sendEvent($egrid, zSpacing, 1/$ny);
}

sub create_main {
	my $window = Gtk::Window->new("toplevel");
        $window->signal_connect("destroy" => \&Gtk::main_quit);
        $window->signal_connect("delete_event" => \&Gtk::false);

	$box = Gtk::VBox->new(0,0);
	$window->add($box);

	$label = Gtk::Label->new("Function:");
	$box->pack_start($label,1,1,0);

	$entry = Gtk::Entry->new;
	$entry->set_usize(0,25);
	$entry->set_text($func);
	$entry->signal_connect(changed => sub {$func = $_[0]->get_text();});
	$box->pack_start($entry,1,1,0);

	$button = Gtk::Button->new("Plot");
	$box->pack_start($button,1,1,0);
	$button->signal_connect("clicked" => \&do_plot);

	$box->pack_start(($l1 = Gtk::Label->new("")),1,1,0);
	$box->pack_start(($l2 = Gtk::Label->new("")),1,1,0);
	$box->pack_start(($l3 = Gtk::Label->new("")),1,1,0);

	$idle = Gtk->idle_add(sub {$b->tick;1}, $window);

	for($label,$l1,$l2,$l3,$box,$entry,$button,$window) { $_->show }

}

$b->prepare();

create_main();
do_plot();
set_point([0,0,0]);
Gtk->main;
