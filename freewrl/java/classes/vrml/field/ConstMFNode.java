package vrml.field;
import vrml.*;
import java.util.StringTokenizer;
import vrml.BaseNode;

public class ConstMFNode extends ConstMField {

    public ConstMFNode() {}
    public ConstMFNode(BaseNode[] node) {
        this(node.length, node);    }
    public ConstMFNode(int size, BaseNode[] node) {
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