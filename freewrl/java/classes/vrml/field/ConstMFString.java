//AUTOMATICALLY GENERATED BY genfields.pl.
//DO NOT EDIT!!!!

package vrml.field;
import vrml.*;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;

public class ConstMFString extends ConstMField {
    public ConstMFString() {
    }

    public ConstMFString(String[] s) {
        this(s.length, s);
    }

    public ConstMFString(int size, String[] s) {
        for (int i = 0; i < size; i++)	
            __vect.addElement(new ConstSFString(s[i]));
    }
	
    public void getValue(String[] s) {
        __updateRead();
        int size = __vect.size();
        for (int i = 0; i < size; i++) {
            ConstSFString sfString = (ConstSFString) __vect.elementAt(i);
            s[i] = sfString.s;
        }
    }

    public String get1Value(int index) {
        __update1Read(index);
        return ((ConstSFString) __vect.elementAt(index)).getValue();
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
            ConstSFString sf = new ConstSFString();
            sf.__fromPerl(in);
            __vect.addElement(sf);
        }
    }

    public void __toPerl(DataOutputStream out)  throws IOException {
        StringBuffer sb = new StringBuffer("");
        int size = __vect.size();
	out.writeInt(size);
        for (int i = 0; i < size; i++)
            ((ConstSFString) __vect.elementAt(i)).__toPerl(out);
    }
}