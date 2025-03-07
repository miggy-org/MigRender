package com.jordan.textrender;

import android.app.Activity;
import android.content.Context;
import android.content.ContextWrapper;
import android.graphics.Bitmap;
import android.util.AttributeSet;
import android.util.Log;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.jordan.textrender.json.RenderConfig;
import com.jordan.textrender.json.SettingsConfig;

import java.util.concurrent.Callable;

public class Preview extends androidx.appcompat.widget.AppCompatImageView implements MiggyInterface.IMiggyCallback {
    protected MiggyRender render;

    private Bitmap bmp;
    private Callable<Boolean> queuedSetupFunc;
    private MiggyInterface.IMiggyCallback callback;

    public Preview(@NonNull Context context) {
        super(context);
        render = MiggyRender.getInstance();
    }

    public Preview(@NonNull Context context, @Nullable AttributeSet attrs) {
        super(context, attrs);
        render = MiggyRender.getInstance();
    }

    public Preview(@NonNull Context context, @Nullable AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        render = MiggyRender.getInstance();
    }

    public void abortPreview() {
        render.signalAbortRender(this);
    }

    protected void init(int width, int height, MiggyInterface.IMiggyCallback callback) {
        this.callback = callback;

        bmp = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888);

        SettingsConfig config = SettingsConfig.loadFromPrefs(getActivity(), new SettingsConfig());
        render.initRender(false, false, false, false, config.multiCore);
    }

    protected void init(RenderConfig renderConfig, MiggyInterface.IMiggyCallback callback) {
        Options.Resolution resolution = MiggyConst.resolutionOptions.getOption(renderConfig.resolutionId);

        // generate a preview dimension based upon the aspect ration of the chosen resolution
        int width = 320;
        int height = (resolution.getWidth() / (double) resolution.getHeight() > 1.5 ? 180 : 240);
        init(width, height, callback);
    }

    protected boolean queueRender(Callable<Boolean> setupFunc) {
        if (!render.isRendering()) {
            boolean proceed = false;
            try {
                proceed = setupFunc.call();
            } catch (Exception e) {
                Log.e("", "Caught exception: ", e);
            }

            if (proceed) {
                return render.startRender(bmp, this);
            }
        } else {
            queuedSetupFunc = setupFunc;
            render.signalAbortRender(this);
        }
        return false;
    }

    private Activity getActivity() {
        Context context = getContext();
        while (context instanceof ContextWrapper) {
            if (context instanceof Activity) {
                return (Activity)context;
            }
            context = ((ContextWrapper)context).getBaseContext();
        }
        return null;
    }

    @Override
    public boolean miggyRenderStarted() {
        return (callback == null || callback.miggyRenderStarted());
    }

    @Override
    public boolean miggyRenderUpdate(int percent) {
        return (callback == null || callback.miggyRenderUpdate(percent));
    }

    private void callQueueSetupFunc(Activity act) {
        if (queuedSetupFunc != null) {
            try {
                Callable<Boolean> localSetupFunc = queuedSetupFunc;
                queuedSetupFunc = null;

                if (act != null) {
                    act.runOnUiThread(() -> queueRender(localSetupFunc));
                }
            } catch (Exception e) {
                Log.e("", "Caught exception: ", e);
            }
        }
    }

    @Override
    public void miggyRenderComplete() {
        if (callback != null) {
            callback.miggyRenderComplete();
        }

        final Activity act = getActivity();
        if (act != null) {
            act.runOnUiThread(() -> setImageBitmap(bmp));
        }

        callQueueSetupFunc(act);
    }

    @Override
    public void miggyRenderAborted() {
        if (callback != null) {
            callback.miggyRenderAborted();
        }

        final Activity act = getActivity();
        callQueueSetupFunc(act);
    }
}
