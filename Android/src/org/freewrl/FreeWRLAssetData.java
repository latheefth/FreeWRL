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

/* Notes:

An Asset, in FreeWRL terms, is a resource (vrml file, jpg file, etc) that resides
*SOMEWHERE*.

It can reside within the FreeWRL apk (eg, font files reside there), or it can
reside on the SD card.

We ALWAYS look in the assets in the apk file first; if not there, then we go elsewhere.

*/


package org.freewrl;

//DJ import android.app.Activity;
//DJ import android.os.Bundle;
//DJ import android.view.WindowManager;
// 
import android.util.Log;
import android.content.Context;

import android.graphics.BitmapFactory;
import android.graphics.Bitmap;
import android.graphics.Bitmap.Config;
import java.io.File;
import java.io.FileDescriptor;
import java.io.InputStream;
import java.io.Reader;
import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.io.ByteArrayOutputStream ;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.BufferUnderflowException;

//DJ import android.content.res.Resources;
import android.content.res.AssetManager;
import android.content.res.AssetFileDescriptor;

public class FreeWRLAssetData {
	Integer offset;
	Integer length;
	FileDescriptor fd;
	byte[] myBytes;
	int imageWidth;
	int imageHeight;
	boolean hasAlpha;
	Integer knownType = 0;

	private static String TAG = "FreeWRLAssetData";

	// new constructor from Dave Joubert, 5 Aug 2011
	private String convertStreamToString(InputStream is) throws IOException {
        	if (is != null) {
                	StringBuilder sb = new StringBuilder();
                	String line;

			Log.w(TAG, "file " + new Throwable().getStackTrace()[0].getFileName() +
				" class " + new Throwable().getStackTrace()[0].getClassName() +
				" method " + new Throwable().getStackTrace()[0].getMethodName() +
				" line " + new Throwable().getStackTrace()[0].getLineNumber());
                	try {
                	        BufferedReader reader = new BufferedReader(
                	                        new InputStreamReader(is, "UTF-8"));
				Log.w(TAG, "file " + new Throwable().getStackTrace()[0].getFileName() +
					" class " + new Throwable().getStackTrace()[0].getClassName() +
					" method " + new Throwable().getStackTrace()[0].getMethodName() +
					" line " + new Throwable().getStackTrace()[0].getLineNumber());
                	        while ((line = reader.readLine()) != null) {
					Log.w(TAG, "line " + line +"\n") ;
                	                sb.append(line).append("\n");
                	        }
                	}
                	finally {
                	        is.close();
                	}
			Log.w(TAG, "file " + new Throwable().getStackTrace()[0].getFileName() +
				" class " + new Throwable().getStackTrace()[0].getClassName() +
				" method " + new Throwable().getStackTrace()[0].getMethodName() +
				" line " + new Throwable().getStackTrace()[0].getLineNumber());
                	return sb.toString();
        	} else {
			Log.w(TAG, "file " + new Throwable().getStackTrace()[0].getFileName() +
				" class " + new Throwable().getStackTrace()[0].getClassName() +
				" method " + new Throwable().getStackTrace()[0].getMethodName() +
				" line " + new Throwable().getStackTrace()[0].getLineNumber());
                	return "";
        	}
	}

        public FreeWRLAssetData (Context myAppContext, String assetName, Integer forceType) {
		Bitmap mybitmap = null;

                this.offset = 0;
		this.length = 0;
                this.fd = null;
                this.myBytes = null; 
                this.imageWidth = -1;
                this.imageHeight = -1;
                this.hasAlpha = false;
                this.knownType = forceType;

		Log.w(TAG, "file " + new Throwable().getStackTrace()[0].getFileName() +
			" class " + new Throwable().getStackTrace()[0].getClassName() +
			" method " + new Throwable().getStackTrace()[0].getMethodName() +
			" line " + new Throwable().getStackTrace()[0].getLineNumber());
		Log.w(TAG, "FROM : " + 
			" class " + new Throwable().getStackTrace()[1].getClassName() +
			" method " + new Throwable().getStackTrace()[1].getMethodName() +
			" line " + new Throwable().getStackTrace()[1].getLineNumber());

                AssetManager am = myAppContext.getAssets();
                if (forceType == 1) {
                        // String. Based on example code from APress Pro Android
                        try {
                                InputStream is = am.open(assetName) ;
				ByteArrayOutputStream bos = new ByteArrayOutputStream();

				Log.w(TAG, "file " + new Throwable().getStackTrace()[0].getFileName() +
					" class " + new Throwable().getStackTrace()[0].getClassName() +
					" method " + new Throwable().getStackTrace()[0].getMethodName() +
					" line " + new Throwable().getStackTrace()[0].getLineNumber());
				byte[] b = new byte[1024];
				int bytesRead ;
				while ( (bytesRead = is.read(b)) != -1) {
					bos.write(b, 0, bytesRead);
					length += bytesRead;
				}
				myBytes = bos.toByteArray();
				mybitmap = BitmapFactory.decodeByteArray(myBytes, 0, length);

				if (mybitmap != null) {
					Log.w(TAG, "file " + new Throwable().getStackTrace()[0].getFileName() +
						" class " + new Throwable().getStackTrace()[0].getClassName() +
						" method " + new Throwable().getStackTrace()[0].getMethodName() +
						" line " + new Throwable().getStackTrace()[0].getLineNumber());
					Log.w(TAG,"BITMAP!!! bitmap is a " + mybitmap.getConfig());
					// convert this bitmap into ARGB_8888 if it is not.
					if (mybitmap.getConfig() != Bitmap.Config.ARGB_8888) {
						Log.w(TAG, "BITMAP changing");
						mybitmap = mybitmap.copy(Bitmap.Config.ARGB_8888,false);
						Log.w(TAG,"BITMAP!!! bitmap is a " + mybitmap.getConfig());
					}
				
					// convert this to a char array for us to send to FreeWRL
					imageWidth = mybitmap.getWidth();
					imageHeight = mybitmap.getHeight();
					hasAlpha = mybitmap.hasAlpha();
					int sz = 4 * imageWidth * imageHeight;
					ByteBuffer bb = ByteBuffer.allocate(sz);
					mybitmap.copyPixelsToBuffer(bb);
		
					// convert the ByteBuffer to a byte[] array.
					myBytes = bb.array();
					length = sz ;
				}
			} catch (IOException e) {}
                }
        }

	public FreeWRLAssetData (Integer of, Integer len, FileDescriptor fd, InputStream myStream) {
		Bitmap mybitmap = null;
	
		this.offset = of;
		this.length = len;
		this.fd = fd;
		this.myBytes = null;
		this.imageWidth = -1;
		this.imageHeight = -1;
		this.hasAlpha = false;

				Log.w(TAG, "file " + new Throwable().getStackTrace()[0].getFileName() +
					" class " + new Throwable().getStackTrace()[0].getClassName() +
					" method " + new Throwable().getStackTrace()[0].getMethodName() +
					" line " + new Throwable().getStackTrace()[0].getLineNumber());

		// any hope of finding anything? if so, lets continue...
		if (myStream != null) {

			// if the InputStream is null, we had a failure, thus we can not have a bitmap.
			mybitmap = BitmapFactory.decodeStream(myStream);
			
			// do we have a valid input stream, that is NOT a bitmap?
			if (mybitmap == null) {
				Log.w(TAG,"Most likely a text file  - myStream" + myStream);
	
				
				try {
					InputStreamReader irs = new InputStreamReader(myStream);

				Log.w(TAG, "file " + new Throwable().getStackTrace()[0].getFileName() +
					" class " + new Throwable().getStackTrace()[0].getClassName() +
					" method " + new Throwable().getStackTrace()[0].getMethodName() +
					" line " + new Throwable().getStackTrace()[0].getLineNumber());
					//if (irs.ready()) Log.w(TAG,"InputStreamReader ready to go"); else Log.w(TAG,"InputStreamReader NOT ready");

					StringBuilder mycharstring = new StringBuilder();
					Reader in = new BufferedReader(irs);
	
					int ch;
					while ((ch = in.read()) > -1) {
						mycharstring.append((char)ch);
					}
					in.close();
					irs.close();
					Log.w(TAG,"Text actual length " + mycharstring.length() + " input length was " + len);
	
					// convert this to a char array for us to send to FreeWRL
					try {
						String mys = new String(mycharstring);
						myBytes = mys.getBytes();
					} catch (NullPointerException e) {
						//Log.w(TAG,"String ops" + e);
					}
				} catch (IOException e) {
					//Log.w(TAG,"io exception on read of text string: " + e);
					length = 0;
				}
			} else {

				//Log.w(TAG,"BITMAP!!! bitmap is a " + mybitmap.getConfig());
				// convert this bitmap into ARGB_8888 if it is not.
				if (mybitmap.getConfig() != Bitmap.Config.ARGB_8888) {
					//Log.w(TAG, "BITMAP changing");
					mybitmap = mybitmap.copy(Bitmap.Config.ARGB_8888,false);
					//Log.w(TAG,"BITMAP!!! bitmap is a " + mybitmap.getConfig());
				}
			
				// convert this to a char array for us to send to FreeWRL
				imageWidth = mybitmap.getWidth();
				imageHeight = mybitmap.getHeight();
				hasAlpha = mybitmap.hasAlpha();
				int sz = 4 * imageWidth * imageHeight;
				ByteBuffer bb = ByteBuffer.allocate(sz);
				mybitmap.copyPixelsToBuffer(bb);
	
				// convert the ByteBuffer to a byte[] array.
				myBytes = bb.array();
			}
	
			// we are done with this InputStream now.
			try {
				myStream.close();
			} catch (IOException e) {}

			//Log.w(TAG,"file read in, length " + myBytes.length);
		}
	}
}
