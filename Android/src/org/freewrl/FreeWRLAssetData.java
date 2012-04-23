package org.freewrl;

import android.app.Activity;
import android.os.Bundle;
import android.util.Log;
import android.view.WindowManager;

import java.io.IOException;

import android.content.res.AssetManager;
import android.content.res.AssetFileDescriptor;
import android.content.res.Resources;

import android.content.Context;


import java.io.File;

public class FreeWRLAssetData {
	Integer offset;
	Integer length;
	AssetFileDescriptor ad;

	public FreeWRLAssetData (Integer of, Integer len, AssetFileDescriptor ad) {
		this.offset = of;
		this.length = len;
		this.ad = ad;
	}
}
