// Specification of the base interface for all eventIn types.
package vrml.external.field;
import vrml.external.field.FieldTypes;
import vrml.external.Browser;



public class EventIn {

  int EventType = FieldTypes.UnknownType;
  public String inNode;
  public String command;
 public String nodeptr; //pointer to start of FreeWRL structure in memory
 public String offset;  //offset of actual field in memory from base.
 public String datasize; // how long this data really is
 public String datatype; 



  // Get the type of this EventIn (specified in FieldTypes.java)
  public int           getType() {
  return EventType;
  }
}
