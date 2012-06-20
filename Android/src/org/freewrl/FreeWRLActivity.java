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
import android.view.ViewGroup;
import java.util.List;
import android.content.IntentFilter;
import android.content.Context;

import android.view.Menu;
import android.view.MenuItem;

import java.util.Timer;
import java.util.TimerTask;

import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;

// File Dialog 2
import java.io.File;
import android.content.DialogInterface;
import android.app.AlertDialog;
import android.widget.ListView;
import android.os.Environment;
// File Dialog 2

import android.os.Looper;

// logcat stuff
import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
// end logcat stuff

// for removing local file and console Views
import android.view.View;

public class FreeWRLActivity extends Activity implements IFolderItemListener {
	FreeWRLView mView;
	ViewGroup overViewGroup;

	private static String TAG = "FreeWRLActivity";

	// are we currently getting a resource? if so, we just ignore 
	// front end request for a file, because the requests are synchronous.
	public static boolean currentlyGettingResource = false;

	static final int NEW_WORLD= 0;
	static final int VIEWPOINT_CHANGE= 1;
	static final int LOG_LOOK = 2;
	//static final int DISMISS = 3;

	// timer trials
	private static Timer myTimer = null;

        // Fonts
        static FreeWRLAssets fontAsset_01 = null;
        static FreeWRLAssetData fontAssetSize_01;


	// New File and Console View.
	FolderLayout localFolders = null;
	ConsoleLayout myConsole = null;
                        
	// Cannot open Folder
	public void OnCannotFileRead(File file) {
		new AlertDialog.Builder(this)
		.setIcon(R.drawable.icon)
		.setTitle(
		"[" + file.getName()
		+ "] folder can't be read!")
		.setPositiveButton("OK",
		new DialogInterface.OnClickListener() {

			public void onClick(DialogInterface dialog,
				int which) {

				}
			}).show();

	}


	//File Click
	public void OnFileClicked(File file) {

		Log.w(TAG,"OnFileClicked - file " + file);
Log.w(TAG,"trying to load a new file here no matter what user does");
mView.setPossibleNewFileName(""+file);

		new AlertDialog.Builder(this)
		.setIcon(R.drawable.icon)
		.setTitle("Load " + file.getName() + "?")
		.setPositiveButton("OK",
			new DialogInterface.OnClickListener() {
				public void onClick(DialogInterface dialog,
				int which) {
					mView.setLoadNewX3DFile();

					// remove the folder viewing View.
					localFolders.setVisibility(View.GONE);
				}
			})
		.setNegativeButton("NO",
			new DialogInterface.OnClickListener() {
				public void onClick(DialogInterface dialog,
				int which) {
				mView.discardPossibleNewFileName();
				}
			}).show();
	}


private final String[] LOGCAT_CMD = new String[] { "logcat", "" };
public static final String[] LOGCAT_CLEAR_CMD = new String[] { "logcat", "-c" };
private Process mLogcatProc = null;
private BufferedReader reader = null;
private final int BUFFER_SIZE = 1024;

private String readLog() throws IOException{
Log.w("Logger","started readLog");
		String returnLogLines = "";

                reader = new BufferedReader(new InputStreamReader(mLogcatProc.getInputStream()),  BUFFER_SIZE);

                String line;
                //while ( !Thread.currentThread().isInterrupted() && (line = reader.readLine())  != null )
                while ((line = reader.readLine())  != null )
                {
Log.w("Logger","read a line");
                        if(line.contains("FreeWRL")){
				returnLogLines = returnLogLines + "\n" + line;
                        }
                }
Log.w("Logger","returning");
		return returnLogLines;
    }

private String getXXX() {
	String retString = "";
 try{
                mLogcatProc = Runtime.getRuntime().exec(LOGCAT_CMD);
        }catch( IOException e ){
                Log.i("LogReader", "Logcat process failed. " + e.getMessage());
                return "ERROR";
        }

/* 
Log.w(TAG,"sleeping here");
try {Thread.sleep (5000);} catch (InterruptedException f) { }
Log.w(TAG,"finished sleeping here");
*/
        try{

                retString = readLog();

        }catch( IOException e ){
                Log.i("LogReader", "Logcat process error. " + e.getMessage());
        }finally{

                if (reader != null){
                        Log.i("LogReader", "reader.close()");
                try {
                        reader.close();
                }catch(IOException e) {}
                }
        }
	return retString;
}

public boolean onCreateOptionsMenu(Menu menu){

	Log.w(TAG,"onCreateOptionsMenu");
	menu.add(0,NEW_WORLD,0,"New");
	menu.add(0,VIEWPOINT_CHANGE,0,"Viewpoint");
	menu.add(0,LOG_LOOK,0,"Info");
	//menu.add(0,DISMISS,0,"Dismiss");
	return true;
}
public boolean onOptionsItemSelected (MenuItem item){
	Log.w(TAG,"onOptionsItemSelected");
	switch (item.getItemId()){
	
		case NEW_WORLD: {
			Context origContext = getApplication();

			/* Actions in case that Edid Contacts is pressed */
			//Log.w(TAG,"NEW_WORLD");
			// File Dialog 2
			if (localFolders != null) localFolders.setVisibility(View.GONE);
			localFolders = new FolderLayout(getApplication(),null);

			//Log.w(TAG, "2 going to findViewById");
			localFolders.setIFolderItemListener(this);

			//Log.w(TAG, "3 going to findViewById");
			localFolders.setDir(Environment.getExternalStorageDirectory().getPath());

			// set the background colour - let FreeWRL show through sometimes.
			localFolders.setBackgroundColor(0xAF000000 );

			// display it
			getWindow().addContentView(localFolders, new ViewGroup.LayoutParams(ViewGroup.LayoutParams.FILL_PARENT, ViewGroup.LayoutParams.FILL_PARENT));

			break;
		}
	
		case VIEWPOINT_CHANGE : {
			Log.w(TAG,"VIEWPOINT_CHANGE");
			/* Actions in case that Delete Contact is pressed */
			FreeWRLLib.nextViewpoint();
			break;
		}

		case LOG_LOOK : {
			Context origContext = getApplication();

			/* Actions in case that Edid Contacts is pressed */
			//Log.w(TAG,"LOG_LOOK");
			// File Dialog 2
			// remove an older one, if it exists.
			if (myConsole != null) myConsole.setVisibility(View.GONE);
			myConsole = new ConsoleLayout(getApplication(),null);

			Log.w(TAG, "3 going to findViewById");
			myConsole.setConsoleListing(FreeWRLVersion.version,FreeWRLVersion.compileDate,getXXX());

			// set the background colour - let FreeWRL show through sometimes.
			myConsole.setBackgroundColor(0xAF000000 );

			// display it
			getWindow().addContentView(myConsole, new ViewGroup.LayoutParams(ViewGroup.LayoutParams.FILL_PARENT, ViewGroup.LayoutParams.FILL_PARENT));

			break;

		}
	
		//case DISMISS: {
		//	Log.w (TAG,"DISMISS");
		//	finish();
		//	break;
		//}

	}

	return true;

}


    @Override protected void onCreate(Bundle icicle) {
	Log.w(TAG,"onCreate");
        super.onCreate(icicle);
        mView = new FreeWRLView(getApplication());

	// tell the library to (re)create it's internal databases
	FreeWRLLib.createInstance();

	// for gestures
	mView.setLayoutParams(new ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,
		ViewGroup.LayoutParams.MATCH_PARENT));

	setContentView(mView);

	// send in font directory pointers.
	if (fontAsset_01 == null) {
		Log.w(TAG,"creating font assets");
		fontAsset_01 = new FreeWRLAssets();
	}

	// send in the font file descriptor on create.
	fontAssetSize_01 = fontAsset_01.openAsset(getApplicationContext(),"fonts/Vera.ttf.mp3");
	int res = FreeWRLLib.sendFontFile(01,fontAssetSize_01.fd,
		(int) fontAssetSize_01.offset,
		(int) fontAssetSize_01.length);
	
	Log.w(TAG,"---- assets for Vera.ttf; " + fontAssetSize_01.length);


	mView.setLoadNewX3DFile();

	Log.w(TAG, "onCreate - lets do some lookin");


	String apkFilePath = null;
	ApplicationInfo appInfo = null;
	PackageManager packMgmr = this.getPackageManager();
	try {
	        appInfo = packMgmr.getApplicationInfo("org.freewrl", 0);
	} catch (NameNotFoundException e) {
 		e.printStackTrace();
		throw new RuntimeException("Unable to locate assets, aborting...");
	}
	apkFilePath = appInfo.sourceDir;

	Log.w(TAG,"++++++++++++++++++++Activity:  apkFilePath is " + apkFilePath);

	Log.w(TAG,"starting timer task");
	myTimer = new Timer();
	myTimer.schedule(new TimerTask() {
		@Override
		public void run() {
			TimerMethod();
		}

	// do it 30 times per second (1/30)
	}, 0, 33);
}



    @Override protected void onRestart() {
	Log.w (TAG,"onRestart");
        super.onRestart();
    }

    @Override protected void onStop() {
	Log.w (TAG,"onStop");
        super.onStop();
    }

    @Override protected void onStart() {
	Log.w (TAG,"onStart");
        super.onStart();
    }

    @Override protected void onDestroy() {
	Log.w (TAG,"onDestroy");
        super.onDestroy();
	FreeWRLLib.doQuitInstance();
	Log.w (TAG,"FreeWRL onDestroyed");
    }

    @Override protected void onPause() {
	Log.w (TAG,"onPause");
        super.onPause();
        mView.onPause();
    }

    @Override protected void onResume() {
	Log.w (TAG,"onResume");
        super.onResume();
        mView.onResume();
    }




	// timer stuff 
	private void TimerMethod()
	{
		//This method is called directly by the timer
		//and runs in the same thread as the timer.

		//We call the method that will work with the UI
		//through the runOnUiThread method.
		this.runOnUiThread(Timer_Tick);
	}

	private Runnable Timer_Tick = new Runnable() {
		public void run() {

			// This method runs in the same thread as the UI.       
			// it checks if a resource is requested by the back end, and if one is,
			// it checks to see that we have not already started a thread to 
			// satisfy the request. The back end will send only work on one file
			// at a time, so in essence, we just need to keep track of this
			// synchronous stream and thus we'll be getting only one resource at
			// a time, too.

			// Log.w(TAG,"timer tick");
			// do we want a new resource? are we currently NOT getting a resource??
			if (FreeWRLLib.resourceWanted()&& (!currentlyGettingResource)) {
				// we are getting a resource...
				currentlyGettingResource = true;

				FreeWRLAssetGetter task = new FreeWRLAssetGetter();
				task.sendInContext(getApplication());

				// execute the task, and then the ending of the task will 
				// set the currentlyGettingResource to false.
				task.execute (new String (FreeWRLLib.resourceNameWanted()));
			}

		}
	};
}
