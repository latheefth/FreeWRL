package vrml;
import java.io.*;
import java.net.URL;
import java.net.MalformedURLException;
import java.util.Enumeration;
import java.util.NoSuchElementException;

public final class FWJavaScriptClassLoader extends ClassLoader {
    URL baseURL;
    
    /**
     * @param url base url for loading classes.
     */
    public FWJavaScriptClassLoader(String url) {
	try {
	    baseURL = new URL(url);
	} catch (MalformedURLException ex) {
	    throw new InternalError("Script URL malformed: "+url);
	}
    }

    protected Class findClass(String name)
	throws ClassNotFoundException
    {
	System.err.println("LOADING CLASS '"+name+"'");
	try {
	    byte[] b = readFile(name.replace('.', '/') + ".class");
	    return defineClass(name, b, 0, b.length);
	} catch (IOException e) {
	    throw new ClassNotFoundException(name);
	}
    }

    private byte[] readFile(String name) throws IOException
    {
	InputStream is = getResourceAsStream(name);
	ByteArrayOutputStream bao = new ByteArrayOutputStream();
	byte[] buff = new byte[4096];
	int len;
	while ((len = is.read(buff)) > 0)
	    bao.write(buff, 0, len);
	return bao.toByteArray();
    }

    protected URL findResource(String name)
    {
	try {
	    System.err.println("LOADING RESOURCE '"+name+"'");
	    return new URL(baseURL, name);
	} catch (MalformedURLException ex) {
	    return null;
	}
    }

    protected Enumeration findResources(String name) throws IOException
    {
	final URL url = new URL(baseURL, name);
	return new Enumeration() {
		boolean hasMore = true;
		public boolean hasMoreElements() {
		    return hasMore;
		}
		public Object nextElement() {
		    if (!hasMore)
			throw new NoSuchElementException();
		    hasMore = false;
		    return url;
		}
	    };
    }
}
