package org.freewrl;

import java.io.FileDescriptor;
public class FreeWRLLib {

     static {
         System.loadLibrary("FreeWRL");
     }

     public static native void init(int width, int height);
     public static native void step();
     public static native void initialFile(String initFile);
	public static native boolean resourceWanted();
	public static native String resourceNameWanted();
	public static native void resourceData(String data);
	public static native int resourceFile(FileDescriptor ad, int offset, int length);

	public static native int sendFontFile(int which, FileDescriptor ad, int offset, int length);

	public static native void setButDown(int but, int state);
	public static native void setLastMouseEvent(int state);
	public static native void handleAqua(int but, int state, int x, int y);
	public static native void reloadAssets();
	public static native void nextViewpoint();
}
