// Specification of the External Interface for a VRML applet browser.
// FreeWRL Viewer Interface - bypass netscape and go directly
// to the viewer. 

package vrml.external;


import java.util.*;
import java.applet.*;
import java.awt.*;
import java.net.*;
import java.io.*;
import java.lang.reflect.*;
import vrml.external.Node;
import vrml.external.field.EventOut;
import vrml.external.field.EventOutObserver;
import vrml.external.field.EventInMFNode;
import vrml.external.FreeWRLEAI.EAIoutThread;
import vrml.external.FreeWRLEAI.EAIinThread;
import vrml.external.FreeWRLEAI.EAIAsyncThread;

public synchronized class Browser
{
    // The thread that reads and processes FreeWRL EAI replies...
     Thread 		FreeWRLThread;

    // The Thread that sends stuff to the EAI port for FreeWRL...
    static EAIoutThread 		EAIoutSender;


    // The following are used to send to/from the FreeWLR Browser...
    ServerSocket	EAISocket;
    Socket		sock;
    public static PrintStream         EAIout;

    // The following pipe listens for replies to events sent to
    // the FreeWRL VRML viewer via the EAI port.

    private PipedInputStream EAIfromFreeWRLStream;
    public static DataInputStream EAIfromFreeWRLInputStream;

    private String              reply = "";

    // Sending to FreeWRL needs to synchronize on an object;
    
    // static Object BrowserGlobals.FreeWRLToken = new Object();

    // Associates this instance with the first embedded plugin in the current frame.
    public Browser(Applet pApplet) {



  	// Create a socket here for an EAI server on localhost
	int incrport = -1;
	EAISocket = null;

	while ((EAISocket == null) && (incrport < 30))
  	try {
		incrport = incrport + 1;
  		EAISocket = new ServerSocket(2000 + incrport);
  	} catch (IOException e) {
  	  System.out.print ("Error creating socket for FreeWRL EAI on port " + incrport + "\n");
  	}
	System.out.println ("opened port on port " + incrport );
  
  	try {
  		sock=EAISocket.accept();
  	} catch (IOException e) {
  	  System.out.print ("error creating sub-scoket in FreeWrl Javascript\n");
  	}
  
  	// Start the readfrom FREEWRL thread...
   	FreeWRLThread = new Thread ( new EAIinThread(sock, pApplet));
           FreeWRLThread.start();
  
  	// Open the pipe for EAI replies to be sent to us...
        try {
          EAIfromFreeWRLStream = new PipedInputStream (EAIinThread.EAItoBrowserStream);
          EAIfromFreeWRLInputStream = new DataInputStream (EAIfromFreeWRLStream);	
        } catch (IOException ie) {
          System.out.println (ie);
        }
  
  
  	// Wait for the FreeWRL browser to send us something...
        try {
          System.out.println (EAIfromFreeWRLInputStream.readLine());
        } catch (IOException ie) {System.out.println (ie);}
  
  	// Send the correct response...
	try {
		EAIout = new PrintStream (sock.getOutputStream());
		EAIout.print ("FreeWRL EAI Serv0.21");
		EAIout.flush ();
	} catch (IOException e) {
		System.out.print ("error on reiniting output stream");
	}
  	// Browser is "gotten", and is started.

  	// Start the SendTo FREEWRL thread...
	EAIoutSender = new EAIoutThread(EAIout);
        EAIoutSender.start();

	// Start the thread that allows Registered Listenered
	// updates to come in.
	BrowserGlobals.RL_Async = new EAIAsyncThread();
	BrowserGlobals.RL_Async.start();

	
  	return;
    }
  
    // construct an instance of the Browser class
    // If frameName is NULL, current frame is assumed.
    public Browser(Applet pApplet, String frameName, int index) {
      System.out.println ("Browser2 Not Implemented");
  
    }
  
    public String        getVersion() {
      System.out.println ("getVersion Not Implemented");
  
       return "0.17";
     }
  
    // Get the current velocity of the bound viewpoint in meters/sec,
    // if available, or 0.0 if not
    public float         getCurrentSpeed() {
      System.out.println ("getCurrentSpeed Not Implemented");
  
      return (float) 0.0;
    }
  
    // Get the current frame rate of the browser, or 0.0 if not available
    public float         getCurrentFrameRate() {
  
    String retval;

    synchronized (BrowserGlobals.FreeWRLToken) {
      EAIoutSender.send ("" + BrowserGlobals.queryno + "\nGCFR\n");
      retval = getVRMLreply(BrowserGlobals.queryno);
      BrowserGlobals.queryno += 1;
    }

    return Float.valueOf(retval).floatValue();
    }
  
    // Get the URL for the root of the current world, or an empty string
    // if not available
    public String        getWorldURL() {

      String retval;

      
       synchronized (BrowserGlobals.FreeWRLToken) {
         EAIoutSender.send ("" + BrowserGlobals.queryno + "\nGWU\n");
         retval = getVRMLreply(BrowserGlobals.queryno);
         BrowserGlobals.queryno += 1;
       }
      return retval;

    }
  
    // Replace the current world with the passed array of nodes
    public void          replaceWorld(Node[] nodes)
         throws IllegalArgumentException {
      System.out.println ("illegalargumentexception Not Implemented");
  
    return; }
  
  
    // Load the given URL with the passed parameters (as described
    // in the Anchor node)
    public void          loadURL(String[] url, String[] parameter) {
      System.out.println ("loadURL Not Implemented");
  
      return;
    }
  
  
    // Set the description of the current world in a browser-specific
    // manner. To clear the description, pass an empty string as argument
    public void          setDescription(String description) {
      System.out.println ("setDescription Not Implemented");
  
      return;
    }
  
  
    // Parse STRING into a VRML scene and return the list of root
    // nodes for the resulting scene
    public Node[]        createVrmlFromString(String vrmlSyntax) {

      Node[]  x = {new Node()};
      StringTokenizer tokens;
      String retval;
      String temp;
      int count;

      synchronized (BrowserGlobals.FreeWRLToken) {
        EAIoutSender.send ("" +BrowserGlobals.queryno + "\nCVS "+vrmlSyntax+"\nEOT\n");
        retval = getVRMLreply(BrowserGlobals.queryno);

        tokens = new StringTokenizer (retval);
        count = 0;

        while (tokens.hasMoreTokens()) {
          x[count] = new Node();
          x[count].NodeName = tokens.nextToken();
          count ++;
        }
        BrowserGlobals.queryno += 1;
      }
      return x;
    }
  
  
  
    // Tells the browser to load a VRML scene from the passed URL or
    // URLs. After the scene is loaded, an event is sent to the MFNode
    // eventIn in node NODE named by the EVENT argument
    public void          createVrmlFromURL(String[] url,
                                           Node node,
                                           String event) {

      EventInMFNode		Evin;
      String retval;
      Node[]  x = {new Node()};
      StringTokenizer tokens;
      int count;


       synchronized (BrowserGlobals.FreeWRLToken) {
         EAIoutSender.send ("" + BrowserGlobals.queryno + "\nCVU " + url[0] + "\n");

         retval = getVRMLreply(BrowserGlobals.queryno);

         tokens = new StringTokenizer (retval);
         count = 0;
 	x = new Node[tokens.countTokens()];

         while (tokens.hasMoreTokens()) {
           x[count] = new Node();
           x[count].NodeName = tokens.nextToken();
           count ++;
         }
 	BrowserGlobals.queryno += 1;
       }

      // Now, sent the event to the event!
      Evin = (EventInMFNode) node.getEventIn(event);
      Evin.setValue(x);
    }
  
  
    // Add and delete, respectively, a route between the specified eventOut
    // and eventIn of the given nodes
    public void          addRoute(Node fromNode, String fromEventOut,
                                  Node toNode, String toEventIn) {
      System.out.println ("AddRoute Not Implemented");
      return;
  
    }
  
  
    public void          deleteRoute(Node fromNode, String fromEventOut,
                                     Node toNode, String toEventIn) {
      System.out.println ("DeleteRoute Not Implemented");
  
      return;
    }
  
    // begin and end an update cycle
    public void          beginUpdate() {} 
    public void          endUpdate() {} 
  
    // called after the scene is loaded, before the first event is processed
    public void initialize() {
      System.out.println ("Initialize Not Implemented");
  
    }
    
  
    // called just before the scene is unloaded
    public void shutdown() {
      System.out.println ("Shutdown Not Implemented");
  
    }
  
    // return an instance of the Browser class
    // This returns the first embedded plugin in the current frame.
    static public Browser getBrowser(Applet pApplet) {
      return (new Browser(pApplet));
    }
  
    // return an instance of the Browser class
    // If frameName is NULL, current frame is assumed.
    static public Browser getBrowser(Applet pApplet, String frameName, int index) {
      // We don't have frames and indexes yet...
      Browser x = getBrowser(pApplet);
      System.out.println ("getBrowser possibly Not Implemented");
  
      return x;
    }
  
  
    // Get a DEFed node by name. Nodes given names in the root scene
    // graph must be made available to this method. DEFed nodes in inlines,
    // as well as DEFed nodes returned from createVrmlFromString/URL, may
    // or may not be made available to this method, depending on the
    // browser's implementation
  
    public Node getNode (String NodeName)
      {
      Node temp;

      temp = new Node();  

      synchronized (BrowserGlobals.FreeWRLToken) {
        EAIoutSender.send ("" + BrowserGlobals.queryno + "\nGN " + NodeName + "\n");
        temp.NodeName = getVRMLreply (BrowserGlobals.queryno);
        BrowserGlobals.queryno += 1;
      }

      return temp;
      }




  //
  // Send Event to the VRML Browser. Note the different methods, depending
  // on the parameters.
  //

//REDO  public static void SendEvent (String NodeName, String FieldName, VSFVec3f Value)
//REDO    {
//REDO      float vals[];
//REDO      Float f0;
//REDO      Float f1;
//REDO      Float f2;

//REDO      vals = Value.getValue();
//REDO      f0 = new Float(vals[0]); f1 = new Float(vals[1]); 
//REDO      f2 = new Float(vals[2]);
//REDO      SendEvent (NodeName, FieldName, (String) f0.toString() + 
//REDO	" " + f1.toString() + " " + f2.toString());
//REDO      return;
//REDO    }

//REDO  public static void SendEvent (String NodeName, String FieldName, VSFRotation Value)
//REDO    {
//REDO      float vals[];
//REDO      Float f0;
//REDO      Float f1;
//REDO      Float f2;
//REDO      Float f3;

//REDO      vals = Value.getValue();
//REDO      f0 = new Float(vals[0]); f1 = new Float(vals[1]); 
//REDO      f2 = new Float(vals[2]); f3 = new Float(vals[3]);
//REDO      SendEvent (NodeName, FieldName, (String) f0.toString() + 
//REDO	" " + f1.toString() + " " + f2.toString() + " " + f3.toString());
//REDO      return;
//REDO    }


  public static void SendEvent (String NodeName, String FieldName, String Value)
    {

      synchronized (BrowserGlobals.FreeWRLToken) {
        EAIoutSender.send ("" + BrowserGlobals.queryno + "\nSE " + NodeName + " " + 
           FieldName + "\n" + Value + "\n");
        // JAS - don't wait. getVRMLreply(BrowserGlobals.queryno);
        BrowserGlobals.queryno += 1;
      }
      return;
    }

  // Get the browser name
  public static String getName() {
      String retval;

       synchronized (BrowserGlobals.FreeWRLToken) {
         EAIoutSender.send ("" + BrowserGlobals.queryno + "\nGNAM\n");
         retval = getVRMLreply(BrowserGlobals.queryno);
         BrowserGlobals.queryno += 1;
       }
      return retval;
  }


  protected static String SendEventType (String NodeName, String FieldName) {
      // get a type from a particular node.

      String retval;

      synchronized (BrowserGlobals.FreeWRLToken) {
        EAIoutSender.send ("" + BrowserGlobals.queryno + "\nGT " + NodeName + " " +
           FieldName + "\n");
        retval = getVRMLreply(BrowserGlobals.queryno);
        BrowserGlobals.queryno += 1;
      }
      return retval;
}

  public static String SendEventOut (String NodeName, String FieldName) {
      // get a value from a particular node.

      String retval;

       synchronized (BrowserGlobals.FreeWRLToken) {
         EAIoutSender.send ("" + BrowserGlobals.queryno + "\nGV " + NodeName + " " +
            FieldName + "\n");
         retval = getVRMLreply(BrowserGlobals.queryno);
         BrowserGlobals.queryno += 1;
      }
     return retval;
}

  public static void RegisterListener (EventOutObserver f, Object userData)
    {
       EventOut me = (EventOut) userData;

       synchronized (BrowserGlobals.FreeWRLToken) {
         EAIoutSender.send ("" + BrowserGlobals.queryno + "\nRL " + me.outNode + 
 		" " + me.command + " " + BrowserGlobals.queryno + "\n");
 
         BrowserGlobals.EVarray [BrowserGlobals.EVno] =  BrowserGlobals.queryno;
         BrowserGlobals.EVtype [BrowserGlobals.EVno] = me.EventType;     
         BrowserGlobals.EVObject[BrowserGlobals.EVno] = userData;
         BrowserGlobals.EVObserver[BrowserGlobals.EVno] = f;

	 // Is this a short, consise answer type? 
	 // (see field/FieldTypes.java for more info)

	 switch (me.EventType) {
		case 1:
		case 3:
		case 4:
		case 5:
		case 6:
		case 7:
		case 8:
		case 9:
		case 12:
		case 13:
		case 14:
		case 15:
		case 16:
		case 17:
		case 18:
		case 19:
			BrowserGlobals.EVshortreply[BrowserGlobals.EVno] = true;
			break;
		default:
			BrowserGlobals.EVshortreply[BrowserGlobals.EVno] = false;
	 } 


         BrowserGlobals.EVno += 1;
       
         getVRMLreply(BrowserGlobals.queryno); 
         BrowserGlobals.queryno += 1;
       }
    }

    protected synchronized static String getVRMLreply (int queryno) 
      {
  
        String req = "0"; 
        String rep = "";
  

 	// This waits for the correct event toe be returned. Note that
	// sendevents dont wait, so there is the possibility that
	// we will have to while away a bit of time waiting...
 
 	while (queryno != Integer.parseInt(req)) { 
           try {
             req = Browser.EAIfromFreeWRLInputStream.readLine();
           } catch (IOException ie) {System.out.println (ie);}

           if (queryno != Integer.parseInt(req)) {
             System.out.println ("getVRMLreply - REPLIES MIXED UP!!! Expecting " 
                + queryno + 
   	     " got " + req);
           }
       
           try {
                 rep = Browser.EAIfromFreeWRLInputStream.readLine(); 
           } catch (IOException ie) { System.out.println ("getVRMLreply failed");}
	

         }

        return rep; 
      }
  
}

