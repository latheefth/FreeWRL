package vrml.external.field;
import java.util.*;
import vrml.external.field.FieldTypes;
import vrml.external.Browser;


public class EventOutMField extends EventOut {
  public EventOutMField() {EventType = FieldTypes.UnknownType;}

  public int           getSize() {
    System.out.println ("ERROR: EventOutMField Not Implemented");
    return 0;
  }
}
