package vrml;
import vrml.FWJavaScript;
import java.io.IOException;

public class FWJavaScriptBinding {
    BaseNode node;
    String fieldName;
    String lastUpdate;
    
    
    public FWJavaScriptBinding(BaseNode n, String f) {
	node = n; fieldName = f;
    }
    public BaseNode node() {return node;}
    public String field() {return fieldName;}

    public void updateRead(Field field) {
	if (lastUpdate == FWJavaScript.reqid)
	    return;
	try {
	    // System.err.println("FWJB: "+node._get_nodeid()+".readField("+fieldName+")");
	    FWJavaScript.out.println("READFIELD");
	    FWJavaScript.out.println(node._get_nodeid());
	    FWJavaScript.out.println(fieldName);
	    FWJavaScript.out.flush();
	    
	    field.__fromPerl(FWJavaScript.in.readLine().trim());
	    lastUpdate = FWJavaScript.reqid;
	    // System.err.println("FWJB: got: "+field);
	} catch (IOException e) {
	    throw new InternalError("Communication error: "+e);
	}
    }

    public void updateWrite(Field field) {
	// System.err.println("FWJB: add_touched:"+node._get_nodeid()+".readField("+fieldName+")");
	FWJavaScript.add_touched(field);
	lastUpdate = FWJavaScript.reqid;
    }

    public String toString() {
	return node._get_nodeid()+"."+fieldName;
    }
}
