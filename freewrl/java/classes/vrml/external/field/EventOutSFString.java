package vrml.external.field;
import java.util.*;
import vrml.external.field.FieldTypes;
import vrml.external.Browser;


public class EventOutSFString extends EventOut {
  public EventOutSFString() {EventType = FieldTypes.SFSTRING;}

  public String        getValue() {

    if (command != null) {
      String rep;
      rep = Browser.SendEventOut (outNode, command);
      if (rep.length() > 2) {
        // remove quotes at the beginning and end
        rep = rep.substring (1,rep.length()-1);
      }
      return rep;
    } else {
      return RLreturn;
    }
  }
}
