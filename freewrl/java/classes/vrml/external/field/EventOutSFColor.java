package vrml.external.field;
import java.util.*;
import vrml.external.Browser;
import vrml.external.field.FieldTypes;


public class EventOutSFColor extends EventOut {
  public EventOutSFColor() {EventType = FieldTypes.SFCOLOR;}

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
