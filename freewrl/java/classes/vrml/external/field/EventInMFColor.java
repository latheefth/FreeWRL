package vrml.external.field;
import vrml.external.field.FieldTypes;
import vrml.external.Node;
import vrml.external.Browser;

public class EventInMFColor extends EventIn {

  public EventInMFColor() { EventType = FieldTypes.MFCOLOR; }

  public void          setValue() {
    System.out.println ("ERROR: eventinMFColor Not Implemented");
    return;
  }

  public void          set1Value(int index) {
    System.out.println ("ERROR: eventinMFColor.set1vanue Not Implemented");

  return;
  }
}
