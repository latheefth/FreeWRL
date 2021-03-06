// Tiny3D -- simple example of an "authoring tool" in Java
// using the VRML External Interface.

// Kenneth B. Russell (kbrussel@media.mit.edu)

import java.applet.*;
import java.awt.*;
import java.util.*;
// import netscape.javascript.JSObject;
import vrml.external.field.*;
import vrml.external.exception.*;
import vrml.external.Node;
import vrml.external.Browser;
import TinyClump;
import VbRotation;
import VbVec3f;

public class Tiny3D extends Applet {
  // One instance variable per slider, to tell what controls what
  Scrollbar transx;
  Scrollbar transy;
  Scrollbar transz;
  Scrollbar rotx;
  Scrollbar roty;
  Scrollbar rotz;
  Scrollbar scalex;
  Scrollbar scaley;
  Scrollbar scalez;
  Scrollbar colr;
  Scrollbar colg;
  Scrollbar colb;

  // Browser we're using
  Browser browser;
  // Root of the scene graph (to which we add our clumps)
  Node root;

  // Array of the transform hierarchies we've added to the scene
  Vector clumps;
  int topIndex;

  // Current clump we're editing
  TinyClump curClump = null;

  static int transformRange = 40;

  public void makeButton(String name,
			 GridBagLayout gridBag,
			 GridBagConstraints c) {
    Button button = new Button(name);
    gridBag.setConstraints(button, c);
    add(button);
  }

  public void makeLabel(String name,
			GridBagLayout gridBag,
			GridBagConstraints c) {
    Label label = new Label(name, Label.CENTER);
    gridBag.setConstraints(label, c);
    add(label);
  }

  public Scrollbar makeScrollbar(GridBagLayout gridBag,
				 GridBagConstraints c,
				 int orientation,
				 int value,
				 int min,
				 int max) {
    Scrollbar bar = new Scrollbar(orientation, value, 1, min, max);
    gridBag.setConstraints(bar, c);
    add(bar);
    return bar;
  }

  public void init() {

    // Initialization of our instance variables
    clumps = new Vector();
    topIndex = -1;

    //
    // Initialize connection to Cosmo Player
    //

    // JSObject win = JSObject.getWindow(this);
    // JSObject doc = (JSObject) win.getMember("document");
    // JSObject embeds = (JSObject) doc.getMember("embeds");
    // browser = (Browser) embeds.getSlot(0);
    browser = Browser.getBrowser(this);

    //
    // Get handle to root of the scene graph
    //

    try {
      root = browser.getNode("ROOT");
    }
    catch (InvalidNodeException e) {
      System.out.println("PROBLEMS!: " + e);
    }

    //
    // Build the user interface
    //

    float firstColWidth = 0.04f;
    float thirdColWidth = 0.04f;
    float secondColWidth = 1.0f - firstColWidth - thirdColWidth;

    GridBagLayout gridBag = new GridBagLayout();
    GridBagConstraints c = new GridBagConstraints();
    setFont(new Font("Helvetica", Font.PLAIN, 12));
    setLayout(gridBag);

    c.fill = GridBagConstraints.BOTH;
    c.gridwidth = 2;
    c.gridheight = 2;
    c.weightx = firstColWidth;
    //    c.weighty = 0.3333;
    makeButton("Cube", gridBag, c);
    c.gridheight = 1;
    c.gridwidth = 3;
    c.weightx = secondColWidth;
    //    c.weighty = 0.16667;
    makeLabel("Translation", gridBag, c);
    c.gridwidth = 3;
    c.weightx = thirdColWidth;
    //    c.weighty = 0.16667;
    makeLabel("Color", gridBag, c);
    c.gridheight = 5;
    c.gridwidth = 1;
    c.gridx = 5;
    c.weightx = thirdColWidth / 3.0;
    //    c.weighty = 0.83333;
    colr = makeScrollbar(gridBag, c, Scrollbar.VERTICAL,
			 0, 0, 255);
    c.gridx = 6;
    colg = makeScrollbar(gridBag, c, Scrollbar.VERTICAL,
			 0, 0, 255);
    c.gridx = 7;
    colb = makeScrollbar(gridBag, c, Scrollbar.VERTICAL,
			 0, 0, 255);
    c.gridwidth = 1;
    c.gridheight = 1;
    c.gridy = 1;
    c.gridx = 2;
    c.weightx = secondColWidth / 3.0;
    //    c.weighty = 0.16667;
    transx = makeScrollbar(gridBag, c, Scrollbar.HORIZONTAL,
			   transformRange / 2, 0, transformRange);
    c.gridx = 3;
    transy = makeScrollbar(gridBag, c, Scrollbar.HORIZONTAL,
			   transformRange / 2, 0, transformRange);
    c.gridx = 4;
    transz = makeScrollbar(gridBag, c, Scrollbar.HORIZONTAL,
			   transformRange / 2, 0, transformRange);
    c.gridwidth = 2;
    c.gridheight = 2;
    c.weightx = firstColWidth;
    //    c.weighty = 0.3333;
    c.gridx = 0;
    c.gridy = 2;
    //    c.gridy = GridBagConstraints.RELATIVE;
    makeButton("Sphere", gridBag, c);
    c.gridwidth = 3;
    c.gridheight = 1;
    c.gridx = 2;
    c.weightx = secondColWidth;
    //    c.weighty = 0.16667;
    makeLabel("Rotation", gridBag, c);
    c.gridwidth = 1;
    c.gridx = 2;
    c.gridy = 3;
    c.weightx = secondColWidth / 3.0;
    rotx = makeScrollbar(gridBag, c, Scrollbar.HORIZONTAL,
			 0, 0, 360);
    c.gridx = 3;
    roty = makeScrollbar(gridBag, c, Scrollbar.HORIZONTAL,
			 0, 0, 360);
    c.gridx = 4;
    rotz = makeScrollbar(gridBag, c, Scrollbar.HORIZONTAL,
			 0, 0, 360);
    c.gridwidth = 2;
    c.gridheight = 2;
    c.weightx = firstColWidth;
    //    c.weighty = 0.3333;
    c.gridx = 0;
    c.gridy = 4;
    makeButton("Cone", gridBag, c);
    c.gridx = 2;
    c.gridwidth = 3;
    c.gridheight = 1;
    c.weightx = secondColWidth;
    //    c.weighty = 0.16667;
    makeLabel("Scale", gridBag, c);
    c.gridx = 2;
    c.gridy = 5;
    c.gridheight = 1;
    c.gridwidth = 1;
    c.weightx = secondColWidth / 3.0;
    scalex = makeScrollbar(gridBag, c, Scrollbar.HORIZONTAL,
			   10, 1, 100);
    c.gridx = 3;
    scaley = makeScrollbar(gridBag, c, Scrollbar.HORIZONTAL,
			   10, 1, 100);
    c.gridx = 4;
    scalez = makeScrollbar(gridBag, c, Scrollbar.HORIZONTAL,
			   10, 1, 100);
  }

  public Browser getBrowser() {
    return browser;
  }

  public boolean handleEvent(Event event) {
    if (event.target instanceof Scrollbar)
      {
	if (curClump == null)
	  return true;

	// Depends on which scrollbar changed.
	Scrollbar bar = (Scrollbar) event.target;

	if ((bar == transx) ||
	    (bar == transy) ||
	    (bar == transz))
	  {
	    float[] val = new float[3];
	    // Center about origin
	    val[0] = (float) transx.getValue() - (float) transformRange / 2.0f;
	    val[1] = (float) transy.getValue() - (float) transformRange / 2.0f;
	    val[2] = (float) transz.getValue() - (float) transformRange / 2.0f;
	    curClump.set_translation.setValue(val);
	  }

	if ((bar == scalex) ||
	    (bar == scaley) ||
	    (bar == scalez))
	  {
	    float[] val = new float[3];
	    // Center about origin
	    val[0] = ((float) scalex.getValue()) / 10.0f;
	    val[1] = ((float) scaley.getValue()) / 10.0f;
	    val[2] = ((float) scalez.getValue()) / 10.0f;
	    curClump.set_scale.setValue(val);
	  }

	if ((bar == colr) ||
	    (bar == colg) ||
	    (bar == colb))
	  {
	    float[] val = new float[3];
	    // Center about origin
	    val[0] = (float) (255 - colr.getValue()) / 255.0f;
	    val[1] = (float) (255 - colg.getValue()) / 255.0f;
	    val[2] = (float) (255 - colb.getValue()) / 255.0f;
	    curClump.set_diffuseColor.setValue(val);
	  }

	if ((bar == rotx) ||
	    (bar == roty) ||
	    (bar == rotz))
	  {
	    curClump.xang = rotx.getValue();
	    curClump.yang = roty.getValue();
	    curClump.zang = rotz.getValue();

	    VbRotation xrot = new VbRotation(new VbVec3f(1.0f, 0.0f, 0.0f),
					     (float) curClump.xang *
					     (float) (Math.PI / 180.0));
	    VbRotation yrot = new VbRotation(new VbVec3f(0.0f, 1.0f, 0.0f),
					     (float) curClump.yang *
					     (float) (Math.PI / 180.0));
	    VbRotation zrot = new VbRotation(new VbVec3f(0.0f, 0.0f, 1.0f),
					     (float) curClump.zang *
					     (float) (Math.PI / 180.0));

	    VbRotation r1 = xrot.times(yrot);
	    VbRotation r2 = r1.times(zrot);

	    VbVec3f axis = new VbVec3f();
	    float angle = r2.getValue(axis);
	    float[] val = new float[4];
	    float[] axisVal = axis.getValue();
	    val[0] = axisVal[0];
	    val[1] = axisVal[1];
	    val[2] = axisVal[2];
	    val[3] = angle;
	    curClump.set_rotation.setValue(val);
	  }

	return true;
      }
    return super.handleEvent(event);
  }

  public boolean action(Event event, Object what) {
    if (event.target instanceof Button)
      {
	TinyClump newClump = null;

	Button b = (Button) event.target;
	if (b.getLabel() == "Cube") {
	  newClump = new TinyClump(this, TinyClump.CUBE);
	}

	if (b.getLabel() == "Sphere") {
	  newClump = new TinyClump(this, TinyClump.SPHERE);
	}

	if (b.getLabel() == "Cone") {
	  newClump = new TinyClump(this, TinyClump.CONE);
	}

	// Add this clump to the end of the vector.
	// We really only do this to protect our clumps
	// from Java's garbage collector
	clumps.addElement(newClump);
	// Increment the index of the high element
	topIndex++;

	// Initialize slider's values from this clump
	curClump = newClump;
	initSlidersFromClump();

	// Add clump to the scene graph
	try {
	  EventInMFNode addChildren =
	    (EventInMFNode) root.getEventIn("addChildren");
	  addChildren.setValue(curClump.transArray);
	}
	catch (InvalidEventInException e) {
	  System.out.println("PROBLEMS!: " + e);
	}
      }

    // All other actions ignored.
    return true;
  }

  // Make a given clump the one we're editing
  public void makeCurrent(TinyClump which) {
    curClump = which;
    initSlidersFromClump();
  }

  void initSlidersFromClump() {
    float[] val = curClump.scale_changed.getValue();
    scalex.setValue((int) (val[0] * 10.0f));
    scaley.setValue((int) (val[1] * 10.0f));
    scalez.setValue((int) (val[2] * 10.0f));

    val = curClump.translation_changed.getValue();
    transx.setValue((int) val[0] + (transformRange / 2));
    transy.setValue((int) val[1] + (transformRange / 2));
    transz.setValue((int) val[2] + (transformRange / 2));

    val = curClump.diffuseColor_changed.getValue();
    colr.setValue(255 - (int) (val[0] * 255.0f));
    colg.setValue(255 - (int) (val[1] * 255.0f));
    colb.setValue(255 - (int) (val[2] * 255.0f));

    rotx.setValue(curClump.xang);
    roty.setValue(curClump.yang);
    rotz.setValue(curClump.zang);
  }
}
