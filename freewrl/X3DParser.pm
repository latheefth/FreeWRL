#
#  X3DParser.pm,
#
# Copyright (C) 2004 Clayton Cottingham 1998 Tuomas J. Lukka 1999 John Stewart CRC Canada.
# DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
# See the GNU Library General Public License (file COPYING in the distribution)
# for conditions of use and redistribution.
#
# X3DParser.pm -- implement a X3D parser
#

use strict vars;

package X3D::Error;
use vars qw/@ISA @EXPORT $Word $qre $cre $Float $Integer/;
require Exporter;
@ISA = qw/Exporter/;

@EXPORT = qw/parsefail parsewarnstd $Word $qre $cre $Float $Integer/;

# Define the RE for a VRML word.
# $Word = q|[^\-+0-9"'#,\.\[\]\\{}\0-\x20][^"'#,\.\{\}\\{}\0-\x20]*|;

## Bug 424524:
## It was reported that there was some difficulty parsing VRML words when the
## character > occurred at the end of a word.
##
## The problem lay in the usage of the Perl assertion \b to designate a word
## boundary : \b is the position between \w and \W, either \W\w at the beginning
## of a word or \w\W at the end. Characters such as >, while legal in VRML97,
## are not included in \w, causing the truncation of the word.
##
## This bug was fixed by including the possibility of a premature word boundary:

$Word =
qr|[^\x30-\x39\x0-\x20\x22\x23\x27\x2b\x2c\x2d\x2e\x5b\x5c\x5d\x7b\x7d\x7f][^\x0-\x20\x22\x23\x27\x2c\x2e\x5b\x5c\x5d\x7b\x7d\x7f]*(?:\b[^\x0-\x20\x22\x23\x27\x2c\x2e\x5b\x5c\x5d\x7b\x7d\x7f])?|;

$qre = qr{(?<!\\)\"};    # " Regexp for unquoted double quote
$cre = qr{[^\"\n]};      # " Regexp for not dquote, not \n char

# Spec:
# ([+/-]?(
#         (
#           ([0-9]+(\.)?)
#          |([0-9]*\.[0-9]+)
#         )
#         ([eE][+\-]?[0-9]+)?
#         )
#        )
# XXX This is correct but might be too slow...
# $Float = q~[+-]?(?:[0-9]+\.?|[0-9]*\.[0-9]+)(?:[eE][+-]?[0-9]+)?~
###$Float = q~[\deE+\-\.]+~;
### Defer to regex compiler:
$Float = qr~[\deE+\-\.]+~;

# ([+\-]?(([0-9]+)|(0[xX][0-9a-fA-F]+)))

###$Integer = q~[\da-fA-FxX+\-]+~;
### Defer to regex compiler:
$Integer = qr~[\da-fA-FxX+\-]+~;

sub parsefail {
    my $p     = pos $_[0];
    my $n     = ( $p >= 50 ? 50 : $p );
    my $textb = substr( $_[0], $p - $n, $n );
    my $texta = substr( $_[0], $p, 50 );

    print("PARSE ERROR: '$textb' XXX '$texta', $_[1] $_[2]\n");
    exit(1);
}

sub parsewarnstd {
    my $p     = pos $_[0];
    my $n     = ( $p >= 100 ? 100 : $0 );
    my $textb = substr( $_[0], $p - $n, $n );
    warn("Parse warning: nonstandard feature: '$textb': $_[1]");
}

package X3D::Parser;

use strict vars;
use warnings;
use XML::Parser;
use Data::Dumper;

require 'VRML/VRMLNodes.pm';
require 'VRML/X3DFields.pm';
#require 'VRML/Parser.pm';

use vars qw/$Word $qre $cre/;

X3D::Error->import;

# Parse a whole file into $scene.
sub parse {
    my ( $scene, $text ) = @_;
    $VRML::verbose::parse = 1;

    print "entering x3d parser\n*****************\n" if $VRML::verbose::parse;

    my $p = XML::Parser->new( Style => 'Tree' );

    my $doc = $p->parse($text);

    my $start_level = 0;
    my @a;
    my @c;
    my  $b  = process_node( @$doc, $start_level,\@c);

    foreach my $bnub (@$b){
      if (ref $bnub eq 'ARRAY'){


#	warn $bnub->[0];
	#my $nt="X3D".$bnub->[0];
	my $nt=$bnub->[0];
	my $no = $VRML::Nodes{ $nt};

  if ( !defined $no ) {
        parsefail( $_[2], "Invalid node '$nt'" );
    }

	##look for proto/extern/


	###
	my %f;
	#check for validity? could do this before loading $doc 


	if (exists $bnub->[1]){
#it has attributes

#	  warn Dumper( $bnub->[1]);
#	  warn ref $bnub->[1];
	  my $attrs=$bnub->[1];


 my @array= map { $_ . ":" . $attrs->{$_} } keys %{$attrs} ;
foreach my $atts (keys  %$attrs){
#warn $atts ."=".$attrs->{$atts};




#warn $no;
#	  my	  $f = X3D::Parser::parse_exposedField( $atts, $no );

my $f=$atts;
#warn $f;
	  my	  $ft = $no->{FieldTypes}{$f};
#warn $ft;
	  	  if ( !defined $ft ) {
	  	      warn "\nInvalid field '$f' for node '$nt'\n";
	              warn "Possible fields are: ";
	              foreach ( keys %{ $no->{FieldTypes} } ) {
	  	      warn "$_ ";
	              }
	              warn "\n";
	  
	  #           exit(1);
		    }
	  	

#wrap this with is calls
  warn"Storing values... $ft for node $nt, f $f";
 ##if is 


###else....
#do checks on this later!
  $f{$f} = $attrs->{$atts};
#"VRML::Field::$ft"->parse( $scene, $_[2] );
            warn "storing type 2, $f, (",
              VRML::NodeIntern::dump_name( $f{$f} ),
              ")" ;#if $VRML::verbose::parse;

	my  $n = $scene->new_node($nt, \%f);



#	#push to array if defined;

	if (defined $n)
	  {
	    push @a,$n;
	  }


	}



	}

      }

    }
    

#end of x3d  doc



#push to scene

    $scene->topnodes( \@a );
}



sub process_node {
  my ($nt, $content,$level,$a ) = @_;

  if ($nt) {
   my $attrs = shift @$content;

warn Dumper($attrs);
warn $nt;
   unless ( $nt =~ /(X3D|Scene)/ ) {

  #NO THIS IS supposed to be a field put it in to the node!


   if ($nt=~/(Group|Transform)/){
my %hash=%$attrs;
#foreach (keys  %hash){warn $_ ,"",$hash{$_};}
$hash{children}=undef;
}
     elsif ($nt=~/(Box|Sphere|Cone)/){
my %hash=%$attrs;
#foreach (keys  %hash){warn $_ ,"",$hash{$_};}
$hash{geometry}=undef;
}
else {


}
     push (@{$a},[$nt,$attrs]);
   }
    ++$level;
    while (my @node = splice(@$content, 0, 2)) {
      process_node(@node,$level,$a);
    }
    --$level;
  }

  if ($level==0){
return $a;
  }
}

sub trim {
    my @out = @_;
    for (@out) {
        s/^\s+//;
        s/\s+$//;
    }
    return wantarray ? @out : $out[0];
}



sub parse_exposedField {
  my ( $field, $nodeType ) = @_;

#  my $tmp;
#  if ( $field =~ /^set_($VRML::Error::Word+)/ ) {
#    $tmp = $1;
#    if ( $nodeType->{EventIns}{$tmp} and $nodeType->{FieldKinds}{$tmp} =~ /^exposed/ ) {
#      $field = $tmp;
#    }
#  }

#  if ( $field =~ /($VRML::Error::Word+)_changed$/ ) {
#    $tmp = $1;
#    if ( $nodeType->{EventOuts}{$tmp} and $nodeType->{FieldKinds}{$tmp} =~ /^exposed/ ) {
#      $field = $tmp;
#    }
#  }
  return $field;
}

=cut

# Parse a statement, return a node if it is a node, otherwise
# return undef.
sub parse_statement {    # string in $_[1]
    my ($scene) = @_;
    warn caller;
    ## commas again
    #warn $_[1];
    $_[1] =~ /\G\s*,\s*/gsc;
    my $justcheck = $_[2];
    print "PARSE: '", substr( $_[1], pos $_[1] ), "'\n"
      if $VRML::verbose::parse;

    # Peek-ahead to see what is to come... store pos.
    my $p = pos $_[1];

    print "POSN: $p\n"
      if $VRML::verbose::parse;

    #warn $_[1];


  if ( $_[1] =~ /\G\s*EXTERNPROTO\b/gsc ) {
        ( pos $_[1] ) = $p;
        parse_externproto( $scene, $_[1] );
        return undef;
    }
    elsif ( $_[1] =~ /\G\s*PROTO\b/gsc ) {
        ( pos $_[1] ) = $p;
        parse_proto( $scene, $_[1] );
        return undef;
    }
    elsif ( $_[1] =~ /\G\s*ROUTE\b/gsc ) {
        ( pos $_[1] ) = $p;
        parse_route( $scene, $_[1] );
        return undef;
    }
    elsif ($justcheck) {
        return -1;
    }
    els

    #if ( $_[1] =~ /\G\s*($Word)/gsc ) {
    #       ( pos $_[1] ) = $p;
    print "AND NOW: ", ( pos $_[1] ), "\n";
    return X3D::Field::SFNode->parse( $scene, $_[1] );

    #    }



    else {
        print "WORD WAS: '$Word'\n"
          if $VRML::verbose::parse;
        parsefail( $_[1], "Can't find next statement" );
    }

}
=cut

package X3D::Field::SFNode;
use vars qw/$Word/;
VRML::Error->import;

my $LASTDEF = 1;

sub parse {
    my ( $type, $scene ) = @_;
    $_[2] =~ /\G\s*/gsc;
    $VRML::verbose::parse = 1;
    if ($VRML::verbose::parse) {
        my ( $package, $filename, $line ) = caller;
        print "X3D::Field::SFNode::parse: ", ( pos $_[2] ), " ", length $_[2],
          " from $package, $line\n";
    }

    $_[2] =~ /\G\s*($Word)/ogsc
      or parsefail( $_[2], "didn't match for sfnode fword" );

    my $nt = $1;
    warn $nt;
    if ( $nt eq "NULL" ) {
        return "NULL";
    }
    my $vrmlname;
    my $is_name;
    my $p;
    my $rep_field;
    my $field_counter = 1;

    if ( $nt eq "DEF" ) {
        $_[2] =~ /\G\s*($Word)/ogsc
          or parsefail( $_[2], "DEF must be followed by a defname" );

       # store this as a sequence number, because multiple DEFS of the same name
       # must be unique. (see the spec)
        $vrmlname = $1;
        warn $vrmlname;
        VRML::Handles::def_reserve( $vrmlname, "DEF$LASTDEF" );
        $LASTDEF++;
        my $defname = VRML::Handles::return_def_name($vrmlname);
        print "Parser.pm: DEF $vrmlname as $defname\n"
          if $VRML::verbose::parse;

        my $node = X3D::Field::SFNode->parse( $scene, $_[2] );
        print "DEF - node $defname is $node \n" if $VRML::verbose::parse;

        return $scene->new_def( $defname, $node, $vrmlname );

    }
    if ( $nt eq "USE" ) {
        $_[2] =~ /\G\s*($Word)/ogsc
          or parsefail( $_[2], "USE must be followed by a defname" );

        $vrmlname = $1;

        # is is already DEF'd???
        my $dn = VRML::Handles::return_def_name($vrmlname);
        if ( !defined $dn ) {
            print "USE name $vrmlname not DEFined yet\n";
            exit(1);
        }

        print "USE $dn\n"
          if $VRML::verbose::parse;
        return $scene->new_use($dn);
    }
    if ( $nt eq "Script" ) {
        print "SCRIPT!\n"
          if $VRML::verbose::parse;
        return VRML::Parser::parse_script( $scene, $_[2] );
    }
    my $proto;
    $p = pos $_[2];

    my $no = $VRML::Nodes{$nt};
    use Data::Dumper;

    #warn Dumper($no);
    ## look in PROTOs that have already been processed
    if ( !defined $no ) {
        $no = $scene->get_proto($nt);
        print "PROTO? '$no'\n"
          if $VRML::verbose::parse;
    }

    ## next, let's try looking for EXTERNPROTOs in the file
    if ( !defined $no ) {
        ## return to the beginning
        pos $_[2] = 0;
        VRML::Parser::parse_externproto( $scene, $_[2], $nt );

        ## reset position and try looking for PROTO nodes again
        pos $_[2] = $p;
        $no = $scene->get_proto($nt);
        print "PROTO? '$no'\n"
          if $VRML::verbose::parse;
    }

    if ( !defined $no ) {
        parsefail( $_[2], "Invalid node '$nt'" );
    }

    $proto = 1;
    print "Match: '$nt'\n"
      if $VRML::verbose::parse;

    $_[2] =~ /\G\s*{\s*/gsc or parsefail( $_[2], "didn't match brace!\n" );
    my $isscript = ( $nt eq "Script" );
    my %f;
    while (1) {
        while ( VRML::Parser::parse_statement( $scene, $_[2], 1 ) != -1 ) { }
        ;    # check for PROTO & co
        last if ( $_[2] =~ /\G\s*}\s*/gsc );
        print "Pos: ", ( pos $_[2] ), "\n"
          if $VRML::verbose::parse;

        # Apparently, some people use it :(
        $_[2] =~ /\G\s*,\s*/gsc
          and parsewarnstd( $_[2], "Comma not really right" );

        $_[2] =~ /\G\s*($Word)\s*/gsc
          or parsefail( $_[2], "Node body", "field name not found" );
        print "FIELD: '$1'\n"
          if $VRML::verbose::parse;

        my $f = VRML::Parser::parse_exposedField( $1, $no );


        my $ft = $no->{FieldTypes}{$f};
        print "FT: $ft\n"
          if $VRML::verbose::parse;
        if ( !defined $ft ) {
            print "Invalid field '$f' for node '$nt'\n";
            print "Possible fields are: ";
            foreach ( keys %{ $no->{FieldTypes} } ) {
                print "$_ ";
            }
            print "\n";

            exit(1);
        }

        # the following lines return something like:
        # 	Storing values... SFInt32 for node VNetInfo, f port
        #       storing type 2, port, (8888)

        print "Storing values... $ft for node $nt, f $f\n"
          if $VRML::verbose::parse;

        if ( $_[2] =~ /\G\s*IS\s+($Word)/gsc ) {
            $is_name = $1;

            # Allow multiple IS statements for a single field in a node in
            # a prototype definition.
            # Prepending digits to the field name should be safe, since legal
            # VRML names may not begin with numerical characters.
            #
            # See NIST test Misc, PROTO, #19 (30eventouts.wrl) as example.
            if ( exists $f{$f} ) {
                $rep_field = ++$field_counter . $f;
                print
"VRML::Field::SFNode::parse: an IS for $ft $f exists, try $rep_field.\n"
                  if $VRML::verbose::parse;
                $no->{FieldTypes}{$rep_field} = $no->{FieldTypes}{$f};
                $no->{FieldKinds}{$rep_field} = $no->{FieldKinds}{$f};
                $no->{Defaults}{$rep_field}   = $no->{Defaults}{$f};

                if ( exists $no->{EventIns}{$f} ) {
                    $no->{EventIns}{$rep_field} = $rep_field;
                }

                if ( exists $no->{EventOuts}{$f} ) {
                    $no->{EventOuts}{$rep_field} = $rep_field;
                }

                $f{$rep_field} = $scene->new_is( $is_name, $rep_field );
            }
            else {
                $f{$f} = $scene->new_is( $is_name, $f );
            }
            print "storing type 1, $f, (name ", VRML::Debug::toString( $f{$f} ),
              ")\n"
              if $VRML::verbose::parse;
        }
        else {
            warn $ft;

            $f{$f} = "VRML::Field::$ft"->parse( $scene, $_[2] );
            print "storing type 2, $f, (",
              VRML::NodeIntern::dump_name( $f{$f} ), ")\n"
              if $VRML::verbose::parse;
        }
    }
    print "END\n"
      if $VRML::verbose::parse;

    warn Dumper($nt);
    warn Dumper(%f);
    return $scene->new_node( $nt, \%f );
}

sub print {
    my ( $typ, $this ) = @_;
    if ( $this->{Type}{Name} eq "DEF" ) {
        print "DEF $this->{Fields}{id} ";
        $this->{Type}{Fields}{node}->print( $this->{Fields}{node} );
        return;
    }
    if ( $this->{Type}{Name} eq "USE" ) {
        print "USE $this->{Fields}{id} ";
        return;
    }
    print "$this->{Type}{Name} {";
    for ( keys %{ $this->{Fields} } ) {
        print "$_ ";
        $this->{Type}{Fields}{$_}->print( $this->{Fields}{$_} );
        print "\n";
    }
    print "}\n";
}

1;
