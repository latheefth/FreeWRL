
for (qw( EventIn EventOut ExposedField FieldChange Field Route )) {
	$exc = "Invalid".$_."Exception";
	open O, ">$exc.java";
	print O "package vrml;\n\npublic class $exc extends IllegalArgumentException {\n";
	print O "    public $exc() { super(); }\n";
	print O "    public $exc(String s) { super(s); }\n";
	print O "}\n";
	close O;
}
