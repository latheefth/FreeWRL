package vrml.external.field;
import vrml.external.field.FieldTypes;
import vrml.external.Node;
import vrml.external.Browser;

public class EventInSFColor extends EventIn {

  public EventInSFColor() { EventType = FieldTypes.SFCOLOR; }

  public void          setValue() {
    System.out.println ("ERROR: eventinSFColor Not Implemented");
    return;
  }

  public void          set1Value(int index) {
    System.out.println ("ERROR: eventinSFColor.set1vanue Not Implemented");

  return;
  }
}
