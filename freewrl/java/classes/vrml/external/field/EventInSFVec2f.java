package vrml.external.field;
import vrml.external.field.FieldTypes;
import vrml.external.Browser;

public class EventInSFVec2f extends EventIn {
  public EventInSFVec2f() { EventType = FieldTypes.SFVEC2F; };

  public void          setValue(float[] value) {
    Browser.SendEvent (inNode , command, "" + value[0] + 
           " " + value[1]);

  return;
  }

}
