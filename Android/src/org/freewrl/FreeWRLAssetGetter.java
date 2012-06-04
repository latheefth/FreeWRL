package org.freewrl;

import android.util.Log;
import java.io.FileDescriptor;
import android.content.Context;
import android.os.Looper;

import android.os.AsyncTask; //<Params, Progress, Result>;

public class FreeWRLAssetGetter extends AsyncTask<String, String, String> {

private static String TAG="FreeWRL";
private static Context myContext= null;
private static FreeWRLAssets myAsset = null;
FreeWRLAssetData myAssetSize;

	public void sendInContext(Context c) {
		myContext = c;
	}

	@Override
	protected String doInBackground(String... wantedName) {
		FreeWRLAssetData myAssetSize;

		// local munged copy of the requested file name.
		String myName ;

		Log.w(TAG,"AsyncTask, calling Looper.prepare here");	
		Looper.prepare();

		myAsset = new FreeWRLAssets();

		Log.w(TAG,"AsyncTask, doInBackground");
		Log.w(TAG,"AsyncTask, ignoring FONT ASSETS");

		// remove slash at the beginning, if it exists
		// as Android assets are not root based but getwd returns root base.
		if (wantedName[0].indexOf('/') == 0)
			myName = wantedName[0].substring(1);
		else
			myName = wantedName[0];

		Log.w(TAG,"now, RESOURCE Wanted name is " + wantedName);

		myAssetSize = myAsset.openAsset(myContext,myName);

		Log.w(TAG,"-------------myAssetSize offset is " + myAssetSize.offset);
		Log.w(TAG,"-------------myAssetSize size is " + myAssetSize.length);

		// send this to FreeWRL
		FileDescriptor fd = myAssetSize.ad.getFileDescriptor();
		int off = (int) myAssetSize.ad.getStartOffset();
		int len = (int) myAssetSize.ad.getLength();
		int res = FreeWRLLib.resourceFile(fd, off, len);
		//Log.w(TAG,"-------------and, the getStartOffset, getLength is " + off + "  " + len);
		//Log.w (TAG,"------------resourceFile NDK call returns " + res);

		//return new String(""); // dummy return value
		return myName;
	}

	@Override
	protected void onPostExecute(String result) {
		Log.w(TAG, "AsyncTask onPostExecute done - string "+ result);
		FreeWRLActivity.currentlyGettingResource=false;

	}
}

