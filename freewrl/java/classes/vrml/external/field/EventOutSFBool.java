package vrml.external.field;
import java.util.*;
import vrml.external.Browser;
import vrml.external.field.FieldTypes;


public class EventOutSFBool extends EventOut {
  public EventOutSFBool() {EventType = FieldTypes.SFBOOL;}

  public boolean       getValue() {
	//System.out.println ("in Eventoutsfbool, we have " + RLreturn);

    return RLreturn.equals("TRUE");
  }
}
