package vrml.field;
import vrml.*;

public class ConstSFInt32 extends ConstField {
    int value;

    public ConstSFInt32() {} /* only for internal use */
    public ConstSFInt32(int value) {
        this.value = value;
    }
    public int getValue() {
        __updateRead();
        return value;
    }


    public String toString() {
        return String.valueOf(value);
    }

    public void __fromPerl(String str) {
        value = Integer.parseInt(str);
    }

    public String __toPerl() {
        return toString();
    }
}