package vrml.field;
import vrml.*;

public class SFTime extends Field {
    double value;

    public SFTime() {}
    public SFTime(double value) {
        this.value = value;
    }
    public double getValue() {
        __updateRead();
        return value;
    }

    public void setValue(double value) {
        this.value = value;
        __updateWrite();
    }
    public void setValue(ConstSFTime sfTime) {
        sfTime.__updateRead();
        value = sfTime.value;
        __updateWrite();
    }
    public void setValue(SFTime sfTime) {
        sfTime.__updateRead();
        value = sfTime.value;
        __updateWrite();
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