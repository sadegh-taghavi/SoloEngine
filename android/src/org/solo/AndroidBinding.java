package org.solo;

import android.os.Bundle;
import android.app.Activity;
import android.util.Log;
import android.content.pm.PackageManager;
import android.content.pm.ComponentInfo;
import android.content.pm.ActivityInfo;


public class AndroidBinding extends Activity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        try {
            ActivityInfo ai = getPackageManager()
                .getActivityInfo(this.getComponentName(), PackageManager.GET_META_DATA);
            String libName = null;
            if ( ai.metaData.containsKey("android.app.lib_name") )
                libName = ai.metaData.getString("android.app.lib_name");
            System.loadLibrary(libName);
        } catch (Exception e) {

        }
    }
}
