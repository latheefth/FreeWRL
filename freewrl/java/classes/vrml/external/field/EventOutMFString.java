package vrml.external.field;
import java.util.*;
import vrml.external.field.FieldTypes;
import vrml.external.Browser;


public class EventOutMFString extends EventOutMField {
	// retstr is an array of string values.
	// mySize is the size of retstr.

	String[] retstr;
	int mySize = 0;

	public EventOutMFString() {EventType = FieldTypes.MFSTRING;}

	public String[] getValue() {
		String rep;
		StringTokenizer tokens;

		if (command != null) {
			rep = Browser.SendEventOut (nodeptr, offset, datasize, datatype, command);
			//System.out.println("DEBUG - EventOutMFString returns " + rep);
			tokens = new StringTokenizer (rep,"\"");
		} else {
			tokens = new StringTokenizer (RLreturn,"\"");
		}

		retstr = new String[(tokens.countTokens()/2)];
		mySize = 0;

		rep = "";
		while (tokens.hasMoreTokens()) {
			retstr[mySize] = tokens.nextToken();

			if (retstr[mySize].equals("XyZZtitndi")) {
				//System.out.println ("found the gibberish line");
				retstr[mySize] = "";
			}

			if (tokens.hasMoreTokens()) rep = tokens.nextToken();
			mySize ++;
		}
		// for getSize call
		sizeof = mySize;

		return retstr;
	}

	public String get1Value(int index) {
		if ((index > mySize) || (index < 0)) {
			System.out.println ("EventOutMFString.get1Value - index " + index +
			" out of range");
			index = 0;
		}
		return retstr[index];
	}
}
