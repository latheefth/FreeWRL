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
				 sendeventsproc,
				 sendevent,
				 cleanup
			 };

bootstrap VRML::JS $VERSION;


## Debug:
##$VRML::verbose::js=1;


if ($VRML::verbose::js) {
	setVerbose(1);
}

our $ECMAScriptNative = qr{^SF(?:Bool|Float|Time|Int32|String)$};


## See VRML97, section 4.12 (Scripting)
my $DefaultScriptMethods = "function initialize() {}; function shutdown() {}; function eventsProcessed() {}; TRUE=true; FALSE=false;";


sub new {
	my ($type, $text, $node, $browser) = @_;
	my $this = bless {
					  Node => $node,
					  Browser => $browser,
					  BrowserIntern => undef,
					  JSContext => undef,
					  JSGlobal => undef
					 }, $type;
	print "VRML::JS::new\n$text\n" if $VRML::verbose::js;
	${$this->{Browser}}{JSCleanup} = \&{cleanup};

	init($this->{JSContext}, $this->{JSGlobal}, $this->{BrowserIntern}, $this);
	print "\tcontext $this->{JSContext}, global object $this->{JSGlobal}\n"
		if $VRML::verbose::js;

	my ($rs, $rval);

	if (!runScript($this->{JSContext},
				   $this->{JSGlobal},
				   $DefaultScriptMethods,
				   $rs,
				   $rval)) {
		cleanupDie("runScript failed in VRML::JS");
	}

	if (!runScript($this->{JSContext}, $this->{JSGlobal}, $text, $rs, $rval)) {
		cleanupDie("runScript failed in VRML::JS");
	}

	# Initialize fields.
	my $nt = $node->{Type};
	my @k = keys %{$nt->{Defaults}};
	my ($type, $ftype, $value, $constr, $v);

	print "\tFields: \n" if $VRML::verbose::js;
	for (@k) {
		next if $_ eq "url" or
			$_ eq "directOutput" or
			$_ eq "mustEvaluate";

		$this->initScriptFields($node, $_);
	}
	# Ignore all events we may have sent while building
	$this->gatherSentEvents(1);

	return $this;
}

sub initScriptFields {
	my ($this, $node, $field) = @_;
	my $nt = $node->{Type};
	my $type = $nt->{FieldTypes}{$field};
	my $ftype = "VRML::Field::$type";
	my $fkind = $nt->{FieldKinds}{$field};
	my ($rstr, $v);

	print "VRML::JS::initScriptFields: field $field, type $type, field kind ",
		$nt->{FieldKinds}{$field}, "\n"
		if $VRML::verbose::js;

	if ($fkind eq "eventIn") {
		if ($type !~ /$ECMAScriptNative/) {
			$constr = $this->constrString($type, 0);
			if (!addGlobalAssignProperty($this->{JSContext}, $this->{JSGlobal},
								   "__tmp_arg_$field", $constr)) {
				$this->cleanupDie("addGlobalAssignProperty failed in VRML::JS::new");
			}
		}
	} elsif ($fkind eq "eventOut") {
		if ($type =~ /$ECMAScriptNative/) {
			if (!addGlobalECMANativeProperty($this->{JSContext}, $this->{JSGlobal},
											 $field)) {
				$this->cleanupDie("addGlobalECMANativeProperty failed in VRML::JS::new");
			}
		} else {
			if ($type eq "SFNode") {
				$value = $node->{RFields}{$field};
				print "\tJS field property $field, value ",
					(ref $value eq "ARRAY" ?
					 "(".join(', ', @{$value}).")" : "$value"), "\n"
				 if $VRML::verbose::js;
				$constr = $this->constrString($type, $value);
				$this->initSFNodeFields($field, $value);
			} else {
				$constr = $this->constrString($type, 0);
			}
			if (!addGlobalAssignProperty($this->{JSContext}, $this->{JSGlobal},
								   $field, $constr)) {
				$this->cleanupDie("addGlobalAssignProperty failed in VRML::JS::new");
			}
		}
	} elsif ($fkind eq "field") {
		$value = $node->{RFields}{$field};
		print "\tJS field property $field, value ",
			(ref $value eq "ARRAY" ?
			 "(".join(', ', @{$value}).")" : "$value"), "\n"
				 if $VRML::verbose::js;

		if ($type =~ /$ECMAScriptNative/) {
			if (!addGlobalECMANativeProperty($this->{JSContext}, $this->{JSGlobal},
											 $field)) {
				$this->cleanupDie("addGlobalECMANativeProperty failed in VRML::JS::new");
			}
			if (!runScript($this->{JSContext}, $this->{JSGlobal},
						   "$field=".$ftype->as_string($value, 1), $rstr, $v)) {
				$this->cleanupDie("runScript failed in VRML::JS::new");
			}
		} else {
			$constr = $this->constrString($type, $value);
			if (!addGlobalAssignProperty($this->{JSContext}, $this->{JSGlobal},
								   $field, $constr)) {
				$this->cleanupDie("addGlobalAssignProperty failed in VRML::JS::new");
			}
			if ($type eq "SFNode") {
				$this->initSFNodeFields($field, $value);
			}
		}
	} else {
		warn("Invalid field $fkind $field for $node->{TypeName} in initScriptFields");
	}
}

sub initSFNodeFields {
	my ($this, $nodeName, $node) = @_;
	my $nt = $node->{Type};
	my $ntn = $node->{TypeName};
	my ($constr, $fkind, $ftype, $type, $value, $rstr, $v);
	my @fields = keys %{$node->{Fields}};

	print "VRML::JS::initSFNodeFields: $nodeName, $ntn, [ ",
		join(", ",
			 map("$nt->{FieldTypes}{$_} $_", @fields)
			), " ]\n"
				if $VRML::verbose::js;

	for (@fields) {
		next if $_ eq "url" or
			$_ eq "directOutput" or
			$_ eq "mustEvaluate";

		$fkind = $nt->{FieldKinds}{$_};
		$type = $nt->{FieldTypes}{$_};
		$ftype = "VRML::Field::$type";

		if ($fkind eq "eventIn") { ## correct???
			if ($type !~ /$ECMAScriptNative/) {
				$constr = $this->constrString($type, 0);
				if (!addSFNodeProperty($this->{JSContext}, $this->{JSGlobal},
									   $nodeName, $_, $constr)) {
					$this->cleanupDie("addSFNodeProperty failed in VRML::JS::new");
				}
			}
		} elsif ($fkind eq "eventOut") {
			if ($type !~ /$ECMAScriptNative/) {
				if ($type eq "SFNode") {
					$value = $node->{RFields}{$_};
					print "\tJS field property $_, value ",
						(ref $value eq "ARRAY" ?
						 "(".join(', ', @{$value}).")" : "$value"), "\n"
							 if $VRML::verbose::js;

					$constr = $this->constrString($type, $value);
				} else {
					$constr = $this->constrString($type, 0);
				}
				if (!addSFNodeProperty($this->{JSContext}, $this->{JSGlobal},
									   $nodeName, $_, $constr)) {
					$this->cleanupDie("addSFNodeProperty failed in VRML::JS::new");
				}
			}
		} elsif ($fkind =~ /^(?:exposed)??[Ff]ield$/) {
			$value = $node->{RFields}{$_};
			print "\tJS field property $_, value ",
				(ref $value eq "ARRAY" ?
				 "(".join(', ', @{$value}).")" : "$value"), "\n"
					 if $VRML::verbose::js;

			if ($type !~ /$ECMAScriptNative/) {
				$constr = $this->constrString($type, $value);
				if (!addSFNodeProperty($this->{JSContext}, $this->{JSGlobal},
									   $nodeName, $_, $constr)) {
					$this->cleanupDie("addSFNodeProperty failed in VRML::JS::new");
				}
			}
		} else {
			warn("Invalid field $fkind $_ for $ntn in initSFNodeFields");
		}
	}
}

sub constrString {
	my ($this, $ft, $v) = @_;
	my ($i, $l, $sft, $h, $c);

	if ($v) {
		$c = "new $ft(";
		if ($ft =~ /^SFNode/) {
			if (VRML::Handles::check($v)) {
				$h = VRML::Handles::get($v);
			} else {
				$h = VRML::Handles::reserve($v);
			}
			$c .= "'".VRML::Field::SFNode->as_string($v)."','".$h."'";
		} elsif ($ft =~ /^MFString/) {
			$l = $#{$v} + 1;
			for ($i = 0; $i < $l; $i++) {
				$c .= "'".$v->[$i]."'";
				$c .= "," unless ($i == ($l - 1));
			}
		} elsif ($ft =~ /^MFNode/) {
			$l = $#{$v} + 1;
			for ($i = 0; $i < $l; $i++) {
				if (VRML::Handles::check($v->[$i])) {
					$h = VRML::Handles::get($v->[$i]);
				} else {
					$h = VRML::Handles::reserve($v->[$i]);
				}
				$c .= "new SFNode('".VRML::Field::SFNode->as_string($v->[$i])."','".$h."')";
				$c .= "," unless ($i == ($l - 1));
			}
		} elsif ($ft =~ /^MF(?:Color|Rotation|Vec2f|Vec3f)$/) {
			$sft = $ft;
			$sft =~ s/^MF/SF/;
			$l = $#{$v} + 1;
			for ($i = 0; $i < $l; $i++) {
				if (ref($v->[$i]) eq "ARRAY") {
					$h = join(",", @{$v->[$i]});
					$c .= "new $sft(".$h.")";
					$c .= "," unless ($i == ($l - 1));
				}
			}
		} elsif (ref($v) eq "ARRAY") {
			$c .= join(",", @{$v});
		} else {
			$c .= $v;
		}
		$c .= ")";
	} else {
		$c = "new $ft()";
	}
	print "VRML::JS::constrString: $c\n" if $VRML::verbose::js;
	return $c;
}


sub cleanupDie {
	my ($this, $msg) = @_;
	cleanup();
	die($msg);
}

sub cleanup {
	my ($this) = @_;
	cleanupJS($this->{JSContext}, $this->{BrowserIntern});
}

sub initialize {
	my ($this) = @_;
	my ($rs, $val);
	print "VRML::JS::initialize\n" if $VRML::verbose::js;

	if (!runScript($this->{JSContext}, $this->{JSGlobal},
				   "initialize()", $rs, $val)) {
		cleanupDie("runScript failed in VRML::JS::initialize");
	}

	$this->gatherSentEvents();
}

sub sendeventsproc {
	my($this) = @_;
	my ($rs, $rval);
	print "VRML::JS::sendeventproc\n" if $VRML::verbose::js;
	if (!runScript($this->{JSContext}, $this->{JSGlobal},
				   "eventsProcessed()", $rs, $rval)) {
		cleanupDie("runScript failed in VRML::JS::sendeventproc");
	}

	return $this->gatherSentEvents();
}

sub gatherSentEvents {
	my ($this, $ignore) = @_;
	my $node = $this->{Node};
	my $t = $node->{Type};
	my @k = keys %{$t->{Defaults}};
	my @a;
	my ($rstr, $rnum, $runused, $propval);
	print "VRML::JS::gatherSentEvents: ",
		($ignore ? "events ignored" : ""), "\n"
			if $VRML::verbose::js;
	for (@k) {
		next if $_ eq "url";
		my $type = $t->{FieldTypes}{$_};
		my $ftyp = $type;
		if ($t->{FieldKinds}{$_} eq "eventOut") {
			print "\teventOut $_\n" if $VRML::verbose::js;

			if ($type =~ /^MF/) {
				if (!runScript($this->{JSContext}, $this->{JSGlobal},
							   "$_.__touched_flag", $rstr, $rnum)) {
					cleanupDie("runScript failed in VRML::JS::gatherSentEvents");
				}
				if (!runScript($this->{JSContext}, $this->{JSGlobal},
							   "$_.__touched_flag=0", $rstr, $runused)) {
					cleanupDie("runScript failed in VRML::JS::gatherSentEvents");
				}
			} elsif ($ftyp =~ /$ECMAScriptNative/) {
				if (!runScript($this->{JSContext}, $this->{JSGlobal},
							   "_${_}_touched", $rstr, $rnum)) {
					cleanupDie("runScript failed in VRML::JS::gatherSentEvents");
				}
				if (!runScript($this->{JSContext}, $this->{JSGlobal},
							   "_${_}_touched=0", $rstr, $runused)) {
					cleanupDie("runScript failed in VRML::JS::gatherSentEvents");
				}
			} else {
				if (!runScript($this->{JSContext}, $this->{JSGlobal},
							   "$_.__touched()", $rstr, $rnum)) {
					cleanupDie("runScript failed in VRML::JS::gatherSentEvents");
				}
			}
			if ($rnum and !$ignore) {
				$propval = $this->getProperty($type, $_);
				push @a, [$node, $_, $propval];
			}
		}
	}
	return @a;
}

sub sendevent {
	my ($this, $node, $event, $value, $timestamp) = @_;
	my $type = $node->{Type}{FieldTypes}{$event};
	my $ftype = "VRML::Field::$type";
	my $aname = "__tmp_arg_$event";
	my ($rs, $rval);


	print "VRML::JS::sendevent $node $event $value $timestamp ($type)\n"
		if $VRML::verbose::js;
	## this is a problem for SFNode!!!
	$this->setProperty($event, $value, $aname);

	if (!runScript($this->{JSContext}, $this->{JSGlobal},
				   "$event($aname,$timestamp)", $rs, $rval)) {
		cleanupDie("runScript failed in VRML::JS::sendevent");
	}
	return $this->gatherSentEvents();

	### executes ???

#	unless ($type =~ /$ECMAScriptNative|^SFNode$/) {
#		&{"js$type"."Set"}($this->{JSContext}, $this->{JSGlobal}, "__evin", $value);
#		if (!runScript($this->{JSContext}, $this->{JSGlobal},
#					   "$event(__evin,$timestamp)", $rs, $rval)) {
#			cleanupDie("runScript failed in VRML::JS::sendevent");
#		}
#	} else {
#		print "\t$event $timestamp\n"."$event(".$ftype->as_string($value, 1).",$timestamp)\n"
#				if $VRML::verbose::js;
#		if (!runScript($this->{JSContext}, $this->{JSGlobal},
#					   "$event(".$ftype->as_string($value, 1).",$timestamp)", $rs, $rval)) {
#			cleanupDie("runScript failed in VRML::JS::sendevent");
#		}
#	}
#	$this->gatherSentEvents();
}


sub setProperty { # Assigns a value to a property.
	my ($this, $field, $value, $prop) = @_;
	my $typ = $this->{Node}{Type};
	my ($ftype, $rs, $i, $rval, $styp);
	if ($field =~ s/^____//) { # recurse hack
		$ftype = $field;
	} else {
		$ftype = $typ->{FieldTypes}{$field};
	}
	$vftype = "VRML::Field::$ftype";

	print "VRML::JS::setProperty args: field $field, value $value, property $prop\n"
		if $VRML::verbose::js;
	## problem with MF types - what to do from new ???
	if ($ftype =~ /^MF/) {
		$styp = $ftype;
		$styp =~ s/^MF/SF/;
		for ($i = 0; $i < $#{$value}; $i++) {
			print "\tsetProperty(\"____$styp\", [ ", @{$value->[$i]}, " ], \"____tmp\")\n"
				if $VRML::verbose::js;
			$this->setProperty("____$styp", $value->[$i], "____tmp");
			if (!runScript($this->{JSContext}, $this->{JSGlobal},
						   "$prop"."[$i]=____tmp", $rs, $rval)) {
				cleanupDie("runScript failed in VRML::JS::setProperty");
			}
		}
		if (!runScript($this->{JSContext}, $this->{JSGlobal},
					   "$prop.__touched_flag=0", $rs, $rval)) {
			cleanupDie("runScript failed in VRML::JS::setProperty");
		}
	} elsif ($ftype =~ /$ECMAScriptNative/) {
		if (!runScript($this->{JSContext}, $this->{JSGlobal},
					   "$prop=".$vftype->as_string($value, 1), $rs, $rval)) {
			cleanupDie("runScript failed in VRML::JS::setProperty");
		}
		if (!runScript($this->{JSContext}, $this->{JSGlobal},
					   "_${prop}__touched=0", $rs, $rval)) {
			cleanupDie("runScript failed in VRML::JS::setProperty");
		}
	} elsif ($ftype ne "SFNode") {
		print "\tjs$ftype"."Set: $prop, [ ", @{$value}, " ]\n"
			if $VRML::verbose::js;
		&{"js$ftype"."Set"}($this->{JSContext}, $this->{JSGlobal}, $prop, $value);
		if (!runScript($this->{JSContext}, $this->{JSGlobal},
					   "$prop.__touched()", $rs, $rval)) {
			cleanupDie("runScript failed in VRML::JS::setProperty");
		}
	} else {
		print "VRML::JS::setProperty: type $ftype. Do nothing for now.\n";
	}
}

sub getProperty {
	my ($this, $type, $prop) = @_;
	my ($rstr, $rval, $l);

	print "VRML::JS::getProperty: type $type, property $prop\n"
		if $VRML::verbose::js;


	if ($type =~ /^SFNode$/) {
		if (!runScript($this->{JSContext}, $this->{JSGlobal},
					   "$prop.__handle", $rstr, $rval)) {
			cleanupDie("runScript failed in VRML::JS::getProperty");
		}
		return VRML::Handles::get($rstr);
	} elsif ($type =~ /$ECMAScriptNative/) {
		if (!runScript($this->{JSContext}, $this->{JSGlobal},
					   "_${_}_touched=0; $prop", $rstr, $rval)) {
			cleanupDie("runScript failed in VRML::JS::getProperty");
		}
		return $rval;
	} elsif ($type =~ /^MFNode$/) {
		if (!runScript($this->{JSContext}, $this->{JSGlobal},
					   "$prop.length", $rstr, $l)) {
			cleanupDie("runScript failed in VRML::JS::getProperty for \"$prop.length\"");
		}
		print "\trunScript returned length $l for MFNode\n" if $VRML::verbose::js;
		my $fn = $prop;
		my @res = map {
			if (!runScript($this->{JSContext}, $this->{JSGlobal},
						   "$fn"."[$_].__handle", $rstr, $rval)) {
				cleanupDie("runScript failed in VRML::JS::getProperty");
			}
			## needed in case (for whatever reason) there is no SFNode at
			## a given index
			if ($rstr !~ /^undef/) {
				VRML::Handles::get($rstr);
			} else {
				VRML::Handles::get("NULL");
			}
		} (0..$l-1);
		return \@res;
	} elsif ($type =~ /^MFString$/) {
		if (!runScript($this->{JSContext}, $this->{JSGlobal}, "$prop.length", $rstr, $l)) {
			cleanupDie("runScript failed in VRML::JS::getProperty");
		}
		print "\trunScript returned length $l for MFString\n" if $VRML::verbose::js;
		my $fn = $prop;
		my @res = map {
			if (!runScript($this->{JSContext}, $this->{JSGlobal}, "$fn"."[$_]", $rstr, $rval)) {
				cleanupDie("runScript failed in VRML::JS::getProperty");
			}
			$rstr;
		} (0..$l-1);
		return \@res;
	} elsif ($type =~ /^MF/) {
		if (!runScript($this->{JSContext}, $this->{JSGlobal}, "$prop.length", $rstr, $l)) {
			cleanupDie("runScript failed in VRML::JS::getProperty");
		}
		print "\trunScript returned length $l for $type\n" if $VRML::verbose::js;
		my $fn = $prop;
		my $st = $type;
		$st =~ s/MF/SF/;
		my @res = map {
			if (!runScript($this->{JSContext}, $this->{JSGlobal}, "$fn"."[$_]", $rstr, $rval)) {
				cleanupDie("runScript failed in VRML::JS::getProperty");
			}
			(pos $rstr) = 0;
			"VRML::Field::$st"-> parse(undef, $rstr);
		} (0..$l-1);
		print "\tarray \@res: ( " if $VRML::verbose::js;
		for (@res) {
			if ("ARRAY" eq ref $_) {
				##print "(@$_)\n"
				print "(", join(', ', @{$_}), ") "
					if $VRML::verbose::js;
			}
		}
		print ")\n" if $VRML::verbose::js;
		my $r = \@res;
		print "\treference to \@ref: $r\n" if $VRML::verbose::js;
		return $r;
	} else {
		if (!runScript($this->{JSContext}, $this->{JSGlobal}, "$prop", $rstr, $rval)) {
			cleanupDie("runScript failed in VRML::JS::getProperty");
		}
		(pos $rstr) = 0;
		return "VRML::Field::$type"->parse(undef, $rstr);
	}
}

sub jspSFNodeSetProperty {
	my ($this, $prop) = @_;
	my ($handle, $node, $val, $vt, $actualField);
	my $scene = $this->{Browser}{Scene};
	my @av;

	if (!runScript($this->{JSContext}, $this->{JSGlobal},
				   "__node.__handle", $handle, $rval)) {
		cleanupDie("runScript failed in VRML::JS::jspSFNodeSetProperty");
	}

	$node = VRML::Handles::get($handle);
	if (defined $node->{IsProto}) {
		$node = $node->real_node();
	}

	if ($prop =~ /^set_($VRML::Error::Word+)/) {
		$actualField = $1;
	} elsif ($prop =~ /($VRML::Error::Word+)_changed$/) {
		$actualField = $1;
	} else {
		$actualField = $prop;
	}

	$vt = $node->{Type}{FieldTypes}{$actualField};
	if (!defined $vt) {
		cleanupDie("Invalid property $prop");
	}
	$val = $this->getProperty($vt, $prop);

	print "VRML::JS::jspSFNodeSetProperty: setting $actualField, $val",
		(ref $val eq "ARRAY" ?
		 " [ ".join(", ", map(VRML::NodeIntern::dump_name($_), @{$val}))." ] " :
		 " "),
			 "for $prop of $handle\n"
		if $VRML::verbose::js;

	$node->{RFields}{$actualField} = $val;

	if ($actualField eq "removeChildren") {
		if ($this->{Browser}->checkChildPresent($node, $val)) {
			@av = $this->{Browser}->removeChild($node, $val);
			##$this->{Browser}->api__sendEvent($node, "children", \@av);
			$this->{Browser}->api__sendEvent($node, $actualField, \@av);
		}
	} elsif ($actualField eq "addChildren") {
		if (!($this->{Browser}->checkChildPresent($node, $val))) {
			if (ref $val eq "ARRAY") {
				push @av, @{$val};
			} else {
				push @av, $val;
			}
			$this->{Browser}->api__sendEvent($node, $actualField, \@av);
		}
	}
}

sub jspSFNodeAssign {
	my ($this, $id) = @_;
	my ($vt, $val);
	my $scene = $this->{Browser}{Scene};
	my $root = $scene->{RootNode};
	my $field = "addChildren";
	my @av;

	print "VRML::JS::jspSFNodeAssign: id $id\n" if $VRML::verbose::js;

	$vt = $this->{Node}{Type}{FieldTypes}{$id};
	if (!defined $vt) {
		cleanupDie("Invalid id $id");
	}
	$val = $this->getProperty($vt, $id);
	$this->{Node}{RFields}{$id} = $val;

	if (!($this->{Browser}->checkChildPresent($root, $val))) {
		push @av, $val;
		$this->{Browser}->api__sendEvent($root, $field, \@av);
	}
}

sub jspSFNodeConstr {
	my ($this, $str) = @_;
	my $scene = $this->{Browser}{Scene};
	my $handle = $this->{Browser}->createVrmlFromString($str);

	$node = VRML::Handles::get($handle);
	if (defined $node->{IsProto}) {
		$node = $node->real_node();
	}
	print "VRML::JS::jspSFNodeConstr: handle $handle, string \"$str\"\n"
		if $VRML::verbose::js;

	if (!runScript($this->{JSContext}, $this->{JSGlobal},
				   "__node.__handle=\"$handle\"", $rs, $rval)) {
		cleanupDie("runScript failed in VRML::JS::jspSFNodeConstr");
	}
}


sub jspBrowserGetName {
	my ($this) = @_;
	my ($rval, $rs);

	print "VRML::JS::jspBrowserGetName\n"
		if $VRML::verbose::js;
	my $n = $this->{Browser}->getName();
	if (!runScript($this->{JSContext}, $this->{JSGlobal},
				   "Browser.__bret=\"$n\"", $rs, $rval)) {
		cleanupDie("runScript failed in VRML::JS::jspBrowserGetName");
	}
}

sub jspBrowserGetVersion {
	my ($this) = @_;
	my ($rval, $rs);

	print "VRML::JS::jspBrowserGetVersion ($this) !\n"
		if $VRML::verbose::js;
	my $n = $this->{Browser}->getVersion();

	if (!runScript($this->{JSContext}, $this->{JSGlobal},
				   "Browser.__bret=\"$n\"", $rs, $rval)) {
		cleanupDie("runScript failed in VRML::JS::jspBrowserGetVersion");
	}
}

## VRML::Browser::setDescription doesn't return anything...FIX!!!
sub jspBrowserSetDescription {
	my ($this, $desc) = @_;
	my ($rval, $rs);
	print "VRML::JS::jspBrowserSetDescription\n"
		if $VRML::verbose::js;
	my $n = $this->{Browser}->setDescription($desc);
	if (!runScript($this->{JSContext}, $this->{JSGlobal},
				   "Browser.__bret=\"$n\"", $rs, $rval)) {
		cleanupDie("runScript failed in VRML::JS::jspBrowserSetDescription");
	}
}

## VRML::Browser::addRoute doesn't return anything...FIX!!!
sub jspBrowserAddRoute {
	my ($this, $str) = @_;
	my ($rval, $rs);

	print "VRML::JS::jspBrowserAddRoute: route \"$str\"\n"
			if $VRML::verbose::js;
	if ($str =~ /^([^ ]+) ([^ ]+) ([^ ]+) ([^ ]+)$/) {
		my ($fn, $ff, $tn, $tf) = ($1, $2, $3, $4);

		my $n = $this->{Browser}->addRoute($fn, $ff, $tn, $tf, $this->{Browser}{Scene});

		if (!runScript($this->{JSContext}, $this->{JSGlobal},
					   "Browser.__bret=$n", $rs, $rval)) {
			cleanupDie("runScript failed in VRML::JS::jspBrowserAddRoute");
		}
	} else {
		cleanupDie("Invalid route for addRoute.");
	}
}

## VRML::Browser::deleteRoute doesn't return anything...FIX!!!
sub jspBrowserDeleteRoute {
	my ($this, $str) = @_;
	my ($rval, $rs);

	print "VRML::JS::jspBrowserDeleteRoute: route \"$str\"\n"
			if $VRML::verbose::js;
	if ($str =~ /^([^ ]+) ([^ ]+) ([^ ]+) ([^ ]+)$/) {
		my ($fn, $ff, $tn, $tf) = ($1, $2, $3, $4);

		my $n = $this->{Browser}->deleteRoute($fn, $ff, $tn, $tf, $this->{Browser}{Scene});

		if (!runScript($this->{JSContext}, $this->{JSGlobal}, "Browser.__bret=$n", $rs, $rval)) {
			cleanupDie("runScript failed in VRML::JS::jspBrowserDeleteRoute");
		}
	} else {
		cleanupDie("Invalid route for deleteRoute.");
	}
}

sub jspBrowserGetCurrentFrameRate {
	my ($this) = @_;
	my ($rval, $rs);

	print "VRML::JS::jspBrowserGetCurrentFrameRate\n"
		if $VRML::verbose::js;
	my $n = $this->{Browser}->getCurrentFrameRate();

	if (!runScript($this->{JSContext}, $this->{JSGlobal}, "Browser.__bret=$n", $rs, $rval)) {
		cleanupDie("runScript failed in VRML::JS::jspBrowserGetCurrentFrameRate");
	}
}

sub jspBrowserGetWorldURL {
	my ($this) = @_;
	my ($rval, $rs);

	print "VRML::JS::jspBrowserGetWorldURL\n"
		if $VRML::verbose::js;
	my $n = $this->{Browser}->getWorldURL();

	if (!runScript($this->{JSContext}, $this->{JSGlobal}, "Browser.__bret=$n", $rs, $rval)) {
		cleanupDie("runScript failed in VRML::JS::jspBrowserGetWorldURL");
	}
}

sub jspBrowserCreateVrmlFromString {
	my ($this, $str) = @_;
	my ($rval, $rs);
	my @handles;
	print "VRML::JS::jspBrowserCreateVrmlFromString: \"$str\"\n"
		if $VRML::verbose::js;

	my $h = $this->{Browser}->createVrmlFromString($str);
	push @handles, split(" ", $h);

	my @nodes = map(VRML::Handles::get($_), @handles);
	my $constr = $this->constrString(MFNode, \@nodes);
	my $script = "Browser.__bret=$constr";

	if (!runScript($this->{JSContext}, $this->{JSGlobal},
				   $script, $rs, $rval)) {
		cleanupDie("runScript failed in VRML::JS::jspBrowserCreateVrmlFromString");
	}
	## Debug:
	##$this->{Browser}{Scene}->dump(0);
}

sub jspBrowserCreateVrmlFromURL {
	my ($this, $u, $handle, $event) = @_;
	my ($h, $rval, $rstr);
	my @handles;
	my @rootNodes;
	my @av;

	print "VRML::JS::jspBrowserCreateVrmlFromURL: $u, $handle, $event\n"
		if $VRML::verbose::js;
	my $scene = $this->{Browser}{Scene};
	my $root = $scene->{RootNode};
	my $urls = VRML::Field::MFString->parse($scene, $u);

	for (@{$urls}) {
		$h = $this->{Browser}->createVrmlFromURL($_);
		push @handles, split(" ", $h);
	}
	my @nodes = map(VRML::Handles::get($_), @handles);

	my $constr = $this->constrString(MFNode, \@nodes);
	my $script = "Browser.__bret=$constr";

	if (!runScript($this->{JSContext}, $this->{JSGlobal},
				   $script, $rs, $rval)) {
		cleanupDie("runScript failed in VRML::JS::jspBrowserCreateVrmlFromURL");
	}
	my $node = VRML::Handles::get($handle);

	for(@nodes) {
		if (defined $_->{Scene}{RootNode}) {
			push @rootNodes, $_->{Scene}{RootNode};
		}
	}

	print "VRML::JS::jspBrowserCreateVrmlFromURL: handles [ ",
		join(", ", @handles), " ], root nodes [ ",
			join(", ",
				 map(VRML::NodeIntern::dump_name($_)." $_", @rootNodes)),
					 " ]\n"
						 if $VRML::verbose::js;

	if ($event eq "removeChildren") {
		for (@nodes) {
			if ($this->{Browser}->checkChildPresent($root, $_)) {
				push @av, $this->{Browser}->removeChild($root, $_);
			}
		}
		$this->{Browser}->api__sendEvent($root, $event, \@av);
	} elsif ($event !~ /^[ar].{2,5}?Children$/) {
		for (@nodes) {
			if (!($this->{Browser}->checkChildPresent($root, $_))) {
				push @av, $_;
			}
		}
		$this->{Browser}->api__sendEvent($root, "addChildren", \@av);
	}
	$this->{Browser}->api__sendEvent($node, $event, \@rootNodes);

	## Debug:
	##$this->{Browser}{Scene}->dump(0);
}
