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
#$VRML::verbose::js=1;


if ($VRML::verbose::js) {
	VRML::VRMLFunc::setJSVerbose(1);
}

our $ECMAScriptNative = qr{^SF(?:Bool|Float|Time|Int32|String)$};

## See VRML97, section 4.12 (Scripting)
my $DefaultScriptMethods = "function initialize() {}; function shutdown() {}; function eventsProcessed() {}; TRUE=true; FALSE=false;";

my $eventInArg = "__tmp_arg_";
my $tmpFieldKind = "____tmp_f_";

## see jsUtils.h
my $browserSFNodeHandle = "__node.__handle";
my $browserRetval = "Browser.__ret";


sub new {
	my ($type, $number, $text, $node, $browser) = @_;
	my $this = bless {
					  Node => $node,
					  Browser => $browser,
					  ScriptNum => undef,
					 }, $type;
	print "VRML::JS::new: num $number $this $this->{Node}{TypeName} ",
		VRML::NodeIntern::dump_name($node),
				"\n$text\n" if $VRML::verbose::js;
	${$this->{Browser}}{JSCleanup} = \&{cleanup};

	$this->{ScriptNum} = $number;  # we go by this script number

	VRML::VRMLFunc::jsinit($number,$this);

	if (!VRML::VRMLFunc::jsrunScript($number, $text, $rs, $rval)) {
		cleanupDie("VRML::VRMLFunc::jsrunScript failed in VRML::JS");
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

	print "VRML::JS::initScriptFields: ", $nt->{FieldKinds}{$field},
		" $type $field\n" if $VRML::verbose::js;

	if ($fkind eq "eventIn") {
		if ($type !~ /$ECMAScriptNative/) {
			$constr = $this->constrString($type, 0);
			if (!VRML::VRMLFunc::addGlobalAssignProperty($this->{ScriptNum},
								   "$eventInArg"."$field", $constr)) {
				$this->cleanupDie("VRML::VRMLFunc::addGlobalAssignProperty failed in initScriptFields");
			}
		}
	} elsif ($fkind eq "eventOut") {
		if ($type =~ /$ECMAScriptNative/) {
			if (!VRML::VRMLFunc::addGlobalECMANativeProperty($this->{ScriptNum},
											 $field)) {
				$this->cleanupDie("VRML::VRMLFunc::addGlobalECMANativeProperty failed in initScriptFields");
			}
		} else {
			if ($type eq "SFNode") {
				$value = $node->{RFields}{$field};
				print "\tJS field property $field, value ",
					VRML::Debug::toString($value), "\n"
							if $VRML::verbose::js;
				$constr = $this->constrString($type, $value);
				$this->initSFNodeFields($field, $value);
			} else {
				$constr = $this->constrString($type, 0);
			}
			if (!VRML::VRMLFunc::addGlobalAssignProperty($this->{ScriptNum}, 
								   $field, $constr)) {
				$this->cleanupDie("VRML::VRMLFunc::addGlobalAssignProperty failed in initScriptFields");
			}
		}
	} elsif ($fkind eq "field") {
		$value = $node->{RFields}{$field};
		print "\tJS field property $field, value ",
			VRML::Debug::toString($value), "\n"
					if $VRML::verbose::js;

		if ($type =~ /$ECMAScriptNative/) {
			if (!VRML::VRMLFunc::addGlobalECMANativeProperty($this->{ScriptNum},
											 $field)) {
				$this->cleanupDie("VRML::VRMLFunc::addGlobalECMANativeProperty failed in initScriptFields");
			}
			if (!VRML::VRMLFunc::jsrunScript($this->{ScriptNum}, 
						   "$field=".$ftype->as_string($value, 1), $rstr, $v)) {
				$this->cleanupDie("VRML::VRMLFunc::jsrunScript failed in initScriptFields");
			}
		} else {
			$constr = $this->constrString($type, $value);
			if (!VRML::VRMLFunc::addGlobalAssignProperty($this->{ScriptNum},
								   $field, $constr)) {
				$this->cleanupDie("VRML::VRMLFunc::addGlobalAssignProperty failed in initScriptFields");
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
	my ($constr, $fkind, $ftype, $type, $value);
	my @fields = keys %{$node->{Fields}};

	print "VRML::JS::initSFNodeFields: $ntn $nodeName fields ",
		VRML::Debug::toString(\@fields), "\n"
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
				if (!VRML::VRMLFunc::addSFNodeProperty($this->{ScriptNum}, 
									   $nodeName, $_, $constr)) {
					$this->cleanupDie("VRML::VRMLFunc::addSFNodeProperty failed in initSFNodeFields");
				}
			}
		} elsif ($fkind eq "eventOut") {
			$value = $node->{RFields}{$_};
			print "\tJS field property $_, value ",
				VRML::Debug::toString($value), "\n"
						if $VRML::verbose::js;
			if ($type !~ /$ECMAScriptNative/) {
				if ($type eq "SFNode") {
					$constr = $this->constrString($type, $value);
				} else {
					$constr = $this->constrString($type, 0);
				}
				if (!VRML::VRMLFunc::addSFNodeProperty($this->{ScriptNum}, 
									   $nodeName, $_, $constr)) {
					$this->cleanupDie("VRML::VRMLFunc::addSFNodeProperty failed in initSFNodeFields");
				}
			}
		} elsif ($fkind =~ /^(?:exposed)??[Ff]ield$/) {
			$value = $node->{RFields}{$_};
			print "\tJS field property $_, value ",
				VRML::Debug::toString($value), "\n"
						if $VRML::verbose::js;
			if ($type !~ /$ECMAScriptNative/) {
				$constr = $this->constrString($type, $value);

				if (!VRML::VRMLFunc::addSFNodeProperty($this->{ScriptNum}, 
									   $nodeName, $_, $constr)) {
					$this->cleanupDie("VRML::VRMLFunc::addSFNodeProperty failed in initSFNodeFields");
				}
			}
		} else {
			warn("Invalid field $fkind $_ for $ntn in initSFNodeFields");
			return;
		}
	}
}

sub constrString {
	my ($this, $ft, $v) = @_;
	my ($i, $l, $sft, $c);

	if ($v) {
		# check for CNode constructor type
		if ($ft eq "CNode") {
			$c = "new MFNode(";
		} else {
			$c = "new $ft(";
		}

		if ($ft =~ /^SFNode/) {
			if (VRML::Handles::check($v)) {
				$h = VRML::Handles::get($v);
			} else {
				$h = VRML::Handles::reserve($v);
			}
			$c .= "'".VRML::Field::SFNode->as_string($v)."','".$h."'";
		} elsif ($ft =~ /^MFString/) {
			$l = scalar(@{$v});
			for ($i = 0; $i < $l; $i++) {
				$c .= "'".$v->[$i]."'";
				$c .= "," unless ($i == ($l - 1));
			}
		} elsif ($ft =~ /^MFNode/) {
			$l = scalar(@{$v});
			for ($i = 0; $i < $l; $i++) {
				if (VRML::Handles::check($v->[$i])) {
					$h = VRML::Handles::get($v->[$i]);
				} else {
					$h = VRML::Handles::reserve($v->[$i]);
				}
				$c .= "new SFNode('".VRML::Field::SFNode->as_string($v->[$i])."','".$h."')";
				$c .= "," unless ($i == ($l - 1));
			}
		} elsif ($ft =~ /^CNode/) {
			my $cn;		# CNode backend structure

			$l = scalar(@{$v});
			for ($i = 0; $i < $l; $i++) {
				if (VRML::Handles::check($v->[$i])) {
					$h = VRML::Handles::get($v->[$i]);
				} else {
					$h = VRML::Handles::reserve($v->[$i]);
				}

				if (!defined ($cn=$h->{BackNode}{CNode})) {
					cleanupDie("ConstrString: no backend node for node $h");
				}

				$c .= "new SFNode('".$cn."','".VRML::Field::SFNode->as_string($h)."')";
				$c .= "," unless ($i == ($l - 1));
			}
		} elsif ($ft =~ /^MF(?:Color|Rotation|Vec2f|Vec3f)$/) {
			$sft = $ft;
			$sft =~ s/^MF/SF/;
			$l = scalar(@{$v});
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
	VRML::VRMLFunc::jscleanup($this->{ScriptNum});
}

sub sendeventsproc {
	my($this) = @_;
	my ($rs, $rval);
	print "VRML::JS::sendeventproc: $this\n" if $VRML::verbose::js;
	if (!VRML::VRMLFunc::jsrunScript($this->{ScriptNum},
				   "eventsProcessed()", $rs, $rval)) {
		cleanupDie("VRML::VRMLFunc::jsrunScript failed in VRML::JS::sendeventproc");
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
		VRML::Debug::toString(\@_), "\n"
				if $VRML::verbose::js;
	for (@k) {
		next if $_ eq "url";
		my $type = $t->{FieldTypes}{$_};
		my $ftyp = $type;
		if ($t->{FieldKinds}{$_} eq "eventOut") {
			print "\teventOut $_\n" if $VRML::verbose::js;

			if ($type =~ /^MF/) {
				if (!VRML::VRMLFunc::jsrunScript($this->{ScriptNum},
							   "$_.__touched_flag", $rstr, $rnum)) {
					cleanupDie("VRML::VRMLFunc::jsrunScript failed in VRML::JS::gatherSentEvents");
				}
				if (!VRML::VRMLFunc::jsrunScript($this->{ScriptNum},
							   "$_.__touched_flag=0", $rstr, $runused)) {
					cleanupDie("VRML::VRMLFunc::jsrunScript failed in VRML::JS::gatherSentEvents");
				}
			} elsif ($ftyp =~ /$ECMAScriptNative/) {
				if (!VRML::VRMLFunc::jsrunScript($this->{ScriptNum},
							   "_${_}_touched", $rstr, $rnum)) {
					cleanupDie("VRML::VRMLFunc::jsrunScript failed in VRML::JS::gatherSentEvents");
				}
				if (!VRML::VRMLFunc::jsrunScript($this->{ScriptNum},
							   "_${_}_touched=0", $rstr, $runused)) {
					cleanupDie("VRML::VRMLFunc::jsrunScript failed in VRML::JS::gatherSentEvents");
				}
			} else {
				if (!VRML::VRMLFunc::jsrunScript($this->{ScriptNum},
							   "$_.__touched()", $rstr, $rnum)) {
					cleanupDie("VRML::VRMLFunc::jsrunScript failed in VRML::JS::gatherSentEvents");
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


#JASsub getProperty {
#JAS	my ($this, $type, $prop) = @_;
#JAS	my ($rstr, $rval, $l, $i);
#JAS	my @res;
#JAS
#JAS	print "VRML::JS::getProperty: ", VRML::Debug::toString(\@_), "\n"
#JAS		;#JAS if $VRML::verbose::js;
#JAS
#JAS	if ($type eq "SFNode") {
#JAS		if (!VRML::VRMLFunc::jsrunScript($this->{ScriptNum},
#JAS					   "$prop.__handle", $rstr, $rval)) {
#JAS			cleanupDie("VRML::VRMLFunc::jsrunScript failed in VRML::JS::getProperty");
#JAS		}
#JAS		return VRML::Handles::get($rstr);
#JAS	} elsif ($type =~ /$ECMAScriptNative/) {
#JAS		if (!VRML::VRMLFunc::jsrunScript($this->{ScriptNum},
#JAS					   "_".$prop."_touched=0; $prop", $rstr, $rval)) {
#JAS			cleanupDie("VRML::VRMLFunc::jsrunScript failed in VRML::JS::getProperty");
#JAS		}
#JAS		return $rval;
#JAS	} elsif ($type eq "MFNode") {
#JAS		if (!VRML::VRMLFunc::jsrunScript($this->{ScriptNum},
#JAS					   "$prop.length", $rstr, $l)) {
#JAS			cleanupDie("VRML::VRMLFunc::jsrunScript failed in VRML::JS::getProperty for \"$prop.length\"");
#JAS		}
#JAS		print "\tVRML::VRMLFunc::jsrunScript returned length $l for MFNode\n"
#JAS			if $VRML::verbose::js;
#JAS		for ($i = 0; $i < $l; $i++) {
#JAS			if (!VRML::VRMLFunc::jsrunScript($this->{ScriptNum}, 
#JAS						   "$prop"."[$i].__handle", $rstr, $rval)) {
#JAS				cleanupDie("VRML::VRMLFunc::jsrunScript failed in VRML::JS::getProperty");
#JAS			}
#JAS			if ($rstr !~ /^undef/) {
#JAS				push @res, VRML::Handles::get($rstr);
#JAS			}
#JAS		}
#JAS		print "\treturn ", VRML::Debug::toString(\@res), "\n"
#JAS			if $VRML::verbose::js;
#JAS		return \@res;
#JAS	} else {
#JAS		if (!VRML::VRMLFunc::jsrunScript($this->{ScriptNum}, "$prop", $rstr, $rval)) {
#JAS			cleanupDie("VRML::VRMLFunc::jsrunScript failed in VRML::JS::getProperty");
#JAS		}
#JAS		print "\tVRML::VRMLFunc::jsrunScript returned \"$rstr\" for $type.\n"
#JAS			if $VRML::verbose::js;
#JAS		(pos $rstr) = 0;
#JAS		return "VRML::Field::$type"->parse($this->{Browser}{Scene}, $rstr);
#JAS	}
#JAS}


sub addRemoveChildren {
	my ($this, $node, $field, $c) = @_;

	if ($field !~ /^(?:add|remove)Children$/) {
		warn("Invalid field $field for VRML::JS::addChildren");
		return;
	}

	print "VRML::JS::addRemoveChildren: ", VRML::Debug::toString(\@_), "\n"
		if $VRML::verbose::js;

	if (ref $c eq "ARRAY") {
		return if (!@{$c});
		$this->{Browser}->api__sendEvent($node, $field, $c);
	} else {
		return if (!$c);
		$this->{Browser}->api__sendEvent($node, $field, [$c]);
	}
}

sub jspSFNodeGetProperty {
	my ($this, $prop, $handle) = @_;

	$node = VRML::Handles::get($handle);
	my $type = $node->{Type}{FieldTypes}{$prop};
	my $ftype = "VRML::Field::$type";
	my ($rs, $rval);

	my $value = $node->{RFields}{$prop};
	if (!VRML::VRMLFunc::jsrunScript($this->{ScriptNum}, 
				   "$handle"."_$prop=".$ftype->as_string($value, 1),
				   $rs, $rval)) {
		cleanupDie("VRML::VRMLFunc::jsrunScript failed in VRML::JS::jspSFNodeGetProperty");
	}
}

sub jspSFNodeSetProperty {
	my ($this, $prop, $handle) = @_;
	my ($node, $val, $vt, $actualField, $rval);
	my $scene = $this->{Browser}{Scene};

	$node = VRML::Handles::get($handle);

	## see VRML97, section 4.7 (field, eventIn, and eventOut semantics)
	if ($prop =~ /^set_($VRML::Error::Word+)/ and
		$node->{Type}{FieldKinds}{$prop} !~ /in$/i) {
		$actualField = $1;
	} elsif ($prop =~ /($VRML::Error::Word+)_changed$/ and
			 $node->{Type}{FieldKinds}{$prop} !~ /out$/i) {
		$actualField = $1;
	} else {
		$actualField = $prop;
	}

	$vt = $node->{Type}{FieldTypes}{$actualField};

	if (!defined $vt) {
		cleanupDie("Invalid property $prop");
	}
	$val = $this->getProperty($vt, "$handle"."_$prop");

	print "VRML::JS::jspSFNodeSetProperty: setting $actualField, ",
		VRML::Debug::toString($val), " for $prop of $handle\n"
				if $VRML::verbose::js;

	if ($actualField =~ /^(?:add|remove)Children$/) {
		$this->addRemoveChildren($node, $actualField, $val);
	} else {
		$node->{RFields}{$actualField} = $val;
	}
}

sub jspSFNodeAssign {
	my ($this, $id) = @_;
	my ($vt, $val);
	my $scene = $this->{Browser}{Scene};
	my $root = $scene->{RootNode};
	my $field = "addChildren";
	my @av;

	print "VRML::JS::jspSFNodeAssign: id $id\n"
		if $VRML::verbose::js;

	$vt = $this->{Node}{Type}{FieldTypes}{$id};
	if (!defined $vt) {
		cleanupDie("Invalid id $id");
	}
	$val = $this->getProperty($vt, $id);
	$this->{Node}{RFields}{$id} = $val;

	$this->addRemoveChildren($root, $field, $val);
}

sub jspSFNodeConstr {
	my ($this, $str) = @_;
	my $scene = $this->{Browser}{Scene};
	my $handle = $this->{Browser}->createVrmlFromString($str);

	$node = VRML::Handles::get($handle);

	print "VRML::JS::jspSFNodeConstr: handle $handle, string \"$str\"\n"
		if $VRML::verbose::js;

	if (!VRML::VRMLFunc::jsrunScript($this->{ScriptNum}, 
				   "$browserSFNodeHandle"."=\"$handle\"", $rs, $rval)) {
		cleanupDie("VRML::VRMLFunc::jsrunScript failed in VRML::JS::jspSFNodeConstr");
	}
}


sub jspBrowserGetName {
	my ($this) = @_;
	my ($rval, $rs);

	print "VRML::JS::jspBrowserGetName\n"
		if $VRML::verbose::js;
	my $n = $this->{Browser}->getName();
	if (!VRML::VRMLFunc::jsrunScript($this->{ScriptNum}, 
				   "$browserRetval"."=\"$n\"", $rs, $rval)) {
		cleanupDie("VRML::VRMLFunc::jsrunScript failed in VRML::JS::jspBrowserGetName");
	}
}

sub jspBrowserGetVersion {
	my ($this) = @_;
	my ($rval, $rs);

	print "VRML::JS::jspBrowserGetVersion\n"
		if $VRML::verbose::js;
	my $n = $this->{Browser}->getVersion();

	if (!VRML::VRMLFunc::jsrunScript($this->{ScriptNum}, 
				   "$browserRetval"."=\"$n\"", $rs, $rval)) {
		cleanupDie("VRML::VRMLFunc::jsrunScript failed in VRML::JS::jspBrowserGetVersion");
	}
}

sub jspBrowserGetCurrentSpeed {
	my ($this) = @_;
	my ($rval, $rs);

	print "VRML::JS::jspBrowserGetCurrentSpeed\n"
		if $VRML::verbose::js;
	my $n = $this->{Browser}->getCurrentSpeed();

	if (!VRML::VRMLFunc::jsrunScript($this->{ScriptNum}, 
				   "$browserRetval"."=$n", $rs, $rval)) {
		cleanupDie("VRML::VRMLFunc::jsrunScript failed in VRML::JS::jspBrowserGetCurrentSpeed");
	}
}

sub jspBrowserGetCurrentFrameRate {
	my ($this) = @_;
	my ($rval, $rs);

	print "VRML::JS::jspBrowserGetCurrentFrameRate\n"
		if $VRML::verbose::js;
	my $n = $this->{Browser}->getCurrentFrameRate();

	if (!VRML::VRMLFunc::jsrunScript($this->{ScriptNum},
				   "$browserRetval"."=$n", $rs, $rval)) {
		cleanupDie("VRML::VRMLFunc::jsrunScript failed in VRML::JS::jspBrowserGetCurrentFrameRate");
	}
}

sub jspBrowserGetWorldURL {
	my ($this) = @_;
	my ($rval, $rs);

	print "VRML::JS::jspBrowserGetWorldURL\n"
		if $VRML::verbose::js;
	my $n = $this->{Browser}->getWorldURL();

	if (!VRML::VRMLFunc::jsrunScript($this->{ScriptNum}, 
				   "$browserRetval"."=\"$n\"", $rs, $rval)) {
		cleanupDie("VRML::VRMLFunc::jsrunScript failed in VRML::JS::jspBrowserGetWorldURL");
	}
}

sub jspBrowserReplaceWorld {
	my ($this, $handles) = @_;

	print "VRML::JS::jspBrowserReplaceWorld $handles\n"
		if $VRML::verbose::js;

	$this->{Browser}->replaceWorld($handles);
}

sub jspBrowserLoadURL {
	my ($this, $url, $parameter) = @_;

	print "VRML::JS::jspBrowserLoadURL $url, $parameter\n"
		if $VRML::verbose::js;

	## VRML::Browser::loadURL not implemented yet
	$this->{Browser}->loadURL($url, $handles);
}

sub jspBrowserSetDescription {
	my ($this, $desc) = @_;
	$desc = join(" ", split(" ", $desc));

	## see VRML97, section 4.12.10.8
	if (!$this->{Node}{Type}{Defaults}{"mustEvaluate"}) {
		warn "VRML::JS::jspBrowserSetDescription: mustEvaluate for ",
			$this->{Node}{TypeName},
				" (", VRML::NodeIntern::dump_name($this->{Node}),
					") must be set to TRUE to call setDescription($desc)";
		return;
	}

	print "VRML::JS::jspBrowserSetDescription: $desc\n"
		if $VRML::verbose::js;

	$this->{Browser}->setDescription($desc);
}

# create vrml, send it back to javascript interpreter. works with CRoutes JAS 
sub jspBrowserCreateVrmlFromString {
	my ($this, $str) = @_;
	my ($rval, $rs);
	my @handles;
	print "VRML::JS::jspBrowserCreateVrmlFromString: \"$str\"\n"
		if $VRML::verbose::js;

	my $h = $this->{Browser}->createVrmlFromString($str);
	print "cvs returns $h\n";
	push @handles, split(" ", $h);

	#JAS my @nodes = map(VRML::Handles::get($_), @handles);
	#JAS my $constr = $this->constrString(CNode, \@nodes);
	my $constr = $this->constrString(CNode, \@handles);
	print "jspBrowserCreateVrmlFromString, constr is:\n$constr\n";

	my $script = "$browserRetval"."=$constr";

	print "passed in context ",$this->{JSContext}," scriptnum ",$this->{ScriptNum},"\n";
	if (!VRML::VRMLFunc::jsrunScript($this->{ScriptNum}, $script, $rs, $rval)) {
		cleanupDie("runScript failed in VRML::JS::jspBrowserCreateVrmlFromString");
	}
}

sub jspBrowserCreateVrmlFromURL {
	my ($this, $url, $handle, $event) = @_;
	my $h;
	my @createdHandles;
	my @rootNodes;

	print "VRML::JS::jspBrowserCreateVrmlFromURL: ",
		VRML::Debug::toString(\@_), "\n"
				if $VRML::verbose::js;

	my $scene = $this->{Browser}{Scene};
	my $root = $scene->{RootNode};
	my $urls = VRML::Field::MFString->parse($scene, $url);
	my $node = VRML::Handles::get($handle);

	for (@{$urls}) {
		$h = $this->{Browser}->createVrmlFromURL($_);
		push @createdHandles, split(" ", $h);
	}
	my @createdNodes = map(VRML::Handles::get($_), @createdHandles);

	for(@createdNodes) {
		if (defined $_->{Scene}{RootNode}) {
			push @rootNodes, $_->{Scene}{RootNode};
		}
	}

	print "VRML::JS::jspBrowserCreateVrmlFromURL: createdNodes",
		VRML::Debug::toString(\@createdNodes),
				", root nodes ",
					VRML::Debug::toString(\@rootNodes),
							"\n" if $VRML::verbose::js;

	$this->{Browser}->api__sendEvent($node, $event, \@rootNodes);
}

## update routing???
sub jspBrowserAddRoute {
	my ($this, $str) = @_;

	print "VRML::JS::jspBrowserAddRoute: route \"$str\"\n"
			if $VRML::verbose::js;
	if ($str =~ /^([^ ]+) ([^ ]+) ([^ ]+) ([^ ]+)$/) {
		my ($fn, $ff, $tn, $tf) = ($1, $2, $3, $4);

		$this->{Browser}->addRoute($fn, $ff, $tn, $tf);
	} else {
		cleanupDie("Invalid route for addRoute.");
	}
}

sub jspBrowserDeleteRoute {
	my ($this, $str) = @_;

	print "VRML::JS::jspBrowserDeleteRoute: route \"$str\"\n"
			if $VRML::verbose::js;
	if ($str =~ /^([^ ]+) ([^ ]+) ([^ ]+) ([^ ]+)$/) {
		my ($fn, $ff, $tn, $tf) = ($1, $2, $3, $4);

		$this->{Browser}->deleteRoute($fn, $ff, $tn, $tf);
	} else {
		cleanupDie("Invalid route for deleteRoute.");
	}
}
