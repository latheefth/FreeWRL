/*******************************************************************
 Copyright (C) 2004 John Stewart, CRC Canada.
 Based on methods produced and documented in the (now obsolete)
 FreeWRL file VRMLJava.pm:
 Copyright (C) 1998 Tuomas J. Lukka
               1999 John Stewart CRC Canada
               2002 Jochen Hoenicke

 DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
 See the GNU Library General Public License (file COPYING in the distribution)
 for conditions of use and redistribution.
*********************************************************************/

/* Open and communicate with a java .class file. We use as much EAI code
as we can.
*/

#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

#include <math.h>

#ifdef AQUA 
#include <gl.h>
#include <glu.h>
#include <glext.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glx.h>
#endif

#include "headers.h"
#include "EAIheaders.h"

#define MURLLEN 2000
#define CLASSVER "JavaClass version 1.0 - www.crc.ca"

/* Function definitions for local functions */
int newClassConnection ();
void makeJavaInvocation (char *commandline, int scriptno);
void send_int (int node, int fd);

int JavaClassVerbose = 1;

int fd, lfd;	/* socket descriptors */

/* input buffer */
int bufcount; 
int bufsize;
char *buffer;
int eid = 0; /* event id */

/* a new .class file has been called - lets load it */
int newJavaClass(int scriptInvocationNumber,char * nodestr,int *node) {
	char newURL [MURLLEN];
	char *ri;


	/* register this script.... */
	CRoutes_js_new(scriptInvocationNumber, 0, 0, 0);

	/* make sure we tell the EAI socket code that this is not opened yet */
	fd = -1;
	lfd = -1;

	/* we have a url; have to make it into a real url for Java.  */
	if ((strncmp("file:",nodestr,5)!=0) &&
		(strncmp("FILE:",nodestr,5)!=0)  &&
		(strncmp("HTTP:",nodestr,5)!=0)  &&
		(strncmp("http:",nodestr,5)!=0)) {

		/* start file off with a file: */
		strcpy (newURL,"file:");

		/* now, is the world relative to cwd, or not? */
		if (BrowserURL[0] == '/') {
			strncat (&newURL[5],BrowserURL,MURLLEN-10);
		} else {
			getcwd (&newURL[5],MURLLEN-10);
			strcat (newURL,"/");
			strncat (newURL,BrowserURL,MURLLEN-100);
		}

		/* now, strip off the wrl filename, and place our name here */
		ri = rindex(newURL,'/'); 
		*ri = '\0';

		strcat (newURL,"/");
		strcat (newURL,nodestr);
	} else {
		strncpy (newURL,nodestr,MURLLEN-4);
		newURL[MURLLEN-1] = '\0';		/* always terminate */
	}
	printf ("newURL, step 3, :%s:\n",newURL);
		
	if (JavaClassVerbose) printf ("class url is now %s\n",newURL);

	if (!newClassConnection(scriptInvocationNumber)) return FALSE;

	send_string("NEWSCRIPT", lfd);
	send_string ("SFNode", lfd);
	send_int (node,lfd);
	printf ("newURL :%s:\n",newURL);
	send_string (newURL, lfd);
		
	/* run initialize method */
        eid++;
        send_string("INITIALIZE",lfd);
	send_string ("SFNode", lfd);
	send_int (node,lfd);
	send_int (eid,lfd);
        receive_string();

	/* is this the eventid, or a command? */
	printf ("recieved string is %s\n",buffer);

	return TRUE;
}

/* recieve a string from the java class */
int receive_string () {
	int sleepcount;

	sleepcount = 1;
	while (bufcount == 0) {
		if (sleepcount>=3000) {
			printf ("FreeWRL Timeout: java class on socket - class problem?\n");
			return FALSE;
		}
		usleep (100000);
		read_EAI_socket(buffer,&bufcount, &bufsize, &lfd);
		printf ("readEAIsocket loop, bufcount %d\n",bufcount);
	}
	return TRUE;
}


/* send a constant string along... */
void send_string (char *string, int fd) {
	strcpy (buffer, string);
	EAI_send_string(buffer,lfd);
}

/* convert an integer to a string, and send it */
void send_int (int node, int fd) {
	char myintbuf[100];
	sprintf (myintbuf,"%d",node);
	EAI_send_string(myintbuf,fd);
}

/* new class - open file */
int newClassConnection (int scriptno) {
	char commandline[MURLLEN];
	int sleepcount;

	/* allocate memory for input buffer */
	bufcount = 0;
	bufsize = 2 * EAIREADSIZE; // initial size
	buffer = malloc(bufsize * sizeof (char));
	if (buffer == 0) {
		printf ("can not malloc memory for input buffer in create_EAI\n");
		return FALSE;
	}

	/* make the communications socket */
	if (!conEAIorCLASS(scriptno+1, &fd, &lfd)) {
		printf ("could not open CLASS socket for script %d\n",scriptno);
		return FALSE;
	}
	if (JavaClassVerbose) printf ("socket %d lsocket %d\n",fd, lfd);

	/* make the commandline */
	makeJavaInvocation (commandline,scriptno+1);

	/* invoke the java interpreter */
	if (strlen(commandline) <= 0) return FALSE;
	system (commandline);

	/* wait for the connection to happen */
	sleepcount = 1;
	while (lfd<0) {
		if (sleepcount>=3000) {
			printf ("FreeWRL Timeout: java class on socket - class problem?\n");
			return FALSE;
		}
		usleep (100000);
		conEAIorCLASS(scriptno+1, &fd, &lfd);
		sleepcount++;
	}

	/* get the handshake version */
	if (!receive_string()) return FALSE;

	/* do we have the correct version? */
	if (strncmp(CLASSVER, buffer, strlen(CLASSVER)) != 0) {
		printf ("FreeWRL - JavaClass version prob; expected :%s: got :%s:\n",
			CLASSVER, buffer);
		return FALSE;
	}

	/* throw away buffer contents */
	bufcount = 0;

	/* whew ! done. */
	return TRUE;
}

/* make up the command line required to invoke the Java interpreter */
void makeJavaInvocation (char *commandline, int scriptno) {

	char vrmlJar[MURLLEN];
	char javaPolicy[MURLLEN];
	char *libdir;
	FILE *vJfile, *jPfile;
	char *myenv;
	int lenenv;
	char myc[100];


	if (JavaClassVerbose) 
		printf ("perlpath: %s, builddir %s\n",myPerlInstallDir, BUILDDIR);
	commandline[0] = '\0';

	/* get the CLASSPATH, if one exists */
	myenv = getenv ("CLASSPATH");
	if (myenv == NULL) {
		lenenv = 0; /* no classpath */
	} else {
		lenenv = strlen(myenv);
	}

	/* find the vrml.jar and the java.policy files */
	/* look in the perl path first */
	libdir = myPerlInstallDir; /* assume it is installed... */
	strncpy (vrmlJar,myPerlInstallDir,MURLLEN-20);
	strncpy (javaPolicy,myPerlInstallDir,MURLLEN-20);
	strcat (vrmlJar,"/vrml.jar");
	strcat (javaPolicy,"/java.policy");

	vJfile = fopen (vrmlJar,"r");
	jPfile = fopen (javaPolicy,"r");

	/* did we find the vrml.jar file? */
	if (!vJfile) {
		strncpy (vrmlJar,BUILDDIR,MURLLEN-20);
		strcat (vrmlJar, "/java/classes/vrml.jar");
		vJfile = fopen (vrmlJar,"r");
		if (!vJfile) {
			printf ("FreeWRL can not find vrml.jar\n");
			commandline[0] = '\0';
			return;
		}

		libdir = BUILDDIR;
	}
	fclose (vJfile);

	/* did we find the java.policy file? */
	if (!jPfile) {
		strncpy (javaPolicy,BUILDDIR,MURLLEN-20);
		strcat (javaPolicy, "/java/classes/java.policy");
		jPfile = fopen (javaPolicy,"r");
		if (!jPfile) {
			printf ("FreeWRL can not find java.policy\n");
			commandline[0] = '\0';
			return;
		}
	}
	fclose (jPfile);

	/* ok, we have found the jar and policy files... */
	if (JavaClassVerbose)
		printf ("found %s and %s\n",vrmlJar,javaPolicy);

	/* expect to make a line like:
		java -Dfreewrl.lib.dir=/usr/lib/perl5/site_perl/5.8.0/i586-linux-thread-multi/VRML 
		  -Djava.security.policy=/usr/lib/perl5/site_perl/5.8.0/i586-linux-thread-multi/VRML/java.policy 
		  -classpath /usr/lib/perl5/site_perl/5.8.0/i586-linux-thread-multi/VRML/vrml.jar 
		  vrml.FWJavaScript
	*/

	/* basic string checking - bounds checking of commandline length */
	if ((lenenv + strlen(vrmlJar) + strlen(javaPolicy) + strlen (myPerlInstallDir)) > (MURLLEN - 100)) {

		printf ("we have a memory problem with MURLLEN...\n");
		commandline[0] = '\0';
		return;
	}

	strcat (commandline,"java -Dfreewrl.lib.dir=");
	strcat (commandline,libdir);
	strcat (commandline, " -Djava.security.policy=");
	strcat (commandline, javaPolicy);

	/* classpath, if one exists */
	strcat (commandline, " -classpath ");
	strcat (commandline, vrmlJar); 
	if (lenenv > 0) {
		strcat (commandline,":");
		strcat (commandline, myenv);
	}

	/* and, the command to run */
	sprintf (myc, " vrml.FWJavaScript %d &",scriptno+EAIBASESOCKET);
	strcat (commandline, myc);
	
	if (JavaClassVerbose) printf ("command line %s\n",commandline);
}















/*
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
		#AK - #$o->printUTF(VRML::Handles::reserve($value->real_node(1)));
		$o->printUTF(VRML::Handles::reserve($value->real_node()));
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
*/
