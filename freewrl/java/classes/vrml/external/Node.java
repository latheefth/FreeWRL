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
  public String NodeAddress = "UNDEFINED";

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

      StringTokenizer tokens;
      String nodeptr;
      String offset;
      String datasize;
      String datatype;

  // Return the type that is asked for. To determine the
  // subclass, look at the string. 
  // There *HAS* to be a better way than this!
  // Maybe, send a GFT to the FreeWRL Browser, to see what
  // it thinks...

   // lets remember the addChildren and removeChildren specials...
   // FreeWRL does not differentiate between addChildren and removeChildren
   if (name.equals("addChildren") || (name.equals("removeChildren"))) {
      EventInMFNode ret = new EventInMFNode();
      ret.command = name; ret.inNode = NodeName; return ret;
    }

    String st = Browser.SendEventType(NodeName, name, "EventIn");

	System.out.println ("Node.java  st = " + st);

    // Did this fail??? if so, then keep trying...
    if (st.equals("")) {

      // Do we have a "set_" at the beginning????
      if (name.length() > 5) {
	  // possibly....
	  if (name.substring(0,4).equals ("set_")) {
            name = name.substring(4);
            st = Browser.SendEventType(NodeName, name, "EventIn");
	  }
      }
    }

    tokens = new StringTokenizer (st);
    nodeptr = tokens.nextToken();
    offset = tokens.nextToken();
    datasize = tokens.nextToken();
    datatype = tokens.nextToken();
    System.out.println ("EventOut: nodeptr " + nodeptr + " offset " + offset +
	" datasize " + datasize + " datatype " + datatype);


    // check out the return values specified in CFuncs/EAIServ.c
    if(datatype.equals("p")) {
      EventIn ret = new EventInMFString(); ret.command = name; ret.inNode = NodeName; ret.datatype=datatype;
		ret.nodeptr = nodeptr; ret.offset = offset; ret.datasize = datasize; return ret;
    } else if(datatype.equals("k")) {
      EventIn ret = new EventInSFImage(); ret.command = name; ret.inNode = NodeName; ret.datatype=datatype;
		ret.nodeptr = nodeptr; ret.offset = offset; ret.datasize = datasize; return ret;
    } else if(datatype.equals("e")) {
      EventIn ret = new EventInSFTime(); ret.command = name; ret.inNode = NodeName; ret.datatype=datatype;
		ret.nodeptr = nodeptr; ret.offset = offset; ret.datasize = datasize; return ret;
    } else if(datatype.equals("c")) {
      EventIn ret = new EventInSFColor(); ret.command = name; ret.inNode = NodeName; ret.datatype=datatype;
		ret.nodeptr = nodeptr; ret.offset = offset; ret.datasize = datasize; return ret;
    } else if(datatype.equals("l")) {
      EventIn ret = new EventInMFColor(); ret.command = name; ret.inNode = NodeName; ret.datatype=datatype;
		ret.nodeptr = nodeptr; ret.offset = offset; ret.datasize = datasize; return ret;
    } else if(datatype.equals("d")) {
      EventIn ret = new EventInSFFloat(); ret.command = name; ret.inNode = NodeName; ret.datatype=datatype;
		ret.nodeptr = nodeptr; ret.offset = offset; ret.datasize = datasize; return ret;
    } else if(datatype.equals("m")) {
      EventIn ret = new EventInMFFloat(); ret.command = name; ret.inNode = NodeName; ret.datatype=datatype;
		ret.nodeptr = nodeptr; ret.offset = offset; ret.datasize = datasize; return ret;
    } else if(datatype.equals("o")) {
      EventIn ret = new EventInMFInt32(); ret.command = name; ret.inNode = NodeName; ret.datatype=datatype;
		ret.nodeptr = nodeptr; ret.offset = offset; ret.datasize = datasize; return ret;
    } else if(datatype.equals("h")) {
      EventIn ret = new EventInSFNode(); ret.command = name; ret.inNode = NodeName; ret.datatype=datatype;
		ret.nodeptr = nodeptr; ret.offset = offset; ret.datasize = datasize; return ret;
    } else if(datatype.equals("r")) {
      EventIn ret = new EventInMFRotation(); ret.command = name; ret.inNode = NodeName; ret.datatype=datatype;
		ret.nodeptr = nodeptr; ret.offset = offset; ret.datasize = datasize; return ret;
    } else if(datatype.equals("s")) {
      EventIn ret = new EventInMFVec2f(); ret.command = name; ret.inNode = NodeName; ret.datatype=datatype;
		ret.nodeptr = nodeptr; ret.offset = offset; ret.datasize = datasize; return ret;
    } else if(datatype.equals("j")) {
      EventIn ret = new EventInSFVec2f(); ret.command = name; ret.inNode = NodeName; ret.datatype=datatype;
		ret.nodeptr = nodeptr; ret.offset = offset; ret.datasize = datasize; return ret;
    } else if(datatype.equals("l")) {
      EventIn ret = new EventInMFVec3f(); ret.command = name; ret.inNode = NodeName; ret.datatype=datatype;
		ret.nodeptr = nodeptr; ret.offset = offset; ret.datasize = datasize; return ret;
    } else if(datatype.equals("q")) {
      EventIn ret = new EventInMFNode(); ret.command = name; ret.inNode = NodeName; ret.datatype=datatype;
		ret.nodeptr = nodeptr; ret.offset = offset; ret.datasize = datasize; return ret;
    //} else if(datatype.equals("unknown")) {
     // EventIn ret = new EventInMField(); ret.command = name; ret.inNode = NodeName; ret.datatype=datatype;
//		ret.nodeptr = nodeptr; ret.offset = offset; ret.datasize = datasize; return ret;
    } else if(datatype.equals("i")) {
      EventIn ret = new EventInSFRotation(); ret.command = name; ret.inNode = NodeName;ret.datatype=datatype;
		ret.nodeptr = nodeptr; ret.offset = offset; ret.datasize = datasize; return ret;
    } else if(datatype.equals("g")) {
      EventIn ret = new EventInSFString(); ret.command = name; ret.inNode = NodeName; ret.datatype=datatype;
		ret.nodeptr = nodeptr; ret.offset = offset; ret.datasize = datasize; return ret;
    } else if(datatype.equals("b")) {
      EventIn ret = new EventInSFBool(); ret.command = name; ret.inNode = NodeName; ret.datatype=datatype;
		ret.nodeptr = nodeptr; ret.offset = offset; ret.datasize = datasize; return ret;
    } else if(datatype.equals("f")) {
      EventIn ret = new EventInSFInt32(); ret.command = name; ret.inNode = NodeName; ret.datatype=datatype;
		ret.nodeptr = nodeptr; ret.offset = offset; ret.datasize = datasize; return ret;
    } else if(datatype.equals("u")) {
      EventIn ret = new EventInSFVec3f(); ret.command = name; ret.inNode = NodeName; ret.datatype=datatype;
		ret.nodeptr = nodeptr; ret.offset = offset; ret.datasize = datasize; return ret;
    }
    // Return default
    EventInMFNode ret = new EventInMFNode();
    System.out.println ("WARNING: getEventIn - don't know how to handle " + name
		+ " asked for " + st + " returning Class EventInMFnode");

    ret.command = name; ret.inNode = NodeName; 
    return ret;
  }







  // Means of getting a handle to an EventOut of this node

  public EventOut      getEventOut(String name) throws InvalidEventOutException {
      StringTokenizer tokens;
      String nodeptr;
      String offset;
      String datasize;
      String datatype;

    String st = Browser.SendEventType(NodeName, name, "EventOut");
    // Did this fail??? if so, then keep trying...
    if (st.equals("")) {
     // Do we have a "_changed" at the end????
     if (name.length() > 8) {
	// possibly....
	if (name.substring(name.length()-8).equals ("_changed")) {
	  System.out.println ("java::eventOut:: removing changed from " + name);
          name = name.substring(0,name.length()-8);
	  System.out.println ("java::eventOut:: done " + name);
          st = Browser.SendEventType(NodeName, name, "EventOut");
	}
      }
    }
    tokens = new StringTokenizer (st);
    nodeptr = tokens.nextToken();
    offset = tokens.nextToken();
    datasize = tokens.nextToken();
    datatype = tokens.nextToken();

    System.out.println ("EventOut: nodeptr " + nodeptr + " offset " + offset +
	" datasize " + datasize + " datatype " + datatype);

    // check out the return values specified in CFuncs/EAIServ.c
    if(datatype.equals("p")) {
      EventOut ret = new EventOutMFString(); ret.command = name; ret.outNode = NodeName; ret.datatype=datatype;
		ret.nodeptr = nodeptr; ret.offset = offset; ret.datasize = datasize; return ret;
    } else if(datatype.equals("k")) {
      EventOut ret = new EventOutSFImage(); ret.command = name; ret.outNode = NodeName; ret.datatype=datatype;
		ret.nodeptr = nodeptr; ret.offset = offset; ret.datasize = datasize; return ret;
    } else if(datatype.equals("e")) {
      EventOut ret = new EventOutSFTime(); ret.command = name; ret.outNode = NodeName; ret.datatype=datatype;
		ret.nodeptr = nodeptr; ret.offset = offset; ret.datasize = datasize; return ret;
    } else if(datatype.equals("c")) {
      EventOut ret = new EventOutSFColor(); ret.command = name; ret.outNode = NodeName; ret.datatype=datatype;
		ret.nodeptr = nodeptr; ret.offset = offset; ret.datasize = datasize; return ret;
    } else if(datatype.equals("l")) {
      EventOut ret = new EventOutMFColor(); ret.command = name; ret.outNode = NodeName; ret.datatype=datatype;
		ret.nodeptr = nodeptr; ret.offset = offset; ret.datasize = datasize; return ret;
    } else if(datatype.equals("d")) {
      EventOut ret = new EventOutSFFloat(); ret.command = name; ret.outNode = NodeName; ret.datatype=datatype;
		ret.nodeptr = nodeptr; ret.offset = offset; ret.datasize = datasize; return ret;
    } else if(datatype.equals("m")) {
      EventOut ret = new EventOutMFFloat(); ret.command = name; ret.outNode = NodeName; ret.datatype=datatype;
		ret.nodeptr = nodeptr; ret.offset = offset; ret.datasize = datasize; return ret;
    } else if(datatype.equals("o")) {
      EventOut ret = new EventOutMFInt32(); ret.command = name; ret.outNode = NodeName; ret.datatype=datatype;
		ret.nodeptr = nodeptr; ret.offset = offset; ret.datasize = datasize; return ret;
    } else if(datatype.equals("h")) {
      EventOut ret = new EventOutSFNode(); ret.command = name; ret.outNode = NodeName; ret.datatype=datatype;
		ret.nodeptr = nodeptr; ret.offset = offset; ret.datasize = datasize; return ret;
    } else if(datatype.equals("r")) {
      EventOut ret = new EventOutMFRotation(); ret.command = name; ret.outNode = NodeName; ret.datatype=datatype;
		ret.nodeptr = nodeptr; ret.offset = offset; ret.datasize = datasize; return ret;
    } else if(datatype.equals("s")) {
      EventOut ret = new EventOutMFVec2f(); ret.command = name; ret.outNode = NodeName; ret.datatype=datatype;
		ret.nodeptr = nodeptr; ret.offset = offset; ret.datasize = datasize; return ret;
    } else if(datatype.equals("j")) {
      EventOut ret = new EventOutSFVec2f(); ret.command = name; ret.outNode = NodeName; ret.datatype=datatype;
		ret.nodeptr = nodeptr; ret.offset = offset; ret.datasize = datasize; return ret;
    } else if(datatype.equals("l")) {
      EventOut ret = new EventOutMFVec3f(); ret.command = name; ret.outNode = NodeName; ret.datatype=datatype;
		ret.nodeptr = nodeptr; ret.offset = offset; ret.datasize = datasize; return ret;
    } else if(datatype.equals("q")) {
      EventOut ret = new EventOutMFNode(); ret.command = name; ret.outNode = NodeName; ret.datatype=datatype;
		ret.nodeptr = nodeptr; ret.offset = offset; ret.datasize = datasize; return ret;
    } else if(datatype.equals("unknown")) {
      EventOut ret = new EventOutMField(); ret.command = name; ret.outNode = NodeName; ret.datatype=datatype;
		ret.nodeptr = nodeptr; ret.offset = offset; ret.datasize = datasize; return ret;
    } else if(datatype.equals("i")) {
      EventOut ret = new EventOutSFRotation(); ret.command = name; ret.outNode = NodeName;ret.datatype=datatype;
		ret.nodeptr = nodeptr; ret.offset = offset; ret.datasize = datasize; return ret;
    } else if(datatype.equals("g")) {
      EventOut ret = new EventOutSFString(); ret.command = name; ret.outNode = NodeName; ret.datatype=datatype;
		ret.nodeptr = nodeptr; ret.offset = offset; ret.datasize = datasize; return ret;
    } else if(datatype.equals("b")) {
      EventOut ret = new EventOutSFBool(); ret.command = name; ret.outNode = NodeName; ret.datatype=datatype;
		ret.nodeptr = nodeptr; ret.offset = offset; ret.datasize = datasize; return ret;
    } else if(datatype.equals("f")) {
      EventOut ret = new EventOutSFInt32(); ret.command = name; ret.outNode = NodeName; ret.datatype=datatype;
		ret.nodeptr = nodeptr; ret.offset = offset; ret.datasize = datasize; return ret;
    } else if(datatype.equals("u")) {
      EventOut ret = new EventOutSFVec3f(); ret.command = name; ret.outNode = NodeName; ret.datatype=datatype;
		ret.nodeptr = nodeptr; ret.offset = offset; ret.datasize = datasize; return ret;
    }
    
    // Return default
    System.out.println ("WARNING: getEventOut - don't know how to handle " + name
  		+ " returning Class EventOut");
    throw new InvalidEventOutException();
  }
}
