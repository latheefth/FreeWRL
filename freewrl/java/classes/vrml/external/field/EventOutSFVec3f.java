package vrml.external.field;
import java.util.*;
import vrml.external.field.FieldTypes;
import vrml.external.Browser;

public class EventOutSFVec3f extends EventOut {
	public EventOutSFVec3f() {EventType = FieldTypes.SFVEC3F;}

	public float[]       getValue() {

		float[] fvals = new float[3];
		int count;
		String rep;
		StringTokenizer tokens;

		if (command != null) {
			rep = Browser.SendEventOut (nodeptr, offset, datasize, datatype, command);
			tokens = new StringTokenizer (rep);
		} else {
			tokens = new StringTokenizer (RLreturn);
		}

		fvals[0]=Float.valueOf(tokens.nextToken()).floatValue();
		fvals[1]=Float.valueOf(tokens.nextToken()).floatValue();
		fvals[2]=Float.valueOf(tokens.nextToken()).floatValue();
    
		return fvals;
	}
}
