# Copyright (C) 1998 Tuomas J. Lukka
#               1999 John Stewart CRC Canada
#               2002 Jochen Hoenicke
# DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
# See the GNU Library General Public License (file COPYING in the distribution)
# for conditions of use and redistribution.

# Implement VRMLPERL-JAVA communication.

package VRML::JavaCom::OHandle;
@ISA=FileHandle;

sub new {
	my($type,$handle) = @_; 
	binmode $handle;
	bless $handle,$type;
}
sub printUTF {
	my $this = shift;
	my $str = join "", @_;
	$this->print (pack("n", length($str)),$str);
}
sub println {
	print "TO JAVA:\t\t---",@_[1..$#_],"---\n" if $VRML::verbose::java >= 2;
	printUTF @_;
}
sub toJava {
	my ($o, $type, $value) = @_;
	print "TO JAVA ($type):\t---",$value,"---\n"
		if($VRML::verbose::java >= 2);
	if ($type =~ /MF(.*)/) {
		my $subtype = "SF$1";
		my @result = ();
		$o->print(pack("N", scalar(@{$value})));
		for (@{$value}) {
			$o->toJava($subtype, $_);
		}
	} elsif ($type =~ /SFNode/) {
		$o->printUTF(VRML::Handles::reserve($value->real_node(1)));
	} elsif ($type =~ /SFString/) {
		$o->printUTF($value);
	} elsif ($type =~ /SFImage/) {
		die "SFImage to java not implemented";
	} elsif ($type =~ /SF(Float|Time)/) {
		$o->printUTF($value);
	} elsif ($type =~ /SFVec2f/) {
		$o->printUTF($value->[0]);
		$o->printUTF($value->[1]);
	} elsif ($type =~ /SF(Color|Vec3f)/) {
		$o->printUTF($value->[0]);
		$o->printUTF($value->[1]);
		$o->printUTF($value->[2]);
	} elsif ($type =~ /SFRotation/) {
		$o->printUTF($value->[0]);
		$o->printUTF($value->[1]);
		$o->printUTF($value->[2]);
		$o->printUTF($value->[3]);
	} elsif ($type =~ /SFInt32/) {
		$o->print(pack("N", $value));
	} elsif ($type =~ /SFBool/) {
		$o->print(pack("c", $value));
	} else {
		die "Illegal Field $type";
	}
}

package VRML::JavaCom::IHandle;
@ISA=FileHandle;

sub new {
	my($type,$handle) = @_; 
	binmode $handle;
	bless $handle,$type;
}
sub readfully {
	my $h = $_[0];
	my $len = $_[1];
	my $val = "";
	my $offset = read $h, $val, $len;
	while ($offset < $len) {
		$offset += read $h, $val, $len-$offset, $offset;
	}
	return $val;
}

sub getUTF2 {
	my $len = unpack("n", $_[0]->readfully(2));
	return $_[0]->readfully($len);
}

sub getUTF {
	my $value = $_[0]->getUTF2();
	print "FM JAVA:\t\t---",$value,"---\n" 
		if $VRML::verbose::java >= 2;
	return $value;
}

sub fromJava2 {
	my ($i, $type) = @_;
	if ($type =~ /MF(.*)/) {
		my $subtype = "SF$1";
		my $values = unpack("N", $_[0]->readfully(4));
		my @result = ();
		while ($values-- > 0) {
			push @result, $i->fromJava($subtype);
		}
		return \@result;
	} elsif ($type =~ /SFNode/) {
		return VRML::Handles::get($_[0]->getUTF2);
	} elsif ($type =~ /SFString/) {
		return $_[0]->getUTF2;
	} elsif ($type =~ /SFImage/) {
		die "SFImage from java not implemented";
	} elsif ($type =~ /SF(Float|Time)/) {
		return $_[0]->getUTF2();
	} elsif ($type =~ /SFVec2f/) {
		return [ $_[0]->getUTF2(), $_[0]->getUTF2() ];
	} elsif ($type =~ /SF(Color|Vec3f)/) {
		return [ $_[0]->getUTF2(), $_[0]->getUTF2(), $_[0]->getUTF2() ];
	} elsif ($type =~ /SFRotation/) {
		return [ $_[0]->getUTF2(), $_[0]->getUTF2(), $_[0]->getUTF2(), $_[0]->getUTF2() ];
	} elsif ($type =~ /SFInt32/) {
		return unpack("N", $_[0]->readfully(4));
	} elsif ($type =~ /SFBool/) {
		return unpack("c", $_[0]->readfully(1));
	} else {
		die "Illegal Field $type";
	}
}

sub fromJava {
	my ($i, $type) = @_;
	$value = $i->fromJava2($type);
	print "FM JAVA ($type):\t---",$value,"---",ref $value,"\n" 
		if $VRML::verbose::java >= 2;
	return $value;
}


package VRML::JavaCom;
use FileHandle;
use IPC::Open2;
use Fcntl;

use MIME::Base64;

my $eid;


sub new {
	my($type, $browser) = @_;
	bless { "B" => $browser }, $type;
}

sub connect {
	my ($this) = @_;

	print "VRMLJava.pm - connect called\n";
	my $libdir;
	for (@INC) {
	    print "$_\n";
	    if (-f "$_/VRML/vrml.jar" && -f "$_/VRML/java.policy") {
		$libdir = "$_/VRML";
		last;
	    }
	}
	die "Can't find vrml.jar and java.policy" if !$libdir;
	my $cmdline ="java -Dfreewrl.lib.dir=$libdir "
	    . "-Djava.security.policy=$libdir/java.policy "
		. "-classpath $libdir/vrml.jar vrml.FWJavaScript";
	print "VRMLJava.pm: Running $cmdline\n";

#	unless ($pid = fork()) {
#	 	exec "$cmdline"
#	}
#	print "VRMLJava.pm - pid is $pid\n";

#	($this->{I} = FileHandle->new)->open("<.javapipej");
#	print "VRMLJava.pm - input file handle is ",$this->{I},"\n";
#	$this->{I}->setvbuf("",_IONBF,0);
#	($this->{O} = FileHandle->new)->open(">.javapipep");
#	print "VRMLJava.pm -  output file handle is ",$this->{O},"\n";


	# I'm still not sure if communicating via stdin/stdout is the
	# right thing.  It makes it impossible for Java to write something
	# to stdout.
	# Alternatives are named pipes or a socket.
	my $i = FileHandle->new;
	my $o = FileHandle->new;
	open2($i, $o, $cmdline);

	$this->{O} = $o = VRML::JavaCom::OHandle->new($o);
	$this->{I} = $i = VRML::JavaCom::IHandle->new($i);

	$o->println( "TJL XXX PERL-JAVA 0.10" );
	$o->flush();
	if($i->getUTF ne "TJL XXX JAVA-PERL 0.10") {
		die("Invalid response from java scripter: '$str'");
	}
}

sub initialize {
	my($this,$scene,$node) = @_;
	$eid++;
	print "INITIALIZE $node\n" if $VRML::verbose::java;
	$this->{O}->println("INITIALIZE");
	$this->{O}->toJava("SFNode", $node);
	$this->{O}->println($eid);
	$this->{O}->flush();
	$this->receive($eid);
    }

$URLSTART = "[A-Za-z]+://";

sub newscript {
	my($this, $purl, $url, $node) = @_;

	print ("VRMLJava.pm: url $url, purl $purl node $node\n");
	## FIXME:  The caller of newscript should probably handle this!
	if ($url !~ /^$URLSTART/) {
	    # url is not a complete url, make it relative to purl
		if ($url =~ /^\//) {
			if ($purl =~ /^($URLSTART[^\/]+)/) {
				# url is server relative, purl contains server
				$url = "$1$url";
			} else {
				# purl is not a url, so make url a file url.
				$url = "file:/$url";
			}
		} elsif ($purl =~ /^($URLSTART.*)\/[^\/]*$/
				 || $purl =~ /^($URLSTART[^\/]+)$/) {
			# url is relative to purl and purl is complete.
			$url = "$1/$url";
		} else {
			if ($purl =~ /^(.*\/)[^\/]+$/) {
				# purl contains preceding directories append them
				$url = "$1$url";
			}
			if ($url !~ /^\//) {
				# url is not yet absolute, append the current path
				chomp($_ = `pwd`);
				$url = $_."/$url";
			}
			# Now make it a file url.
			$url = "file:$url";
		}
	}
	print ("VRMLJava.pm: resulting url $url\n");

	if(!$this->{O}) {$this->connect}
	print "VRMLJava.pm - connect passed\n";
	my $o = $this->{O};

	$o->println("NEWSCRIPT");
	$o->toJava("SFNode", $node);
	$o->println($url);
	$o->flush();
}

sub sendevent {
	my($this,$node,$event,$value,$timestamp) = @_;
	my $o = $this->{O};
	$eid++;
	print "SENDEVENT $node.$event = $value...\n" if $VRML::verbose::java == 1;
	$o->println("SENDEVENT");
	$o->toJava("SFNode", $node);
	$o->println($eid);
	$o->println($event);
	$o->println($node->{Type}{FieldTypes}{$event});
	$o->toJava($node->{Type}{FieldTypes}{$event}, $value);
	$o->println($timestamp);
	$o->flush();
	$this->receive($eid);
}

sub sendeventsproc {
	my($this,$node) = @_;
	my $o = $this->{O};
	$eid++;
	$o->println("EVENTSPROCESSED");
	$o->toJava("SFNode", $node);
	$o->println("$eid");
	$o->flush();
	$this->receive($eid);
}

sub receive {
	my($this,$id) = @_;
	my @a;
	my $i = $this->{I};
	my $o = $this->{O};
	while(1) {
		print "WAITING FOR JAVA EVENT...\n" if $VRML::verbose::java;
		my $cmd = $i->getUTF; 
		die("EOF on java filehandle") 
			if !defined $cmd;
		chomp $cmd;
		print "JAVA EVENT '$cmd'\n" if $VRML::verbose::java == 1;
		if($cmd eq "FINISHED") {
			my $ri = $i->getUTF;
			if($ri ne $id) {
				die("Invalid request id from java scripter: '$ri' should be '$id'\n");
			}
			#return;
			return @a;
		} elsif($cmd eq "GETFIELD") {
			my $node = $i->fromJava("SFNode");
			my $field = $i->getUTF;
			my $kind = $i->getUTF;
			my $t = $node->{Type};
			print "Name: ".$node->dump_name().".$field\n"
			    if $VRML::verbose::java;

			if (exists $t->{FieldKinds}{$field}
			    && $t->{FieldKinds}{$field} eq $kind) {
				$o->println($t->{FieldTypes}{$field});
			} else {
				$o->println("ILLEGAL");
			}
			$o->flush();
		} elsif($cmd eq "READFIELD") {
			my $node = $i->fromJava("SFNode");
			my $field = $i->getUTF;
			my $t = $node->{Type};
			my $ft = $t->{FieldTypes}{$field};
			my $value = $node->{Fields}{$field};

			print "$node->dump_name().$field is "
			    . (ref $value eq "ARRAY"
			       ? join ",", @{$value} : $value) . "\n"
				   if $VRML::verbose::java;
			
			$o->toJava($ft, $node->{Fields}{$field});
			$o->flush();
		} elsif($cmd eq "SENDEVENT") {
			my $node = $i->fromJava("SFNode");
			my $field = $i->getUTF;
			$value = $i->fromJava($node->{Type}{FieldTypes}{$field});
			print "$node.$field := ".
				(ref $value eq "ARRAY" ? join ",",@{$value} : "$value")."\n"
				if $VRML::verbose::java;
			if ($node->{Type}{FieldKinds}{$field} eq "eventOut") {
				push @a, [$node, $field, $value];
			} else {
				$node->{EventModel}->send_event_to($node, $field, $value);
			}
		} elsif($cmd eq "GETTYPE") {
			my $node = $i->fromJava("SFNode");
			$o->println($node->{NodeType}{Name});
			$o->flush();
		} elsif($cmd eq "CREATEVRML") {
			my($this) = @_; 
			my $rs = $i->fromJava("SFString");
			print "createVRML($rs)\n" if $VRML::verbose::java == 1;
			my $scene = $this->{B}->createVrmlFromString($rs);
			print "Result: $scene\n" if $VRML::verbose::java == 1;
			## AARRGH createVrml returns node handles instead of nodes!!!
			my @nodes = ();
			for (split " ", $scene) {
				push @nodes, VRML::Handles::get($_);
			}
			$o->toJava("MFNode", \@nodes);
		} else {
			die("Invalid Java event '$cmd'");
		}
	}
}

1;
