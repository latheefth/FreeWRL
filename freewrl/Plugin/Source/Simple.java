/*******************************************************************************
 * Simple LiveConnect Sample Plugin
 * Copyright (c) 1996 Netscape Communications. All rights reserved.
 ******************************************************************************/

import netscape.plugin.Plugin;

class Simple extends Plugin {

    /*
    ** A plug-in can consist of code written in java as well as
    ** natively. Here's a dummy method.
    */
    public static int fact(int n) {
	if (n == 1)
	    return 1;
	else
	    return n * fact(n-1);
    }

    /*
    ** This instance variable is used to keep track of the number of
    ** times we've called into this plug-in.
    */
    int count;

    /*
    ** This native method will give us a way to print to stdout from java
    ** instead of just the java console.
    */
    native void printToStdout(String msg);

    /*
    ** This is a publically callable new feature that our plug-in is
    ** providing. We can call it from JavaScript, Java, or from native
    ** code.
    */
    public void doit(String text) {
	/* construct a message */
	String msg = "" + (++count) + ". " + text + "\n";
	/* write it to the console */
	System.out.print(msg);
	/* and also write it to stdout */
	printToStdout(msg);
    }
}
