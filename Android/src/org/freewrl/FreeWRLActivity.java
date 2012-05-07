package org.freewrl;


import android.app.Activity;
import android.os.Bundle;
import android.util.Log;
import android.view.ViewGroup;
import android.content.Context;
import java.util.List;
import android.content.IntentFilter;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.net.wifi.WifiConfiguration;

import android.view.Menu;
import android.view.MenuItem;

public class FreeWRLActivity extends Activity {

    FreeWRLView mView;
    private static String TAG = "FreeWRLView";
	WifiManager wifi;


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

//	// Create buttons - Hackfest time
//	LinearLayout ll = new LinearLayout(this);
//
//	Button daMiss = new Button(this);
//	daMiss.setText("Dismiss");
//	ll.addView(daMiss);
//	//ll.setGravity(Gravity.CENTER_HORIZONTAL | Gravity.CENTER_VERTICAL);
//	ll.setGravity(Gravity.BOTTOM | Gravity.LEFT);
//	this.addContentView(ll, new LayoutParams(LayoutParams.FILL_PARENT, LayoutParams.FILL_PARENT));
//
//	// set listener for it
//	daMiss.setOnTouchListener(new OnTouchListener() {
//
//            @Override
//            public boolean onTouch(View v, MotionEvent event) {
//		Log.w(TAG,"quit button onTouched");
//
//        	final int action = event.getAction();
//		Log.w(TAG,"action "+ action + "masked action "+ MotionEvent.ACTION_MASK);
//		if (action == MotionEvent.ACTION_DOWN) {
//			finish();
//		}
//		return true;
//		}
//        });
//
//	Button nextVP = new Button(this);
//	nextVP.setText("ViewPoint");
//	ll.addView(nextVP);
//
//	// set listener for it
//	nextVP.setOnTouchListener(new OnTouchListener() {
//
//            @Override
//            public boolean onTouch(View v, MotionEvent event) {
//		Log.w(TAG,"quit button onTouched");
//
//        	final int action = event.getAction();
//		if (action == MotionEvent.ACTION_DOWN) FreeWRLLib.nextViewpoint();
//		return true;
//            }
//        });
//
//	// finish Create buttons - Hackfest time
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
