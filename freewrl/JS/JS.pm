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

	if (!VRML::VRMLFunc::jsrunScript($number, "initialize()", $rs, $rval)) {
		cleanupDie("VRML::VRMLFunc::jsrunScript initialize failed in VRML::JS");
	}

	# Ignore all events we may have sent while building
	#JAS $this->gatherSentEvents(1);

	return $this;
}

sub initScriptFields {
	my ($this, $node, $field) = @_;
	my $nt = $node->{Type};
	my $type = $nt->{FieldTypes}{$field};
	my $ftype = "VRML::Field::$type";
	my $fkind = $nt->{FieldKinds}{$field};
	my ($rstr, $v);

	print "VRML::JS::initScriptFields: fkind $fkind, type $type, fiel $field\n"  if $VRML::verbose::js;

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
		$c = "new $ft(";

		if ($ft =~ /^SFNode/) {
			if (VRML::Handles::check($v)) {
				$h = VRML::Handles::get($v);
			} else {
				$h = VRML::Handles::reserve($v);
			}

			# print "constrString, handle for $v is $h\n";

			# is this made yet? if so, return the backnode
			if (!defined $h->{BackNode}{CNode}) {
				$cn = $h;
			} else {
				$cn = $h->{BackNode}{CNode};
			}

			$c .= "'".VRML::Field::SFNode->as_string($v)."','".$cn."'";
		} elsif ($ft =~ /^MFNode/) {
			my $cn;		# CNode backend structure

			$l = scalar(@{$v});
			for ($i = 0; $i < $l; $i++) {
				if (VRML::Handles::check($v->[$i])) {
					$h = VRML::Handles::get($v->[$i]);
				} else {
					$h = VRML::Handles::reserve($v->[$i]);
				}

				# is this made yet? if so, return the backnode
				if (!defined $h->{BackNode}{CNode}) {
					$cn = $h;
				} else {
					$cn = $h->{BackNode}{CNode};
				}

				$c .= "new SFNode('".VRML::Field::SFNode->as_string($v->[$i])."','".$cn."')";
				$c .= "," unless ($i == ($l - 1));
			}
		} elsif ($ft =~ /^MFString/) {
			$l = scalar(@{$v});
			for ($i = 0; $i < $l; $i++) {
				$c .= "'".$v->[$i]."'";
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

}

# this is called if the node handle was created at initialization (thus we
# do not have a backend node pointer yet)

sub getNodeCNode {
	my ($this, $id) = @_;
	my ($vt, $val);
	my $scene = $this->{Browser}{Scene};
	my $root = $scene->{RootNode};
	my @av;
	my $h;
	my $cn;
	my $c;


	print "VRML::JS::getNodeCNode: id $id\n"
	 	if $VRML::verbose::js;

	$vt = $this->{Node}{Type}{FieldTypes}{$id};
	$h = VRML::Handles::get($id);

	# is this made yet? if so, return the backnode
	if (!defined $h->{BackNode}{CNode}) {
		#print "no back node yet\n";
		$cn = $h;
	} else {
		#print "back node\n";
		$cn = $h->{BackNode}{CNode};
	}

	if (!VRML::VRMLFunc::jsrunScript($this->{ScriptNum},
		#"$browserRetval"."=$cn",
		"__ret=$cn",
		$rs, $rval)) {
		cleanupDie("VRML::VRMLFunc::jsrunScript failed in VRML::JS::getNodeCNode");
	}
}

sub jspBrowserReplaceWorld {
	my ($this, $handles) = @_;

	print "VRML::JS::jspBrowserReplaceWorld $handles\n"
	;#JAS 	if $VRML::verbose::js;

	$this->{Browser}->replaceWorld($handles);
}

sub jspBrowserLoadURL {
	my ($this, $url, $parameter) = @_;

	print "VRML::JS::jspBrowserLoadURL $url, $parameter\n"
	;#JAS 	if $VRML::verbose::js;

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
	;#JAS 	if $VRML::verbose::js;

	$this->{Browser}->setDescription($desc);
}

# create vrml, send it back to javascript interpreter. works with CRoutes JAS 
sub jspBrowserCreateVrmlFromString {
	my ($this, $str) = @_;
	my ($rval, $rs);
	my @handles;
	print "VRML::JS::jspBrowserCreateVrmlFromString: \"$str\"\n"
	 ;#JAS 	if $VRML::verbose::js;

	my $h = $this->{Browser}->createVrmlFromString($str);
	push @handles, split(" ", $h);

	my $constr = $this->constrString(MFNode, \@handles);
	my $script = "$browserRetval"."=$constr";

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
	;#JAS 			if $VRML::verbose::js;

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
#JAS
#JAS## update routing???
#JASsub jspBrowserAddRoute {
#JAS	my ($this, $str) = @_;
#JAS
#JAS	print "VRML::JS::jspBrowserAddRoute: route \"$str\"\n"
#JAS			if $VRML::verbose::js;
#JAS	if ($str =~ /^([^ ]+) ([^ ]+) ([^ ]+) ([^ ]+)$/) {
#JAS		my ($fn, $ff, $tn, $tf) = ($1, $2, $3, $4);
#JAS
#JAS		$this->{Browser}->addRoute($fn, $ff, $tn, $tf);
#JAS	} else {
#JAS		cleanupDie("Invalid route for addRoute.");
#JAS	}
#JAS}
#JAS
#JASsub jspBrowserDeleteRoute {
#JAS	my ($this, $str) = @_;
#JAS
#JAS	print "VRML::JS::jspBrowserDeleteRoute: route \"$str\"\n"
#JAS			if $VRML::verbose::js;
#JAS	if ($str =~ /^([^ ]+) ([^ ]+) ([^ ]+) ([^ ]+)$/) {
#JAS		my ($fn, $ff, $tn, $tf) = ($1, $2, $3, $4);
#JAS
#JAS		$this->{Browser}->deleteRoute($fn, $ff, $tn, $tf);
#JAS	} else {
#JAS		cleanupDie("Invalid route for deleteRoute.");
#JAS	}
#JAS}
