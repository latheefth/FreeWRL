/*
  $Id$

*/

/****************************************************************************
    This file is part of the FreeWRL/FreeX3D Distribution.

    Copyright 2012 CRC Canada. (http://www.crc.gc.ca)

    FreeWRL/FreeX3D is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    FreeWRL/FreeX3D is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with FreeWRL/FreeX3D.  If not, see <http://www.gnu.org/licenses/>.
****************************************************************************/

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
		String myName  = wantedName[0];

		if (Looper.myLooper () == null) {
			//Log.w(TAG,"FreeWRLAssetGetter, no looper yet");
			Looper.prepare();
		}
 

		myAsset = new FreeWRLAssets();

		Log.w(TAG,"now, RESOURCE Wanted name is " + myName);

		myAssetSize = myAsset.openAsset(myContext,myName);
		Log.w(TAG,"-------------myAssetSize offset is " + myAssetSize.offset);
		Log.w(TAG,"-------------myAssetSize size is " + myAssetSize.length);
		Log.w(TAG,"-------------myAssetSize fd is " + myAssetSize.fd);

		// send this to FreeWRL
		FileDescriptor fd = myAssetSize.fd;
		int off = (int) myAssetSize.offset;
		int len = (int) myAssetSize.length;
		int res = FreeWRLLib.resourceFile(fd, off, len);
		Log.w(TAG,"-------------and, the getStartOffset, getLength is " + off + "  " + len);
		Log.w (TAG,"------------resourceFile NDK call returns " + res);

		return myName;
	}

	@Override
	protected void onPostExecute(String result) {
		Log.w(TAG, "FreeWRLAssetGetter onPostExecute done - string "+ result);
		FreeWRLActivity.currentlyGettingResource=false;

	}
}

