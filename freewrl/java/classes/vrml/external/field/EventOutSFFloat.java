package vrml.external.field;
import java.util.*;
import vrml.external.Browser;
import vrml.external.field.FieldTypes;


public class EventOutSFFloat extends EventOut {
	public EventOutSFFloat() {EventType = FieldTypes.SFFLOAT;}

	public float getValue() {
		String rep;

		rep = Browser.SendEventOut (nodeptr, offset, datasize, datatype, command);
		return Float.valueOf(rep).floatValue();
	}
}
