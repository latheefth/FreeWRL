package vrml.external.field;
import java.util.*;
import vrml.external.field.FieldTypes;
import vrml.external.Browser;


public class EventOutSFInt32 extends EventOut {
  public EventOutSFInt32() {EventType = FieldTypes.SFINT32;}

  public int           getValue() {
     String rep;
      rep = Browser.SendEventOut (nodeptr, offset, datasize, datatype, command);
     return Integer.valueOf(rep).intValue();
  }
}

