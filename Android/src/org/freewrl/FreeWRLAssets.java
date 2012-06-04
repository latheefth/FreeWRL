package org.freewrl;

import android.app.Activity;
import android.os.Bundle;
import android.util.Log;
import android.view.WindowManager;

import java.io.IOException;

import android.content.res.AssetManager;
import android.content.res.AssetFileDescriptor;
import android.content.res.Resources;

import android.content.Context;


import java.io.File;
import java.net.URLConnection; //file type guessing

public class FreeWRLAssets {

   private static String TAG = "FreeWRLView";


public FreeWRLAssetData openAsset(Context context, String path )
{
	Integer offset;
	Integer length;
	AssetFileDescriptor ad;
   try
   {
	Log.w(TAG,"---------------- GOING TO OPEN ASSET FILE ------------------");
	//Log.w(TAG," VRML guessing it is a " + URLConnection.guessContentTypeFromName("/sdcard/1.wrl"));
	//Log.w(TAG," VRML guessing it is a " + URLConnection.guessContentTypeFromName("/sdcard/1.x3d"));
	//Log.w(TAG," guessing it is a " + URLConnection.guessContentTypeFromName(path));
	      ad = context.getResources().getAssets().openFd( path );
	      Integer off = (int) ad.getStartOffset();
	      Integer len = (int) ad.getLength();
		FreeWRLAssetData rv = new FreeWRLAssetData(off,len,ad);
      		return rv;
   } catch( IOException e ) {
      Log.e( TAG, "openAsset: " + e.toString() );
   }
	return new FreeWRLAssetData(0,0,null);
}

}
