//AUTOMATICALLY GENERATED BY genfields.pl.
//DO NOT EDIT!!!!

package vrml.field;
import vrml.*;
import java.io.BufferedReader;
import java.io.PrintWriter;
import java.io.IOException;

public class SFString extends Field {
    String s;

    public SFString() {
    }
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
        __updateRead();
        return vrml.FWHelper.quote(s);
    }

    public void __fromPerl(BufferedReader in)  throws IOException {
        
		System.out.println ("fromPerl, String");
		s = in.readLine();
    }

    public void __toPerl(PrintWriter out)  throws IOException {
        out.print(s);
    }
}