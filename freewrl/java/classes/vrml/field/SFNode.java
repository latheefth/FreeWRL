package vrml.field;
import vrml.*;
import vrml.BaseNode;

public class SFNode extends Field {
    BaseNode node;

    public SFNode() {}
    public SFNode(BaseNode node) {
        this.node = node;
    }
    public BaseNode getValue() {
        __updateRead();
        return node;
    }

    public void setValue(BaseNode node) {
        this.node = node;
        __updateWrite();
    }
    public void setValue(ConstSFNode sfNode) {
        sfNode.__updateRead();
        node = sfNode.node;
        __updateWrite();
    }
    public void setValue(SFNode sfNode) {
        sfNode.__updateRead();
        node = sfNode.node;
        __updateWrite();
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