package vrml.external.field;
import vrml.external.field.FieldTypes;
import vrml.external.Browser;


public class EventInSFString extends EventIn {
  public EventInSFString() { EventType = FieldTypes.SFSTRING;}

  public void          setValue(String value) {
    Browser.SendEvent (nodeptr,offset,datasize , datatype, "\"" + value + "\"");
    return;
  }
}
