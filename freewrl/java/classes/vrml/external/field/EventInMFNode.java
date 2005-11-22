package vrml.external.field;
import vrml.external.field.FieldTypes;
import vrml.external.Node;
import vrml.external.Browser;

public class EventInMFNode extends EventIn {

  public EventInMFNode() { EventType = FieldTypes.MFNODE; }

  public void          setValue(Node[] node) throws IllegalArgumentException {
    int count;

    for (count = 0; count < node.length; count++) {
	if (node[count].nodeptr == null) {
		throw new IllegalArgumentException();
	}
      Browser.SendChildEvent (nodeptr,offset, command, node[count].nodeptr);
    }
  return;
  }

  public void          set1Value(int index, Node node) throws IllegalArgumentException {
	if (node.nodeptr == null) {
		throw new IllegalArgumentException();
	}

      Browser.SendChildEvent (nodeptr,offset, command, node.nodeptr);

  return;
  }
}
