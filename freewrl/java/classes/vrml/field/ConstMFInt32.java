package vrml.field;
import vrml.*;
import java.util.StringTokenizer;

public class ConstMFInt32 extends ConstMField {

    public ConstMFInt32() {}
    public ConstMFInt32(int[] value) {
        this(value.length, value);    }
    public ConstMFInt32(int size, int[] value) {
        for (int i = 0; i < size; i++)
            __vect.addElement(new ConstSFInt32(value[i]));
    }
    public void getValue(int[] value) {
        __updateRead();
        int size = __vect.size();
        for (int i = 0; i < size; i++) {
            value[i] = ((ConstSFInt32) __vect.elementAt(i)).getValue();
        }
    }


    public int get1Value(int index) {
        __update1Read(index);
        return ((ConstSFInt32) __vect.elementAt(index)).getValue();
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
            ConstSFInt32 sf = new ConstSFInt32();
            sf.__fromPerl(st.nextToken());
            __vect.addElement(sf);
        }
    }

    public String __toPerl() {
        StringBuffer sb = new StringBuffer("");
        int size = __vect.size();
        for (int i = 0; i < size; i++) {
            if (i > 0) sb.append(",");
            sb.append(((ConstSFInt32) __vect.elementAt(i)).__toPerl());
        }
        return sb.append("").toString();
    }
}