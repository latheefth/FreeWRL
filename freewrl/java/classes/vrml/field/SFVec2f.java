package vrml.field;
import vrml.*;
import java.util.StringTokenizer;

public class SFVec2f extends Field {
    float x;
    float y;

    public SFVec2f() {}
    public SFVec2f(float x, float y) {
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

    public void setValue(float x, float y) {
        this.x = x;
        this.y = y;
        __updateWrite();
    }
    public void setValue(float[] values) {
        this.x = values[0];
        this.y = values[1];
        __updateWrite();
    }
    public void setValue(ConstSFVec2f sfVec2f) {
        sfVec2f.__updateRead();
        x = sfVec2f.x;
        y = sfVec2f.y;
        __updateWrite();
    }
    public void setValue(SFVec2f sfVec2f) {
        sfVec2f.__updateRead();
        x = sfVec2f.x;
        y = sfVec2f.y;
        __updateWrite();
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