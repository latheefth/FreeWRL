package vrml.field;
import vrml.*;

public class SFInt32 extends Field {
    int value;

    public SFInt32() {}
    public SFInt32(int value) {
        this.value = value;
    }
    public int getValue() {
        __updateRead();
        return value;
    }

    public void setValue(int value) {
        this.value = value;
        __updateWrite();
    }
    public void setValue(ConstSFInt32 sfInt32) {
        sfInt32.__updateRead();
        value = sfInt32.value;
        __updateWrite();
    }
    public void setValue(SFInt32 sfInt32) {
        sfInt32.__updateRead();
        value = sfInt32.value;
        __updateWrite();
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