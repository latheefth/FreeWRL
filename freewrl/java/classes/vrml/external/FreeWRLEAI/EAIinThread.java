package vrml.external.FreeWRLEAI;

import vrml.external.*;

import java.util.*;
import java.applet.*;
import java.awt.*;
import java.net.*;
import java.io.*;


  // The Thread that reads input from the FreeWRL browser...
public  class EAIinThread implements Runnable {
  //class EAIinThread extends Thread {
  
    DataInputStream	EAIin;
    Socket		sock;
    Applet		FreeWLRSceneInterface;

    boolean debug = false;
  
    // The following are used to send from the event thread to the
    // browser thread. The event thread gets stuff from the EAI port
    // from the FreeWRL Browser, and sends Replies back to the
    // browser thread.
  
    public static PipedOutputStream EAItoBrowserStream = new PipedOutputStream();
    private PrintStream EAItoBrowserPrintStream = new PrintStream(EAItoBrowserStream);
  
    // Initialization - get the socket and the FreeWLRSceneInterfaces thread
    public EAIinThread (Socket s, Applet d) {
      sock = s;  
      FreeWLRSceneInterface=d;
    }
     
    public void run() {
      // Open the socket, and wait for the first reply....
  
      String 	reply;
      String	EVentno;
      String	EVentreply;
      int	EVcounter;
      String	REreply;
      String	Stemp;
  
      try {
        EAIin = new DataInputStream( sock.getInputStream());
        // wait for FreeWRL to send us the opening sequence...
        EAItoBrowserPrintStream.println (EAIin.readLine());
      } catch (IOException e) {
        System.out.print ("error reiniting data input stream");
      }
  
      // Now, this is the loop that loops to end all loops....
  
      try {
        // wait for FreeWRL to send us the correct number of lines...
        // rep 1, 2, 3 should be "RE" "2" "0" , with maybe another 
        // parameter at the end.
        // EVs are events, and have two following lines.
  
        reply = EAIin.readLine();

        while (true)  {
          // Loop here, processing incoming events

    	  if (reply.equals("EV")) {
            EVentno = EAIin.readLine();
	    if (debug) System.out.println ("EAIinThread 3 reply is " + EVentno);
 
	    // Is the Event expecting one line, or MORE??? 
    	    int temp = Integer.parseInt(EVentno);
    	    for (EVcounter=0; EVcounter<BrowserGlobals.EVno; EVcounter++) {
    	      if (BrowserGlobals.EVarray[EVcounter] == temp) {
    	        break;
    	      }
    	    }
	    if (debug)
		System.out.println ("registered at " + EVcounter +
			" type " + BrowserGlobals.EVtype[EVcounter]);

            EVentreply = EAIin.readLine();
            if (debug) System.out.println ("EAIinThread 4 reply is " + EVentreply);
   
	    // Is this an event with only one line for a reply?	
	    // if so, then, continue (waits can last for hours...)
	    if (BrowserGlobals.EVshortreply[EVcounter]) { 
              if (debug) System.out.println ("EAIinThread short reply is " + EVentreply);
    	      BrowserGlobals.RL_Async.send(EVentreply,EVcounter);
              reply = EAIin.readLine();
	    } else {

              reply = EAIin.readLine();
              if (debug) System.out.println ("EAIinThread 5 reply is " + reply);

              // Now, is this a multi-line reply???
              while ( (!reply.equals("RE")) && (!reply.equals("EV"))) {
                reply = EAIin.readLine();
                if ((!reply.equals("RE")) && (!reply.equals("EV"))) {
                  EVentreply = EVentreply + reply;
                }
              }

              if (debug) System.out.println ("EAIinThread 5.5; EVentno: " + 
	  	EVentno + "  EventReply " + EVentreply + " reply " + reply);

    	      BrowserGlobals.RL_Async.send(EVentreply,EVcounter);
	    }
    
          } else if (reply.equals("RE")) {
    
            // This is the integer reply to the command... number...
            EAItoBrowserPrintStream.println(EAIin.readLine());
    
            // ... and the boolean value of success or fail...
            REreply = EAIin.readLine();
            if (debug) System.out.println ("EAIinThread 6 reply is " + REreply);
  
            // Is this a 3 or 4 line reply??? If the following
            // returns an "EV" or "RE", then we have gone far enough...
            reply = EAIin.readLine();
	    if (debug) System.out.println ("EAIinThread 7 reply is " + reply);
  
            if (reply.equals("RE") || reply.equals("EV")) {
              // send the previous line down the pipe...
              EAItoBrowserPrintStream.println(REreply); 
              EAItoBrowserPrintStream.flush();
            } else {
              // send the current line down the pipe, and read in the next line...
              EAItoBrowserPrintStream.println(reply); 
              EAItoBrowserPrintStream.flush();
              reply = EAIin.readLine();
              if (debug) System.out.println ("EAIinThread 8 reply is " + reply);
            } 
          } else {
    	    System.out.println ("expecting REor EV, got " + reply);
            reply = EAIin.readLine();
            if (debug) System.out.println ("EAIinThread 9 reply is " + reply);
          }
        }
  
      } catch (IOException e) {
          System.out.print ("error reiniting data input stream\n");
      }
    }
}
