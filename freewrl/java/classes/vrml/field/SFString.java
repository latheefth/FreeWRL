package vrml.field;
import vrml.*;

public class SFString extends Field {
    String s;

    public SFString() {}
    public SFString(String s) {
        this.s = s;
    }
    public String getValue() {
        __updateRead();
        return s;
    }

    public void setValue(String s) {
        this.s = s;
        __updateWrite();
    }
    public void setValue(ConstSFString sfString) {
        sfString.__updateRead();
        s = sfString.s;
        __updateWrite();
    }
    public void setValue(SFString sfString) {
        sfString.__updateRead();
        s = sfString.s;
        __updateWrite();
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