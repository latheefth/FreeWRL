package vrml.external.field;
import vrml.external.field.FieldTypes;
import vrml.external.Node;
import vrml.external.Browser;

public class EventInSFInt32 extends EventIn {

  public EventInSFInt32() { EventType = FieldTypes.SFINT32; }

  public void          setValue(Integer value) {
    int count;

    Browser.SendEvent (nodeptr,offset,datasize , datatype, "" + value);

  return;
  }
}
