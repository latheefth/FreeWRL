package vrml.field;
import vrml.*;
import java.util.StringTokenizer;

public class ConstSFRotation extends ConstField {
    float axisX;
    float axisY;
    float axisZ;
    float angle;

    public ConstSFRotation() {} /* only for internal use */
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
        return ""+axisX+" "+axisY+" "+axisZ+" "+angle;
    }

    public void __fromPerl(String str) {
        StringTokenizer tok = new StringTokenizer(str, " ");
	axisX = new Float(tok.nextToken()).floatValue();
	axisY =	new Float(tok.nextToken()).floatValue();
	axisZ =	new Float(tok.nextToken()).floatValue();
	angle =	new Float(tok.nextToken()).floatValue();
    }

    public String __toPerl() {
        return toString();
    }
}