# $Id$
#
# Copyright (C) 1998 Tuomas J. Lukka 1999 John Stewart CRC Canada
# Portions Copyright (C) 1998 Bernhard Reiter
# DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
# See the GNU Library General Public License (file COPYING in the distribution)
# for conditions of use and redistribution.

# The C routines to render various nodes quickly
#
# Field values by subs so that generalization possible..
#
# getf(Node,"fieldname",[index,...]) returns c string to get field name.
# getaf(Node,"fieldname",n) returns comma-separated list of all the field values.
# getfn(Node,"fieldname"
#
# Of these, VP is taken into account by Transform
#
# Why so elaborate code generation?
#  - makes it easy to change structs later
#  - makes it very easy to add fast implementations for new proto'ed 
#    node types
#
#
# TODO:
#  Test indexedlineset
#
# $Log$
# Revision 1.147  2004/07/16 13:17:50  crc_canada
# SFString as a Java .class script field entry.
#
# Revision 1.146  2004/07/12 13:30:36  crc_canada
# more steps to getting frustum culling working.
#
# Revision 1.145  2004/06/25 18:19:09  crc_canada
# EXTERNPROTO geturl; Solaris changes, and general changing the way URLs are
# handled.
#
# Revision 1.144  2004/06/10 20:05:52  crc_canada
# Extrusion (with concave endcaps) bug fixed; some javascript changes.
#
# Revision 1.143  2004/05/25 18:18:51  crc_canada
# more sorting of nodes
#
# Revision 1.140  2004/05/06 14:37:21  crc_canada
# more .class changes.
#
# Revision 1.138  2004/04/20 19:20:23  crc_canada
# Alberto Dubuc cleanup; java .class work.
#
# Revision 1.137  2004/04/02 21:32:42  crc_canada
# anchor work
#
# Revision 1.136  2004/03/29 20:39:54  crc_canada
# Compile for IRIX
#
# Revision 1.135  2004/03/29 19:14:14  crc_canada
# Irix compilation fixes
#
# Revision 1.134  2004/02/25 19:09:14  crc_canada
# code cleanup.
#

# To allow faster internal representations of nodes to be calculated,
# there is the field '_change' which can be compared between the node
# and the internal rep - if different, needs to be regenerated.
#
# the rep needs to be allocated if _intern == 0.
# XXX Freeing?!!?

require 'VRMLFields.pm';
require 'VRMLNodes.pm';
require 'VRMLRend.pm';

#######################################################################
#######################################################################
#######################################################################
#
# Constants
#  VRMLFunc constants, shared amongs the Perl and C code.
# 

%Constants = (
	      VF_Viewpoint     => 0x0001,
	      VF_Geom          => 0x0002,
	      VF_Lights        => 0x0004,
	      VF_Sensitive     => 0x0008,
	      VF_Blend         => 0x0010,
	      VF_Proximity     => 0x0020,
	      VF_Collision     => 0x0040,
);



#######################################################################
#######################################################################
#######################################################################
#
# RendRay --
#  code for checking whether a ray (defined by mouse pointer)
#  intersects with the geometry of the primitive.
#
#

# Y axis rotation around an unit vector:
# alpha = angle between Y and vec, theta = rotation angle
#  1. in X plane ->
#   Y = Y - sin(alpha) * (1-cos(theta))
#   X = sin(alpha) * sin(theta)
#
#  
# How to find out the orientation from two vectors (we are allowed
# to assume no negative scales)
#  1. Y -> Y' -> around any vector on the plane midway between the 
#                two vectors
#     Z -> Z' -> around any vector ""
#
# -> intersection.
#
# The plane is the midway normal between the two vectors
# (if the two vectors are the same, it is the vector).

%RendRayC = (
Box => '
	float x = $f(size,0)/2;
	float y = $f(size,1)/2;
	float z = $f(size,2)/2;
	/* 1. x=const-plane faces? */
	if(!XEQ) {
		float xrat0 = XRAT(x);
		float xrat1 = XRAT(-x);
		if(verbose) printf("!XEQ: %f %f\\n",xrat0,xrat1);
		if(TRAT(xrat0)) {
			float cy = MRATY(xrat0);
			if(verbose) printf("TRok: %f\\n",cy);
			if(cy >= -y && cy < y) {
				float cz = MRATZ(xrat0);
				if(verbose) printf("cyok: %f\\n",cz);
				if(cz >= -z && cz < z) {
					if(verbose) printf("czok:\\n");
					rayhit(xrat0, x,cy,cz, 1,0,0, -1,-1, "cube x0");
				}
			}
		}
		if(TRAT(xrat1)) {
			float cy = MRATY(xrat1);
			if(cy >= -y && cy < y) {
				float cz = MRATZ(xrat1);
				if(cz >= -z && cz < z) {
					rayhit(xrat1, -x,cy,cz, -1,0,0, -1,-1, "cube x1");
				}
			}
		}
	}
	if(!YEQ) {
		float yrat0 = YRAT(y);
		float yrat1 = YRAT(-y);
		if(TRAT(yrat0)) {
			float cx = MRATX(yrat0);
			if(cx >= -x && cx < x) {
				float cz = MRATZ(yrat0);
				if(cz >= -z && cz < z) {
					rayhit(yrat0, cx,y,cz, 0,1,0, -1,-1, "cube y0");
				}
			}
		}
		if(TRAT(yrat1)) {
			float cx = MRATX(yrat1);
			if(cx >= -x && cx < x) {
				float cz = MRATZ(yrat1);
				if(cz >= -z && cz < z) {
					rayhit(yrat1, cx,-y,cz, 0,-1,0, -1,-1, "cube y1");
				}
			}
		}
	}
	if(!ZEQ) {
		float zrat0 = ZRAT(z);
		float zrat1 = ZRAT(-z);
		if(TRAT(zrat0)) {
			float cx = MRATX(zrat0);
			if(cx >= -x && cx < x) {
				float cy = MRATY(zrat0);
				if(cy >= -y && cy < y) {
					rayhit(zrat0, cx,cy,z, 0,0,1, -1,-1,"cube z0");
				}
			}
		}
		if(TRAT(zrat1)) {
			float cx = MRATX(zrat1);
			if(cx >= -x && cx < x) {
				float cy = MRATY(zrat1);
				if(cy >= -y && cy < y) {
					rayhit(zrat1, cx,cy,-z, 0,0,-1,  -1,-1,"cube z1");
				}
			}
		}
	}
',

# Distance to zero as function of ratio is
# sqrt(
#	((1-r)t_r1.x + r t_r2.x)**2 +
#	((1-r)t_r1.y + r t_r2.y)**2 +
#	((1-r)t_r1.z + r t_r2.z)**2
# ) == radius
# Therefore,
# radius ** 2 == ... ** 2 
# and 
# radius ** 2 = 
# 	(1-r)**2 * (t_r1.x**2 + t_r1.y**2 + t_r1.z**2) +
#       2*(r*(1-r)) * (t_r1.x*t_r2.x + t_r1.y*t_r2.y + t_r1.z*t_r2.z) +
#       r**2 (t_r2.x**2 ...)
# Let's name tr1sq, tr2sq, tr1tr2 and then we have
# radius ** 2 =  (1-r)**2 * tr1sq + 2 * r * (1-r) tr1tr2 + r**2 tr2sq
# = (tr1sq - 2*tr1tr2 + tr2sq) r**2 + 2 * r * (tr1tr2 - tr1sq) + tr1sq
# 
# I.e.
# 
# (tr1sq - 2*tr1tr2 + tr2sq) r**2 + 2 * r * (tr1tr2 - tr1sq) + 
#	(tr1sq - radius**2) == 0
#
# I.e. second degree eq. a r**2 + b r + c == 0 where
#  a = tr1sq - 2*tr1tr2 + tr2sq
#  b = 2*(tr1tr2 - tr1sq)
#  c = (tr1sq-radius**2)
# 
# 
Sphere => '
	float r = $f(radius);
	/* Center is at zero. t_r1 to t_r2 and t_r1 to zero are the vecs */
	float tr1sq = VECSQ(t_r1);
	float tr2sq = VECSQ(t_r2);
	float tr1tr2 = VECPT(t_r1,t_r2);
	struct pt dr2r1;
	float dlen;
	float a,b,c,disc;

	VECDIFF(t_r2,t_r1,dr2r1);
	dlen = VECSQ(dr2r1);

	a = dlen; /* tr1sq - 2*tr1tr2 + tr2sq; */
	b = 2*(VECPT(dr2r1, t_r1));
	c = tr1sq - r*r;

	disc = b*b - 4*a*c; /* The discriminant */
	
	if(disc > 0) { /* HITS */
		float q ;
		float sol1 ;
		float sol2 ;
		float cx,cy,cz;
		q = sqrt(disc);
		/* q = (-b+(b>0)?q:-q)/2; */
		sol1 = (-b+q)/(2*a);
		sol2 = (-b-q)/(2*a);
		/*
		printf("SPHSOL0: (%f %f %f) (%f %f %f)\\n",
			t_r1.x, t_r1.y, t_r1.z, t_r2.x, t_r2.y, t_r2.z);
		printf("SPHSOL: (%f %f %f) (%f) (%f %f) (%f) (%f %f)\\n",
			tr1sq, tr2sq, tr1tr2, a, b, c, und, sol1, sol2);
		*/
		cx = MRATX(sol1);
		cy = MRATY(sol1);
		cz = MRATZ(sol1);
		rayhit(sol1, cx,cy,cz, cx/r,cy/r,cz/r, -1,-1, "sphere 0");
		cx = MRATX(sol2);
		cy = MRATY(sol2);
		cz = MRATZ(sol2);
		rayhit(sol2, cx,cy,cz, cx/r,cy/r,cz/r, -1,-1, "sphere 1");
	}
',

# Cylinder: first test the caps, then against infinite cylinder.
Cylinder => '
	float h = $f(height)/2; /* pos and neg dir. */
	float r = $f(radius);
	float y = h;
	/* Caps */
	if(!YEQ) {
		float yrat0 = YRAT(y);
		float yrat1 = YRAT(-y);
		if(TRAT(yrat0)) {
			float cx = MRATX(yrat0);
			float cz = MRATZ(yrat0);
			if(r*r > cx*cx+cz*cz) {
				rayhit(yrat0, cx,y,cz, 0,1,0, -1,-1, "cylcap 0");
			}
		}
		if(TRAT(yrat1)) {
			float cx = MRATX(yrat1);
			float cz = MRATZ(yrat1);
			if(r*r > cx*cx+cz*cz) {
				rayhit(yrat1, cx,-y,cz, 0,-1,0, -1,-1, "cylcap 1");
			}
		}
	}
	/* Body -- do same as for sphere, except no y axis in distance */
	if((!XEQ) && (!ZEQ)) {
		float dx = t_r2.x-t_r1.x; float dz = t_r2.z-t_r1.z;
		float a = dx*dx + dz*dz;
		float b = 2*(dx * t_r1.x + dz * t_r1.z);
		float c = t_r1.x * t_r1.x + t_r1.z * t_r1.z - r*r;
		float und;
		b /= a; c /= a;
		und = b*b - 4*c;
		if(und > 0) { /* HITS the infinite cylinder */
			float sol1 = (-b+sqrt(und))/2;
			float sol2 = (-b-sqrt(und))/2;
			float cy,cx,cz;
			cy = MRATY(sol1);
			if(cy > -h && cy < h) {
				cx = MRATX(sol1);
				cz = MRATZ(sol1);
				rayhit(sol1, cx,cy,cz, cx/r,0,cz/r, -1,-1, "cylside 1");
			}
			cy = MRATY(sol2);
			if(cy > -h && cy < h) {
				cx = MRATX(sol2);
				cz = MRATZ(sol2);
				rayhit(sol2, cx,cy,cz, cx/r,0,cz/r, -1,-1, "cylside 2");
			}
		}
	}
',

# For cone, this is most difficult. We have
# sqrt(
#	((1-r)t_r1.x + r t_r2.x)**2 +
#	((1-r)t_r1.z + r t_r2.z)**2
# ) == radius*( -( (1-r)t_r1.y + r t_r2.y )/(2*h)+0.5)
# == radius * ( -( r*(t_r2.y - t_r1.y) + t_r1.y )/(2*h)+0.5)
# == radius * ( -r*(t_r2.y-t_r1.y)/(2*h) + 0.5 - t_r1.y/(2*h))

#
# Other side: r*r*(
Cone => '
	float h = $f(height)/2; /* pos and neg dir. */
	float y = h;
	float r = $f(bottomRadius);
	float dx = t_r2.x-t_r1.x; float dz = t_r2.z-t_r1.z;
	float dy = t_r2.y-t_r1.y;
	float a = dx*dx + dz*dz - (r*r*dy*dy/(2*h*2*h));
	float b = 2*(dx*t_r1.x + dz*t_r1.z) +
		2*r*r*dy/(2*h)*(0.5-t_r1.y/(2*h));
	float tmp = (0.5-t_r1.y/(2*h));
	float c = t_r1.x * t_r1.x + t_r1.z * t_r1.z 
		- r*r*tmp*tmp;
	float und;
	b /= a; c /= a;
	und = b*b - 4*c;
	/* 
	printf("CONSOL0: (%f %f %f) (%f %f %f)\\n",
		t_r1.x, t_r1.y, t_r1.z, t_r2.x, t_r2.y, t_r2.z);
	printf("CONSOL: (%f %f %f) (%f) (%f %f) (%f)\\n",
		dx, dy, dz, a, b, c, und);
	*/
	if(und > 0) { /* HITS the infinite cylinder */
		float sol1 = (-b+sqrt(und))/2;
		float sol2 = (-b-sqrt(und))/2;
		float cy,cx,cz;
		float cy0;
		cy = MRATY(sol1);
		if(cy > -h && cy < h) {
			cx = MRATX(sol1);
			cz = MRATZ(sol1);
			/* XXX Normal */
			rayhit(sol1, cx,cy,cz, cx/r,0,cz/r, -1,-1, "conside 1");
		}
		cy0 = cy;
		cy = MRATY(sol2);
		if(cy > -h && cy < h) {
			cx = MRATX(sol2);
			cz = MRATZ(sol2);
			rayhit(sol2, cx,cy,cz, cx/r,0,cz/r, -1,-1, "conside 2");
		}
		/*
		printf("CONSOLV: (%f %f) (%f %f)\\n", sol1, sol2,cy0,cy);
		*/
	}
	if(!YEQ) {
		float yrat0 = YRAT(-y);
		if(TRAT(yrat0)) {
			float cx = MRATX(yrat0);
			float cz = MRATZ(yrat0);
			if(r*r > cx*cx + cz*cz) {
				rayhit(yrat0, cx, -y, cz, 0, -1, 0, -1, -1, "conbot");
			}
		}
	}
',

GeoElevationGrid => ( '
		$mk_polyrep();
		render_ray_polyrep(this_, 0, NULL);
'),

ElevationGrid => ( '
		printf ("ELEV VRMLC before render_ray_polyrep\n");
		$mk_polyrep();
		render_ray_polyrep(this_, 0, NULL);
		printf ("ELEV VRMLC after render_ray_polyrep\n");
'),

Text => ( '
		$mk_polyrep();
		render_ray_polyrep(this_, 0, NULL);
'),

Extrusion => ( '
		$mk_polyrep();
		render_ray_polyrep(this_, 0, NULL);
'),

IndexedFaceSet => '
		struct SFColor *points; int npoints;
		$fv(coord, points, get3, &npoints);
		$mk_polyrep();
		render_ray_polyrep(this_, 
			npoints, points
		);
',

);

#####################################################################3
#####################################################################3
#####################################################################3
#
# GenPolyRep
#  code for generating internal polygonal representations
#  of some nodes (ElevationGrid, Text, Extrusion and IndexedFaceSet)
#
#

%GenPolyRepC = (
	ElevationGrid => 'make_elevationgrid(this_);',
	Extrusion => 'make_extrusion(this_);',
	IndexedFaceSet => 'make_indexedfaceset(this_);',
	Text => 'make_text(this_);',
	GeoElevationGrid => (do "VRMLGeoElevationGrid.pm"),
);

######################################################################
######################################################################
######################################################################
#
# Get3
#  get a coordinate / color / normal array from the node.
#

%Get3C = (
Coordinate => '
	*n = $f_n(point); 
	return $f(point);
',
Color => '
	*n = $f_n(color); 
	return $f(color);
',
Normal => '
	*n = $f_n(vector);
	return $f(vector);
'
);

%Get2C = (
TextureCoordinate => '
	*n = $f_n(point);
	return $f(point);
',
);

######################################################################
######################################################################
######################################################################
#
# Generation
#  Functions for generating the code
#


{
	my %AllNodes = (%RendC, %RendRayC, %PrepC, %FinC, %ChildC, %Get3C, %Get2C, %LightC,
		%ChangedC, %ProximityC, %CollisionC);
	@NodeTypes = keys %AllNodes;
}

sub assgn_m {
	my($f, $l) = @_;
	return ((join '',map {"m[$_] = ".getf(Material, $f, $_).";"} 0..2).
		"m[3] = $l;");
}

# XXX Might we need separate fields for separate structs?
sub getf {
	my ($t, $f, @l) = @_;
	my $type = $VRML::Nodes{$t}{FieldTypes}{$f};
	if($type eq "") {
		die("Invalid type $t $f '$type'");
	}
	return "VRML::Field::$type"->cget("(this_->$f)",@l);
}

sub getfn {
	my($t, $f) = @_;
	my $type = $VRML::Nodes{$t}{FieldTypes}{$f};
	return "VRML::Field::$type"->cgetn("(this_->$f)");
}

# XXX Code copy :(
sub fvirt {
	my($t, $f, $ret, $v, @a) = @_;
	# Die if not exists
	my $type = $VRML::Nodes{$t}{FieldTypes}{$f};
	if($type ne "SFNode") {
		die("Fvirt must have SFNode");
	}
	if($ret) {$ret = "$ret = ";}
	return "if(this_->$f) {
		  if(!(*(struct VRML_Virt **)(this_->$f))->$v) {
		  	freewrlDie(\"NULL METHOD $t $f $v\");
		  }
		  $ret ((*(struct VRML_Virt **)(this_->$f))->$v(this_->$f,
		    ".(join ',',@a).")) ;}
 	  else { (freewrlDie(\"NULL FIELD $t $f $a\"));}";
}

sub fvirt_null {
	my($t, $f, $ret, $v, @a) = @_;
	# Die if not exists
	my $type = $VRML::Nodes{$t}{FieldTypes}{$f};
	if($type ne "SFNode") {
		die("Fvirt must have SFNode");
	}
	if($ret) {$ret = "$ret = ";}
	return "if(this_->$f) {
		  if(!(*(struct VRML_Virt **)(this_->$f))->$v) {
		  	freewrlDie(\"NULL METHOD $t $f $v\");
		  }
		  $ret ((*(struct VRML_Virt **)(this_->$f))->$v(this_->$f,
		    ".(join ',',@a).")) ;
		}";
}


sub fgetfnvirt_n {
	my($n, $ret, $v, @a) = @_;
	if($ret) {$ret = "$ret = ";}
	return "if($n) {
	         if(!(*(struct VRML_Virt **)n)->$v) {
		  	freewrlDie(\"NULL METHOD $n $ret $v\");
		 }
		 $ret ((*(struct VRML_Virt **)($n))->$v($n,
		    ".(join ',',@a).")) ;}
	"
}

sub rend_geom {
	return $_[0];
}

################################################################
#
# gen_struct - Generate a node structure, adding fields for
# internal use

sub gen_struct {
	my($name,$node) = @_;
	my @f = keys %{$node->{FieldTypes}};
	my $nf = scalar @f;
	# /* Store actual point etc. later */
       my $s = "struct VRML_$name {\n" .
               " /***/ struct VRML_Virt *v;\n"         	.
               " /*s*/ int _sens; \n"                  	.
               " /*t*/ int _hit; \n"                   	.
               " /*a*/ int _change; \n"                	.
	       " /*n*/ int _dlchange; \n"              	.
               " /*d*/ GLuint _dlist; \n"              	.
	       "       void **_parents; \n"	  	.
	       "       int _nparents; \n"		.
	       "       int _nparalloc; \n"		.
	       "       int _ichange; \n"		.
	       "       float _dist; /*sorting for blending */ \n".
	       "	float _extent[3]; /* used for boundingboxes */ \n" .
               " /*d*/ void *_intern; \n"              	.
               " /***/\n";
	
	my $o = "
void *
get_${name}_offsets(p)
	SV *p;
CODE:
	int *ptr_;
	int xx;
	SvGROW(p,($nf+1)*sizeof(int));
	SvCUR_set(p,($nf+1)*sizeof(int));
	ptr_ = (int *)SvPV(p,xx);
";
	my $p = " {
		my \$s = '';
		my \$v = get_${name}_offsets(\$s);
		\@{\$n->{$name}{Offs}}{".(join ',',map {"\"$_\""} @f,'_end_')."} =
			unpack(\"i*\",\$s);
		\$n->{$name}{Virt} = \$v;
 }
	";
	for(@f) {
		my $cty = "VRML::Field::$node->{FieldTypes}{$_}"->ctype($_);
		$s .= "\t$cty;\n";
		$o .= "\t*ptr_++ = offsetof(struct VRML_$name, $_);\n";
	}
	$o .= "\t*ptr_++ = sizeof(struct VRML_$name);\n";
	$o .= "RETVAL=&(virt_${name});
	if(verbose) printf(\"$name virtual: %d \\n \", RETVAL);
OUTPUT:
	RETVAL
";
	$s .= $ExtraMem{$name};
	$s .= "};\n";
	return ($s,$o,$p);
}

#########################################################
sub get_offsf {
	my($f) = @_;
	my ($ct) = ("VRML::Field::$_")->ctype("*ptr_");
	my ($ctp) = ("VRML::Field::$_")->ctype("*");
	my ($c) = ("VRML::Field::$_")->cfunc("(*ptr_)", "sv_");
	my ($ca) = ("VRML::Field::$_")->calloc("(*ptr_)");
	my ($cf) = ("VRML::Field::$_")->cfree("(*ptr_)");
	return "

void 
set_offs_$f(ptr,offs,sv_)
	void *ptr
	int offs
	SV *sv_
CODE:
	$ct = ($ctp)(((char *)ptr)+offs);
	update_node(ptr);
	$c


void 
alloc_offs_$f(ptr,offs)
	void *ptr
	int offs
CODE:
	$ct = ($ctp)(((char *)ptr)+offs);
	$ca

void
free_offs_$f(ptr,offs)
	void *ptr
	int offs
CODE:
	$ct = ($ctp)(((char *)ptr)+offs);
	$cf

"
}
#######################################################

sub get_rendfunc {
	my($n) = @_;
	print "RENDF $n ";
	# XXX
	my @f = qw/Prep Rend Child Fin RendRay GenPolyRep Light Get3 Get2
		Changed Proximity Collision/;
	my $f;
	my $v = "
static struct VRML_Virt virt_${n} = { ".
	(join ',',map {${$_."C"}{$n} ? "${n}_$_" : "NULL"} @f).
",\"$n\"};";
	for(@f) {
		my $c =${$_."C"}{$n};
		next if !defined $c;
		print "$_ (",length($c),") ";
		# Substitute field gets

		$c =~ s/\$f\(([^)]*)\)/getf($n,split ',',$1)/ge;
		$c =~ s/\$i\(([^)]*)\)/(this_->$1)/g;
		$c =~ s/\$f_n\(([^)]*)\)/getfn($n,split ',',$1)/ge;
		$c =~ s/\$fv\(([^)]*)\)/fvirt($n,split ',',$1)/ge;
		$c =~ s/\$fv_null\(([^)]*)\)/fvirt_null($n,split ',',$1)/ge;
		$c =~ s/\$mk_polyrep\(\)/if(!this_->_intern || 
			this_->_change != ((struct VRML_PolyRep *)this_->_intern)->_change)
				regen_polyrep(this_);/g;
		if($_ eq "Get3") {
			$f .= "\n\nstruct SFColor *${n}_$_(void *nod_,int *n)";
		} elsif($_ eq "Get2") {
			$f .= "\n\nstruct SFVec2f *${n}_$_(void *nod_,int *n)";
		} else {
			$f .= "\n\nvoid ${n}_$_(void *nod_)";
		}
		$f .= "{ /* GENERATED FROM HASH ${_}C, MEMBER $n */
			struct VRML_$n *this_ = (struct VRML_$n *)nod_;
			{$c}
			}";
	}
	print "\n";
	return ($f,$v);
}


######################################################################
######################################################################
######################################################################
#
# gen_constants_xxx - generate constants and values in perl and C.
#
#

sub gen_constants_pm_header {

    my @k = keys %Constants;

    return "\@EXPORT = qw( @k );\n";
    
}

sub gen_constants_pm_body {
    my $code = "";

    $code .= "\n# VRMLFunc constants. Generated by gen_constants_pm_body in VRMLC.pm \n";
    while(($var,$value) = each(%Constants)) {
	$code .= "sub $var () { $value } \n";
    }
    $code .= "# End of autogenerated constants. \n\n";

    return $code;
}

sub gen_constants_c {

    my $code = "";
    $code .= "\n/* VRMLFunc constants. Generated by gen_constants_c in VRMLC.pm */\n";
    while(($var,$value) = each(%Constants)) {
	$code .= "#define $var $value\n";
    }
    $code .= "/* End of autogenerated constants. */\n\n";

    return $code;
}

######################################################################
######################################################################
######################################################################
#
# gen - the main function. this contains much verbatim code
#
#

sub gen {
	for(@VRML::Fields) {
		push @str, ("VRML::Field::$_")->cstruct . "\n";
		push @xsfn, get_offsf($_);
	}
        push @str, "\n/* and now the structs for the nodetypes */ \n";
	for(@NodeTypes) {
		my $no = $VRML::Nodes{$_}; 
		my($str, $offs, $perl) = gen_struct($_, $no);
		push @str, $str;
		push @xsfn, $offs;
		push @poffsfn, $perl;
		my($f, $vstru) = get_rendfunc($_);
		push @func, $f;
		push @vstruc, $vstru;
	}

	# print out structures to a file
	open STRUCTS, ">CFuncs/Structs.h";
	print STRUCTS '
/* Structs.h generated by VRMLC.pm. DO NOT MODIFY, MODIFY VRMLC.pm INSTEAD */

/* Code here comes almost verbatim from VRMLC.pm */

#ifndef STRUCTSH
#define STRUCTSH

#include "EXTERN.h"
#include "perl.h"
#ifdef AQUA
#include <gl.h>
#else
#include "GL/gl.h"
#endif

/* for time tick calculations */
#include <sys/time.h>

struct pt {GLdouble x,y,z;};
struct orient {GLdouble x,y,z,a;};

struct VRML_Virt {
	void (*prep)(void *);
	void (*rend)(void *); 
	void (*children)(void *);
	void (*fin)(void *);
	void (*rendray)(void *);
	void (*mkpolyrep)(void *);
	void (*light)(void *);
	/* And get float coordinates : Coordinate, Color */
	/* XXX Relies on MFColor repr.. */
	struct SFColor *(*get3)(void *, int *); /* Number in int */
	struct SFVec2f *(*get2)(void *, int *); /* Number in int */
	void (*changed)(void *);
	void (*proximity)(void *);
	void (*collision)(void *);
	char *name;
};

/* Internal representation of IndexedFaceSet, Text, Extrusion & ElevationGrid:
 * set of triangles.
 * done so that we get rid of concave polygons etc.
 */
struct VRML_PolyRep { /* Currently a bit wasteful, because copying */
	int _change;
	int ccw;	/* ccw field for single faced structures */
	int ntri; /* number of triangles */
	int alloc_tri; /* number of allocated triangles */
	int *cindex;   /* triples (per triangle) */
	float *coord; /* triples (per point) */
	int *colindex;   /* triples (per triangle) */
	float *color; /* triples or null */
	int *norindex;
	float *normal; /* triples or null */
        int *tcindex; /* triples or null */
        float *tcoord;	/* triples (per triangle) of texture coords */
};

/* viewer dimentions (for collision detection) */
struct sNaviInfo {
        double width;
        double height;
        double step;
};

';



	# print out the generated structures
	print STRUCTS join '',@str;

	print STRUCTS '
#endif /* ifndef */
';
	

	open CST, ">CFuncs/constants.h";
	print CST  &gen_constants_c();
	close CST;


	open XS, ">VRMLFunc.xs";
	print XS '
/* VRMLFunc.c generated by VRMLC.pm. DO NOT MODIFY, MODIFY VRMLC.pm INSTEAD */

/* Code here comes almost verbatim from VRMLC.pm */

#include "CFuncs/headers.h"
#include "XSUB.h"

#include <math.h>

#ifndef AQUA
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glx.h>
#else
#include <gl.h>
#include <glu.h>
#include <glext.h>
#endif

#include "CFuncs/Viewer.h"
#include "CFuncs/OpenGL_Utils.h"
#include "CFuncs/Collision.h"
#include "CFuncs/Bindable.h"
#include "CFuncs/Textures.h"
#include "CFuncs/Polyrep.h"
#include "CFuncs/sounds.h"
#include "CFuncs/SensInterps.h"


/* Rearrange to take advantage of headlight when off */
int curlight = 0;
int nlightcodes = 7;
int lightcode[7] = {
	GL_LIGHT1,
	GL_LIGHT2,
	GL_LIGHT3,
	GL_LIGHT4,
	GL_LIGHT5,
	GL_LIGHT6,
	GL_LIGHT7,
};
int nextlight() {
	if(curlight == nlightcodes) { return -1; }
	return lightcode[curlight++];
}


/* material node usage depends on texture depth; if rgb (depth1) we blend color field
   and diffusecolor with texture, else, we dont bother with material colors */
int last_texture_depth = 0;
float last_transparency = 0.0;

/* Sounds can come from AudioClip nodes, or from MovieTexture nodes. Different
   structures on these */
int sound_from_audioclip = 0;

/* and, we allow a maximum of so many pixels per texture */
/* if this is zero, first time a texture call is made, this is set to the OpenGL implementations max */
GLint global_texSize = 0;

/* for printing warnings about Sound node problems - only print once per invocation */
int soundWarned = FALSE;

int verbose;
int verbose_collision; /*print out collision info*/

int render_vp; /*set up the inverse viewmatrix of the viewpoint.*/
int render_geom;
int render_light;
int render_sensitive;
int render_blend;
int render_proximity;
int render_collision;

int display_status = 1;  /* display a status bar? */
int be_collision = 1;	/* do collision detection? */

int found_vp; /*true when viewpoint found*/

GLuint last_bound_texture;
int	have_transparency;	/* did this Shape have transparent material? */

int smooth_normals = -1; /* -1 means, uninitialized */

int cur_hits=0;

/* Collision detection results */
struct sCollisionInfo CollisionInfo = { {0,0,0} , 0, 0. };

/* Displacement of viewer , used for colision calculation  PROTYPE, CURRENTLY UNUSED*/
struct pt ViewerDelta = {0,0,0};

/* dimentions of viewer, and "up" vector (for collision detection) */
struct sNaviInfo naviinfo = {0.25, 1.6, 0.75};

/* for alignment of collision cylinder, and gravity (later). */
struct pt ViewerUpvector = {0,0,0};

VRML_Viewer Viewer;


/* These two points define a ray in window coordinates */

struct pt r1 = {0,0,-1},r2 = {0,0,0},r3 = {0,1,0};
struct pt t_r1,t_r2,t_r3; /* transformed ray */
void *hypersensitive = 0; int hyperhit = 0;
struct pt hyper_r1,hyper_r2; /* Transformed ray for the hypersensitive node */

GLint viewport[4] = {-1,-1,2,2};

/* These three points define 1. hitpoint 2., 3. two different tangents
 * of the surface at hitpoint (to get transformation correctly */

/* All in window coordinates */

struct pt hp, ht1, ht2;
double hpdist; /* distance in ray: 0 = r1, 1 = r2, 2 = 2*r2-r1... */

/* used to save rayhit and hyperhit for later use by C functions */
struct SFColor hyp_save_posn, hyp_save_norm, ray_save_posn;

/* Any action for the Browser (perl code) to do? */
int BrowserAction = FALSE;
struct Multi_String Anchor_url;


struct currayhit  rh,rph,rhhyper;
/* used to test new hits */

/* this is used to return the duration of an audioclip to the perl
side of things. SvPV et al. works, but need to figure out all
references, etc. to bypass this fudge JAS */
float AC_LastDuration[50]  = {-1.0,-1.0,-1.0,-1.0,-1.0,
				-1.0,-1.0,-1.0,-1.0,-1.0,
				-1.0,-1.0,-1.0,-1.0,-1.0,
				-1.0,-1.0,-1.0,-1.0,-1.0,
				-1.0,-1.0,-1.0,-1.0,-1.0,
				-1.0,-1.0,-1.0,-1.0,-1.0,
				-1.0,-1.0,-1.0,-1.0,-1.0,
				-1.0,-1.0,-1.0,-1.0,-1.0,
				-1.0,-1.0,-1.0,-1.0,-1.0,
				-1.0,-1.0,-1.0,-1.0,-1.0} ;

/* is the sound engine started yet? */
int SoundEngineStarted = FALSE;

/* stored FreeWRL version, pointers to initialize data */
char *BrowserVersion = NULL;
char *BrowserURL = NULL;
char *BrowserName = "FreeWRL VRML/X3D Browser";

int rootNode=0;	// scene graph root node

/*******************************************************************************/

/* Sub, rather than big macro... */
void rayhit(float rat, float cx,float cy,float cz, float nx,float ny,float nz, 
float tx,float ty, char *descr)  {
	GLdouble modelMatrix[16];
	GLdouble projMatrix[16];
	GLdouble wx, wy, wz;
	/* Real rat-testing */
	if(verbose) 
		printf("RAY HIT %s! %f (%f %f %f) (%f %f %f)\nR: (%f %f %f) (%f %f %f)\n",
		descr, rat,cx,cy,cz,nx,ny,nz,
		t_r1.x, t_r1.y, t_r1.z,
		t_r2.x, t_r2.y, t_r2.z
		);
	if(rat<0 || (rat>hpdist && hpdist >= 0)) {
		return;
	}
	glGetDoublev(GL_MODELVIEW_MATRIX, modelMatrix);
	glGetDoublev(GL_PROJECTION_MATRIX, projMatrix);
	gluProject(cx,cy,cz, modelMatrix, projMatrix, viewport,
		&hp.x, &hp.y, &hp.z);
	hpdist = rat;
	rh=rph;
	rhhyper=rph;
}

/* Call this when modelview and projection modified */
void upd_ray() {
	GLdouble modelMatrix[16];
	GLdouble projMatrix[16];
	glGetDoublev(GL_MODELVIEW_MATRIX, modelMatrix);
	glGetDoublev(GL_PROJECTION_MATRIX, projMatrix);
	gluUnProject(r1.x,r1.y,r1.z,modelMatrix,projMatrix,viewport,
		&t_r1.x,&t_r1.y,&t_r1.z);
	gluUnProject(r2.x,r2.y,r2.z,modelMatrix,projMatrix,viewport,
		&t_r2.x,&t_r2.y,&t_r2.z);
	gluUnProject(r3.x,r3.y,r3.z,modelMatrix,projMatrix,viewport,
		&t_r3.x,&t_r3.y,&t_r3.z);
/*	printf("Upd_ray: (%f %f %f)->(%f %f %f) == (%f %f %f)->(%f %f %f)\n",
		r1.x,r1.y,r1.z,r2.x,r2.y,r2.z,
		t_r1.x,t_r1.y,t_r1.z,t_r2.x,t_r2.y,t_r2.z);
*/
}


/* if a node changes, void the display lists */
/* Courtesy of Jochen Hoenicke */

void update_node(void *ptr) {
	struct VRML_Box *p = ptr;
	int i;
	p->_change ++;
	for (i = 0; i < p->_nparents; i++) {
		update_node(p->_parents[i]);
	}
}

/*explicit declaration. Needed for Collision_Child*/
void Group_Child(void *nod_);

/*********************************************************************
 * Code here is generated from the hashes in VRMLC.pm and VRMLRend.pm
 */
	';

#######################################################j


	print XS join '',@func;
	print XS join '',@vstruc;
#######################################################
	print XS <<'ENDHERE'

/*********************************************************************
 * Code here again comes almost verbatim from VRMLC.pm 
 */

/*********************************************************************
 *********************************************************************
 *
 * render_node : call the correct virtual functions to render the node
 * depending on what we are doing right now.
 */

void render_node(void *node) {
	struct VRML_Virt *v;
	struct VRML_Box *p;
	int srg;
	int sch;
	struct currayhit srh;
	int glerror = GL_NONE;
	char* stage = "";

	if(verbose) printf("\nRender_node %u\n",(unsigned int) node);
	if(!node) {return;}
	v = *(struct VRML_Virt **)node;
	p = node;

	if(verbose) {
	    printf("=========================================NODE RENDERED===================================================\n");
	    printf("Render_node_v %d (%s) PREP: %d REND: %d CH: %d FIN: %d RAY: %d HYP: %d\n",v,
		   v->name, 
		   v->prep, 
		   v->rend, 
		   v->children, 
		   v->fin, 
		   v->rendray,
		   hypersensitive);
	    printf("Render_state geom %d light %d sens %d\n",
		   render_geom, 
		   render_light, 
		   render_sensitive);
	    printf ("pchange %d pichange %d vchanged %d\n",p->_change, p->_ichange,v->changed);
	}

        /* we found viewpoint on render_vp pass, stop exploring tree.. */
        if(render_vp && found_vp) return;

	if(p->_change != p->_ichange && v->changed) 
	  {
	    if (verbose) printf ("rs 1 pch %d pich %d vch %d\n",p->_change,p->_ichange,v->changed);
	    v->changed(node);
	    p->_ichange = p->_change;
	    if(glerror == GL_NONE && ((glerror = glGetError()) != GL_NONE) ) stage = "change";
	  }

	if(v->prep) 
	  {
	    if (verbose) printf ("rs 2\n");
	    v->prep(node);
	    if(render_sensitive && !hypersensitive) 
	      {
		upd_ray();
	      }
	    if(glerror == GL_NONE && ((glerror = glGetError()) != GL_NONE) ) stage = "prep";
	  }

	if(render_proximity && v->proximity) 
	{
	    if (verbose) printf ("rs 2a\n");
	    v->proximity(node);
	    if(glerror == GL_NONE && ((glerror = glGetError()) != GL_NONE) ) stage = "render_proximity";
	}

	if(render_collision && v->collision) 
	{
	    if (verbose) printf ("rs 2b\n");
	    v->collision(node);
	    if(glerror == GL_NONE && ((glerror = glGetError()) != GL_NONE) ) stage = "render_collision";
	}

	if(render_geom && !render_sensitive && v->rend) 
	  {
	    if (verbose) printf ("rs 3\n");
	    v->rend(node);
	    if(glerror == GL_NONE && ((glerror = glGetError()) != GL_NONE) ) stage = "render_geom";
	  }
	if(render_light && v->light) 
	  {
	    if (verbose) printf ("rs 4\n");
	    v->light(node);
	    if(glerror == GL_NONE && ((glerror = glGetError()) != GL_NONE) ) stage = "render_light";
	  }
	/* Future optimization: when doing VP/Lights, do only 
	 * that child... further in future: could just calculate
	 * transforms myself..
	 */
	if(render_sensitive && p->_sens) 
	  {
	    if (verbose) printf ("rs 5\n");
	    srg = render_geom;
	    render_geom = 1;
	    if(verbose) printf("CH1 %d: %d\n",node, cur_hits, p->_hit);
	    sch = cur_hits;
	    cur_hits = 0;
	    /* HP */
	      srh = rph;
	    rph.node = node;
	    glGetDoublev(GL_MODELVIEW_MATRIX, rph.modelMatrix);
	    glGetDoublev(GL_PROJECTION_MATRIX, rph.projMatrix);
	    if(glerror == GL_NONE && ((glerror = glGetError()) != GL_NONE) ) stage = "render_sensitive";

	  }
	if(render_geom && render_sensitive && !hypersensitive && v->rendray) 
	  {
	    if (verbose) printf ("rs 6\n");
	    v->rendray(node);
	    if(glerror == GL_NONE && ((glerror = glGetError()) != GL_NONE) ) stage = "rs 6";
	  }
        

        if((render_sensitive) && (hypersensitive == node)) {
            if (verbose) printf ("rs 7\n");
            hyper_r1 = t_r1;
            hyper_r2 = t_r2;
            hyperhit = 1;
        }
        if(v->children) {
            if (verbose) printf ("rs 8\n");
            v->children(node);
	    if(glerror == GL_NONE && ((glerror = glGetError()) != GL_NONE) ) stage = "children";
        }

	if(render_sensitive && p->_sens) 
	  {
	    if (verbose) printf ("rs 9\n");
	    render_geom = srg;
	    cur_hits = sch;
	    if(verbose) printf("CH3: %d %d\n",cur_hits, p->_hit);
	    /* HP */
	      rph = srh;
	  }
	if(v->fin) 
	  {
	    if (verbose) printf ("rs A\n");
	    v->fin(node);
	    if(render_sensitive && v == &virt_Transform) 
	      { 
		upd_ray();
	      }
	    if(glerror != GL_NONE && ((glerror = glGetError()) != GL_NONE) ) stage = "fin";
	  }
	if (verbose) printf("(end render_node)\n");

	if(glerror != GL_NONE)
	  {
	    printf("============== GLERROR : %s in stage %s =============\n",gluErrorString(glerror),stage);
	    printf("Render_node_v %d (%s) PREP: %d REND: %d CH: %d FIN: %d RAY: %d HYP: %d\n",v,
		   v->name, 
		   v->prep, 
		   v->rend, 
		   v->children, 
		   v->fin, 
		   v->rendray,
		   hypersensitive);
	    printf("Render_state geom %d light %d sens %d\n",
		   render_geom, 
		   render_light, 
		   render_sensitive);
	    printf ("pchange %d pichange %d vchanged %d\n",p->_change, p->_ichange,v->changed);
	    printf("==============\n");
	  }
}

/*
 * The following code handles keeping track of the parents of a given
 * node. This enables us to traverse the scene on C level for optimizations.
 *
 * We use this array code because VRML nodes usually don't have
 * hundreds of children and don't usually shuffle them too much.
 */

#define NODE_ADD_PARENT(a) add_parent(a,ptr)

void add_parent(void *node_, void *parent_) {
	struct VRML_Box *node = node_;
	struct VRML_Box *parent = parent_;
	if(!node) return;
	//printf ("adding node %d to parent %d\n",node_, parent_);

	node->_nparents ++;
	if(node->_nparents > node->_nparalloc) {
		node->_nparalloc += 10;
		node->_parents = 
		 (node->_parents ? 
			realloc(node->_parents, sizeof(node->_parents[0])*
							node->_nparalloc) 
							:
			malloc(sizeof(node->_parents[0])* node->_nparalloc) 
		 );
	}
	node->_parents[node->_nparents-1] = parent_;
}

#define NODE_REMOVE_PARENT(a) add_parent(a,ptr)

void remove_parent(void *node_, void *parent_) {
	struct VRML_Box *node = node_;
	struct VRML_Box *parent = parent_;
	int i;
	if(!node) return;
	node->_nparents --;
	for(i=0; i<node->_nparents; i++) {
		if(node->_parents[i] == parent) {
			break;
		}
	}
	for(; i<node->_nparents; i++) {
		node->_parents[i] = node->_parents[i+1];
	}
}

void
render_hier(void *p, int rwhat)
{
	struct pt upvec = {0,1,0};
	GLdouble modelMatrix[16];

	render_vp = rwhat & VF_Viewpoint;
	found_vp = 0;
	render_geom =  rwhat & VF_Geom;
	render_light = rwhat & VF_Lights;
	render_sensitive = rwhat & VF_Sensitive;
	render_blend = rwhat & VF_Blend;
	render_proximity = rwhat & VF_Proximity;
	render_collision = rwhat & VF_Collision;
	curlight = 0;
	hpdist = -1;

	//printf ("render_hier vp %d geom %d light %d sens %d blend %d prox %d col %d\n",
	//render_vp,render_geom,render_light,render_sensitive,render_blend,render_proximity,render_collision);

	if (!p) {
		/* we have no geometry yet, sleep for a tiny bit */
		usleep(1000);
		return;
	}

	if (verbose)
  		printf("Render_hier node=%d what=%d\n", p, rwhat);

	/* status bar */
	if ((render_geom) && (display_status)) {
		render_status();
	}
	
	if (render_sensitive) {
		upd_ray();
	}
	render_node(p);

	/*get viewpoint result, only for upvector*/
	if (render_vp &&
		ViewerUpvector.x == 0 &&
		ViewerUpvector.y == 0 &&
		ViewerUpvector.z == 0) {

		/* store up vector for gravity and collision detection */
		/* naviinfo.reset_upvec is set to 1 after a viewpoint change */
		glGetDoublev(GL_MODELVIEW_MATRIX, modelMatrix);
		matinverse(modelMatrix,modelMatrix);
		transform3x3(&ViewerUpvector,&upvec,modelMatrix);

		if (verbose) printf("ViewerUpvector = (%f,%f,%f)\n", ViewerUpvector);
	}
}

MODULE = VRML::VRMLFunc PACKAGE = VRML::VRMLFunc
PROTOTYPES: ENABLE



####################################################################
#
# Save Font Paths for later use in C, if Text nodes exist
#
####################################################################

int
save_font_path(myfp)
	char *myfp
CODE:
		strncpy(sys_fp,myfp,fp_name_len-20);



void *
alloc_struct(siz,virt)
	int siz
	void *virt
CODE:
	void *ptr = malloc(siz);
	struct VRML_Box *p = ptr;
	/* printf("Alloc: %d %d -> %d\n", siz, virt, ptr);  */
	*(struct VRML_Virt **)ptr = (struct VRML_Virt *)virt;
	p->_sens = p->_hit = 0;
	p->_intern = 0;
	p->_change = 153;
	p->_dlchange = 0;
	p->_dlist = 0;
        p->_parents = 0;
        p->_nparents = 0;
        p->_nparalloc = 0;
	p->_ichange = 0;
	p->_dist = -10000.0; /* put unsorted nodes first in rendering */
	//p->_dist = 0.0; /* put unsorted nodes first in rendering */
	p->_extent[0] = 0.0;
	p->_extent[1] = 0.0;
	p->_extent[2] = 0.0;

	RETVAL=ptr;
OUTPUT:
	RETVAL

void
release_struct(ptr)
	void *ptr
CODE:
	struct VRML_Box *p = ptr;
	if(p->_parents) free(p->_parents);
	if(p->_dlist) glDeleteLists(p->_dlist,1);
	printf ("release_struct, texture needs deletion \n");
	free(ptr); /* COULD BE MEMLEAK IF STUFF LEFT INSIDE */

void
set_sensitive(ptr,datanode,type)
	int ptr
	int datanode
	char *type
CODE:
	setSensitive ((void *)ptr,datanode,type);

void
render_verbose(i)
	int i;
CODE:
	verbose=i;

void 
render_verbose_collision(i)
	int i;
CODE:
	verbose_collision=i;

#*****************************************************************************
# return a C pointer to a func for the interpolator functions. Used in CRoutes
# to enable event propagation to call the correct interpolator
#
unsigned int
InterpPointer(x)
	char *x
CODE:
	void *pt;
	void do_Oint4(void *x);

	/* XXX still to do; Fog, Background, Viewpoint, NavigationInfo, Collision */

	if (strncmp("OrientationInterpolator",x,strlen("OrientationInterpolator"))==0) {
		pt = do_Oint4;
		RETVAL = (unsigned int) pt;
	} else if (strncmp("ScalarInterpolator",x,strlen("ScalarInterpolator"))==0) {
		pt = do_OintScalar;
		RETVAL = (unsigned int) pt;
	} else if (strncmp("ColorInterpolator",x,strlen("ColorInterpolator"))==0) {
		pt = do_Oint3;
		RETVAL = (unsigned int) pt;
	} else if (strncmp("PositionInterpolator",x,strlen("PositionInterpolator"))==0) {
		pt = do_Oint3;
		RETVAL = (unsigned int) pt;
	} else if (strncmp("CoordinateInterpolator",x,strlen("CoordinateInterpolator"))==0) {
		pt = do_OintCoord;
		RETVAL = (unsigned int) pt;
	} else if (strncmp("NormalInterpolator",x,strlen("NormalInterpolator"))==0) {
		pt = do_OintCoord;
		RETVAL = (unsigned int) pt;
	} else if (strncmp("GeoPositionInterpolator",x,strlen("GeoPositionInterpolator"))==0) {
		pt = do_GeoOint;
		RETVAL = (unsigned int) pt;
	} else {
		RETVAL = 0;
	}
OUTPUT:
	RETVAL


#*******************************************************************************
# return lengths if C types - used for Routing C to C structures. Platform
# dependent sizes...
# These have to match the clength subs in VRMLFields.pm
# the value of zero (0) is used in VRMLFields.pm to indicate a "don't know".
# some, eg MultiVec3f have a value of -1, and this is just passed back again.
# (this is handled by the routing code; it's more time consuming)
# others, eg ints, have a value from VRMLFields.pm of 1, and return the
# platform dependent size. 

int
getClen(x)
	int x
CODE:
	switch (x) {	
		case 1:	RETVAL = sizeof (int);
			break;
		case 2:	RETVAL = sizeof (float);
			break;
		case 3:	RETVAL = sizeof (double);
			break;
		case 4:	RETVAL = sizeof (struct SFRotation);
			break;
		case 5: RETVAL = sizeof (struct SFColor);
			break;
		case 6: RETVAL = sizeof (struct SFVec2f);
			break;
		case -11: RETVAL = sizeof (unsigned int);
			break;
		default:	RETVAL = x;
	}
OUTPUT:
	RETVAL



#********************************************************************************
#
# We have a new class invocation to worry about...

int
do_newJavaClass(scriptInvocationNumber,nodeURLstr,node)
	int scriptInvocationNumber
	char *nodeURLstr
	char *node 
CODE:
	RETVAL = (int) newJavaClass(scriptInvocationNumber,nodeURLstr,node);
OUTPUT:
	RETVAL


#********************************************************************************
#
# register a route that can go via C, rather than perl.

void
do_CRoutes_Register(adrem, from, fromoffset, to_count, tonode_str, len, intptr, scrpt, extra)
	int adrem
	void *from
	int fromoffset
	int to_count
	char *tonode_str
	int len
	void *intptr
	int scrpt
	int extra
CODE:
	CRoutes_Register(adrem, (unsigned int)from, fromoffset, to_count, tonode_str, len, intptr, scrpt, extra);

#********************************************************************************
#
# Free memory allocated in CRoutes_Register

void
do_CRoutes_free()
CODE:
	CRoutes_free();


#********************************************************************************
# Viewer functions implemented in C replacing viewer Perl module

unsigned int
do_get_buffer()
CODE:
	RETVAL = get_buffer(&Viewer);
OUTPUT:
	RETVAL

void
set_root(rn)
	unsigned int rn
CODE:
	rootNode = rn;

#********************************************************************************
# Register a timesensitive node so that it gets "fired" every event loop

void
add_first(clocktype,node)
	char *clocktype
	int node
CODE:
	add_first(clocktype,node);

#********************************************************************************

# save the specific FreeWRL version number from the Config files.
void
SaveVersion(str)
	char *str
CODE:
	BrowserVersion = malloc (strlen(str)+1);
	strcpy (BrowserVersion,str);

SV *
GetBrowserURL()
CODE:
	RETVAL = newSVpv(BrowserURL, strlen(BrowserURL));
OUTPUT:
	RETVAL

#****************JAVASCRIPT FUNCTIONS*********************************

void
setJSVerbose(v)
	int v;
CODE:
{
	JSVerbose = v;
}


## worry about garbage collection here ???
void
jsinit(num, sv_js)
	int num
	SV *sv_js
CODE:
	JSInit(num,sv_js);

void
jscleanup(num)
	int num
CODE:
	JScleanup(num);

int
jsrunScript(num, script, rstr, rnum)
	int num
	char *script
	SV *rstr
	SV *rnum
CODE:
	RETVAL = JSrunScript (num, script, rstr, rnum);
OUTPUT:
RETVAL
rstr
rnum


int
jsGetProperty(num, script, rstr)
	int num
	char *script
	SV *rstr
CODE:
	RETVAL = JSGetProperty (num, script, rstr);
OUTPUT:
RETVAL
rstr

int
addSFNodeProperty(num, nodeName, name, str)
	int num
	char *nodeName
	char *name
	char *str
CODE:
	RETVAL = JSaddSFNodeProperty(num, nodeName, name, str);
OUTPUT:
RETVAL


int
addGlobalAssignProperty(num, name, str)
	int num
	char *name
	char *str
CODE:
	RETVAL = JSaddGlobalAssignProperty(num, name, str);
OUTPUT:
RETVAL


int
addGlobalECMANativeProperty(num, name)
	int num
	char *name
CODE:
	RETVAL = JSaddGlobalECMANativeProperty(num, name);
OUTPUT:
RETVAL

int
paramIndex(evin, evtype)
	char *evin
	char *evtype
CODE:
	RETVAL = JSparamIndex (evin,evtype);
OUTPUT:
RETVAL


# allow Javascript to add/remove children when the parent is a USE - see JS/JS.pm.
void
jsManipulateChild(par, fiel, child)
	int par
	char *fiel
	int child
CODE:
	char onechildline[100];

	sprintf (onechildline, "[ %d ]",child);
	getMFNodetype (onechildline, (struct Multi_Node *) par,
		!strncmp (fiel,"addChild",strlen ("addChild")));

# link into EAI.

int
EAIExtraMemory (type,size,data)
	char *type
	int size
	SV *data
	CODE:
	RETVAL = EAI_do_ExtraMemory (size,data,type);
OUTPUT:
RETVAL

# simple malloc - used for Java CLASS parameters
int
malloc_this (size)
	int size
	CODE:
	RETVAL = (int) malloc(size);
OUTPUT:
RETVAL

# remove comments, etc, from a string.
SV *
sanitizeInput(string)
	char *string
CODE:
	char *buffer;
	buffer = sanitizeInputString(string);
	RETVAL = newSVpv(buffer,strlen(buffer));
	OUTPUT:
RETVAL

# read in a string from a file
SV *
readFile(fn,parent)
	char *fn
	char *parent
CODE:
	char *buffer;

	buffer = readInputString(fn,parent);
	RETVAL= newSVpv(buffer,strlen(buffer));
	OUTPUT:
RETVAL


#****************JAVASCRIPT FUNCTIONS*********************************

ENDHERE
;
	print XS join '',@xsfn;
	print XS '
';

	open PM, ">VRMLFunc.pm";
	print PM "
# VRMLFunc.pm, generated by VRMLC.pm. DO NOT MODIFY, MODIFY VRMLC.pm INSTEAD
package VRML::VRMLFunc;
require DynaLoader;
require Exporter;
\@ISA=qw(Exporter DynaLoader);
";
        print PM &gen_constants_pm_header();
	print PM "
bootstrap VRML::VRMLFunc;
sub load_data {
	my \$n = \\\%VRML::CNodes;
";
	print PM join '',@poffsfn;
	print PM "
}
";
        print PM &gen_constants_pm_body();

}


gen();


