package vrml.external.field;
import java.util.*;
import vrml.external.Browser;
import vrml.external.field.FieldTypes;


public class EventOutSFBool extends EventOut {
	public EventOutSFBool() {EventType = FieldTypes.SFBOOL;}

	public boolean       getValue() {
		return RLreturn.equals("TRUE");
	}
}
