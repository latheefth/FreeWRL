package vrml.field;
import vrml.*;
import java.util.StringTokenizer;
import java.util.StringTokenizer;

public class MFVec2f extends MField {

    public MFVec2f() {}
    public MFVec2f(float[] vec2fs) {
        this(vec2fs.length, vec2fs);
    }
    public MFVec2f(int size, float[] vec2fs) {
        for (int i = 0; i < size; i += 2)
            __vect.addElement(new ConstSFVec2f(vec2fs[i], vec2fs[i+1]));
    }
    public MFVec2f(float[][] vec2fs) {
        for (int i = 0; i < vec2fs.length; i++)
            __vect.addElement(new ConstSFVec2f(vec2fs[i][0], vec2fs[i][1]));
    }

    public void getValue(float[] vec2fs) {
        __updateRead();
        int size = __vect.size();
        for (int i = 0; i < size; i++) {
            ConstSFVec2f sfVec2f = (ConstSFVec2f) __vect.elementAt(i);
            vec2fs[2*i+0] = sfVec2f.getX();
            vec2fs[2*i+1] = sfVec2f.getY();
        }
    }
    public void getValue(float[][] vec2fs) {
        __updateRead();
        int size = __vect.size();
        for (int i = 0; i < size; i++)
            ((ConstSFVec2f) __vect.elementAt(i)).getValue(vec2fs[i]);
    }

    public void get1Value(int index, float[] vec2fs) {
        __update1Read(index);
        ((ConstSFVec2f) __vect.elementAt(index)).getValue(vec2fs);
    }
    public void get1Value(int index, SFVec2f sfVec2f) {
        __update1Read(index);
        sfVec2f.setValue((ConstSFVec2f) __vect.elementAt(index));
    }

    public void setValue(float[] vec2fs) {
        setValue(vec2fs.length, vec2fs);
    }
    public void setValue(int size, float[] vec2fs) {
        __vect.clear();
        for (int i = 0; i < size; i += 2)
            __vect.addElement(new ConstSFVec2f(vec2fs[i], vec2fs[i+1]));
        __updateWrite();
    };
    public void set1Value(int index, float x, float y) {
        __set1Value(index, new ConstSFVec2f(x, y));
    };
    public void set1Value(int index, SFVec2f sfVec2f) {
        sfVec2f.__updateRead();
        __set1Value(index, new ConstSFVec2f(sfVec2f.x, sfVec2f.y));
    };
    public void set1Value(int index, ConstSFVec2f sfVec2f) {
        __set1Value(index, sfVec2f);
    };
    public void addValue(float x, float y) {
        __addValue(new ConstSFVec2f(x, y));
    };
    public void addValue(SFVec2f sfVec2f) {
        sfVec2f.__updateRead();
        __addValue(new ConstSFVec2f(sfVec2f.x, sfVec2f.y));
    };
    public void addValue(ConstSFVec2f sfVec2f) {
        __addValue(sfVec2f);
    };
    public void insertValue(int index, float x, float y) {
        __insertValue(index, new ConstSFVec2f(x, y));
    };
    public void insertValue(int index, SFVec2f sfVec2f) {
        sfVec2f.__updateRead();
        __insertValue(index, new ConstSFVec2f(sfVec2f.x, sfVec2f.y));
    };
    public void insertValue(int index, ConstSFVec2f sfVec2f) {
        __insertValue(index, sfVec2f);
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
            ConstSFVec2f sf = new ConstSFVec2f();
            sf.__fromPerl(st.nextToken());
            __vect.addElement(sf);
        }
    }

    public String __toPerl() {
        StringBuffer sb = new StringBuffer("");
        int size = __vect.size();
        for (int i = 0; i < size; i++) {
            if (i > 0) sb.append(",");
            sb.append(((ConstSFVec2f) __vect.elementAt(i)).__toPerl());
        }
        return sb.append("").toString();
    }
}