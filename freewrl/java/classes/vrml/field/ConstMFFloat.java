package vrml.field;
import vrml.*;
import java.util.StringTokenizer;

public class ConstMFFloat extends ConstMField {

    public ConstMFFloat() {}
    public ConstMFFloat(float[] f) {
        this(f.length, f);    }
    public ConstMFFloat(int size, float[] f) {
        for (int i = 0; i < size; i++)
            __vect.addElement(new ConstSFFloat(f[i]));
    }
    public void getValue(float[] f) {
        __updateRead();
        int size = __vect.size();
        for (int i = 0; i < size; i++) {
            f[i] = ((ConstSFFloat) __vect.elementAt(i)).getValue();
        }
    }


    public float get1Value(int index) {
        __update1Read(index);
        return ((ConstSFFloat) __vect.elementAt(index)).getValue();
    }


    public String toString() {
        StringBuffer sb = new StringBuffer("[");
        int size = __vect.size();
        for (int i = 0; i < size; i++) {
            if (i > 0) sb.append(", ");
            sb.append(__vect.elementAt(i));
        }
        return sb.append("]").toString();
    }

    public void __fromPerl(String str) {
        StringTokenizer st = new StringTokenizer(str,",");
        while (st.hasMoreTokens()) {
            ConstSFFloat sf = new ConstSFFloat();
            sf.__fromPerl(st.nextToken());
            __vect.addElement(sf);
        }
    }

    public String __toPerl() {
        StringBuffer sb = new StringBuffer("");
        int size = __vect.size();
        for (int i = 0; i < size; i++) {
            if (i > 0) sb.append(",");
            sb.append(((ConstSFFloat) __vect.elementAt(i)).__toPerl());
        }
        return sb.append("").toString();
    }
}