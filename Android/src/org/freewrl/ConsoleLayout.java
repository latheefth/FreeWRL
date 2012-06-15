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

// for setContentView
//import android.app.Activity;
//import android.os.Bundle;




   public class ConsoleLayout extends LinearLayout {

    Context context;
    //JAS IFolderItemListener folderListener;
    private List<String> item = null;
    private List<String> path = null;
    private TextView myVersion;
    private TextView lstView;
    private Button CancelButton;

    private View newFileView;

    private static String TAG = "FreeWRLView";

    public ConsoleLayout(Context context, AttributeSet attrs) {
        super(context, attrs);

        this.context = context;


        LayoutInflater layoutInflater = (LayoutInflater) context
                .getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        newFileView = layoutInflater.inflate(R.layout.consoleview, this);

        myVersion = (TextView) findViewById(R.id.path);
        lstView = (TextView) findViewById(R.id.list);
	CancelButton = (Button) findViewById(R.id.CancelButton);
	CancelButton.setOnClickListener(new View.OnClickListener() {
             public void onClick(View v) {
                 // Perform action on click
		newFileView.setVisibility(View.GONE);
             }
         });

        Log.w(TAG, "Constructed");
        //setList(root, lstView);

    }

    //Set Directory for view at anytime
    public void setConsoleListing(String myVers, String consoleText){
        setList(myVers, consoleText, lstView);
    }


    private void setList(String myVers, String consoleText, TextView v) {

        myVersion.setText("FreeWRL  Version " + myVers + "\n");
	lstView.setText("Console Text:\n" + consoleText);
    }

/*
    //can manually set Item to display, if u want
    public void setItemList(List<String> item){
        ArrayAdapter<String> fileList = new ArrayAdapter<String>(context,
                R.layout.row, item);

        //JAS lstView.setAdapter(fileList);
    }
*/
}

