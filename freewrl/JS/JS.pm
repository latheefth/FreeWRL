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
			#print "constr string, checking for $v\n";

			if ((ref $v eq "VRML::USE") ||
			    ( ref $v eq "VRML::DEF")) {
				$v = $v->real_node();
				#print "constr string, v now is $v\n";
			}

			if (VRML::Handles::check($v)) {
				$h = VRML::Handles::get($v);
			} else {
				$h = VRML::Handles::reserve($v);
			}

			#print "constrString, handle for $v is $h\n";

			# is this made yet? if so, return the backnode
			if (!defined $v->{BackNode}{CNode}) {
				$cn = $h;
			} else {
				$cn = $v->{BackNode}{CNode};
			}

			# save the association so we can look up handle from CNode if need be.
			VRML::Handles::CNodeLinkreserve("NODE$cn",$v);

			$c .= "'".VRML::Field::SFNode->as_string($v)."','".$cn."'";
		} elsif ($ft =~ /^MFNode/) {
			my $cn;		# CNode backend structure

			$l = scalar(@{$v});
			for ($i = 0; $i < $l; $i++) {
				$node = $v->[$i];
				#print "constr string, checking for $node\n";

				if ((ref $node eq "VRML::USE") ||
				    ( ref $node eq "VRML::DEF")) {
					$node = $node->real_node();
					#print "constr string, node now is $node\n";
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

				#print "reserving in MFNode, $node\n";
				VRML::Handles::CNodeLinkreserve("NODE$cn",$node);

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

	# get a hash of node values/CNode values. Will return a hash
	# with the index being the xx of NODExx, and value being CNode.
	my %retval = VRML::Browser::EAI_CreateVrmlFromString($str);
	my $key; foreach $key (keys(%{retval})) {
		# gets the CNODE values: push @handles, $retval{$key};
		push @handles, "NODE".$key;
	}

	# get a list of nodes, eg VRML::NodeIntern=HASH(xxx);
        my @nodes = map(VRML::Handles::get($_), @handles);
	
	# and, combine the two together; VRML text and node CNode.
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


sub jspSFNodeConstr {
	my ($this, $str) = @_;
	my ($rval, $rs);
	my @handles;
	my $scene = $this->{Browser}{Scene};


	print "VRML::JS::jspSFNodeConstr: \"$str\"\n"
	  	if $VRML::verbose::js;

        my $h = $this->{Browser}->createVrmlFromString($str);
	my ($handle, $stuff) = split (" ",$h);
	print "jspSFNodeConstr, first handle is $handle\n";

        my $constr = $this->constrString(SFNode, VRML::Handles::get($handle));
        my $script = "__ret"."=$constr";

	print "jspSFNodeConstr, script is $script\n";

	if (!VRML::VRMLFunc::jsrunScript($this->{ScriptNum}, $script, $rs, $rval)) {
		cleanupDie("runScript failed in VRML::JS::jspBrowserCreateVrmlFromString");
	}
}


# Property is stored in Javascript; we need the values here in Perl. Get the
# values from Javascript and return them to the Perl caller.

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
		#print "JS.pm - getProperty for MFNode, prop $prop\n";
		if (!VRML::VRMLFunc::jsGetProperty($this->{ScriptNum},
					   "$prop", $rstr)) {
			cleanupDie("runScript failed in VRML::JS::getProperty");
		}
		print "\trunScript returned prop $rstr for MFNode\n"
			if $VRML::verbose::js;
		$rstr =~ s/\[//;	# remove initial bracket
		$rstr =~ s/^\s+//;	# remove initial spaces
		$rstr =~s/\]//;		# remove trailing bracket
		$rstr =~s/\s+$//;	# remove trailing spaces

		push @res, split(" ", $rstr);	# split it up.

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

# assign a value to a node that is stored in VRML-space; this is an assign,
# NOT a route. eg: 
# DEF Fertig Transform {}
# SCRIPT xx field SFNode node USE Fertig
# .... node.addChildren(newSFNode);
# 
# this is called from CFuncs/jsVRMLClasses.c
# 
# we do this in Perl, because we don't have the field offset handy in C, as we
# would have with a ROUTE.

sub jspSFNodeSetProperty {
	my ($this, $prop, $cn) = @_;
	my ($node, $handle, $val, $vt, $actualField, $rval);
	my $scene = $this->{Browser}{Scene};

	# print "jspSFNodeSetProperty, this $this, prop $prop, cn $cn\n";

	$node = VRML::Handles::get("NODE$cn");

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
	#print "VRML::JS::jspSFNodeSetProperty, getting vt $vt, cn $cn, prop $prop\n";
	#print "VRML::JS::jspSFNodeSetProperty, node $node, handle $handle\n";
	my $gf = "__node_"."$prop";

	$val = $this->getProperty($vt, $gf, $cn);

	print "VRML::JS::jspSFNodeSetProperty: setting $actualField, ",
		VRML::Debug::toString($val), " for $prop of $cn\n"
			if $VRML::verbose::js;

	#print "comparing :$actualField: for children\n";
	if (($actualField eq "addChildren") || ($actualField eq "removeChildren")) {
		my $outoffset;

		#print "this is an add/remove children node\n";

		$outoffset=$VRML::CNodes{$node->{TypeName}}{Offs}{children};
	        foreach $mych (@{$val}) {
			#print "sendevto ",$node->{BackNode}{CNode}, 
			#	" field $actualField child $mych, BN ", $mych,"\n";
			VRML::VRMLFunc::jsManipulateChild(
				$node->{BackNode}{CNode}+$outoffset,
				$node->{BackNode}{CNode},
							$actualField, $mych);
        	}

	} else {
		$node->{RFields}{$actualField} = $val;
	}
}

# get info for a node in Perl space, save it (via a running script) for 
# JavaScript/C to get a hold of 
sub jspSFNodeGetProperty {
	my ($this, $prop, $handle) = @_;
	my $realele; my $fieldname; my $direction;

	#print "jspSFNodeGetProperty start, handle $handle, prop $prop\n";

	$node = VRML::Handles::get($handle);

	# is this possibly a CNode (aka, straight memory pointer?) if so, make
	# it into a NODExxxx, and re-look for it.
	if ($node eq $handle) {
		$node = VRML::Handles::get("NODE".$handle);
	}

	($node, $prop, $direction) =
		VRML::Browser::EAI_LocateNode ($handle, $prop, "eventIn");

	print "EAI_LocateNode returns node $node, prop $prop, direction $direction\n"; 

	my $type = $node->{Type}{FieldTypes}{$prop};
	my $ftype = "VRML::Field::$type";
	print "jspSFNodeGetProperty, handle $handle, type $type, ftype $ftype\n";
	my ($rs, $rval,$levnode, $script);

	$levnode = $node->{Fields}{$prop};
	$script = "NODE"."$handle"."_$prop";
	
	# my $key;
	
	#print "node is ",VRML::NodeIntern::dump_name($node),", $node\n";
	# foreach $key (keys(%{$node->{Fields}})) {
		#	 		print "node ",
		# 	VRML::NodeIntern::dump_name($node),
		# 	" Fields $key\n";
		# }
	if (("MFNode" eq $type) || ("SFNode" eq $type)) {
		$script = $script."=".$this->constrString($type,$levnode);
	} else {
		my $value;
		$value = $node->{Fields}{$prop};
		$script = $script."=\"".$ftype->as_string($value, 1)."\"";
	}
	print "script is:$script\n";
	if (!VRML::VRMLFunc::jsrunScript($this->{ScriptNum},
				   $script,$rs, $rval)) {
		cleanupDie("runScript failed in VRML::JS::jspSFNodeGetProperty");
	}
	print "script $script finished\n";
}

sub jspBrowserAddRoute {
	my ($this, $route) = @_;
	VRML::Browser::JSRoute($this,1,$route);
}

sub jspBrowserDeleteRoute {
	my ($this, $route) = @_;
	VRML::Browser::JSRoute($this,0,$route);
}
