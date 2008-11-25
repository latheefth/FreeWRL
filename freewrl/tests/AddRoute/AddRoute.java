import vrml.external.Browser;
import vrml.external.field.*;
import vrml.external.Node;

import java.applet.*;

public class AddRoute extends Applet
{
	public static void main(String[] args)
	{
		AddRoute ee = new AddRoute();
		ee.start();
	}
	public void start() {

	Browser browser;
	Node root;
	
	Node[] Shape;
	Node[] Clock;
	Node[] ColumnPath;
	
	// EventIns of the root node
	EventInMFNode addChildren;
	EventInMFNode removeChildren;
	
	// Get the browser
	browser = Browser.getBrowser(this);
	// Get root node of the scene
	root = browser.getNode("ROOT"); 
	
	// Instantiate (get handle to) the EventIn objects
	
	addChildren = (EventInMFNode) root.getEventIn("addChildren");
	removeChildren = (EventInMFNode) root.getEventIn("removeChildren");
	
	
	// Instantiate a lovely shape
	    Shape = browser.createVrmlFromString(
		"Transform { \n" +
		"	rotation 0.0 0.0 1.0 0.0 \n" +
		"	children Shape { \n" +
		"		appearance Appearance { \n" +
		"			material Material {} \n" +
		"		} \n" +
		"		geometry Cylinder { \n" +
		"			height 1.0 \n" +
		"			radius 0.2 \n" +
		"		} \n" +
		"	} \n" +
		"} ");
	
	Clock =  browser.createVrmlFromString(
		"TimeSensor { \n" +
		"	cycleInterval 4.0 \n" +
		"	loop TRUE \n" +
		"}");
	
	ColumnPath =  browser.createVrmlFromString(
		"OrientationInterpolator { \n" +
		"	key [ 0.0 0.5 1.0 ] \n" +
		"	keyValue [ \n" +
		"		0.0 0.0 1.0 0.0, \n" +
		"		0.0 0.0 1.0 3.14, \n" +
		"		0.0 0.0 1.0 6.28 \n" +
		"	] \n" +
		"}");
	
	// Add the clock and interpolator here, even if they are not used yet.
	addChildren.setValue(Clock);
	addChildren.setValue(ColumnPath);
	addChildren.setValue(Shape);

	try {
		browser.addRoute (Clock[0], "fraction_changed", ColumnPath[0], "set_fraction");
		browser.addRoute (ColumnPath[0], "value_changed", Shape[0], "set_rotation");
	} catch (IllegalArgumentException e) {
		System.out.println ("caught " + e);
	}


	// sleep for a bit, then remove routes
	try {Thread.sleep (2000);} catch (InterruptedException f) { }

	System.out.println ("removing ROUTE");
	try {
		browser.deleteRoute (Clock[0], "fraction_changed", ColumnPath[0], "set_fraction");
		browser.deleteRoute (ColumnPath[0], "value_changed", Shape[0], "set_rotation");
	} catch (IllegalArgumentException e) {
		System.out.println ("caught " + e);
	}

	}
}
