// Specification of the base interface for all eventOut types.
package vrml.external.field;
import vrml.external.Browser;
import vrml.external.field.FieldTypes;

public class EventOut {

 public int EventType = FieldTypes.UnknownType;
 public String outNode;	// Node to send the command to... NULL if not
			// a get value from viewer call (ie, a Listener
			// response...
 public String command;	// the actual command...
 public String RLreturn;	// If this is a register listener response...


  // Get the type of this EventOut (specified in FieldTypes.java)
  public int           getType() {
    return EventType;
  }

  // Mechanism for setting up an observer for this field.
  // The EventOutObserver's callback gets called when the
  // EventOut's value changes.
  public void          advise(EventOutObserver f, Object userData) {

    Browser.RegisterListener (f, userData, outNode, command, EventType);
  return;
  }

  // terminate notification on the passed EventOutObserver
  public void          unadvise(EventOutObserver f) {
    System.out.println ("ERROR: Eventout.unadvise Not Implemented yet...");

  return;
  }
}
