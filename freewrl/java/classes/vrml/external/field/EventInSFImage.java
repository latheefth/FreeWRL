package vrml.external.field;
import vrml.external.field.FieldTypes;
//JAS import vrml.external.Node;
import vrml.external.Browser;

public class EventInSFImage extends EventIn {

  public EventInSFImage() { EventType = FieldTypes.SFIMAGE; }

  public void          setValue(Integer value) {
    int count;

    Browser.newSendEvent (this, "" + value);

  return;
  }
}
