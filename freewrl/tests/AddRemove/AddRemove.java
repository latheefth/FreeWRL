// Simple applet illustrating use of add/removeChildren fields.
//  Slightly modified applet of AddRemoveTest found in the EAI Spec
//  It is better policy to include some error handling in the code, this example
//  is designed to teach both Java and EAI basics.
// 1/98 Daniel.Schneider@tecfa.unige.ch
// Freeware (of course)

import java.awt.*;
import java.applet.*;
import vrml.external.field.*;
import vrml.external.Node;
import vrml.external.Browser;
import vrml.external.exception.*;

public class AddRemove extends Applet {
  TextArea output = null;
  boolean error = false;

  // Browser we're using
  Browser browser;
  // Root of the scene graph (to which we add our nodes)
  Node root;
  // Shape group hierarchy
  Node[] shape;

  // EventIns of the root node
  EventInMFNode addChildren;
  EventInMFNode removeChildren;

  // Add and Remove Buttons
  Button addButton;
  Button removeButton;

  public void init() {
System.out.println ("start of init");
    // Paint the Java Buttons
    add(addButton = new Button("Add Sphere"));
    add(removeButton = new Button("Remove Sphere"));

System.out.println ("buttons added");
    // Get the browser
    browser = Browser.getBrowser(this);

    // Get root node of the scene
    try 
	{ root = browser.getNode("ROOT"); }
    catch (InvalidNodeException e) {
	System.out.println ("caught " + e);
    }

    // Instantiate (get handle to) the EventIn objects
    addChildren = (EventInMFNode) root.getEventIn("addChildren");
    removeChildren = (EventInMFNode) root.getEventIn("removeChildren");
    
    // Instantiate a lovely blue ball
    shape = browser.createVrmlFromString("Shape {\n" +
					 "  appearance Appearance {\n" +
					 "    material Material {\n" +
					 "      diffuseColor 0.2 0.2 0.8\n" +
					 "    }\n" +
					 "  }\n" +
					 "  geometry Sphere {}\n" +
					 "}\n");
  }
  
  // Main Program
  // Event Handler for the Java Buttons

  public boolean action(Event event, Object what) {
    // Catch all Events from type Button
    if (event.target instanceof Button) {
      Button b = (Button) event.target;
      // either addButton or removeButton has been clicked on
      if (b == addButton) {
	addChildren.setValue(shape);
      }
      else if (b == removeButton) {
	removeChildren.setValue(shape);
      }
    }
    return true;
  }
}
