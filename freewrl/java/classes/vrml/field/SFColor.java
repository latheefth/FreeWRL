package vrml.field;
import vrml.*;
import java.util.StringTokenizer;

public class SFColor extends Field {
    float red;
    float green;
    float blue;

    public SFColor() {}
    public SFColor(float red, float green, float blue) {
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

    public void setValue(float red, float green, float blue) {
        this.red = red;
        this.green = green;
        this.blue = blue;
        __updateWrite();
    }
    public void setValue(float[] values) {
        this.red = values[0];
        this.green = values[1];
        this.blue = values[2];
        __updateWrite();
    }
    public void setValue(ConstSFColor sfColor) {
        sfColor.__updateRead();
        red = sfColor.red;
        green = sfColor.green;
        blue = sfColor.blue;
        __updateWrite();
    }
    public void setValue(SFColor sfColor) {
        sfColor.__updateRead();
        red = sfColor.red;
        green = sfColor.green;
        blue = sfColor.blue;
        __updateWrite();
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