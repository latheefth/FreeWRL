package vrml.external.field;
import vrml.external.field.FieldTypes;
import vrml.external.Browser;

public class EventInSFBool extends EventIn {
  public EventInSFBool() { EventType = FieldTypes.SFBOOL; }

  public void          setValue(boolean value) {
  if (value) {
    Browser.SendEvent (inNode , command, "TRUE");
  } else {
    Browser.SendEvent (inNode, command, "FALSE");
  }
  return;
  }
}
