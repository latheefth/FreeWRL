//AUTOMATICALLY GENERATED BY genfields.pl.
//DO NOT EDIT!!!!

package vrml.field;
import vrml.*;
import java.io.BufferedReader;
import java.io.PrintWriter;
import java.io.IOException;

public class MFRotation extends MField {
    public MFRotation() {
    }

    public MFRotation(float[] rotations) {
        this(rotations.length, rotations);
    }

    public MFRotation(int size, float[] rotations) {
        for (int i = 0; i < size; i += 4)	
            __vect.addElement(new ConstSFRotation(rotations[i], rotations[i+1], rotations[i+2], rotations[i+3]));
    }

    public MFRotation(float[][] rotations) {
        for (int i = 0; i < rotations.length; i++)
            __vect.addElement(new ConstSFRotation(rotations[i][0], rotations[i][1], rotations[i][2], rotations[i][3]));
    }
	
    public void getValue(float[] rotations) {
        __updateRead();
        int size = __vect.size();
        for (int i = 0; i < size; i++) {
            ConstSFRotation sfRotation = (ConstSFRotation) __vect.elementAt(i);
            rotations[4*i+0] = sfRotation.axisX;
            rotations[4*i+1] = sfRotation.axisY;
            rotations[4*i+2] = sfRotation.axisZ;
            rotations[4*i+3] = sfRotation.angle;
        }
    }

    public void getValue(float[][] rotations) {
        __updateRead();
        int size = __vect.size();
        for (int i = 0; i < size; i++)
            ((ConstSFRotation) __vect.elementAt(i)).getValue(rotations[i]);
    }

    public void get1Value(int index, float[] rotations) {
        __update1Read(index);
        ((ConstSFRotation) __vect.elementAt(index)).getValue(rotations);
    }

    public void get1Value(int index, SFRotation sfRotation) {
        __update1Read(index);
        sfRotation.setValue((ConstSFRotation) __vect.elementAt(index));
    }

    public void setValue(float[] rotations) {
        setValue(rotations.length, rotations);
    }

    public void setValue(int size, float[] rotations) {
        __vect.clear();
        for (int i = 0; i < size; i += 4)
            __vect.addElement(new ConstSFRotation(rotations[i], rotations[i+1], rotations[i+2], rotations[i+3]));
        __updateWrite();
    }

    public void set1Value(int index, float axisX, float axisY, float axisZ, float angle) {
        __set1Value(index, new ConstSFRotation(axisX, axisY, axisZ, angle));
    }

    public void set1Value(int index, SFRotation sfRotation) {
        sfRotation.__updateRead();
        __set1Value(index, new ConstSFRotation(sfRotation.axisX, sfRotation.axisY, sfRotation.axisZ, sfRotation.angle));
    }

    public void set1Value(int index, ConstSFRotation sfRotation) {
        __set1Value(index, sfRotation);
    }

    public void addValue(float axisX, float axisY, float axisZ, float angle) {
        __addValue(new ConstSFRotation(axisX, axisY, axisZ, angle));
    }

    public void addValue(SFRotation sfRotation) {
        sfRotation.__updateRead();
        __addValue(new ConstSFRotation(sfRotation.axisX, sfRotation.axisY, sfRotation.axisZ, sfRotation.angle));
    }

    public void addValue(ConstSFRotation sfRotation) {
        __addValue(sfRotation);
    }

    public void insertValue(int index, float axisX, float axisY, float axisZ, float angle) {
        __insertValue(index, new ConstSFRotation(axisX, axisY, axisZ, angle));
    }

    public void insertValue(int index, SFRotation sfRotation) {
        sfRotation.__updateRead();
        __insertValue(index, new ConstSFRotation(sfRotation.axisX, sfRotation.axisY, sfRotation.axisZ, sfRotation.angle));
    }

    public void insertValue(int index, ConstSFRotation sfRotation) {
        __insertValue(index, sfRotation);
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
            ConstSFRotation sf = new ConstSFRotation();
            sf.__fromPerl(in);
            __vect.addElement(sf);
        }
    }

    public void __toPerl(PrintWriter out)  throws IOException {
        StringBuffer sb = new StringBuffer("");
        int size = __vect.size();
	out.print(size);
        for (int i = 0; i < size; i++)
            ((ConstSFRotation) __vect.elementAt(i)).__toPerl(out);
    }
    //public void setOffset(String offs) { this.offset = offs; } //JAS2
    //public String getOffset() { return this.offset; } //JAS2
}