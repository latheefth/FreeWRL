package vrml.external.field;
import vrml.external.field.FieldTypes;
import vrml.external.Browser;
import java.awt.*;

public class EventInSFImage extends EventIn {

  public EventInSFImage() { EventType = FieldTypes.SFIMAGE; }

  public void          setValue(int width, int height, int numComponents, byte[] pixels) throws IllegalArgumentException {
    int count;
	String value;
	Integer val;
	int counter;
	int pixIndex;
	Integer i,j,k,a;
	
	// are there enough pixels here? 
	if (pixels.length != (width * height * numComponents)) {
		throw new IllegalArgumentException("not enough components");
	}

	if ((numComponents < 0) || (numComponents > 4)) {
		throw new IllegalArgumentException("numComponents out of range");
	}


	// Treat this the same as an SFString
	value = "\"" + width + " " + height + " " + numComponents + " ";

	// Turn byte values into a String value
	pixIndex = 0;

//	for (count = 0; count < width * height; count++) {
//
//		if (numComponents == 1) {
//		}
//
//		if (numComponents == 2) {
//		}
//
//		if (numComponents == 3) {
//			i = intValue(pixels[pixIndex]); pixIndex++;	
//			j = Integer.parseInt(pixels[pixIndex]); pixIndex++;	
//			k = Integer.parseInt(pixels[pixIndex]); pixIndex++;	
//			val = i * 0xffff + j * 0xff + k;
//			value = value + Integer.toHexString(val) + " ";
//		}
//
//		if (numComponents == 4) {
//			i = pixels[pixIndex]; pixIndex++;	
//			j = pixels[pixIndex]; pixIndex++;	
//			k = pixels[pixIndex]; pixIndex++;	
//			a = pixels[pixIndex]; pixIndex++;	
//		}
//	}	

	Browser.newSendEvent (this, value + "\"");

	return;
  }
}
