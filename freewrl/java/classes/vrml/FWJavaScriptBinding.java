package vrml;
import vrml.FWJavaScript;
import vrml.node.BaseNode;

public class FWJavaScriptBinding {
	BaseNode node;
	String field;
	public FWJavaScriptBinding(BaseNode n, String f) {
		node = n; field = f;
	}
	public void invoke() {
		FWJavaScript.add_touched(this);
	};
	public BaseNode node() {return node;}
	public String field() {return field;}
}
