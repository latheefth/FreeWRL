//AUTOMATICALLY GENERATED BY genfields.pl.
//DO NOT EDIT!!!!

package vrml.field;
import vrml.*;
import java.io.BufferedReader;
import java.io.PrintWriter;
import java.io.IOException;

public class ConstSFRotation extends ConstField {
     float axisX;
     float axisY;
     float axisZ;
     float angle;

    public ConstSFRotation() { }

    public ConstSFRotation(float axisX, float axisY, float axisZ, float angle) {
	        this.axisX = axisX;
        this.axisY = axisY;
        this.axisZ = axisZ;
        this.angle = angle;
    }

    public void getValue(float[] values) {
        __updateRead();
        values[0] = axisX;
        values[1] = axisY;
        values[2] = axisZ;
        values[3] = angle;
    }

    public String toString() {
        __updateRead();
        return ""+axisX+" "+axisY+" "+axisZ+" "+angle;
    }

    public void __fromPerl(BufferedReader in)  throws IOException {

	//System.out.println ("fromPerl, Rotation");
		axisX = Float.parseFloat(in.readLine());
	        axisY = Float.parseFloat(in.readLine());
        	axisZ = Float.parseFloat(in.readLine());
        	angle = Float.parseFloat(in.readLine());
    }

    public void __toPerl(PrintWriter out)  throws IOException {
        out.print(axisX+" "+axisY+" "+axisZ+" "+angle);
	//out.println();
    }
    //public void setOffset(String offs) { this.offset = offs; } //JAS2
    //public String getOffset() { return this.offset; } //JAS2
}