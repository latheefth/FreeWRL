package vrml.external.field;
import vrml.external.field.FieldTypes;
import vrml.external.Browser;

public class EventInSFRotation extends EventIn {
  public EventInSFRotation() { EventType = FieldTypes.SFROTATION; }

  public void          setValue(float[] value) {
    Browser.SendEvent (inNode , command, "" + value[0] + " " + value[1] +
                  " " + value[2] + " " + value[3]);
    return;
  }
}
