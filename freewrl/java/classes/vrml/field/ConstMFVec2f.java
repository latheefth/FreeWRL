package vrml.field;
import vrml.*;
import java.util.StringTokenizer;
import java.util.StringTokenizer;

public class ConstMFVec2f extends ConstMField {

    public ConstMFVec2f() {}
    public ConstMFVec2f(float[] vec2fs) {
        this(vec2fs.length, vec2fs);    }
    public ConstMFVec2f(int size, float[] vec2fs) {
        for (int i = 0; i < size; i += 2)
            __vect.addElement(new ConstSFVec2f(vec2fs[i], vec2fs[i+1]));
    }
    public ConstMFVec2f(float[][] vec2fs) {
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