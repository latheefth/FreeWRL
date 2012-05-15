package org.freewrl;


import android.app.Activity;
import android.os.Bundle;
import android.util.Log;
import android.view.ViewGroup;
import android.content.Context;
import java.util.List;
import android.content.IntentFilter;

import android.view.Menu;
import android.view.MenuItem;


import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;

public class FreeWRLActivity extends Activity {

    FreeWRLView mView;
    private static String TAG = "FreeWRLView";


static final int NEW_WORLD= 0;
static final int VIEWPOINT_CHANGE= 1;
static final int DISMISS =2;


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
			/* Actions in case that Edid Contacts is pressed */
			Log.w(TAG,"NEW_WORLD");
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

    }

    @Override protected void onPause() {
        super.onPause();
        mView.onPause();
    }

    @Override protected void onResume() {
        super.onResume();
        mView.onResume();
    }
}
