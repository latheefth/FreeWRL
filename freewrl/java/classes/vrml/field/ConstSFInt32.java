//AUTOMATICALLY GENERATED BY genfields.pl.
//DO NOT EDIT!!!!

package vrml.field;
import vrml.*;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;

public class ConstSFInt32 extends ConstField {
    int value;

    public ConstSFInt32() {
    }
    public ConstSFInt32(int value) {
        this.value = value;
    }

    public int getValue() {
        __updateRead();
        return value;
    }

    public String toString() {
        __updateRead();
        return String.valueOf(value);
    }

    public void __fromPerl(DataInputStream in)  throws IOException {
        value = in.readInt();
    }

    public void __toPerl(DataOutputStream out)  throws IOException {
        out.writeInt(value);
    }
}