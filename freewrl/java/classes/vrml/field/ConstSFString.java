package vrml.field;
import vrml.*;

public class ConstSFString extends ConstField {
    String s;

    public ConstSFString() {} /* only for internal use */
    public ConstSFString(String s) {
        this.s = s;
    }
    public String getValue() {
        __updateRead();
        return s;
    }


    public String toString() {
        return vrml.FWHelper.quote(s);
    }

    public void __fromPerl(String str) {
        s = FWHelper.base64decode(str);
    }

    public String __toPerl() {
        return FWHelper.base64encode(s);
    }
}