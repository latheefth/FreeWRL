package vrml.field;
import vrml.*;

public class ConstSFTime extends ConstField {
    double value;

    public ConstSFTime() {} /* only for internal use */
    public ConstSFTime(double value) {
        this.value = value;
    }
    public double getValue() {
        __updateRead();
        return value;
    }


    public String toString() {
        return String.valueOf(value);
    }

    public void __fromPerl(String str) {
        value = new Double(str).doubleValue();
    }

    public String __toPerl() {
        return toString();
    }
}