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
        static FreeWRLAssetData fontAssetDatum_01;


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

	//Directory click. Keep a copy of this directory.
	public void OnDirectoryClicked(File file) {
		//Log.w (TAG,"OnDirectoryClicked " + file);	
		lastDirectoryBrowsed = file.toString();
	}

	//File Click
	public void OnFileClicked(File file) {

		//Log.w(TAG,"OnFileClicked - file " + file);
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

// where do we look for files on the Android device?
private static String lastDirectoryBrowsed = null;

// get the last so many console messages. Message 0 is most recent, 1 is prev, 2, prev
// to that, etc. etc.

private String getLastConsoleMessages() {
	String retString = FreeWRLLib.androidGetLastMessage(0) + 
			"\n(prev):\n" + FreeWRLLib.androidGetLastMessage(1) +
			"\n(prev):\n" + FreeWRLLib.androidGetLastMessage(2);
	//Log.w(TAG,"getLastConsoleMessages returns: " + retString);
	return retString;
}

public boolean onCreateOptionsMenu(Menu menu){

	//Log.w(TAG,"onCreateOptionsMenu");
	menu.add(0,NEW_WORLD,0,"New");
	menu.add(0,VIEWPOINT_CHANGE,0,"Viewpoint");
	menu.add(0,LOG_LOOK,0,"Info");
	//menu.add(0,DISMISS,0,"Dismiss");
	return super.onCreateOptionsMenu(menu);
	//return true;
}

// throw the console on the screen - either the user wanted it, or we have an error
private void displayConsole() {
	Context origContext = getApplication();

	// remove an older one, if it exists.
	if (myConsole != null) myConsole.setVisibility(View.GONE);
	myConsole = new ConsoleLayout(getApplication(),null);

	myConsole.setConsoleListing(FreeWRLVersion.version,FreeWRLVersion.compileDate,getLastConsoleMessages());

	// set the background colour - let FreeWRL show through sometimes.
	myConsole.setBackgroundColor(0xAF000000 );

	// display it
	getWindow().addContentView(myConsole, new ViewGroup.LayoutParams(ViewGroup.LayoutParams.FILL_PARENT, ViewGroup.LayoutParams.FILL_PARENT));
}

// user hit the menu button - display our main selections.
public boolean onOptionsItemSelected (MenuItem item){
	//Log.w(TAG,"onOptionsItemSelected");
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
			if (lastDirectoryBrowsed == null) 
				lastDirectoryBrowsed = Environment.getExternalStorageDirectory().getPath();

			//localFolders.setDir(Environment.getExternalStorageDirectory().getPath());
			localFolders.setDir(lastDirectoryBrowsed);

			// set the background colour - let FreeWRL show through sometimes.
			localFolders.setBackgroundColor(0xAF000000 );

			// display it
			getWindow().addContentView(localFolders, new ViewGroup.LayoutParams(ViewGroup.LayoutParams.FILL_PARENT, ViewGroup.LayoutParams.FILL_PARENT));

			break;
		}
	
		case VIEWPOINT_CHANGE : {
			//Log.w(TAG,"VIEWPOINT_CHANGE");
			/* Actions in case that Delete Contact is pressed */
			FreeWRLLib.nextViewpoint();
			break;
		}

		case LOG_LOOK : {
			displayConsole();
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
	//Log.w(TAG,"onCreate");
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
		//Log.w(TAG,"creating font assets");
		fontAsset_01 = new FreeWRLAssets();
	}

	// send in the font file descriptor on create.
	fontAssetDatum_01 = fontAsset_01.openAsset(getApplicationContext(),"fonts/Vera.ttf.mp3");
	int res = FreeWRLLib.sendFontFile(01,fontAssetDatum_01.fd,
		(int) fontAssetDatum_01.offset, fontAssetDatum_01.length);
	
	//Log.w(TAG,"---- assets for Vera.ttf; " + fontAssetDatum_01.length);

	// send in the temp file, used by FreeWRL for creating tmp files, what else?
	FreeWRLLib.setTmpDir(getApplicationContext().getCacheDir().getAbsolutePath());


	mView.setLoadNewX3DFile();

	//Log.w(TAG,"starting timer task");
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
	//Log.w (TAG,"onRestart");
        super.onRestart();
    }

    @Override protected void onStop() {
	//Log.w (TAG,"onStop");
        super.onStop();
    }

    @Override protected void onStart() {
	//Log.w (TAG,"onStart");
        super.onStart();
    }

    @Override protected void onDestroy() {
	//Log.w (TAG,"onDestroy");
        super.onDestroy();
	FreeWRLLib.doQuitInstance();
	//Log.w (TAG,"FreeWRL onDestroyed");
    }

    @Override protected void onPause() {
	//Log.w (TAG,"onPause");
        super.onPause();
        mView.onPause();
    }

    @Override protected void onResume() {
	//Log.w (TAG,"onResume");
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

			// Do we have any console messages not shown? 
			if (FreeWRLLib.androidGetUnreadMessageCount() > 0) {
				displayConsole();
			}

		}
	};
}
