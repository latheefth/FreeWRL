package vrml.field;
import vrml.*;
import java.util.StringTokenizer;

public class SFVec3f extends Field {
    float x;
    float y;
    float z;

    public SFVec3f() {}
    public SFVec3f(float x, float y, float z) {
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

    public void setValue(float x, float y, float z) {
        this.x = x;
        this.y = y;
        this.z = z;
        __updateWrite();
    }
    public void setValue(float[] values) {
        this.x = values[0];
        this.y = values[1];
        this.z = values[2];
        __updateWrite();
    }
    public void setValue(ConstSFVec3f sfVec3f) {
        sfVec3f.__updateRead();
        x = sfVec3f.x;
        y = sfVec3f.y;
        z = sfVec3f.z;
        __updateWrite();
    }
    public void setValue(SFVec3f sfVec3f) {
        sfVec3f.__updateRead();
        x = sfVec3f.x;
        y = sfVec3f.y;
        z = sfVec3f.z;
        __updateWrite();
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