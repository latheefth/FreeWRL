package vrml.external.field;
import vrml.external.field.FieldTypes;
import vrml.external.Node;
import vrml.external.Browser;

public class EventInMFNode extends EventIn {

  public EventInMFNode() { EventType = FieldTypes.MFNODE; }

  public void          setValue(Node[] node) {
    int count;

    for (count = 0; count < node.length; count++) {
      Browser.SendChildEvent (inNode , command, node[count].NodeName);
    }
System.out.println ("EventInMFNode done");
  return;
  }

  public void          set1Value(int index, Node node) {
    System.out.println ("ERROR: eventinmfnode.set1vanue Not Implemented");
    // Browser.SendEvent (inNode , command, "TRUE");

  return;
  }
}
