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

public class FreeWRLAssets {

   private static String TAG = "FreeWRLView";


public FreeWRLAssetData openAsset(Context context, String path )
{
	Integer offset;
	Integer length;
	AssetFileDescriptor ad;
   try
   {
	//Log.w(TAG,"---------------- GOING TO OPEN ASSET FILE ------------------");
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
