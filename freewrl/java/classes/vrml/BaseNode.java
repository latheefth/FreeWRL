package vrml;

// This is the general BaseNode class
// 
public abstract class BaseNode 
{
    String nodeid;  // id for communication

    public BaseNode() {}
    
    public BaseNode(String id) {
	nodeid = id;
    }

    public void _set_nodeid(String id) {
	nodeid = id;
    }
    public String _get_nodeid() {
	return nodeid;
    }

    // Returns the type of the node.  If the node is a prototype
    // it returns the name of the prototype.
    public String getType() { 
	return FWJavaScript.getNodeType(this);
    }
    
    // Get the Browser that this node is contained in.
    public Browser getBrowser() { 
	return FWJavaScript.getBrowser();
    }
}

