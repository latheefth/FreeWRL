package vrml.field;
import vrml.*;
import java.util.StringTokenizer;

public class ConstSFVec3f extends ConstField {
    float x;
    float y;
    float z;

    public ConstSFVec3f() {} /* only for internal use */
    public ConstSFVec3f(float x, float y, float z) {
        this.x = x;
        this.y = y;
        this.z = z;
    }
    public void getValue(float[] values) {
        __updateRead();
        values[0] = x;
        values[1] = y;
        values[2] = z;
    }

    public float getX() {
        __updateRead();
        return x;
    }

    public float getY() {
        __updateRead();
        return y;
    }

    public float getZ() {
        __updateRead();
        return z;
    }


    public String toString() {
        return ""+x+" "+y+" "+z;
    }

    public void __fromPerl(String str) {
        StringTokenizer tok = new StringTokenizer(str, " ");
	x = new Float(tok.nextToken()).floatValue();
	y = new Float(tok.nextToken()).floatValue();
	z = new Float(tok.nextToken()).floatValue();
    }

    public String __toPerl() {
        return toString();
    }
}