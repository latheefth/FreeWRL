package vrml.field;
import vrml.*;
import java.util.StringTokenizer;

public class ConstMFString extends ConstMField {

    public ConstMFString() {}
    public ConstMFString(String[] s) {
        this(s.length, s);    }
    public ConstMFString(int size, String[] s) {
        for (int i = 0; i < size; i++)
            __vect.addElement(new ConstSFString(s[i]));
    }
    public void getValue(String[] s) {
        __updateRead();
        int size = __vect.size();
        for (int i = 0; i < size; i++) {
            s[i] = ((ConstSFString) __vect.elementAt(i)).getValue();
        }
    }


    public String get1Value(int index) {
        __update1Read(index);
        return ((ConstSFString) __vect.elementAt(index)).getValue();
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
            ConstSFString sf = new ConstSFString();
            sf.__fromPerl(st.nextToken());
            __vect.addElement(sf);
        }
    }

    public String __toPerl() {
        StringBuffer sb = new StringBuffer("");
        int size = __vect.size();
        for (int i = 0; i < size; i++) {
            if (i > 0) sb.append(",");
            sb.append(((ConstSFString) __vect.elementAt(i)).__toPerl());
        }
        return sb.append("").toString();
    }
}