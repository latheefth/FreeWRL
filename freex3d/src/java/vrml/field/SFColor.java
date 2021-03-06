//AUTOMATICALLY GENERATED BY genfields.pl.
//DO NOT EDIT!!!!

package vrml.field;
import vrml.*;
import java.io.BufferedReader;
import java.io.PrintWriter;
import java.io.IOException;

public class SFColor extends Field {
     float red;
     float green;
     float blue;

    public SFColor() { }

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
        __updateRead();
        return ""+red+" "+green+" "+blue;
    }

    public void __fromPerl(BufferedReader in)  throws IOException {

	//System.out.println ("fromPerl, Color");
		red = Float.parseFloat(in.readLine());
        	green = Float.parseFloat(in.readLine());
        	blue = Float.parseFloat(in.readLine());
    }

    public void __toPerl(PrintWriter out)  throws IOException {
        out.print(red+ " "+green+" "+blue);
	//out.println();
    }
    //public void setOffset(String offs) { this.offset = offs; } //JAS2
    //public String getOffset() { return this.offset; } //JAS2
}