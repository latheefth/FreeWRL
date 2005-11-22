package vrml.external.field;
import vrml.external.field.FieldTypes;
import vrml.external.Node;
import vrml.external.Browser;

public class EventInMFColor extends EventIn {

  public EventInMFColor() { EventType = FieldTypes.MFCOLOR; }

  public void          setValue(float[][] value) throws IllegalArgumentException {
        int count;
        if (value == null) {
                throw new IllegalArgumentException();
        }
        Browser.newSendEvent(this,  " " + value.length);
        for (count = 0; count < value.length; count++) {
                if ((value[count].length < 3) || (value[count][0] < 0) || (value[count][1] < 0) || (value[count][2] < 0) || (value[count][0] > 1) || (value[count][1] > 1) || (value[count][2] > 1)) {
                        throw new IllegalArgumentException();
                }
                Browser.newSendEvent(this,  " " + value[count][0] + " " + value[count][1] + " " + value[count][2]);
        }

    return;
  }

  public void          set1Value(int index, float[] value) throws IllegalArgumentException {

        if ((value == null) || (index < 0) || (value.length < 3) || (value[0] < 0) || (value[1] < 0) || (value[2] < 0) || (value[0] > 1) || (value[1] > 1) || (value[2] > 1)) {
                throw new IllegalArgumentException();
        }
        Browser.newSendEvent(this,  " " + index + " " + value[0] + " " + value[1] + " " + value[2]);
  return;
  }
}
