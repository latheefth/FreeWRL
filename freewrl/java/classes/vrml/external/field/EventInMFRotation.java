package vrml.external.field;
import vrml.external.field.FieldTypes;
//JAS import vrml.external.Node;
//JAS import vrml.external.Browser;

public class EventInMFRotation extends EventIn {

  public EventInMFRotation() { EventType = FieldTypes.MFROTATION; }

  public void          setValue() {
    System.out.println ("ERROR: eventinMFRotation Not Implemented");
    return;
  }

  public void          set1Value(int index) {
    System.out.println ("ERROR: eventinMFRotation.set1vanue Not Implemented");

  return;
  }
}
