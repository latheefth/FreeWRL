package vrml.field;
import vrml.*;
import java.util.StringTokenizer;

public class MFFloat extends MField {

    public MFFloat() {}
    public MFFloat(float[] f) {
        this(f.length, f);
    }
    public MFFloat(int size, float[] f) {
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

    public void setValue(float[] f) {
        setValue(f.length, f);
    }
    public void setValue(int size, float[] f) {
        __vect.clear();
        for (int i = 0; i < size; i++)
            __vect.addElement(new ConstSFFloat(f[i]));
        __updateWrite();
    };
    public void set1Value(int index, float f) {
        __set1Value(index, new ConstSFFloat(f));
    };
    public void set1Value(int index, SFFloat sfFloat) {
        sfFloat.__updateRead();
        __set1Value(index, new ConstSFFloat(sfFloat.f));
    };
    public void set1Value(int index, ConstSFFloat sfFloat) {
        __set1Value(index, sfFloat);
    };
    public void addValue(float f) {
        __addValue(new ConstSFFloat(f));
    };
    public void addValue(SFFloat sfFloat) {
        sfFloat.__updateRead();
        __addValue(new ConstSFFloat(sfFloat.f));
    };
    public void addValue(ConstSFFloat sfFloat) {
        __addValue(sfFloat);
    };
    public void insertValue(int index, float f) {
        __insertValue(index, new ConstSFFloat(f));
    };
    public void insertValue(int index, SFFloat sfFloat) {
        sfFloat.__updateRead();
        __insertValue(index, new ConstSFFloat(sfFloat.f));
    };
    public void insertValue(int index, ConstSFFloat sfFloat) {
        __insertValue(index, sfFloat);
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