package vrml.external.field;
import vrml.external.field.FieldTypes;
import vrml.external.Node;
import vrml.external.Browser;

public class EventInMFFloat extends EventIn {

  public EventInMFFloat() { EventType = FieldTypes.MFFLOAT; }

  public void          setValue(float[] value) throws IllegalArgumentException {
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

  public void          set1Value(int index, float value) throws IllegalArgumentException {
       if (index < 0) {
                throw new IllegalArgumentException();
        }
        Browser.newSendEvent(this, " " + index + " " + value);
  return;
  }
}
