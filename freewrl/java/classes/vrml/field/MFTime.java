package vrml.field;
import vrml.*;
import java.util.StringTokenizer;

public class MFTime extends MField {

    public MFTime() {}
    public MFTime(double[] value) {
        this(value.length, value);
    }
    public MFTime(int size, double[] value) {
        for (int i = 0; i < size; i++)
            __vect.addElement(new ConstSFTime(value[i]));
    }

    public void getValue(double[] value) {
        __updateRead();
        int size = __vect.size();
        for (int i = 0; i < size; i++) {
            value[i] = ((ConstSFTime) __vect.elementAt(i)).getValue();
        }
    }

    public double get1Value(int index) {
        __update1Read(index);
        return ((ConstSFTime) __vect.elementAt(index)).getValue();
    }

    public void setValue(double[] value) {
        setValue(value.length, value);
    }
    public void setValue(int size, double[] value) {
        __vect.clear();
        for (int i = 0; i < size; i++)
            __vect.addElement(new ConstSFTime(value[i]));
        __updateWrite();
    };
    public void set1Value(int index, double value) {
        __set1Value(index, new ConstSFTime(value));
    };
    public void set1Value(int index, SFTime sfTime) {
        sfTime.__updateRead();
        __set1Value(index, new ConstSFTime(sfTime.value));
    };
    public void set1Value(int index, ConstSFTime sfTime) {
        __set1Value(index, sfTime);
    };
    public void addValue(double value) {
        __addValue(new ConstSFTime(value));
    };
    public void addValue(SFTime sfTime) {
        sfTime.__updateRead();
        __addValue(new ConstSFTime(sfTime.value));
    };
    public void addValue(ConstSFTime sfTime) {
        __addValue(sfTime);
    };
    public void insertValue(int index, double value) {
        __insertValue(index, new ConstSFTime(value));
    };
    public void insertValue(int index, SFTime sfTime) {
        sfTime.__updateRead();
        __insertValue(index, new ConstSFTime(sfTime.value));
    };
    public void insertValue(int index, ConstSFTime sfTime) {
        __insertValue(index, sfTime);
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
            ConstSFTime sf = new ConstSFTime();
            sf.__fromPerl(st.nextToken());
            __vect.addElement(sf);
        }
    }

    public String __toPerl() {
        StringBuffer sb = new StringBuffer("");
        int size = __vect.size();
        for (int i = 0; i < size; i++) {
            if (i > 0) sb.append(",");
            sb.append(((ConstSFTime) __vect.elementAt(i)).__toPerl());
        }
        return sb.append("").toString();
    }
}