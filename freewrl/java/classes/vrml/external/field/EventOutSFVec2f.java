package vrml.external.field;
import java.util.*;
import vrml.external.field.FieldTypes;
import vrml.external.Browser;

public class EventOutSFVec2f extends EventOut {
  public EventOutSFVec2f() {EventType = FieldTypes.SFVEC2F;}

public float[]       getValue() {

    float[] fvals = new float[2];
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
    
   return fvals;
  }
}
