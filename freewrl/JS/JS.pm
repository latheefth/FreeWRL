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
				 sendevent,
			 };

bootstrap VRML::JS $VERSION;


## Debug:
#$VRML::verbose::js=1;

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

	my $fieldstring;

	print "VRML::JS::initScriptFields: fkind $fkind, type $type, field $field\n" if $VRML::verbose::js;

	my $value = $node->{RFields}{$field};
	my $asciival = $ftype->as_string($value, 1);

VRML::VRMLFunc::initScriptFields($this->{ScriptNum},$fkind, $type, $field,$asciival);


	if ($fkind eq "eventIn") {
#print "JS - handling eventIn\n";
#		if ($type !~ /$ECMAScriptNative/) {
#			$constr = $this->constrString($type, 0);
#			if (!VRML::VRMLFunc::addGlobalAssignProperty($this->{ScriptNum},
#								   "$eventInArg"."$field", $constr)) {
#				$this->cleanupDie("VRML::VRMLFunc::addGlobalAssignProperty failed in initScriptFields");
#			}
#		}
	} elsif ($fkind eq "eventOut") {
		if ($type =~ /$ECMAScriptNative/) {
#			if (!VRML::VRMLFunc::addGlobalECMANativeProperty($this->{ScriptNum},
#											 $field)) {
#				$this->cleanupDie("VRML::VRMLFunc::addGlobalECMANativeProperty failed in initScriptFields");
#			}
		} else {
			if ($type eq "SFNode") {
				print "JS - handling eventOut SFNode\n";
				$value = $node->{RFields}{$field};
				print "\tJS field property $field, value ",
					VRML::Debug::toString($value), "\n"
							if $VRML::verbose::js;
				$constr = $this->constrString($type, $value);
				$this->initSFNodeFields($field, $value);
				if (!VRML::VRMLFunc::addGlobalAssignProperty($this->{ScriptNum},
								   $field, $constr)) {
					$this->cleanupDie("VRML::VRMLFunc::addGlobalAssignProperty failed in initScriptFields");
				}
#			} else {
#				$constr = $this->constrString($type, 0);
#
#			if (!VRML::VRMLFunc::addGlobalAssignProperty($this->{ScriptNum},
#								   $field, $constr)) {
#				$this->cleanupDie("VRML::VRMLFunc::addGlobalAssignProperty failed in initScriptFields");
#			}
			}
		}
	} elsif ($fkind eq "field") {
		$value = $node->{RFields}{$field};

		print "\tJS field property $field, value ",
			VRML::Debug::toString($value), "\n"
					if $VRML::verbose::js;

		if ($type =~ /$ECMAScriptNative/) {
#print "field, 1, now done entirely in C \n";
#			if (!VRML::VRMLFunc::addGlobalECMANativeProperty($this->{ScriptNum},
#											 $field)) {
#				$this->cleanupDie("VRML::VRMLFunc::addGlobalECMANativeProperty failed in initScriptFields");
#			}
#			if (!VRML::VRMLFunc::jsrunScript($this->{ScriptNum},
#						   "$field=".$ftype->as_string($value, 1), $rstr, $v)) {
#				$this->cleanupDie("VRML::VRMLFunc::jsrunScript failed in initScriptFields");
#			}
#			print "just ran script "."$field=".$ftype->as_string($value, 1)."\n";
		} else {
			if ($type eq "SFNode") {
				print "field, 2\n";
				$constr = $this->constrString($type, $value);
				print "constr sring is $constr\n";
				if (!VRML::VRMLFunc::addGlobalAssignProperty($this->{ScriptNum},
								   $field, $constr)) {
					$this->cleanupDie("VRML::VRMLFunc::addGlobalAssignProperty failed in initScriptFields");
				}
				$this->initSFNodeFields($field, $value);
			}
		}
	} else {
		warn("Invalid field $fkind $field for $node->{TypeName} in initScriptFields");
	}

print "\n\n";
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
		(warn, next) if	$_ eq "";
		next if $_ eq "url" or
			$_ eq "directOutput" or
			$_ eq "mustEvaluate";

		$fkind = $nt->{FieldKinds}{$_};
		$type = $nt->{FieldTypes}{$_};

		warn "no FieldTypes{$_}\n" unless defined $type and length $type != 0;
		$ftype = "VRML::Field::$type";

		# skip internal FreeWRLPTRs
		next if "VRML::Field::FreeWRLPTR" eq $ftype;


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
			warn("Invalid field '$fkind' ('$ftype') '$_' for '$ntn' in initSFNodeFields");
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
        			if (defined $v->{IsProto}) {
                			#print "getSceneNode - this is a PROTO\n";
                			$v = $v->{ProtoExp}{Nodes}[0]->real_node();
        			}               

			#print "constr string, checking for $v\n";

			if ((ref $v eq "VRML::USE") ||
			    ( ref $v eq "VRML::DEF")) {
				$v = $v->real_node();
				print "constr string, v now is $v\n";
			}

			#print "constrString, handle for $v is $h\n";

			# is this made yet? if so, return the backnode
			if (!defined $v->{BackNode}{CNode}) {
				my $be = VRML::Browser::getBE();
                        	# make a backend
                        	$v->make_backend($be,$be);
			}
			$cn = $v->{BackNode}{CNode};

			# save the association so we can look up handle from CNode if need be.
			VRML::Handles::CNodeLinkreserve("NODE$cn",$v);

			$c .= "'".VRML::Field::SFNode->as_string($v)."','".$cn."'";
		} elsif ($ft =~ /^MFNode/) {
			my $cn;		# CNode backend structure

			$l = scalar(@{$v});
			for ($i = 0; $i < $l; $i++) {
				$node = $v->[$i];

        			if (defined $node->{IsProto}) {
                			#print "getSceneNode - this is a PROTO\n";
                			$node = $node->{ProtoExp}{Nodes}[0]->real_node();
        			}               

				#print "constr string, checking for $node\n";

				if ((ref $node eq "VRML::USE") ||
				    ( ref $node eq "VRML::DEF")) {
					$node = $node->real_node();
					#print "constr string, node now is $node\n";
				}

				# is this made yet? if so, return the backnode
				if (!defined $node->{BackNode}{CNode}) {
					my $be = VRML::Browser::getBE();
                        		# make a backend
                        		$node->make_backend($be,$be);
				}
				$cn = $node->{BackNode}{CNode};

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
	die($msg);
}
