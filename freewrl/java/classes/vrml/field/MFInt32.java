package vrml.field;
import vrml.*;
import java.util.StringTokenizer;

public class MFInt32 extends MField {

    public MFInt32() {}
    public MFInt32(int[] value) {
        this(value.length, value);
    }
    public MFInt32(int size, int[] value) {
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

    public void setValue(int[] value) {
        setValue(value.length, value);
    }
    public void setValue(int size, int[] value) {
        __vect.clear();
        for (int i = 0; i < size; i++)
            __vect.addElement(new ConstSFInt32(value[i]));
        __updateWrite();
    };
    public void set1Value(int index, int value) {
        __set1Value(index, new ConstSFInt32(value));
    };
    public void set1Value(int index, SFInt32 sfInt32) {
        sfInt32.__updateRead();
        __set1Value(index, new ConstSFInt32(sfInt32.value));
    };
    public void set1Value(int index, ConstSFInt32 sfInt32) {
        __set1Value(index, sfInt32);
    };
    public void addValue(int value) {
        __addValue(new ConstSFInt32(value));
    };
    public void addValue(SFInt32 sfInt32) {
        sfInt32.__updateRead();
        __addValue(new ConstSFInt32(sfInt32.value));
    };
    public void addValue(ConstSFInt32 sfInt32) {
        __addValue(sfInt32);
    };
    public void insertValue(int index, int value) {
        __insertValue(index, new ConstSFInt32(value));
    };
    public void insertValue(int index, SFInt32 sfInt32) {
        sfInt32.__updateRead();
        __insertValue(index, new ConstSFInt32(sfInt32.value));
    };
    public void insertValue(int index, ConstSFInt32 sfInt32) {
        __insertValue(index, sfInt32);
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