/*******************************************************************
 Copyright (C) 1998 Tuomas J. Lukka 2003 John Stewart, Ayla Khan CRC Canada
 Portions Copyright (C) 1998 John Breen
 DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
 See the GNU Library General Public License (file COPYING in the distribution)
 for conditions of use and redistribution.
*********************************************************************/

/*
 *
 */

/************************************************************************/
/*									*/
/* implement EAI server functionality for FreeWRL.			*/
/*									*/
/************************************************************************/

#include "headers.h"
#include "Structs.h"
#include "Viewer.h"
#include <sys/time.h>

#ifdef __APPLE__
#include <sys/socket.h>
#endif

extern char *BrowserName, *BrowserVersion, *BrowserURL; // defined in VRMLC.pm


#define MAXEAIHOSTNAME	255		// length of hostname on command line
#define EAIREADSIZE	2048		// maximum we are allowed to read in from socket

int EAIwanted = FALSE;			// do we want EAI?
char EAIhost[MAXEAIHOSTNAME];		// host we are connecting to
int EAIport;				// port we are connecting to
int EAIinitialized = FALSE;		// are we running?
int EAIrecount = 0;			// retry counter for opening socket interface
int EAIfailed = FALSE;			// did we not succeed in opening interface?
int EAIconnectstep = 0;			// where we are in the connect sequence

/* socket stuff */
int 	sockfd;
struct sockaddr_in	servaddr;
fd_set rfds;
struct timeval tv;

/* eai connect line */
char *inpline;

int EAIVerbose = 0;

void EAI_send_string(char *str){
	int n;

	if (EAIVerbose) printf ("EAI_send_string, %s\n",str);
	n = send (sockfd, str, strlen(str) ,0);

}



/* open the socket connection - thanks to the stevens network programming book */
void connect_EAI() {
	char vers[200];

	if (EAIfailed) return;

	if (EAIVerbose) printf ("connect step %d\n",EAIconnectstep);

	switch (EAIconnectstep) {
	case 0: {
			if ((sockfd = socket (AF_INET, SOCK_STREAM, 0)) < 0) {
				printf ("EAI Socket open error\n");
				EAIfailed = TRUE;
				return;
			}

			EAIconnectstep ++;
			break;
		}

	case 1: {
			bzero (&servaddr, sizeof (servaddr));

			servaddr.sin_family = AF_INET;
			servaddr.sin_port = htons (2000);

			if (inet_pton(AF_INET,"127.0.0.1", &servaddr.sin_addr) < 0) {
				printf ("EAI inet_pton error\n");
				EAIfailed = TRUE;
				return;
			}
			EAIconnectstep ++;
			break;
		}

	case 2: {
			/* wait for EAI to come on line  - thus we never fail at this step */	
			if (connect (sockfd, (struct sockaddr_in *) &servaddr, sizeof (servaddr)) < 0) {
				// printf ("EAI connect error\n");
				// keep going until success EAIfailed = TRUE;
				return;
			}

			/* set up the select polling data structure */
			FD_ZERO(&rfds);
			FD_SET(sockfd, &rfds);

			/* tell the EAI what version of FreeWRL we are running */
			strncpy (vers,BrowserVersion,190);
			strcat (vers,"\n");
			EAI_send_string(vers);

			/* seems like we are up and running now, and waiting for a command */
			EAIinitialized = TRUE;	
			EAIconnectstep ++;
			break;
		}

	default: {}
	}
}



void create_EAI(char *eailine) {
        if (EAIVerbose) printf ("create_EAI called :%s:\n",eailine);

	/* already wanted? if so, just return */
	if (EAIwanted) return;

	/* so we know we want EAI */
	EAIwanted = TRUE;

	/* copy over the eailine to a local variable */
	inpline = malloc((strlen (eailine)+1) * sizeof (char));

	if (inpline == 0) {
		printf ("can not malloc memory in create_EAI\n");
		EAIwanted = FALSE;
		return;
	}

	strcpy (inpline,eailine);
	
	/* have we already started? */
	if (!EAIinitialized) {
		connect_EAI();
	}
}

void handle_EAI () {
	int retval;

	char buf[EAIREADSIZE];


	/* do nothing unless we are wanted */
	if (!EAIwanted) return;
	if (!EAIinitialized) {
		connect_EAI();
		return;
	}

	tv.tv_sec = 0;
	tv.tv_usec = 0;
	FD_ZERO(&rfds);
	FD_SET(sockfd, &rfds);
	retval = select(1, &rfds, NULL, NULL, &tv);

	if (retval) {
		printf("Data is available now.\n");
		retval = read (sockfd, buf,EAIREADSIZE);
		printf ("read in %d , max %d\n",retval,EAIREADSIZE);
	}
}



//sub gulp {
//	my($this, $handle) = @_;
//	my ($s,$b);
//	my($rin,$rout);
//	do {
//		# print "GULPING\n";
//		my $n = $handle->sysread($b,1000);
//		# print "GULPED $n ('$b')\n";
//		goto OUT if !$n;
//		$s .= $b;
//		vec($rin,$handle->fileno,1) = 1;
//		select($rout=$rin,"","",0);
//		# print "ROUT : $rout\n";
//	} while(vec($rout,$handle->fileno,1));
//	# print "ENDGULP\n";
//  OUT:
//	return $s;
//}
//
//# However funny this is, we do connect as the client ;)
//sub connect {
//	my($this, $addr) = @_;
//	$addr =~ /^(.*):(.*)$/ or 
//		die  "Invalid EAI adress '$addr'";
//	
//	($EAIhost, $EAIport) = ($1,$2);
//
//	#print ("FreeWRL: connect: remote $EAIhost  port $EAIport\n");
//	my $sock;
//	$sock = IO::Socket::INET->new(
//		Proto => "tcp",
//		PeerAddr => $EAIhost,
//		PeerPort => $EAIport
//	);
//
//	# is socket open? If not, wait.....
//	if (!$sock) { 
//		# print "FreeWRL: Connect: socket not opened yet...\n";
//		return;
//	}
//
//	$this->doconnect($sock);
//}
//
//sub doconnect {
//	my($this,$sock) = @_;
//
//	# set up a socket, when it is connected, then send EAI an initial message. 
//	$sock->autoflush(1);
//
//	$sock->setvbuf("",&_IONBF,0);
//	push @{$this->{Conn}}, $sock;
//	$sock->print("$VRML::Config{VERSION}\n");
//}
//
//sub poll {
//	my($this) = @_;
//	my ($nfound, $timeleft,$rout);
//
//
//	# if the socket is not open yet, try it, once again...
//	if (!defined $this->{Conn}) {
//		# print "FreeWRL: Poll: socket not opened yet for host $EAIhost port $EAIport\n";
//		# lets just try again in a while...
//		if ($EAIrecount < 100) {
//			$EAIrecount +=1;
//			return;
//		}
//
//		# woops! While is up! lets try connecting again.
//        	my $sock;
//       		 $sock = IO::Socket::INET->new(
//       		         Proto => "tcp",
//       		         PeerAddr => $EAIhost,
//       		         PeerPort => $EAIport
//       		 );
//
//       		 # is socket open? If not, wait.....
//		if (!$sock && $EAIfailure < $VRML::ENV{EAI_CONN_RETRY}) {
//			 $EAIfailure += 1;
//			 $EAIrecount = 0;
//       		         #print "FreeWRL: Poll: socket not opened yet...\n";
//       		 } elsif (!$sock
//			     && $EAIfailure >= $VRML::ENV{EAI_CONN_RETRY}
//			     && $VRML::ENV{AS_PLUGIN}) {
//			 print "FreeWRL: Poll: connect to EAI socket timed-out.\n"
//			     if $VRML::verbose::EAI;
//			 ## remove the sub poll from array reference
//			 shift(@{$this->{B}->{Periodic}});
//       		 } else {
//			#print "FreeWRL: Poll: Socket finally opened!!! \n";
//        		$this->doconnect($sock);
//		}
//
//	}
//
//	if (defined $this->{Conn}) {
//		my $rin = '';
//
//	#print "poll opened, eof is ", ref $this->{Conn},"\n";
//	
//		for(@{$this->{Conn}}) {
//			vec($rin, $_->fileno, 1) = 1;
//		}
//		($nfound, $timeleft) = select($rout = $rin, '', '', 0);
//		if($nfound) {
//			for(@{$this->{Conn}}) {
//				if(vec($rout, $_->fileno, 1)) {
//					$this->handle_input($_);
//				}
//			}
//		}
//	}
//}
//
//
//
//sub find_actual_node_and_field {
//	my ($id, $field, $eventin) = @_;
//	my $actualfield;
//
//	# print "find_actual_node_and_field, looking for ",
//	# 	VRML::NodeIntern::dump_name($id)," field $field\n";
//	my $node = VRML::Handles::get($id);
//	if ($node eq ""){
//		# node was not registered
//		$node = $id;
//	}
//
//	# remove the set_ and _changed, if it exists.
//	# actual fields (NOT EventIN/OUTs) are what we are usually after
//	$actualfield = $field;
//	if ($eventin == 1) {
//  		$actualfield =~ s/^set_//; # trailing new line....
//	} else {
//  		$actualfield =~ s/_changed$//; # trailing new line....
//	}
//
//	 # print "find_actual_node, looking at node ",
//	 # 	VRML::NodeIntern::dump_name($node),"  ref ", ref $node, 
//	 # 	" field :$field:, actualfield $actualfield eventin flag $eventin\n";
//
//	if (exists $node->{Fields}{$field}) {
//		# print "find_actual_node this is a direct pointer to the field, just returning\n";
//		return ($node, $field);
//	}
//
//	if (exists $node->{Fields}{$actualfield}) {
//		# print "find_actual_node this is a direct pointer to the actualfield, just returning\n";
//		return ($node, $actualfield);
//	}
//
//	# print "find_actual_node, step 0.3\n";
//
//	#AK - try to let FieldHash deal with locating IS references...
//	#AK - #my $direction;
//	#AK - #if ($eventin == 1) { $direction = "IS_ALIAS_IN"}
//	#AK - #else {$direction = "IS_ALIAS_OUT"};
//	#AK - #
//	#AK - #if (defined $node->{Scene}{$direction}{$field}) {
//		# aha! is this an "IS"?
//		# print "find_actual_node, this is a IsProto\n";
//
//		#AK - #my $n; my $f;
//
//		#AK - #$n = $node->{Scene}{$direction}{$field}[0][0];
//		#AK - #$f = $node->{Scene}{$direction}{$field}[0][1];
//		# print "find_actual_node, n is ",VRML::NodeIntern::dump_name($n)," f is $f\n";
//		#AK - #if ($n != "") {
//			# it is an is...
//			# print "find_actual_node, returning n,f\n";
//			#AK - #return ($n, $f);
//		#AK - #}
//		# it is a protoexp, and it it not an IS, so, lets just return
//		# the original, and cross our fingers that it is right.
//		# print "find_actual_node, actually, did not find node, returning defaults\n";
//		#AK - #return ($node, $field);
//	#AK - #}
//
//	#JAS . dont actually know if any of the rest is of importance, but
//	#JAS keeping it here for now.
//
//	# Hmmm - it was not a PROTO, lets just see if the field
//	# exists here.
//
//	if (defined $node->{$field}) {
//		return ($node, $field);
//	}
//	# Well, next test. Is this an EventIn/EventOut, static parameter to
//	# a PROTO?
//	# print "find_actual_node, step3\n";
//
//	#AK - #if (defined $node->{Scene}) {
//		#AK - #$node = VRML::Handles::front_end_child_get($node);
//		#JAS ($node,$field) = find_actual_node_and_field ($node,$field,$eventin);
//		#AK - #return ($node,$field);
//	#AK - #}
//
//	return ($node,$field);
//}
//
//
//sub handle_input {
//	my($this, $hand) = @_;
//
//	my @lines = split "\n",$this->gulp($hand);
//
//	# is the socket closed???
//	if ($#lines == -1) {
//		# send a "quitpressed" - note that this will only be
//		# intercepted when not running in netscape. It is 
//		# very useful, however, when running eai standalone.
//		$this->{B}->{BE}->{QuitPressed} = 1;
//	}
//
//	while (@lines) {
//		if($VRML::verbose::EAI) {
//		  print "Handle input $#lines\nEAI input:\n";
//		  my $myline; 
//		  foreach $myline (@lines) {
//			print "....",$myline,".... \n";
//		  }
//		  print ".. finished\n\n";
//		}
//
//		my $reqid = shift @lines; # reqid + newline
//
//                if ($reqid eq '') {
//		  $reqid = shift @lines; # reqid + newline
//                }
//
//		my $str = shift @lines; 
//
//		if($str =~ /^GN (.*)$/) { # Get node
//        		# Kevin Pulo <kev@pulo.com.au>
//			my $node = $this->{B}->api_getNode($1);
//			if (!defined $node) {
//				warn("Node $1 is not defined");
//				next;
//			}
//
//			if ("VRML::DEF" eq ref $node) {
//				$node = $this->{B}->api_getNode(VRML::Handles::return_def_name($1));
//				if (!defined $node) {
//					warn("DEF node $1 is not defined");
//					next;
//				}
//			}
//
//			#AK - #if (defined $node->{IsProto}) {
//				# print "GN of $1 is a proto, getting the real node\n";
//				#AK - #$node = $node->real_node();
//			#AK - #}
//
//			my $id = VRML::Handles::reserve($node);
//
//			# remember this - this node is displayed already
//			VRML::Handles::displayed($node);
//
//			if ($VRML::verbose::EAI) {
//				  print "GN returns $id\n";
//			}
//			$hand->print("RE\n$reqid\n1\n$id\n");
//
//		} elsif($str =~ /^GT ([^ ]+) ([^ ]+)$/) { # get eventOut type
//			my($id, $field) = ($1, $2);
//
//			my $node;
//
//			($node,$field) = find_actual_node_and_field($id,$field,0);
//
//                        if ($VRML::verbose::EAI) {
//				print "GT, looking for type for node $node\n";
//			}
//
//			my ($kind, $type) = 
//			 $this->{B}->api__getFieldInfo($node, $field);
//
//			 if ($VRML::verbose::EAI) { print " type  $type\n";}
//		        $hand->print("RE\n$reqid\n0\n$type\n");
//
//		} elsif($str =~ /^GV ([^ ]+) ([^ ]+)$/) { # get eventOut Value
//			my($id, $field) = ($1, $2);
//			my $node;
//
//
//			($node,$field) = find_actual_node_and_field($id,$field,0);
//
//                        if ($VRML::verbose::EAI) {
//				print "GV, looking for type for node $node\n";
//			}
//
//			my ($kind, $type) = 
//			 $this->{B}->api__getFieldInfo($node, $field);
//			# print "GV - kind is $kind, type is $type\n";
//
//			#print "GV, trying first get\n";
//			my $val = $node->{RFields}{$field};
//			if ($val eq '') {
//				#print "GV, woops,, have to try normal fields\n";
//				$val = $node->{Fields}{$field};
//			}
//			#print "GV, got the value now\n";
//
//			my $strval;
//
//			if ($type eq "MFNode") {
//				if ($VRML::verbose::EAI) {
//					print "VRMLServ.pm: GV found a MFNode for $node\n";
//				}
//
//				# if this is a MFnode, we don't want the VRML CODE
//				# if ("ARRAY" eq ref $node->{Fields}{$field}) { print "and, it is an ARRAY\n";}
//
//				# $strval = "@{$node->{Fields}{$field}}";
//
//				foreach (@{$node->{Fields}{$field}}) {
//					if (VRML::Handles::check($_) == 0) {
//						$strval = $strval ." ". VRML::Handles::reserve($_);
//					} else {
//					 	$strval = $strval ." ". VRML::Handles::get($_);
//					}
//					# print " so, Im setting strval to $strval\n";
//				}
//
//			} else {
//				$strval = "VRML::Field::$type"->as_string($val);
//				# print "GV value is $val, strval is $strval\n";
//			}
//
//			if ($VRML::verbose::EAI) {
//				print "GV returns $strval\n";
//			}
//			$hand->print("RE\n$reqid\n2\n$strval\n");
//
//		} elsif($str =~ /^UR ([^ ]+) ([^ ]+)$/) { # update routing - for 0.27's adding
//							# MFNodes - to get touchsensors, etc, in there
//
//			# we should do things with this, rather than go through the
//			# whole scene graph again... JAS.
//			my($id, $field) = ($1, $2);
//			my $v = (shift @lines)."\n";
//		        # JS - sure hope we can remove the trailing whitespace ALL the time...
//  			$v =~ s/\s+$//; # trailing new line....
//
//			my $cnode = VRML::Handles::get($v);
//			$this->{B}->api__updateRouting($cnode, $field);
//
//			# are there any routes?
//         		#AK if (defined $cnode->{SceneRoutes}) {
//				# print "VRMLServ.pm - UR ROUTE ",
//        		        #  $cnode->{SceneRoutes}[0][0] , " ",
//        		        #  $cnode->{SceneRoutes}[0][1] , " ",
//        		        #  $cnode->{SceneRoutes}[0][2] , " ",
//        		        #  $cnode->{SceneRoutes}[0][3] , " from $this to node: $cnode\n";
//
//				#AK my $scene = $this->{B}{Scene};
//	
//	             #AK if($field eq "removeChildren") {
//					#AK my $item;		
//					#AK foreach $item (@{$cnode->{SceneRoutes}}) {
//						# print "deleting route ",$_[0], $_[1], $_[2], $_[3],"\n";
//						#AK $scene->delete_route($item);
//					#AK }
//				#AK } else {
//					#AK my $item;		
//					#AK foreach $item (@{$cnode->{SceneRoutes}}) {
//						#AK my ($fn,$ff,$tn,$tf) = @{$item};
//						# print "VRMLServ.pm - expanded: $fn $ff $tn $tf\n";
//						#my $fh = VRML::Handles::return_def_name($fn);
//        					#my $th = VRML::Handles::return_def_name($tn);
//
//						#my $fh = VRML::Handles::get($fn);
//						#my $th = VRML::Handles::get($tn);
//						
//						#AK my @ar=[$fn,$ff,$tn,$tf];
//						#AK $scene->new_route(@ar);
//					#AK }
//				#AK }
//			# } else {
//			# 	print "VRMLServ.pm - no routes defined\n";
//			#AK }
//
//			#AK $this->{B}->prepare2();
//			# make sure it gets rendered
//			#AK VRML::OpenGL::set_render_frame();
//
//			$hand->print("RE\n$reqid\n0\n0\n");
//
//		} elsif($str =~ /^SC ([^ ]+) ([^ ]+)$/) { # send SFNode eventIn to node
//			my($id, $field) = ($1,$2);
//			my $v = (shift @lines)."\n";
//
//		        # JS - sure hope we can remove the trailing whitespace ALL the time...
//  			$v =~ s/\s+$//; # trailing new line....
//
//			my $node;
//			($node,$field) = find_actual_node_and_field($id,$field,1);
//
//			my $child = VRML::Handles::get($v);
//
//			if ($VRML::verbose::EAI) {
//				print "VRMLServ.pm - node $node child $child field $field\n";
//			}
//
//			#AK - #if (defined $child->{IsProto}) {
//				#AK - #my $temp = $child;
//				#AK - #$child = $child->real_node();
//				
//				#print "VRMLServ.pm - child proto got $child\n";
//			#AK - #}
//
//			# the events are as follows:
//			# VRMSServ.pm - api__sendEvent(
//			#	handle VRML::Handles::get($id); 
//			#	"children"
//			#	 array VRML::Handles::get($v) + previous children
//			#
//			# Browser.pm:api__sendEvent(
//			#	->{EV}->send_event_to (same parameters)
//			#	   ie, node, field, value
//			#
//			# Events.pm:send_event_to(
//			#	push on {ToQueue}, [parameters] 
//			#
//			# then, some time later....
//			# Browser.pm:tick calls
//			# Events.pm:propagate_events sends this eventually to
//			#
//			# Scene.pm:receive_event (this, field, value...)
//			# Tuomas' comments follow:	
//			# The FieldHash
//			#
//			# This is the object behind the "RFields" hash member of
//			# the object VRML::NodeIntern. It allows you to send an event by
//			# simply saying "$node->{RFields}{xyz} = [3,4,5]" for which
//			# calls the STORE method here which then queues the event.
//			#
//			# so, 
//			# Scene.pm:STORE (node, "children" value)
//			#	$$v = $value;
//			# 	$node->set_backend_fields ("children");
//			#
//			# Scene.pm:set_backend_fields (field)
//			#	calls make_backend for $v
//			#	takes the global $v, creates a global $f{"children"}=$v, 
//			#	and calls
//			#	$be->set_fields($this->{BackNode},\%f);
//			#	
//			# and the backend sets the fields, and everyone lives happily
//			# ever after...
//			#	
//
//			# AK - #if ($field eq "removeChildren") {
//				# AK - #if ($this->{B}->checkChildPresent($node,$child)) {
//		  			# AK - #my @av = $this->{B}->removeChild($node, $child);
//		  			# AK - #$this->{B}->api__sendEvent($node, "children",\@av);
//				# AK - #}
//			# AK - #} else {
//				# is the child already there???
//				# AK - #if ($field eq "addChildren") {
//					# AK - #$field = "children";
//				# AK - #}
//				# AK - #if (!($this->{B}->checkChildPresent($node,$child))) {
//					#JAS my @av = @{$node->{RFields}{$field}};
//					# AK - #my @av = @{$node->{Fields}{$field}};
//					# AK - #push @av, $child;
//		  			# AK - #$this->{B}->api__sendEvent($node, $field,\@av);
//				# AK - #}
//			# AK - #}
//
//			$this->{B}->api__sendEvent($node, $field, [$child]);
//
//			# make sure it gets rendered
//			VRML::OpenGL::set_render_frame();
//		    $hand->print("RE\n$reqid\n0\n0\n");
//
//		} elsif($str =~ /^SE ([^ ]+) ([^ ]+)$/) { # send eventIn to node
//			my($id, $field) = ($1,$2);
//			my $v = (shift @lines)."\n";
//
//			# JS - sure hope we can remove the trailing whitespace ALL the time...
//  			$v =~ s/\s+$//; # trailing new line....
//
//			my $node = VRML::Handles::get($id);
//	
//			#AK # $node = VRML::Handles::front_end_child_get($node);
//
//			my ($x,$ft) = $this->{B}->api__getFieldInfo($node,$field);
//
//			# make sure it gets rendered
//			VRML::OpenGL::set_render_frame();
//	
//			($node,$field) = find_actual_node_and_field($id,$field,1);
//
//
//			if ($ft eq "SFNode"){
//				#print "VRMLServ.pm - doing a SFNode\n";
//				my $child = VRML::Handles::get($v);
//				#AK - #if (defined $child->{IsProto}) {
//					#print "VRMLServ.pm - SE child proto got\n";
//					#AK - #$child =  $child->real_node();
//				#AK - #}
//				#print "VRMLServ.pm, ft $ft child $child\n";
//				$this->{B}->api__sendEvent($node, $field, $child);
//			} else {
//			    	my $value = "VRML::Field::$ft"->parse("FOO",$v);
//		    		#print "VRMLServ.pm, at 3, sending to $node ",
//				# 	" field $field value $v\n";
//		    		$this->{B}->api__sendEvent($node, $field, $value);
//			}
//
//		} elsif($str =~ /^DN (.*)$/) { # Dispose node
//			VRML::Handles::release($1);
//		        $hand->print("RE\n$reqid\n0\n");
//
//		} elsif($str =~ /^RL ([^ ]+) ([^ ]+) ([^ ]+)$/) {
//			my($id, $field, $lid) = ($1,$2,$3);
//			my $node;
//		
//			# Register Listener - send an event if changed.
//
//			($node,$field) = find_actual_node_and_field($id,$field,0);
//
//			# print "RL, field $field, node $node\n";
//			$this->{B}->api__registerListener(
//				$node,
//				$field,
//				sub {
//					$this->send_listened($hand,
//						$node,$id,$field,$lid,
//						$_[0]);
//				}
//			);
//		        $hand->print("RE\n$reqid\n0\n0\n");
//
//		} elsif($str =~ /^AR ([^ ]+) ([^ ]+) ([^ ]+) ([^ ]+)$/) {  # addRoute
//			my($fn, $ff, $tn, $tf) = ($1,$2,$3,$4);
//
//			my $nfn;
//			my $ntn;
//			my $field;
//			($nfn, $field) = find_actual_node_and_field($fn, $ff, 0);
//			($ntn, $field) = find_actual_node_and_field($tn, $tf, 1);
//
//			#AK - #my @ar = [$nfn,$ff,$ntn,$tf];
//			my $scene = $this->{B}{Scene};
//
//			$scene->new_route($nfn, $ff, $ntn, $tf);
//
//			$this->{B}->prepare2();
//			# make sure it gets rendered
//			VRML::OpenGL::set_render_frame();
//
//
//			$hand->print("RE\n$reqid\n0\n0\n");
//
//		} elsif($str =~ /^DR ([^ ]+) ([^ ]+) ([^ ]+) ([^ ]+)$/) {  # deleteRoute
//			my($fn, $ff, $tn, $tf) = ($1,$2,$3,$4);
//			print "deleteroute, $fn $ff, $tn $tf\n";
//
//			my $scene = $this->{B}{Scene};
//			#AK - #my $fromNode = VRML::Handles::get($fn)->real_node();
//			#AK - #my $toNode = VRML::Handles::get($tn)->real_node();
//			#AK - #my @ar = [$fromNode, $ff, $toNode, $tf];
//			
//			my $nfn;
//			my $ntn;
//			my $field;
//			($nfn, $field) = find_actual_node_and_field($fn, $ff, 0);
//			($ntn, $field) = find_actual_node_and_field($tn, $tf, 1);
//
//			$scene->delete_route($nfn, $ff, $ntn, $tf);
//
//			$this->{B}->prepare2();
//
//			$hand->print("RE\n$reqid\n0\n0\n");
//
//		} elsif($str =~ /^GNAM$/) { # Get name
//		        $hand->print("RE\n$reqid\n0\n", $this->{B}->getName(), "\n");
//
//		} elsif($str =~ /^GVER$/) { # Get Version
//		        $hand->print("RE\n$reqid\n0\n", $this->{B}->getVersion(), "\n");
//
//		} elsif($str =~ /^GCS$/) { # Get Current Speed
//		        $hand->print("RE\n$reqid\n0\n", $this->{B}->getCurrentSpeed(), "\n");
//
//		} elsif($str =~ /^GCFR$/) { # Get Current Frame Rate
//		        $hand->print("RE\n$reqid\n0\n", $this->{B}->getCurrentFrameRate(), "\n");
//
//		} elsif($str =~ /^GWU$/) { # Get WorldURL
//		        $hand->print("RE\n$reqid\n0\n", $this->{B}->getWorldURL(), "\n");
//
//		} elsif($str =~ /^RW (.*)$/) { # replaceWorld
//		        $hand->print("RE\n$reqid\n0\n", $this->{B}->replaceWorld($1), "\n");
//
//		} elsif($str =~ /^LU$/) { # LoadURL
//		        $hand->print("RE\n$reqid\n0\n", $this->{B}->loadURL(), "\n");
//
//		} elsif($str =~ /^SD$/) { # set Description
//		        $hand->print("RE\n$reqid\n0\n", $this->{B}->setDescription(), "\n");
//
//		} elsif($str =~ /^CVS (.*)$/) { # Create VRML From String
//			my $vrmlcode = $1;
//			my $ll = "";
//			while ($ll ne "EOT") {
//			  $vrmlcode = "$vrmlcode $ll\n";
//                          $ll = shift @lines;
//			}
//
//			my $retval = $this->{B}->createVrmlFromString($vrmlcode);
//
//			if ($VRML::verbose::EAI) { print "CVS returns $retval\n";}
//		        $hand->print("RE\n$reqid\n0\n", $retval, "\n");
//
//		} elsif($str =~ /^CVU (.*)$/) { # Create VRML From URL
//			my $retval = $this->{B}->createVrmlFromURL($1,$2);
//			if ($VRML::verbose::EAI) { print "CVU returns $retval\n";}
//		        $hand->print("RE\n$reqid\n0\n", $retval, "\n");
//
//
//		} elsif($str =~ /^STOP$/) { # set Description
//			print "FreeWRL got a stop!!\n";
//		} else {
//			if ($str ne  "") {
//				die("Invalid EAI input: '$str'");
//			}
//		}
//	}
//	# print "ENDLINES\n";
//}
//
//sub send_listened {
//	my($this, $hand, $node, $id, $field, $lid, $value) = @_;
//
//	my $ft = $node->{Type}{FieldTypes}{$field};
//
//	if ($VRML::verbose::EAI) {
//		print "send_listened, hand $hand node $node id $id  field $field  lid $lid  value $value\n";
//		print "field type is $ft\n";
//	 	if ("ARRAY" eq ref $value) { 
//			my $item;
//		 	foreach $item (@{$value}) {
//				print "	value element $item \n";
//			}
//		}
//	}
//
//
//	
//	my $str = "VRML::Field::$ft"->as_string($value);
//
//	# position and orientation sends an event per loop, we do not need
//	# to send duplicate positions and orientations, but this may give us a bug...
//
//
//	if ($VRML::EAIServer::evvals{$lid} eq $str) {
//		return;
//	}
//
//	$VRML::EAIServer::evvals{$lid} = $str; # save it for next time.
//	$hand->print("EV\n"); # Event incoming
//	$hand->print("$lid\n");
//	$hand->print("$str\n");
//}
//
//
//1;
