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
    static Browser theBrowser = new Browser();
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
		if(!ver.equals("TJL XXX PERL-JAVA 0.01")) {
			throw new Exception(
				"Wrong script version '"+ver+"'!"
			);
		}
		System.err.println("Sending a line:'"+ver+"'");
		out.println("TJL XXX JAVA-PERL 0.01");
		System.err.println("Sent a line:'"+ver+"'");
		out.flush();
		String dir = in.readLine().trim();
		System.err.println("GOT DIRl a line:'"+dir+"'");
		FWJavaScriptClassLoader tl = new FWJavaScriptClassLoader(dir,
			dir.getClass().getClassLoader());
		out.flush();
		while(true) {
			String cmd = in.readLine();
			//System.err.println("got ");
			//System.err.println("--- "+cmd);
			cmd = cmd.trim();
			String nodeid =	in.readLine().trim();
			if(cmd.equals("NEWSCRIPT")) {
				String url = in.readLine().trim();
				System.err.println("NEWSCRIPT");
				Constructor mrc = tl.loadFile(url).
					getConstructor(new Class[0]);
				System.err.println("GOt constructor");
				Script s = (Script)mrc.newInstance(
						new Class[0]);
				s._set_nodeid(nodeid);
				System.err.println("GOt instance");
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
				Constructor cons = 
					Class.forName("vrml.field.Const"+ftype).
					 getConstructor(new Class[0]);
				ConstField fval;
				try {
				    fval = (vrml.ConstField)cons.newInstance(new Object[0]);
				} catch(InvocationTargetException e) {
				    throw e.getTargetException();
				}
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

    public static Field getField(BaseNode node, 
				 String fieldname, String type) {
	try {
	    System.err.println("FWJ: "+node._get_nodeid()+".getField("+fieldname+")");
	    out.println("GETFIELD");
	    out.println(node._get_nodeid());
	    out.println(fieldname);
	    out.println(type);
	    out.flush();
	    
	    String ftype = in.readLine().trim();
	    if (ftype.equals("ILLEGAL"))
		throw new InvalidFieldException(""+node._get_nodeid()+": "
						+type+" "+fieldname);
	    
	    String cname = "vrml.field."+ftype;
	    System.err.println("CONS FIELD "+cname);
	    Constructor cons = Class.forName(cname)
		.getConstructor(new Class[0]);
	    System.err.println("GOt fieldcons");
	    Field fval;
	    try {
		fval = (vrml.Field)cons.newInstance(new Object[0]);
	    } catch(Exception e) {
		throw new InternalError("Can't create field: "+e);
	    }
	    fval.bind_to(new FWJavaScriptBinding(node, fieldname));
	    return fval;
	} catch (ClassNotFoundException e) {
	    throw new NoClassDefFoundError(e.toString());
	} catch (NoSuchMethodException e) {
	    throw new NoSuchMethodError(e.toString());
	} catch (IOException e) {
	    throw new InternalError("Communication error: "+e);
	}
    }

    public static ConstField getEventOut(BaseNode node, String fieldname) {
	try {
	    //System.err.println("FWJ: "+node+".getField("+fieldname+")");
	    out.println("GETFIELD");
	    out.println(node._get_nodeid());
	    out.println(fieldname);
	    out.println("eventOut");
	    out.flush();
	    
	    String ftype = in.readLine().trim();
	    if (ftype.equals("ILLEGAL"))
		throw new InvalidEventOutException(""+node._get_nodeid()+": "
						   +fieldname);
	    
	    String cname = "vrml.field.Const"+ftype;
	    //System.err.println("CONS FIELD "+cname);
	    Constructor cons = Class.forName(cname)
		.getConstructor(new Class[0]);
	    //System.err.println("GOt fieldcons");
	    ConstField fval;
	    try {
		fval = (vrml.ConstField)cons.newInstance(new Object[0]);
	    } catch(Exception e) {
		throw new InternalError("Can't create field.");
	    }
	    fval.bind_to(new FWJavaScriptBinding(node, fieldname));
	    return fval;
	} catch (ClassNotFoundException e) {
	    throw new NoClassDefFoundError(e.toString());
	} catch (NoSuchMethodException e) {
	    throw new NoSuchMethodError(e.toString());
	} catch (IOException e) {
	    throw new InternalError("Communication error: "+e);
	}
    }
}

