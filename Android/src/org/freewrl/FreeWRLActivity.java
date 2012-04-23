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

// Hackfest
import android.widget.Button;
import android.widget.LinearLayout.LayoutParams;
import android.widget.LinearLayout;
import android.view.Gravity;
import android.view.View.OnTouchListener;
import android.view.View;
import android.view.MotionEvent;
// end Hackfest

public class FreeWRLActivity extends Activity {

    FreeWRLView mView;
    private static String TAG = "FreeWRLView";
	WifiManager wifi;

    @Override protected void onCreate(Bundle icicle) {
        super.onCreate(icicle);
        mView = new FreeWRLView(getApplication());

	// for gestures
	mView.setLayoutParams(new ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,
		ViewGroup.LayoutParams.MATCH_PARENT));

	setContentView(mView);

	// Create buttons - Hackfest time
	LinearLayout ll = new LinearLayout(this);

	Button daMiss = new Button(this);
	daMiss.setText("Dismiss");
	ll.addView(daMiss);
	//ll.setGravity(Gravity.CENTER_HORIZONTAL | Gravity.CENTER_VERTICAL);
	ll.setGravity(Gravity.BOTTOM | Gravity.LEFT);
	this.addContentView(ll, new LayoutParams(LayoutParams.FILL_PARENT, LayoutParams.FILL_PARENT));

	// set listener for it
	daMiss.setOnTouchListener(new OnTouchListener() {

            @Override
            public boolean onTouch(View v, MotionEvent event) {
		Log.w(TAG,"quit button onTouched");

        	final int action = event.getAction();
		Log.w(TAG,"action "+ action + "masked action "+ MotionEvent.ACTION_MASK);
		if (action == MotionEvent.ACTION_DOWN) {
			finish();
		}
		return true;
		}
        });

	Button nextVP = new Button(this);
	nextVP.setText("ViewPoint");
	ll.addView(nextVP);

	// set listener for it
	nextVP.setOnTouchListener(new OnTouchListener() {

            @Override
            public boolean onTouch(View v, MotionEvent event) {
		Log.w(TAG,"quit button onTouched");

        	final int action = event.getAction();
		if (action == MotionEvent.ACTION_DOWN) FreeWRLLib.nextViewpoint();
		return true;
            }
        });

	// finish Create buttons - Hackfest time
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
