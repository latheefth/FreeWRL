package vrml.field;
import vrml.*;
import java.util.StringTokenizer;

public class SFRotation extends Field {
    float axisX;
    float axisY;
    float axisZ;
    float angle;

    public SFRotation() {}
    public SFRotation(float axisX, float axisY, float axisZ, float angle) {
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

    public void setValue(float axisX, float axisY, float axisZ, float angle) {
        this.axisX = axisX;
        this.axisY = axisY;
        this.axisZ = axisZ;
        this.angle = angle;
        __updateWrite();
    }
    public void setValue(float[] values) {
        this.axisX = values[0];
        this.axisY = values[1];
        this.axisZ = values[2];
        this.angle = values[3];
        __updateWrite();
    }
    public void setValue(ConstSFRotation sfRotation) {
        sfRotation.__updateRead();
        axisX = sfRotation.axisX;
        axisY = sfRotation.axisY;
        axisZ = sfRotation.axisZ;
        angle = sfRotation.angle;
        __updateWrite();
    }
    public void setValue(SFRotation sfRotation) {
        sfRotation.__updateRead();
        axisX = sfRotation.axisX;
        axisY = sfRotation.axisY;
        axisZ = sfRotation.axisZ;
        angle = sfRotation.angle;
        __updateWrite();
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