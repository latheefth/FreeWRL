package vrml.field;
import vrml.*;

public class ConstSFBool extends ConstField {
    boolean value;

    public ConstSFBool() {} /* only for internal use */
    public ConstSFBool(boolean value) {
        this.value = value;
    }
    public boolean getValue() {
        __updateRead();
        return value;
    }


    public String toString() {
        return value ? "TRUE" : "FALSE";
    }

    public void __fromPerl(String str) {
        value = str.equals("1");
    }

    public String __toPerl() {
        return value ? "1" : "0";
    }
}