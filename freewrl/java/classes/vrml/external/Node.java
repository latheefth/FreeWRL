package vrml.external;

import vrml.external.field.*;
import vrml.external.exception.*;
import java.util.*;
//JAS import java.awt.*;
//JAS import java.net.*;
//JAS import java.io.*;
//JAS import java.lang.reflect.*;

public class Node {
  // Get a string specifying the type of this node. May return the
  // name of a PROTO, or the class name


  public String NodeName  = "Not initiated yet";

// the following fields are for treating this node as an eventIn or eventOut.
 public int EventType = FieldTypes.UnknownType;
 public String outNode;	// Node to send the command to... NULL if not
			// a get value from viewer call (ie, a Listener
			// response...
 public String inNode;
 public String command;	// the actual command...
 public String RLreturn;	// If this is a register listener response...
 public String nodeptr; //pointer to start of FreeWRL structure in memory
 public String offset;  //offset of actual field in memory from base.
 public String datasize; // how long this data really is
 public String datatype;
 public String ScriptType; // non zero indicates sending to a javascript



  public String toString() {
    return "NODE"+NodeName;
  }

  public String        getType() {
    // If we actually do this thingie, it means that we have not
    // been overriden.  So far, only EventMFNodes reach here.
    // Things like VSFRotations get their getTypes from VSFrotation.java.

    return "Transform";
  }


  public EventIn       getEventIn(String name) {

	EventIn ret;

	String NNN = "nodeFrom_getEventIn";
      StringTokenizer tokens;

  // Return the type that is asked for. To determine the
  // subclass, look at the string.

    String st = Browser.SendEventType(NodeName, name, "eventIn");

    tokens = new StringTokenizer (st);
    String NNPR = tokens.nextToken();
    String NOFF = tokens.nextToken();
    String NDS = tokens.nextToken();
    String NewDT = tokens.nextToken();
    String ScrT = tokens.nextToken();
    // System.out.println ("EventIn: NNPR " + NNPR + " NOFF " + NOFF +
    //	" NDS " + NDS + " NewDT " + NewDT + " ScrTyp" + ScrT);

    // check out the return values specified in CFuncs/EAIServ.c
    if(NewDT.equals("p")) { ret = new EventInMFString();
    } else if(NewDT.equals("k")) { ret = new EventInSFImage();
    } else if(NewDT.equals("e")) { ret = new EventInSFTime();
    } else if(NewDT.equals("c")) { ret = new EventInSFColor();
    } else if(NewDT.equals("l")) { ret = new EventInMFColor();
    } else if(NewDT.equals("d")) { ret = new EventInSFFloat();
    } else if(NewDT.equals("m")) { ret = new EventInMFFloat();
    } else if(NewDT.equals("o")) { ret = new EventInMFInt32();
    } else if(NewDT.equals("h")) { ret = new EventInSFNode();
    } else if(NewDT.equals("r")) { ret = new EventInMFRotation();
    } else if(NewDT.equals("s")) { ret = new EventInMFVec2f();
    } else if(NewDT.equals("j")) { ret = new EventInSFVec2f();
    } else if(NewDT.equals("l")) { ret = new EventInMFVec3f();
    } else if(NewDT.equals("q")) { ret = new EventInMFNode();
    } else if(NewDT.equals("i")) { ret = new EventInSFRotation();
    } else if(NewDT.equals("g")) { ret = new EventInSFString();
    } else if(NewDT.equals("b")) { ret = new EventInSFBool();
    } else if(NewDT.equals("f")) { ret = new EventInSFInt32();
    } else if(NewDT.equals("u")) { ret = new EventInSFVec3f();
    } else {
    	// Return default

	    ret = new EventInMFNode();
	    System.out.println ("WARNING: getEventIn - don't know how to handle " + name
		+ " asked for " + st + " returning Class EventInMFnode");
    }

    ret.command = name; ret.inNode = NNN; ret.datatype=NewDT; ret.nodeptr=NNPR; ret.offset=NOFF;
    ret.datasize = NDS; ret.ScriptType = ScrT;
    return ret;
  }


  // Means of getting a handle to an EventOut of this node

  public EventOut      getEventOut(String name) throws InvalidEventOutException {
	EventOut ret;
      StringTokenizer tokens;
	String NNN = "nodeFrom_getEventOut";

    String st = Browser.SendEventType(NodeName, name, "eventOut");

    tokens = new StringTokenizer (st);
    String NNPR = tokens.nextToken();
    String NOFF = tokens.nextToken();
    String NDS = tokens.nextToken();
    String NewDT = tokens.nextToken();
    String ScrT = tokens.nextToken();
    // System.out.println ("EventOut: NNPR " + NNPR + " NOFF " + NOFF +
    //	" NDS " + NDS + " NewDT " + NewDT + " ScrTyp" + ScrT);

    // check out the return values specified in CFuncs/EAIServ.c
    if(NewDT.equals("p")) { ret = new EventOutMFString();
    } else if(NewDT.equals("k")) { ret = new EventOutSFImage();
    } else if(NewDT.equals("e")) { ret = new EventOutSFTime();
    } else if(NewDT.equals("c")) { ret = new EventOutSFColor();
    } else if(NewDT.equals("l")) { ret = new EventOutMFColor();
    } else if(NewDT.equals("d")) { ret = new EventOutSFFloat();
    } else if(NewDT.equals("m")) { ret = new EventOutMFFloat();
    } else if(NewDT.equals("o")) { ret = new EventOutMFInt32();
    } else if(NewDT.equals("h")) { ret = new EventOutSFNode();
    } else if(NewDT.equals("r")) { ret = new EventOutMFRotation();
    } else if(NewDT.equals("s")) { ret = new EventOutMFVec2f();
    } else if(NewDT.equals("j")) { ret = new EventOutSFVec2f();
    } else if(NewDT.equals("l")) { ret = new EventOutMFVec3f();
    } else if(NewDT.equals("q")) { ret = new EventOutMFNode();
    } else if(NewDT.equals("i")) { ret = new EventOutSFRotation();
    } else if(NewDT.equals("g")) { ret = new EventOutSFString();
    } else if(NewDT.equals("b")) { ret = new EventOutSFBool();
    } else if(NewDT.equals("f")) { ret = new EventOutSFInt32();
    } else if(NewDT.equals("u")) { ret = new EventOutSFVec3f();
    } else {
	throw new InvalidEventOutException();
    }

    ret.command = name; ret.inNode = NNN; ret.datatype=NewDT; ret.nodeptr=NNPR; ret.offset=NOFF;
    ret.datasize = NDS; ret.ScriptType = ScrT;
    return ret;
  }
}
