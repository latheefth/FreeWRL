package vrml.external;

import vrml.external.field.*;
import vrml.external.exception.*;
import java.util.*;
import java.awt.*;
import java.net.*;
import java.io.*;
import java.lang.reflect.*;

public class Node {
  // Get a string specifying the type of this node. May return the
  // name of a PROTO, or the class name

  public String NodeName  = "Not initiated yet";
  public String nodeptr = "0";
  public String offset = "0";
  public String datasize = "0";
  public String datatype;

  public String toString() {
    return NodeName;
  }

  public String        getType() {
    // If we actually do this thingie, it means that we have not
    // been overriden.  So far, only EventMFNodes reach here.
    // Things like VSFRotations get their getTypes from VSFrotation.java.

    return "Transform";
  }


  public EventIn       getEventIn(String name) {

	String NNN = "nodeFrom_getEventIn";
      StringTokenizer tokens;

  // Return the type that is asked for. To determine the
  // subclass, look at the string. 
  // There *HAS* to be a better way than this!
  // Maybe, send a GFT to the FreeWRL Browser, to see what
  // it thinks...

    String st = Browser.SendEventType(NodeName, name, "eventIn");

    // Did this fail??? if so, then keep trying...
    if (st.equals("")) {

      // Do we have a "set_" at the beginning????
      if (name.length() > 5) {
	  // possibly....
	  if (name.substring(0,4).equals ("set_")) {
            name = name.substring(4);
            st = Browser.SendEventType(NodeName, name, "eventIn");
	  }
      }
    }

    tokens = new StringTokenizer (st);
    String NNPR = tokens.nextToken();
    String NOFF = tokens.nextToken();
    String NDS = tokens.nextToken();
    String NewDT = tokens.nextToken();
    //System.out.println ("EventOut: NNPR " + NNPR + " NOFF " + NOFF +
    //	" NDS " + NDS + " NewDT " + NewDT);


    // check out the return values specified in CFuncs/EAIServ.c
    if(NewDT.equals("p")) {
      EventIn ret = new EventInMFString(); ret.command = name; ret.inNode = NNN; ret.datatype=NewDT;
		ret.nodeptr = NNPR; ret.offset = NOFF; ret.datasize = NDS; return ret;
    } else if(NewDT.equals("k")) {
      EventIn ret = new EventInSFImage(); ret.command = name; ret.inNode = NNN; ret.datatype=NewDT;
		ret.nodeptr = NNPR; ret.offset = NOFF; ret.datasize = NDS; return ret;
    } else if(NewDT.equals("e")) {
      EventIn ret = new EventInSFTime(); ret.command = name; ret.inNode = NNN; ret.datatype=NewDT;
		ret.nodeptr = NNPR; ret.offset = NOFF; ret.datasize = NDS; return ret;
    } else if(NewDT.equals("c")) {
      EventIn ret = new EventInSFColor(); ret.command = name; ret.inNode = NNN; ret.datatype=NewDT;
		ret.nodeptr = NNPR; ret.offset = NOFF; ret.datasize = NDS; return ret;
    } else if(NewDT.equals("l")) {
      EventIn ret = new EventInMFColor(); ret.command = name; ret.inNode = NNN; ret.datatype=NewDT;
		ret.nodeptr = NNPR; ret.offset = NOFF; ret.datasize = NDS; return ret;
    } else if(NewDT.equals("d")) {
      EventIn ret = new EventInSFFloat(); ret.command = name; ret.inNode = NNN; ret.datatype=NewDT;
		ret.nodeptr = NNPR; ret.offset = NOFF; ret.datasize = NDS; return ret;
    } else if(NewDT.equals("m")) {
      EventIn ret = new EventInMFFloat(); ret.command = name; ret.inNode = NNN; ret.datatype=NewDT;
		ret.nodeptr = NNPR; ret.offset = NOFF; ret.datasize = NDS; return ret;
    } else if(NewDT.equals("o")) {
      EventIn ret = new EventInMFInt32(); ret.command = name; ret.inNode = NNN; ret.datatype=NewDT;
		ret.nodeptr = NNPR; ret.offset = NOFF; ret.datasize = NDS; return ret;
    } else if(NewDT.equals("h")) {
      EventIn ret = new EventInSFNode(); ret.command = name; ret.inNode = NNN; ret.datatype=NewDT;
		ret.nodeptr = NNPR; ret.offset = NOFF; ret.datasize = NDS; 
		return ret;
    } else if(NewDT.equals("r")) {
      EventIn ret = new EventInMFRotation(); ret.command = name; ret.inNode = NNN; ret.datatype=NewDT;
		ret.nodeptr = NNPR; ret.offset = NOFF; ret.datasize = NDS; return ret;
    } else if(NewDT.equals("s")) {
      EventIn ret = new EventInMFVec2f(); ret.command = name; ret.inNode = NNN; ret.datatype=NewDT;
		ret.nodeptr = NNPR; ret.offset = NOFF; ret.datasize = NDS; return ret;
    } else if(NewDT.equals("j")) {
      EventIn ret = new EventInSFVec2f(); ret.command = name; ret.inNode = NNN; ret.datatype=NewDT;
		ret.nodeptr = NNPR; ret.offset = NOFF; ret.datasize = NDS; return ret;
    } else if(NewDT.equals("l")) {
      EventIn ret = new EventInMFVec3f(); ret.command = name; ret.inNode = NNN; ret.datatype=NewDT;
		ret.nodeptr = NNPR; ret.offset = NOFF; ret.datasize = NDS; return ret;
    } else if(NewDT.equals("q")) {
      EventIn ret = new EventInMFNode(); ret.command = name; ret.inNode = NNN; ret.datatype=NewDT;
		ret.nodeptr = NNPR; ret.offset = NOFF; ret.datasize = NDS;
	return ret;
    } else if(NewDT.equals("i")) {
      EventIn ret = new EventInSFRotation(); ret.command = name; ret.inNode = NNN;ret.datatype=NewDT;
		ret.nodeptr = NNPR; ret.offset = NOFF; ret.datasize = NDS; return ret;
    } else if(NewDT.equals("g")) {
      EventIn ret = new EventInSFString(); ret.command = name; ret.inNode = NNN; ret.datatype=NewDT;
		ret.nodeptr = NNPR; ret.offset = NOFF; ret.datasize = NDS; return ret;
    } else if(NewDT.equals("b")) {
      EventIn ret = new EventInSFBool(); ret.command = name; ret.inNode = NNN; ret.datatype=NewDT;
		ret.nodeptr = NNPR; ret.offset = NOFF; ret.datasize = NDS; return ret;
    } else if(NewDT.equals("f")) {
      EventIn ret = new EventInSFInt32(); ret.command = name; ret.inNode = NNN; ret.datatype=NewDT;
		ret.nodeptr = NNPR; ret.offset = NOFF; ret.datasize = NDS; return ret;
    } else if(NewDT.equals("u")) {
      EventIn ret = new EventInSFVec3f(); ret.command = name; ret.inNode = NNN; ret.datatype=NewDT;
		ret.nodeptr = NNPR; ret.offset = NOFF; ret.datasize = NDS; return ret;
    }
    // Return default
    EventInMFNode ret = new EventInMFNode();
    System.out.println ("WARNING: getEventIn - don't know how to handle " + name
		+ " asked for " + st + " returning Class EventInMFnode");

    ret.command = name; ret.inNode = NNN; ret.datatype=NewDT; ret.nodeptr=NNPR; ret.offset=NOFF;
    ret.datasize = NDS;
    return ret;
  }







  // Means of getting a handle to an EventOut of this node

  public EventOut      getEventOut(String name) throws InvalidEventOutException {
      StringTokenizer tokens;
	String NNN = "nodeFrom_getEventOut";

    String st = Browser.SendEventType(NodeName, name, "eventOut");
    // Did this fail??? if so, then keep trying...
    if (st.equals("")) {
     // Do we have a "_changed" at the end????
     if (name.length() > 8) {
	// possibly....
	if (name.substring(name.length()-8).equals ("_changed")) {
	  //System.out.println ("java::eventOut:: removing changed from " + name);
          name = name.substring(0,name.length()-8);
	  //System.out.println ("java::eventOut:: done " + name);
          st = Browser.SendEventType(NodeName, name, "eventOut");
	}
      }
    }

    tokens = new StringTokenizer (st);
    String NNPR = tokens.nextToken();
    String NOFF = tokens.nextToken();
    String NDS = tokens.nextToken();
    String NewDT = tokens.nextToken();
    //System.out.println ("EventOut: NNPR " + NNPR + " NOFF " + NOFF +
    //	" NDS " + NDS + " NewDT " + NewDT);


    // check out the return values specified in CFuncs/EAIServ.c
    if(NewDT.equals("p")) {
      EventOut ret = new EventOutMFString(); ret.command = name; ret.outNode = NNN; ret.datatype=NewDT;
		ret.nodeptr = NNPR; ret.offset = NOFF; ret.datasize = NDS; return ret;
    } else if(NewDT.equals("k")) {
      EventOut ret = new EventOutSFImage(); ret.command = name; ret.outNode = NNN; ret.datatype=NewDT;
		ret.nodeptr = NNPR; ret.offset = NOFF; ret.datasize = NDS; return ret;
    } else if(NewDT.equals("e")) {
      EventOut ret = new EventOutSFTime(); ret.command = name; ret.outNode = NNN; ret.datatype=NewDT;
		ret.nodeptr = NNPR; ret.offset = NOFF; ret.datasize = NDS; return ret;
    } else if(NewDT.equals("c")) {
      EventOut ret = new EventOutSFColor(); ret.command = name; ret.outNode = NNN; ret.datatype=NewDT;
		ret.nodeptr = NNPR; ret.offset = NOFF; ret.datasize = NDS; return ret;
    } else if(NewDT.equals("l")) {
      EventOut ret = new EventOutMFColor(); ret.command = name; ret.outNode = NNN; ret.datatype=NewDT;
		ret.nodeptr = NNPR; ret.offset = NOFF; ret.datasize = NDS; return ret;
    } else if(NewDT.equals("d")) {
      EventOut ret = new EventOutSFFloat(); ret.command = name; ret.outNode = NNN; ret.datatype=NewDT;
		ret.nodeptr = NNPR; ret.offset = NOFF; ret.datasize = NDS; return ret;
    } else if(NewDT.equals("m")) {
      EventOut ret = new EventOutMFFloat(); ret.command = name; ret.outNode = NNN; ret.datatype=NewDT;
		ret.nodeptr = NNPR; ret.offset = NOFF; ret.datasize = NDS; return ret;
    } else if(NewDT.equals("o")) {
      EventOut ret = new EventOutMFInt32(); ret.command = name; ret.outNode = NNN; ret.datatype=NewDT;
		ret.nodeptr = NNPR; ret.offset = NOFF; ret.datasize = NDS; return ret;
    } else if(NewDT.equals("h")) {
      EventOut ret = new EventOutSFNode(); ret.command = name; ret.outNode = NNN; ret.datatype=NewDT;
		ret.nodeptr = NNPR; ret.offset = NOFF; ret.datasize = NDS; 
		return ret;
    } else if(NewDT.equals("r")) {
      EventOut ret = new EventOutMFRotation(); ret.command = name; ret.outNode = NNN; ret.datatype=NewDT;
		ret.nodeptr = NNPR; ret.offset = NOFF; ret.datasize = NDS; return ret;
    } else if(NewDT.equals("s")) {
      EventOut ret = new EventOutMFVec2f(); ret.command = name; ret.outNode = NNN; ret.datatype=NewDT;
		ret.nodeptr = NNPR; ret.offset = NOFF; ret.datasize = NDS; return ret;
    } else if(NewDT.equals("j")) {
      EventOut ret = new EventOutSFVec2f(); ret.command = name; ret.outNode = NNN; ret.datatype=NewDT;
		ret.nodeptr = NNPR; ret.offset = NOFF; ret.datasize = NDS; return ret;
    } else if(NewDT.equals("l")) {
      EventOut ret = new EventOutMFVec3f(); ret.command = name; ret.outNode = NNN; ret.datatype=NewDT;
		ret.nodeptr = NNPR; ret.offset = NOFF; ret.datasize = NDS; return ret;
    } else if(NewDT.equals("q")) {
      EventOut ret = new EventOutMFNode(); ret.command = name; ret.outNode = NNN; ret.datatype=NewDT;
		ret.nodeptr = NNPR; ret.offset = NOFF; ret.datasize = NDS;
	return ret;
    } else if(NewDT.equals("i")) {
      EventOut ret = new EventOutSFRotation(); ret.command = name; ret.outNode = NNN;ret.datatype=NewDT;
		ret.nodeptr = NNPR; ret.offset = NOFF; ret.datasize = NDS; return ret;
    } else if(NewDT.equals("g")) {
      EventOut ret = new EventOutSFString(); ret.command = name; ret.outNode = NNN; ret.datatype=NewDT;
		ret.nodeptr = NNPR; ret.offset = NOFF; ret.datasize = NDS; return ret;
    } else if(NewDT.equals("b")) {
      EventOut ret = new EventOutSFBool(); ret.command = name; ret.outNode = NNN; ret.datatype=NewDT;
		ret.nodeptr = NNPR; ret.offset = NOFF; ret.datasize = NDS; return ret;
    } else if(NewDT.equals("f")) {
      EventOut ret = new EventOutSFInt32(); ret.command = name; ret.outNode = NNN; ret.datatype=NewDT;
		ret.nodeptr = NNPR; ret.offset = NOFF; ret.datasize = NDS; return ret;
    } else if(NewDT.equals("u")) {
      EventOut ret = new EventOutSFVec3f(); ret.command = name; ret.outNode = NNN; ret.datatype=NewDT;
		ret.nodeptr = NNPR; ret.offset = NOFF; ret.datasize = NDS; return ret;
    }
    // unknown type - throw an exception

	throw new InvalidEventOutException();
  }

}
