# Copyright (C) 1998 Tuomas J. Lukka 1999 John Stewart CRC Canada
# DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
# See the GNU Library General Public License (file COPYING in the distribution)
# for conditions of use and redistribution.

# Implement VRMLPERL-JAVA communication.

package VRML::JavaCom::OHandle;
sub new {my($type,$handle) = @_; bless {Handle => $handle},$type;}
sub print {my $this = shift;
	if($VRML::verbose::java >= 2) {
		print "TO JAVA:\n---\n";
		print @_; print "---\n";
	}
	$this->{Handle}->print(@_);}
sub flush {$_[0]{Handle}->flush}

package VRML::JavaCom::IHandle;
sub new {my($type,$handle) = @_; bless {Handle => $handle},$type;}
sub getline {
	my $l = $_[0]{Handle}->getline;
	print "FROM JAVA:\n---\n$l---\n" if $VRML::verbose::java >= 2;
	return $l;
}

package VRML::JavaCom;
use FileHandle;
use IPC::Open2;
use Fcntl;

use MIME::Base64;

my $eid;

sub toJava {
	my ($type, $value) = @_;
	if ($type =~ /MF(.*)/) {
		my $subtype = "SF$1";
		my @result = ();
		for (@{$value}) {
			push @result, toJava($subtype, $_);
		}
		return join ",", @result;
	} elsif ($type =~ /SFNode/) {
		return VRML::Handles::reserve($value->real_node(1));
	} elsif ($type =~ /SFString/) {
		return encode_base64($value);
	} elsif ($type =~ /SFImage/) {
		die "SFImage to java not implemented";
	} elsif ($type =~ /SF(Color|Rotation|Vec2f|Vec3f)/) {
		return join " ", @{$value};
	} else {
		return "".(0+$value);
	}
}

sub fromJava {
	my ($type, $str) = @_;
	if ($type =~ /MF(.*)/) {
		my $subtype = "SF$1";
		my @result = ();
		for (split ",", $str) {
			push @result, fromJava($subtype, $_);
		}
		return \@result;
	} elsif ($type =~ /SFNode/) {
		return VRML::Handles::get($_[1]);
	} elsif ($type =~ /SFString/) {
		return decode_base64($str);
	} elsif ($type =~ /SFImage/) {
		die "SFImage from java not implemented";
	} elsif ($type =~ /SF(Color|Rotation|Vec2f|Vec3f)/) {
		return [ split " ", $str ];
	} else {
		return $str;
	}
}


sub new {
	my($type) = @_;
	bless {
	}, $type;
}

sub connect {
	my($this) = @_;
# XXX java VM name

	print "VRMLJava.pm - connect called\n";

	# my @cmd = split ' ','java FWJavaScript';
	# $pid = system 1, @cmd;
#	unless ($pid = fork()) {
#	 	exec 'java vrml.FWJavaScript';
#	}
#	print "VRMLJava.pm - pid is $pid\n";

#	($this->{I} = FileHandle->new)->open("<.javapipej");
#	print "VRMLJava.pm - input file handle is ",$this->{I},"\n";
#	$this->{I}->setvbuf("",_IONBF,0);
#	($this->{O} = FileHandle->new)->open(">.javapipep");
#	print "VRMLJava.pm -  output file handle is ",$this->{O},"\n";


	$this->{I} = FileHandle->new;
	$this->{O} = FileHandle->new;
	open2($this->{I}, $this->{O}, 'java vrml.FWJavaScript');
	$this->{I}->setvbuf("",_IONBF,0);

	# $pid = open3("<, $this->{O}, 'java_g -v FWJavaScript ');
	$this->{O} = VRML::JavaCom::OHandle->new($this->{O});
	$this->{I} = VRML::JavaCom::IHandle->new($this->{I});
	$this->{O}->print( "TJL XXX PERL-JAVA 0.02\n" );
	$this->{O}->flush();
	$str = $this->{I}->getline; chomp $str;
	if("TJL XXX JAVA-PERL 0.02" ne $str) {
		die("Invalid response from java scripter: '$str'");
	}
	$this->{O}->print("\n"); # Directory - currently ""
}

sub initialize {
	my($this,$scene,$node) = @_;
	$eid++;
	print "INITIALIZE $node\n" if $VRML::verbose::java;
	$this->{O}->print("INITIALIZE\n$node\n$eid\n");
	$this->{O}->flush();
	$this->receive($eid);
    }

sub newscript {
	my($this, $purl, $url, $node) = @_;
	#JAS undef $1;
	$purl =~ /^(.*\/)[^\/]+$/;
	$url = $1.$url; # XXXX!!
	print ("VRMLJava.pm: url $url, purl $purl node $node\n");

	if(!$this->{O}) {$this->connect}
	print "VRMLJava.pm - connect passed\n";

	$this->{O}->print("NEWSCRIPT\n"
			  .toJava("SFNode", $node)."\n$url\n");
	$this->{O}->flush();
}

sub sendevent {
	my($this,$node,$event,$value,$timestamp) = @_;
	$eid++;
	print "SENDEVENT $node.$event = $value...\n" if $VRML::verbose::java;
	$this->{O}->print("SENDEVENT\n$node\n$eid\n$event\n");
	$this->{O}->print("$node->{Type}{FieldTypes}{$event}\n");
	$this->{O}->print(toJava($node->{Type}{FieldTypes}{$event}, $value)
			  ."\n");
	$this->{O}->print("$timestamp\n");
	$this->{O}->flush();
	$this->receive($eid);
}

sub sendeventsproc {
	my($this,$node) = @_;
	$eid++;
	$this->{O}->print("EVENTSPROCESSED\n$node\n$eid\n");
	$this->{O}->flush();
	$this->receive($eid);
}

sub receive {
	my($this,$id) = @_;
	my @a;
	$i = $this->{I};
	while(1) {
		print "WAITING FOR JAVA EVENT...\n" if $VRML::verbose::java;
		my $cmd = $i->getline; 
		die("EOF on java filehandle") 
			if !defined $cmd;
		chomp $cmd;
		print "JAVA EVENT '$cmd'\n" if $VRML::verbose::java;
		if($cmd eq "FINISHED") {
			my $ri = $i->getline; chomp $ri;
			if($ri ne $id) {
				die("Invalid request id from java scripter: '$ri' should be '$id'\n");
			}
			#return;
			return @a;
		} elsif($cmd eq "GETFIELD") {
			my $nid = $i->getline; chomp $nid;
			my $node = fromJava("SFNode", $nid);
			my $field = $i->getline; chomp $field;
			my $kind = $i->getline; chomp $kind;
			my $t = $node->{Type};
			print "Name: ".$node->dump_name().".$field\n"
			    if $VRML::verbose::java;

			if (exists $t->{FieldKinds}{$field}
			    && $t->{FieldKinds}{$field} eq $kind) {
				$this->{O}->print($t->{FieldTypes}{$field}
						  ."\n");
			} else {
				$this->{O}->print("ILLEGAL\n");
			}
			$this->{O}->flush();
		} elsif($cmd eq "READFIELD") {
			my $nid = $i->getline; chomp $nid;
			my $node = fromJava("SFNode", $nid);
			my $field = $i->getline; chomp $field;
			my $t = $node->{Type};
			my $ft = $t->{FieldTypes}{$field};

			print "$node->dump_name().$field is $node->{Fields}{$field}\n"
			    if $VRML::verbose::java;
			
			$this->{O}->print(toJava($ft, $node->{Fields}{$field})
					  ."\n");
			$this->{O}->flush();
		} elsif($cmd eq "SENDEVENT") {
			my $nid = $i->getline; chomp $nid;
			my $field = $i->getline; chomp $field;
			my $value = $i->getline; chomp $value;
			my $node = fromJava("SFNode", $nid);
			$value = fromJava($node->{Type}{FieldTypes}{$field}, $value);
			print "$node.$field := ".
				(ref $value eq "ARRAY" ? join ",",@{$value} : "$value")."\n"
				if $VRML::verbose::java;
			push @a, [$node, $field, $value];
		} elsif($cmd eq "GETBROWSER") {
			my $nid = $i->getline; chomp $nid;
			my $node = fromJava("SFNode", $nid);
			my $browser = VRML::Handles::reserve($node->{Scene}->get_browser());

			print "$node->Browser is $browser\n"
			    if $VRML::verbose::java;
			
			$this->{O}->print("$browser\n");
			$this->{O}->flush();
		} elsif($cmd eq "CREATEVRML") {
			my($this) = @_; 
			my $bid = $i->getline; chomp $bid;
			my $browser = VRML::Handles::get($bid);
			my $input = $i->getline; chomp $input;
			my $rs = fromJava("SFString", $input);
			print "createVRML($rs)\n" if $VRML::verbose::java;
			my $scene = $browser->createVrmlFromString($rs);
			print "Result: $scene\n" if $VRML::verbose::java;
			my @nodes = split " ", $scene;
			$this->{O}->print(scalar(@nodes)."\n");
			for (@nodes) {
				$this->{O}->print("$_\n");
			}
		} else {
			die("Invalid Java event '$cmd'");
		}
	}
}

1;
