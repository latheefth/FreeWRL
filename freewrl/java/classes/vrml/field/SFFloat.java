package vrml.field;
import vrml.*;

public class SFFloat extends Field {
    float f;

    public SFFloat() {}
    public SFFloat(float f) {
        this.f = f;
    }
    public float getValue() {
        __updateRead();
        return f;
    }

    public void setValue(float f) {
        this.f = f;
        __updateWrite();
    }
    public void setValue(ConstSFFloat sfFloat) {
        sfFloat.__updateRead();
        f = sfFloat.f;
        __updateWrite();
    }
    public void setValue(SFFloat sfFloat) {
        sfFloat.__updateRead();
        f = sfFloat.f;
        __updateWrite();
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