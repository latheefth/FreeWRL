package vrml.node; 
import java.util.Hashtable;
import vrml.*;
//
// This is the general Script class, to be subclassed by all scripts.
// Note that the provided methods allow the script author to explicitly
// throw tailored exceptions in case something goes wrong in the
// script.
//
public abstract class Script extends BaseNode
{ 
   public Script() { 
   }

   // This method is called before any event is generated
   public void initialize() { }

   // Get a Field by name.
   //   Throws an InvalidFieldException if fieldName isn't a valid
   //   field name for a node of this type.
   protected final Field getField(String fieldName) {
       return FWJavaScript.getField(this, fieldName, "field");
   }

   // Get an EventOut by name.
   //   Throws an InvalidEventOutException if eventOutName isn't a valid
   //   eventOut name for a node of this type.
   // spec: protected
   public final Field getEventOut(String eventOutName) {
       return FWJavaScript.getField(this, eventOutName, "eventOut");
   }

   // Get an EventIn by name.
   //   Throws an InvalidEventInException if eventInName isn't a valid
   //   eventIn name for a node of this type.
   protected final Field getEventIn(String eventInName) {
       try {
	   return FWJavaScript.getField(this, eventInName, "eventIn");
       } catch (InvalidFieldException ex) {
	   throw new InvalidEventInException(ex.getMessage());
       }
   }

   // processEvents() is called automatically when the script receives 
   //   some set of events. It shall not be called directly except by its subclass.
   //   count indicates the number of events delivered.
   public void processEvents(int count, Event events[]) { }

   // processEvent() is called automatically when the script receives 
   // an event. 
   public void processEvent(Event event) { }

   // eventsProcessed() is called after every invocation of processEvents().
   public void eventsProcessed() { }

   // shutdown() is called when this Script node is deleted.
   public void shutdown() { }
}


