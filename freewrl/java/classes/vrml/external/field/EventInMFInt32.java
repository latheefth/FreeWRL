package vrml.external.field;
import vrml.external.field.FieldTypes;
//JAS import vrml.external.Node;
//JAS import vrml.external.Browser;

public class EventInMFInt32 extends EventIn {

  public EventInMFInt32() { EventType = FieldTypes.MFINT32; }

  public void          setValue() {
    System.out.println ("ERROR: eventinMFInt32 Not Implemented");
    return;
  }

  public void          set1Value(int index) {
    System.out.println ("ERROR: eventinMFInt32.set1vanue Not Implemented");

  return;
  }
}
