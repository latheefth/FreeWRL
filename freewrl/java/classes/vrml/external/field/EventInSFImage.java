package vrml.external.field;
import vrml.external.field.FieldTypes;
import vrml.external.Node;
import vrml.external.Browser;

public class EventInSFImage extends EventIn {

  public EventInSFImage() { EventType = FieldTypes.SFIMAGE; }

  public void          setValue(Integer value) {
    int count;

    // lord knows if this will work....
    Browser.SendEvent (inNode , command, "" + value);

  return;
  }
}
