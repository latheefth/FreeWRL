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
    static DataOutputStream out;
    static DataInputStream  in;
    static String reqid;
    static Browser theBrowser;
    

    public static void add_touched(Field f) {
	touched.put(f, Boolean.TRUE);
    }

    public static void send_touched(String reqid) throws IOException {
	// System.err.println("send_touched\n");
	Enumeration e = touched.keys();
	while(e.hasMoreElements()) {
	    // System.err.println("send_touched one\n");
	    Field val = (Field) e.nextElement();
	    FWJavaScriptBinding b = val.__binding;
	    BaseNode n = b.node();
	    String f = b.field();
	    String nodeid = n._get_nodeid();
	    out.writeUTF("SENDEVENT");
	    out.writeUTF(nodeid);
	    out.writeUTF(f);
	    val.__toPerl(out);
	}
	touched.clear();
	out.writeUTF("FINISHED");
	out.writeUTF(reqid);
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
	in  = new DataInputStream(System.in);
	out = new DataOutputStream(System.out);

	/* Protect our communication channels */
	System.setOut(System.err);
	System.setIn(new ByteArrayInputStream(new byte[0]));
	
	/* Install security */
	System.setSecurityManager(new SecurityManager());

	/* And Go... */
	theBrowser = new Browser();

	 	Hashtable scripts = new Hashtable();
		// String dirname = argv[0];
		// out.writeUTF("FWJavaScript says: Hello World! dirname is " + dirname);
//		Class mr = Class.forName("MuchRed");
//		Constructor mrc = mr.getConstructor(
//			new Class[] {});
//		Script c = (Script)mrc.newInstance(new Class[] {});
//		c.eventsProcessed();
//		out.writeUTF("Goodbye World!\n");
		try {
		String ver = in.readUTF();
		System.err.println("Got a line:'"+ver+"'");
		// Stupid handshake - just make sure about protocol.
		// Change this often between versions.
		if(!ver.equals("TJL XXX PERL-JAVA 0.10")) {
			throw new Exception(
				"Wrong script version '"+ver+"'!"
			);
		}
		System.err.println("Sending a line:'"+ver+"'");
		out.writeUTF("TJL XXX JAVA-PERL 0.10");
		System.err.println("Sent a line:'"+ver+"'");
		out.flush();
		while(true) {
			String cmd = in.readUTF();
			//System.err.println("got ");
			//System.err.println("--- "+cmd);
			String nodeid =	in.readUTF();
			if(cmd.equals("NEWSCRIPT")) {
				String url = in.readUTF();
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
				reqid = in.readUTF();
				s.initialize();
				send_touched(reqid);
			} else if(cmd.equals("EVENTSPROCESSED")) {
				Script s = (Script)scripts.get(nodeid);
				reqid = in.readUTF();
				s.eventsProcessed();
				send_touched(reqid);
			} else if(cmd.equals("SENDEVENT")) {
				Script s = (Script)scripts.get(nodeid);
				reqid = in.readUTF();
				String fname = in.readUTF();
				String ftype = in.readUTF();
				ConstField fval = 
				    FWCreateField.createConstField(ftype);
				fval.__fromPerl(in);
				double timestamp = 
				    Double.parseDouble(in.readUTF());
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

	    out.writeUTF("GETFIELD");
	    out.writeUTF(node._get_nodeid());
	    out.writeUTF(fieldname);
	    out.writeUTF(kind);
	    out.flush();

	    return in.readUTF();
	} catch (IOException e) {
	    throw new InternalError("Communication error: "+e);
	}
    }

    public static void readField(BaseNode node, String fieldName, Field fld) {
	try {
	    System.err.println("FWJ: "+node._get_nodeid()+".readField("+fieldName+")");
	    FWJavaScript.out.writeUTF("READFIELD");
	    FWJavaScript.out.writeUTF(node._get_nodeid());
	    FWJavaScript.out.writeUTF(fieldName);
	    FWJavaScript.out.flush();
	    fld.__fromPerl(in);
	} catch (IOException e) {
	    throw new InternalError("Communication error: "+e);
	}
    }

    public static String getNodeType(BaseNode node)
    {
	try {
	    System.err.println("creating VRML.");
	    FWJavaScript.out.writeUTF("GETTYPE");
	    FWJavaScript.out.writeUTF(node._get_nodeid());
	    FWJavaScript.out.flush();
	    return in.readUTF();
	} catch (IOException e) {
	    throw new InternalError("Communication error: "+e);
	}
    }

    public static Browser getBrowser()
    {
	return theBrowser;
    }


    public static BaseNode[] createVrmlFromString(String vrmlSyntax) 
	throws InvalidVRMLSyntaxException
    {
	try {
	    FWJavaScript.out.writeUTF("CREATEVRML");
	    FWJavaScript.out.writeUTF(vrmlSyntax);
	    FWJavaScript.out.flush();
	    int number = in.readInt();
	    if (number == -1)
		throw new InvalidVRMLSyntaxException(in.readUTF());
	    Node[] nodes = new Node[number];
	    for (int i = 0; i < number; i++)
		nodes[i] = new Node(in.readUTF());
	    return nodes;
	} catch (IOException e) {
	    throw new InternalError("Communication error: "+e);
	}
    }
}
