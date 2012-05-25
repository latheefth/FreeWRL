package org.freewrl;

import android.content.Context;
import android.widget.LinearLayout;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.TextView;
import android.widget.ListView;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.util.AttributeSet;
import android.view.View;
import android.view.LayoutInflater;
import android.util.Log;

import java.io.File;
import java.util.List;
import java.util.ArrayList;

   public class FolderLayout extends LinearLayout implements OnItemClickListener {

    Context context;
    IFolderItemListener folderListener;
    private List<String> item = null;
    private List<String> path = null;
    private String root = "/";
    private TextView myPath;
    private ListView lstView;

    private static String TAG = "FreeWRLView";

    public FolderLayout(Context context, AttributeSet attrs) {
        super(context, attrs);

        // TODO Auto-generated constructor stub
        this.context = context;


        LayoutInflater layoutInflater = (LayoutInflater) context
                .getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        View view = layoutInflater.inflate(R.layout.folderview, this);

Log.w (TAG,"layoutInflater parent is " + view.getParent());

        myPath = (TextView) findViewById(R.id.path);
        lstView = (ListView) findViewById(R.id.list);
Log.w(TAG,"R.id.path " + myPath + " R.id.list " + lstView);

        Log.w(TAG, "Constructed");
        getDir(root, lstView);

    }

    public void setIFolderItemListener(IFolderItemListener folderItemListener) {
        this.folderListener = folderItemListener;
    }

    //Set Directory for view at anytime
    public void setDir(String dirPath){
        getDir(dirPath, lstView);
    }


    private void getDir(String dirPath, ListView v) {

        myPath.setText("Location: " + dirPath);
        item = new ArrayList<String>();
        path = new ArrayList<String>();
        File f = new File(dirPath);
        File[] files = f.listFiles();

        if (!dirPath.equals(root)) {

            item.add(root);
            path.add(root);
            item.add("../");
            path.add(f.getParent());

        }
        for (int i = 0; i < files.length; i++) {
            File file = files[i];
            path.add(file.getPath());
            if (file.isDirectory())
                item.add(file.getName() + "/");
            else
                item.add(file.getName());

        }

        Log.w(TAG, "local folders getDir returns " + files.length + "");

        setItemList(item);

    }

    //can manually set Item to display, if u want
    public void setItemList(List<String> item){
        ArrayAdapter<String> fileList = new ArrayAdapter<String>(context,
                R.layout.row, item);

        lstView.setAdapter(fileList);
        lstView.setOnItemClickListener(this);
    }


    public void onListItemClick(ListView l, View v, int position, long id) {
        File file = new File(path.get(position));
Log.w(TAG,"onListItemClick, listView " + l + " view " + v + " position " + position + " id " + id);

        if (file.isDirectory()) {
	Log.w(TAG,"onListItemClick, directory");
            if (file.canRead())
                getDir(path.get(position), l);
            else {
//what to do when folder is unreadable
                if (folderListener != null) {
                    folderListener.OnCannotFileRead(file);

                }

            }
        } else {

//what to do when file is clicked
//You can add more,like checking extension,and performing separate actions
	Log.w(TAG,"onListItemClick, NOT directory");
            if (folderListener != null) {
		Log.w(TAG,"onListItemClick, NOT dir + folderListener not null");
                folderListener.OnFileClicked(file);
            }

        }
    }

    public void onItemClick(AdapterView<?> arg0, View arg1, int arg2, long arg3) {
        // TODO Auto-generated method stub
        onListItemClick((ListView) arg0, arg0, arg2, arg3);
    }

}

