// Specification of the base interface for all eventIn types.
package vrml.external.field;
import vrml.external.field.FieldTypes;
import vrml.external.Browser;



public class EventIn {

  int EventType = FieldTypes.UnknownType;
  public String inNode;
  public String command;

  // Get the type of this EventIn (specified in FieldTypes.java)
  public int           getType() {
  return EventType;
  }
}
