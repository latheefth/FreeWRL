package vrml.external.field;
import vrml.external.field.FieldTypes;
//JAS import vrml.external.Node;
//JAS import vrml.external.Browser;

public class EventInMFVec2f extends EventIn {

  public EventInMFVec2f() { EventType = FieldTypes.MFVEC2F; }

  public void          setValue() {
    System.out.println ("ERROR: eventinMFVec2f Not Implemented");
    return;
  }

  public void          set1Value(int index) {
    System.out.println ("ERROR: eventinMFVec2f.set1vanue Not Implemented");

  return;
  }
}
