# Copyright (C) 1998 Tuomas J. Lukka
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

my %imports = ("Color" => "java.util.StringTokenizer",
	       "Vec2f" => "java.util.StringTokenizer",
	       "Vec3f" => "java.util.StringTokenizer",
	       "Rotation" => "java.util.StringTokenizer",
	       "Node" => "vrml.BaseNode");
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
	"Bool"   => 'return value ? "1" : "0";',
	"Color"  => 'return toString();',
	"Float"  => 'return toString();',
	"Image"  => 'return toString();',
	"Int32"  => 'return toString();',
	"Node"   => 'return node._get_nodeid();;',
	"Rotation" => 'return toString();',
	"String" => 'return FWHelper.base64encode(s);',
	"Time"   => 'return toString();',
	"Vec2f"  => 'return toString();',
	"Vec3f"  => 'return toString();'
	    );

my %fromPerl = (
	"Bool"   => 'value = str.equals("1");',
	"Color"  => 'StringTokenizer tok = new StringTokenizer(str, " ");
	red = 	new Float(tok.nextToken()).floatValue();
	green =	new Float(tok.nextToken()).floatValue();
	blue =	new Float(tok.nextToken()).floatValue();',
	"Float"  => 'f = new Float(str).floatValue();',
	"Image"  => '/*XXX*/',
	"Int32"  => 'value = Integer.parseInt(str);',
	"Node"   => 'node = new vrml.node.Node(str);',
	"Rotation" => 'StringTokenizer tok = new StringTokenizer(str, " ");
	axisX = new Float(tok.nextToken()).floatValue();
	axisY =	new Float(tok.nextToken()).floatValue();
	axisZ =	new Float(tok.nextToken()).floatValue();
	angle =	new Float(tok.nextToken()).floatValue();',
	"String" => 's = FWHelper.base64decode(str);',
	"Time"   => 'value = new Double(str).doubleValue();',
	"Vec2f"  => 'StringTokenizer tok = new StringTokenizer(str, " ");
	x = new Float(tok.nextToken()).floatValue();
	y = new Float(tok.nextToken()).floatValue();',
	"Vec3f"  => 'StringTokenizer tok = new StringTokenizer(str, " ");
	x = new Float(tok.nextToken()).floatValue();
	y = new Float(tok.nextToken()).floatValue();
	z = new Float(tok.nextToken()).floatValue();'
		);

my $multival  = "Color|Vec.f|Rotation";
my $multiname = "Color|Image|Vec.f";


### the SF classes ###########################

foreach (keys %fieldtypes) {
	$ft = $_;
	my @values = split ",", $fieldtypes{$ft};

	print "Generating SF$ft.java\n";
	open O, ">field/SF$ft.java";
	print O "package vrml.field;\n";
	print O "import vrml.*;\n";
	if (exists $imports{$ft}) {
		for (split ",", $imports{$ft}) {
			print O "import $_;\n";
		} 
	}
	print O "\npublic class SF$ft extends Field {\n";

	for (@values) {
		print O "    $_;\n";
	}
	# no arg constructor
	print O "\n    public SF$ft() {}\n";

	# arg constructor
	print O "    public SF$ft(".join(", ", @values).") {\n";
	for (@values) {
		@_ = split " ",$_;
		print O "        this.$_[1] = $_[1];\n";
	}
	print O "    }\n";

	# get method(s)
	if (@values == 1) {
		@_ = split " ",$values[0];
		print O "    public $_[0] getValue() {\n";
		print O "        __updateRead();\n";
		print O "        return $_[1];\n";
		print O "    }\n\n";
	} else {
		if ($ft =~ /$multival/) {
			@_ = split " ",$values[0];
			print O "    public void getValue($_[0]\[] values) {\n";
			my $i = 0;
			print O "        __updateRead();\n";
			for (@values) {
				@_ = split " ",$_;
				print O "        values[$i] = $_[1];\n";
				$i++;
			}
			print O "    }\n\n";
		}
		if ($ft =~ /$multiname/) {
			for (@values) {
				@_ = split " ",$_;
				$upcase = "\u$_[1]";
				print O "    public $_[0] get$upcase() {\n";
				print O "        __updateRead();\n";
				print O "        return $_[1];\n";
				print O "    }\n\n";
			}
		}
	}

	# set method(s)
	print O "    public void setValue(".join(", ", @values).") {\n";
	for (@values) {
		@_ = split " ",$_;
		print O "        this.$_[1] = $_[1];\n";
	}
	print O "        __updateWrite();\n";
	print O "    }\n";

	if ($ft =~ /$multival/) {
		@_ = split " ",$values[0];
		print O "    public void setValue($_[0]\[] values) {\n";
		my $i = 0;
		for (@values) {
			@_ = split " ",$_;
			print O "        this.$_[1] = values[$i];\n";
			$i++;
		}
		print O "        __updateWrite();\n";
		print O "    }\n";
	}

	# set methods with (Const)SF$ft
	print O "    public void setValue(ConstSF$ft sf$ft) {\n";
	print O "        sf$ft.__updateRead();\n";
	for (@values) {
		@_ = split " ",$_;
		print O "        $_[1] = sf$ft.$_[1];\n";
	}
	print O "        __updateWrite();\n";
	print O "    }\n";

	print O "    public void setValue(SF$ft sf$ft) {\n";
	print O "        sf$ft.__updateRead();\n";
	for (@values) {
		@_ = split " ",$_;
		print O "        $_[1] = sf$ft.$_[1];\n";
	}
	print O "        __updateWrite();\n";
	print O "    }\n";

	# toString
	print O "\n    public String toString() {\n";
	print O "        ".$toString{$ft}."\n";
	print O "    }\n";

	# from/toPerl
	print O "\n    public void __fromPerl(String str) {\n";
	print O "        ".$fromPerl{$ft}."\n";
	print O "    }\n";
	print O "\n    public String __toPerl() {\n";
	print O "        ".$toPerl{$ft}."\n";
	print O "    }\n";

	print O "}";
	close O;
}

### the ConstSF classes ###########################

foreach (keys %fieldtypes) {
	$ft = $_;
	my @values = split ",", $fieldtypes{$ft};

	print "Generating ConstSF$ft.java\n";
	open O, ">field/ConstSF$ft.java";
	print O "package vrml.field;\n";
	print O "import vrml.*;\n";
	if (exists $imports{$ft}) {
		for (split ",", $imports{$ft}) {
			print O "import $_;\n";
		} 
	}
	print O "\npublic class ConstSF$ft extends ConstField {\n";

	for (@values) {
		print O "    $_;\n";
	}
	# no arg constructor
	print O "\n    public ConstSF$ft() {} /* only for internal use */\n";

	# arg constructor
	print O "    public ConstSF$ft(".join(", ", @values).") {\n";
	for (@values) {
		@_ = split " ",$_;
		print O "        this.$_[1] = $_[1];\n";
	}
	print O "    }\n";

	# get method(s)
	if (@values == 1) {
		@_ = split " ",$values[0];
		print O "    public $_[0] getValue() {\n";
		print O "        __updateRead();\n";
		print O "        return $_[1];\n";
		print O "    }\n\n";
	} else {
		if ($ft =~ /$multival/) {
			@_ = split " ",$values[0];
			print O "    public void getValue($_[0]\[] values) {\n";
			my $i = 0;
			print O "        __updateRead();\n";
			for (@values) {
				@_ = split " ",$_;
				print O "        values[$i] = $_[1];\n";
				$i++;
			}
			print O "    }\n\n";
		}
		if ($ft =~ /$multiname/) {
			for (@values) {
				@_ = split " ",$_;
				$upcase = "\u$_[1]";
				print O "    public $_[0] get$upcase() {\n";
				print O "        __updateRead();\n";
				print O "        return $_[1];\n";
				print O "    }\n\n";
			}
		}
	}

	# toString
	print O "\n    public String toString() {\n";
	print O "        ".$toString{$ft}."\n";
	print O "    }\n";

	# from/toPerl
	print O "\n    public void __fromPerl(String str) {\n";
	print O "        ".$fromPerl{$ft}."\n";
	print O "    }\n";
	print O "\n    public String __toPerl() {\n";
	print O "        ".$toPerl{$ft}."\n";
	print O "    }\n";

	print O "}";
	close O;
}


### the MF classes ###########################

foreach (keys %fieldtypes) {
	next if $_ =~ /Bool|Image/;
	$ft = $_;
	my @values = split ",", $fieldtypes{$ft};
	my $numval = @values;
	my ($valtype, $valname) = split " ", $values[0];
	my $i;
	$valname = "\L$ft\Es" if ($numval > 1);
	
	print "Generating MF$ft.java\n";
	open O, ">field/MF$ft.java";
	print O "package vrml.field;\n";
	print O "import vrml.*;\n";
	print O "import java.util.StringTokenizer;\n";
	if (exists $imports{$ft}) {
		for (split ",", $imports{$ft}) {
			print O "import $_;\n";
		} 
	}
	print O "\npublic class MF$ft extends MField {\n";
	
	# no arg constructor
	print O "\n    public MF$ft() {}\n";
	
	# arg constructor
	print O "    public MF$ft($valtype\[] $valname) {\n";
	print O "        this($valname.length, $valname);\n";
	print O "    }\n";
	
	print O "    public MF$ft(int size, $valtype\[] $valname) {\n";
	print O "        for (int i = 0; i < size; " 
	    . ($numval == 1 ? "i++" : "i += $numval") . ")\n";
	
	print O "            __vect.addElement(new ConstSF$ft($valname\[i]";
	for ($i = 1; $i < $numval; $i++) {
		print O ", $valname\[i+$i]";
	}
	print O "));\n";
	print O "    }\n";

	if ($numval > 1) {
		print O "    public MF$ft($valtype\[][] $valname) {\n";
		print O "        for (int i = 0; i < $valname.length; i++)\n";
		print O "            __vect.addElement(new ConstSF$ft($valname\[i][0]";
		for ($i = 1; $i < $numval; $i++) {
			print O ", $valname\[i][$i]";
		}
		print O "));\n";
		print O "    }\n";
	}
	print O "\n";

	# get method(s)
	
	print O "    public void getValue($valtype\[] $valname) {\n";
	print O "        __updateRead();\n";
	print O "        int size = __vect.size();\n";
	if ($numval > 1 && $_ !~ /$multiname/) {
		print O "        $valtype\[] tmp = new $valtype\[$numval];\n";
	}
	print O "        for (int i = 0; i < size; i++) {\n";
        if ($numval == 1) {
		print O "            $valname\[i] = ((ConstSF$ft) __vect.elementAt(i)).getValue();\n";
	} else {
		print O "            ConstSF$ft sf$ft = (ConstSF$ft) __vect.elementAt(i);\n";
		if ($_ =~ /$multiname/) {
			$i = 0;
			for (@values) {
				@_ = split " ",$_;
				$upcase = "\u$_[1]";
				print O "            $valname\[$numval*i+$i]";
				print O " = sf$ft.get$upcase();\n";
				$i++
			}
		} else {
			print O "            sf$ft.getValue(tmp);\n";
			print O "            System.arraycopy(tmp, 0, $valname, $numval*i, $numval);\n";
		}
	}
	print O "        }\n";
	print O "    }\n";

	if ($numval > 1) {
		print O "    public void getValue($valtype\[][] $valname) {\n";
		print O "        __updateRead();\n";
		print O "        int size = __vect.size();\n";
		print O "        for (int i = 0; i < size; i++)\n";
		print O "            ((ConstSF$ft) __vect.elementAt(i)).getValue($valname\[i]);\n";
		print O "    }\n";
	}
	print O "\n";

	# get1 method(s)

	if ($numval == 1) {
		print O "    public $valtype get1Value(int index) {\n";
		print O "        __update1Read(index);\n";
		print O "        return ((ConstSF$ft) __vect.elementAt(index)).getValue();\n";
		print O "    }\n\n";
	} else {
		print O "    public void get1Value(int index, $valtype\[] $valname) {\n";
		print O "        __update1Read(index);\n";
		print O "        ((ConstSF$ft) __vect.elementAt(index)).getValue($valname);\n";
		print O "    }\n";
		print O "    public void get1Value(int index, SF$ft sf$ft) {\n";
		print O "        __update1Read(index);\n";
		print O "        sf$ft.setValue((ConstSF$ft) __vect.elementAt(index));\n";
		print O "    }\n\n";
	}

	# set method(s)
	
	print O "    public void setValue($valtype\[] $valname) {\n";
	print O "        setValue($valname.length, $valname);\n";
	print O "    }\n";
	print O "    public void setValue(int size, $valtype\[] $valname) {\n";
	print O "        __vect.clear();\n";
	print O "        for (int i = 0; i < size; " 
	    . ($numval == 1 ? "i++" : "i += $numval") . ")\n";
	
	print O "            __vect.addElement(new ConstSF$ft($valname\[i]";
	for ($i = 1; $i < $numval; $i++) {
		print O ", $valname\[i+$i]";
	}
	print O "));\n";
	print O "        __updateWrite();\n";
	print O "    };\n";

	for $method ("set1Value", "addValue", "insertValue") {
		my @names   = map { my $val = $_; $val =~ s/^\S*\s//; $val } @values;
		my @sfnames = map { my $val = $_; $val =~ s/^\S*\s/sf$ft\./; $val } @values;
		$intindex = $method eq "addValue" ? "" : "int index, ";
		$index = $method eq "addValue" ? "" : "index, ";
		print O "    public void $method($intindex"
		    . join(", ", @values).") {\n";
		print O "        __$method($index". "new ConstSF$ft("
		    . join(", ", @names). "));\n";
		print O "    };\n";
		print O "    public void $method($intindex"."SF$ft sf$ft) {\n";
		print O "        sf$ft.__updateRead();\n";
		print O "        __$method($index". "new ConstSF$ft("
		    . join(", ", @sfnames ). "));\n";
		print O "    };\n";
		print O "    public void $method($intindex"."ConstSF$ft sf$ft) {\n";
		print O "        __$method($index". "sf$ft);\n";
		print O "    };\n";
	}

	# toString
	print O "\n    public String toString() {\n";
	print O "        StringBuffer sb = new StringBuffer(\"[\");\n";
	print O "        int size = __vect.size();\n";
	print O "        for (int i = 0; i < size; i++) {\n";
	print O "            if (i > 0) sb.append(\", \");\n";
	print O "            sb.append(__vect.elementAt(i));\n";
	print O "        }\n";
	print O "        return sb.append(\"]\").toString();\n";
	print O "    }\n";



	# from/toPerl
	print O "\n    public void __fromPerl(String str) {\n";
	print O "        StringTokenizer st = new StringTokenizer(str,\",\");\n";
        print O "        while (st.hasMoreTokens()) {\n";
	print O "            ConstSF$ft sf = new ConstSF$ft();\n";
        print O "            sf.__fromPerl(st.nextToken());\n";
	print O "            __vect.addElement(sf);\n";
	print O "        }\n";
	print O "    }\n";
	print O "\n    public String __toPerl() {\n";
	print O "        StringBuffer sb = new StringBuffer(\"\");\n";
	print O "        int size = __vect.size();\n";
	print O "        for (int i = 0; i < size; i++) {\n";
	print O "            if (i > 0) sb.append(\",\");\n";
	print O "            sb.append(((ConstSF$ft) __vect.elementAt(i)).__toPerl());\n";
	print O "        }\n";
	print O "        return sb.append(\"\").toString();\n";
	print O "    }\n";


	print O "}";
	close O;
}


### the ConstMF classes ###########################

foreach (keys %fieldtypes) {
	next if $_ =~ /Bool|Image/;
	$ft = $_;
	my @values = split ",", $fieldtypes{$ft};
	my $numval = @values;
	my ($valtype, $valname) = split " ", $values[0];
	my $i;
	$valname = "\L$ft\Es" if ($numval > 1);
	
	print "Generating ConstMF$ft.java\n";
	open O, ">field/ConstMF$ft.java";
	print O "package vrml.field;\n";
	print O "import vrml.*;\n";
	print O "import java.util.StringTokenizer;\n";
	if (exists $imports{$ft}) {
		for (split ",", $imports{$ft}) {
			print O "import $_;\n";
		} 
	}
	print O "\npublic class ConstMF$ft extends ConstMField {\n";
	
	# no arg constructor
	print O "\n    public ConstMF$ft() {}\n";
	
	# arg constructor
	print O "    public ConstMF$ft($valtype\[] $valname) {\n";
	print O "        this($valname.length, $valname);";
	print O "    }\n";
	
	print O "    public ConstMF$ft(int size, $valtype\[] $valname) {\n";
	print O "        for (int i = 0; i < size; " 
	    . ($numval == 1 ? "i++" : "i += $numval") . ")\n";
	
	print O "            __vect.addElement(new ConstSF$ft($valname\[i]";
	for ($i = 1; $i < $numval; $i++) {
		print O ", $valname\[i+$i]";
	}
	print O "));\n";
	print O "    }\n";

	if ($numval > 1) {
		print O "    public ConstMF$ft($valtype\[][] $valname) {\n";
		print O "        for (int i = 0; i < $valname.length; i++)\n";
		print O "            __vect.addElement(new ConstSF$ft($valname\[i][0]";
		for ($i = 1; $i < $numval; $i++) {
			print O ", $valname\[i][$i]";
		}
		print O "));\n";
		print O "    }\n";
	}

	# get method(s)
	
	print O "    public void getValue($valtype\[] $valname) {\n";
	print O "        __updateRead();\n";
	print O "        int size = __vect.size();\n";
	if ($numval > 1 && $_ !~ /$multiname/) {
		print O "        $valtype\[] tmp = new $valtype\[$numval];\n";
	}
	print O "        for (int i = 0; i < size; i++) {\n";
        if ($numval == 1) {
		print O "            $valname\[i] = ((ConstSF$ft) __vect.elementAt(i)).getValue();\n";
	} else {
		print O "            ConstSF$ft sf$ft = (ConstSF$ft) __vect.elementAt(i);\n";
		if ($_ =~ /$multiname/) {
			$i = 0;
			for (@values) {
				@_ = split " ",$_;
				$upcase = "\u$_[1]";
				print O "            $valname\[$numval*i+$i]";
				print O " = sf$ft.get$upcase();\n";
				$i++
			}
		} else {
			print O "            sf$ft.getValue(tmp);\n";
			print O "            System.arraycopy(tmp, 0, $valname, $numval*i, $numval);\n";
		}
	}
	print O "        }\n";
	print O "    }\n\n";

	if ($numval > 1) {
		print O "    public void getValue($valtype\[][] $valname) {\n";
		print O "        __updateRead();\n";
		print O "        int size = __vect.size();\n";
		print O "        for (int i = 0; i < size; i++)\n";
		print O "            ((ConstSF$ft) __vect.elementAt(i)).getValue($valname\[i]);\n";
		print O "    }\n";
	}
	print O "\n";

	# get1 method(s)

	if ($numval == 1) {
		print O "    public $valtype get1Value(int index) {\n";
		print O "        __update1Read(index);\n";
		print O "        return ((ConstSF$ft) __vect.elementAt(index)).getValue();\n";
		print O "    }\n\n";
	} else {
		print O "    public void get1Value(int index, $valtype\[] $valname) {\n";
		print O "        __update1Read(index);\n";
		print O "        ((ConstSF$ft) __vect.elementAt(index)).getValue($valname);\n";
		print O "    }\n";
		print O "    public void get1Value(int index, SF$ft sf$ft) {\n";
		print O "        __update1Read(index);\n";
		print O "        sf$ft.setValue((ConstSF$ft) __vect.elementAt(index));\n";
		print O "    }\n\n";
	}

	# toString
	print O "\n    public String toString() {\n";
	print O "        StringBuffer sb = new StringBuffer(\"[\");\n";
	print O "        int size = __vect.size();\n";
	print O "        for (int i = 0; i < size; i++) {\n";
	print O "            if (i > 0) sb.append(\", \");\n";
	print O "            sb.append(__vect.elementAt(i));\n";
	print O "        }\n";
	print O "        return sb.append(\"]\").toString();\n";
	print O "    }\n";

	# from/toPerl
	print O "\n    public void __fromPerl(String str) {\n";
	print O "        StringTokenizer st = new StringTokenizer(str,\",\");\n";
        print O "        while (st.hasMoreTokens()) {\n";
	print O "            ConstSF$ft sf = new ConstSF$ft();\n";
        print O "            sf.__fromPerl(st.nextToken());\n";
	print O "            __vect.addElement(sf);\n";
	print O "        }\n";
	print O "    }\n";
	print O "\n    public String __toPerl() {\n";
	print O "        StringBuffer sb = new StringBuffer(\"\");\n";
	print O "        int size = __vect.size();\n";
	print O "        for (int i = 0; i < size; i++) {\n";
	print O "            if (i > 0) sb.append(\",\");\n";
	print O "            sb.append(((ConstSF$ft) __vect.elementAt(i)).__toPerl());\n";
	print O "        }\n";
	print O "        return sb.append(\"\").toString();\n";
	print O "    }\n";

	print O "}";
	close O;
}
