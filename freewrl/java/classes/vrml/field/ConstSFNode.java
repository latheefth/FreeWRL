package vrml.field;
import vrml.*;
import vrml.BaseNode;

public class ConstSFNode extends ConstField {
    BaseNode node;

    public ConstSFNode() {} /* only for internal use */
    public ConstSFNode(BaseNode node) {
        this.node = node;
    }
    public BaseNode getValue() {
        __updateRead();
        return node;
    }


    public String toString() {
        return FWHelper.nodeToString(node);
    }

    public void __fromPerl(String str) {
        node = new vrml.node.Node(str);
    }

    public String __toPerl() {
        return node._get_nodeid();;
    }
}