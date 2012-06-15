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
import android.view.WindowManager;

import java.io.IOException;

//import android.content.res.AssetManager;
//import android.content.res.AssetFileDescriptor;
import java.io.FileDescriptor;
import android.content.res.Resources;

import android.content.Context;


import java.io.File;

public class FreeWRLAssetData {
	Integer offset;
	Integer length;
	FileDescriptor fd;

	public FreeWRLAssetData (Integer of, Integer len, FileDescriptor fd) {
		this.offset = of;
		this.length = len;
		this.fd = fd;
	}
}
