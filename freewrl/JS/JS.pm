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

our $VERSION = '0.20';

require Exporter;
require DynaLoader;

our @ISA = qw{Exporter DynaLoader};
our @EXPORT = qw{
				 new,
				 initialize,
				 cleanup
				};

bootstrap VRML::JS $VERSION;

## Debug:
##$VRML::verbose::js = 1;

if ($VRML::verbose::js) {
	setVerbose(1);
}

sub numeric { $_[0]+0 }

## Referenced from JS global object ???
our %Types = (
			  SFBool => sub { $_[0] ? "true" : "false" },
			  SFFloat => \&numeric,
			  SFTime => \&numeric,
			  SFInt32 => \&numeric,
			  SFString => sub { '"'.$_[0].'"' }, # XXX
			  ##SFNode => sub { print "\nnew SFNode: $_[0]\n"; 'new SFNode("","'.(VRML::Handles::reserve($_[0])).'")'}
			  SFNode => sub {
				  my $handle;
				  if (VRML::Handles::check($_[0])) {
					  $handle = VRML::Handles::get($_[0]);
				  } else {
					  $handle = VRML::Handles::reserve($_[0]);
				  }
				  return 'new SFNode('."\"$handle\"".')';
			  }
			 );


## See VRML97, section 4.12 (Scripting)
my $DefaultScriptMethods = "function initialize() {}; function shutdown() {}; function eventsProcessed() {}; TRUE=true; FALSE=false;";


sub new {
	my ($type, $text, $node, $browser) = @_;
	my $this = bless {
					  Node => $node,
					  Browser => $browser,
					  JSContext => undef,
					  JSGlobal => undef,
					  BrowserIntern => undef
					 }, $type;
	print "VRML::JS::new\n$text\n" if $VRML::verbose::js;
	${$this->{Browser}}{JSCleanup} = \&{cleanup};

	init($this->{JSContext}, $this->{JSGlobal}, $this->{BrowserIntern}, $this);
	print "\tcontext = $this->{JSContext}, global object = $this->{JSGlobal}\n"
		if $VRML::verbose::js;

	my ($rs, $rval);

	if (!runScript($this->{JSContext},
				   $this->{JSGlobal},
				   $DefaultScriptMethods,
				   $rs,
				   $rval)) {
		cleanupDie("runScript failed in VRML::JS");
	}
	print "\trunScript returned: $rs, $rval\n" if $VRML::verbose::js;
	if (!runScript($this->{JSContext}, $this->{JSGlobal}, $text, $rs, $rval)) {
		cleanupDie("runScript failed in VRML::JS");
	}
	print "\trunScript returned: $rs, $rval\n" if $VRML::verbose::js;

	# Initialize fields.
	my $nt = $node->{Type};
	my @k = keys %{$nt->{Defaults}};
	my ($type, $ftype, $value, $constr, $rval, $v);

	print "\tNode type = ", %{$nt}, ", " if $VRML::verbose::js;
	print "\tFields: \n" if $VRML::verbose::js;
	for (@k) {
		next if $_ eq "url" or $_ eq "mustEvaluate" or $_ eq "directOutput";

		#my $type = $nt->{FieldTypes}{$_};
		#my $ftype = "VRML::Field::$type";
		$type = $nt->{FieldTypes}{$_};
		$ftype = "VRML::Field::$type";
		$constr = "new "."$type"."()";

		print "\t\tJS field $_ of type $ftype, ", $nt->{FieldKinds}{$_}, "\n"
			if $VRML::verbose::js;
		if ($nt->{FieldKinds}{$_} eq "field" or $nt->{FieldKinds}{$_} eq "eventOut") {
			if ($Types{$type}) {
				# addwatchprop($this->{JSContext},$this->{JSGlobal}, $_);
				addTouchableProperty($this->{JSContext}, $this->{JSGlobal}, $_);
			} else {
				# addasgnprop($this->{JSContext},$this->{JSGlobal}, $_, $ftype->js_default);
				addAssignProperty($this->{JSContext}, $this->{JSGlobal}, $_, $constr);
			}

			if ($nt->{FieldKinds}{$_} eq "field") {
				$value = $node->{RFields}{$_};
				print "\t\t\tJS field property $_\n" if $VRML::verbose::js;
				if ($Types{$type}) {
					print "\t\t\t\tset type $_ '$value'\n" if $VRML::verbose::js;

					# my $v = runScript($this->{CX},$this->{GLO},"$_=".$Types{$type}->($value),$rs);
					if (!runScript($this->{JSContext}, $this->{JSGlobal}, "$_=".$Types{$type}->($value), $rs, $v)) {
						cleanupDie("runScript failed in VRML::JS::new");
					}
				} else {
					$this->setProperty($_, $value, $_);
				}
			}
		} elsif ($nt->{FieldKinds}{$_} eq "eventIn") {
			if (!$Types{$type}) {
				# addasgnprop($this->{JSContext},$this->{JSGlobal}, "__tmp_arg_$_", $ftype->js_default);
				addAssignProperty($this->{JSContext}, $this->{JSGlobal}, "__tmp_arg_$_", $constr);
			}
		} else {
			warn("Invalid field type '$_' for $node->{TypeName}");
		}
	}
	# Ignore all events we may have sent while building
	$this->gatherSentEvents(1);


	## Debug:
	#$this->{Browser}{Scene}->dump(0);

	return $this;
}

sub cleanupDie {
	my ($this, $msg) = @_;
	cleanup();
	die($msg);
}

sub initialize {
	my ($this) = @_;
	my ($rs, $rval);

	if (!runScript($this->{JSContext}, $this->{JSGlobal}, "initialize()", $rs, $rval)) {
		cleanupDie("runScript failed in VRML::JS::initialize");
	}
	print "VRML::JS::initialize: runScript returned $rval, $rs\n" if $VRML::verbose::js;
	$this->gatherSentEvents();
}

sub cleanup {
	my ($this) = @_;
	cleanupJS($this->{JSContext}, $this->{BrowserIntern});
}

sub sendeventsproc {
	my($this) = @_;
	my ($rs, $rval);
	if (!runScript($this->{JSContext}, $this->{JSGlobal}, "eventsProcessed()", $rs, $rval)) {
		cleanupDie("runScript failed in VRML::JS::initialize");
	}
	print "VRML::JS::sendeventproc: runScript returned $rval, $rs\n" if $VRML::verbose::js;
	$this->gatherSentEvents();
}

sub gatherSentEvents {
	my ($this, $ignore) = @_;
	my $node = $this->{Node};
	my $t = $node->{Type};
	my @k = keys %{$t->{Defaults}};
	my @a;
	my ($rs, $v, $rval);
	print "VRML::JS::gatherSentEvents\n" if $VRML::verbose::js;
	for (@k) {
		next if $_ eq "url";
		my $type = $t->{FieldTypes}{$_};
		my $ftyp = $type;
		if ($t->{FieldKinds}{$_} eq "eventOut") {
			print "\teventOut $_\n" if $VRML::verbose::js;

			if ($type =~ /^MF/) {
				if (!runScript($this->{JSContext}, $this->{JSGlobal}, "$_.__touched_flag", $rs, $v)) {
					cleanupDie("runScript failed in VRML::JS::gatherSentEvents");
				}
				print "\trunScript returned: $v, $rs, $_\n" if $VRML::verbose::js;
				if (!runScript($this->{JSContext}, $this->{JSGlobal}, "$_.__touched_flag=0", $rs, $rval)) {
					cleanupDie("runScript failed in VRML::JS::gatherSentEvents");
				}
				print "\trunScript returned: $rval, $rs, $_\n" if $VRML::verbose::js;
			} elsif ($Types{$ftyp}) {
				if (!runScript($this->{JSContext}, $this->{JSGlobal}, "_${_}_touched", $rs, $v)) {
					cleanupDie("runScript failed in VRML::JS::gatherSentEvents");
				}
				print "\trunScript returned: $v, $rs, $_\n" if $VRML::verbose::js;
				if (!runScript($this->{JSContext}, $this->{JSGlobal}, "_${_}_touched=0", $rs, $rval)) {
					cleanupDie("runScript failed in VRML::JS::gatherSentEvents");
				}
				print "\trunScript returned: $rval, $rs, $_\n" if $VRML::verbose::js;
				# print "SIMP_TOUCH $v\n";
			} else {
				if (!runScript($this->{JSContext}, $this->{JSGlobal}, "$_.__touched()", $rs, $v)) {
					cleanupDie("runScript failed in VRML::JS::gatherSentEvents");
				}
				print "\trunScript returned: $v, $rs, $_\n" if $VRML::verbose::js;
			}
			if ($v && !$ignore) {
				push @a, [$node, $_, $this->getProperty($type, $_)]; # $this->get_prop($type,$_)];
			}
		}
		# $this->{O}->print("$t->{FieldKinds}{$_}\n
	}
	return @a;
}

sub sendevent {
	my ($this, $node, $event, $value, $timestamp) = @_;
	my $type = $node->{Type}{FieldTypes}{$event};
	my $aname = "__tmp_arg_$event";
	my ($rs, $rval);

	print "VRML::JS::sendevent $node $event $value $timestamp ($type)\n" if $VRML::verbose::js;
	$this->setProperty($event, $value, $aname);

	if (!runScript($this->{JSContext}, $this->{JSGlobal}, "$event($aname,$timestamp)", $rs, $rval)) {
		cleanupDie("runScript failed in VRML::JS::sendevent");
	}
	print "\trunScript returned: $rval, $rs\n" if $VRML::verbose::js;
	return $this->gatherSentEvents();

	unless($Types{$type}) {
		##&{"$node->{Type}{FieldTypes}{$event}"."SetInternal"}($this->{JSContext}, $this->{JSGlobal}, "__evin", $value);
		&{"js$type"."Set"}($this->{JSContext}, $this->{JSGlobal}, "__evin", $value);
		# runScript($this->{JSContext}, $this->{JSGlobal}, "$event(__evin,$timestamp)", $rs);
		if (!runScript($this->{JSContext}, $this->{JSGlobal}, "$event(__evin,$timestamp)", $rs, $rval)) {
			cleanupDie("runScript failed in VRML::JS::sendevent");
		}
		print "\trunScript returned: $rval, $rs\n" if $VRML::verbose::js;
	} else {
		print "\t$event $timestamp\n"."$event(".$Types{$type}->($value).",$timestamp)\n"
				if $VRML::verbose::js;
		# my $v = runScript($this->{JSContext}, $this->{JSGlobal}, "$event(".$Types{$type}->($value).",$timestamp)", $rs);
		if (!runScript($this->{JSContext}, $this->{JSGlobal}, "$event(".$Types{$type}->($value).",$timestamp)", $rs, $rval)) {
			cleanupDie("runScript failed in VRML::JS::sendevent");
		}
		print "\trunScript returned: $rval, $rs\n" if $VRML::verbose::js;
	}
	$this->gatherSentEvents();
}


sub setProperty { # Assigns a value to a property.
	my ($this, $field, $value, $prop) = @_;
	my $typ = $this->{Node}{Type};
	my ($ftype, $rs, $i, $rval);
	if ($field =~ s/^____//) { # recurse hack
		$ftype = $field;
	} else {
		$ftype = $typ->{FieldTypes}{$field};
	}

	print "VRML::JS::setProperty\n" if $VRML::verbose::js;
	if ($ftype =~ /^MF/) {
		my $styp = $ftype;
		$styp =~ s/^MF/SF/;
		for ($i = 0; $i < $#{$value}; $i++) {
			$this->setProperty("____$styp", $value->[$i], "____tmp");
#JS -added the rs parameter; lets see if this works....

			# runScript($this->{JSContext}, $this->{JSGlobal}, "$prop"."[$i] = ____tmp", $rs);
			if (!runScript($this->{JSContext}, $this->{JSGlobal}, "$prop"."[$i]=____tmp", $rs, $rval)) {
				cleanupDie("runScript failed in VRML::JS::setProperty");
			}
		}
		# runScript($this->{JSContext},$this->{JSGlobal}, "$prop.__touched_flag = 0", $rs);
		if (!runScript($this->{JSContext}, $this->{JSGlobal}, "$prop.__touched_flag=0", $rs, $rval)) {
			cleanupDie("runScript failed in VRML::JS::setProperty");
		}
	} elsif ($Types{$ftype}) {
		# runScript($this->{JSContext},$this->{JSGlobal}, "$prop = ".(&{$Types{$ftype}}($value)), $rs);
		if (!runScript($this->{JSContext}, $this->{JSGlobal}, "$prop=".(&{$Types{$ftype}}($value)), $rs, $rval)) {
			cleanupDie("runScript failed in VRML::JS::setProperty");
		}
		# runScript($this->{JSContext},$this->{JSGlobal},"_${prop}__touched=0", $rs);
		if (!runScript($this->{JSContext}, $this->{JSGlobal}, "_${prop}__touched=0", $rs, $rval)) {
			cleanupDie("runScript failed in VRML::JS::setProperty");
		}
	} else {
		print "\tjs$ftype"."Set $prop $value (", @{$value}, ")\n"
			if $VRML::verbose::js;
		&{"js$ftype"."Set"}($this->{JSContext}, $this->{JSGlobal}, $prop, $value);
		# runScript($this->{JSContext},$this->{JSGlobal},"$prop.__touched()", $rs);
		if (!runScript($this->{JSContext}, $this->{JSGlobal}, "$prop.__touched()", $rs, $rval)) {
			cleanupDie("runScript failed in VRML::JS::setProperty");
		}
	}
}

sub getProperty {
	my ($this, $type, $prop) = @_;
	my ($rs, $rval, $l);

	print "VRML::JS::getProperty args: @_\n" if $VRML::verbose::js;


	if ($type =~ /^SFNode$/) {
		# runScript($this->{JSContext},$this->{JSGlobal}, "$prop.__id",$rs);
		if (runScript($this->{JSContext}, $this->{JSGlobal}, "$prop.__id", $rs, $rval)) {
			cleanupDie("runScript failed in VRML::JS::getProperty");
		}
		return VRML::Handles::get($rs);
	} elsif ($type =~ /^MFNode$/) {
		# my $l = runScript($this->{JSContext},$this->{JSGlobal}, "$prop.length",$rs);
		if (runScript($this->{JSContext}, $this->{JSGlobal}, "$prop.length", $rs, $l)) {
			cleanupDie("runScript failed in VRML::JS::getProperty for \"$prop.length\"");
		}
		print "\trunScript returned length $l, $rs\n" if $VRML::verbose::js;
		my $fn = $prop;
		my @res = map {
			# runScript($this->{JSContext},$this->{JSGlobal}, "$fn",$rs);
			if (!runScript($this->{JSContext}, $this->{JSGlobal}, "$fn", $rs, $rval)) {
				cleanupDie("runScript failed in VRML::JS::getProperty");
			}
			print "\trunScript returned mfnode $rval, $rs\n" if $VRML::verbose::js;
			# runScript($this->{JSContext},$this->{JSGlobal}, "$fn"."[$_]",$rs);
			if (!runScript($this->{JSContext}, $this->{JSGlobal}, "$fn"."[$_]", $rs, $rval)) {
				cleanupDie("runScript failed in VRML::JS::getProperty");
			}
			print "\trunScript returned node $rval, $rs\n" if $VRML::verbose::js;
			# runScript($this->{JSContext},$this->{JSGlobal}, "$fn"."[$_][0]",$rs);
			if (!runScript($this->{JSContext}, $this->{JSGlobal}, "$fn"."[$_][0]", $rs, $rval)) {
				cleanupDie("runScript failed in VRML::JS::getProperty");
			}
			print "\trunScript returned node[0] $rval, $rs\n" if $VRML::verbose::js;
			# runScript($this->{JSContext},$this->{JSGlobal}, "$fn"."[$_].__id",$rs);
			if (!runScript($this->{JSContext}, $this->{JSGlobal}, "$fn"."[$_].__id", $rs, $rval)) {
				cleanupDie("runScript failed in VRML::JS::getProperty");
			}
			print "\trunScript returned $rval, $rs\n" if $VRML::verbose::js;
			VRML::Handles::get($rs);
		} (0..$l-1);
		return \@res;
	} elsif ($type =~ /^MFString$/) {
		# runScript($this->{JSContext}, $this->{JSGlobal}, "$prop.length", $rs, \$l);
		if (!runScript($this->{JSContext}, $this->{JSGlobal}, "$prop.length", $rs, $l)) {
			cleanupDie("runScript failed in VRML::JS::getProperty");
		}
		print "\trunScript returned length $l, $rs\n" if $VRML::verbose::js;
		my $fn = $prop;
		my @res = map {
			# runScript($this->{JSContext},$this->{JSGlobal}, "$fn"."[$_]",$rs);
			if (!runScript($this->{JSContext}, $this->{JSGlobal}, "$fn"."[$_]", $rs, $rval)) {
				cleanupDie("runScript failed in VRML::JS::getProperty");
			}
			print "\trunScript returned $rval, $rs\n" if $VRML::verbose::js;
			if (!runScript($this->{JSContext}, $this->{JSGlobal}, "$fn"."[$_]", $rs, $rval)) {
				cleanupDie("runScript failed in VRML::JS::getProperty");
			}
			print "\trunScript returned $rval, $rs\n" if $VRML::verbose::js;
			$rs;
		} (0..$l-1);
		return \@res;
	} elsif ($type =~ /^MF/) {
		# my $l = runScript($this->{JSContext},$this->{JSGlobal}, "$prop.length",$rs);
		if (!runScript($this->{JSContext}, $this->{JSGlobal}, "$prop.length", $rs, $l)) {
			cleanupDie("runScript failed in VRML::JS::getProperty");
		}
		print "\trunScript returned length $l, $rs\n" if $VRML::verbose::js;
		my $fn = $prop;
		my $st = $type;
		$st =~ s/MF/SF/;
		my @res = map {
			# runScript($this->{JSContext},$this->{JSGlobal}, "$fn"."[$_]",$rs);
			if (!runScript($this->{JSContext}, $this->{JSGlobal}, "$fn"."[$_]", $rs, $rval)) {
				cleanupDie("runScript failed in VRML::JS::getProperty");
			}
			print "\trunScript returned $rval, $rs\n" if $VRML::verbose::js;
			(pos $rs) = 0;
			"VRML::Field::$st"-> parse(undef, $rs);
		} (0..$l-1);
		print "\tarray \@res:\n" if $VRML::verbose::js;
		for (@res) {
			if ("ARRAY" eq ref $_) {
				print "@$_\n"
					if $VRML::verbose::js;
			}
		}
		my $r = \@res;
		print "\treference to \@ref: $r\n" if $VRML::verbose::js;
		return $r;
	} elsif ($Types{$type}) {
		# my $v = runScript($this->{JSContext},$this->{JSGlobal}, "_${_}_touched=0; $prop",$rs);
		if (!runScript($this->{JSContext}, $this->{JSGlobal}, "_${_}_touched=0; $prop", $rs, $rval)) {
			cleanupDie("runScript failed in VRML::JS::getProperty");
		}
		print "\trunScript returned: $rval, $rs\n" if $VRML::verbose::js;
		# return $v;
		return $rval;
	} else {
		# runScript($this->{JSContext},$this->{JSGlobal}, "$prop",$rs);
		if (!runScript($this->{JSContext}, $this->{JSGlobal}, "$prop", $rs, $rval)) {
			cleanupDie("runScript failed in VRML::JS::getProperty");
		}
		print "\trunScript returned: $rval, $rs\n" if $VRML::verbose::js;
		# print "VAL: $rs\n";
		(pos $rs) = 0;
		return "VRML::Field::$type"->parse(undef, $rs);
	}
}

sub nodeSetProperty {
#	my ($this) = @_;
#	my ($node, $prop, $val, $rval);
	my ($this, $node, $prop) = @_;
	my ($val, $rval);

	print "VRML::JS::nodeSetProperty args: @_\n" if $VRML::verbose::js;

	# runScript($this->{JSContext},$this->{JSGlobal},"__node.__id",$node);
	# runScript($this->{JSContext},$this->{JSGlobal},"__prop",$prop);
	if (!runScript($this->{JSContext}, $this->{JSGlobal}, "__node.__id", $node, $rval)) {
		cleanupDie("runScript failed in VRML::JS::nodeSetProperty");
	}
	if (!runScript($this->{JSContext}, $this->{JSGlobal}, "__prop", $prop, $rval)) {
		cleanupDie("runScript failed in VRML::JS::nodeSetProperty");
	}

	print "\tsetting property $prop for $node\n" if $VRML::verbose::js;
	$node = VRML::Handles::get($node);

	my $vt = $node->{Type}{FieldTypes}{$prop};
	if (!defined $vt) {
		cleanupDie("Javascript tried to assign to invalid property!\n");
	}
##print "\t$node"."\-\>{Type}{FieldTypes}{$prop} is $vt\n";
	my $val = $this->getProperty($vt, "__val");

	if (0) {
		#	if ($vt =~ /Node/) {cleanupDie("Can't handle yet");}
		#	if ($Types{$vt}) {
		#		runScript($this->{JSContext},$this->{JSGlobal},"__val",$val);
		#		print "GOT '$val'\n";
		#		$val = "VRML::Field::$vt"->parse(undef, $val);
		#	} else {
		#		runScript($this->{JSContext},$this->{JSGlobal},"__val.toString()",$val);
		#		print "GOT '$val'\n";
		#		$val = "VRML::Fields::$vt"->parse(undef, $val);
		#	}
	}

	print "\tsetting property to $val\n" if $VRML::verbose::js;
	$node->{RFields}{$prop} = $val;
}

## browser object set in JS global object ???
sub browserGetName {
	my ($this) = @_;
	my ($rval, $rs);

	print "browserGetName ($this) !\n"
		if $VRML::verbose::js;
	my $n = $this->{Browser}->getName();
	# runScript($this->{JSContext}, $this->{JSGlobal}, "Browser.__bret = \"$n\"", $rs);
	if (!runScript($this->{JSContext}, $this->{JSGlobal}, "Browser.__bret=\"$n\"", $rs, $rval)) {
		cleanupDie("runScript failed in VRML::JS::browserGetName");
	}
}

sub browserGetVersion {
	my ($this) = @_;
	my ($rval, $rs);

	print "browserGetVersion ($this) !\n"
		if $VRML::verbose::js;
	my $n = $this->{Browser}->getVersion();

	# runScript($this->{JSContext}, $this->{JSGlobal}, "Browser.__bret = \"$n\"", $rs);
	runScript($this->{JSContext}, $this->{JSGlobal}, "Browser.__bret=\"$n\"", $rs, $rval);
}

## ????
#sub browserSetDescription {
#	my ($this, $desc) = @_;
#	print "browserSetDescription ($this) !\n"
#		if $VRML::verbose::js;
#	my $n = $this->{Browser}->setDescription($desc);
#	my $rs;
#	runScript($this->{JSContext}, $this->{JSGlobal}, "Browser.__bret = \"$n\"", $rs);
#}

## VRML::Browser::addRoute doesn't return anything...FIX!!!
sub browserAddRoute {
	my ($this, $str) = @_;
	my ($rval, $rs);

	if ($str =~ /^([^ ]+) ([^ ]+) ([^ ]+) ([^ ]+)$/) {
		my ($fn, $ff, $tn, $tf) = ($1, $2, $3, $4);
		print "browserAddRoute args: $fn, $ff, $tn, $tf\n"
			if $VRML::verbose::js;

		my $n = $this->{Browser}->addRoute($fn, $ff, $tn, $tf, $this->{Browser}{Scene});

		# runScript($this->{JSContext}, $this->{JSGlobal}, "Browser.__bret = $n", $rs);
		if (!runScript($this->{JSContext}, $this->{JSGlobal}, "Browser.__bret=$n", $rs, $rval)) {
			cleanupDie("runScript failed in VRML::JS::browserAddRoute");
		}
	} else {
		cleanupDie("Invalid route for addRoute.");
	}
}

## VRML::Browser::deleteRoute doesn't return anything...FIX!!!
sub browserDeleteRoute {
	my ($this, $str) = @_;
	my ($rval, $rs);

	if ($str =~ /^([^ ]+) ([^ ]+) ([^ ]+) ([^ ]+)$/) {
		my ($fn, $ff, $tn, $tf) = ($1, $2, $3, $4);
		print "browserDeleteRoute args: $fn, $ff, $tn, $tf\n"
			if $VRML::verbose::js;

		my $n = $this->{Browser}->deleteRoute($fn, $ff, $tn, $tf, $this->{Browser}{Scene});

		if (!runScript($this->{JSContext}, $this->{JSGlobal}, "Browser.__bret=$n", $rs, $rval)) {
			cleanupDie("runScript failed in VRML::JS::browserAddRoute");
		}
	} else {
		cleanupDie("Invalid route for deleteRoute.");
	}
}

sub browserGetCurrentFrameRate {
	my ($this) = @_;
	my ($rval, $rs);

	print "browserGetCurrentFrameRate ($this) !\n"
		if $VRML::verbose::js;
	my $n = $this->{Browser}->getCurrentFrameRate();

	# runScript($this->{JSContext}, $this->{JSGlobal}, "Browser.__bret = $n", $rs);
	if (!runScript($this->{JSContext}, $this->{JSGlobal}, "Browser.__bret=$n", $rs, $rval)) {
		cleanupDie("runScript failed in VRML::JS::browserGetCurrentFrameRate");
	}
}

sub browserGetWorldURL {
	my ($this) = @_;
	my ($rval, $rs);

	print "browserGetWorldURL ($this) !\n"
		if $VRML::verbose::js;
	my $n = $this->{Browser}->getWorldURL();

	# runScript($this->{JSContext}, $this->{JSGlobal}, "Browser.__bret = $n", $rs);
	if (!runScript($this->{JSContext}, $this->{JSGlobal}, "Browser.__bret=$n", $rs, $rval)) {
		cleanupDie("runScript failed in VRML::JS::browserGetWorldURL");
	}
}

sub browserCreateVrmlFromString {
	my ($this, $str) = @_;
	my ($rval, $rs);

	print "browserCreateVrmlFromString: string = \"$str\"\n"
		if $VRML::verbose::js;
	my $mfn = $this->{Browser}->createVrmlFromString($str);
	my @hs = map {VRML::Handles::reserve($_)} @$mfn;
##print "\nbrowserCreateVrmlFromString: $mfn, ", %{$mfn}, ", @hs\n";
	my $sc = "Browser.__bret=new MFNode(".(join ',',map {qq{new SFNode("$_")}} @hs).")";

	# runScript($this->{JSContext}, $this->{JSGlobal}, $sc, $rs);
	if (!runScript($this->{JSContext}, $this->{JSGlobal}, $sc, $rs, $rval)) {
		cleanupDie("runScript failed in VRML::JS::browserCreateVrmlFromString");
	}
}

sub browserCreateVrmlFromURL {
	my ($this) = @_;
	my ($rval, $rs);

	# runScript($this->{JSContext}, $this->{JSGlobal}, "Browser.__arg0", $rs);
	if (!runScript($this->{JSContext}, $this->{JSGlobal}, "Browser.__arg0", $rs, $rval)) {
		cleanupDie("runScript failed in VRML::JS::browserCreateVrmlFromURL");
	}

	print "browserCreateVrmlFromURL '$rs'\n"
		if $VRML::verbose::js;

	my $mfn = $this->{Browser}->createVrmlFromURL($rs);
	my @hs = map {VRML::Handles::reserve($_)} @$mfn;
	my $sc = "Browser.__bret=new MFNode(".(join ',',map {qq{new SFNode("$_")}} @hs).")";

	# runScript($this->{JSContext}, $this->{JSGlobal}, $sc, $rs);
	if (!runScript($this->{JSContext}, $this->{JSGlobal}, $sc, $rs, $rval)) {
		cleanupDie("runScript failed in VRML::JS::browserCreateVrmlFromURL");
	}
}
