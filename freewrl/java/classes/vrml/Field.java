package vrml;
import java.io.*;

public abstract class Field implements Cloneable
{
    FWJavaScriptBinding __binding = null;
    //int mftype = 0;	// this is NOT an MF* type field
    
    public Object clone() {
	try {
	    Field f = (Field) super.clone();
	    f.__binding = null;
	    return f;
	} catch (CloneNotSupportedException ex) {
	    throw new InternalError();
	}
    }

    public void bind_to(FWJavaScriptBinding b) {
	__binding = b;
    }

    public final void __updateRead() {
	    System.out.println ("Field, updateRead starting");
	if (__binding != null)
	    __binding.updateRead(this);
    }
    protected final void __updateWrite() {
	if (__binding != null)
	    __binding.updateWrite(this);
    }

    public abstract void __fromPerl(BufferedReader in) throws IOException;
    public abstract void __toPerl(PrintWriter out) throws IOException;
}


