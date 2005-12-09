package vrml.external.field;
import vrml.external.field.FieldTypes;
import vrml.external.Browser;

public class EventInSFBool extends EventIn {
  public EventInSFBool() { EventType = FieldTypes.SFBOOL; }

  public void          setValue(boolean value) {
  if (value) {
	System.out.println("sending TRUE");
    Browser.newSendEvent (this, "TRUE");
  } else {
    Browser.newSendEvent (this, "FALSE");
  }
  return;
  }
}
