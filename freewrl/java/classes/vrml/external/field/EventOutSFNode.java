package vrml.external.field;
//JAS import java.util.*;
//JAS import vrml.external.Browser;
import vrml.external.field.FieldTypes;


public class EventOutSFNode extends EventOut {
  public EventOutSFNode() {EventType = FieldTypes.SFNODE;}

  public String       getValue() {
	//System.out.println ("in EventoutsfNode, we have " + RLreturn);

    System.out.println ("ERROR: EventOutSFNode not implemented");
    return RLreturn;
  }
}
