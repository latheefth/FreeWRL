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

# keep an association of CNode to name - eg, if you have 1433423423, return NODE132
my %CNTONAME = ();




if ($VRML::verbose::js) {
	VRML::VRMLFunc::setJSVerbose(1);
}

our $ECMAScriptNative = qr{^SF(?:Bool|Float|Time|Int32|String)$};

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
	my ($i, $l, $sft, $c, $node);

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

			# save the association so we can look up handle from CNode if need be.
        		$CNTONAME{$cn} = $h;

			$c .= "'".VRML::Field::SFNode->as_string($v)."','".$cn."'";
		} elsif ($ft =~ /^MFNode/) {
			my $cn;		# CNode backend structure

			$l = scalar(@{$v});
			for ($i = 0; $i < $l; $i++) {
				$node = $v->[$i];
				# print "constr string, checking for $node\n";

				if (ref $node eq "VRML::USE") {
					$node = $node->real_node();
				}

				if (VRML::Handles::check($node)) {
					$h = VRML::Handles::get($node);
				} else {
					$h = VRML::Handles::reserve($node);
				}

				# is this made yet? if so, return the backnode
				if (!defined $node->{BackNode}{CNode}) {
					$cn = $h;
				} else {
					$cn = $node->{BackNode}{CNode};
				}
				# print "constr string, node now is $node, handle $h cn $cn\n";

				# save the association so we can look up handle from CNode if need be.
        			$CNTONAME{$cn} = $h;

				$c .= "new SFNode('".VRML::Field::SFNode->as_string($node)."','".$cn."')";
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

	print "VRML::JS::jspBrowserReplaceWorld $handles\n";
	#JAS 	if $VRML::verbose::js;

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
					") must be set to TRUE to call setDescription($desc) - refer to the VRML97 spec (http://www.web3d.org/Specifications/VRML97/), chapters 4.12.10.8 and 6.40";
		return;
	}

	print "VRML::JS::jspBrowserSetDescription: $desc\n" if $VRML::verbose::js;

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
        push @handles, split(" ", $h);


        my @nodes = map(VRML::Handles::get($_), @handles);
        my $constr = $this->constrString(MFNode, \@nodes);
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


sub jspSFNodeConstr {
	my ($this, $str) = @_;
	my $scene = $this->{Browser}{Scene};
	my $handle = $this->{Browser}->createVrmlFromString($str);

	$node = VRML::Handles::get($handle);

	print "VRML::JS::jspSFNodeConstr: handle $handle, string \"$str\"\n"
		if $VRML::verbose::js;

	if (!VRML::VRMLFunc::jsrunScript($this->{ScriptNum},
				   "$browserSFNodeHandle"."=\"$handle\"", $rs, $rval)) {
		cleanupDie("runScript failed in VRML::JS::jspSFNodeConstr");
	}
}


sub getProperty {
	my ($this, $type, $prop, $handle) = @_;
	my ($rstr, $rval, $l, $i);
	my @res;

	print "VRML::JS::getProperty: ", VRML::Debug::toString(\@_), "\n"
		if $VRML::verbose::js;

	if ($type eq "SFNode") {
		if (!VRML::VRMLFunc::jsGetProperty($this->{ScriptNum},
					   "$prop.__handle", $rstr)) {
			cleanupDie("runScript failed in VRML::JS::getProperty");
		}
		return VRML::Handles::get($rstr);
	} elsif ($type =~ /$ECMAScriptNative/) {
		if (!VRML::VRMLFunc::jsrunScript($this->{ScriptNum},
					   "_".$prop."_touched=0", $rstr, $l)) {
			cleanupDie("runScript failed in VRML::JS::getProperty for \"$prop.length\"");
		}
		if (!VRML::VRMLFunc::jsGetProperty($this->{ScriptNum},
					   "$prop", $rstr)) {
			cleanupDie("runScript failed in VRML::JS::getProperty");
		}

		if ($type eq "SFBool") {
			if ($rstr eq "true") {
				return 1;
			} else {
				return 0;
			}
		}
		return "VRML::Field::$type"->parse($this->{Browser}{Scene}, $rstr);
	} elsif ($type eq "MFNode") {
		if (!VRML::VRMLFunc::jsrunScript($this->{ScriptNum},
					   "$prop.length", $rstr, $l)) {
			cleanupDie("runScript failed in VRML::JS::getProperty for \"$prop.length\"");
		}
		print "\trunScript returned length $l for MFNode\n"
			if $VRML::verbose::js;
		for ($i = 0; $i < $l; $i++) {
			if (!VRML::VRMLFunc::jsGetProperty($this->{ScriptNum},
						   "$prop"."[$i].__handle", $rstr)) {
				cleanupDie("runScript failed in VRML::JS::getProperty");
			}
			if ($rstr !~ /^undef/) {
				push @res, VRML::Handles::get($rstr);
			}
		}
		print "\treturn ", VRML::Debug::toString(\@res), "\n"
			if $VRML::verbose::js;
		return \@res;
	} else {
		if (!VRML::VRMLFunc::jsGetProperty($this->{ScriptNum}, "$prop", $rstr)) {
			print "runScript failed in VRML::JS::getProperty for property $prop\n";
			return;
		}
		print "\tjsGetProperty returned \"$rstr\" for $type.\n"
			 if $VRML::verbose::js;
		(pos $rstr) = 0;
		return "VRML::Field::$type"->parse($this->{Browser}{Scene}, $rstr);
	}
}


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


sub jspSFNodeSetProperty {
	my ($this, $prop, $cn) = @_;
	my ($node, $handle, $val, $vt, $actualField, $rval);
	my $scene = $this->{Browser}{Scene};

	$handle = $CNTONAME{$cn};
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
		print "Invalid property $prop for handle $handle\n";
		return;
	}
	$val = $this->getProperty($vt, "$cn"."_$prop", $cn);

	print "VRML::JS::jspSFNodeSetProperty: setting $actualField, ",
		VRML::Debug::toString($val), " for $prop of $cn\n"
			if $VRML::verbose::js;

	if ($actualField =~ /^(?:add|remove)Children$/) {
		my $outoffset;

		$outoffset=$VRML::CNodes{$node->{TypeName}}{Offs}{children};
	        foreach $mych (@{$val}) {
        	        #print "sendevto ",$node->{BackNode}{CNode}, " field $actualField child $mych, BN ",
                	#                $mych->{BackNode}{CNode},"\n";
			VRML::VRMLFunc::jsManipulateChild($node->{BackNode}{CNode}+$outoffset,
							$actualField, $mych->{BackNode}{CNode});
        	}

	} else {
		$node->{RFields}{$actualField} = $val;
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
		cleanupDie("runScript failed in VRML::JS::jspSFNodeGetProperty");
	}
}
