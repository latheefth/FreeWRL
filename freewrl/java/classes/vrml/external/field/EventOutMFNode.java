package vrml.external.field;
import vrml.external.Node;
import java.util.*;
import vrml.external.field.FieldTypes;
import vrml.external.Browser;


public class EventOutMFNode extends EventOutMField {

  // retnodes is an array of string values.
  // sizeof is the size of retnodes.

  Node[] retnodes;
  int sizeof = 0;


  public EventOutMFNode() {EventType = FieldTypes.MFNODE;}

  public Node[]      getValue() {
    String rep;
    StringTokenizer tokens;
    int counttokens;

    if (command != null) {
      rep = Browser.SendEventOut (outNode, command);
      tokens = new StringTokenizer (rep);
    } else {
      tokens = new StringTokenizer (RLreturn);
    }

    counttokens = tokens.countTokens();
    retnodes = new Node[counttokens];
    sizeof = 0;

    while (sizeof < counttokens) {

      retnodes[sizeof] = new Node();
      rep = tokens.nextToken();
      retnodes[sizeof].NodeName = new String(rep);
      sizeof ++;
    }
    return retnodes;
  }

  public Node        get1Value(int index) {

    // MyNode is used to ensure that the getValue call is called before this.

    Node[] MyNode = getValue();

    if ((index > sizeof) || (index < 0)) {
	System.out.println ("EventOutMFNode.get1Value - index " + index +
		" out of range");
	index = 0;
    }
    return MyNode[index];
  }

  public int           getSize() {
	return sizeof;
  }
}
