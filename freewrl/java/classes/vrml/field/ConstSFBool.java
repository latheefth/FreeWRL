//AUTOMATICALLY GENERATED BY genfields.pl.
//DO NOT EDIT!!!!

package vrml.field;
import vrml.*;
import java.io.BufferedReader;
import java.io.PrintWriter;
import java.io.IOException;

public class ConstSFBool extends ConstField {
    boolean value;

    public ConstSFBool() {
    }
    public ConstSFBool(boolean value) {
        this.value = value;
    }

    public boolean getValue() {
        __updateRead();
        return value;
    }

    public String toString() {
        __updateRead();
        return value ? "TRUE" : "FALSE";
    }

    public void __fromPerl(BufferedReader in)  throws IOException {
        
			String myline;
			System.out.println ("fromPerl, Bool");
			myline = in.readLine();
			// direct from perl, will be 0 or 1, from a route, TRUE, FALSE
			value = (myline.equals("TRUE") || myline.equals("1"));
			//System.out.println ("reading in a boolean value is " + value 
		          //      + " for string " + myline);
		
    }

    public void __toPerl(PrintWriter out)  throws IOException {
        out.print (""+value);
    }
}