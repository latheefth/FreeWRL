package vrml.external.field;
import java.util.*;
import vrml.external.field.FieldTypes;
import vrml.external.Browser;


public class EventOutMFString extends EventOutMField {

  // retstr is an array of string values.
  // sizeof is the size of retstr.

  String[] retstr;
  int sizeof = 0;


  public EventOutMFString() {EventType = FieldTypes.MFSTRING;}

  public String[]      getValue() {
    String rep;
    StringTokenizer tokens;

    // System.out.println ("DEBUG - EventOutMFString");
    if (command != null) {
      // System.out.println ("DEBUG - EventOutMFString - command not equals null " + command);
      rep = Browser.SendEventOut (outNode, command);
      tokens = new StringTokenizer (rep,"\"");
    } else {
      tokens = new StringTokenizer (RLreturn,"\"");
    }
    retstr = new String[(tokens.countTokens()/2)];
    sizeof = 0;

    rep = tokens.nextToken();
    while (tokens.hasMoreTokens()) {
      retstr[sizeof] = tokens.nextToken();
      if (tokens.hasMoreTokens()) rep = tokens.nextToken();
      sizeof ++;
    }
    //sizeof = sizeof-1;

    return retstr;
  }

  public String        get1Value(int index) {
    if ((index > sizeof) || (index < 0)) {
	System.out.println ("EventOutMFString.get1Value - index " + index +
		" out of range");
	index = 0;
    }
    return retstr[index];
  }

  public int           getSize() {
	return sizeof;
  }
}
