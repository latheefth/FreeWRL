package vrml.field;
import vrml.*;
import java.util.StringTokenizer;
import java.util.StringTokenizer;

public class MFColor extends MField {

    public MFColor() {}
    public MFColor(float[] colors) {
        this(colors.length, colors);
    }
    public MFColor(int size, float[] colors) {
        for (int i = 0; i < size; i += 3)
            __vect.addElement(new ConstSFColor(colors[i], colors[i+1], colors[i+2]));
    }
    public MFColor(float[][] colors) {
        for (int i = 0; i < colors.length; i++)
            __vect.addElement(new ConstSFColor(colors[i][0], colors[i][1], colors[i][2]));
    }

    public void getValue(float[] colors) {
        __updateRead();
        int size = __vect.size();
        for (int i = 0; i < size; i++) {
            ConstSFColor sfColor = (ConstSFColor) __vect.elementAt(i);
            colors[3*i+0] = sfColor.getRed();
            colors[3*i+1] = sfColor.getGreen();
            colors[3*i+2] = sfColor.getBlue();
        }
    }
    public void getValue(float[][] colors) {
        __updateRead();
        int size = __vect.size();
        for (int i = 0; i < size; i++)
            ((ConstSFColor) __vect.elementAt(i)).getValue(colors[i]);
    }

    public void get1Value(int index, float[] colors) {
        __update1Read(index);
        ((ConstSFColor) __vect.elementAt(index)).getValue(colors);
    }
    public void get1Value(int index, SFColor sfColor) {
        __update1Read(index);
        sfColor.setValue((ConstSFColor) __vect.elementAt(index));
    }

    public void setValue(float[] colors) {
        setValue(colors.length, colors);
    }
    public void setValue(int size, float[] colors) {
        __vect.clear();
        for (int i = 0; i < size; i += 3)
            __vect.addElement(new ConstSFColor(colors[i], colors[i+1], colors[i+2]));
        __updateWrite();
    };
    public void set1Value(int index, float red, float green, float blue) {
        __set1Value(index, new ConstSFColor(red, green, blue));
    };
    public void set1Value(int index, SFColor sfColor) {
        sfColor.__updateRead();
        __set1Value(index, new ConstSFColor(sfColor.red, sfColor.green, sfColor.blue));
    };
    public void set1Value(int index, ConstSFColor sfColor) {
        __set1Value(index, sfColor);
    };
    public void addValue(float red, float green, float blue) {
        __addValue(new ConstSFColor(red, green, blue));
    };
    public void addValue(SFColor sfColor) {
        sfColor.__updateRead();
        __addValue(new ConstSFColor(sfColor.red, sfColor.green, sfColor.blue));
    };
    public void addValue(ConstSFColor sfColor) {
        __addValue(sfColor);
    };
    public void insertValue(int index, float red, float green, float blue) {
        __insertValue(index, new ConstSFColor(red, green, blue));
    };
    public void insertValue(int index, SFColor sfColor) {
        sfColor.__updateRead();
        __insertValue(index, new ConstSFColor(sfColor.red, sfColor.green, sfColor.blue));
    };
    public void insertValue(int index, ConstSFColor sfColor) {
        __insertValue(index, sfColor);
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
            ConstSFColor sf = new ConstSFColor();
            sf.__fromPerl(st.nextToken());
            __vect.addElement(sf);
        }
    }

    public String __toPerl() {
        StringBuffer sb = new StringBuffer("");
        int size = __vect.size();
        for (int i = 0; i < size; i++) {
            if (i > 0) sb.append(",");
            sb.append(((ConstSFColor) __vect.elementAt(i)).__toPerl());
        }
        return sb.append("").toString();
    }
}