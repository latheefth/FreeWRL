package vrml.external.field;
import vrml.external.field.FieldTypes;
import vrml.external.Node;
import vrml.external.Browser;

public class EventInMFFloat extends EventIn {

  public EventInMFFloat() { EventType = FieldTypes.MFFLOAT; }

  public void          setValue() {
    System.out.println ("ERROR: eventinMFFloat Not Implemented");
    return;
  }

  public void          set1Value(int index) {
    System.out.println ("ERROR: eventinMFFloat.set1vanue Not Implemented");

  return;
  }
}
