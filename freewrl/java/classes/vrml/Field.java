package vrml;

public abstract class Field implements Cloneable
{
    FWJavaScriptBinding __binding = null;
    
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
	if (__binding != null)
	    __binding.updateRead(this);
    }
    protected final void __updateWrite() {
	if (__binding != null)
	    __binding.updateWrite(this);
    }

    public abstract void __fromPerl(String str);
    public abstract String __toPerl();
}


