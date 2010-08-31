//AUTOMATICALLY GENERATED BY genfields.pl.
//DO NOT EDIT!!!!

package vrml.field;
import vrml.*;
import java.io.BufferedReader;
import java.io.PrintWriter;
import java.io.IOException;

public class ConstMFNode extends ConstMField {
    public ConstMFNode() {
    }

    public ConstMFNode(BaseNode[] node) {
        this(node.length, node);
    }

    public ConstMFNode(int size, BaseNode[] node) {
        for (int i = 0; i < size; i++)
            __vect.addElement(new ConstSFNode(node[i]));
    }

    public void getValue(BaseNode[] node) {
        __updateRead();
        int size = __vect.size();
        for (int i = 0; i < size; i++) {
            ConstSFNode sfNode = (ConstSFNode) __vect.elementAt(i);
            node[i] = sfNode.node;
        }
    }

    public BaseNode get1Value(int index) {
        __update1Read(index);
        return ((ConstSFNode) __vect.elementAt(index)).getValue();
    }

    public String toString() {
        __updateRead();
        StringBuffer sb = new StringBuffer("[");
        int size = __vect.size();
        for (int i = 0; i < size; i++) {
            if (i > 0) sb.append(", ");
            sb.append(__vect.elementAt(i));
        }
        return sb.append("]").toString();
    }

    public void __fromPerl(BufferedReader in)  throws IOException {
        __vect.clear();
	String lenline = in.readLine();
	//System.out.println ("__fromPerl, read in length as " + lenline);
        //int len = Integer.parseInt(in.readLine());
	int len = Integer.parseInt(lenline);
        for (int i = 0; i < len; i++) {
            ConstSFNode sf = new ConstSFNode();
            sf.__fromPerl(in);
            __vect.addElement(sf);
        }
    }

    public void __toPerl(PrintWriter out)  throws IOException {
        StringBuffer sb = new StringBuffer("");
        int size = __vect.size();
	//out.print(size);
        for (int i = 0; i < size; i++) {
            ((ConstSFNode) __vect.elementAt(i)).__toPerl(out);
	    if (i != (size-1)) out.print (", ");
	}
	//out.println();
    }
    //public void setOffset(String offs) { this.offset = offs; } //JAS2
    //public String getOffset() { return this.offset; } //JAS2
}