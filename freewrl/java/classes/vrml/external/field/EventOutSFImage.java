package vrml.external.field;
//JAS import java.util.*;
import vrml.external.field.FieldTypes;
//JAS import vrml.external.Browser;


public class EventOutSFImage extends EventOut {
  public EventOutSFImage() {EventType = FieldTypes.SFIMAGE;}

  public int           getSize() {
    System.out.println ("ERROR: EventOutSFImage Not Implemented");
    return 0;
  }
}
