package vrml;
import java.lang.reflect.*;
import java.io.*;
import java.util.Hashtable;
import java.util.Vector;
import java.util.Enumeration;
import vrml.*;
import vrml.node.*;

public final class FWJavaScript {
    static Hashtable touched = new Hashtable();
    static PrintWriter out;
    static LineNumberReader in;
    static String reqid;
    

    public static void add_touched(Field f) {
	touched.put(f, Boolean.TRUE);
    }

    public static void send_touched(String reqid) {
	// System.err.println("send_touched\n");
	Enumeration e = touched.keys();
	while(e.hasMoreElements()) {
	    // System.err.println("send_touched one\n");
	    Field val = (Field) e.nextElement();
	    FWJavaScriptBinding b = val.__binding;
	    BaseNode n = b.node();
	    String f = b.field();
	    String nodeid = n._get_nodeid();
	    out.println("SENDEVENT");
	    out.println(nodeid);
	    out.println(f);
	    out.println(val.__toPerl());
	}
	touched.clear();
	out.println("FINISHED");
	out.println(reqid);
	out.flush();
    }

    public static void main (String argv[]) 
	throws ClassNotFoundException,
	       NoSuchMethodException,
	       InstantiationException,
	       IllegalAccessException,
	       InvocationTargetException,
	       Exception,
	       Throwable
    {
	out = new PrintWriter(System.out);
	System.setOut(System.err);
	//new PrintWriter(new FileOutputStream(".javapipej")));
	in = new LineNumberReader(new InputStreamReader(System.in));
	//new LineNumberReader(new InputStreamReader(new FileInputStream(".javapipep")));
	 	Hashtable scripts = new Hashtable();
		// String dirname = argv[0];
		// out.println("FWJavaScript says: Hello World! dirname is " + dirname);
//		Class mr = Class.forName("MuchRed");
//		Constructor mrc = mr.getConstructor(
//			new Class[] {});
//		Script c = (Script)mrc.newInstance(new Class[] {});
//		c.eventsProcessed();
//		out.println("Goodbye World!\n");
		try {
		String ver = in.readLine().trim();
		System.err.println("Got a line:'"+ver+"'");
		// Stupid handshake - just make sure about protocol.
		// Change this often between versions.
		if(!ver.equals("TJL XXX PERL-JAVA 0.02")) {
			throw new Exception(
				"Wrong script version '"+ver+"'!"
			);
		}
		System.err.println("Sending a line:'"+ver+"'");
		out.println("TJL XXX JAVA-PERL 0.03");
		System.err.println("Sent a line:'"+ver+"'");
		out.flush();
		while(true) {
			String cmd = in.readLine();
			//System.err.println("got ");
			//System.err.println("--- "+cmd);
			cmd = cmd.trim();
			String nodeid =	in.readLine().trim();
			if(cmd.equals("NEWSCRIPT")) {
				String url = in.readLine().trim();
				System.err.println("NEWSCRIPT: "+url);
				FWJavaScriptClassLoader classloader = 
				    new FWJavaScriptClassLoader(url);
				String classname
				    = url.substring(url.lastIndexOf('/')+1);
				if (classname.endsWith(".class"))
				    classname = classname
					.substring(0, classname.length() - 6);
				Script s;
				try {
				    s = (Script) classloader
					.loadClass(classname).newInstance();
				} catch (Exception ex) {
				    System.err.println("Can't load script: "
						       + url);
				    throw ex;
				}
				s._set_nodeid(nodeid);
				scripts.put(nodeid,s);
			} else if(cmd.equals("SETFIELD")) {
			} else if(cmd.equals("INITIALIZE")) {
				Script s = (Script)scripts.get(nodeid);
				reqid = in.readLine().trim();
				s.initialize();
				send_touched(reqid);
			} else if(cmd.equals("EVENTSPROCESSED")) {
				Script s = (Script)scripts.get(nodeid);
				reqid = in.readLine().trim();
				s.eventsProcessed();
				send_touched(reqid);
			} else if(cmd.equals("SENDEVENT")) {
				Script s = (Script)scripts.get(nodeid);
				reqid = in.readLine().trim();
				String fname = in.readLine().trim();
				String ftype = in.readLine().trim();
				String fs = in.readLine().trim();
				ConstField fval = 
				    FWCreateField.createConstField(ftype);
				fval.__fromPerl(fs);
				double timestamp = new 
				    Double(in.readLine().trim()).doubleValue();
				Event ev = new Event(
					fname, 
					timestamp,
					fval
				);
				s.processEvent(ev);
				send_touched(reqid);
			} else {
				throw new Exception("Invalid command '"
						+ cmd + "'");
			}
		}
		} catch(IOException e) {
			System.err.println(e);
			throw new Exception("Io error");
		}
		
	}

    public static String getFieldType(BaseNode node, String fieldname, 
				      String kind)
    {
	try {
	    System.err.println("Java: "+node._get_nodeid()
			       +".getField("+fieldname+")");

	    out.println("GETFIELD");
	    out.println(node._get_nodeid());
	    out.println(fieldname);
	    out.println(kind);
	    out.flush();

	    return in.readLine().trim();
	} catch (IOException e) {
	    throw new InternalError("Communication error: "+e);
	}
    }

    public static String readField(BaseNode node, String fieldName) {
	try {
	    System.err.println("FWJ: "+node._get_nodeid()+".readField("+fieldName+")");
	    FWJavaScript.out.println("READFIELD");
	    FWJavaScript.out.println(node._get_nodeid());
	    FWJavaScript.out.println(fieldName);
	    FWJavaScript.out.flush();
	    
	    String value = FWJavaScript.in.readLine().trim();
	    System.err.println("FWJ value: "+value);
	    return value;
	} catch (IOException e) {
	    throw new InternalError("Communication error: "+e);
	}
    }

    public static Browser getBrowser(BaseNode node)
    {
	try {
	    System.err.println("Java: "+node._get_nodeid()+".getBrowser()");

	    out.println("GETBROWSER");
	    out.println(node._get_nodeid());
	    out.flush();

	    return Browser.lookup(in.readLine().trim());
	} catch (IOException e) {
	    throw new InternalError("Communication error: "+e);
	}
    }


    public static BaseNode[] createVrmlFromString(String browserID, 
						  String vrmlSyntax) 
	throws InvalidVRMLSyntaxException
    {
	try {
	    System.err.println("creating VRML.");
	    FWJavaScript.out.println("CREATEVRML");
	    FWJavaScript.out.println(browserID);
	    FWJavaScript.out.println(FWHelper.base64encode(vrmlSyntax));
	    FWJavaScript.out.flush();
	    int number = Integer.parseInt(in.readLine().trim());
	    if (number == -1)
		throw new InvalidVRMLSyntaxException(in.readLine().trim());
	    Node[] nodes = new Node[number];
	    for (int i = 0; i < number; i++)
		nodes[i] = new Node(in.readLine().trim());
	    return nodes;
	} catch (IOException e) {
	    throw new InternalError("Communication error: "+e);
	}
    }
}
