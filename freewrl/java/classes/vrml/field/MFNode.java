package vrml.field;
import vrml.*;
import java.util.StringTokenizer;
import vrml.BaseNode;

public class MFNode extends MField {

    public MFNode() {}
    public MFNode(BaseNode[] node) {
        this(node.length, node);
    }
    public MFNode(int size, BaseNode[] node) {
        for (int i = 0; i < size; i++)
            __vect.addElement(new ConstSFNode(node[i]));
    }

    public void getValue(BaseNode[] node) {
        __updateRead();
        int size = __vect.size();
        for (int i = 0; i < size; i++) {
            node[i] = ((ConstSFNode) __vect.elementAt(i)).getValue();
        }
    }

    public BaseNode get1Value(int index) {
        __update1Read(index);
        return ((ConstSFNode) __vect.elementAt(index)).getValue();
    }

    public void setValue(BaseNode[] node) {
        setValue(node.length, node);
    }
    public void setValue(int size, BaseNode[] node) {
        __vect.clear();
        for (int i = 0; i < size; i++)
            __vect.addElement(new ConstSFNode(node[i]));
        __updateWrite();
    };
    public void set1Value(int index, BaseNode node) {
        __set1Value(index, new ConstSFNode(node));
    };
    public void set1Value(int index, SFNode sfNode) {
        sfNode.__updateRead();
        __set1Value(index, new ConstSFNode(sfNode.node));
    };
    public void set1Value(int index, ConstSFNode sfNode) {
        __set1Value(index, sfNode);
    };
    public void addValue(BaseNode node) {
        __addValue(new ConstSFNode(node));
    };
    public void addValue(SFNode sfNode) {
        sfNode.__updateRead();
        __addValue(new ConstSFNode(sfNode.node));
    };
    public void addValue(ConstSFNode sfNode) {
        __addValue(sfNode);
    };
    public void insertValue(int index, BaseNode node) {
        __insertValue(index, new ConstSFNode(node));
    };
    public void insertValue(int index, SFNode sfNode) {
        sfNode.__updateRead();
        __insertValue(index, new ConstSFNode(sfNode.node));
    };
    public void insertValue(int index, ConstSFNode sfNode) {
        __insertValue(index, sfNode);
    };

    public String toString() {
        StringBuffer sb = new StringBuffer("[");
        int size = __vect.size();
        for (int i = 0; i < size; i++) {
            if (i > 0) sb.append(", ");
            sb.append(__vect.elementAt(i));
        }
        return sb.append("]").toString();
    }

    public void __fromPerl(String str) {
        StringTokenizer st = new StringTokenizer(str,",");
        while (st.hasMoreTokens()) {
            ConstSFNode sf = new ConstSFNode();
            sf.__fromPerl(st.nextToken());
            __vect.addElement(sf);
        }
    }

    public String __toPerl() {
        StringBuffer sb = new StringBuffer("");
        int size = __vect.size();
        for (int i = 0; i < size; i++) {
            if (i > 0) sb.append(",");
            sb.append(((ConstSFNode) __vect.elementAt(i)).__toPerl());
        }
        return sb.append("").toString();
    }
}