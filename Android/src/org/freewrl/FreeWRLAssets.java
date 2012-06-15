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

import android.app.Activity;
import android.os.Bundle;
import android.util.Log;
import android.view.WindowManager;

import java.io.IOException;

import java.io.FileDescriptor;
import java.io.FileInputStream;
import android.content.res.AssetManager;
import android.content.res.AssetFileDescriptor;
import java.io.FileDescriptor;
import android.content.res.Resources;

import android.content.Context;


import java.io.File;
import java.io.FileReader;
import java.io.FileNotFoundException;
import java.net.URLConnection; //file type guessing

public class FreeWRLAssets {

   private static String TAG = "FreeWRLView";


public FreeWRLAssetData openAsset(Context context, String path )
{
	Integer offset;
	Integer length;
	AssetFileDescriptor ad;
	FileDescriptor fd;
   try
   {
	Log.w(TAG,"---------------- GOING TO OPEN ASSET FILE ------------------");
	//Log.w(TAG," VRML guessing it is a " + URLConnection.guessContentTypeFromName("/sdcard/1.wrl"));
	//Log.w(TAG," VRML guessing it is a " + URLConnection.guessContentTypeFromName("/sdcard/1.x3d"));
	//Log.w(TAG," guessing it is a " + URLConnection.guessContentTypeFromName(path));

		// First, is this an ASSET file?

		String tryInAppAssets = "";

                // remove slash at the beginning, if it exists
                // as Android assets are not root based but getwd returns root base.
                if (path.indexOf('/') == 0)
                        tryInAppAssets = path.substring(1);
                else
                        tryInAppAssets = path;


	      ad = context.getResources().getAssets().openFd(tryInAppAssets);

		fd = ad.getFileDescriptor();

		if (ad == null) {Log.w(TAG,"FreeWRLAssets - NOT an asset file");} else {Log.w(TAG,"FreeWRLAssets - IS an asset file");}
		if (fd == null) {Log.w(TAG,"FreeWRLAssets FD - NOT an asset file");} else {Log.w(TAG,"FreeWRLAssets FD - IS an asset file");}


Log.w(TAG,"opened, ad " + ad + " fd " + fd);

	      Integer off = (int) ad.getStartOffset();
	      Integer len = (int) ad.getLength();
		FreeWRLAssetData rv = new FreeWRLAssetData(off,len,fd);
Log.w(TAG,"FreeWRLAssetData content is off " + rv.offset + " len " + rv.length + " fd " + rv.fd);
      		return rv;

   } catch( IOException e ) {
      Log.e( TAG, "openAsset: " + e.toString() );
   }



    Log.w(TAG,"openAsset: Obviously NOT in the applications asset area");
    FileInputStream in;
    try {
	in = new FileInputStream(path);
    } catch (FileNotFoundException e) {
        Log.e(TAG, "Couldn't find or open favorites file " + path);
        return new FreeWRLAssetData(0,0,null);
    }

	Log.w (TAG,"successfully opened " + path);
	try {
	fd = in.getFD();
	} catch (IOException e) {
		return new FreeWRLAssetData(-1,-1,null);
	}

	return new FreeWRLAssetData(0,0,fd);
}

}
