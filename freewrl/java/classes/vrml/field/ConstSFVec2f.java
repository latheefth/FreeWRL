//AUTOMATICALLY GENERATED BY genfields.pl.
//DO NOT EDIT!!!!

package vrml.field;
import vrml.*;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;

public class ConstSFVec2f extends ConstField {
    float x;
    float y;

    public ConstSFVec2f() {
    }
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
        __updateRead();
        return ""+x+" "+y;
    }

    public void __fromPerl(DataInputStream in)  throws IOException {
        x = Float.parseFloat(in.readUTF());
        y = Float.parseFloat(in.readUTF());
    }

    public void __toPerl(DataOutputStream out)  throws IOException {
        out.writeUTF(""+x);
        out.writeUTF(""+y);
    }
}