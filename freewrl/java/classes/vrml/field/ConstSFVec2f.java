package vrml.field;
import vrml.*;
import java.util.StringTokenizer;

public class ConstSFVec2f extends ConstField {
    float x;
    float y;

    public ConstSFVec2f() {} /* only for internal use */
    public ConstSFVec2f(float x, float y) {
        this.x = x;
        this.y = y;
    }
    public void getValue(float[] values) {
        __updateRead();
        values[0] = x;
        values[1] = y;
    }

    public float getX() {
        __updateRead();
        return x;
    }

    public float getY() {
        __updateRead();
        return y;
    }


    public String toString() {
        return ""+x+" "+y;
    }

    public void __fromPerl(String str) {
        StringTokenizer tok = new StringTokenizer(str, " ");
	x = new Float(tok.nextToken()).floatValue();
	y = new Float(tok.nextToken()).floatValue();
    }

    public String __toPerl() {
        return toString();
    }
}