package vrml.external.field;
import vrml.external.field.FieldTypes;
//JAS import vrml.external.Node;
//JAS import vrml.external.Browser;

public class EventInMFVec3f extends EventIn {

  public EventInMFVec3f() { EventType = FieldTypes.MFVEC3F; }

  public void          setValue() {
    System.out.println ("ERROR: eventinMFVec3f Not Implemented");
    return;
  }

  public void          set1Value(int index) {
    System.out.println ("ERROR: eventinMFVec3f.set1vanue Not Implemented");

  return;
  }
}
