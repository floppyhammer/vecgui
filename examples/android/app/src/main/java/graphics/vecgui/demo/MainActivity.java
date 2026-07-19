package graphics.vecgui.demo;

import com.google.androidgamesdk.GameActivity;

import android.os.Bundle;
import android.view.View;

public class MainActivity extends GameActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        // Hide navigation bar.
        getWindow().getDecorView().setSystemUiVisibility(View.SYSTEM_UI_FLAG_HIDE_NAVIGATION);
    }

    static {
        System.loadLibrary("vecgui_android_demo");
    }
}
