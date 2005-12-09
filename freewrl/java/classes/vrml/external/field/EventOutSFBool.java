package vrml.external.field;
//JAS import java.util.*;
import vrml.external.Browser;
import vrml.external.field.FieldTypes;


public class EventOutSFBool extends EventOut {
	public EventOutSFBool() {EventType = FieldTypes.SFBOOL;}

	public boolean       getValue() {
                String rep;

                rep = Browser.SendEventOut (nodeptr, offset, datasize, datatype, command);
System.out.println ("EventOutSFBool returns " + rep);

                //return Float.valueOf(rep).floatValue();

		return rep.equals("TRUE");
	}
}
