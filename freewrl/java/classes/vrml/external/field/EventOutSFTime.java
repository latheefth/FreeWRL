package vrml.external.field;
import java.util.*;
import vrml.external.field.FieldTypes;
import vrml.external.Browser;


public class EventOutSFTime extends EventOut {
  public EventOutSFTime() {EventType = FieldTypes.SFTIME;}

  public int           getSize() {
    System.out.println ("ERROR: EventOutSFTime Not Implemented");
    return 0;
  }
}
