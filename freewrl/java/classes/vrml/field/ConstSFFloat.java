package vrml.field;
import vrml.*;

public class ConstSFFloat extends ConstField {
    float f;

    public ConstSFFloat() {} /* only for internal use */
    public ConstSFFloat(float f) {
        this.f = f;
    }
    public float getValue() {
        __updateRead();
        return f;
    }


    public String toString() {
        return String.valueOf(f);
    }

    public void __fromPerl(String str) {
        f = new Float(str).floatValue();
    }

    public String __toPerl() {
        return toString();
    }
}