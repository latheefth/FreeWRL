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
import vrml.external.exception.InvalidNodeException;
import vrml.external.exception.InvalidVrmlException;
import vrml.external.BrowserGlobals;
//JAS - no longer using Netscape import netscape.security.*;

public class Browser implements BrowserInterface

{

	//====================================================================
	// Threads:

		// Replies from FreeWRL.
		static Thread 		FreeWRLThread; // of type EAIinThread

		// Send commands to FreeWRL.
		static EAIoutThread 		EAIoutSender;

		// Handle Async communications from FreeWRL (eg, Regisered Listeners)
    		static EAIAsyncThread        RL_Async;


	//====================================================================
	// Communication Paths:

		// FreeWRLThread to Browser - responses from commands to FreeWRL.
    		PrintWriter EAIinThreadtoBrowser;
    		PipedWriter EAIinThreadtoBrowserPipe = null;

		// 
    		static BufferedReader BrowserfromEAI = null;
    		PipedReader BrowserfromEAIPipe = null;




    // The following are used to send to/from the FreeWRL Browser by:
    // 		- EAIinThread

    Socket	EAISocket;
    Socket		sock;
    static PrintWriter         EAIout;
    

    // The following pipe listens for replies to events sent to
    // the FreeWRL VRML viewer via the EAI port.


    private String              reply = "";

    
    // Query Number as sent to the FreeWRL Browser.
    static int   queryno = 1;
    
    
    // Sending to FreeWRL needs to synchronize on an object;
    static Object FreeWRLToken = new Object();
    

    // Interface methods.
    public int get_Browser_EVtype (int event)
      {
	//System.out.println ("get_Browser_EVtype is returning " + 
	//	BrowserGlobals.EVtype[event] + 
	//	" for event " + event);
        return BrowserGlobals.EVtype[event];
      }

    public EventOutObserver get_Browser_EVObserver (int eventno)
      {
	// System.out.println ("get_Browser_EVObserver is returning " +  
	// 	BrowserGlobals.EVObserver[eventno]);
        return BrowserGlobals.EVObserver[eventno];
      }

    public boolean get_Browser_EV_short_reply (int event)
      {
	int EVcounter;
        for (EVcounter=0; EVcounter<BrowserGlobals.EVno; EVcounter++) {
          if (BrowserGlobals.EVarray[EVcounter] == event) {
            break;
          }
        }
	// System.out.println ("get_Browser_EV_short_reply is returning " + BrowserGlobals.EVshortreply[EVcounter]);
        return BrowserGlobals.EVshortreply[EVcounter];

      }

    public void Browser_RL_Async_send (String EVentreply, int eventno) 
      {
        int EVcounter;
        for (EVcounter=0; EVcounter<BrowserGlobals.EVno; EVcounter++) {
          if (BrowserGlobals.EVarray[EVcounter] == eventno) {
            break;
          }
        }
	// System.out.println ("Browser_RL_Async_send sending " + EVentreply + " to number " + EVcounter);
        RL_Async.send(EVentreply, EVcounter);
      }

    // Associates this instance with the first embedded plugin in the current frame.
    public Browser(Applet pApplet) {

  	// Create a socket here for an EAI server on localhost
	int incrport = -1;
	EAISocket = null;

	//System.out.println ("trying to open socket on 9877");
	//enable privleges
	//JAS try {
	//JAS 	PrivilegeManager.enablePrivilege ("UniversalConnect");
	//JAS } catch (Throwable e) {
	//JAS 	System.out.println("EAI: not using Netscape");
	//JAS }
	
	try {
		EAISocket = new Socket("localhost",9877);
	} catch (IOException e) {
		System.out.println ("EAI: Error creating socket for FreeWRL EAI on port 2000");
	}
	//System.out.println ("EAI: opened TCP Client port, ready for data" );


  	//JAStry {
  	//JAS	sock=EAISocket.accept();
  	//JAS} catch (IOException e) {
  	  //JASSystem.out.print ("EAI: System error on accept method\n");
  	//JAS}


	sock = EAISocket;	//JAS - these are the same now...



	//===================================================================
	// create the EAIinThread to Browser.

  	// Open the pipe for EAI replies to be sent to us...
        try {
		EAIinThreadtoBrowserPipe = new PipedWriter();
		BrowserfromEAIPipe = new PipedReader(EAIinThreadtoBrowserPipe);
        } catch (IOException ie) {
          System.out.println ("EAI: caught error in new PipedReader: " + ie);
        }

	EAIinThreadtoBrowser = new PrintWriter(EAIinThreadtoBrowserPipe);
	BrowserfromEAI = new BufferedReader (BrowserfromEAIPipe);	

  	// Start the readfrom FREEWRL thread...
   	FreeWRLThread = new Thread ( new EAIinThread(sock, pApplet, 
		EAIinThreadtoBrowser, this));
        FreeWRLThread.start();


	//====================================================================
	// Start the thread that allows Registered Listenered
	// updates to come in.
	RL_Async = new EAIAsyncThread();
	RL_Async.start();

  
	//====================================================================
	// create the EAIoutThread - send data to FreeWRL.
	try {
		EAIout = new PrintWriter (sock.getOutputStream());
	} catch (IOException e) {
		System.out.print ("EAI: Problem in handshaking with Browser");
	}


  	// Start the SendTo FREEWRL thread...
	EAIoutSender = new EAIoutThread(EAIout);
        EAIoutSender.start();



	//====================================================================
  	// Browser is "gotten", and is started.
  	return;
    }
  
    // construct an instance of the Browser class
    // If frameName is NULL, current frame is assumed.
    public Browser(Applet pApplet, String frameName, int index) {
      System.out.println ("Browser2 Not Implemented");
  
    }
  
  // Get the browser name
  public static String getName() {
      String retval;
       synchronized (FreeWRLToken) {
         EAIoutSender.send ("" + queryno + "K\n");
         retval = getVRMLreply(queryno);
         queryno += 1;
       }
      return retval;
  }

    public String        getVersion() {
       String retval;
       synchronized (FreeWRLToken) {
         EAIoutSender.send ("" + queryno + "L\n");
         retval = getVRMLreply(queryno);
         queryno += 1;
       }
      return retval;
     }
  
    // Get the current velocity of the bound viewpoint in meters/sec,
    public float         getCurrentSpeed() {
       String retval;
       synchronized (FreeWRLToken) {
         EAIoutSender.send ("" + queryno + "M\n");
         retval = getVRMLreply(queryno);
         queryno += 1;
       }
      return Float.valueOf(retval).floatValue();
    }
  
    // Get the current frame rate of the browser, or 0.0 if not available
    public float         getCurrentFrameRate() {
    String retval;
    synchronized (FreeWRLToken) {
      EAIoutSender.send ("" + queryno + "N\n");
      retval = getVRMLreply(queryno);
      queryno += 1;
    }
    return Float.valueOf(retval).floatValue();
    }
  
    // Get the URL for the root of the current world, or an empty string
    // if not available
    public String        getWorldURL() {
      String retval;
       synchronized (FreeWRLToken) {
         EAIoutSender.send ("" + queryno + "O\n");
         retval = getVRMLreply(queryno);
         queryno += 1;
       }
      return retval;
    }
  
    // Replace the current world with the passed array of nodes
    public void          replaceWorld(Node[] nodes)
         throws IllegalArgumentException {

 	String SysString = "";
	String retval;
	int count;

        for (count=0; count<nodes.length; count++) {
		SysString = SysString + " " + nodes[count];
	}

        synchronized (FreeWRLToken) {
          EAIoutSender.send ("" + queryno + "RW" + SysString);
          retval = getVRMLreply(queryno);
          queryno += 1;
        }

    } 
  
  
    // Load the given URL with the passed parameters (as described
    // in the Anchor node)
    public void          loadURL(String[] url, String[] parameter) {
      System.out.println ("EAI: loadURL Not Implemented");
  
      return;
    }
  
  
    // Set the description of the current world in a browser-specific
    // manner. To clear the description, pass an empty string as argument
    public void          setDescription(String description) {
      System.out.println ("EAI: setDescription Not Implemented");
  
      return;
    }
  
  
    // Parse STRING into a VRML scene and return the list of root
    // nodes for the resulting scene
    public Node[]        createVrmlFromString(String vrmlSyntax) 
			throws InvalidVrmlException {

      Node[]  x = new Node[100];
      StringTokenizer tokens;
      String retval;
      String temp;
      int count;

      synchronized (FreeWRLToken) {
        EAIoutSender.send ("" +queryno + "S "+vrmlSyntax+"\nEOT\n");
        retval = getVRMLreply(queryno);

	// System.out.println ("Browser.java - got " + retval);

        tokens = new StringTokenizer (retval);

	// We return a (node-index C-pointer) pair, so we have to divide by two
	x = new Node[tokens.countTokens()/2];
        count = 0;

	// Lets go through the output, and temporarily store it
	// XXX - is there a better way? ie, using a vector?
        while (tokens.hasMoreTokens()) {
          x[count] = new Node();
          x[count].NodeName = tokens.nextToken();
          x[count].nodeptr = tokens.nextToken();
          x[count].offset = "0";
          x[count].datasize = "4";
          x[count].datatype = "h"; // see CFuncs/EAIServ.c for a complete desc.

	  //System.out.println ("CVS - for this one, we have NODE" + x[count].NodeName +
		//" pointer:" + x[count].nodeptr + " offset:" + x[count].offset +
		//" datasize: " + x[count].datasize + " datatype:" + x[count].datatype);

          count ++;
	  if (count == 100) {
		count = 99;
		System.out.println ("EAI: createVrmlFromString - warning, tied to 100 nodes");
	  }
        }
        queryno += 1;
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
      Node[]  x = new Node[1];
      StringTokenizer tokens;
      int count;


       synchronized (FreeWRLToken) {
         EAIoutSender.send ("" + queryno + "T " + url[0] + "\n");

         retval = getVRMLreply(queryno);

         tokens = new StringTokenizer (retval);
         count = 0;
 	 x = new Node[tokens.countTokens()];

         while (tokens.hasMoreTokens()) {
           x[count] = new Node();
           x[count].NodeName = tokens.nextToken();
           count ++;
         }
 	queryno += 1;
       }

      // Now, sent the event to the event!
      Evin = (EventInMFNode) node.getEventIn(event);
      Evin.setValue(x);
    }
  
  
    // Add and delete, respectively, a route between the specified eventOut
    // and eventIn of the given nodes
    public void          addRoute(Node fromNode, String fromEventOut,
                                  Node toNode, String toEventIn) throws
				  IllegalArgumentException {
      String retval;

      synchronized (FreeWRLToken) {
         EAIoutSender.send ("" + queryno + "\nAR " + fromNode + " " + fromEventOut +
		" " + toNode + " " + toEventIn +"\n");
         retval = getVRMLreply(queryno);
         queryno += 1;
       }
      return;

      // throw new IllegalArgumentException ("AddRoute Not Implemented");
    }
  
  
    public void          deleteRoute(Node fromNode, String fromEventOut,
                                     Node toNode, String toEventIn) 
			 throws IllegalArgumentException {
      String retval;

      synchronized (FreeWRLToken) {
         EAIoutSender.send ("" + queryno + "\nDR " + fromNode + " " + fromEventOut +
		" " + toNode + " " + toEventIn +"\n");
         retval = getVRMLreply(queryno);
         queryno += 1;
       }
      return;

    //  throw new IllegalArgumentException ("DeleteRoute Not Implemented");
    }
  
    // begin and end an update cycle
    public void          beginUpdate() {} 
    public void          endUpdate() {} 
  
    // called after the scene is loaded, before the first event is processed
    public void initialize() {
      System.out.println ("EAI: Initialize Not Implemented");
  
    }
    
  
    // called just before the scene is unloaded
    public void shutdown() {
      System.out.println ("EAI: Shutdown Not Implemented");
  
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
      System.out.println ("EAI: getBrowser possibly Not Implemented");
  
      return x;
    }
  
  
    // Get a DEFed node by name. Nodes given names in the root scene
    // graph must be made available to this method. DEFed nodes in inlines,
    // as well as DEFed nodes returned from createVrmlFromString/URL, may
    // or may not be made available to this method, depending on the
    // browser's implementation
  
    public Node getNode (String NodeName) throws InvalidNodeException
      {
      Node temp;

      temp = new Node();  

      synchronized (FreeWRLToken) {
        EAIoutSender.send ("" + queryno + "A " + NodeName + "\n");
        temp.NodeName = getVRMLreply (queryno);
        queryno += 1;
      }
      if (temp.NodeName.equals("undefined"))
	throw new InvalidNodeException(NodeName + "undefined");

      return temp;
      }




  //
  // Send Event to the VRML Browser. Note the different methods, depending
  // on the parameters.
  //
  // SendChildEvent waits for confirmation that child is added/removed to MFNode array.
  // This gets around the problem of sending two adds in succession, and having
  // the second overwrite the first.
  public static void SendChildEvent (String parent, String offset, String FieldName, String Value)
    {
      String retval;

	// System.out.println ("SendChildEvent: sending to " + parent + " ofs:" + 
	//	offset + " field " + FieldName + " child: " + Value);

      synchronized (FreeWRLToken) {
        EAIoutSender.send ("" + queryno + "C " + parent + " " + offset + " " +
           FieldName + " "+ Value + "\n");
        retval = getVRMLreply(queryno);
        queryno += 1;
	
        // Now, tell FreeWRL to update routes
        //JAS EAIoutSender.send ("" + queryno + "B "  
         //JAS   FieldName + " " + Value + "\n");
        //JAS retval = getVRMLreply(queryno);
        //JAS queryno += 1;
      }
      return;
    }

  // Most events don't need us to wait around for it.
  public static void SendEvent (String NodePtr, String Offset, String datasize, String NodeType, String Value)
    {

	//System.out.println ("SendEvent, sending NodeType " + NodeType + " NodePtr " + NodePtr +
	//	" Offset " + Offset);
      synchronized (FreeWRLToken) {
        EAIoutSender.send ("" + queryno + "D" + NodeType + " " + 
           NodePtr + " " + Offset + " " + Value + "\n");
        // JAS - don't wait. getVRMLreply(queryno);
        queryno += 1;
      }
      return;
    }


  protected static String SendEventType (String NodeName, String FieldName, String direction) {

      // get a type from a particular node.

      String retval;

      synchronized (FreeWRLToken) {
        EAIoutSender.send ("" + queryno + "F " + NodeName + " " +
           FieldName + " " + direction + "\n");
        retval = getVRMLreply(queryno);
        queryno += 1;
      }
      return retval;
}

  public static String SendEventOut (String NodeName, String FieldName) {
      // get a value from a particular node.

      String retval;

       synchronized (FreeWRLToken) {
         EAIoutSender.send ("" + queryno + "\nGV " + NodeName + " " +
            FieldName + "\n");
         retval = getVRMLreply(queryno);
         queryno += 1;
      }
     return retval;
}

  public static void RegisterListener (EventOutObserver f, Object userData,
			String outNode, String command,  int EventType)
    {
       synchronized (FreeWRLToken) {
         EAIoutSender.send ("" + queryno + "\nRL " + outNode + 
 		" " + command + " " + queryno + "\n");
 
         BrowserGlobals.EVarray [BrowserGlobals.EVno] =  queryno;
         BrowserGlobals.EVtype [BrowserGlobals.EVno] = EventType;     
         BrowserGlobals.EVObject[BrowserGlobals.EVno] = userData;
         BrowserGlobals.EVObserver[BrowserGlobals.EVno] = f;
	 // Is this a short, consise answer type? 
	 // (see field/FieldTypes.java for more info)

	 switch (EventType) {
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
       
         getVRMLreply(queryno); 
         queryno += 1;
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
             req = BrowserfromEAI.readLine();
           } catch (IOException ie) {
		System.out.println ("EAI: caught " + ie);
		return rep;
	   }

           if (queryno != Integer.parseInt(req)) {
             System.out.println ("EAI: getVRMLreply - REPLIES MIXED UP!!! Expecting " 
                + queryno + 
   	     " got " + req);
           }
       
           try {
                 rep = BrowserfromEAI.readLine(); 
           } catch (IOException ie) { System.out.println ("EAI: getVRMLreply failed"); return null; }
	

         }

        return rep; 
      }  
	public void close() {
		System.out.println("EAI: closing socket");
		try {
			EAIoutSender.stopThread();
			EAISocket.close();
			//JAS EAIfromFreeWRLStream.close();
  
		} catch (IOException e) {
		}
	}
}

