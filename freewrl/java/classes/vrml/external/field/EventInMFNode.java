package vrml.external.field;
import vrml.external.field.FieldTypes;
import vrml.external.Node;
import vrml.external.Browser;

public class EventInMFNode extends EventIn {

  public EventInMFNode() { EventType = FieldTypes.MFNODE; }

  public void          setValue(Node[] node) {
    int count;

    for (count = 0; count < node.length; count++) {
      Browser.SendChildEvent (nodeptr,offset, command, node[count].nodeptr);
    }
  return;
  }

  public void          set1Value(int index, Node node) {
      Browser.SendChildEvent (nodeptr,offset, command, node.nodeptr);

  return;
  }
}
