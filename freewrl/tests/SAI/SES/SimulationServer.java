import java.io.*;
import java.net.*;

class SimulationServer {
    private static final int sleepTime = 100;

    private static final int animPort = 4445;

    private long startTime;

    private AnimationServerThread fromSim, fromAnim;
  
    private ServerSocket animServerSocket = null;
    private Socket animClientSocket = null;

    private DataInputStream animIS = null;
    private PrintStream animOS = null;

    private static final String sigmsg[] = {
	"Stop", "StopDemand", "Go"
    };
    private static final int IDLE  = 0;
    private static final int GO1   = 1;
    private static final int OUT1  = 2;
    private static final int GO2   = 3;
    private static final int OUT2  = 4;

    boolean[][][] sensors = new boolean[2][3][2];
    boolean[][] sensState = new boolean[2][3];

    int state = IDLE;
    int[][] counters = new int[2][2];
    boolean error;

    public void run() {
	try {
	    animServerSocket = new ServerSocket(animPort);
	    System.out.println ("Server OK! ...waiting for animation client...");

	    animClientSocket = animServerSocket.accept();
	    animIS = new DataInputStream
		(new BufferedInputStream
		    (animClientSocket.getInputStream()));
	    animOS = new PrintStream
		(new BufferedOutputStream
		    (animClientSocket.getOutputStream()));
	} catch (IOException ex) {
	    ex.printStackTrace();
	}
	System.out.println ("Client OK! ...reading from connections...");

	startTime = System.currentTimeMillis();
	int lastSignals = 0;
	sendMessage("time 0");
	    try {
	Thread.sleep(1000);
	    } catch (InterruptedException ex) {
	    }

	while (true) {
	    try {
		Thread.sleep(sleepTime);
	    } catch (InterruptedException ex) {
	    }
	    handleMessages();
	    long time = (System.currentTimeMillis() - startTime) / 10;
	    updateCounters(time);

	    int signals = calcSignals();
	    if (signals != lastSignals) {
		if ((signals & 3) != (lastSignals & 3)) {
		    sendMessage("signal 1 "+sigmsg[signals&3]+" "+time);
		}
		if ((signals & 12) != (lastSignals & 12)) {
		    sendMessage("signal 2 "+sigmsg[(signals&12) >> 2]
				+" "+time);
		}
		lastSignals = signals;
	    } else {
		sendMessage("time "+time);
	    }
	}
    }
    public void sendMessage(String message) {
	//System.err.println("SEND MESSAGE: "+message);
	MobyProtocol.printMobyMessage(animOS, message);
    }

    public int calcSignals() {
	if (error)
	    return 0;
	switch (state) {
	    case IDLE:
		if (counters[0][0] != 0)
		    state = GO1;
		else if (counters[1][0] != 0)
		    state = GO2;
		break;

	    case GO1:
		if (counters[0][0] == 0)
		    state = OUT1;
		break;
	    case GO2:
		if (counters[1][0] == 0)
		    state = OUT2;
		break;
		
	    case OUT1:
		if (counters[1][0] == 0 && counters[0][0] != 0)
		    state = GO1;
		else if (counters[0][1] == 0) {
		    if (counters[1][0] != 0)
			state = GO2;
		    else
			state = IDLE;
		}
		break;
	    case OUT2:
		if (counters[0][0] == 0 && counters[1][0] != 0)
		    state = GO2;
		else if (counters[1][1] == 0) {
		    if (counters[0][0] != 0)
			state = GO1;
		    else
			state = IDLE;
		}
		break;
	}
	return ((state == GO1 ? 2 : counters[0][0] != 0 ? 1 : 0)
		| (state == GO2 ? 8 : counters[1][0] != 0 ? 4 : 0));
    }

    public void updateCounters(long time) {
	for (int dir = 0; dir < 2; dir++) {
	    for (int pos = 0; pos < 3; pos++) {
		if (sensors[dir][pos][0] == sensors[dir][pos][1])
		    error = true;
		if (sensors[dir][pos][0] != sensState[dir][pos]) {
		    sensState[dir][pos] = sensors[dir][pos][0];
		    if (sensState[dir][pos]) {
			if (pos < 2)
			    counters[dir][pos]++;
			if (pos > 0) {
			    counters[dir][pos-1]--;
			    if (counters[dir][pos-1] < 0)
				error = true;
			}
		    }
		}
	    }
	}
    }
	


    public void handleMessages() {
	try {
	    while (animIS.available() > 3) {
		String input = MobyProtocol.readMobyMessage (animIS);
		// System.err.println("GOT MESSAGE: "+input);
		TokenScanner tokenScanner = new TokenScanner (input);
		String keyword = tokenScanner.nextToken ();
		if (keyword.equals("sensor")) {
		    int inverse = tokenScanner.nextToken().equals("CV") ? 1 : 0;
		    String sensdescr = tokenScanner.nextToken();
		    int number = sensdescr.equals("ES") ? 0
			: sensdescr.equals("CS") ? 1 : 2;
		    int direction = Integer.parseInt(tokenScanner.nextToken());
		    boolean value = tokenScanner.nextToken().equals("true");
		    sensors[direction-1][number][inverse] = value;
		}
	    }
	} catch (IOException e) { 
	    e.printStackTrace(); 
	}
    }	    


    public static void main(String[] args) {
      new SimulationServer().run();
    }

}
