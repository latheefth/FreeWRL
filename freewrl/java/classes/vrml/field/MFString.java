package vrml.field;
import vrml.*;
import java.util.StringTokenizer;

public class MFString extends MField {

    public MFString() {}
    public MFString(String[] s) {
        this(s.length, s);
    }
    public MFString(int size, String[] s) {
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

    public void setValue(String[] s) {
        setValue(s.length, s);
    }
    public void setValue(int size, String[] s) {
        __vect.clear();
        for (int i = 0; i < size; i++)
            __vect.addElement(new ConstSFString(s[i]));
        __updateWrite();
    };
    public void set1Value(int index, String s) {
        __set1Value(index, new ConstSFString(s));
    };
    public void set1Value(int index, SFString sfString) {
        sfString.__updateRead();
        __set1Value(index, new ConstSFString(sfString.s));
    };
    public void set1Value(int index, ConstSFString sfString) {
        __set1Value(index, sfString);
    };
    public void addValue(String s) {
        __addValue(new ConstSFString(s));
    };
    public void addValue(SFString sfString) {
        sfString.__updateRead();
        __addValue(new ConstSFString(sfString.s));
    };
    public void addValue(ConstSFString sfString) {
        __addValue(sfString);
    };
    public void insertValue(int index, String s) {
        __insertValue(index, new ConstSFString(s));
    };
    public void insertValue(int index, SFString sfString) {
        sfString.__updateRead();
        __insertValue(index, new ConstSFString(sfString.s));
    };
    public void insertValue(int index, ConstSFString sfString) {
        __insertValue(index, sfString);
    };

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