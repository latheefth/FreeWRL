package vrml.field;
import vrml.*;
import java.util.StringTokenizer;

public class ConstSFColor extends ConstField {
    float red;
    float green;
    float blue;

    public ConstSFColor() {} /* only for internal use */
    public ConstSFColor(float red, float green, float blue) {
        this.red = red;
        this.green = green;
        this.blue = blue;
    }
    public void getValue(float[] values) {
        __updateRead();
        values[0] = red;
        values[1] = green;
        values[2] = blue;
    }

    public float getRed() {
        __updateRead();
        return red;
    }

    public float getGreen() {
        __updateRead();
        return green;
    }

    public float getBlue() {
        __updateRead();
        return blue;
    }


    public String toString() {
        return ""+red+" "+green+" "+blue;
    }

    public void __fromPerl(String str) {
        StringTokenizer tok = new StringTokenizer(str, " ");
	red = 	new Float(tok.nextToken()).floatValue();
	green =	new Float(tok.nextToken()).floatValue();
	blue =	new Float(tok.nextToken()).floatValue();
    }

    public String __toPerl() {
        return toString();
    }
}