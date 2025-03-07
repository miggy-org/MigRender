package com.jordan.textrender.service;

import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.Color;
import android.os.IBinder;
import android.util.Log;
import android.widget.ImageView;
import android.widget.Toast;

import androidx.annotation.Nullable;
import androidx.core.app.NotificationCompat;

import com.jordan.textrender.MiggyConst;
import com.jordan.textrender.MiggyInterface;
import com.jordan.textrender.MiggyRender;
import com.jordan.textrender.Options;
import com.jordan.textrender.R;
import com.jordan.textrender.activity.RenderActivity;
import com.jordan.textrender.activity.RenderFragment;
import com.jordan.textrender.json.BackgroundConfig;
import com.jordan.textrender.json.RenderConfig;
import com.jordan.textrender.json.SettingsConfig;

public class RenderService extends Service implements MiggyInterface.IMiggyCallback {
    public interface IRenderUpdate {
        void onUpdate(boolean isComplete);
    }

    public static class NotificationBroadcastReceiver extends BroadcastReceiver {
        @Override
        public void onReceive(Context context, Intent intent) {
            signalAbortRender();
        }
    }

    private static final String NOTIFICATION_CHANNEL_ID = "textrender_01";
    private static final int NOTIFICATION_IN_PROGRESS_ID = 1237;
    private static final int NOTIFICATION_COMPLETE_ID = 1238;
    NotificationCompat.Builder builder;

    private static IRenderUpdate updateHandler = null;
    public static void setUpdateHandler(Context context, IRenderUpdate handler) {
        updateHandler = handler;

        if (updateHandler != null) {
            NotificationManager notificationManager =
                    (NotificationManager) context.getSystemService(Context.NOTIFICATION_SERVICE);
            notificationManager.cancel(NOTIFICATION_COMPLETE_ID);
        }
    }

    private static Bitmap theBmp = null;
    private static Class<?> callingActivityClass = null;

    private static void initRenderBitmap(int width, int height) {
        theBmp = Bitmap.createBitmap(width, height, Bitmap.Config.ARGB_8888);
    }

    private static void clearRenderBitmap() {
        theBmp = null;
    }

    public static Bitmap getRenderBitmap() {
        return theBmp;
    }

    public static boolean isRendering() {
        return MiggyRender.getInstance().isRendering();
    }

    public static void signalAbortRender()	{
        MiggyRender.getInstance().signalAbortRender(null);
    }

    public static void startRender(Context context, SettingsConfig settings, RenderConfig config, BackgroundConfig background) {
        callingActivityClass = context.getClass();

        if (!MiggyRender.getInstance().isRendering()) {
            if (MiggyRender.getInstance().initRender(settings)) {
                Options.Resolution resolution = MiggyConst.resolutionOptions.getOption(config.resolutionId);
                initRenderBitmap(resolution.getWidth(), resolution.getHeight());

                if (!MiggyRender.getInstance().setBackground(background)) {
                    //tv.setText("SetBackground() failed");
                    return;
                }

                if (!MiggyRender.getInstance().setEnvironment(context, config)) {
                    //tv.setText("SetText() failed");
                    return;
                }

                Intent intent = new Intent(context, RenderService.class);
                context.startForegroundService(intent);
            }
        } else {
            signalAbortRender();
        }
    }

    @Override
    public void onCreate() {
        createNotificationChannel();
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        if (MiggyRender.getInstance().startRender(theBmp, this)) {
            createInProgressNotification();
        }

        return START_NOT_STICKY;
    }

    @Nullable
    @Override
    public IBinder onBind(Intent intent) {
        // not supported
        return null;
    }

    @Override
    public void onDestroy() {
        callingActivityClass = null;
    }

    private void createNotificationChannel() {
        NotificationManager notificationManager = (NotificationManager) getSystemService(Context.NOTIFICATION_SERVICE);
        NotificationChannel notificationChannel =
                new NotificationChannel(NOTIFICATION_CHANNEL_ID, getString(R.string.notify_channel_name), NotificationManager.IMPORTANCE_LOW);
        notificationChannel.setDescription(getString(R.string.notify_channel_desc));
        notificationManager.createNotificationChannel(notificationChannel);
    }

    private void createInProgressNotification() {
        PendingIntent pendingIntent =
                PendingIntent.getActivity(this, 0,
                        new Intent(this, callingActivityClass),
                        PendingIntent.FLAG_IMMUTABLE);

        PendingIntent stopIntent =
                PendingIntent.getBroadcast(this, 0,
                        new Intent(this, NotificationBroadcastReceiver.class),
                        PendingIntent.FLAG_IMMUTABLE);

        builder = new NotificationCompat.Builder(this, NOTIFICATION_CHANNEL_ID)
                        .setOngoing(true)
                        .setContentTitle(getString(R.string.render_in_progress))
                        .setContentText(getString(R.string.render_notify_content))
                        .setContentIntent(pendingIntent)
                        .setSmallIcon(R.drawable.ic_panorama)
                        .setProgress(100, 0, false)
                        .addAction(android.R.drawable.ic_menu_close_clear_cancel,
                                getString(R.string.cancel), stopIntent)
                        .setTicker(getString(R.string.notify_ticker_in_progress));
        startForeground(NOTIFICATION_IN_PROGRESS_ID, builder.build());
    }

    private void createCompleteNotification() {
        if (updateHandler == null) {
            PendingIntent pendingIntent =
                    PendingIntent.getActivity(this, 0,
                            new Intent(this, callingActivityClass),
                            PendingIntent.FLAG_IMMUTABLE);

            Notification notification =
                    new NotificationCompat.Builder(this, NOTIFICATION_CHANNEL_ID)
                            .setContentTitle(getString(R.string.render_complete))
                            .setContentText(getString(R.string.render_notify_content))
                            .setContentIntent(pendingIntent)
                            .setSmallIcon(R.drawable.ic_panorama)
                            .setTicker(getString(R.string.notify_ticker_complete))
                            .setWhen(System.currentTimeMillis())
                            .setAutoCancel(true)
                            .build();
            NotificationManager notificationManager = (NotificationManager) getSystemService(Context.NOTIFICATION_SERVICE);
            notificationManager.notify(NOTIFICATION_COMPLETE_ID, notification);
        }
    }

    @Override
    public boolean miggyRenderStarted() {
        return true;
    }

    @Override
    public boolean miggyRenderUpdate(int percent) {
        if (updateHandler != null) {
            updateHandler.onUpdate(false);
        }

        builder.setProgress(100, percent, false);
        NotificationManager notificationManager = (NotificationManager) getSystemService(Context.NOTIFICATION_SERVICE);
        notificationManager.notify(NOTIFICATION_IN_PROGRESS_ID, builder.build());
        return true;
    }

    @Override
    public void miggyRenderComplete() {
        if (updateHandler != null) {
            updateHandler.onUpdate(true);
        }

        createCompleteNotification();

        stopSelf();
    }

    @Override
    public void miggyRenderAborted() {
        if (updateHandler != null) {
            updateHandler.onUpdate(true);
        }

        stopSelf();
    }
}
