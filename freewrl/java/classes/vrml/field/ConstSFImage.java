package vrml.field;
import vrml.*;

public class ConstSFImage extends ConstField {
    int width;
    int height;
    int components;
    byte[] pixels;

    public ConstSFImage() {} /* only for internal use */
    public ConstSFImage(int width, int height, int components, byte[] pixels) {
        this.width = width;
        this.height = height;
        this.components = components;
        this.pixels = pixels;
    }
    public int getWidth() {
        __updateRead();
        return width;
    }

    public int getHeight() {
        __updateRead();
        return height;
    }

    public int getComponents() {
        __updateRead();
        return components;
    }

    public byte[] getPixels() {
        __updateRead();
        return pixels;
    }


    public String toString() {
        StringBuffer sb = new StringBuffer();
        sb.append(width).append(' ').append(height).append(' ').append(components);
        for (int i = 0; i < pixels.length; i+=components) {
	    sb.append(" 0x");
	    for (int j = i; j < i+components; j++)
		sb.append("0123456789ABCDEF".charAt((pixels[i+j] & 0xf0) >> 4))
		    .append("0123456789ABCDEF".charAt(pixels[i+j] & 0x0f));
	}
        return sb.toString();
    }

    public void __fromPerl(String str) {
        /*XXX*/
    }

    public String __toPerl() {
        return toString();
    }
}