package vrml.external.field;
import vrml.external.field.FieldTypes;
import vrml.external.Browser;

public class EventInSFVec3f extends EventIn {
  public EventInSFVec3f() { EventType = FieldTypes.SFVEC3F; };

  public void          setValue(float[] value) {
    Browser.newSendEvent (this, "" + value[0] + " " + value[1] + " " + value[2]);

  return;
  }

}
