package vrml.field;
import vrml.*;

public class SFBool extends Field {
    boolean value;

    public SFBool() {}
    public SFBool(boolean value) {
        this.value = value;
    }
    public boolean getValue() {
        __updateRead();
        return value;
    }

    public void setValue(boolean value) {
        this.value = value;
        __updateWrite();
    }
    public void setValue(ConstSFBool sfBool) {
        sfBool.__updateRead();
        value = sfBool.value;
        __updateWrite();
    }
    public void setValue(SFBool sfBool) {
        sfBool.__updateRead();
        value = sfBool.value;
        __updateWrite();
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