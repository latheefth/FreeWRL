# Copyright (C) 2002 Jochen Hoenicke
# DISTRIBUTED WITH NO WARRANTY, EXPRESS OR IMPLIED.
# See the GNU Library General Public License (file COPYING in the distribution)
# for conditions of use and redistribution.

my %fieldtypes = (
	"Bool"  => "boolean value",
	"Color" => "float red,float green,float blue",
	"Float" => "float f",
	"Image" => "int width,int height,int components,byte[] pixels",
	"Int32" => "int value",
	"Node"  => "BaseNode node",
	"Rotation"  => "float axisX,float axisY,float axisZ,float angle",
	"String" => "String s",
	"Time"  => "double value",
	"Vec2f" => "float x,float y",
	"Vec3f" => "float x,float y,float z"
);

my %toString = (
	"Bool"   => 'return value ? "TRUE" : "FALSE";',
	"Color"  => 'return ""+red+" "+green+" "+blue;',
	"Float"  => 'return String.valueOf(f);',
	"Image"  => q{StringBuffer sb = new StringBuffer();
        sb.append(width).append(' ').append(height).append(' ').append(components);
        for (int i = 0; i < pixels.length; i+=components) {
	    sb.append(" 0x");
	    for (int j = i; j < i+components; j++)
		sb.append("0123456789ABCDEF".charAt((pixels[i+j] & 0xf0) >> 4))
		    .append("0123456789ABCDEF".charAt(pixels[i+j] & 0x0f));
	}
        return sb.toString();},
	"Int32"  => 'return String.valueOf(value);',
	"Node"   => 'return FWHelper.nodeToString(node);',
	"Rotation" => 'return ""+axisX+" "+axisY+" "+axisZ+" "+angle;',
	"String" => 'return vrml.FWHelper.quote(s);',
	"Time"   => 'return String.valueOf(value);',
	"Vec2f"  => 'return ""+x+" "+y;',
	"Vec3f"  => 'return ""+x+" "+y+" "+z;'
	    );


my %toPerl = (
	"Bool"   => 'out.writeBoolean(value);',
	"Color"  => 'out.writeUTF(""+red);
        out.writeUTF(""+green); 
        out.writeUTF(""+blue);',
	"Float"  => 'out.writeUTF(""+f);',
	"Image"  => 'out.writeInt(width);
        out.writeInt(height); 
        out.writeInt(components);
        out.write(pixels);',
	"Int32"  => 'out.writeInt(value);',
	"Node"   => 'out.writeUTF(node._get_nodeid());',
	"Rotation"  => 'out.writeUTF(""+axisX);
        out.writeUTF(""+axisY);
        out.writeUTF(""+axisZ);
        out.writeUTF(""+angle);',
	"String" => 'out.writeUTF(s);',
	"Time"   => 'out.writeUTF(""+value);',
	"Vec2f"  => 'out.writeUTF(""+x);
        out.writeUTF(""+y);',
	"Vec3f"  => 'out.writeUTF(""+x);
        out.writeUTF(""+y);
        out.writeUTF(""+z);'
	    );

my %fromPerl = (
	"Bool"   => 'value = in.readBoolean();',
	"Color"  => 'red = Float.parseFloat(in.readUTF());
        green = Float.parseFloat(in.readUTF()); 
        blue = Float.parseFloat(in.readUTF());',
	"Float"  => 'f = Float.parseFloat(in.readUTF());',
	"Image"  => 'width = in.readInt();
        height = in.readInt(); 
        components = in.readInt();
        pixels = new byte[height*width*components];
        in.readFully(pixels);',
	"Int32"  => 'value = in.readInt();',
	"Node"   => 'node = new vrml.node.Node(in.readUTF());',
	"Rotation"  => 'axisX = Float.parseFloat(in.readUTF());
        axisY = Float.parseFloat(in.readUTF());
        axisZ = Float.parseFloat(in.readUTF());
        angle = Float.parseFloat(in.readUTF());',
	"String" => 's = in.readUTF();',
	"Time"   => 'value = Double.parseDouble(in.readUTF());',
	"Vec2f"  => 'x = Float.parseFloat(in.readUTF());
        y = Float.parseFloat(in.readUTF());',
	"Vec3f"  => 'x = Float.parseFloat(in.readUTF());
        y = Float.parseFloat(in.readUTF());
        z = Float.parseFloat(in.readUTF());'
		);

my $multival  = "Color|Vec.f|Rotation";
my $multiname = "Color|Vec.f|Image";

sub startclass
{
	my ($class, $super) = @_;
	open O, ">field/$class.java";
	print O "//AUTOMATICALLY GENERATED BY genfields.pl.\n";
	print O "//DO NOT EDIT!!!!\n\n";
	print O "package vrml.field;\n";
	print O "import vrml.*;\n";
	print O "import java.io.DataInputStream;\n";
	print O "import java.io.DataOutputStream;\n";
	print O "import java.io.IOException;\n";
	print O "\npublic class $class extends $super {\n";
}

sub endclass
{
	print O "}";
	close O;
}

### output functions for SF classes ##########

sub sf_constructor
{
	my ($class, $ft, @values) = @_;
	my $params = join(", ", @values);
	my $init = join("\n        ", 
					map { @_ = split " ",$_; "this.$_[1] = $_[1];" } @values);
	# fields
	for (@values) {
		print O "    $_;\n";
	}

	# constructor
	print O <<EOF;

    public $class() {
    }
    public $class($params) {
        $init
    }
EOF
}

sub sf_getvalue
{
	my ($ft, @values) = @_;

	if (@values == 1) {
		@_ = split " ",$values[0];
		print O <<EOF;

    public $_[0] getValue() {
        __updateRead();
        return $_[1];
    }
EOF
	} else {
		if ($ft =~ /$multival/) {
			my $i = -1;
			my $body = join("\n        ",
							map { $i++; split " ",$_; "values[$i] = $_[1];"} 
							@values);
			@_ = split " ",$values[0];
			print O <<EOF;

    public void getValue($_[0]\[] values) {
        __updateRead();
        $body
    }
EOF
		}
		if ($ft =~ /$multiname/) {
			for (@values) {
				@_ = split " ",$_;
				$upcase = "\u$_[1]";

				print O <<EOF;

    public $_[0] get$upcase() {
        __updateRead();
        return $_[1];
    }
EOF
			}
		}
	}
}

sub sf_setvalue
{
	my ($ft, @values) = @_;

	my $body = join("\n        ",
					map { split " ",$_; "this.$_[1] = $_[1];"} 
					@values);
	my $params = join(", ", @values);
	
	print O <<EOF;

    public void setValue($params) {
        $body
        __updateWrite();
    }
EOF

	if ($ft =~ /$multival/) {
		my $i = -1;
		$body = join("\n        ",
					 map { $i++; split " ",$_; "this.$_[1] = values[$i];"} 
					 @values);
		@_ = split " ",$values[0];
	print O <<EOF;

    public void setValue($_[0]\[] values) {
        $body
        __updateWrite();
    }
EOF
	}

	# set methods with (Const)SF$ft
	$body = join("\n        ",
				 map { split " ",$_; "$_[1] = sf$ft.$_[1];"} 
				 @values);
	
	print O <<EOF;

    public void setValue(ConstSF$ft sf$ft) {
        sf$ft.__updateRead();
        $body
        __updateWrite();
    }

    public void setValue(SF$ft sf$ft) {
        sf$ft.__updateRead();
        $body
        __updateWrite();
    }
EOF
}

sub sf_stringfuncs
{
	# toString
	print O <<EOF;

    public String toString() {
        __updateRead();
        $toString{$ft}
    }

    public void __fromPerl(DataInputStream in)  throws IOException {
        $fromPerl{$ft}
    }

    public void __toPerl(DataOutputStream out)  throws IOException {
        $toPerl{$ft}
    }
EOF
}

### output functions for MF classes ##########

sub mf_typename
{
	my ($ft, @values) = @_;
	@typename = split " ", $values[0];
	$typename[1] = "\L$ft\Es" if (scalar(@values) > 1);
	@typename;
}

sub mf_constructor
{
	my ($class, $ft, @values) = @_;
	my $numval = scalar(@values);
	my ($valtype, $valname) = mf_typename($ft, @values);
	my $incr = ($numval == 1 ? "i++" : "i += $numval");
	my $constrargs = join(", ",
						  map  { $valname."[i". ($_ ? "+$_" : ""). "]" }
						  0..$numval-1);

	# constructors
	print O <<EOF;
    public $class() {
    }

    public $class($valtype\[] $valname) {
        this($valname.length, $valname);
    }

    public $class(int size, $valtype\[] $valname) {
        for (int i = 0; i < size; $incr)	
            __vect.addElement(new ConstSF$ft($constrargs));
    }
EOF

	$constrargs = join(", ", map  { $valname."[i][$_]" } 0..$numval-1);

	if ($numval > 1) {
		print O <<EOF;

    public $class($valtype\[][] $valname) {
        for (int i = 0; i < $valname.length; i++)
            __vect.addElement(new ConstSF$ft($constrargs));
    }
EOF
	}

}

sub	mf_getvalue
{
	my ($ft, @values) = @_;
	my $numval = scalar(@values);
	my ($valtype, $valname) = mf_typename($ft, @values);

	# get method(s)

	my $tmpdecl = "";
	my $forbody = "ConstSF$ft sf$ft = (ConstSF$ft) __vect.elementAt(i);";
	my $i = 0;
	for (@values) {
		@_ = split " ",$_;
		$index = $numval == 1 ? "i" : "$numval*i+$i";
		$forbody .= "\n            "
			. "$valname\[$index] = sf$ft.$_[1];";
		$i++
	}

	print O <<EOF;
	
    public void getValue($valtype\[] $valname) {
        __updateRead();
        int size = __vect.size();$tmpdecl
        for (int i = 0; i < size; i++) {
            $forbody
        }
    }
EOF

	if ($numval > 1) {
		print O <<EOF;

    public void getValue($valtype\[][] $valname) {
        __updateRead();
        int size = __vect.size();
        for (int i = 0; i < size; i++)
            ((ConstSF$ft) __vect.elementAt(i)).getValue($valname\[i]);
    }
EOF
    }

	# get1 method(s)
	if ($numval == 1) {
		print O <<EOF;

    public $valtype get1Value(int index) {
        __update1Read(index);
        return ((ConstSF$ft) __vect.elementAt(index)).getValue();
    }
EOF
	} else {
		print O <<EOF;

    public void get1Value(int index, $valtype\[] $valname) {
        __update1Read(index);
        ((ConstSF$ft) __vect.elementAt(index)).getValue($valname);
    }

    public void get1Value(int index, SF$ft sf$ft) {
        __update1Read(index);
        sf$ft.setValue((ConstSF$ft) __vect.elementAt(index));
    }
EOF
	}
}

sub mf_setvalue
{
	my ($ft, @values) = @_;
	my $numval = scalar(@values);
	my ($valtype, $valname) = mf_typename($ft, @values);
	my $incr = ($numval == 1 ? "i++" : "i += $numval");
	my $constrargs = join(", ",
						  map  { $valname."[i". ($_ ? "+$_" : ""). "]" }
						  0..$numval-1);

	# set method(s)
	print O <<EOF;

    public void setValue($valtype\[] $valname) {
        setValue($valname.length, $valname);
    }

    public void setValue(int size, $valtype\[] $valname) {
        __vect.clear();
        for (int i = 0; i < size; $incr)
            __vect.addElement(new ConstSF$ft($constrargs));
        __updateWrite();
    }
EOF


	for $method ("set1Value", "addValue", "insertValue") {
		my $namelist   = join(", ", 
							  map { @_ = split " ", $_; $_[1] } @values);
		my $sfnamelist = join(", ",
							  map { @_ = split " ", $_; "sf$ft.$_[1]" }
							  @values);
        my $intindex = $method eq "addValue" ? "" : "int index, ";
		my $index = $method eq "addValue" ? "" : "index, ";
        my $params = join(", ", @values);
        
		print O <<EOF;

    public void $method(${intindex}$params) {
        __$method(${index}new ConstSF$ft($namelist));
    }

    public void $method(${intindex}SF$ft sf$ft) {
        sf$ft.__updateRead();
        __$method(${index}new ConstSF$ft($sfnamelist));
    }

    public void $method(${intindex}ConstSF$ft sf$ft) {
        __$method(${index}sf$ft);
    }
EOF
	}
}

sub mf_stringfuncs 
{
    my ($ft) = @_;

    print O <<EOF;

    public String toString() {
        __updateRead();
        StringBuffer sb = new StringBuffer("[");
        int size = __vect.size();
        for (int i = 0; i < size; i++) {
            if (i > 0) sb.append(", ");
            sb.append(__vect.elementAt(i));
        }
        return sb.append("]").toString();
    }

    public void __fromPerl(DataInputStream in)  throws IOException {
        __vect.clear();
        int len = in.readInt();
        for (int i = 0; i < len; i++) {
            ConstSF$ft sf = new ConstSF$ft();
            sf.__fromPerl(in);
            __vect.addElement(sf);
        }
    }

    public void __toPerl(DataOutputStream out)  throws IOException {
        StringBuffer sb = new StringBuffer("");
        int size = __vect.size();
	out.writeInt(size);
        for (int i = 0; i < size; i++)
            ((ConstSF$ft) __vect.elementAt(i)).__toPerl(out);
    }
EOF
}

foreach (keys %fieldtypes) {
	$ft = $_;
	my @values = split ",", $fieldtypes{$ft};

	print "Generating $ft fields\n";

    # SF class
	startclass("SF$ft", "Field");
	sf_constructor("SF$ft", $ft, @values);
	sf_getvalue($ft, @values);
	sf_setvalue($ft, @values);
	sf_stringfuncs($ft);
	endclass();

    # ConstSF class
	startclass("ConstSF$ft", "ConstField");
	sf_constructor("ConstSF$ft", $ft, @values);
	sf_getvalue($ft,@values);
	sf_stringfuncs($ft);
	endclass();

    # There're no MF classes for Bool and Image.
	next if $_ =~ /Bool|Image/;

    # MF class
	startclass("MF$ft", "MField");
	mf_constructor("MF$ft", $ft, @values);
	mf_getvalue($ft, @values);
	mf_setvalue($ft, @values);
	mf_stringfuncs($ft);
	endclass();

    # ConstMF class
	startclass("ConstMF$ft", "ConstMField");
	mf_constructor("ConstMF$ft", $ft, @values);
	mf_getvalue($ft, @values);
	mf_stringfuncs($ft);
	endclass();
}
