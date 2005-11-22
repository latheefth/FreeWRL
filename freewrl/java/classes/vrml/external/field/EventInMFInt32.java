package vrml.external.field;
import vrml.external.field.FieldTypes;
import vrml.external.Node;
import vrml.external.Browser;

public class EventInMFInt32 extends EventIn {

  public EventInMFInt32() { EventType = FieldTypes.MFINT32; }

  public void          setValue(int value[]) throws IllegalArgumentException {
	int count;
	if (value == null) {
		throw new IllegalArgumentException();
	}
	Browser.newSendEvent(this, " " + value.length);
	
	for (count = 0; count < value.length; count++) {
		Browser.newSendEvent(this, " " + value[count]);
	}
    return;
  }

  public void          set1Value(int index, int value) throws IllegalArgumentException {
	if (index < 0) {
		throw new IllegalArgumentException();
	}
	Browser.newSendEvent(this, " " + index + " " + value);

  return;
  }
}
