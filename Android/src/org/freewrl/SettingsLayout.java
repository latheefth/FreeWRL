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

import android.content.Context;
import android.widget.LinearLayout;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.TextView;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.util.AttributeSet;
import android.view.View;
import android.view.LayoutInflater;
import android.util.Log;

import java.io.File;
import java.util.List;
import java.util.ArrayList;
import android.widget.Button;
import android.widget.RadioButton;

   public class SettingsLayout extends LinearLayout {

    Context context;
    private List<String> item = null;
    private List<String> path = null;
    //private TextView myVersion;
    //private TextView myBuildDate;
    //private TextView lstView;
    private Button CancelButton;
    private RadioButton headlightOn;
    private RadioButton headlightOff;

    private View newSettingsView;

    private static String TAG = "FreeWRLSettingsLayout";

    public SettingsLayout(Context context, AttributeSet attrs) {
        super(context, attrs);

        this.context = context;


        LayoutInflater layoutInflater = (LayoutInflater) context
                .getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        newSettingsView = layoutInflater.inflate(R.layout.settingsview, this);

	headlightOn = (RadioButton) findViewById(R.id.headlightOn);
	headlightOn.setOnClickListener(new View.OnClickListener() {
             public void onClick(View v) {
    		boolean checked = ((RadioButton) v).isChecked();
		Log.w(TAG,"onheadlightOnClicked, id " + v.getId() + " value " + checked);
             }
         });

	headlightOff = (RadioButton) findViewById(R.id.headlightOff);
	headlightOff.setOnClickListener(new View.OnClickListener() {
             public void onClick(View v) {
    		boolean checked = ((RadioButton) v).isChecked();
		Log.w(TAG,"onheadlightOffClicked, id " + v.getId() + " value " + checked);
             }
         });

	CancelButton = (Button) findViewById(R.id.CancelButton);
	CancelButton.setOnClickListener(new View.OnClickListener() {
             public void onClick(View v) {
                 // clicked dismiss; back to main window.
		FreeWRLActivity.popBackToMainView();
             }
         });
    }


}

