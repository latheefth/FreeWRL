package vrml.external.field;
import java.util.*;
import vrml.external.Browser;
import vrml.external.field.FieldTypes;


public class EventOutSFFloat extends EventOut {
  public EventOutSFFloat() {EventType = FieldTypes.SFFLOAT;}

  public String       getValue() {
	//System.out.println ("in EventoutsfFloat, we have " + RLreturn);

    System.out.println ("ERROR: EventOutSFFloat not implemented");
    return RLreturn;
  }
}
