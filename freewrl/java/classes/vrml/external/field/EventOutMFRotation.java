// This one is interesting... Is it a real node, or are we parsing
// the value of something like a string of VRML stuff????

package vrml.external.field;
import vrml.external.Node;
import vrml.external.Browser;
import vrml.external.field.FieldTypes;

public class EventOutMFRotation extends EventOutMField {
   public EventOutMFRotation() { EventType = FieldTypes.MFROTATION; }

  public Node[]        getValue() {
    Node x[] = {new Node()};
    String rep;

    // Ok, lets first see what the value of this thing is...
    // I guess, lets see if the RLreturn field has something...
    // if it does, then this is an ASYNC value sent from the FreeWRL VRML Browser.

    if (RLreturn == null) {
      rep = Browser.SendEventOut (nodeptr, offset, datasize, datatype, command);
    } else {
      rep = RLreturn;
    }

    System.out.println ("DEBUG: EventOutMFRotation getValue - rep = " + rep);

    // ok, so now we have some VRML text in the String rep... 
    // XXX - maybe we can split this up on matching []'s???????

    x[0].NodeName = rep;

    return x;
  }

    
  public Node          get1Value(int index) {
    Node all[] = getValue();

    return all[index]; 
  }
}

