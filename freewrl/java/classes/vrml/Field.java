package vrml;
import java.lang.Cloneable;

public abstract class Field implements Cloneable
{
   FWJavaScriptBinding tb[] = {};
   public Field() {} // tb = new FWJavaScriptBinding[] {};}
   public abstract Object clone();

   public void bind_to(FWJavaScriptBinding b) {
	tb = new FWJavaScriptBinding[1]; tb[0] = b;
   }
   protected void value_touched() {
	for(int i=0; i<tb.length; i++) 
		tb[i].invoke();
   }
}


