// Interface for Browser.java
//

package vrml.external;

import vrml.external.field.EventOutObserver;

public interface BrowserInterface {

public int get_Browser_EVtype (int event);
public EventOutObserver get_Browser_EVObserver (int eventno);
public boolean get_Browser_EV_short_reply (int event);
public void Browser_RL_Async_send(String EVentreply, int eventno);


}

