package vrml.external.field;
import vrml.external.field.FieldTypes;
import vrml.external.Browser;

public class EventInMFString extends EventIn {
	public EventInMFString() { EventType = FieldTypes.MFSTRING; }

	public void          setValue(String[] value) {
		int count;

		for (count = 0; count < value.length; count++) {
			Browser.newSendEvent (this, "\"" + value[count] + "\"");
		}
	}

	public void          set1Value(int index, String value) {
		Browser.newSendEvent (this, "\"" + value + "\"");
	}
}
