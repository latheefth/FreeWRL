package vrml;

import java.net.*;
import java.lang.System;
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
	//static PrintWriter EAIout;
	//static BufferedReader  EAIin;
	static DataInputStream EAIin;
	static DataOutputStream EAIout;


    

    public static void add_touched(Field f) {
	touched.put(f, Boolean.TRUE);
    }

    public static void send_touched(String reqid) throws IOException {
	// System.out.println("send_touched\n");
	Enumeration e = touched.keys();
	while(e.hasMoreElements()) {
	    // System.out.println("send_touched one\n");
	    Field val = (Field) e.nextElement();
	    FWJavaScriptBinding b = val.__binding;
	    BaseNode n = b.node();
	    String f = b.field();
	    String nodeid = n._get_nodeid();
	    EAIout.writeBytes("SENDEVENT");
	    EAIout.writeBytes(nodeid);
	    EAIout.writeBytes(f);
System.out.println ("commented out val.__toPerl");
	    //val.__toPerl(out);
	}
	touched.clear();
	EAIout.writeBytes("FIN "+reqid);
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
	String nodeid = "";

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

	//EAIout = new PrintWriter (sock.getOutputStream());
	//EAIin = new BufferedReader( new InputStreamReader(sock.getInputStream()));
	EAIout = new DataOutputStream (sock.getOutputStream());
	EAIin = new DataInputStream(sock.getInputStream());

	/* Install security */
	System.out.println ("Security manager commented out");
	//System.setSecurityManager(new SecurityManager());

	/* And Go... */
	theBrowser = new Browser();

	 	Hashtable scripts = new Hashtable();
		EAIout.writeBytes("JavaClass version 1.0 - www.crc.ca");
		EAIout.flush();

		while(true) {
			String cmd = EAIin.readLine();

			// did FreeWRL leave us?
			if (cmd == null) {
				System.out.println ("have null string  exiting...\n"); 
				System.exit(1);
			}

			System.out.println("got ");
			System.out.println("--- "+cmd);

			nodeid =EAIin.readLine() + EAIin.readLine();

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
				    System.out.println("Can't load script: "
						       + url);
				    throw ex;
				}
				s._set_nodeid(nodeid);
				System.out.println ("setting nodeid to " + nodeid);
				scripts.put(nodeid,s);
			} else if(cmd.equals("SETFIELD")) {
			} else if(cmd.equals("INITIALIZE")) {
				Script s = (Script)scripts.get(nodeid);
				reqid = EAIin.readLine();
				s.initialize();
				send_touched(reqid);
			} else if(cmd.equals("EVENTSPROCESSED")) {
				Script s = (Script)scripts.get(nodeid);
				reqid = EAIin.readLine();
				s.eventsProcessed();
				send_touched(reqid);
			} else if(cmd.equals("SENDEVENT")) {
				System.out.println ("FWJ - got a SENDEVENT");
				System.out.println ("NodeID is " + nodeid);
				Script s = (Script)scripts.get(nodeid);
				System.out.println ("FWJ - got the script from nodeid");
				reqid = EAIin.readLine();
				String fname = EAIin.readLine();
				String ftype = EAIin.readLine();
				System.out.println ("FWJ, got SENDEVENT, script " + nodeid
						+ " field " + fname + " type " + ftype);

				ConstField fval = 
				    FWCreateField.createConstField(ftype);
				fval.__fromPerl(EAIin);
				double timestamp = 
				    Double.parseDouble(EAIin.readLine());
				Event ev = new Event(
					fname, 
					timestamp,
					fval
				);
				System.out.println ("FWJ: calling processEvent");
				s.processEvent(ev);
				System.out.println ("FWJ: sending touched");
				send_touched(reqid);
				System.out.println ("JWJ: finished sendevent");
			} else {
				throw new Exception("Invalid command '"
						+ cmd + "'");
			}
		EAIout.flush();
		}
	}

    public static String getFieldType(BaseNode node, String fieldname, 
				      String kind)
    {
	try {
	    System.out.println ();
	    System.out.println("FWJ:start getFieldType "+node._get_nodeid()
			       +".getField("+fieldname+")");

	    EAIout.writeBytes("GF " + node._get_nodeid() + " " + fieldname + " " + kind + "\n");
	    EAIout.writeBytes(kind);
	    EAIout.flush();

	    System.out.println ("FWJ: getFieldType complete for" +node._get_nodeid()
			                                   +".getField("+fieldname+")");
	    return EAIin.readLine();
	} catch (IOException e) {
	    throw new InternalError("Communication error: "+e);
	}
    }

    public static void readField(BaseNode node, String fieldName, Field fld) {
	try {
	    System.out.println ();
	    System.out.println("FWJ:start readField "+
			    node._get_nodeid()+".readField("+fieldName+") " +
			    fld);
	    FWJavaScript.EAIout.writeBytes("RF " + 
			    node._get_nodeid() + " " + fieldName + "\n");
	    FWJavaScript.EAIout.flush();
	    fld.__fromPerl(EAIin);
	} catch (IOException e) {
	    throw new InternalError("Communication error: "+e);
	}
	System.out.println ("FWJ: readField complete for " +node._get_nodeid()+".readField("+fieldName+")");
    }

    public static String getNodeType(BaseNode node)
    {
	try {
	    System.out.println("FWJ: creating VRML.");
	    FWJavaScript.EAIout.writeBytes("GT "+ node._get_nodeid() + "\n");
	    FWJavaScript.EAIout.flush();
	    System.out.println("FWJ: getNodeType complete");
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
	    FWJavaScript.EAIout.writeBytes("CV");
	    FWJavaScript.EAIout.writeBytes(vrmlSyntax);
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
