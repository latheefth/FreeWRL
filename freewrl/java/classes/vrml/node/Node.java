package vrml.node; 
import java.util.Hashtable;
import vrml.Field;
import vrml.ConstField;
import vrml.BaseNode;
import vrml.FWJavaScript;
import vrml.*;

//
// This is the general Node class
// 
public class Node extends BaseNode
{ 
    public Node(String id) {
	super(id);
    }


   // Get an EventIn by name. Return value is write-only.
   //   Throws an InvalidEventInException if eventInName isn't a valid
   //   eventIn name for a node of this type.
   public final Field getEventIn(String eventInName) {
       try {
	   return FWJavaScript.getField(this, eventInName, "eventIn");
       } catch (InvalidFieldException ex) {
	   throw new InvalidEventInException(ex.getMessage());
       }
   }

   // Get an EventOut by name. Return value is read-only.
   //   Throws an InvalidEventOutException if eventOutName isn't a valid
   //   eventOut name for a node of this type.
   public final ConstField getEventOut(String eventOutName) {
       return FWJavaScript.getEventOut(this, eventOutName);
   }

   // Get an exposed field by name. 
   //   Throws an InvalidExposedFieldException if exposedFieldName isn't a valid
   //   exposedField name for a node of this type.
   public final Field getExposedField(String exposedFieldName) {
       try {
	   return FWJavaScript.getField(this, exposedFieldName, "exposedField");
       } catch (InvalidFieldException ex) {
	   throw new InvalidExposedFieldException(ex.getMessage());
       }
   }
}


