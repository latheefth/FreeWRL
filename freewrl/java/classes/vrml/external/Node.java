package vrml.external;

import vrml.external.field.*;
import vrml.external.exception.*;
import java.util.*;
import java.applet.*;
import java.awt.*;
import java.net.*;
import java.io.*;
import java.lang.reflect.*;

public class Node {
  // Get a string specifying the type of this node. May return the
  // name of a PROTO, or the class name

  public String NodeName  = "Not initiated yet";

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

  // Return the type that is asked for. To determine the
  // subclass, look at the string. 
  // There *HAS* to be a better way than this!
  // Maybe, send a GFT to the FreeWRL Browser, to see what
  // it thinks...

   // lets remember the addChildren and removeChildren specials...
   if (name.equals("addChildren") || (name.equals("removeChildren"))) {
      EventInMFNode ret = new EventInMFNode();
      ret.command = name; ret.inNode = NodeName; return ret;
    }

    String st = Browser.SendEventType(NodeName, name);

    // Did this fail??? if so, then keep trying...
    if (st.equals("")) {

      // Do we have a "set_" at the beginning????
      if (name.length() > 5) {
	  // possibly....
	  if (name.substring(0,4).equals ("set_")) {
            name = name.substring(4);
            st = Browser.SendEventType(NodeName, name);
	  }
      }
    }

    if(st.equals("MFString")) {
      EventIn ret = new EventInMFString(); ret.command = name; ret.inNode = NodeName; return ret; 
    } else if(st.equals("SFImage")) {
      EventIn ret = new EventInSFImage(); ret.command = name; ret.inNode = NodeName; return ret; 
    } else if(st.equals("SFTime")) {
      EventIn ret = new EventInSFTime(); ret.command = name; ret.inNode = NodeName; return ret; 
    } else if(st.equals("SFColor")) {
      EventIn ret = new EventInSFColor(); ret.command = name; ret.inNode = NodeName; return ret; 
    } else if(st.equals("MFColor")) {
      EventIn ret = new EventInMFColor(); ret.command = name; ret.inNode = NodeName; return ret; 
    } else if(st.equals("SFFloat")) {
      EventIn ret = new EventInSFFloat(); ret.command = name; ret.inNode = NodeName; return ret; 
    } else if(st.equals("MFFloat")) {
      EventIn ret = new EventInMFFloat(); ret.command = name; ret.inNode = NodeName; return ret; 
    } else if(st.equals("MFInt32")) {
      EventIn ret = new EventInMFInt32(); ret.command = name; ret.inNode = NodeName; return ret; 
    } else if(st.equals("SFNode")) {
      EventIn ret = new EventInSFNode(); ret.command = name; ret.inNode = NodeName; return ret; 
    } else if(st.equals("MFRotation")) {
      EventIn ret = new EventInMFRotation(); ret.command = name; ret.inNode = NodeName; return ret; 
    } else if(st.equals("MFVec2f")) {
      EventIn ret = new EventInMFVec2f(); ret.command = name; ret.inNode = NodeName; return ret; 
    } else if(st.equals("SFVec2f")) {
      EventIn ret = new EventInSFVec2f(); ret.command = name; ret.inNode = NodeName; return ret; 
    } else if(st.equals("MFVec3f")) {
      EventIn ret = new EventInMFVec3f(); ret.command = name; ret.inNode = NodeName; return ret; 
    } else if(st.equals("MFNode")) {
      EventIn ret = new EventInMFNode(); ret.command = name; ret.inNode = NodeName; return ret; 
    } else if(st.equals("SFRotation")) {
      EventIn ret = new EventInSFRotation(); ret.command = name; ret.inNode = NodeName; return ret; 
    } else if(st.equals("SFString")) {
      EventIn ret = new EventInSFString(); ret.command = name; ret.inNode = NodeName; return ret; 
    } else if(st.equals("SFBool")) {
      EventIn ret = new EventInSFBool(); ret.command = name; ret.inNode = NodeName; return ret; 
    } else if(st.equals("SFInt32")) {
      EventIn ret = new EventInSFInt32(); ret.command = name; ret.inNode = NodeName; return ret; 
    } else if(st.equals("SFVec3f")) {
      EventIn ret = new EventInSFVec3f(); ret.command = name; ret.inNode = NodeName; return ret; 
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

    String st = Browser.SendEventType(NodeName, name);

    // Did this fail??? if so, then keep trying...
    if (st.equals("")) {
     // Do we have a "_changed" at the end????
     if (name.length() > 8) {
	// possibly....
	if (name.substring(name.length()-8).equals ("_changed")) {
	  System.out.println ("java::eventOut:: removing changed from " + name);
          name = name.substring(0,name.length()-8);
	  System.out.println ("java::eventOut:: done " + name);
          st = Browser.SendEventType(NodeName, name);
	}
      }
    }

    if(st.equals("MFString")) {
      EventOut ret = new EventOutMFString(); ret.command = name; ret.outNode = NodeName; return ret;
    } else if(st.equals("SFImage")) {
      EventOut ret = new EventOutSFImage(); ret.command = name; ret.outNode = NodeName; return ret;
    } else if(st.equals("SFTime")) {
      EventOut ret = new EventOutSFTime(); ret.command = name; ret.outNode = NodeName; return ret;
    } else if(st.equals("SFColor")) {
      EventOut ret = new EventOutSFColor(); ret.command = name; ret.outNode = NodeName; return ret;
    } else if(st.equals("MFColor")) {
      EventOut ret = new EventOutMFColor(); ret.command = name; ret.outNode = NodeName; return ret;
    } else if(st.equals("SFFloat")) {
      EventOut ret = new EventOutSFFloat(); ret.command = name; ret.outNode = NodeName; return ret;
    } else if(st.equals("MFFloat")) {
      EventOut ret = new EventOutMFFloat(); ret.command = name; ret.outNode = NodeName; return ret;
    } else if(st.equals("MFInt32")) {
      EventOut ret = new EventOutMFInt32(); ret.command = name; ret.outNode = NodeName; return ret;
    } else if(st.equals("SFNode")) {
      EventOut ret = new EventOutSFNode(); ret.command = name; ret.outNode = NodeName; return ret;
    } else if(st.equals("MFRotation")) {
      EventOut ret = new EventOutMFRotation(); ret.command = name; ret.outNode = NodeName; return ret;
    } else if(st.equals("MFVec2f")) {
      EventOut ret = new EventOutMFVec2f(); ret.command = name; ret.outNode = NodeName; return ret;
    } else if(st.equals("SFVec2f")) {
      EventOut ret = new EventOutSFVec2f(); ret.command = name; ret.outNode = NodeName; return ret;
    } else if(st.equals("MFVec3f")) {
      EventOut ret = new EventOutMFVec3f(); ret.command = name; ret.outNode = NodeName; return ret;
    } else if(st.equals("MFNode")) {
      EventOut ret = new EventOutMFNode(); ret.command = name; ret.outNode = NodeName; return ret;
    } else if(st.equals("MField")) {
      EventOut ret = new EventOutMField(); ret.command = name; ret.outNode = NodeName; return ret;
    } else if(st.equals("SFRotation")) {
      EventOut ret = new EventOutSFRotation(); ret.command = name; ret.outNode = NodeName; return ret;
    } else if(st.equals("SFString")) {
      EventOut ret = new EventOutSFString(); ret.command = name; ret.outNode = NodeName; return ret;
    } else if(st.equals("SFBool")) {
      EventOut ret = new EventOutSFBool(); ret.command = name; ret.outNode = NodeName; return ret;
    } else if(st.equals("SFInt32")) {
      EventOut ret = new EventOutSFInt32(); ret.command = name; ret.outNode = NodeName; return ret;
    } else if(st.equals("SFVec3f")) {
      EventOut ret = new EventOutSFVec3f(); ret.command = name; ret.outNode = NodeName; return ret;
    }
    
    // Return default
    System.out.println ("WARNING: getEventOut - don't know how to handle " + name
  		+ " returning Class EventOut");
    throw new InvalidEventOutException();
  }
}
