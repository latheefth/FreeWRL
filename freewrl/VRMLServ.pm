
# $Id$
#
# Copyright (C) 1998 Tuomas J. Lukka 1999 John Stewart CRC Canada
# DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
# See the GNU Library General Public License (file COPYING in the distribution)
# for conditions of use and redistribution.

# Implement communication with EAI and script processes.

#
# $Log$
# Revision 1.22  2002/11/14 20:09:42  crc_canada
# handled EAI socket open hangs (I hope), and if socket closes, nicely
# stops freewrl
#
# Revision 1.21  2002/09/19 19:40:14  crc_canada
# much EAI work
#
# Revision 1.20  2002/08/06 04:41:36  ayla
#
# Merging mozilla development branch to trunk.
# The plugin for the Mozilla browser (version 1.0) is only partially working.
#
# Revision 1.19.2.1  2002/07/11 19:00:22  ayla
#
# Got freewrl partly working as a plugin.
# Doesn't handle resizing in Mozilla yet and there's a problem with EAI.
#
# Revision 1.19  2002/05/22 21:49:51  ayla
#
# Files changed to reflect change from VRML::Node to VRML::NodeIntern.
#
# Revision 1.18  2002/05/08 18:13:27  crc_canada
# debugging statements removed
#
# Revision 1.17  2002/05/08 14:46:57  crc_canada
# Had a problem finding a reference to a route that was within a proto. Fixed
# I hope.
#
# send_listened, only sends if value is changed.
#
# Revision 1.16  2002/04/22 18:20:42  crc_canada
# Extra debugging print statements removed (in GV routine)
#
# Revision 1.14  2001/12/12 17:02:04  crc_canada
# more updates for EAI
#
# Revision 1.13  2001/10/24 14:47:03  crc_canada
# removed extra debug prints
#
# Revision 1.12  2001/10/24 14:15:02  crc_canada
# Additions for EAI functionality
#
# Revision 1.11  2001/08/17 20:11:05  ayla
#
# Begin initial trunk-NetscapeIntegration merge.
#
# Revision 1.10  2001/07/31 16:24:13  crc_canada
# added hooks to only update scene when an event happens (reduces cpu usage)
#
# Revision 1.9.4.3  2001/08/17 17:50:54  ayla
#
# Added a condition to stop polling for an EAI socket when FreeWRL is run as
# a Netscape plugin after a time-out period.
#
# The POD at the beginning of freewrl.PL has also been updated.
#
# Revision 1.9.4.2  2001/08/17 14:42:34  ayla
#
# A parameter was added to the VRML env hash to establish how long it takes to
# timeout on an attempt to connect to an EAI socket when FreeWRL is a Netscape
# plugin.  This is necessary because the command line options used by the plugin
# include -eai host:port.  See VRMLServ.pm for how the timing is done.
#
# Revision 1.10  2001/07/31 16:24:13  crc_canada
# added hooks to only update scene when an event happens (reduces cpu usage)
#
# Revision 1.9  2000/11/29 18:46:46  crc_canada
# add replaceWorld call
#
# Revision 1.8  2000/11/24 19:02:45  crc_canada
# Works with find_transform now!
#
# Revision 1.7  2000/10/28 17:43:20  crc_canada
# EAI addchildren, etc, should be ok now.
#
# Revision 1.6  2000/10/16 17:07:32  crc_canada
# Finish EAI add/remove children code.
#
# Revision 1.5  2000/10/13 14:13:48  crc_canada
# removed some unneeded print statements
#
# Revision 1.4  2000/10/12 12:18:20  crc_canada
# fixed a send event type mfnode bug, whereby two events in succession would
# destroy the preceeding.
#
# Revision 1.3  2000/09/22 13:55:24  crc_canada
# Fixed bugs where EAI would hang if appletviewer opened after freewrl. Also
# making changes in EAI code - all new java files in vrml/external needed.
#
# Revision 1.2  2000/08/05 11:56:56  rcoscali
# Add CVS keywords
#
#

package VRML::EAIServer;
use FileHandle;
use IO::Socket;
use strict;

# EAIhost and EAIport are passed in to freewrl as command line parameters.
my $EAIhost = "";
my $EAIport = 0;

# EAIrecount  is used for when a connection is requested, but is not
# opened. This is a retry counter.
my $EAIrecount = 0;
# Every 100th EAIrecount represents a failure.
my $EAIfailure = 0;

sub new {
	my($type,$browser) = @_;
	my $this = bless {
		B => $browser,
	}, $type;
	## NB: This code reference will be shifted out of the browser's
	## Periodic array if we hit the maximum connection failures, as
	## specified in $VRML::ENV{EAI_CONN_RETRY} when FreeWRL is used
	## as a Netscape plugin.
	$browser->add_periodic(sub {$this->poll});
	return $this;
}

sub gulp {
	my($this, $handle) = @_;
	my ($s,$b);
	my($rin,$rout);
	do {
		# print "GULPING\n";
		my $n = $handle->sysread($b,1000);
		# print "GULPED $n ('$b')\n";
		goto OUT if !$n;
		$s .= $b;
		vec($rin,$handle->fileno,1) = 1;
		select($rout=$rin,"","",0);
		# print "ROUT : $rout\n";
	} while(vec($rout,$handle->fileno,1));
	# print "ENDGULP\n";
  OUT:
	return $s;
}

# However funny this is, we do connect as the client ;)
sub connect {
	my($this, $addr) = @_;
	$addr =~ /^(.*):(.*)$/ or 
		die  "Invalid EAI adress '$addr'";
	
	($EAIhost, $EAIport) = ($1,$2);

	#print ("FreeWRL: connect: remote $EAIhost  port $EAIport\n");
	my $sock;
	$sock = IO::Socket::INET->new(
		Proto => "tcp",
		PeerAddr => $EAIhost,
		PeerPort => $EAIport
	);

	# is socket open? If not, wait.....
	if (!$sock) { 
		# print "FreeWRL: Connect: socket not opened yet...\n";
		return;
	}

	$this->doconnect($sock);
}

sub doconnect {
	my($this,$sock) = @_;

	# set up a socket, when it is connected, then send EAI an initial message. 
	$sock->autoflush(1);

	$sock->setvbuf("",&_IONBF,0);
	push @{$this->{Conn}}, $sock;
	$sock->print("$VRML::Config{VERSION}\n");
}

sub poll {
	my($this) = @_;
	my ($nfound, $timeleft,$rout);


	# if the socket is not open yet, try it, once again...
	if (!defined $this->{Conn}) {
		# print "FreeWRL: Poll: socket not opened yet for host $EAIhost port $EAIport\n";
		# lets just try again in a while...
		if ($EAIrecount < 100) {
			$EAIrecount +=1;
			return;
		}

		# woops! While is up! lets try connecting again.
        	my $sock;
       		 $sock = IO::Socket::INET->new(
       		         Proto => "tcp",
       		         PeerAddr => $EAIhost,
       		         PeerPort => $EAIport
       		 );

       		 # is socket open? If not, wait.....
		if (!$sock && $EAIfailure < $VRML::ENV{EAI_CONN_RETRY}) {
			 $EAIfailure += 1;
			 $EAIrecount = 0;
       		         #print "FreeWRL: Poll: socket not opened yet...\n";
       		 } elsif (!$sock
			     && $EAIfailure >= $VRML::ENV{EAI_CONN_RETRY}
			     && $VRML::ENV{AS_PLUGIN}) {
			 print "FreeWRL: Poll: connect to EAI socket timed-out.\n"
			     if $VRML::verbose::EAI;
			 ## remove the sub poll from array reference
			 shift(@{$this->{B}->{Periodic}});
       		 } else {
			#print "FreeWRL: Poll: Socket finally opened!!! \n";
        		$this->doconnect($sock);
		}

	}

	if (defined $this->{Conn}) {
		my $rin = '';

	#print "poll opened, eof is ", ref $this->{Conn},"\n";
	
		for(@{$this->{Conn}}) {
			vec($rin, $_->fileno, 1) = 1;
		}
		($nfound, $timeleft) = select($rout = $rin, '', '', 0);
		if($nfound) {
			for(@{$this->{Conn}}) {
				if(vec($rout, $_->fileno, 1)) {
					$this->handle_input($_);
				}
			}
		}
	}
}



sub find_actual_node_and_field {
	my ($id, $field, $eventin) = @_;
	my $actualfield;

	# print "find_actual_node_and_field, looking for ",
	# 	VRML::NodeIntern::dump_name($id)," field $field\n";
	my $node = VRML::Handles::get($id);
	if ($node eq ""){ 
# print "find_actual_node_and_field, node $id was not registered\n";
		# node was not registered
		$node = $id;
	}

	# remove the set_ and _changed, if it exists.
	# actual fields (NOT EventIN/OUTs) are what we are usually after
	$actualfield = $field;
	if ($eventin == 1) {
  		$actualfield =~ s/^set_//; # trailing new line....
	} else {
  		$actualfield =~ s/_changed$//; # trailing new line....
	}

# foreach (keys %{$node->{Fields}}) {
# 	print "	Field $_\n";
# }

	 # print "find_actual_node, looking at node ",
	 # 	VRML::NodeIntern::dump_name($node),"  ref ", ref $node, 
	 # 	" field :$field:, actualfield $actualfield eventin flag $eventin\n";

	if (exists $node->{Fields}{$field}) {
		# print "find_actual_node this is a direct pointer to the field, just returning\n";
		return ($node, $field);
	}

	if (exists $node->{Fields}{$actualfield}) {
		# print "find_actual_node this is a direct pointer to the actualfield, just returning\n";
		return ($node, $actualfield);
	}

	# print "find_actual_node, step 0.3\n";

	my $direction;
	if ($eventin ==1) { $direction = "IS_ALIAS_IN"}
	else {$direction = "IS_ALIAS_OUT"};

# print "find_actual, looking if IS defined\n";
# foreach (keys %{$node->{Scene}}) {
# 	print "	Scene $_\n";
# }

	if (defined $node->{Scene}{$direction}{$field}) {
		# aha! is this an "IS"?
		# print "find_actual_node, this is a IsProto\n";

		my $n; my $f;


		$n = $node->{Scene}{$direction}{$field}[0][0];
		$f = $node->{Scene}{$direction}{$field}[0][1];
		# print "find_actual_node, n is ",VRML::NodeIntern::dump_name($n)," f is $f\n";
		if ($n != "") {
			# it is an is...
			# print "find_actual_node, returning n,f\n";
			return ($n, $f);
		}
		# it is a protoexp, and it it not an IS, so, lets just return
		# the original, and cross our fingers that it is right.
		# print "find_actual_node, actually, did not find node, returning defaults\n";
		return ($node,$field);
	}

	#JAS . dont actually know if any of the rest is of importance, but
	#JAS keeping it here for now.

	# Hmmm - it was not a PROTO, lets just see if the field 
	# exists here.

	# print "find_actual_node, step2\n";

	if (defined $node->{$field}) {
		return ($node,$field);
	}
	# Well, next test. Is this an EventIn/EventOut, static parameter to
	# a PROTO?
	# print "find_actual_node, step3\n";

	if (defined $node->{Scene}) {
	
		# print "find_actual_node, Scene defd, was ", VRML::NodeIntern::dump_name($node);
		$node = VRML::Handles::front_end_child_get($node);
		# print " is ", VRML::NodeIntern::dump_name($node)," field $field\n";

		#JAS ($node,$field) = find_actual_node_and_field ($node,$field,$eventin);
		return ($node,$field);
	}
	# print "find_actual_node, step4\n";
	return ($node,$field);
}


sub handle_input {
	my($this, $hand) = @_;

	my @lines = split "\n",$this->gulp($hand);

	# is the socket closed???
	if ($#lines == -1) {
		# send a "quitpressed" - note that this will only be
		# intercepted when not running in netscape. It is 
		# very useful, however, when running eai standalone.
		$this->{B}->{BE}->{QuitPressed} = 1;
	}

	while(@lines) {
		if($VRML::verbose::EAI) {
		  print "Handle input $#lines\nEAI input:\n";
		  my $myline; 
		  foreach $myline (@lines) {
			print "....",$myline,".... \n";
		  }
		  print ".. finished\n\n";
		}

		my $reqid = shift @lines; # reqid + newline

                if ($reqid eq '') {
		  $reqid = shift @lines; # reqid + newline
                }

		my $str = shift @lines; 

		if($str =~ /^GN (.*)$/) { # Get node
        		# Kevin Pulo <kev@pulo.com.au>
			my $node = $this->{B}->api_getNode($1);
			if ("VRML::DEF" eq ref $node) {
				$node = $this->{B}->api_getNode(VRML::Handles::return_def_name($1));
			}

			if (defined $node->{IsProto}) {
				# print "GN of $1 is a proto, getting the real node\n";
				$node = $node->real_node();
			}

			my $id = VRML::Handles::reserve($node);

                        # remember this - this node is displayed already
                        VRML::Handles::displayed($node);

			if($VRML::verbose::EAI) {
	                  print "GN returns $id\n";
       		        }
		        $hand->print("RE\n$reqid\n1\n$id\n");

		} elsif($str =~ /^GT ([^ ]+) ([^ ]+)$/) { # get eventOut type
			my($id, $field) = ($1, $2);

			my $node;

			($node,$field) = find_actual_node_and_field($id,$field,0);

                        if ($VRML::verbose::EAI) {
				print "GT, looking for type for node $node\n";
			}

			my ($kind, $type) = 
			 $this->{B}->api__getFieldInfo($node, $field);

			 if ($VRML::verbose::EAI) { print " type  $type\n";}
		        $hand->print("RE\n$reqid\n0\n$type\n");

		} elsif($str =~ /^GV ([^ ]+) ([^ ]+)$/) { # get eventOut Value
			my($id, $field) = ($1, $2);
			my $node;


			($node,$field) = find_actual_node_and_field($id,$field,0);

                        if ($VRML::verbose::EAI) {
				print "GV, looking for type for node $node\n";
			}

			my ($kind, $type) = 
			 $this->{B}->api__getFieldInfo($node, $field);
			# print "GV - kind is $kind, type is $type\n";

			#print "GV, trying first get\n";
			my $val = $node->{RFields}{$field};
			if ($val eq '') {
				#print "GV, woops,, have to try normal fields\n";
				$val = $node->{Fields}{$field};
			}
			#print "GV, got the value now\n";

			my $strval;

                        if ($type eq "MFNode") {
                                if ($VRML::verbose::EAI) {
                                        print "VRMLServ.pm: GV found a MFNode for $node\n";
                                }

                                # if this is a MFnode, we don't want the VRML CODE
				# if ("ARRAY" eq ref $node->{Fields}{$field}) { print "and, it is an ARRAY\n";}

                                # $strval = "@{$node->{Fields}{$field}}";

				foreach (@{$node->{Fields}{$field}}) {
					 if (VRML::Handles::check($_) == 0) {
					$strval = $strval ." ". VRML::Handles::reserve($_);
					 } else {
					 	$strval = $strval ." ". VRML::Handles::get($_);
					 }
					# print " so, Im setting strval to $strval\n";
				}

                        } else {
                                $strval = "VRML::Field::$type"->as_string($val);
                                # print "GV value is $val, strval is $strval\n";
                        }

			if($VRML::verbose::EAI) {
	                  print "GV returns $strval\n";
       		        }
		        $hand->print("RE\n$reqid\n2\n$strval\n");

		} elsif($str =~ /^UR ([^ ]+) ([^ ]+)$/) { # update routing - for 0.27's adding
							# MFNodes - to get touchsensors, etc, in there

			# we should do things with this, rather than go through the
			# whole scene graph again... JAS.
			my($id, $field) = ($1,$2);
			my $v = (shift @lines)."\n";
		        # JS - sure hope we can remove the trailing whitespace ALL the time...
  			$v =~ s/\s+$//; # trailing new line....

			my $cnode = VRML::Handles::get($v);

			# are there any routes?
         		if (defined $cnode->{SceneRoutes}) {
				# print "VRMLServ.pm - UR ROUTE ",
        		        #  $cnode->{SceneRoutes}[0][0] , " ",
        		        #  $cnode->{SceneRoutes}[0][1] , " ",
        		        #  $cnode->{SceneRoutes}[0][2] , " ",
        		        #  $cnode->{SceneRoutes}[0][3] , " from $this to node: $cnode\n";

				my $scene = $this->{B}{Scene};
	
	                        if($field eq "removeChildren") {
					my $item;		
					foreach $item (@{$cnode->{SceneRoutes}}) {
						# print "deleting route ",$_[0], $_[1], $_[2], $_[3],"\n";
						$scene->delete_route($item);
					}
				} else {
					my $item;		
					foreach $item (@{$cnode->{SceneRoutes}}) {
						my ($fn,$ff,$tn,$tf) = @{$item};
						# print "VRMLServ.pm - expanded: $fn $ff $tn $tf\n";
						#my $fh = VRML::Handles::return_def_name($fn);
        					#my $th = VRML::Handles::return_def_name($tn);

						#my $fh = VRML::Handles::get($fn);
						#my $th = VRML::Handles::get($tn);
						
						my @ar=[$fn,$ff,$tn,$tf];
						$scene->new_route(@ar);
					}
				}
			# } else {
			# 	print "VRMLServ.pm - no routes defined\n";
			}



			$this->{B}->prepare2();
			# make sure it gets rendered
			VRML::OpenGL::set_render_frame();

		        $hand->print("RE\n$reqid\n0\n0\n");

		} elsif($str =~ /^SC ([^ ]+) ([^ ]+)$/) { # send SFNode eventIn to node
			my($id, $field) = ($1,$2);
			my $v = (shift @lines)."\n";

		        # JS - sure hope we can remove the trailing whitespace ALL the time...
  			$v =~ s/\s+$//; # trailing new line....

			my $node;
			($node,$field) = find_actual_node_and_field($id,$field,1);

			my $child = VRML::Handles::get($v);

			if ($VRML::verbose::EAI) {
				print "VRMLServ.pm - node $node child $child field $field\n";
			}

			if (defined $child->{IsProto}) {
				my $temp = $child;
				$child = $child->real_node();
				
				#print "VRMLServ.pm - child proto got $child\n";
			}

			# the events are as follows:
			# VRMSServ.pm - api__sendEvent(
			#	handle VRML::Handles::get($id); 
			#	"children"
			#	 array VRML::Handles::get($v) + previous children
			#
			# Browser.pm:api__sendEvent(
			#	->{EV}->send_event_to (same parameters)
			#	   ie, node, field, value
			#
			# Events.pm:send_event_to(
			#	push on {ToQueue}, [parameters] 
			#
			# then, some time later....
			# Browser.pm:tick calls
			# Events.pm:propagate_events sends this eventually to
			#
			# Scene.pm:receive_event (this, field, value...)
			# Tuomas' comments follow:	
			# The FieldHash
			#
			# This is the object behind the "RFields" hash member of
			# the object VRML::NodeIntern. It allows you to send an event by
			# simply saying "$node->{RFields}{xyz} = [3,4,5]" for which
			# calls the STORE method here which then queues the event.
			#
			# so, 
			# Scene.pm:STORE (node, "children" value)
			#	$$v = $value;
			# 	$node->set_backend_fields ("children");
			#
			# Scene.pm:set_backend_fields (field)
			#	calls make_backend for $v
			#	takes the global $v, creates a global $f{"children"}=$v, 
			#	and calls
			#	$be->set_fields($this->{BackNode},\%f);
			#	
			# and the backend sets the fields, and everyone lives happily
			# ever after...
			#	

			if($field eq "removeChildren") {
				if ($this->{B}->checkChildPresent($node,$child)) {
		  			my @av = $this->{B}->removeChild($node, $child);
		  			$this->{B}->api__sendEvent($node, "children",\@av);
				}
			} else {
				# is the child already there???
				if ($field eq "addChildren") {
					$field = "children";
				}
				if (!($this->{B}->checkChildPresent($node,$child))) {
					#JAS my @av = @{$node->{RFields}{$field}};
					my @av = @{$node->{Fields}{$field}};
					push @av, $child;
		  			$this->{B}->api__sendEvent($node, $field,\@av);
				}
			}
			# make sure it gets rendered
			VRML::OpenGL::set_render_frame();

		        $hand->print("RE\n$reqid\n0\n0\n");

		} elsif($str =~ /^SE ([^ ]+) ([^ ]+)$/) { # send eventIn to node
			my($id, $field) = ($1,$2);
			my $v = (shift @lines)."\n";
                        
		        # JS - sure hope we can remove the trailing whitespace ALL the time...
  			$v =~ s/\s+$//; # trailing new line....

			my $node = VRML::Handles::get($id);
	
			$node = VRML::Handles::front_end_child_get($node);

			my ($x,$ft) = $this->{B}->api__getFieldInfo($node,$field);

			# make sure it gets rendered
			VRML::OpenGL::set_render_frame();
	
			($node,$field) = find_actual_node_and_field($id,$field,1);


			if ($ft eq "SFNode"){
				#print "VRMLServ.pm - doing a SFNode\n";
				my $child = VRML::Handles::get($v);
				if (defined $child->{IsProto}) {
					#print "VRMLServ.pm - SE child proto got\n";
					$child =  $child->real_node();
				}
				#print "VRMLServ.pm, ft $ft child $child\n";
			    	$this->{B}->api__sendEvent($node, $field, $child);
			} else {
			    	my $value = "VRML::Field::$ft"->parse("FOO",$v);
		    		#print "VRMLServ.pm, at 3, sending to $node ",
				# 	" field $field value $v\n";
		    		$this->{B}->api__sendEvent($node, $field, $value);
			}

		} elsif($str =~ /^DN (.*)$/) { # Dispose node
			VRML::Handles::release($1);
		        $hand->print("RE\n$reqid\n0\n");

		} elsif($str =~ /^RL ([^ ]+) ([^ ]+) ([^ ]+)$/) {
			my($id, $field, $lid) = ($1,$2,$3);
			my $node;
		
			# Register Listener - send an event if changed.

			($node,$field) = find_actual_node_and_field($id,$field,0);

			# print "RL, field $field, node $node\n";
			$this->{B}->api__registerListener(
				$node,
				$field,
				sub {
					$this->send_listened($hand,
						$node,$id,$field,$lid,
						$_[0]);
				}
			);
		        $hand->print("RE\n$reqid\n0\n0\n");

		} elsif($str =~ /^AR ([^ ]+) ([^ ]+) ([^ ]+) ([^ ]+)$/) {  # addRoute
			my($fn, $ff, $tn, $tf) = ($1,$2,$3,$4);
			# print "addroute, $fn $ff, $tn $tf\n";

			#JAS if ("VRML::DEF" eq ref $fn) {
			#JAS 	print "$fn is a DEF, changing it to ";
			#JAS 	$fn = VRML::Handles::return_def_name($fn);
			#JAS 	print "$fn\n";
			#JAS }

			#JAS my $fromNode = VRML::Handles::get($fn)->real_node();
			#JAS my $toNode = VRML::Handles::get($tn)->real_node();
			
			my $nfn;
			my $ntn;
			my $field;
			($nfn,$field) = find_actual_node_and_field($fn,$ff,0);
			($ntn,$field) = find_actual_node_and_field($tn,$tf,1);

			my @ar=[$nfn,$ff,$ntn,$tf];
			my $scene = $this->{B}{Scene};

                        $scene->new_route(@ar);

			$this->{B}->prepare2();
                        # make sure it gets rendered
			VRML::OpenGL::set_render_frame();
                        

		        $hand->print("RE\n$reqid\n0\n0\n");

		} elsif($str =~ /^DR ([^ ]+) ([^ ]+) ([^ ]+) ([^ ]+)$/) {  # deleteRoute
			my($fn, $ff, $tn, $tf) = ($1,$2,$3,$4);
			print "deleteroute, $fn $ff, $tn $tf\n";

			my $fromNode = VRML::Handles::get($fn)->real_node();
			my $toNode = VRML::Handles::get($tn)->real_node();
			my @ar=[$fromNode,$ff,$toNode,$tf];
			my $scene = $this->{B}{Scene};

                        $scene->delete_route(@ar);

			$this->{B}->prepare2();

		        $hand->print("RE\n$reqid\n0\n0\n");

		} elsif($str =~ /^GNAM$/) { # Get name
		        $hand->print("RE\n$reqid\n0\n", $this->{B}->getName(), "\n");

		} elsif($str =~ /^GVER$/) { # Get Version
		        $hand->print("RE\n$reqid\n0\n", $this->{B}->getVersion(), "\n");

		} elsif($str =~ /^GCS$/) { # Get Current Speed
		        $hand->print("RE\n$reqid\n0\n", $this->{B}->getCurrentSpeed(), "\n");

		} elsif($str =~ /^GCFR$/) { # Get Current Frame Rate
		        $hand->print("RE\n$reqid\n0\n", $this->{B}->getCurrentFrameRate(), "\n");

		} elsif($str =~ /^GWU$/) { # Get WorldURL
		        $hand->print("RE\n$reqid\n0\n", $this->{B}->getWorldURL(), "\n");

		} elsif($str =~ /^RW (.*)$/) { # replaceWorld
		        $hand->print("RE\n$reqid\n0\n", $this->{B}->replaceWorld($1), "\n");

		} elsif($str =~ /^LU$/) { # LoadURL
		        $hand->print("RE\n$reqid\n0\n", $this->{B}->loadURL(), "\n");

		} elsif($str =~ /^SD$/) { # set Description
		        $hand->print("RE\n$reqid\n0\n", $this->{B}->setDescription(), "\n");

		} elsif($str =~ /^CVS (.*)$/) { # Create VRML From String
			my $vrmlcode = $1;
			my $ll = "";
			while ($ll ne "EOT") {
			  $vrmlcode = "$vrmlcode $ll\n";
                          $ll = shift @lines;
			} 

			my $retval = $this->{B}->createVrmlFromString($vrmlcode);
			if ($VRML::verbose::EAI) { print "CVS returns $retval\n";}
		        $hand->print("RE\n$reqid\n0\n", $retval, "\n");

		} elsif($str =~ /^CVU (.*)$/) { # Create VRML From URL
			my $retval = $this->{B}->createVrmlFromURL($1,$2);
			if ($VRML::verbose::EAI) { print "CVU returns $retval\n";}
		        $hand->print("RE\n$reqid\n0\n", $retval, "\n");


		} elsif($str =~ /^STOP$/) { # set Description
			print "FreeWRL got a stop!!\n";
		} else {
			if ($str ne  "") {
				die("Invalid EAI input: '$str'");
			}
		}
	}
	# print "ENDLINES\n";
}

sub send_listened {
	my($this, $hand, $node, $id, $field, $lid, $value) = @_;

	my $ft = $node->{Type}{FieldTypes}{$field};

	if ($VRML::verbose::EAI) {
		print "send_listened, hand $hand node $node id $id  field $field  lid $lid  value $value\n";
		print "field type is $ft\n";
	 	if ("ARRAY" eq ref $value) { 
			my $item;
		 	foreach $item (@{$value}) {
				print "	value element $item \n";
			}
		}
	}


	
	my $str = "VRML::Field::$ft"->as_string($value);

	# position and orientation sends an event per loop, we do not need
	# to send duplicate positions and orientations, but this may give us a bug...


	if ($VRML::EAIServer::evvals{$lid} eq $str) {
		return;
	}

	$VRML::EAIServer::evvals{$lid} = $str; # save it for next time.
	$hand->print("EV\n"); # Event incoming
	$hand->print("$lid\n");
	$hand->print("$str\n");
}


1;
