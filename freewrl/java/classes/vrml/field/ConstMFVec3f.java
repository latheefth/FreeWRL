package vrml.field;
import vrml.*;
import java.util.StringTokenizer;
import java.util.StringTokenizer;

public class ConstMFVec3f extends ConstMField {

    public ConstMFVec3f() {}
    public ConstMFVec3f(float[] vec3fs) {
        this(vec3fs.length, vec3fs);    }
    public ConstMFVec3f(int size, float[] vec3fs) {
        for (int i = 0; i < size; i += 3)
            __vect.addElement(new ConstSFVec3f(vec3fs[i], vec3fs[i+1], vec3fs[i+2]));
    }
    public ConstMFVec3f(float[][] vec3fs) {
        for (int i = 0; i < vec3fs.length; i++)
            __vect.addElement(new ConstSFVec3f(vec3fs[i][0], vec3fs[i][1], vec3fs[i][2]));
    }
    public void getValue(float[] vec3fs) {
        __updateRead();
        int size = __vect.size();
        for (int i = 0; i < size; i++) {
            ConstSFVec3f sfVec3f = (ConstSFVec3f) __vect.elementAt(i);
            vec3fs[3*i+0] = sfVec3f.getX();
            vec3fs[3*i+1] = sfVec3f.getY();
            vec3fs[3*i+2] = sfVec3f.getZ();
        }
    }

    public void getValue(float[][] vec3fs) {
        __updateRead();
        int size = __vect.size();
        for (int i = 0; i < size; i++)
            ((ConstSFVec3f) __vect.elementAt(i)).getValue(vec3fs[i]);
    }

    public void get1Value(int index, float[] vec3fs) {
        __update1Read(index);
        ((ConstSFVec3f) __vect.elementAt(index)).getValue(vec3fs);
    }
    public void get1Value(int index, SFVec3f sfVec3f) {
        __update1Read(index);
        sfVec3f.setValue((ConstSFVec3f) __vect.elementAt(index));
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
            ConstSFVec3f sf = new ConstSFVec3f();
            sf.__fromPerl(st.nextToken());
            __vect.addElement(sf);
        }
    }

    public String __toPerl() {
        StringBuffer sb = new StringBuffer("");
        int size = __vect.size();
        for (int i = 0; i < size; i++) {
            if (i > 0) sb.append(",");
            sb.append(((ConstSFVec3f) __vect.elementAt(i)).__toPerl());
        }
        return sb.append("").toString();
    }
}