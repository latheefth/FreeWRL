package vrml;

//import java.util.*;
//import java.applet.*;
//import java.awt.*;
import java.net.*;
//import java.io.*;
//import java.lang.reflect.*;




import java.lang.reflect.*;
import java.io.*;
import java.util.Hashtable;
import java.util.Vector;
import java.util.Enumeration;
import vrml.*;
import vrml.node.*;

public final class FWJavaScript {
    static Hashtable touched = new Hashtable();
    static String reqid;
    static Browser theBrowser;

	static Socket sock;		// communication socket with FreeWRL
	static PrintWriter EAIout;
	static BufferedReader  EAIin;


    

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
	    EAIout.println("SENDEVENT");
	    EAIout.println(nodeid);
	    EAIout.println(f);
System.out.println ("commented out val.__toPerl");
	    //val.__toPerl(out);
	}
	touched.clear();
	EAIout.println("FINISHED");
	EAIout.println(reqid);
	EAIout.flush();
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
	int counter;
	String reply;

  	// Create a socket here for an EAI/CLASS server on localhost
	sock = null;

	counter = 1;	
	while (sock == null) {
		try {
			//System.out.println ("FWJavaScript trying socket " + argv[0]);
			sock = new Socket("localhost",Integer.parseInt(argv[0]));
		} catch (IOException e) {
			// wait up to 30 seconds for FreeWRL to answer.
			counter = counter + 1;
			if (counter == 10) {
				System.out.println ("FWJavaScript: Java code timed out finding FreeWRL");
				System.exit(1);
			}
			try {Thread.sleep (500);} catch (InterruptedException f) { }
		}
	}

	EAIout = new PrintWriter (sock.getOutputStream());
	EAIin = new BufferedReader( new InputStreamReader(sock.getInputStream()));

	// send along the initial string
	System.out.println ("sending initial string");
	EAIout.println ("freewrl 1.1111 testing");
	EAIout.println ("freewrl 1.1111 testing");
	EAIout.println ("freewrl 1.1111 testing");
	EAIout.println ("freewrl 1.1111 testing");
	EAIout.println ("freewrl 1.1111 testing");
	EAIout.println ("freewrl 1.1111 testing");
	EAIout.flush();

	/* Install security */
	System.setSecurityManager(new SecurityManager());

	/* And Go... */
	theBrowser = new Browser();

	 	Hashtable scripts = new Hashtable();
		EAIout.println("JavaClass version 1.0 - www.crc.ca");
		EAIout.flush();

		while(true) {
			String cmd = EAIin.readLine();
			System.err.println("got ");
			System.err.println("--- "+cmd);
			String nodeid =	EAIin.readLine() + EAIin.readLine();
			if(cmd.equals("NEWSCRIPT")) {
				String url = EAIin.readLine();
				System.out.println("NEWSCRIPT: "+url);
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
				System.out.println ("setting nodeid to " + nodeid);
				scripts.put(nodeid,s);
			} else if(cmd.equals("SETFIELD")) {
			} else if(cmd.equals("INITIALIZE")) {
				Script s = (Script)scripts.get(nodeid);
System.out.println ("got INITIALIZE, past Script s");

				reqid = EAIin.readLine();
System.out.println ("just read in reqid " + reqid);
				s.initialize();
System.out.println ("just initialized");
				send_touched(reqid);
System.out.println ("sent touched");
			} else if(cmd.equals("EVENTSPROCESSED")) {
				Script s = (Script)scripts.get(nodeid);
				reqid = EAIin.readLine();
				s.eventsProcessed();
				send_touched(reqid);
			} else if(cmd.equals("SENDEVENT")) {
				Script s = (Script)scripts.get(nodeid);
				reqid = EAIin.readLine();
				String fname = EAIin.readLine();
				String ftype = EAIin.readLine();
				ConstField fval = 
				    FWCreateField.createConstField(ftype);
System.out.println ("commented out __fromPerl\n");
				//fval.__fromPerl(in);
				double timestamp = 
				    Double.parseDouble(EAIin.readLine());
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
		EAIout.flush();
		}
		//} catch(IOException e) {
		//	System.err.println(e);
		//	throw new Exception("Io error");
		//}
		
	}

    public static String getFieldType(BaseNode node, String fieldname, 
				      String kind)
    {
	try {
	    System.err.println("Java: "+node._get_nodeid()
			       +".getField("+fieldname+")");

	    EAIout.println("GETFIELD");
	    EAIout.println(node._get_nodeid());
	    EAIout.println(fieldname);
	    EAIout.println(kind);
	    EAIout.flush();

	    return EAIin.readLine();
	} catch (IOException e) {
	    throw new InternalError("Communication error: "+e);
	}
    }

    public static void readField(BaseNode node, String fieldName, Field fld) {
	//try {
	    System.err.println("FWJ: "+node._get_nodeid()+".readField("+fieldName+")");
	    FWJavaScript.EAIout.println("READFIELD");
	    FWJavaScript.EAIout.println(node._get_nodeid());
	    FWJavaScript.EAIout.println(fieldName);
	    FWJavaScript.EAIout.flush();
System.out.println ("commented out fromPerl  xx\n");
	    //fld.__fromPerl(in);
	//} catch (IOException e) {
	 //   throw new InternalError("Communication error: "+e);
	//}
    }

    public static String getNodeType(BaseNode node)
    {
	try {
	    System.err.println("creating VRML.");
	    FWJavaScript.EAIout.println("GETTYPE");
	    FWJavaScript.EAIout.println(node._get_nodeid());
	    FWJavaScript.EAIout.flush();
	    return EAIin.readLine();
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
	    FWJavaScript.EAIout.println("CREATEVRML");
	    FWJavaScript.EAIout.println(vrmlSyntax);
	    FWJavaScript.EAIout.flush();
	    String intstring = FWJavaScript.EAIin.readLine();
	    int number = Integer.parseInt(intstring);
	    if (number == -1)
		throw new InvalidVRMLSyntaxException(EAIin.readLine());
	    Node[] nodes = new Node[number];
	    for (int i = 0; i < number; i++)
		nodes[i] = new Node(EAIin.readLine());
	    return nodes;
	} catch (IOException e) {
	    throw new InternalError("Communication error: "+e);
	}
    }
}
