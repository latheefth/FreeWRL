# Copyright (C) 1998 Tuomas J. Lukka
#               1999 John Stewart CRC Canada
#               2002 Jochen Hoenicke
# DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
# See the GNU Library General Public License (file COPYING in the distribution)
# for conditions of use and redistribution.

# Implement VRMLPERL-JAVA communication.

package VRML::JavaClass;

#debug 
$VRML::verbose::javaclass = 1;
 
sub new {
	my ($this, $node,$scriptInvocationNumber, $url) = @_;

	print "Java Class execution is not yet back in freewrl; it is being worked on\n";
	print "  -- expect it back in March 2004 - sorry\n\n";
	return;

	print "\n 	start of javaClass $this, $node\n";

	$this->{ScriptNum} = $scriptInvocationNumber;  # we go by this script number
	
	my $nt = $node->{Type};
	my @k = keys %{$nt->{Defaults}};

	print "\tFields: \n" ;#JAS if $VRML::verbose::javaclass;
	for (@k) {
		next if $_ eq "url" or
		$_ eq "directOutput" or
		$_ eq "mustEvaluate";

		print "must initClassFields for $_, node $node\n";
		$this->initClassFields($node, $_);
	}
	print "done initClassFields loop...\n\n";
	return VRML::VRMLFunc::do_newJavaClass($scriptInvocationNumber,$_,$node);
}

sub initClassFields {
	my ($this,$node, $field) = @_;
	my $nt = $node->{Type};
	my $type = $nt->{FieldTypes}{$field};
	my $ftype = "VRML::Field::$type";
	my $fkind = $nt->{FieldKinds}{$field};
	my ($rstr, $v);

	print "VRML::JavaClass::initClassFields: fkind $fkind, type $type, fiel $field\n"  if $VRML::verbose::javaclass;

	my $il = VRML::VRMLFunc::getClen("VRML::Field::$node->{Type}{FieldTypes}{$field}"->clength($field));
	
	my $il = VRML::VRMLFunc::malloc_this($il);

	print "my il is $il\n";

	if ($fkind eq "eventIn") {
		$constr = $this->constrString($type, 0);
	} elsif ($fkind eq "eventOut") {
		if ($type eq "SFNode") {
			$value = $node->{RFields}{$field};
			print "\tJS field property $field, value ",
				VRML::Debug::toString($value), "\n"
						if $VRML::verbose::javaclass;
			$constr = $this->constrString($type, $value);
			$this->initSFNodeFields($field, $value);
		} else {
			$constr = $this->constrString($type, 0);
		}

	} elsif ($fkind eq "field") {
		$value = $node->{RFields}{$field};
		print "\tJS field property $field, value ",
			VRML::Debug::toString($value), "\n"
					if $VRML::verbose::javaclass;

		$constr = $this->constrString($type, $value);
		if ($type eq "SFNode") {
			#JAS$this->initSFNodeFields($field, $value);
		}
	} else {
		warn("Invalid field $fkind $field for $node->{TypeName} in initClassFields");
	}
	print "initClassFields, constr $constr\n";
}


sub initSFNodeFields {
	my ($this, $nodeName, $node) = @_;
	my $nt = $node->{Type};
	my $ntn = $node->{TypeName};
	my ($constr, $fkind, $ftype, $type, $value);
	my @fields = keys %{$node->{Fields}};

	print "VRML::JavaClass::initSFNodeFields: $ntn $nodeName fields ",
		VRML::Debug::toString(\@fields), "\n"
			 	if $VRML::verbose::javaclass;

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
					 	if $VRML::verbose::javaclass;
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
					 	if $VRML::verbose::javaclass;
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

	print "constrString, this $this, ft $ft, v $v\n";
	return "constrStringRetVal";
#JAS	if ($v) {
#JAS		$c = "new $ft(";
#JAS
#JAS		if ($ft =~ /^SFNode/) {
#JAS			if (VRML::Handles::check($v)) {
#JAS				$h = VRML::Handles::get($v);
#JAS			} else {
#JAS				$h = VRML::Handles::reserve($v);
#JAS			}
#JAS
#JAS			# print "constrString, handle for $v is $h\n";
#JAS
#JAS			# is this made yet? if so, return the backnode
#JAS			if (!defined $h->{BackNode}{CNode}) {
#JAS				$cn = $h;
#JAS			} else {
#JAS				$cn = $h->{BackNode}{CNode};
#JAS			}
#JAS
#JAS			# save the association so we can look up handle from CNode if need be.
#JAS        		$CNTONAME{$cn} = $h;
#JAS
#JAS			$c .= "'".VRML::Field::SFNode->as_string($v)."','".$cn."'";
#JAS		} elsif ($ft =~ /^MFNode/) {
#JAS			my $cn;		# CNode backend structure
#JAS
#JAS			$l = scalar(@{$v});
#JAS			for ($i = 0; $i < $l; $i++) {
#JAS				$node = $v->[$i];
#JAS				# print "constr string, checking for $node\n";
#JAS
#JAS				if (ref $node eq "VRML::USE") {
#JAS					$node = $node->real_node();
#JAS				}
#JAS
#JAS				if (VRML::Handles::check($node)) {
#JAS					$h = VRML::Handles::get($node);
#JAS				} else {
#JAS					$h = VRML::Handles::reserve($node);
#JAS				}
#JAS
#JAS				# is this made yet? if so, return the backnode
#JAS				if (!defined $node->{BackNode}{CNode}) {
#JAS					$cn = $h;
#JAS				} else {
#JAS					$cn = $node->{BackNode}{CNode};
#JAS				}
#JAS				# print "constr string, node now is $node, handle $h cn $cn\n";
#JAS
#JAS				# save the association so we can look up handle from CNode if need be.
#JAS        			$CNTONAME{$cn} = $h;
#JAS
#JAS				$c .= "new SFNode('".VRML::Field::SFNode->as_string($node)."','".$cn."')";
#JAS				$c .= "," unless ($i == ($l - 1));
#JAS			}
#JAS		} elsif ($ft =~ /^MFString/) {
#JAS			$l = scalar(@{$v});
#JAS			for ($i = 0; $i < $l; $i++) {
#JAS				$c .= "'".$v->[$i]."'";
#JAS				$c .= "," unless ($i == ($l - 1));
#JAS			}
#JAS		} elsif ($ft =~ /^MF(?:Color|Rotation|Vec2f|Vec3f)$/) {
#JAS			$sft = $ft;
#JAS			$sft =~ s/^MF/SF/;
#JAS			$l = scalar(@{$v});
#JAS			for ($i = 0; $i < $l; $i++) {
#JAS				if (ref($v->[$i]) eq "ARRAY") {
#JAS					$h = join(",", @{$v->[$i]});
#JAS					$c .= "new $sft(".$h.")";
#JAS					$c .= "," unless ($i == ($l - 1));
#JAS				}
#JAS			}
#JAS		} elsif (ref($v) eq "ARRAY") {
#JAS			$c .= join(",", @{$v});
#JAS		} else {
#JAS			$c .= $v;
#JAS		}
#JAS		$c .= ")";
#JAS	} else {
#JAS		$c = "new $ft()";
#JAS	}
#JAS	print "VRML::JavaClass::constrString: $c\n" if $VRML::verbose::javaclass;
#JAS	return $c;
}
