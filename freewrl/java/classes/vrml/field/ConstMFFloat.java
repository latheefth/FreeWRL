//AUTOMATICALLY GENERATED BY genfields.pl.
//DO NOT EDIT!!!!

package vrml.field;
import vrml.*;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;

public class ConstMFFloat extends ConstMField {
    public ConstMFFloat() {
    }

    public ConstMFFloat(float[] f) {
        this(f.length, f);
    }

    public ConstMFFloat(int size, float[] f) {
        for (int i = 0; i < size; i++)	
            __vect.addElement(new ConstSFFloat(f[i]));
    }
	
    public void getValue(float[] f) {
        __updateRead();
        int size = __vect.size();
        for (int i = 0; i < size; i++) {
            ConstSFFloat sfFloat = (ConstSFFloat) __vect.elementAt(i);
            f[i] = sfFloat.f;
        }
    }

    public float get1Value(int index) {
        __update1Read(index);
        return ((ConstSFFloat) __vect.elementAt(index)).getValue();
    }

    public String toString() {
        __updateRead();
        StringBuffer sb = new StringBuffer("[");
        int size = __vect.size();
        for (int i = 0; i < size; i++) {
            if (i > 0) sb.append(", ");
            sb.append(__vect.elementAt(i));
        }
        return sb.append("]").toString();
    }

    public void __fromPerl(DataInputStream in)  throws IOException {
        __vect.clear();
        int len = in.readInt();
        for (int i = 0; i < len; i++) {
            ConstSFFloat sf = new ConstSFFloat();
            sf.__fromPerl(in);
            __vect.addElement(sf);
        }
    }

    public void __toPerl(DataOutputStream out)  throws IOException {
        StringBuffer sb = new StringBuffer("");
        int size = __vect.size();
	out.writeInt(size);
        for (int i = 0; i < size; i++)
            ((ConstSFFloat) __vect.elementAt(i)).__toPerl(out);
    }
}