package vrml.external.field;
import vrml.external.field.FieldTypes;
import vrml.external.Browser;


public class EventInSFTime extends EventIn {
  public EventInSFTime() { EventType = FieldTypes.SFTIME;}

  public void          setValue(String value) {
    Browser.SendEvent (inNode , command, "\"" + value + "\"");
    return;
  }
}
