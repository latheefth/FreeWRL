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



public class FreeWRLActivity extends Activity implements IFolderItemListener {
	FreeWRLView mView;
	ViewGroup overViewGroup;

	private static String TAG = "FreeWRLView";

	// are we currently getting a resource? if so, we just ignore 
	// front end request for a file, because the requests are synchronous.
	public static boolean currentlyGettingResource = false;

	static final int NEW_WORLD= 0;
	static final int VIEWPOINT_CHANGE= 1;
	static final int DISMISS =2;

	// timer trials
	private Timer myTimer;

        // Fonts
        static FreeWRLAssets fontAsset_01 = null;
        static FreeWRLAssetData fontAssetSize_01;
                        
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
		new AlertDialog.Builder(this)
		.setIcon(R.drawable.icon)
		.setTitle("[" + file.getName() + "]")
		.setPositiveButton("OK",
			new DialogInterface.OnClickListener() {
				public void onClick(DialogInterface dialog,
				int which) {
					Log.w(TAG,"Clicked file - listener " + which );


				}

			}).show();
	}



public boolean onCreateOptionsMenu(Menu menu){

	Log.w(TAG,"onCreateOptionsMenu");
	menu.add(0,NEW_WORLD,0,"New");
	menu.add(0,VIEWPOINT_CHANGE,0,"Viewpoint");
	menu.add(0,DISMISS,0,"Dismiss");
	return true;
}
public boolean onOptionsItemSelected (MenuItem item){
	Log.w(TAG,"onOptionsItemSelected");
	switch (item.getItemId()){
	
		case NEW_WORLD: {
			FolderLayout localFolders;
			Context origContext = getApplication();

			/* Actions in case that Edid Contacts is pressed */
			Log.w(TAG,"NEW_WORLD");
			// File Dialog 2
			localFolders = new FolderLayout(getApplication(),null);

			Log.w(TAG, "2 going to findViewById");
			localFolders.setIFolderItemListener(this);

			Log.w(TAG, "3 going to findViewById");
			localFolders.setDir(Environment.getExternalStorageDirectory().getPath());

			// display it
			setContentView(localFolders);

			break;
		}
	
		case VIEWPOINT_CHANGE : {
			Log.w(TAG,"VIEWPOINT_CHANGE");
			/* Actions in case that Delete Contact is pressed */
			FreeWRLLib.nextViewpoint();
			break;
		}
	
		case DISMISS: {
			Log.w (TAG,"DISMISS");
			finish();
			break;
		}

	}

	return true;

}


    @Override protected void onCreate(Bundle icicle) {
        super.onCreate(icicle);
        mView = new FreeWRLView(getApplication());

	// for gestures
	mView.setLayoutParams(new ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,
		ViewGroup.LayoutParams.MATCH_PARENT));

	setContentView(mView);

	// send in font directory pointers.
	if (fontAsset_01 == null) {
		Log.w(TAG,"creating font assets");
		fontAsset_01 = new FreeWRLAssets();
		fontAssetSize_01 = fontAsset_01.openAsset(getApplicationContext(),"fonts/Vera.ttf.mp3");
		int res = FreeWRLLib.sendFontFile(01,fontAssetSize_01.ad.getFileDescriptor(),
		(int) fontAssetSize_01.ad.getStartOffset(),
		(int) fontAssetSize_01.ad.getLength());
	}
	Log.w(TAG,"---- assets for Vera.ttf; " + fontAssetSize_01.ad.getLength());




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

    @Override protected void onPause() {
        super.onPause();
        mView.onPause();
    }

    @Override protected void onResume() {
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
