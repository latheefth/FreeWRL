package vrml.external.field;
import vrml.external.field.FieldTypes;
import vrml.external.Browser;

public class EventInMFString extends EventIn {
  public EventInMFString() { EventType = FieldTypes.MFSTRING; }

  public void          setValue(String[] value) {
    // System.out.println ("DEBUG: EventInMFString number of Strings " + value.length);
    
    int count;

    for (count = 0; count < value.length; count++) {
      // System.out.println ("DEBUG: EventInMFString, sending " + value[count]);
      Browser.SendEvent (inNode , command, "\"" + value[count] + "\"");
    }
    // System.out.println ("DEBUG: EventInMFString SENT finished ");
    return;
  }
  public void          set1Value(int index, String value) {
    // System.out.println ("DEBUG: EventInMFString, sending " + value);
    Browser.SendEvent (inNode , command, "\"" + value + "\"");
    return;
  }
}
