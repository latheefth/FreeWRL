package vrml.external.field;
import java.util.*;
import vrml.external.field.FieldTypes;
import vrml.external.Browser;


public class EventOutMFString extends EventOutMField {
  public EventOutMFString() {EventType = FieldTypes.MFSTRING;}

  public String[]      getValue() {
    String rep;
    int count;
    StringTokenizer tokens;

    if (command != null) {
      rep = Browser.SendEventOut (outNode, command);
      tokens = new StringTokenizer (rep,"\"");
    } else {
      tokens = new StringTokenizer (RLreturn,"\"");
    }
    String[] retstr = new String[(tokens.countTokens()/2)];
    count = 0;

    rep = tokens.nextToken();
    while (tokens.hasMoreTokens()) {
      retstr[count] = tokens.nextToken();
      if (tokens.hasMoreTokens()) rep = tokens.nextToken();
      count ++;
    }
    return retstr;
  }

  public String        get1Value(int index) {
    String[] retstr = getValue();
    System.out.println ("ERROR:MFString, get1value, whole string is " + retstr);
    return retstr[index];
  }
}
